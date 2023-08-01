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
#include "pybind11/stl.h"

#include "ndarray/pybind11.h"

#include "lsst/utils/python.h"
#include "lsst/afw/table/Key.h"
#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/FunctorKey.h"
#include "lsst/afw/table/arrays.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace table {

using utils::python::WrapperCollection;

namespace {

// We don't expose base classes (e.g. FunctorKey) to Python, since they're just used to
// define a CRTP interface in C++ and in Python that's just duck-typing.

template <typename T>
using PyArrayKey = py::class_<ArrayKey<T>>;

template <typename T>
void declareArrayKey(WrapperCollection &wrappers, std::string const &suffix) {
    wrappers.wrapType(
            PyArrayKey<T>(wrappers.module, ("Array" + suffix + "Key").c_str()), [](auto &mod, auto &cls) {
                cls.def(py::init<>());
                cls.def(py::init<Key<Array<T>> const &>());
                cls.def(py::init<SubSchema const &>());
                cls.def(py::init<std::vector<Key<T>> const &>());

                cls.def_static("addFields",
                               (ArrayKey<T>(*)(Schema &, std::string const &, std::string const &,
                                               std::string const &, std::vector<T> const &)) &
                                       ArrayKey<T>::addFields,
                               "schema"_a, "name"_a, "doc"_a, "unit"_a, "docData"_a);
                cls.def_static("addFields",
                               (ArrayKey<T>(*)(Schema &, std::string const &, std::string const &,
                                               std::string const &, std::size_t size)) &
                                       ArrayKey<T>::addFields,
                               "schema"_a, "name"_a, "doc"_a, "unit"_a, "size"_a);
                cls.def("get", &ArrayKey<T>::get);
                cls.def("set", &ArrayKey<T>::set);
                cls.def("isValid", &ArrayKey<T>::isValid);
                cls.def("__eq__", &ArrayKey<T>::operator==, py::is_operator());
                cls.def("__ne__", &ArrayKey<T>::operator!=, py::is_operator());
                cls.def("__getitem__",
                        // Special implementation of __getitem__ to support ints and slices
                        [](ArrayKey<T> const &self, py::object const &index) -> py::object {
                            if (py::isinstance<py::slice>(index)) {
                                py::slice slice(index);
                                py::size_t start = 0, stop = 0, step = 0, length = 0;
                                bool valid = slice.compute(self.getSize(), &start, &stop, &step, &length);
                                if (!valid) throw py::error_already_set();
                                if (step != 1) {
                                    throw py::index_error("Step for ArrayKey must be 1.");
                                }
                                return py::cast(self.slice(start, stop));
                            } else {
                                std::size_t n = utils::python::cppIndex(self.getSize(),
                                                                        py::cast<std::ptrdiff_t>(index));
                                return py::cast(self[n]);
                            }
                        });
                cls.def("getSize", &ArrayKey<T>::getSize);
                cls.def("slice", &ArrayKey<T>::slice);
            });
};

}  // namespace

void wrapArrays(WrapperCollection &wrappers) {
    declareArrayKey<float>(wrappers, "F");
    declareArrayKey<double>(wrappers, "D");
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
