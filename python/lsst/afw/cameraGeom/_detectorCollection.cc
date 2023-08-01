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
#include <lsst/utils/python.h>

#include "pybind11/stl.h"

#include "lsst/afw/table/io/python.h"
#include "lsst/afw/cameraGeom/DetectorCollection.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace cameraGeom {

namespace {

template <typename T>
using PyDetectorCollectionBase = py::class_<DetectorCollectionBase<T>>;

using PyDetectorCollection = py::class_<DetectorCollection, DetectorCollectionBase<Detector const>>;

template <typename T>
void declareDetectorCollectionBase(PyDetectorCollectionBase<T> &cls) {
    cls.def("getNameMap", &DetectorCollectionBase<T>::getNameMap);
    cls.def("getIdMap", &DetectorCollectionBase<T>::getIdMap);
    cls.def("__len__", &DetectorCollectionBase<T>::size);
    cls.def("get",
            py::overload_cast<std::string const &, std::shared_ptr<T>>(&DetectorCollectionBase<T>::get,
                                                                       py::const_),
            "name"_a, "default"_a = nullptr);
    cls.def("get", py::overload_cast<int, std::shared_ptr<T>>(&DetectorCollectionBase<T>::get, py::const_),
            "id"_a, "default"_a = nullptr);
    cls.def("__contains__", [](DetectorCollectionBase<T> const &self, std::string const &name) {
        return self.get(name) != nullptr;
    });
    cls.def("__contains__",
            [](DetectorCollectionBase<T> const &self, int id) { return self.get(id) != nullptr; });
}
}  // namespace
void wrapDetectorCollection(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.afw.table.io");
    wrappers.wrapType(
            PyDetectorCollectionBase<Detector const>(wrappers.module, "DetectorCollectionDetectorBase"),
            [](auto &mod, auto &cls) { declareDetectorCollectionBase(cls); });
    wrappers.wrapType(PyDetectorCollection(wrappers.module, "DetectorCollection"), [](auto &mod, auto &cls) {
        ;
        cls.def(py::init<DetectorCollection::List>());
        cls.def("getFpBBox", &DetectorCollection::getFpBBox);
        table::io::python::addPersistableMethods(cls);
    });

    wrappers.wrapType(PyDetectorCollectionBase<Detector::InCameraBuilder>(wrappers.module,
                                                                          "DetectorCollectionBuilderBase"),
                      [](auto &mod, auto &cls) { declareDetectorCollectionBase(cls); });
}
}  // namespace cameraGeom
}  // namespace afw
}  // namespace lsst
