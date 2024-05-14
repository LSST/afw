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
#include "ndarray/nanobind.h"

#include "lsst/cpputils/python.h"

#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/BaseTable.h"
#include "lsst/afw/table/Simple.h"
#include "lsst/afw/table/python/catalog.h"
#include "lsst/afw/table/python/columnView.h"
#include "lsst/afw/table/python/sortedCatalog.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace lsst {
namespace afw {
namespace table {

using cpputils::python::WrapperCollection;

namespace {

using PySimpleTable = nb::class_<SimpleTable, BaseTable>;
using PySimpleRecord = nb::class_<SimpleRecord, BaseRecord>;

PySimpleRecord declareSimpleRecord(WrapperCollection &wrappers) {
    return wrappers.wrapType(PySimpleRecord(wrappers.module, "SimpleRecord"), [](auto &mod, auto &cls) {
        cls.def("getId", &SimpleRecord::getId);
        cls.def("setId", &SimpleRecord::setId);
        cls.def("getTable", &SimpleRecord::getTable);
        cls.def_prop_ro("table", &SimpleRecord::getTable);
        cls.def("getCoord", &SimpleRecord::getCoord);
        cls.def("setCoord", &SimpleRecord::setCoord);
        cls.def("getRa", &SimpleRecord::getRa);
        cls.def("setRa", &SimpleRecord::setRa);
        cls.def("getDec", &SimpleRecord::getDec);
        cls.def("setDec", &SimpleRecord::setDec);
    });
}

PySimpleTable declareSimpleTable(WrapperCollection &wrappers) {
    return wrappers.wrapType(PySimpleTable(wrappers.module, "SimpleTable"), [](auto &mod, auto &cls) {
        cls.def_static("make",
                       (std::shared_ptr<SimpleTable>(*)(Schema const &, std::shared_ptr<IdFactory> const &)) &
                               SimpleTable::make);
        cls.def_static("make", (std::shared_ptr<SimpleTable>(*)(Schema const &)) & SimpleTable::make);
        cls.def_static("makeMinimalSchema", &SimpleTable::makeMinimalSchema);
        cls.def_static("checkSchema", &SimpleTable::checkSchema, "schema"_a);
        cls.def_static("getIdKey", &SimpleTable::getIdKey);
        cls.def_static("getCoordKey", &SimpleTable::getCoordKey);

        cls.def("getIdFactory", (std::shared_ptr<IdFactory>(SimpleTable::*)()) & SimpleTable::getIdFactory);
        cls.def("setIdFactory", &SimpleTable::setIdFactory, "idFactory"_a);
        cls.def("clone", &SimpleTable::clone);
        cls.def("makeRecord", &SimpleTable::makeRecord);
        cls.def("copyRecord",
                (std::shared_ptr<SimpleRecord>(SimpleTable::*)(BaseRecord const &)) & SimpleTable::copyRecord,
                "other"_a);
        cls.def("copyRecord",
                (std::shared_ptr<SimpleRecord>(SimpleTable::*)(BaseRecord const &, SchemaMapper const &)) &
                        SimpleTable::copyRecord,
                "other"_a, "mapper"_a);
    });
}

}  // namespace

void wrapSimple(WrapperCollection &wrappers) {
    auto clsSimpleRecord = declareSimpleRecord(wrappers);
    auto clsSimpleTable = declareSimpleTable(wrappers);
    auto clsSimpleColumnView = table::python::declareColumnView<SimpleRecord>(wrappers, "Simple");
    auto clsSimpleCatalog = table::python::declareSortedCatalog<SimpleRecord>(wrappers, "Simple");

    clsSimpleRecord.attr("Table") = clsSimpleTable;
    clsSimpleRecord.attr("ColumnView") = clsSimpleColumnView;
    clsSimpleRecord.attr("Catalog") = clsSimpleCatalog;
    clsSimpleTable.attr("Record") = clsSimpleRecord;
    clsSimpleTable.attr("ColumnView") = clsSimpleColumnView;
    clsSimpleTable.attr("Catalog") = clsSimpleCatalog;
    clsSimpleCatalog.attr("Record") = clsSimpleRecord;
    clsSimpleCatalog.attr("Table") = clsSimpleTable;
    clsSimpleCatalog.attr("ColumnView") = clsSimpleColumnView;
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
