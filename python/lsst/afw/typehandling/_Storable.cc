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
#include "nanobind/stl/string.h"

#include "lsst/utils/python.h"

#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/typehandling/python.h"

namespace nb = nanobind;
using namespace nb::literals;


namespace lsst {
namespace afw {
namespace typehandling {

using PyStorable = nb::class_<Storable, table::io::Persistable, StorableHelper<>>;

void wrapStorable(cpputils::python::WrapperCollection& wrappers) {
    wrappers.addInheritanceDependency("lsst.afw.table.io");

    wrappers.wrapType(PyStorable(wrappers.module, "Storable"), [](auto& mod, auto& cls) {
        // Do not wrap methods inherited from Persistable
        cls.def(nb::init<>());  // Dummy constructor for pure-Python subclasses
        // Do not wrap optional Storable methods; let subclasses do it as appropriate
        cls.def("__eq__", [](Storable const& self, Storable const& other) { return self.equals(other); },
                "other"_a);
    });

    wrappers.wrapType(
        nb::class_<StorableHelperFactory>(
            wrappers.module, "StorableHelperFactory"
        ),
        [](auto& mod, auto& cls) {
            cls.def(nb::init<std::string&, std::string&>());
        }
    );
}

}  // namespace typehandling
}  // namespace afw
}  // namespace lsst
