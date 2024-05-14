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

#include <limits>

#include "lsst/afw/image/Color.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace image {

using PyColor = nb::class_<Color>;

void wrapColor(lsst::cpputils::python::WrapperCollection &wrappers) {
    /* Module level */
    wrappers.wrapType(PyColor(wrappers.module, "Color"), [](auto &mod, auto &cls) {
        /* Constructors */
        cls.def(nb::init<double>(), "g_r"_a = std::numeric_limits<double>::quiet_NaN());

        /* Operators */
        cls.def(
                "__eq__", [](Color const &self, Color const &other) { return self == other; },
                nb::is_operator());
        cls.def(
                "__ne__", [](Color const &self, Color const &other) { return self != other; },
                nb::is_operator());

        /* Members */
        cls.def("isIndeterminate", &Color::isIndeterminate);
    });
}

}  // namespace image
}  // namespace afw
}  // namespace lsst
