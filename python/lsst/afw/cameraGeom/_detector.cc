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
#include <string>

#include "pybind11/pybind11.h"
#include <lsst/utils/python.h>

#include "pybind11/stl.h"

#include "ndarray/pybind11.h"

#include "lsst/utils/python.h"
#include "lsst/afw/cameraGeom/CameraSys.h"
#include "lsst/afw/cameraGeom/Orientation.h"
#include "lsst/geom.h"
#include "lsst/afw/table/io/python.h"
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/cameraGeom/Detector.h"
#include "lsst/afw/cameraGeom/TransformMap.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace cameraGeom {

namespace {

using PyDetectorBase = py::class_<DetectorBase, std::shared_ptr<DetectorBase>, typehandling::Storable>;
using PyDetector = py::class_<Detector, DetectorBase, std::shared_ptr<Detector>>;
using PyDetectorBuilder = py::class_<Detector::Builder, DetectorBase, std::shared_ptr<Detector::Builder>>;
using PyDetectorPartialRebuilder = py::class_<Detector::PartialRebuilder, Detector::Builder,
                                              std::shared_ptr<Detector::PartialRebuilder>>;
using PyDetectorInCameraBuilder =
        py::class_<Detector::InCameraBuilder, Detector::Builder, std::shared_ptr<Detector::InCameraBuilder>>;

// Declare Detector methods overloaded on one coordinate system class
template <typename SysT, typename PyClass>
void declare1SysMethods(PyClass &cls) {
    cls.def("getCorners",
            (std::vector<lsst::geom::Point2D>(Detector::*)(SysT const &) const) & Detector::getCorners,
            "cameraSys"_a);
    cls.def("getCenter", (lsst::geom::Point2D(Detector::*)(SysT const &) const) & Detector::getCenter,
            "cameraSys"_a);
    cls.def("hasTransform", (bool (Detector::*)(SysT const &) const) & Detector::hasTransform, "cameraSys"_a);
}

// Declare Detector methods templated on two coordinate system classes
template <typename FromSysT, typename ToSysT, typename PyClass>
void declare2SysMethods(PyClass &cls) {
    cls.def("getTransform",
            (std::shared_ptr<geom::TransformPoint2ToPoint2>(Detector::*)(FromSysT const &, ToSysT const &)
                     const) &
                    Detector::getTransform,
            "fromSys"_a, "toSys"_a);
    cls.def("transform",
            (lsst::geom::Point2D(Detector::*)(lsst::geom::Point2D const &, FromSysT const &, ToSysT const &)
                     const) &
                    Detector::transform,
            "point"_a, "fromSys"_a, "toSys"_a);
    cls.def("transform",
            (std::vector<lsst::geom::Point2D>(Detector::*)(std::vector<lsst::geom::Point2D> const &,
                                                           FromSysT const &, ToSysT const &) const) &
                    Detector::transform,
            "points"_a, "fromSys"_a, "toSys"_a);
}

void declareDetectorBase(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyDetectorBase(wrappers.module, "DetectorBase"), [](auto &mod, auto &cls) {
        cls.def("getName", &DetectorBase::getName);
        cls.def("getId", &DetectorBase::getId);
        cls.def("getType", &DetectorBase::getType);
        cls.def("getPhysicalType", &DetectorBase::getPhysicalType);
        cls.def("getSerial", &DetectorBase::getSerial);
        cls.def("getBBox", &DetectorBase::getBBox);
        cls.def("getOrientation", &DetectorBase::getOrientation);
        cls.def("getPixelSize", &DetectorBase::getPixelSize);
        cls.def("hasCrosstalk", &DetectorBase::hasCrosstalk);
        cls.def("getCrosstalk", &DetectorBase::getCrosstalk);
        cls.def("getNativeCoordSys", &DetectorBase::getNativeCoordSys);
        cls.def("makeCameraSys",
                py::overload_cast<CameraSys const &>(&DetectorBase::makeCameraSys, py::const_),
                "cameraSys"_a);
        cls.def("makeCameraSys",
                py::overload_cast<CameraSysPrefix const &>(&DetectorBase::makeCameraSys, py::const_),
                "cameraSysPrefix"_a);
    });
}

void declareDetectorBuilder(PyDetector &parent);
void declareDetectorPartialRebuilder(PyDetector &parent);
void declareDetectorInCameraBuilder(PyDetector &parent);

