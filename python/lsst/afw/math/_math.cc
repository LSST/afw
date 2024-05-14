/*
 * LSST Data Management System
 * Copyright 2008-2021  AURA/LSST.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */

#include <nanobind/nanobind.h>
#include <lsst/cpputils/python.h>

namespace lsst {
namespace afw {
namespace math {

void wrapApproximate(lsst::cpputils::python::WrapperCollection &);
void wrapBackground(lsst::cpputils::python::WrapperCollection &);
void wrapBoundedField(lsst::cpputils::python::WrapperCollection &);
void wrapChebyshevBoundedField(lsst::cpputils::python::WrapperCollection &);
void wrapConvolveImage(lsst::cpputils::python::WrapperCollection &);
void wrapFunction(lsst::cpputils::python::WrapperCollection &);
void wrapFunctionLibrary(lsst::cpputils::python::WrapperCollection &);
void wrapGaussianProcess(lsst::cpputils::python::WrapperCollection &);
void wrapInterpolate(lsst::cpputils::python::WrapperCollection &);
void wrapKernel(lsst::cpputils::python::WrapperCollection &);
void wrapLeastSquares(lsst::cpputils::python::WrapperCollection &);
void wrapMinimize(lsst::cpputils::python::WrapperCollection &);
void wrapOffsetImage(lsst::cpputils::python::WrapperCollection &);
void wrapPixelAreaBoundedField(lsst::cpputils::python::WrapperCollection &);
void wrapProductBoundedField(lsst::cpputils::python::WrapperCollection &);
void wrapRandom(lsst::cpputils::python::WrapperCollection &);
void wrapSpatialCell(lsst::cpputils::python::WrapperCollection &);
void wrapStack(lsst::cpputils::python::WrapperCollection &);
void wrapStatistics(lsst::cpputils::python::WrapperCollection &);
void wrapTransformBoundedField(lsst::cpputils::python::WrapperCollection &);
void wrapWarpExposure(lsst::cpputils::python::WrapperCollection &);

NB_MODULE(_math, mod) {
    lsst::cpputils::python::WrapperCollection wrappers(mod, "lsst.afw.math");
    wrapFunction(wrappers);
    wrapFunctionLibrary(wrappers);
    wrapApproximate(wrappers);
    wrapStatistics(wrappers);
    wrapBackground(wrappers);
    wrapBoundedField(wrappers);
    wrapChebyshevBoundedField(wrappers);
    wrapConvolveImage(wrappers);
    wrapGaussianProcess(wrappers);
    wrapInterpolate(wrappers);
    wrapKernel(wrappers);
    wrapLeastSquares(wrappers);
    wrapMinimize(wrappers);
    wrapOffsetImage(wrappers);
    wrapPixelAreaBoundedField(wrappers);
    wrapProductBoundedField(wrappers);
    wrapRandom(wrappers);
    wrapSpatialCell(wrappers);
    wrapStack(wrappers);
    wrapTransformBoundedField(wrappers);
    wrapWarpExposure(wrappers);
    wrappers.finish();
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
