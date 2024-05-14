/*
 * LSST Data Management System
 * Copyright 2008-2016  AURA/LSST.
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

#include "ndarray/nanobind.h"

#include "lsst/afw/math/LeastSquares.h"

namespace nb = nanobind;
using namespace nanobind::literals;

using namespace lsst::afw::math;
namespace lsst {
namespace afw {
namespace math {
namespace {
template <typename T1, typename T2, int C1, int C2>
void declareLeastSquares(lsst::cpputils::python::WrapperCollection &wrappers) {
   auto clsLeastSquares = nb::class_<LeastSquares>(wrappers.module, "LeastSquares"); 
    wrappers.wrapType(nb::enum_<LeastSquares::Factorization>(clsLeastSquares, "Factorization"),
                      [](auto &mod, auto &enm) {
                          enm.value("NORMAL_EIGENSYSTEM", LeastSquares::Factorization::NORMAL_EIGENSYSTEM);
                          enm.value("NORMAL_CHOLESKY", LeastSquares::Factorization::NORMAL_CHOLESKY);
                          enm.value("DIRECT_SVD", LeastSquares::Factorization::DIRECT_SVD);
                          enm.export_values();
                      });	
    wrappers.wrapType(clsLeastSquares, [](auto &mod, auto &cls) {
                cls.def_static(
                        "fromDesignMatrix",
                        (LeastSquares(*)(ndarray::Array<T1, 2, C1> const &, ndarray::Array<T2, 1, C2> const &,
                                         LeastSquares::Factorization)) &
                                LeastSquares::fromDesignMatrix<T1, T2, C1, C2>,
                        "design"_a, "data"_a, "factorization"_a = LeastSquares::NORMAL_EIGENSYSTEM);
                cls.def_static(
                        "fromNormalEquations",
                        (LeastSquares(*)(ndarray::Array<T1, 2, C1> const &, ndarray::Array<T2, 1, C2> const &,
                                         LeastSquares::Factorization)) &
                                LeastSquares::fromNormalEquations<T1, T2, C1, C2>,
                        "fisher"_a, "rhs"_a, "factorization"_a = LeastSquares::NORMAL_EIGENSYSTEM);
                cls.def("getRank", &LeastSquares::getRank);
                cls.def("setDesignMatrix", (void (LeastSquares::*)(ndarray::Array<T1, 2, C1> const &,
                                                                   ndarray::Array<T2, 1, C2> const &)) &
                                                   LeastSquares::setDesignMatrix<T1, T2, C1, C2>);
                cls.def("getDimension", &LeastSquares::getDimension);
                cls.def("setNormalEquations", (void (LeastSquares::*)(ndarray::Array<T1, 2, C1> const &,
                                                                      ndarray::Array<T2, 1, C2> const &)) &
                                                      LeastSquares::setNormalEquations<T1, T2, C1, C2>);
                cls.def("getSolution", &LeastSquares::getSolution);
                cls.def("getFisherMatrix", &LeastSquares::getFisherMatrix);
                cls.def("getCovariance", &LeastSquares::getCovariance);
                cls.def("getFactorization", &LeastSquares::getFactorization);
                cls.def("getDiagnostic", &LeastSquares::getDiagnostic);
                cls.def("getThreshold", &LeastSquares::getThreshold);
                cls.def("setThreshold", &LeastSquares::setThreshold);
            });
};
}  // namespace

void wrapLeastSquares(lsst::cpputils::python::WrapperCollection &wrappers) {
    declareLeastSquares<double, double, 0, 0>(wrappers);
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
