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

#include <vector>

#include "lsst/utils/python.h"
#include "lsst/geom/Point.h"
#include "lsst/afw/table/io/python.h"
#include "lsst/afw/cameraGeom/CameraSys.h"
#include "lsst/afw/cameraGeom/TransformMap.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace cameraGeom {
namespace {

using PyTransformMap = py::class_<TransformMap>;
using PyTransformMapConnection = py::class_<TransformMap::Connection>;

void declareTransformMap(lsst::utils::python::WrapperCollection &wrappers) {
    auto transformMap = wrappers.wrapType(PyTransformMap(wrappers.module, "TransformMap"), [](auto &mod,
                                                                                              auto &cls) {
        cls.def(py::init([](CameraSys const &reference, TransformMap::Transforms const &transforms) {
                    // An apparent pybind11 bug: it's usually happy to cast away constness, but won't do it
                    // here.
                    return std::const_pointer_cast<TransformMap>(TransformMap::make(reference, transforms));
                }),
                "reference"_a, "transforms"_a);
        cls.def(py::init([](CameraSys const &reference,
                            std::vector<TransformMap::Connection> const &connections) {
                    // An apparent pybind11 bug: it's usually happy to cast away constness, but won't do it
                    // here.
                    return std::const_pointer_cast<TransformMap>(TransformMap::make(reference, connections));
                }),
                "reference"_a, "connections"_a);
        cls.def("__len__", &TransformMap::size);
        cls.def("__contains__", &TransformMap::contains);
        cls.def(
                "__iter__",
                [](TransformMap const &self) { return py::make_iterator(self.begin(), self.end()); },
                py::keep_alive<0, 1>()); /* Essential: keep object alive while iterator exists */

        cls.def("transform",
                py::overload_cast<lsst::geom::Point2D const &, CameraSys const &, CameraSys const &>(
                        &TransformMap::transform, py::const_),
                "point"_a, "fromSys"_a, "toSys"_a);
        cls.def("transform",
                py::overload_cast<std::vector<lsst::geom::Point2D> const &, CameraSys const &,
                                  CameraSys const &>(&TransformMap::transform, py::const_),
                "pointList"_a, "fromSys"_a, "toSys"_a);
        cls.def("getTransform", &TransformMap::getTransform, "fromSys"_a, "toSys"_a);
        cls.def("getConnections", &TransformMap::getConnections);
        table::io::python::addPersistableMethods(cls);
    });
    wrappers.wrapType(PyTransformMapConnection(transformMap, "Connection"), [](auto &mod, auto &cls) {
        cls.def(py::init<std::shared_ptr<afw::geom::TransformPoint2ToPoint2 const>, CameraSys const &,
                         CameraSys const &>(),
                "transform"_a, "fromSys"_a, "toSys"_a);
        cls.def_readwrite("transform", &TransformMap::Connection::transform);
        cls.def_readwrite("fromSys", &TransformMap::Connection::fromSys);
        cls.def_readwrite("toSys", &TransformMap::Connection::toSys);
        utils::python::addOutputOp(cls, "__repr__");
    });
}
}  // namespace
void wrapTransformMap(lsst::utils::python::WrapperCollection &wrappers) { declareTransformMap(wrappers); }

}  // namespace cameraGeom
}  // namespace afw
}  // namespace lsst
