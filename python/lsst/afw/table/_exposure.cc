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
#include "nanobind/stl/shared_ptr.h"
#include "ndarray/nanobind.h"

#include <memory>

#include "lsst/cpputils/python.h"

#include "lsst/afw/detection/Psf.h"  // forward-declared by Exposure.h
#include "lsst/afw/fits.h"
#include "lsst/geom/Box.h"
#include "lsst/geom/Point.h"
#include "lsst/afw/geom/polygon/Polygon.h"     // forward-declared by Exposure.h
#include "lsst/afw/geom/SkyWcs.h"              // forward-declared by Transform.h
#include "lsst/afw/image/ApCorrMap.h"          // forward-declared by Exposure.h
#include "lsst/afw/image/PhotoCalib.h"         // forward-declared by Exposure.h
#include "lsst/afw/image/VisitInfo.h"          // forward-declared by Exposure.h
#include "lsst/afw/image/TransmissionCurve.h"  // forward-declared by Exposure.h
#include "lsst/afw/cameraGeom/Detector.h"      // forward-declared by Exposure.h
#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/BaseTable.h"
#include "lsst/afw/table/Catalog.h"
#include "lsst/afw/table/Exposure.h"
#include "lsst/afw/table/python/catalog.h"
#include "lsst/afw/table/python/columnView.h"
#include "lsst/afw/table/python/sortedCatalog.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace table {

using cpputils::python::WrapperCollection;

