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

#include "pybind11/pybind11.h"

#include "lsst/utils/python.h"

#include "lsst/afw/table/AliasMap.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace table {

using PyAliasMap = py::class_<AliasMap>;

void wrapAliasMap(utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyAliasMap(wrappers.module, "AliasMap"), [](auto &mod, auto &cls) {
        cls.def(py::init<>());
        cls.def(py::init<AliasMap const &>());

        cls.def("__len__", &AliasMap::size);
        cls.def("empty", &AliasMap::empty);
        cls.def("apply", &AliasMap::apply, "name"_a);
        cls.def("get", &AliasMap::get, "alias"_a);
        cls.def("__getitem__", &AliasMap::get, "alias"_a);
        cls.def("set", &AliasMap::set, "alias"_a, "target"_a);
        cls.def("__setitem__", &AliasMap::set);
        cls.def("erase", &AliasMap::erase, "alias"_a);
        cls.def("__delitem__", &AliasMap::erase, "alias"_a);
        cls.def("__eq__", [](AliasMap &self, AliasMap &other) { return self == other; });
        cls.def("__ne__", [](AliasMap &self, AliasMap &other) { return self != other; });
        cls.def("contains", &AliasMap::contains, "other"_a);
        cls.def("__contains__", &AliasMap::contains);
        cls.def("items", [](AliasMap &self) { return py::make_iterator(self.begin(), self.end()); },
                py::keep_alive<0, 1>());
    });
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
