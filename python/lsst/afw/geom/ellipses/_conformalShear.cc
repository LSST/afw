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

#include "lsst/afw/geom/ellipses/ConformalShear.h"
#include "lsst/afw/geom/ellipses/EllipticityBase.h"

namespace nb = nanobind;

using namespace nb::literals;
namespace lsst {
namespace afw {
namespace geom {
namespace ellipses {

void wrapConformalShear(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::class_<ConformalShear, detail::EllipticityBase>(wrappers.module, "ConformalShear"),
                      [](auto &mod, auto &cls) {
                          /* Constructors */
                          cls.def(nb::init<std::complex<double> const &>());
                          cls.def(nb::init<double, double>(), "e1"_a = 0.0, "e2"_a = 0.0);

                          /* Members */
                          //    cls.def("dAssign", (Jacobian (ConformalShear::*)(Distortion const &))
                          //    &ConformalShear::dAssign); cls.def("dAssign", (Jacobian
                          //    (ConformalShear::*)(ReducedShear const &)) &ConformalShear::dAssign);
                          cls.def("getAxisRatio", &ConformalShear::getAxisRatio);
                          cls.def("normalize", &ConformalShear::normalize);
                          cls.def("getName", &ConformalShear::getName);
                          cls.def("__repr__", [](ConformalShear const &self) {
                              return nb::str("{}({}, {})").format(self.getName(), self.getE1(), self.getE2());
                          });
                      });
}
}  // namespace ellipses
}  // namespace geom
}  // namespace afw
}  // namespace lsst
