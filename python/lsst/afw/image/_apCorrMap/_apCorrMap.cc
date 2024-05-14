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
#include "nanobind/stl/vector.h"
#include "lsst/cpputils/python.h"

#include <string>
#include <vector>
#include <nanobind/make_iterator.h>

#include "lsst/afw/table/io/python.h"  // for addPersistableMethods
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/image/ApCorrMap.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace image {
namespace {

using PyApCorrMap = nb::class_<ApCorrMap, typehandling::Storable>;

void wrapApCorrMap(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyApCorrMap(wrappers.module, "ApCorrMap"), [](auto &mod, auto &cls) {
        cls.def(nb::init<>());

        table::io::python::addPersistableMethods<ApCorrMap>(cls);

        /* Operators */
        cls.def("__imul__", &ApCorrMap::operator*=);
        cls.def("__itruediv__", &ApCorrMap::operator/=);

        /* Members */
        cls.def("get", &ApCorrMap::get);
        cls.def("set", &ApCorrMap::set);
        cls.def(
                "items", [](ApCorrMap const &self) {
                    return nb::make_iterator(nb::type<ApCorrMap>(), "iterator", self.begin(), self.end()); },
                nb::keep_alive<0, 1>());
        // values, keys, and __iter__ defined in apCorrMap.py

        cls.def("__len__", &ApCorrMap::size);
        cls.def("__getitem__", &ApCorrMap::operator[]);
        cls.def("__setitem__", &ApCorrMap::set);
        cls.def("__contains__",
                [](ApCorrMap const &self, std::string name) { return static_cast<bool>(self.get(name)); });
    });
}

NB_MODULE(_apCorrMap, mod) {
    lsst::cpputils::python::WrapperCollection wrappers(mod, "lsst.afw.image._apCorrMap");
    wrappers.addInheritanceDependency("lsst.afw.table.io");
    wrappers.addInheritanceDependency("lsst.afw.typehandling");
    wrapApCorrMap(wrappers);
    wrappers.finish();
    /* Declare CRTP base class. */
}
}  // namespace
}  // namespace image
}  // namespace afw
}  // namespace lsst
