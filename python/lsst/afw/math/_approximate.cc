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

#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/math/Approximate.h"

namespace nb = nanobind;

using namespace nb::literals;

namespace lsst {
namespace afw {
namespace math {

namespace {
template <typename PixelT>
void declareApproximate(lsst::cpputils::python::WrapperCollection &wrappers, std::string const &suffix) {
    using Class = Approximate<PixelT>;

    wrappers.wrapType(
            nb::class_<Class>(wrappers.module, ("Approximate" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def("getImage", &Class::getImage, "orderX"_a = -1, "orderY"_a = -1);
                cls.def("getMaskedImage", &Class::getMaskedImage, "orderX"_a = -1, "orderY"_a = -1);

                mod.def("makeApproximate",
                        (std::shared_ptr<Approximate<PixelT>>(*)(
                                std::vector<double> const &, std::vector<double> const &,
                                image::MaskedImage<PixelT> const &, lsst::geom::Box2I const &,
                                ApproximateControl const &))makeApproximate<PixelT>,
                        "x"_a, "y"_a, "im"_a, "bbox"_a, "ctrl"_a);
            });
}
void declareApproximate(lsst::cpputils::python::WrapperCollection &wrappers) {
    auto control =
            wrappers.wrapType(nb::class_<ApproximateControl>(
                                      wrappers.module, "ApproximateControl"),
                              [](auto &mod, auto &cls) {
                                  cls.def(nb::init<ApproximateControl::Style, int, int, bool>(), "style"_a,
                                          "orderX"_a, "orderY"_a = -1, "weighting"_a = true);

                                  cls.def("getStyle", &ApproximateControl::getStyle);
                                  cls.def("setStyle", &ApproximateControl::setStyle);
                                  cls.def("getOrderX", &ApproximateControl::getOrderX);
                                  cls.def("setOrderX", &ApproximateControl::setOrderX);
                                  cls.def("getOrderY", &ApproximateControl::getOrderY);
                                  cls.def("setOrderY", &ApproximateControl::setOrderY);
                                  cls.def("getWeighting", &ApproximateControl::getWeighting);
                                  cls.def("setWeighting", &ApproximateControl::setWeighting);
                              });
    wrappers.wrapType(nb::enum_<ApproximateControl::Style>(control, "Style"), [](auto &mod, auto &enm) {
        enm.value("UNKNOWN", ApproximateControl::Style::UNKNOWN);
        enm.value("CHEBYSHEV", ApproximateControl::Style::CHEBYSHEV);
        enm.value("NUM_STYLES", ApproximateControl::Style::NUM_STYLES);
        enm.export_values();
    });
    declareApproximate<float>(wrappers, "F");
}
}  // namespace
void wrapApproximate(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.image");
    declareApproximate(wrappers);
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
