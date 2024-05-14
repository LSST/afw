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
#include <nanobind/stl/shared_ptr.h>

#include "ndarray/nanobind.h"

#include "lsst/afw/math/Interpolate.h"

namespace nb = nanobind;
using namespace nanobind::literals;

using namespace lsst::afw::math;
namespace lsst {
namespace afw {
namespace math {
void wrapInterpolate(lsst::cpputils::python::WrapperCollection &wrappers) {
    using PyClass = nb::class_<Interpolate>;
    auto clsdef = PyClass(wrappers.module, "Interpolate");
    wrappers.wrapType(nb::enum_<Interpolate::Style>(clsdef, "Style"), [](auto &mod, auto &enm) {
        enm.value("UNKNOWN", Interpolate::Style::UNKNOWN);
        enm.value("CONSTANT", Interpolate::Style::CONSTANT);
        enm.value("LINEAR", Interpolate::Style::LINEAR);
        enm.value("NATURAL_SPLINE", Interpolate::Style::NATURAL_SPLINE);
        enm.value("CUBIC_SPLINE", Interpolate::Style::CUBIC_SPLINE);
        enm.value("CUBIC_SPLINE_PERIODIC", Interpolate::Style::CUBIC_SPLINE_PERIODIC);
        enm.value("AKIMA_SPLINE", Interpolate::Style::AKIMA_SPLINE);
        enm.value("AKIMA_SPLINE_PERIODIC", Interpolate::Style::AKIMA_SPLINE_PERIODIC);
        enm.value("NUM_STYLES", Interpolate::Style::NUM_STYLES);
        enm.export_values();
        });
    auto clsInterpolate = wrappers.wrapType(clsdef, [&](auto &mod, auto &cls) {
        cls.def("interpolate", [](Interpolate &t, double const x) { 
            /*
            We use a lambda function here because interpolate (with a double) is a virtual function
            and therefor cannot be wrapped directly.
            */
            return t.interpolate(x);
        });

        cls.def("interpolate", (std::vector<double>(Interpolate::*)(std::vector<double> const &) const) &
                                       Interpolate::interpolate);
        cls.def("interpolate",
                (ndarray::Array<double, 1>(Interpolate::*)(ndarray::Array<double const, 1> const &) const) &
                        Interpolate::interpolate);

        mod.def("makeInterpolate",
                (std::shared_ptr<Interpolate>(*)(std::vector<double> const &, std::vector<double> const &,
                                                 Interpolate::Style const))makeInterpolate,
                "x"_a, "y"_a, "style"_a = Interpolate::AKIMA_SPLINE);
        mod.def("makeInterpolate",
                (std::shared_ptr<Interpolate>(*)(ndarray::Array<double const, 1> const &,
                                                 ndarray::Array<double const, 1> const &y,
                                                 Interpolate::Style const))makeInterpolate,
                "x"_a, "y"_a, "style"_a = Interpolate::AKIMA_SPLINE);

        mod.def("stringToInterpStyle", stringToInterpStyle, "style"_a);
        mod.def("lookupMaxInterpStyle", lookupMaxInterpStyle, "n"_a);
        mod.def("lookupMinInterpPoints", lookupMinInterpPoints, "style"_a);
    });
#if 0
    wrappers.wrapType(nb::enum_<Interpolate::Style>(clsInterpolate, "Style"), [](auto &mod, auto &enm) {
        enm.value("UNKNOWN", Interpolate::Style::UNKNOWN);
        enm.value("CONSTANT", Interpolate::Style::CONSTANT);
        enm.value("LINEAR", Interpolate::Style::LINEAR);
        enm.value("NATURAL_SPLINE", Interpolate::Style::NATURAL_SPLINE);
        enm.value("CUBIC_SPLINE", Interpolate::Style::CUBIC_SPLINE);
        enm.value("CUBIC_SPLINE_PERIODIC", Interpolate::Style::CUBIC_SPLINE_PERIODIC);
        enm.value("AKIMA_SPLINE", Interpolate::Style::AKIMA_SPLINE);
        enm.value("AKIMA_SPLINE_PERIODIC", Interpolate::Style::AKIMA_SPLINE_PERIODIC);
        enm.value("NUM_STYLES", Interpolate::Style::NUM_STYLES);
        enm.export_values();
    });
#endif    
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
