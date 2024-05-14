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
#include "nanobind/ndarray.h"
#include "lsst/cpputils/python.h"

#include <memory>
#include <vector>

#include "ndarray/nanobind.h"

#include "lsst/afw/image/Calib.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace image {
namespace {

template <typename T>
void declareVectorOperations(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        using Array = ndarray::Array<T, 1>;
        using ConstArray = ndarray::Array<const T, 1>;
        mod.def("abMagFromFlux", (Array(*)(ConstArray const &)) & abMagFromFlux<T>, "flux"_a, nb::rv_policy::automatic_reference);
        mod.def("abMagErrFromFluxErr",
                (Array(*)(ConstArray const &, ConstArray const &)) & abMagErrFromFluxErr<T>, "fluxErr"_a,
                "flux"_a);
        mod.def("fluxFromABMag", (Array(*)(ConstArray const &)) & fluxFromABMag<T>, "mag"_a);
        mod.def("fluxErrFromABMagErr",
                (Array(*)(ConstArray const &, ConstArray const &)) & fluxErrFromABMagErr<T>, "magErr"_a,
                "mag"_a);
    });
}

void declareCalib(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("abMagFromFlux", (double (*)(double)) & abMagFromFlux, "flux"_a);
        mod.def("abMagErrFromFluxErr", (double (*)(double, double)) & abMagErrFromFluxErr, "fluxErr"_a,
                "flux"_a);
        mod.def("fluxFromABMag", (double (*)(double)) & fluxFromABMag, "mag"_a);
        mod.def("fluxErrFromABMagErr", (double (*)(double, double)) & fluxErrFromABMagErr, "magErr"_a,
                "mag"_a);
    });
}
}  // namespace
void wrapCalib(lsst::cpputils::python::WrapperCollection &wrappers) {
    /* Module level */
    declareCalib(wrappers);
    //declareVectorOperations<float>(wrappers);
    declareVectorOperations<double>(wrappers);
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
