/*
 * This file is part of afw.
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/shared_ptr.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/variant.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <utility>
#include <variant>
#include <nanobind/make_iterator.h>

#include "lsst/cpputils/python.h"
#include "lsst/pex/exceptions.h"

#include "lsst/afw/typehandling/GenericMap.h"
#include "lsst/afw/typehandling/python.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace lsst {
namespace afw {
namespace typehandling {

namespace {

// Type safety pointless in Python, use unsafe methods to avoid manual type checking
// https://nanobind.readthedocs.io/en/stable/advanced/classes.html#binding-protected-member-functions
template <typename K>
class Publicist : public MutableGenericMap<K> {
public:
    using typename GenericMap<K>::ConstValueReference;
    using typename GenericMap<K>::StorableType;
    using GenericMap<K>::unsafeLookup;
    using MutableGenericMap<K>::unsafeErase;
};

// A combination std::variant visitor and helper function (apply()) that
// invokes it, to implement item lookup for Python.
//
// We copy/clone because there's no good way to prevent other ++ calls from
// destroying the any reference we might give to Python.
//
// Pybind11 does have its own automatic caster for std::variant, but it doesn't
// handle reference_wrapper types within it (let alone polymorphic Storables),
// and since we need a visitor to deal with that, there's no point in going
// back to std::variant instead of straight to Python.
template <typename K>
class Getter {
public:

    template <typename T>
    nb::object operator()(std::reference_wrapper<T const> const & value) const {
        T copy(value);
        return nb::cast(copy);
    }

    nb::object operator()(std::reference_wrapper<PolymorphicValue const> const & value) const {
        // Specialization for polymorphic Storables: extract the Storable,
        // clone it explicitly, and return that (letting nanobind's downcasting
        // take over from there).
        Storable const& storable = value.get();
        return nb::cast(storable.cloneStorable());
    }

    nb::object apply(GenericMap<K>& self, K const& key) const {
        auto callable = static_cast<typename Publicist<K>::ConstValueReference (GenericMap<K>::*)(K) const>(
            &Publicist<K>::unsafeLookup);
        auto variant = (self.*callable)(key);
        return std::visit(*this, variant);
    }

};

template <typename K>
void declareGenericMap(cpputils::python::WrapperCollection& wrappers, std::string const& suffix,
                       std::string const& key) {
    using Class = GenericMap<K>;
    using PyClass = nb::class_<Class>;

    std::string className = "GenericMap" + suffix;
    // Give the class a custom docstring to avoid confusing Python users
    std::string docstring =
            "An abstract `~collections.abc.Mapping` for use when sharing a map between C++ and Python.\n" +
            declareGenericMapRestrictions(className, key);
    wrappers.wrapType(PyClass(wrappers.module, className.c_str(), docstring.c_str()), [](auto& mod,
                                                                                         auto& cls) {
        // __str__ easier to implement in Python
        // __repr__ easier to implement in Python
        // __eq__ easier to implement in Python
        // __ne__ easier to implement in Python
        // For unknown reasons, overload_cast doesn't work
        // cls.def("__contains__", nb::overload_cast<K const&>(&Class::contains, nb::const_), "key"_a);
        cls.def("__contains__", static_cast<bool (Class::*)(K const&) const>(&Class::contains), "key"_a);

        cls.def("__getitem__",
                [](Class& self, K const& key) {
                    try {
                        return Getter<K>().apply(self, key);
                    } catch (pex::exceptions::OutOfRangeError const& e) {
                        // nanobind doesn't seem to recognize chained exceptions
                        std::stringstream buffer;
                        buffer << "Unknown key: " << key;
                        std::throw_with_nested(nb::key_error(buffer.str().c_str()));
                    }
                },
                "key"_a);
        cls.def("get",
                [](Class& self, K const& key, nb::object const& def) {
                    try {
                        return Getter<K>().apply(self, key);
                    } catch (pex::exceptions::OutOfRangeError const& e) {
                        return def;
                    }
                },
                // Prevent segfaults when assigning a key<Storable> to Python variable, then deleting from map
                // No existing code depends on being able to modify an item stored by value
                "key"_a, "default"_a = nb::none(), nb::rv_policy::copy);
        cls.def("__iter__",
                [](Class const& self) { return nb::make_iterator(nb::type<PyClass>(), "iterator", self.keys().begin(), self.keys().end()); },
                nb::keep_alive<0, 1>());
        cls.def("__len__", &Class::size);
        cls.def("__bool__", [](Class const& self) { return !self.empty(); });
        // Can't wrap keys directly because nanobind always copies vectors, so it won't be a view
        // items easier to implement in Python
        // values easier to implement in Python
    });
}

template <typename V, class PyClass>
void declareMutableGenericMapTypedMethods(PyClass& cls) {
    using Class = typename PyClass::Type;
    cls.def("__setitem__",
            [](Class& self, typename Class::key_type const& key, V const& value) {
                // Need to delete previous key, which may not be of type V
                // TODO: this method provides only basic exception safety
                if (self.contains(key)) {
                    auto callable = &Publicist<typename Class::key_type>::unsafeErase;
                    (self.*callable)(key);
                }
                self.insert(key, value);
            },
            "key"_a, "value"_a=nb::none());
    // setdefault easier to implement in Python
    // pop easier to implement in Python
}

template <typename K>
void declareMutableGenericMap(cpputils::python::WrapperCollection& wrappers, std::string const& suffix,
                              std::string const& key) {
    using Class = MutableGenericMap<K>;
    using PyClass = nb::class_<Class, GenericMap<K>>;

    std::string className = "MutableGenericMap" + suffix;
    // Give the class a custom docstring to avoid confusing Python users
    std::string docstring =
            "An abstract `~collections.abc.MutableMapping` for use when sharing a map between C++ and "
            "Python.\n" +
            declareGenericMapRestrictions(className, key);
    wrappers.wrapType(PyClass(wrappers.module, className.c_str(), docstring.c_str()),
                      [](auto& mod, auto& cls) {
                          // Don't rewrap members of GenericMap
                          declareMutableGenericMapTypedMethods<std::shared_ptr<Storable const>>(cls);
                          declareMutableGenericMapTypedMethods<bool>(cls);
                          // TODO: int32 and float are suppressed for now, see DM-21268
                          declareMutableGenericMapTypedMethods<std::int64_t>(cls);  // chosen for builtins.int
                          declareMutableGenericMapTypedMethods<std::int32_t>(cls);
                          declareMutableGenericMapTypedMethods<double>(cls);  // chosen for builtins.float
                          declareMutableGenericMapTypedMethods<float>(cls);
                          declareMutableGenericMapTypedMethods<std::string>(cls);
                          cls.def("__delitem__",
                                  [](Class& self, K const& key) {
                                      if (self.contains(key)) {
                                          auto callable = &Publicist<K>::unsafeErase;
                                          (self.*callable)(key);
                                      } else {
                                          std::stringstream buffer;
                                          buffer << "Unknown key: " << key;
                                          throw nb::key_error(buffer.str().c_str());
                                      }
                                  },
                                  "key"_a);
                          cls.def("popitem", [](Class& self) {
                              if (!self.empty()) {
                                  K key = self.keys().back();
                                  auto result = std::make_pair(key, Getter<K>().apply(self, key));
                                  auto callable = &Publicist<K>::unsafeErase;
                                  (self.*callable)(key);
                                  return result;
                              } else {
                                  throw nb::key_error("Cannot pop from empty GenericMap.");
                              }
                          });
                          cls.def("clear", &Class::clear);
                          // cls.def("update",, "other"_a);
                      });
}
}  // namespace

void wrapGenericMap(cpputils::python::WrapperCollection& wrappers) {
    declareGenericMap<std::string>(wrappers, "S", "strings");
    declareMutableGenericMap<std::string>(wrappers, "S", "strings");
}

}  // namespace typehandling
}  // namespace afw
}  // namespace lsst
