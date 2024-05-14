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

#include <sstream>

#include <nanobind/nanobind.h>

#include "lsst/cpputils/python.h"

#include "lsst/afw/coord/Weather.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace coord {

void wrapWeather(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::class_<lsst::afw::coord::Weather>(wrappers.module, "Weather"), [](auto &mod,
                                                                                            auto &cls) {
        /* Constructors */
        cls.def(nb::init<double, double, double>(), "airTemperature"_a, "airPressure"_a, "humidity"_a);
        cls.def(nb::init<Weather const &>(), "weather"_a);

        /* Operators */
        cls.def(
                "__eq__", [](Weather const &self, Weather const &other) { return self == other; },
                nb::is_operator());
        cls.def(
                "__ne__", [](Weather const &self, Weather const &other) { return self != other; },
                nb::is_operator());

        /* Members */
        cls.def("getAirPressure", &lsst::afw::coord::Weather::getAirPressure);
        cls.def("getAirTemperature", &lsst::afw::coord::Weather::getAirTemperature);
        cls.def("getHumidity", &lsst::afw::coord::Weather::getHumidity);
        cpputils::python::addOutputOp(cls, "__str__");
        cpputils::python::addOutputOp(cls, "__repr__");
    });
}
}  // namespace coord
}  // namespace afw
}  // namespace lsst
