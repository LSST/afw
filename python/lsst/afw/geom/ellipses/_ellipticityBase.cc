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

#include "lsst/afw/geom/ellipses/EllipticityBase.h"

namespace nb = nanobind;

using namespace nb::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace ellipses {
void wrapEllipticityBase(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::class_<detail::EllipticityBase>(wrappers.module, "EllipticityBase"),
                      [](auto &mod, auto &cls) {
                          /* Member types and enums */
                          nb::enum_<detail::EllipticityBase::ParameterEnum>(cls, "ParameterEnum")
                                  .value("E1", detail::EllipticityBase::ParameterEnum::E1)
                                  .value("E2", detail::EllipticityBase::ParameterEnum::E2)
                                  .export_values();

                          /* Members */
                          cls.def("getComplex", (std::complex<double> & (detail::EllipticityBase::*)()) &
                                                        detail::EllipticityBase::getComplex);
                          cls.def("setComplex", &detail::EllipticityBase::setComplex);
                          cls.def("getE1", &detail::EllipticityBase::getE1);
                          cls.def("setE1", &detail::EllipticityBase::setE1);
                          cls.def("getE2", &detail::EllipticityBase::getE2);
                          cls.def("setE2", &detail::EllipticityBase::setE2);
                          cls.def("getTheta", &detail::EllipticityBase::getTheta);
                          cls.def("__str__", [](detail::EllipticityBase const &self) {
                              return nb::str("({}, {})").format(self.getE1(), self.getE2());
                          });
                      });
}
}  // namespace ellipses
}  // namespace geom
}  // namespace afw
}  // namespace lsst
