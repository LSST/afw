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

#include "lsst/afw/table/io/python.h"  // for addPersistableMethods
#include "lsst/afw/table/Schema.h"
#include "lsst/afw/table/Exposure.h"
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/image/CoaddInputs.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace image {

using PyCoaddInputs = py::class_<CoaddInputs, typehandling::Storable>;

void wrapCoaddInputs(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.afw.typehandling");
    wrappers.wrapType(PyCoaddInputs(wrappers.module, "CoaddInputs"), [](auto &mod, auto &cls) {
        /* Constructors */
        cls.def(py::init<>());
        cls.def(py::init<table::Schema const &, table::Schema const &>(), "visitSchema"_a, "ccdSchema"_a);
        cls.def(py::init<table::ExposureCatalog const &, table::ExposureCatalog const &>(), "visits"_a,
                "ccds"_a);

        table::io::python::addPersistableMethods<CoaddInputs>(cls);

        /* Members */
        cls.def_readwrite("visits", &CoaddInputs::visits);
        cls.def_readwrite("ccds", &CoaddInputs::ccds);
        cls.def("isPersistable", &CoaddInputs::isPersistable);
    });
}

}  // namespace image
}  // namespace afw
}  // namespace lsst
