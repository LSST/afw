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
#include "nanobind/eigen/dense.h"
#include "nanobind/stl/map.h"

#include <memory>

#include "ndarray/nanobind.h"

#include "lsst/cpputils/python.h"

#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/BaseTable.h"
#include "lsst/afw/table/Simple.h"
#include "lsst/afw/table/Schema.h"
#include "lsst/afw/table/slots.h"
#include "lsst/afw/table/Source.h"
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

using PySourceRecord = nb::class_<SourceRecord, SimpleRecord>;
using PySourceTable = nb::class_<SourceTable, SimpleTable>;
using PySourceColumnView =
        nb::class_<SourceColumnViewT<SourceRecord>,
                   ColumnViewT<SourceRecord>>;

/*
Declare member and static functions for a nanobind wrapper of SourceRecord
*/
PySourceRecord declareSourceRecord(WrapperCollection &wrappers) {
    return wrappers.wrapType(PySourceRecord(wrappers.module, "SourceRecord"), [](auto &mod, auto &cls) {
        cls.def("getFootprint", &SourceRecord::getFootprint);
        cls.def("setFootprint", &SourceRecord::setFootprint);
        cls.def("getTable", &SourceRecord::getTable);
        cls.def_prop_ro("table", &SourceRecord::getTable);

        cls.def("getParent", &SourceRecord::getParent);
        cls.def("setParent", &SourceRecord::setParent, "id"_a);

        cls.def("getPsfInstFlux", &SourceRecord::getPsfInstFlux);
        cls.def("getPsfInstFluxErr", &SourceRecord::getPsfInstFluxErr);
        cls.def("getPsfFluxFlag", &SourceRecord::getPsfFluxFlag);

        cls.def("getModelInstFlux", &SourceRecord::getModelInstFlux);
        cls.def("getModelInstFluxErr", &SourceRecord::getModelInstFluxErr);
        cls.def("getModelFluxFlag", &SourceRecord::getModelFluxFlag);

        cls.def("getApInstFlux", &SourceRecord::getApInstFlux);
        cls.def("getApInstFluxErr", &SourceRecord::getApInstFluxErr);
        cls.def("getApFluxFlag", &SourceRecord::getApFluxFlag);

        cls.def("getGaussianInstFlux", &SourceRecord::getGaussianInstFlux);
        cls.def("getGaussianInstFluxErr", &SourceRecord::getGaussianInstFluxErr);
        cls.def("getGaussianFluxFlag", &SourceRecord::getGaussianFluxFlag);

        cls.def("getCalibInstFlux", &SourceRecord::getCalibInstFlux);
        cls.def("getCalibInstFluxErr", &SourceRecord::getCalibInstFluxErr);
        cls.def("getCalibFluxFlag", &SourceRecord::getCalibFluxFlag);

        cls.def("getCentroid", &SourceRecord::getCentroid);
        cls.def("getCentroidErr", &SourceRecord::getCentroidErr);
        cls.def("getCentroidFlag", &SourceRecord::getCentroidFlag);

        cls.def("getShape", &SourceRecord::getShape);
        cls.def("getShapeErr", &SourceRecord::getShapeErr);
        cls.def("getShapeFlag", &SourceRecord::getShapeFlag);

        cls.def("getX", &SourceRecord::getX);
        cls.def("getY", &SourceRecord::getY);
        cls.def("getIxx", &SourceRecord::getIxx);
        cls.def("getIyy", &SourceRecord::getIyy);
        cls.def("getIxy", &SourceRecord::getIxy);
        cls.def("updateCoord", (void (SourceRecord::*)(geom::SkyWcs const &, bool)) & SourceRecord::updateCoord,
                "wcs"_a, "include_covariance"_a=true);
        cls.def("updateCoord",
                (void (SourceRecord::*)(geom::SkyWcs const &, PointKey<double> const &, bool)) &
                        SourceRecord::updateCoord,
                "wcs"_a, "key"_a, "include_covariance"_a=true);
    });
}

