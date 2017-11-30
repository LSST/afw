/*
 * LSST Data Management System
 * See COPYRIGHT file at the top of the source tree.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program. If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

#include "numpy/arrayobject.h"
#include "ndarray/pybind11.h"

#include "lsst/afw/geom/Angle.h"
#include "lsst/afw/geom/Extent.h"
#include "lsst/afw/geom/Endpoint.h"
#include "lsst/afw/geom/transformFactory.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace {

PYBIND11_PLUGIN(transformFactory) {
    py::module mod("transformFactory");

    py::module::import("lsst.afw.geom.transform");

    // Need to import numpy for ndarray and eigen conversions
    if (_import_array() < 0) {
        PyErr_SetString(PyExc_ImportError, "numpy.core.multiarray failed to import");
        return nullptr;
    }

    mod.def("linearizeTransform",
            (AffineTransform(*)(TransformPoint2ToPoint2 const &, Point2D const &)) & linearizeTransform,
            "original"_a, "point"_a);
    mod.def("makeTransform", (TransformPoint2ToPoint2(*)(AffineTransform const &)) & makeTransform,
            "affine"_a);
    mod.def("makeRadialTransform",
            (TransformPoint2ToPoint2(*)(std::vector<double> const &)) & makeRadialTransform, "coeffs"_a);
    mod.def("makeRadialTransform",
            (TransformPoint2ToPoint2(*)(std::vector<double> const &, std::vector<double> const &)) &
                    makeRadialTransform,
            "forwardCoeffs"_a, "inverseCoeffs"_a);
    mod.def("makeIdentityTransform", &makeIdentityTransform);
    mod.def("makeAffineTransformPoint2", &makeAffineTransformPoint2, "offset"_a = Extent2D(0, 0),
            "rotation"_a = 0 * radians, "scale"_a = 1.0);

    return mod.ptr();
}

}  // namespace
}  // namespace geom
}  // namespace afw
}  // namespace lsst
