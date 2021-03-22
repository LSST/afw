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

#include <pybind11/pybind11.h>

#include "lsst/utils/python.h"

#include "lsst/afw/coord/Weather.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace coord {

void wrapWeather(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(py::class_<lsst::afw::coord::Weather>(wrappers.module, "Weather"), [](auto &mod,
                                                                                            auto &cls) {
        /* Constructors */
        cls.def(py::init<double, double, double>(), "airTemperature"_a, "airPressure"_a, "humidity"_a);
        cls.def(py::init<Weather const &>(), "weather"_a);

        /* Operators */
        cls.def(
                "__eq__", [](Weather const &self, Weather const &other) { return self == other; },
                py::is_operator());
        cls.def(
                "__ne__", [](Weather const &self, Weather const &other) { return self != other; },
                py::is_operator());

        /* Members */
        cls.def("getAirPressure", &lsst::afw::coord::Weather::getAirPressure);
        cls.def("getAirTemperature", &lsst::afw::coord::Weather::getAirTemperature);
        cls.def("getHumidity", &lsst::afw::coord::Weather::getHumidity);
        utils::python::addOutputOp(cls, "__str__");
        utils::python::addOutputOp(cls, "__repr__");
    });
}
}  // namespace coord
}  // namespace afw
}  // namespace lsst