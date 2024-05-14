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

#include <nanobind/nanobind.h>
#include <lsst/cpputils/python.h>

#include "lsst/afw/coord/Observatory.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace coord {

void wrapObservatory(lsst::cpputils::python::WrapperCollection& wrappers) {
    wrappers.wrapType(
            nb::class_<Observatory>(wrappers.module, "Observatory"),
            [](auto& mod, auto& cls) {
                /* Constructors */
                cls.def(nb::init<lsst::geom::Angle const, lsst::geom::Angle const, double const>());

                /* Operators */
                cls.def(
                        "__eq__",
                        [](Observatory const& self, Observatory const& other) { return self == other; },
                        nb::is_operator());
                cls.def(
                        "__ne__",
                        [](Observatory const& self, Observatory const& other) { return self != other; },
                        nb::is_operator());
                cls.def("__str__", &Observatory::toString);
                cls.def("__repr__", &Observatory::toString);

                /* Members */
                cls.def("getLongitude", &Observatory::getLongitude);
                cls.def("getLatitude", &Observatory::getLatitude);
                cls.def("getElevation", &Observatory::getElevation);
                cls.def("setLongitude", &Observatory::setLongitude, "longitude"_a);
                cls.def("setLatitude", &Observatory::setLatitude, "latitude"_a);
                cls.def("setElevation", &Observatory::setElevation, "elevation"_a);
            });
}
}  // namespace coord
}  // namespace afw
}  // namespace lsst
