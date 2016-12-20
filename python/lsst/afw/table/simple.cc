/* 
 * LSST Data Management System
 * Copyright 2008-2016  AURA/LSST.
 * 
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
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
 * You should have received a copy of the LSST License Statement and 
 * the GNU General Public License along with this program.  If not, 
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */

#include <pybind11/pybind11.h>
//#include <pybind11/operators.h>
#include <pybind11/stl.h>

#include "lsst/afw/coord/Coord.h"
#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/BaseTable.h"
#include "lsst/afw/table/Simple.h"

namespace py = pybind11;

namespace lsst {
namespace afw {
namespace table {

PYBIND11_PLUGIN(_simple) {
    py::module mod("_simple", "Python wrapper for afw _simple library");

    /* Module level */
    py::class_<SimpleTable, std::shared_ptr<SimpleTable>, BaseTable> clsSimpleTable(mod, "SimpleTable");
    py::class_<SimpleRecord, std::shared_ptr<SimpleRecord>, BaseRecord> clsSimpleRecord(mod, "SimpleRecord");

    /* Member types and enums */

    /* Constructors */

    /* Operators */

    /* Members */
    clsSimpleTable.def_static("make", (PTR(SimpleTable) (*)(Schema const &, PTR(IdFactory) const &))
        &SimpleTable::make);
    clsSimpleTable.def_static("make", (PTR(SimpleTable) (*)(Schema const &)) &SimpleTable::make);
    clsSimpleTable.def_static("makeMinimalSchema", &SimpleTable::makeMinimalSchema);
    // Commented-out methods have not yet been tested
    // clsSimpleTable.def_static("checkSchema", &SimpleTable::checkSchema, "other"_a);
    clsSimpleTable.def_static("getIdKey", &SimpleTable::getIdKey);
    clsSimpleTable.def_static("getCoordKey", &SimpleTable::getCoordKey);

    // clsSimpleTable.def("getIdFactory", (PTR(IdFactory) (*)()) &SimpleTable::getIdFactory);
    // clsSimpleTable.def("getIdFactory", (CONST_PTR(IdFactory) (*)()) &SimpleTable::getIdFactory);
    // clsSimpleTable.def("setIdFactory", &SimpleTable::setIdFactory, "idFactory"_a);
    // clsSimpleTable.def("clone", &SimpleTable::clone);
    clsSimpleTable.def("makeRecord", &SimpleTable::makeRecord);
    // clsSimpleTable.def("copyRecord", (PTR(SimpleRecord) (*)(BaseRecord const &)
    //                    &SimpleTable::copyRecord, "other"_a);
    // clsSimpleTable.def("copyRecord", (PTR(SimpleRecord) (*)(BaseRecord const &, SchemaMapper const &)
    //                    &SimpleTable::copyRecord, "other"_a);

    clsSimpleRecord.def("getId", &SimpleRecord::getId);
    clsSimpleRecord.def("setId", &SimpleRecord::setId);
    clsSimpleRecord.def("getCoord", &SimpleRecord::getCoord);
    clsSimpleRecord.def("setCoord", (void (SimpleRecord::*)(IcrsCoord const &)) &SimpleRecord::setCoord);
    clsSimpleRecord.def("setCoord", (void (SimpleRecord::*)(Coord const &)) &SimpleRecord::setCoord);
    clsSimpleRecord.def("getRa", &SimpleRecord::getRa);
    clsSimpleRecord.def("setRa", &SimpleRecord::setRa);
    clsSimpleRecord.def("getDec", &SimpleRecord::getDec);
    clsSimpleRecord.def("setDec", &SimpleRecord::setDec);

    return mod.ptr();
}

}}}  // namespace lsst::afw::table
