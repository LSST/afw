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
#include <nanobind/stl/vector.h>

#include "lsst/afw/math/minimize.h"

namespace nb = nanobind;

using namespace lsst::afw::math;
namespace lsst {
namespace afw {
namespace math {
namespace {
template <typename ReturnT>
void declareFitResults(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("minimize", (FitResults(*)(lsst::afw::math::Function1<ReturnT> const &,
                                           std::vector<double> const &, std::vector<double> const &,
                                           std::vector<double> const &, std::vector<double> const &,
                                           std::vector<double> const &, double))minimize<ReturnT>);
        mod.def("minimize",
                (FitResults(*)(lsst::afw::math::Function2<ReturnT> const &, std::vector<double> const &,
                               std::vector<double> const &, std::vector<double> const &,
                               std::vector<double> const &, std::vector<double> const &,
                               std::vector<double> const &, double))minimize<ReturnT>);
    });
};

void declareMinimize(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::class_<FitResults>(wrappers.module, "FitResults"), [](auto &mod, auto &cls) {
        cls.def_rw("isValid", &FitResults::isValid);
        cls.def_rw("chiSq", &FitResults::chiSq);
        cls.def_rw("parameterList", &FitResults::parameterList);
        cls.def_rw("parameterErrorList", &FitResults::parameterErrorList);
    });
}
}  // namespace
void wrapMinimize(lsst::cpputils::python::WrapperCollection &wrappers) {
    declareMinimize(wrappers);
    declareFitResults<double>(wrappers);
    declareFitResults<float>(wrappers);
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
