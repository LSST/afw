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

#include "ndarray/nanobind.h"

#include "lsst/afw/geom/ellipses/BaseCore.h"
#include "lsst/afw/geom/ellipses/Convolution.h"
#include "lsst/afw/geom/ellipses/Transformer.h"

namespace nb = nanobind;
namespace lsst {
namespace afw {
namespace geom {
namespace ellipses {
void wrapBaseCore(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::class_<BaseCore>(wrappers.module, "BaseCore"),
                      [](auto &mod, auto &cls) {
                          cls.def("__eq__", &BaseCore::operator==, nb::is_operator());
                          cls.def("__nq__", &BaseCore::operator!=, nb::is_operator());
                          /* Members */
                          cls.def("getName", &BaseCore::getName);
                          cls.def("clone", &BaseCore::clone);
                          cls.def("normalize", &BaseCore::normalize);
                          cls.def("grow", &BaseCore::grow);
                          cls.def("scale", &BaseCore::scale);
                          cls.def("getArea", &BaseCore::getArea);
                          cls.def("getDeterminantRadius", &BaseCore::getDeterminantRadius);
                          cls.def("getTraceRadius", &BaseCore::getTraceRadius);
                          cls.def("convolve", (BaseCore::Convolution(BaseCore::*)(BaseCore const &)) &
                                                      BaseCore::convolve);
                          cls.def("computeDimensions", &BaseCore::computeDimensions);
                          cls.def("getParameterVector", &BaseCore::getParameterVector);
                          cls.def("setParameterVector", &BaseCore::setParameterVector);
                      });
}
}  // namespace ellipses
}  // namespace geom
}  // namespace afw
}  // namespace lsst
