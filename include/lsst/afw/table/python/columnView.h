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
#ifndef AFW_TABLE_PYBIND11_COLUMNVIEW_H_INCLUDED
#define AFW_TABLE_PYBIND11_COLUMNVIEW_H_INCLUDED

#include "pybind11/pybind11.h"

#include "ndarray/pybind11.h"
#include "lsst/utils/python.h"

#include "lsst/afw/table/BaseColumnView.h"

namespace lsst {
namespace afw {
namespace table {
namespace python {

template <typename Record>
using PyColumnView = pybind11::class_<ColumnViewT<Record>, BaseColumnView>;

/**
 * Declare member and static functions for a given instantiation of lsst::afw::table::ColumnViewT<RecordT>.
 *
 * @tparam Record  Record type, e.g. BaseRecord or SimpleRecord.
 *
 * @param[in] wrappers Package manager class will be added to.
 * @param[in] name     Name prefix of the record type, e.g. "Base" or "Simple".
 * @param[in] isBase   Whether this instantiation is only being used as a base class
 *                     (used to set the class name).
 */
template <typename Record>
PyColumnView<Record> declareColumnView(utils::python::WrapperCollection& wrappers, std::string const& name,
                                       bool isBase = false) {
    std::string fullName;
    if (isBase) {
        fullName = "_" + name + "ColumnViewBase";
    } else {
        fullName = name + "ColumnView";
    }
    return wrappers.wrapType(PyColumnView<Record>(wrappers.module, fullName.c_str()),
                             [](auto& mod, auto& cls) {
                                 cls.def("getTable", &ColumnViewT<Record>::getTable);
                                 cls.def_property_readonly("table", &ColumnViewT<Record>::getTable);
                             });
};
}  // namespace python
}  // namespace table
}  // namespace afw
}  // namespace lsst
#endif