void declareDetector(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyDetector(wrappers.module, "Detector"), [](auto &mod, auto &cls) {
        declareDetectorBuilder(cls);
        declareDetectorPartialRebuilder(cls);
        declareDetectorInCameraBuilder(cls);
        cls.def("rebuild", &Detector::rebuild);
        declare1SysMethods<CameraSys>(cls);
        declare1SysMethods<CameraSysPrefix>(cls);
        declare2SysMethods<CameraSys, CameraSys>(cls);
        declare2SysMethods<CameraSys, CameraSysPrefix>(cls);
        declare2SysMethods<CameraSysPrefix, CameraSys>(cls);
        declare2SysMethods<CameraSysPrefix, CameraSysPrefix>(cls);
        cls.def("getTransformMap", &Detector::getTransformMap);
        cls.def("getAmplifiers", &Detector::getAmplifiers);
        // __iter__ defined in pure-Python extension
        cls.def(
                "__getitem__",
                [](Detector const &self, std::ptrdiff_t i) {
                    return self[utils::python::cppIndex(self.size(), i)];
                },
                "i"_a);
        cls.def("__getitem__", py::overload_cast<std::string const &>(&Detector::operator[], py::const_),
                "name"_a);
        cls.def("__len__", &Detector::size);
        table::io::python::addPersistableMethods(cls);
    });
}

void declareDetectorBuilder(PyDetector &parent) {
    PyDetectorBuilder cls(parent, "Builder");
    cls.def("setBBox", &Detector::Builder::setBBox);
    cls.def("setType", &Detector::Builder::setType);
    cls.def("setSerial", &Detector::Builder::setSerial);
    cls.def("setPhysicalType", &Detector::Builder::setPhysicalType);
    cls.def("setCrosstalk", &Detector::Builder::setCrosstalk);
    cls.def("unsetCrosstalk", &Detector::Builder::unsetCrosstalk);
    cls.def("getAmplifiers", &Detector::Builder::getAmplifiers);
    cls.def(
            "__getitem__",
            [](Detector::Builder const &self, std::ptrdiff_t i) {
                return self[utils::python::cppIndex(self.size(), i)];
            },
            "i"_a);
    cls.def("__getitem__", py::overload_cast<std::string const &>(&Detector::Builder::operator[], py::const_),
            "name"_a);
    cls.def("append", &Detector::Builder::append);
    cls.def("clear", &Detector::Builder::clear);
    cls.def("__len__", &Detector::Builder::size);
}

void declareDetectorPartialRebuilder(PyDetector &parent) {
    PyDetectorPartialRebuilder cls(parent, "PartialRebuilder");
    cls.def(py::init<Detector const &>(), "detector"_a);
    cls.def("finish", &Detector::PartialRebuilder::finish);
}

void declareDetectorInCameraBuilder(PyDetector &parent) {
    PyDetectorInCameraBuilder cls(parent, "InCameraBuilder");
    cls.def("setOrientation", &Detector::InCameraBuilder::setOrientation);
    cls.def("setPixelSize", &Detector::InCameraBuilder::setPixelSize);
    cls.def("setTransformFromPixelsTo",
            py::overload_cast<CameraSysPrefix const &,
                              std::shared_ptr<afw::geom::TransformPoint2ToPoint2 const>>(
                    &Detector::InCameraBuilder::setTransformFromPixelsTo),
            "toSys"_a, "transform"_a);
    cls.def("setTransformFromPixelsTo",
            py::overload_cast<CameraSys const &, std::shared_ptr<afw::geom::TransformPoint2ToPoint2 const>>(
                    &Detector::InCameraBuilder::setTransformFromPixelsTo),
            "toSys"_a, "transform"_a);
    cls.def("discardTransformFromPixelsTo",
            py::overload_cast<CameraSysPrefix const &>(
                    &Detector::InCameraBuilder::discardTransformFromPixelsTo),
            "toSys"_a);
    cls.def("discardTransformFromPixelsTo",
            py::overload_cast<CameraSys const &>(&Detector::InCameraBuilder::discardTransformFromPixelsTo),
            "toSys"_a);
}
}  // namespace
void wrapDetector(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.afw.typehandling");
    wrappers.wrapType(py::enum_<DetectorType>(wrappers.module, "DetectorType"), [](auto &mod, auto &enm) {
        enm.value("SCIENCE", DetectorType::SCIENCE);
        enm.value("FOCUS", DetectorType::FOCUS);
        enm.value("GUIDER", DetectorType::GUIDER);
        enm.value("WAVEFRONT", DetectorType::WAVEFRONT);
    });
    declareDetectorBase(wrappers);
    declareDetector(wrappers);
}
}  // namespace cameraGeom
}  // namespace afw
}  // namespace lsst
