/*
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

#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/fits.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace table {
namespace io {

using PyPersistable = py::class_<Persistable>;

void wrapPersistable(utils::python::WrapperCollection &wrappers) {
    // TODO: uncomment once afw.fits uses WrapperCollection
    // wrappers.addSignatureDependency("lsst.afw.fits");

    wrappers.wrapType(PyPersistable(wrappers.module, "Persistable"), [](auto &mod, auto &cls) {
        cls.def("writeFits",
                (void (Persistable::*)(std::string const &, std::string const &) const) &
                        Persistable::writeFits,
                "fileName"_a, "mode"_a = "w");
        cls.def("writeFits",
                (void (Persistable::*)(fits::MemFileManager &, std::string const &) const) &
                        Persistable::writeFits,
                "manager"_a, "mode"_a = "w");
        cls.def("isPersistable", &Persistable::isPersistable);
    });
}

}  // namespace io
}  // namespace table
}  // namespace afw
}  // namespace lsst
