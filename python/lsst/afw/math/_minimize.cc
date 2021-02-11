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

#include <pybind11/pybind11.h>
#include <lsst/utils/python.h>
#include <pybind11/stl.h>

#include "lsst/afw/math/minimize.h"

namespace py = pybind11;

using namespace lsst::afw::math;
namespace lsst {
namespace afw {
namespace math {
namespace {
template <typename ReturnT>
void declareFitResults(lsst::utils::python::WrapperCollection &wrappers) {
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

void declareMinimize(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(py::class_<FitResults>(wrappers.module, "FitResults"), [](auto &mod, auto &cls) {
        cls.def_readwrite("isValid", &FitResults::isValid);
        cls.def_readwrite("chiSq", &FitResults::chiSq);
        cls.def_readwrite("parameterList", &FitResults::parameterList);
        cls.def_readwrite("parameterErrorList", &FitResults::parameterErrorList);
    });
}
}  // namespace
void wrapMinimize(lsst::utils::python::WrapperCollection &wrappers) {
    declareMinimize(wrappers);
    declareFitResults<double>(wrappers);
    declareFitResults<float>(wrappers);
}
}  // namespace math
}  // namespace afw
}  // namespace lsst