// -*- lsst-c++ -*-
/*
 * LSST Data Management System
 * See COPYRIGHT file at the top of the source tree.
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
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

#ifndef LSST_AFW_TABLE_IO_PYTHON_H
#define LSST_AFW_TABLE_IO_PYTHON_H

#include "nanobind/nanobind.h"
#include "nanobind/stl/shared_ptr.h"

#include <memory>
#include <string>

#include "lsst/afw/fits.h"
#include "lsst/afw/table/io/Persistable.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace lsst {
namespace afw {
namespace table {
namespace io {
namespace python {

/**
 * Add table::io::Persistable and PersistableFacade methods to the pybind11 wrapper for a class
 *
 * This allows your class to be used without importing lsst.afw.table in Python.
 *
 * Use as follows:
 * - When declaring the pybind11 class that wraps your Class in afw, do *not* list
 *   table::io::PersistableFacade<Class> and table::io::Persistable as base classes
 *   unless you have to, and in that case be prepared for possible trouble.
 * - Call this function to wrap the methods that make your object persistable.
 */
template <typename Class, typename... Args>
void addPersistableMethods(nanobind::class_<Class, Args...> &cls) {
    cls.def_static("readFits",
                   (std::shared_ptr<Class>(*)(std::string const &, int)) & PersistableFacade<Class>::readFits,
                   "fileName"_a, "hdu"_a = fits::DEFAULT_HDU);
    cls.def_static(
            "readFits",
            (std::shared_ptr<Class>(*)(fits::MemFileManager &, int)) & PersistableFacade<Class>::readFits,
            "manager"_a, "hdu"_a = fits::DEFAULT_HDU);
    cls.def("writeFits", (void (Class::*)(std::string const &, std::string const &) const) & Class::writeFits,
            "fileName"_a, "mode"_a = "w");
    cls.def("writeFits",
            (void (Class::*)(fits::MemFileManager &, std::string const &) const) & Class::writeFits,
            "manager"_a, "mode"_a = "w");
    cls.def("isPersistable", &Class::isPersistable);
}
}  // namespace python
}  // namespace io
}  // namespace table
}  // namespace afw
}  // namespace lsst

#endif
