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
#include "nanobind/eigen/dense.h"
#include "nanobind/stl/vector.h"
#include <lsst/cpputils/python.h>

#include "lsst/afw/geom/ellipses/BaseCore.h"
#include "lsst/afw/geom/ellipses/Quadrupole.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace ellipses {
void wrapQuadrupole(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(
            nb::class_<Quadrupole, BaseCore>(wrappers.module, "Quadrupole"),
            [](auto &mod, auto &cls) {
                /* Member types and enums */
                using Matrix = Eigen::Matrix<double, 2, 2, Eigen::DontAlign>;

                /* Constructors */
                cls.def(nb::init<double, double, double, bool>(), "ixx"_a = 1.0, "iyy"_a = 1.0, "ixy"_a = 0.0,
                        "normalize"_a = false);
                cls.def(nb::init<BaseCore::ParameterVector const &, bool>(), "vector"_a,
                        "normalize"_a = false);
                cls.def(nb::init<Matrix const &, bool>(), "matrix"_a, "normalize"_a = true);
                cls.def(nb::init<Quadrupole const &>());
                cls.def(nb::init<BaseCore const &>());
                cls.def(nb::init<BaseCore::Convolution const &>());

                nb::implicitly_convertible<BaseCore::Convolution, Quadrupole>();

                /* Operators */
                cls.def(
                        "__eq__", [](Quadrupole &self, Quadrupole &other) { return self == other; },
                        nb::is_operator());
                cls.def(
                        "__ne__", [](Quadrupole &self, Quadrupole &other) { return self != other; },
                        nb::is_operator());

                /* Members */
                cls.def("getIxx", &Quadrupole::getIxx);
                cls.def("getIyy", &Quadrupole::getIyy);
                cls.def("getIxy", &Quadrupole::getIxy);
                cls.def("setIxx", &Quadrupole::setIxx);
                cls.def("setIyy", &Quadrupole::setIyy);
                cls.def("setIxy", &Quadrupole::setIxy);
                cls.def("assign", [](Quadrupole &self, Quadrupole &other) { self = other; });
                cls.def("assign", [](Quadrupole &self, BaseCore &other) { self = other; });
                cls.def("transform", [](Quadrupole &self, lsst::geom::LinearTransform const &t) {
                    return std::static_pointer_cast<Quadrupole>(self.transform(t).copy());
                });
                cls.def("transformInPlace", [](Quadrupole &self, lsst::geom::LinearTransform const &t) {
                    self.transform(t).inPlace();
                });
                // TODO: nanobind based on swig wrapper for now. Hopefully can be removed once nanobind gets
                // smarter handling of implicit conversions
                cls.def("convolve", [](Quadrupole &self, BaseCore const &other) {
                    return Quadrupole(self.convolve(other));
                });
            });
}
}  // namespace ellipses
}  // namespace geom
}  // namespace afw
}  // namespace lsst
