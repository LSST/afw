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

#include <memory>
#include <sstream>

#include "lsst/utils/python.h"

#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/BaseTable.h"
#include "lsst/afw/detection/Peak.h"
#include "lsst/afw/table/python/catalog.h"
#include "lsst/afw/table/python/columnView.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace detection {

namespace {

using PyPeakRecord = py::class_<PeakRecord, table::BaseRecord>;
using PyPeakTable = py::class_<PeakTable, table::BaseTable>;

/**
@internal Declare constructors and member and static functions for a pybind11 PeakRecord
*/
void declarePeakRecord(PyPeakRecord &cls) {
    cls.def("getTable", &PeakRecord::getTable);
    cls.def_property_readonly("table", &PeakRecord::getTable);
    cls.def("getId", &PeakRecord::getId);
    cls.def("setId", &PeakRecord::setId);
    cls.def("getIx", &PeakRecord::getIx);
    cls.def("getIy", &PeakRecord::getIy);
    cls.def("setIx", &PeakRecord::setIx);
    cls.def("setIy", &PeakRecord::setIy);
    cls.def("getI", &PeakRecord::getI);
    cls.def("getCentroid", (lsst::geom::Point2I(PeakRecord::*)(bool) const) & PeakRecord::getCentroid);
    cls.def("getCentroid", (lsst::geom::Point2D(PeakRecord::*)() const) & PeakRecord::getCentroid);
    cls.def("getFx", &PeakRecord::getFx);
    cls.def("getFy", &PeakRecord::getFy);
    cls.def("setFx", &PeakRecord::setFx);
    cls.def("setFy", &PeakRecord::setFy);
    cls.def("getF", &PeakRecord::getF);
    cls.def("getPeakValue", &PeakRecord::getPeakValue);
    cls.def("setPeakValue", &PeakRecord::setPeakValue);
    utils::python::addOutputOp(cls, "__str__");
    utils::python::addOutputOp(cls, "__repr__");
}

/**
@internal Declare constructors and member and static functions for a pybind11 PeakTable
*/
void declarePeakTable(PyPeakTable &cls) {
    cls.def_static("make", &PeakTable::make, "schema"_a, "forceNew"_a = false);
    cls.def_static("makeMinimalSchema", &PeakTable::makeMinimalSchema);
    cls.def_static("checkSchema", &PeakTable::checkSchema, "schema"_a);
    cls.def("getIdFactory", (std::shared_ptr<table::IdFactory>(PeakTable::*)()) & PeakTable::getIdFactory);
    cls.def("setIdFactory", &PeakTable::setIdFactory, "factory"_a);
    cls.def_static("getIdKey", &PeakTable::getIdKey);
    cls.def_static("getIxKey", &PeakTable::getIxKey);
    cls.def_static("getIyKey", &PeakTable::getIyKey);
    cls.def_static("getFxKey", &PeakTable::getFxKey);
    cls.def_static("getFyKey", &PeakTable::getFyKey);
    cls.def_static("getPeakValueKey", &PeakTable::getPeakValueKey);
    cls.def("clone", &PeakTable::clone);
    cls.def("makeRecord", &PeakTable::makeRecord);
    cls.def("copyRecord", (std::shared_ptr<PeakRecord>(PeakTable::*)(afw::table::BaseRecord const &)) &
                                  PeakTable::copyRecord);
    cls.def("copyRecord", (std::shared_ptr<PeakRecord>(PeakTable::*)(afw::table::BaseRecord const &,
                                                                     afw::table::SchemaMapper const &)) &
                                  PeakTable::copyRecord);
}

}  // namespace

void wrapPeak(utils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.afw.table");

    auto clsPeakRecord = wrappers.wrapType(PyPeakRecord(wrappers.module, "PeakRecord"),
                                           [](auto &mod, auto &cls) { declarePeakRecord(cls); });
    auto clsPeakTable = wrappers.wrapType(PyPeakTable(wrappers.module, "PeakTable"),
                                          [](auto &mod, auto &cls) { declarePeakTable(cls); });

    auto clsPeakColumnView = table::python::declareColumnView<PeakRecord>(wrappers, "Peak");
    auto clsPeakCatalog = table::python::declareCatalog<PeakRecord>(wrappers, "Peak");

    clsPeakRecord.attr("Table") = clsPeakTable;
    clsPeakRecord.attr("ColumnView") = clsPeakColumnView;
    clsPeakRecord.attr("Catalog") = clsPeakCatalog;
    clsPeakTable.attr("Record") = clsPeakRecord;
    clsPeakTable.attr("ColumnView") = clsPeakColumnView;
    clsPeakTable.attr("Catalog") = clsPeakCatalog;
    clsPeakCatalog.attr("Record") = clsPeakRecord;
    clsPeakCatalog.attr("Table") = clsPeakTable;
    clsPeakCatalog.attr("ColumnView") = clsPeakColumnView;
}

}  // namespace detection
}  // namespace afw
}  // namespace lsst
