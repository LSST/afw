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
#include "lsst/cpputils/python.h"

#include "lsst/afw/table/io/python.h"  // for addPersistableMethods
#include "lsst/afw/table/Schema.h"
#include "lsst/afw/table/Exposure.h"
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/image/CoaddInputs.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace image {

using PyCoaddInputs = nb::class_<CoaddInputs, typehandling::Storable>;

void wrapCoaddInputs(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.afw.typehandling");
    wrappers.wrapType(PyCoaddInputs(wrappers.module, "CoaddInputs"), [](auto &mod, auto &cls) {
        /* Constructors */
        cls.def(nb::init<>());
        cls.def(nb::init<table::Schema const &, table::Schema const &>(), "visitSchema"_a, "ccdSchema"_a);
        cls.def(nb::init<table::ExposureCatalog const &, table::ExposureCatalog const &>(), "visits"_a,
                "ccds"_a);

        table::io::python::addPersistableMethods<CoaddInputs>(cls);

        /* Members */
        cls.def_rw("visits", &CoaddInputs::visits);
        cls.def_rw("ccds", &CoaddInputs::ccds, nb::rv_policy::reference);
        cls.def("isPersistable", &CoaddInputs::isPersistable);
    });
}

}  // namespace image
}  // namespace afw
}  // namespace lsst
