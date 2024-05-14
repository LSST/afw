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
#include "nanobind/stl/vector.h"
#include <lsst/cpputils/python.h>

#include "ndarray/nanobind.h"

#include "lsst/geom/Point.h"
#include "lsst/geom/AffineTransform.h"
#include "lsst/afw/geom/transformFactory.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace {

void declareTransformFactory(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("linearizeTransform",
                (lsst::geom::AffineTransform(*)(TransformPoint2ToPoint2 const &,
                                                lsst::geom::Point2D const &)) &
                        linearizeTransform,
                "original"_a, "point"_a);
        mod.def("makeTransform",
                (std::shared_ptr<TransformPoint2ToPoint2>(*)(lsst::geom::AffineTransform const &)) &
                        makeTransform,
                "affine"_a);
        mod.def("makeRadialTransform",
                (std::shared_ptr<TransformPoint2ToPoint2>(*)(std::vector<double> const &)) &
                        makeRadialTransform,
                "coeffs"_a);
        mod.def("makeRadialTransform",
                (std::shared_ptr<TransformPoint2ToPoint2>(*)(std::vector<double> const &,
                                                             std::vector<double> const &)) &
                        makeRadialTransform,
                "forwardCoeffs"_a, "inverseCoeffs"_a);
        mod.def("makeIdentityTransform", &makeIdentityTransform);
    });
}
}  // namespace
void wrapTransformFactory(lsst::cpputils::python::WrapperCollection &wrappers) {
    declareTransformFactory(wrappers);
    wrappers.addSignatureDependency("astshim");
}
}  // namespace geom
}  // namespace afw
}  // namespace lsst