/*
Declare member and static functions for a nanobind wrapper of SourceTable
*/
PySourceTable declareSourceTable(WrapperCollection &wrappers) {
    return wrappers.wrapType(PySourceTable(wrappers.module, "SourceTable"), [](auto &mod, auto &cls) {
        cls.def("clone", &SourceTable::clone);
        cls.def_static("make",
                       (std::shared_ptr<SourceTable>(*)(Schema const &, std::shared_ptr<IdFactory> const &)) &
                               SourceTable::make);
        cls.def_static("make", (std::shared_ptr<SourceTable>(*)(Schema const &)) & SourceTable::make);
        cls.def_static("makeMinimalSchema", &SourceTable::makeMinimalSchema);
        cls.def_static("getParentKey", &SourceTable::getParentKey);
        cls.def("copyRecord", (std::shared_ptr<SourceRecord>(SourceTable::*)(BaseRecord const &)) &
                                      SourceTable::copyRecord);
        cls.def("copyRecord",
                (std::shared_ptr<SourceRecord>(SourceTable::*)(BaseRecord const &, SchemaMapper const &)) &
                        SourceTable::copyRecord);
        cls.def("makeRecord", &SourceTable::makeRecord);

        cls.def("getPsfFluxSlot", &SourceTable::getPsfFluxSlot);
        cls.def("definePsfFlux", &SourceTable::definePsfFlux, "name"_a);

        cls.def("getModelFluxSlot", &SourceTable::getModelFluxSlot);
        cls.def("defineModelFlux", &SourceTable::defineModelFlux, "name"_a);

        cls.def("getApFluxSlot", &SourceTable::getApFluxSlot);
        cls.def("defineApFlux", &SourceTable::defineApFlux, "name"_a);

        cls.def("getGaussianFluxSlot", &SourceTable::getGaussianFluxSlot);
        cls.def("defineGaussianFlux", &SourceTable::defineGaussianFlux, "name"_a);

        cls.def("getCalibFluxSlot", &SourceTable::getCalibFluxSlot);
        cls.def("defineCalibFlux", &SourceTable::defineCalibFlux, "name"_a);

        cls.def("getCentroidSlot", &SourceTable::getCentroidSlot);
        cls.def("defineCentroid", &SourceTable::defineCentroid, "name"_a);

        cls.def("getShapeSlot", &SourceTable::getShapeSlot);
        cls.def("defineShape", &SourceTable::defineShape, "name"_a);
    });
}

PySourceColumnView declareSourceColumnView(WrapperCollection &wrappers) {
    table::python::declareColumnView<SourceRecord>(wrappers, "Source", true);
    return wrappers.wrapType(PySourceColumnView(wrappers.module, "SourceColumnView"),
                             [](auto &mod, auto &cls) {
                                 using Class = SourceColumnViewT<SourceRecord>;
                                 cls.def("getPsfInstFlux", &Class::getPsfInstFlux);
                                 cls.def("getPsfInstFluxErr", &Class::getPsfInstFluxErr);
                                 cls.def("getApInstFlux", &Class::getApInstFlux);
                                 cls.def("getApInstFluxErr", &Class::getApInstFluxErr);
                                 cls.def("getModelInstFlux", &Class::getModelInstFlux);
                                 cls.def("getModelInstFluxErr", &Class::getModelInstFluxErr);
                                 cls.def("getGaussianInstFlux", &Class::getGaussianInstFlux);
                                 cls.def("getGaussianInstFluxErr", &Class::getGaussianInstFluxErr);
                                 cls.def("getCalibInstFlux", &Class::getCalibInstFlux);
                                 cls.def("getCalibInstFluxErr", &Class::getCalibInstFluxErr);
                                 cls.def("getX", &Class::getX);
                                 cls.def("getY", &Class::getY);
                                 cls.def("getIxx", &Class::getIxx);
                                 cls.def("getIyy", &Class::getIyy);
                                 cls.def("getIxy", &Class::getIxy);
                             });
}

}  // namespace

void wrapSource(WrapperCollection &wrappers) {
    // TODO: uncomment once afw.geom uses WrapperCollection
    // wrappers.addSignatureDependency("lsst.afw.geom.ellipses");

    // SourceFitsFlags enum values are used as integer masks, so wrap as attributes instead of an enum
    // static_cast is required to avoid an import error (nb::cast and nb::int_ do not work by themselves
    // and are not required with the static_cast)
    auto &mod = wrappers.module;
    mod.attr("SOURCE_IO_NO_FOOTPRINTS") = static_cast<int>(SourceFitsFlags::SOURCE_IO_NO_FOOTPRINTS);
    mod.attr("SOURCE_IO_NO_HEAVY_FOOTPRINTS") =
            static_cast<int>(SourceFitsFlags::SOURCE_IO_NO_HEAVY_FOOTPRINTS);

    auto clsSourceRecord = declareSourceRecord(wrappers);
    auto clsSourceTable = declareSourceTable(wrappers);
    auto clsSourceColumnView = declareSourceColumnView(wrappers);
    auto clsSourceCatalog = table::python::declareSortedCatalog<SourceRecord>(wrappers, "Source");

    clsSourceRecord.attr("Table") = clsSourceTable;
    clsSourceRecord.attr("ColumnView") = clsSourceColumnView;
    clsSourceRecord.attr("Catalog") = clsSourceCatalog;
    clsSourceTable.attr("Record") = clsSourceRecord;
    clsSourceTable.attr("ColumnView") = clsSourceColumnView;
    clsSourceTable.attr("Catalog") = clsSourceCatalog;
    clsSourceCatalog.attr("Record") = clsSourceRecord;
    clsSourceCatalog.attr("Table") = clsSourceTable;
    clsSourceCatalog.attr("ColumnView") = clsSourceColumnView;
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
