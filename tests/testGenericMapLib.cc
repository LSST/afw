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

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/shared_ptr.h>
#include <memory>
#include <sstream>

#include "lsst/pex/exceptions.h"

#include "lsst/afw/typehandling/GenericMap.h"
#include "lsst/afw/typehandling/SimpleGenericMap.h"
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/typehandling/python.h"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace std::string_literals;

namespace lsst {
namespace afw {
namespace typehandling {

namespace {

/// A Storable with simple, mutable state.
class CppStorable : public Storable {
public:
    explicit CppStorable(std::string const& value) : Storable(), value(value) {}

    CppStorable(CppStorable const&) = default;
    CppStorable(CppStorable&&) = default;
    CppStorable& operator=(CppStorable const&) = default;
    CppStorable& operator=(CppStorable&&) = default;
    ~CppStorable() noexcept override = default;

    /**
     * Two CppStorables are equal if and only if their internal states are equal.
     *
     * @{
     */
    bool operator==(CppStorable const& other) const noexcept { return value == other.value; }
    bool operator!=(CppStorable const& other) const noexcept { return !(*this == other); }

    /** @} */

    /// Assign a new value to this object
    void reset(std::string const& value) { this->value = value; }
    /// Retrieve the value in this object
    std::string const& get() const noexcept { return value; }

