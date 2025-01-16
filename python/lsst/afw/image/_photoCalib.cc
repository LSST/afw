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
#include "lsst/cpputils/python.h"

#include <memory>

#include "ndarray/pybind11.h"

#include "lsst/cpputils/python.h"

#include "lsst/daf/base/PropertySet.h"
#include "lsst/afw/math/BoundedField.h"
#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/table/io/python.h"  // for addPersistableMethods
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/image/PhotoCalib.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace image {
namespace {

void declareMeasurement(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(py::class_<Measurement, std::shared_ptr<Measurement>>(wrappers.module, "Measurement"),
                      [](auto &mod, auto &cls) {
                          cls.def(py::init<double, double>(), "value"_a, "error"_a);
                          cls.def_readonly("value", &Measurement::value);
                          cls.def_readonly("error", &Measurement::error);

                          cpputils::python::addOutputOp(cls, "__str__");
                          cls.def("__repr__", [](Measurement const &self) {
                              std::ostringstream os;
                              os << "Measurement(" << self << ")";
                              return os.str();
                          });
                      });
}

void declarePhotoCalib(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(
            py::class_<PhotoCalib, std::shared_ptr<PhotoCalib>, typehandling::Storable>(wrappers.module,
                                                                                        "PhotoCalib"),
            [](auto &mod, auto &cls) {
                /* Constructors */
                cls.def(py::init<>());
                cls.def(py::init<double, double, lsst::geom::Box2I>(), "calibrationMean"_a,
                        "calibrationErr"_a = 0.0, "bbox"_a = lsst::geom::Box2I());
                cls.def(py::init<std::shared_ptr<afw::math::BoundedField>, double>(), "calibration"_a,
                        "calibrationErr"_a = 0.0);
                cls.def(py::init<double, double, std::shared_ptr<afw::math::BoundedField>, bool>(),
                        "calibrationMean"_a, "calibrationErr"_a, "calibration"_a, "isConstant"_a);

                table::io::python::addPersistableMethods<PhotoCalib>(cls);

                /* Members - nanojansky */
                cls.def("instFluxToNanojansky",
                        (double(PhotoCalib::*)(double, lsst::geom::Point<double, 2> const &) const) &
                                PhotoCalib::instFluxToNanojansky,
                        "instFlux"_a, "point"_a);
                cls.def("instFluxToNanojansky",
                        (double(PhotoCalib::*)(double) const) & PhotoCalib::instFluxToNanojansky,
                        "instFlux"_a);

                cls.def("instFluxToNanojansky",
                        (Measurement(PhotoCalib::*)(double, double, lsst::geom::Point<double, 2> const &)
                                 const) &
                                PhotoCalib::instFluxToNanojansky,
                        "instFlux"_a, "instFluxErr"_a, "point"_a);
                cls.def("instFluxToNanojansky",
                        (Measurement(PhotoCalib::*)(double, double) const) & PhotoCalib::instFluxToNanojansky,
                        "instFlux"_a, "instFluxErr"_a);

                cls.def("instFluxToNanojansky",
                        (Measurement(PhotoCalib::*)(afw::table::SourceRecord const &, std::string const &)
                                 const) &
                                PhotoCalib::instFluxToNanojansky,
                        "sourceRecord"_a, "instFluxField"_a);

                cls.def("instFluxToNanojansky",
                        (ndarray::Array<double, 2, 2>(PhotoCalib::*)(afw::table::SourceCatalog const &,
                                                                     std::string const &) const) &
                                PhotoCalib::instFluxToNanojansky,
                        "sourceCatalog"_a, "instFluxField"_a);

                cls.def("instFluxToNanojansky",
                        (void(PhotoCalib::*)(afw::table::SourceCatalog &, std::string const &,
                                             std::string const &) const) &
                                PhotoCalib::instFluxToNanojansky,
                        "sourceCatalog"_a, "instFluxField"_a, "outField"_a);

                /* Members - magnitudes */
                cls.def("instFluxToMagnitude",
                        (double(PhotoCalib::*)(double, lsst::geom::Point<double, 2> const &) const) &
                                PhotoCalib::instFluxToMagnitude,
                        "instFlux"_a, "point"_a);
                cls.def("instFluxToMagnitude",
                        (double(PhotoCalib::*)(double) const) & PhotoCalib::instFluxToMagnitude,
                        "instFlux"_a);

                cls.def("instFluxToMagnitude",
                        (Measurement(PhotoCalib::*)(double, double, lsst::geom::Point<double, 2> const &)
                                 const) &
                                PhotoCalib::instFluxToMagnitude,
                        "instFlux"_a, "instFluxErr"_a, "point"_a);
                cls.def("instFluxToMagnitude",
                        (Measurement(PhotoCalib::*)(double, double) const) & PhotoCalib::instFluxToMagnitude,
                        "instFlux"_a, "instFluxErr"_a);

                cls.def("instFluxToMagnitude",
                        (Measurement(PhotoCalib::*)(afw::table::SourceRecord const &, std::string const &)
                                 const) &
                                PhotoCalib::instFluxToMagnitude,
                        "sourceRecord"_a, "instFluxField"_a);

                cls.def("instFluxToMagnitude",
                        (ndarray::Array<double, 2, 2>(PhotoCalib::*)(afw::table::SourceCatalog const &,
                                                                     std::string const &) const) &
                                PhotoCalib::instFluxToMagnitude,
                        "sourceCatalog"_a, "instFluxField"_a);

                cls.def("instFluxToMagnitude",
                        (void(PhotoCalib::*)(afw::table::SourceCatalog &, std::string const &,
                                             std::string const &) const) &
                                PhotoCalib::instFluxToMagnitude,
                        "sourceCatalog"_a, "instFluxField"_a, "outField"_a);

                /* from magnitude. */
                cls.def("magnitudeToInstFlux",
                        py::overload_cast<double, lsst::geom::Point<double, 2> const &>(
                                &PhotoCalib::magnitudeToInstFlux, py::const_),
                        "magnitude"_a, "point"_a);
                cls.def("magnitudeToInstFlux",
                        py::overload_cast<double>(&PhotoCalib::magnitudeToInstFlux, py::const_),
                        "magnitude"_a);

                /* from nanojansky. */
                cls.def("nanojanskyToInstFlux",
                        py::overload_cast<double, lsst::geom::Point<double, 2> const &>(
                                &PhotoCalib::nanojanskyToInstFlux, py::const_),
                        "nanojansky"_a, "point"_a);
                cls.def("nanojanskyToInstFlux",
                        py::overload_cast<double>(&PhotoCalib::nanojanskyToInstFlux, py::const_),
                        "nanojansky"_a);

                /* utilities */
                cls.def("getCalibrationMean", &PhotoCalib::getCalibrationMean);
                cls.def("getCalibrationErr", &PhotoCalib::getCalibrationErr);
                cls.def_property_readonly("_isConstant", &PhotoCalib::isConstant);
                cls.def("getInstFluxAtZeroMagnitude", &PhotoCalib::getInstFluxAtZeroMagnitude);
                cls.def("getLocalCalibration", &PhotoCalib::getLocalCalibration, "point"_a);

                cls.def("computeScaledCalibration", &PhotoCalib::computeScaledCalibration);
                cls.def("computeScalingTo", &PhotoCalib::computeScalingTo);

                cls.def("calibrateImage", &PhotoCalib::calibrateImage, "maskedImage"_a,
                        "includeScaleUncertainty"_a = true);
                cls.def("uncalibrateImage", &PhotoCalib::uncalibrateImage, "maskedImage"_a,
                        "includeScaleUncertainty"_a = true);

                cls.def("calibrateCatalog",
                        py::overload_cast<afw::table::SourceCatalog const &,
                                          std::vector<std::string> const &>(&PhotoCalib::calibrateCatalog,
                                                                            py::const_),
                        "maskedImage"_a, "fluxFields"_a);
                cls.def("calibrateCatalog",
                        py::overload_cast<afw::table::SourceCatalog const &>(&PhotoCalib::calibrateCatalog,
                                                                             py::const_),
                        "maskedImage"_a);

                /* Operators */
                cls.def("__eq__", &PhotoCalib::operator==, py::is_operator());
                cls.def("__ne__", &PhotoCalib::operator!=, py::is_operator());
                cpputils::python::addOutputOp(cls, "__str__");
                cls.def("__repr__", [](PhotoCalib const &self) {
                    std::ostringstream os;
                    os << "PhotoCalib(" << self << ")";
                    return os.str();
                });
            });
}

void declareCalib(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        /* Utility functions */
        mod.def("makePhotoCalibFromMetadata",
                py::overload_cast<daf::base::PropertySet &, bool>(makePhotoCalibFromMetadata), "metadata"_a,
                "strip"_a = false);
        mod.def("makePhotoCalibFromCalibZeroPoint",
                py::overload_cast<double, double>(makePhotoCalibFromCalibZeroPoint), "instFluxMag0"_a,
                "instFluxMag0Err"_a = false);
    });
}
}  // namespace
void wrapPhotoCalib(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.afw.typehandling");
    declareMeasurement(wrappers);
    declarePhotoCalib(wrappers);
    declareCalib(wrappers);
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
