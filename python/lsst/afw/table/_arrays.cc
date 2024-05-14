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

#include "ndarray/nanobind.h"

#include "lsst/cpputils/python.h"
#include "lsst/afw/table/Key.h"
#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/FunctorKey.h"
#include "lsst/afw/table/arrays.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace table {

using cpputils::python::WrapperCollection;

namespace {

// We don't expose base classes (e.g. FunctorKey) to Python, since they're just used to
// define a CRTP interface in C++ and in Python that's just duck-typing.

template <typename T>
using PyArrayKey = nb::class_<ArrayKey<T>>;

template <typename T>
void declareArrayKey(WrapperCollection &wrappers, std::string const &suffix) {
    wrappers.wrapType(
            PyArrayKey<T>(wrappers.module, ("Array" + suffix + "Key").c_str()), [](auto &mod, auto &cls) {
                cls.def(nb::init<>());
                cls.def(nb::init<Key<Array<T>> const &>());
                cls.def(nb::init<SubSchema const &>());
                cls.def(nb::init<std::vector<Key<T>> const &>());

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
                cls.def("__eq__", &ArrayKey<T>::operator==, nb::is_operator());
                cls.def("__ne__", &ArrayKey<T>::operator!=, nb::is_operator());
                cls.def("__getitem__",
                        // Special implementation of __getitem__ to support ints and slices
                        [](ArrayKey<T> const &self, nb::object const &index) -> nb::object {
                            if (nb::isinstance<nb::slice>(index)) {
                                nb::slice slice = (nanobind::slice &&) index;
                                Py_ssize_t start = 0, stop = 0, step = 0;
                                size_t length = 0;
                                auto result  = slice.compute(self.getSize());
                                start = result.template get<0>();
                                stop = result.template get<1>();
                                step = result.template get<2>();
                                length = result.template get<3>();
                                bool valid = true;
                                if (!valid) throw nb::python_error();
                                if (step != 1) {
                                    throw nb::index_error("Step for ArrayKey must be 1.");
                                }
                                return nb::cast(self.slice(start, stop));
                            } else {
                                std::size_t n = cpputils::python::cppIndex(self.getSize(),
                                                                        nb::cast<std::ptrdiff_t>(index));
                                return nb::cast(self[n]);
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
