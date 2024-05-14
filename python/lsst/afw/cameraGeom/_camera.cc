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
#include <lsst/cpputils/python.h>

#include "nanobind/stl/vector.h"
#include "nanobind/stl/string.h"

#include "lsst/afw/table/io/python.h"
#include "lsst/afw/cameraGeom/Camera.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace lsst {
namespace afw {
namespace cameraGeom {

using PyCamera = nb::class_<Camera, DetectorCollection>;
using PyCameraBuilder = nb::class_<Camera::Builder, DetectorCollectionBase<Detector::InCameraBuilder>>;

// Bindings here are ordered to match the order of the declarations in
// Camera.h to the greatest extent possible; modifications to this file should
// attempt to preserve this.

void wrapCamera(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.afw.table.io");
    wrappers.addSignatureDependency("lsst.afw.cameraGeom");
    auto camera = wrappers.wrapType(PyCamera(wrappers.module, "Camera"), [](auto &mod, auto &cls) {
        cls.def("rebuild", &Camera::rebuild);
        cls.def("getName", &Camera::getName);
        cls.def("getPupilFactoryName", &Camera::getPupilFactoryName);
        cls.def("findDetectors", &Camera::findDetectors, "point"_a, "cameraSys"_a);
        cls.def("findDetectorsList", &Camera::findDetectorsList, "pointList"_a, "cameraSys"_a);
        // transform methods are wrapped with lambdas that translate exceptions for backwards compatibility
        cls.def(
                "getTransform",
                [](Camera const &self, CameraSys const &fromSys, CameraSys const &toSys) {
                    try {
                        return self.getTransform(fromSys, toSys);
                    } catch (pex::exceptions::NotFoundError &err) {
                        PyErr_SetString(PyExc_KeyError, err.what());
                        throw nb::python_error();
                    }
                },
                "fromSys"_a, "toSys"_a);
        cls.def("getTransformMap", &Camera::getTransformMap);
        cls.def(
                "transform",
                [](Camera const &self, lsst::geom::Point2D const &point, CameraSys const &fromSys,
                   CameraSys const &toSys) {
                    try {
                        return self.transform(point, fromSys, toSys);
                    } catch (pex::exceptions::NotFoundError &err) {
                        PyErr_SetString(PyExc_KeyError, err.what());
                        throw nb::python_error();
                    }
                },
                "point"_a, "fromSys"_a, "toSys"_a);
        cls.def(
                "transform",
                [](Camera const &self, std::vector<lsst::geom::Point2D> const &points,
                   CameraSys const &fromSys, CameraSys const &toSys) {
                    try {
                        return self.transform(points, fromSys, toSys);
                    } catch (pex::exceptions::NotFoundError &err) {
                        PyErr_SetString(PyExc_KeyError, err.what());
                        throw nb::python_error();
                    }
                },
                "points"_a, "fromSys"_a, "toSys"_a);
        table::io::python::addPersistableMethods(cls);
    });
    wrappers.wrapType(PyCameraBuilder(camera, "Builder"), [](auto &mod, auto &cls) {
        cls.def(nb::init<std::string const &>(), "name"_a);
        cls.def(nb::init<Camera const &>(), "camera"_a);
        cls.def("finish", &Camera::Builder::finish);
        cls.def("getName", &Camera::Builder::getName);
        cls.def("setName", &Camera::Builder::setName);
        cls.def("getPupilFactoryName", &Camera::Builder::getPupilFactoryName);
        cls.def("setPupilFactoryName", &Camera::Builder::setPupilFactoryName);
        cls.def("setPupilFactoryClass", [](Camera::Builder &self, nb::object pupilFactoryClass) {
            std::string pupilFactoryName = "lsst.afw.cameraGeom.pupil.PupilFactory";
            if (!pupilFactoryClass.is(nb::none())) {
                pupilFactoryName = nb::str("{}.{}").format(pupilFactoryClass.attr("__module__"),
                                                           pupilFactoryClass.attr("__name__")).c_str();
            }
            self.setPupilFactoryName(pupilFactoryName);
        });
        cls.def("setTransformFromFocalPlaneTo", &Camera::Builder::setTransformFromFocalPlaneTo, "toSys"_a,
                "transform"_a);
        cls.def("discardTransformFromFocalPlaneTo", &Camera::Builder::discardTransformFromFocalPlaneTo);
        cls.def("add", &Camera::Builder::add);
        cls.def("__delitem__", nb::overload_cast<int>(&Camera::Builder::remove));
        cls.def("__delitem__", nb::overload_cast<std::string const &>(&Camera::Builder::remove));
    });
}

}  // namespace cameraGeom
}  // namespace afw
}  // namespace lsst