    // Storable methods
    std::shared_ptr<Storable> cloneStorable() const override { return std::make_shared<CppStorable>(*this); }
    std::string toString() const override { return value; }
    bool equals(Storable const& other) const noexcept override { return singleClassEquals(*this, other); }

private:
    std::string value;
};

/**
 * Test whether a map contains a strongly typed key.
 *
 * @param map The map to test
 * @param key The key to test.
 *
 * @throws pex::exceptions::NotFoundError Thrown if the key is not present in
 *      the map.
 *
 * @{
 */
template <typename T>
void _assertKey(GenericMap<std::string> const& map, Key<std::string, T> const& key) {
    using lsst::pex::exceptions::NotFoundError;

    if (!map.contains(key.getId())) {
        throw LSST_EXCEPT(NotFoundError, "Map does not contain key " + key.getId());
    }

    if (!map.contains(key)) {
        std::stringstream buffer;
        buffer << "Map maps " << key.getId() << " to a different type than " << key;
        throw LSST_EXCEPT(NotFoundError, buffer.str());
    }
}

/**
 * Test whether a map contains a key-value pair.
 *
 * @param map The map to test
 * @param key, value The key-value pair to test.
 *
 * @throws pex::exceptions::NotFoundError Thrown if the key is not present in
 *      the map, or maps to a different value.
 *
 * @{
 */
template <typename T>
void assertKeyValue(GenericMap<std::string> const& map, std::string const& key, T const& value) {
    using lsst::pex::exceptions::NotFoundError;

    auto typedKey = makeKey<T>(key);
    _assertKey(map, typedKey);

    T const& mapValue = map.at(typedKey);
    if (mapValue != value) {
        std::stringstream buffer;
        buffer << "Map maps " << typedKey << " to " << mapValue << ", expected " << value;
        throw LSST_EXCEPT(NotFoundError, buffer.str());
    }
}

// This specialization may be called for values that are *either* Storable or shared_ptr<Storable>,
// because there is no way to distinguish them from Python
template <>
void assertKeyValue(GenericMap<std::string> const& map, std::string const& key, Storable const& value) {
    using lsst::pex::exceptions::NotFoundError;

    // Non-owning pointer; can't use reference because we can't initialize it yet
    Storable const* mapValue = nullptr;
    try {
        auto typedKey = makeKey<Storable>(key);
        _assertKey(map, typedKey);
        mapValue = &(map.at(typedKey));
    } catch (NotFoundError const&) {
        auto typedKey = makeKey<std::shared_ptr<Storable const>>(key);
        _assertKey(map, typedKey);
        mapValue = map.at(typedKey).get();
    }

    if (!mapValue) {
        std::stringstream buffer;
        buffer << "Map maps " << key << " to null pointer, expected " << value;
        throw LSST_EXCEPT(NotFoundError, buffer.str());
    }

    if (!mapValue->equals(value)) {
        std::stringstream buffer;
        buffer << "Map maps " << key << " to " << *mapValue << ", expected " << value;
        throw LSST_EXCEPT(NotFoundError, buffer.str());
    }
}

/** @} */

/**
 * Test whether a CppStorable contains a specific value.
 *
 * @param storable the storable to test
 * @param value the expected internal string
 */
void assertCppValue(CppStorable const& storable, std::string const& value) {
    using lsst::pex::exceptions::RuntimeError;

    if (storable.get() != value) {
        std::stringstream buffer;
        buffer << "CppStorable contains " << storable << ", expected " << value;
        throw LSST_EXCEPT(RuntimeError, buffer.str());
    }
}

/**
 * Test whether a Storable defined in Python can be correctly retrieved from a GenericMap.
 *
 * @param map The map to test.
 * @param key The key to look up. Assumed to map to a shared_ptr<Storable>.
 * @param repr The expected string representation of the value.
 */
void assertPythonStorable(GenericMap<std::string> const& map, std::string const& key,
                          std::string const& repr) {
    auto const obj = map.at(makeKey<std::shared_ptr<Storable const>>(key));

    std::stringstream stringForm;
    stringForm << *obj;
    if (stringForm.str() != repr) {
        std::stringstream buffer;
        buffer << "Map maps " << key << " to " << *obj << ", expected " << repr;
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeError, buffer.str());
    }
}

/**
 * Test whether a Storable defined in Python can be correctly read.
 *
 * @param storable the Storable, of a Python-only implementation class.
 * @param repr The expected string representation of the value.
 */
void assertPythonStorable(Storable const& storable, std::string const& repr) {
    std::stringstream stringForm;
    stringForm << storable;
    if (stringForm.str() != repr) {
        std::stringstream buffer;
        buffer << "Given " << storable << ", expected " << repr;
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeError, buffer.str());
    }
}

/**
 * Create a MutableGenericMap that can be passed to Python for testing.
 *
 * @returns a map containing the state `{"one": 1, "pi": 3.1415927,
 *          "string": "neither a number nor NaN"}`. This state is hardcoded
 *          into the Python test code, and should be changed with caution.
 */
std::shared_ptr<MutableGenericMap<std::string>> makeInitialMap() {
    auto map = std::make_unique<SimpleGenericMap<std::string>>();
    // TODO: workaround for DM-21268
    map->insert("one", std::int64_t(1));
    map->insert("pi", 3.1415927);
    // TODO: workaround for DM-21216
    map->insert("string", "neither a number nor NaN"s);
    return map;
}

/**
 * Change the values in a GenericMap.
 *
 * @param testmap the map to update. Assumed to be in the state created by
 *                makeInitialMap.
 *
 * This function performs changes equivalent to the following Python:
 *
 *     testmap['answer'] = 42
 *     testmap['pi'] = 3.0
 *     testmap['string'] = False
 *
 * This difference is hardcoded into the Python test code, and should be
 * changed with caution.
 */
void makeCppUpdates(MutableGenericMap<std::string>& testmap) {
    // TODO: workaround for DM-21268
    testmap.insert("answer", std::int64_t(42));

    testmap.at(makeKey<double>("pi"s)) = 3.0;

    testmap.erase(makeKey<std::string>("string"s));
    testmap.insert("string", false);
}

/**
 * Insert two Storable objects into a GenericMap.
 *
 * @param testmap The map object to update.
 *
 * This function adds two extra key-value pairs:
 *
 * * `"cppValue" -> CppStorable("value")`
 * * `"cppPointer" -> shared_ptr(CppStorable("pointer"))`
 *
 * These pairs are hardcoded into the Python test code, and should be changed
 * with caution.
 */
void addCppStorable(MutableGenericMap<std::string>& testmap) {
    testmap.insert("cppValue", CppStorable("value"));
    testmap.insert("cppPointer", std::make_shared<CppStorable>("pointer"));
}

/**
 * Store and retrieve a Storable in C++.
 *
 * To avoid cluttering testGenericMapLib with a special holder class, this
 * function conflates two actions. When called with a Storable, it puts a
 * pointer to it in internal storage. When called with no arguments, it
 * returns the pointer it has been keeping.
 *
 * @param storable The Storable to assign to the pointer [optional].
 * @returns The currently held pointer.
 */
std::shared_ptr<Storable> keepStaticStorable(std::shared_ptr<Storable> storable = nullptr) {
    static std::shared_ptr<Storable> longLived = nullptr;
    if (storable) {
        longLived = storable;
    }
    return longLived;
}

/**
 * Copy a Storable in C++.
 *
 * @param input The Storable to copy.
 * @returns An identical but independent Storable.
 */
std::shared_ptr<Storable> duplicate(std::shared_ptr<Storable> input) { return input->cloneStorable(); }

}  // namespace

namespace {

// Functions for working with values of arbitrary type
template <typename T>
void declareAnyTypeFunctions(nb::module_& mod) {
    mod.def("assertKeyValue",
            static_cast<void (*)(GenericMap<std::string> const&, std::string const&, T const&)>(
                    &assertKeyValue),
            "map"_a, "key"_a, "value"_a);
}

}  // namespace

NB_MODULE(testGenericMapLib, mod) {

    nb::module_::import_("lsst.afw.typehandling");

    declareAnyTypeFunctions<bool>(mod);
    declareAnyTypeFunctions<std::int64_t>(mod);
    declareAnyTypeFunctions<double>(mod);
    declareAnyTypeFunctions<std::string>(mod);
    declareAnyTypeFunctions<Storable>(mod);
    mod.def("assertCppValue", &assertCppValue, "storable"_a, "value"_a);
    mod.def("assertPythonStorable",
            nb::overload_cast<Storable const&, std::string const&>(&assertPythonStorable), "storable"_a,
            "repr"_a);
    mod.def("assertPythonStorable",
            nb::overload_cast<GenericMap<std::string> const&, std::string const&, std::string const&>(
                    &assertPythonStorable),
            "testmap"_a, "key"_a, "repr"_a);

    mod.def("makeInitialMap", &makeInitialMap);
    mod.def("makeCppUpdates", &makeCppUpdates, "testmap"_a);
    mod.def("addCppStorable", &addCppStorable, "testmap"_a);
    mod.def("keepStaticStorable", &keepStaticStorable, "storable"_a = nullptr);
    mod.def("duplicate", &duplicate, "input"_a);

    nb::class_<CppStorable, Storable, StorableHelper<CppStorable>> cls(
            mod, "CppStorable");
    cls.def(nb::init<std::string>());
    cls.def("__eq__", &CppStorable::operator==, nb::is_operator());
    cls.def("__ne__", &CppStorable::operator!=, nb::is_operator());
    cls.def_prop_rw("value", &CppStorable::get, &CppStorable::reset);
    cls.def("__str__", &CppStorable::toString);
    cls.def("__repr__", &CppStorable::toString);
}

}  // namespace typehandling
}  // namespace afw
}  // namespace lsst