namespace {

using PyExposureRecord = nb::class_<ExposureRecord, BaseRecord>;
using PyExposureTable = nb::class_<ExposureTable, BaseTable>;
using PyExposureCatalog =
        nb::class_<ExposureCatalogT<ExposureRecord>,
                   SortedCatalogT<ExposureRecord>>;

PyExposureRecord declareExposureRecord(WrapperCollection &wrappers) {
    return wrappers.wrapType(PyExposureRecord(wrappers.module, "ExposureRecord"), [](auto &mod, auto &cls) {
        cls.def("getId", &ExposureRecord::getId);
        cls.def("setId", &ExposureRecord::setId, "id"_a = nb::none());
        cls.def_prop_rw("id", &ExposureRecord::getId, &ExposureRecord::setId, nb::arg("id") = nb::none());
        cls.def("getBBox", &ExposureRecord::getBBox);
        cls.def("setBBox", &ExposureRecord::setBBox, "bbox"_a);
        // No property for bbox, as it returns by value.
        cls.def("getTable", &ExposureRecord::getTable);
        // table has no setter
        cls.def_prop_ro("table", &ExposureRecord::getTable);
        cls.def("contains",
                (bool(ExposureRecord::*)(lsst::geom::SpherePoint const &, bool) const) &
                        ExposureRecord::contains,
                "coord"_a, "includeValidPolygon"_a = false);
        cls.def("contains",
                (bool(ExposureRecord::*)(lsst::geom::Point2D const &, geom::SkyWcs const &, bool) const) &
                        ExposureRecord::contains,
                "point"_a, "wcs"_a, "includeValidPolygon"_a = false);
        cls.def("getWcs", &ExposureRecord::getWcs);
        cls.def("setWcs", &ExposureRecord::setWcs, "wcs"_a);
        cls.def_prop_rw("wcs", &ExposureRecord::getWcs, &ExposureRecord::setWcs, nb::arg("exposure")= nb::none());
        cls.def("getPsf", &ExposureRecord::getPsf);
        cls.def("setPsf", &ExposureRecord::setPsf, "psf"_a);
        cls.def_prop_rw("psf", &ExposureRecord::getPsf, &ExposureRecord::setPsf);

        cls.def("getPhotoCalib", &ExposureRecord::getPhotoCalib);
        cls.def("setPhotoCalib", &ExposureRecord::setPhotoCalib, "photoCalib"_a = nb::none());
        cls.def_prop_rw("photoCalib", &ExposureRecord::getPhotoCalib, &ExposureRecord::setPhotoCalib, nb::arg("photoCalib") = nb::none());
        cls.def("getApCorrMap", &ExposureRecord::getApCorrMap);
        cls.def("setApCorrMap", &ExposureRecord::setApCorrMap, "apCorrMap"_a  = nb::none());
        cls.def_prop_rw("apCorrMap", &ExposureRecord::getApCorrMap, &ExposureRecord::setApCorrMap, nb::arg("apCorrMap") = nb::none());
        cls.def("getValidPolygon", &ExposureRecord::getValidPolygon);
        cls.def("setValidPolygon", &ExposureRecord::setValidPolygon, nb::arg("polygon").none());
        cls.def_prop_rw("validPolygon", &ExposureRecord::getValidPolygon, &ExposureRecord::setValidPolygon, nb::arg("polygon").none());
        cls.def("getVisitInfo", &ExposureRecord::getVisitInfo);
        cls.def("setVisitInfo", &ExposureRecord::setVisitInfo, "visitInfo"_a);
        cls.def_prop_rw("visitInfo", &ExposureRecord::getVisitInfo, &ExposureRecord::setVisitInfo);
        cls.def("getTransmissionCurve", &ExposureRecord::getTransmissionCurve);
        cls.def("setTransmissionCurve", &ExposureRecord::setTransmissionCurve, "transmissionCurve"_a = nb::none());
        cls.def_prop_rw("transmissionCurve", &ExposureRecord::getTransmissionCurve,
                         &ExposureRecord::setTransmissionCurve, nb::arg("transmissionCurve") = nb::none());
        cls.def("getDetector", &ExposureRecord::getDetector);
        cls.def("setDetector", &ExposureRecord::setDetector, "detector"_a.none());
        cls.def_prop_rw("detector", &ExposureRecord::getDetector, &ExposureRecord::setDetector, nb::arg("detetor") = nb::none());
    });
}

PyExposureTable declareExposureTable(WrapperCollection &wrappers) {
    return wrappers.wrapType(PyExposureTable(wrappers.module, "ExposureTable"), [](auto &mod, auto &cls) {
        cls.def_static("make", &ExposureTable::make);
        cls.def_static("makeMinimalSchema", &ExposureTable::makeMinimalSchema);
        cls.def_static("checkSchema", &ExposureTable::checkSchema, "schema"_a);

        cls.def_static("getIdKey", &ExposureTable::getIdKey);
        cls.def_static("getBBoxMinKey", &ExposureTable::getBBoxMinKey);
        cls.def_static("getBBoxMaxKey", &ExposureTable::getBBoxMaxKey);

        cls.def("clone", &ExposureTable::clone);
        cls.def("makeRecord", &ExposureTable::makeRecord);
        cls.def("copyRecord", (std::shared_ptr<ExposureRecord>(ExposureTable::*)(BaseRecord const &)) &
                                      ExposureTable::copyRecord);
        cls.def("copyRecord", (std::shared_ptr<ExposureRecord>(ExposureTable::*)(BaseRecord const &,
                                                                                 SchemaMapper const &)) &
                                      ExposureTable::copyRecord);
    });
}

PyExposureCatalog declareExposureCatalog(WrapperCollection &wrappers) {
    using Catalog = ExposureCatalogT<ExposureRecord>;
    table::python::declareSortedCatalog<ExposureRecord>(wrappers, "Exposure", true);

    // We need nb::dynamic_attr() in class definition to support our Python-side caching
    // of the associated ColumnView.
    return wrappers.wrapType(
            PyExposureCatalog(wrappers.module, "ExposureCatalog", nb::dynamic_attr()),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<Schema const &>(), "schema"_a);
                cls.def(nb::init<std::shared_ptr<ExposureTable> const &>(),
                        "table"_a = std::shared_ptr<ExposureTable>());
                cls.def(nb::init<Catalog const &>(), "other"_a);
                // Constructor taking C++ iterators not wrapped; we recommend .extend() (defined in pure
                // Python) instead.
                cls.def_static("readFits", (Catalog(*)(std::string const &, int, int)) & Catalog::readFits,
                               "filename"_a, "hdu"_a = fits::DEFAULT_HDU, "flags"_a = 0);
                cls.def_static("readFits", (Catalog(*)(fits::MemFileManager &, int, int)) & Catalog::readFits,
                               "manager"_a, "hdu"_a = fits::DEFAULT_HDU, "flags"_a = 0);
                // readFits taking Fits objects not wrapped, because Fits objects are not wrapped.

                cls.def("subset",
                        (Catalog(Catalog::*)(ndarray::Array<bool const, 1> const &) const) & Catalog::subset,
                        "mask"_a);
                cls.def("subset",
                        (Catalog(Catalog::*)(std::ptrdiff_t, std::ptrdiff_t, std::ptrdiff_t) const) &
                                Catalog::subset,
                        "startd"_a, "stopd"_a, "step"_a);
                cls.def("subsetContaining",
                        (Catalog(Catalog::*)(lsst::geom::SpherePoint const &, bool) const) &
                                Catalog::subsetContaining,
                        "coord"_a, "includeValidPolygon"_a = false);
                cls.def("subsetContaining",
                        (Catalog(Catalog::*)(lsst::geom::Point2D const &, geom::SkyWcs const &, bool) const) &
                                Catalog::subsetContaining,
                        "point"_a, "wcs"_a, "includeValidPolygon"_a = false);
            });
};

}  // anonymous namespace

void wrapExposure(WrapperCollection &wrappers) {
    // wrappers.addSignatureDependency("lsst.afw.geom");
    // TODO: afw.geom, afw.image, and afw.detection cannot be imported due to
    // circular dependencies until at least afw.image uses WrapperCollection
    // in DM-20703

    auto clsExposureRecord = declareExposureRecord(wrappers);
    auto clsExposureTable = declareExposureTable(wrappers);
    auto clsExposureColumnView = table::python::declareColumnView<ExposureRecord>(wrappers, "Exposure");
    auto clsExposureCatalog = declareExposureCatalog(wrappers);

    clsExposureRecord.attr("Table") = clsExposureTable;
    clsExposureRecord.attr("ColumnView") = clsExposureColumnView;
    clsExposureRecord.attr("Catalog") = clsExposureCatalog;
    clsExposureTable.attr("Record") = clsExposureRecord;
    clsExposureTable.attr("ColumnView") = clsExposureColumnView;
    clsExposureTable.attr("Catalog") = clsExposureCatalog;
    clsExposureCatalog.attr("Record") = clsExposureRecord;
    clsExposureCatalog.attr("Table") = clsExposureTable;
    clsExposureCatalog.attr("ColumnView") = clsExposureColumnView;
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
