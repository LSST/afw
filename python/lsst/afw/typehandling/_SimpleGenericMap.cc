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

#include <memory>

#include "nanobind/nanobind.h"
#include "nanobind/stl/string.h"

#include "lsst/cpputils/python.h"

#include "lsst/afw/typehandling/SimpleGenericMap.h"
#include "lsst/afw/typehandling/python.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace lsst {
namespace afw {
namespace typehandling {

namespace {
template <typename K>
void declareSimpleGenericMap(cpputils::python::WrapperCollection& wrappers, std::string const& suffix,
                             std::string const& key) {
    using Class = SimpleGenericMap<K>;
    using PyClass = nb::class_<Class, MutableGenericMap<K>>;

    std::string className = "SimpleGenericMap" + suffix;
    // Give the class a custom docstring to avoid confusing Python users
    std::string docstring =
            "A `dict`-like `~collections.abc.MutableMapping` for use when sharing a map between C++ and "
            "Python.\n" +
            declareGenericMapRestrictions(className, key) +
            R"docstring(
Parameters
----------
mapping : `collections.abc.Mapping`, optional
iterable : iterable, optional
**kwargs
    A ``SimpleGenericMap`` takes the same input arguments as `dict`.
)docstring";
    wrappers.wrapType(PyClass(wrappers.module, className.c_str(), docstring.c_str()),
                      [](auto& mod, auto& cls) {
                          // Don't rewrap members of MutableGenericMap

                          /* need __init__(**kw), __init__(mapping, **kw), __init__(iterable, **kw)
                           * can't find a good way to insert a nb::handle value from C++
                           * can't call MutableGenericMap.__setitem__ without its class_ object, which
                           *    is in a different nanobind module but can't be imported (yet)
                           */
                          cls.def(nb::init<>());
                          cls.def("copy", [](Class const& self) { return Class(self); });

                          // fromkeys easier to implement in Python
                      });
}

}  // namespace

void wrapSimpleGenericMap(cpputils::python::WrapperCollection& wrappers) {
    declareSimpleGenericMap<std::string>(wrappers, "S", "strings");
}

}  // namespace typehandling
}  // namespace afw
}  // namespace lsst
