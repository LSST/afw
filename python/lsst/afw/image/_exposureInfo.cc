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

#include "lsst/daf/base/PropertySet.h"
#include "lsst/afw/cameraGeom/Detector.h"
#include "lsst/afw/detection/Psf.h"
#include "lsst/afw/geom/polygon/Polygon.h"
#include "lsst/afw/geom/SkyWcs.h"
#include "lsst/afw/image/ApCorrMap.h"
#include "lsst/afw/image/PhotoCalib.h"
#include "lsst/afw/image/CoaddInputs.h"
#include "lsst/afw/image/FilterLabel.h"
#include "lsst/afw/image/VisitInfo.h"
#include "lsst/afw/image/TransmissionCurve.h"
#include "lsst/afw/image/ExposureInfo.h"
#include "lsst/afw/typehandling/Storable.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace image {
namespace {

using PyExposureInfo = py::class_<ExposureInfo>;

// Template methods where we can use pybind11's overload resolution (T is input)
template <class T>
void declareGenericMethods(PyExposureInfo &cls) {
    using Class = PyExposureInfo::type;
    cls.def(
            "setComponent",
            [](PyExposureInfo::type &self, std::string const &key, T const &object) {
                self.setComponent(typehandling::makeKey<T>(key), object);
            },
            "key"_a, "object"_a);
}
// Template methods where we need to provide a unified interface (T is not input)
void declareGenericMethodsMerged(PyExposureInfo &cls) {
    using typehandling::Storable;
    using Class = PyExposureInfo::type;
    cls.def(
            "hasComponent",
            [](Class const &self, std::string const &key) {
                return self.hasComponent(typehandling::makeKey<std::shared_ptr<Storable const>>(key));
            },
            "key"_a);
    cls.def(
            "getComponent",
            [](Class const &self, std::string const &key) -> py::object {
                auto sharedKey = typehandling::makeKey<std::shared_ptr<Storable const>>(key);
                // Cascading if-elses to support other types in the future
                if (self.hasComponent(sharedKey)) {
                    return py::cast(self.getComponent(sharedKey));
                } else {
                    return py::none();
                }
            },
            "key"_a);
    cls.def(
            "removeComponent",
            [](Class &self, std::string const &key) {
                self.removeComponent(typehandling::makeKey<std::shared_ptr<Storable const>>(key));
            },
            "key"_a);
}

void declareExposureInfo(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyExposureInfo(wrappers.module, "ExposureInfo"), [](auto &mod, auto &cls) {
        /* Constructors */
        cls.def(py::init<std::shared_ptr<geom::SkyWcs const> const &,
                         std::shared_ptr<detection::Psf const> const &,
                         std::shared_ptr<PhotoCalib const> const &,
                         std::shared_ptr<cameraGeom::Detector const> const &,
                         std::shared_ptr<geom::polygon::Polygon const> const &,
                         std::shared_ptr<daf::base::PropertySet> const &,
                         std::shared_ptr<CoaddInputs> const &, std::shared_ptr<ApCorrMap> const &,
                         std::shared_ptr<VisitInfo const> const &,
                         std::shared_ptr<TransmissionCurve const> const &>(),
                "wcs"_a = std::shared_ptr<geom::SkyWcs const>(),
                "psf"_a = std::shared_ptr<detection::Psf const>(),
                "photoCalib"_a = std::shared_ptr<PhotoCalib const>(),
                "detector"_a = std::shared_ptr<cameraGeom::Detector const>(),
                "polygon"_a = std::shared_ptr<geom::polygon::Polygon const>(),
                "metadata"_a = std::shared_ptr<daf::base::PropertySet>(),
                "coaddInputs"_a = std::shared_ptr<CoaddInputs>(),
                "apCorrMap"_a = std::shared_ptr<ApCorrMap>(),
                "visitInfo"_a = std::shared_ptr<VisitInfo const>(), "transmissionCurve"_a = nullptr);
        cls.def(py::init<>());
        cls.def(py::init<ExposureInfo>(), "other"_a);
        cls.def(py::init<ExposureInfo, bool>(), "other"_a, "copyMetadata"_a);

        /* Members */
        cls.attr("KEY_WCS") = ExposureInfo::KEY_WCS.getId();
        cls.def("hasWcs", &ExposureInfo::hasWcs);
        cls.def("getWcs", (std::shared_ptr<geom::SkyWcs>(ExposureInfo::*)()) & ExposureInfo::getWcs);
        cls.def("setWcs", &ExposureInfo::setWcs, "wcs"_a);

        cls.attr("KEY_DETECTOR") = ExposureInfo::KEY_DETECTOR.getId();
        cls.def("hasDetector", &ExposureInfo::hasDetector);
        cls.def("getDetector", &ExposureInfo::getDetector);
        cls.def(
                "setDetector",
                [](ExposureInfo &self, py::object detector) {
                    if (detector.is(py::none())) {
                        self.setDetector(nullptr);
                    } else {
                        self.setDetector(py::cast<std::shared_ptr<afw::cameraGeom::Detector>>(detector));
                    }
                },
                "detector"_a);

        cls.attr("KEY_FILTER") = ExposureInfo::KEY_FILTER.getId();
        cls.def("hasFilter", &ExposureInfo::hasFilter);
        cls.def("getFilter", &ExposureInfo::getFilter);
        cls.def("setFilter", &ExposureInfo::setFilter, "filterLabel"_a);

        declareGenericMethods<std::shared_ptr<typehandling::Storable const>>(cls);
        declareGenericMethodsMerged(cls);

        cls.attr("KEY_PHOTO_CALIB") = ExposureInfo::KEY_PHOTO_CALIB.getId();
        cls.def("hasPhotoCalib", &ExposureInfo::hasPhotoCalib);
        cls.def("getPhotoCalib", &ExposureInfo::getPhotoCalib);
        cls.def("setPhotoCalib", &ExposureInfo::setPhotoCalib, "photoCalib"_a);

        cls.def("hasId", &ExposureInfo::hasId);
        // Use exception handler to avoid overhead of calling hasId twice, and
        // because asking for a nonexistent ID should be rare.
        auto getId = [](ExposureInfo const &self) -> py::object {
            try {
                return py::cast(self.getId());
            } catch (pex::exceptions::NotFoundError const &) {
                return py::none();
            }
        };
        auto setId = [](ExposureInfo &self, py::object id) {
            if (id.is_none()) {
                self.clearId();
            } else {
                self.setId(id.cast<table::RecordId>());
            }
        };
        cls.def("getId", getId);
        cls.def("setId", setId, "id"_a);
        cls.def("clearId", &ExposureInfo::clearId);
        cls.def_property("id", getId, setId);

        cls.def("getMetadata", &ExposureInfo::getMetadata);
        cls.def("setMetadata", &ExposureInfo::setMetadata, "metadata"_a);

        cls.attr("KEY_PSF") = ExposureInfo::KEY_PSF.getId();
        cls.def("hasPsf", &ExposureInfo::hasPsf);
        cls.def("getPsf", &ExposureInfo::getPsf);
        cls.def(
                "setPsf",
                [](ExposureInfo &self, py::object psf) {
                    if (psf.is(py::none())) {
                        self.setPsf(nullptr);
                    } else {
                        self.setPsf(py::cast<std::shared_ptr<afw::detection::Psf>>(psf));
                    }
                },
                "psf"_a);

        cls.attr("KEY_VALID_POLYGON") = ExposureInfo::KEY_VALID_POLYGON.getId();
        cls.def("hasValidPolygon", &ExposureInfo::hasValidPolygon);
        cls.def("getValidPolygon", &ExposureInfo::getValidPolygon);
        cls.def(
                "setValidPolygon",
                [](ExposureInfo &self, py::object polygon) {
                    if (polygon.is(py::none())) {
                        self.setValidPolygon(nullptr);
                    } else {
                        self.setValidPolygon(py::cast<std::shared_ptr<afw::geom::polygon::Polygon>>(polygon));
                    }
                },
                "polygon"_a);

        cls.attr("KEY_AP_CORR_MAP") = ExposureInfo::KEY_AP_CORR_MAP.getId();
        cls.def("hasApCorrMap", &ExposureInfo::hasApCorrMap);
        cls.def("getApCorrMap", (std::shared_ptr<ApCorrMap>(ExposureInfo::*)()) & ExposureInfo::getApCorrMap);
        cls.def("setApCorrMap", &ExposureInfo::setApCorrMap, "apCorrMap"_a);
        cls.def("initApCorrMap", &ExposureInfo::initApCorrMap);

        cls.attr("KEY_COADD_INPUTS") = ExposureInfo::KEY_COADD_INPUTS.getId();
        cls.def("hasCoaddInputs", &ExposureInfo::hasCoaddInputs);
        cls.def("getCoaddInputs", &ExposureInfo::getCoaddInputs);
        cls.def("setCoaddInputs", &ExposureInfo::setCoaddInputs, "coaddInputs"_a);

        cls.def("hasVisitInfo", &ExposureInfo::hasVisitInfo);
        cls.def("getVisitInfo", &ExposureInfo::getVisitInfo);
        cls.def("setVisitInfo", &ExposureInfo::setVisitInfo, "visitInfo"_a);

        cls.attr("KEY_TRANSMISSION_CURVE") = ExposureInfo::KEY_TRANSMISSION_CURVE.getId();
        cls.def("hasTransmissionCurve", &ExposureInfo::hasTransmissionCurve);
        cls.def("getTransmissionCurve", &ExposureInfo::getTransmissionCurve);
        cls.def("setTransmissionCurve", &ExposureInfo::setTransmissionCurve, "transmissionCurve"_a);
    });
}
}  // namespace
void wrapExposureInfo(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.daf.base");
    wrappers.addSignatureDependency("lsst.afw.geom");
    wrappers.addSignatureDependency("lsst.afw.cameraGeom");
    wrappers.addSignatureDependency("lsst.afw.detection");  // For Psf
    declareExposureInfo(wrappers);
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
