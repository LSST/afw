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

#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/math/Background.h"

namespace nb = nanobind;

using namespace nb::literals;

namespace lsst {
namespace afw {
namespace math {

namespace {
template <typename ImageT>
void declareMakeBackground(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) { mod.def("makeBackground", makeBackground<ImageT>, "img"_a, "bgCtrl"_a); });
}

template <typename PixelT, typename PyClass>
void declareGetImage(PyClass &cls, std::string const &suffix) {
    cls.def(("getImage" + suffix).c_str(),
            (std::shared_ptr<lsst::afw::image::Image<PixelT>>(Background::*)(Interpolate::Style const,
                                                                             UndersampleStyle const) const) &
                    Background::getImage<PixelT>,
            "interpStyle"_a, "undersampleStyle"_a = THROW_EXCEPTION);
    cls.def(("getImage" + suffix).c_str(),
            (std::shared_ptr<lsst::afw::image::Image<PixelT>>(Background::*)(std::string const &,
                                                                             std::string const &) const) &
                    Background::getImage<PixelT>,
            "interpStyle"_a, "undersampleStyle"_a = "THROW_EXCEPTION");
    cls.def(("getImage" + suffix).c_str(),
            (std::shared_ptr<lsst::afw::image::Image<PixelT>>(Background::*)(
                    lsst::geom::Box2I const &, Interpolate::Style const, UndersampleStyle const) const) &
                    Background::getImage<PixelT>,
            "bbox"_a, "interpStyle"_a, "undersampleStyle"_a = THROW_EXCEPTION);
    cls.def(("getImage" + suffix).c_str(),
            (std::shared_ptr<lsst::afw::image::Image<PixelT>>(Background::*)(
                    lsst::geom::Box2I const &, std::string const &, std::string const &) const) &
                    Background::getImage<PixelT>,
            "bbox"_a, "interpStyle"_a, "undersampleStyle"_a = "THROW_EXCEPTION");
    cls.def(("getImage" + suffix).c_str(),
            (std::shared_ptr<lsst::afw::image::Image<PixelT>>(Background::*)() const) &
                    Background::getImage<PixelT>);
}
}  // namespace

void declareBackground(lsst::cpputils::python::WrapperCollection &wrappers) {
    /* Member types and enums */
    wrappers.wrapType(nb::enum_<UndersampleStyle>(wrappers.module, "UndersampleStyle"),
                      [](auto &mod, auto &enm) {
                          enm.value("THROW_EXCEPTION", UndersampleStyle::THROW_EXCEPTION);
                          enm.value("REDUCE_INTERP_ORDER", UndersampleStyle::REDUCE_INTERP_ORDER);
                          enm.value("INCREASE_NXNYSAMPLE", UndersampleStyle::INCREASE_NXNYSAMPLE);
                          enm.export_values();
                      });

    using PyBackgroundControl = nb::class_<BackgroundControl>;
    wrappers.wrapType(PyBackgroundControl(wrappers.module, "BackgroundControl"), [](auto &mod, auto &cls) {
        /* Constructors */
        cls.def(nb::init<int const, int const, StatisticsControl const, Property const,
                         ApproximateControl const>(),
                "nxSample"_a, "nySample"_a, "sctrl"_a = StatisticsControl(), "prop"_a = MEANCLIP,
                "actrl"_a = ApproximateControl(ApproximateControl::UNKNOWN, 1));
        cls.def(nb::init<int const, int const, StatisticsControl const, std::string const &,
                         ApproximateControl const>(),
                "nxSample"_a, "nySample"_a, "sctrl"_a, "prop"_a,
                "actrl"_a = ApproximateControl(ApproximateControl::UNKNOWN, 1));
        cls.def(nb::init<Interpolate::Style const, int const, int const, UndersampleStyle const,
                         StatisticsControl const, Property const, ApproximateControl const>(),
                "style"_a, "nxSample"_a = 10, "nySample"_a = 10, "undersampleStyle"_a = THROW_EXCEPTION,
                "sctrl"_a = StatisticsControl(), "prop"_a = MEANCLIP,
                "actrl"_a = ApproximateControl(ApproximateControl::UNKNOWN, 1));
        cls.def(nb::init<std::string const &, int const, int const, std::string const &,
                         StatisticsControl const, std::string const &, ApproximateControl const>(),
                "style"_a, "nxSample"_a = 10, "nySample"_a = 10, "undersampleStyle"_a = "THROW_EXCEPTION",
                "sctrl"_a = StatisticsControl(), "prop"_a = "MEANCLIP",
                "actrl"_a = ApproximateControl(ApproximateControl::UNKNOWN, 1));

        /* Members */
        cls.def("setNxSample", &BackgroundControl::setNxSample);
        cls.def("setNySample", &BackgroundControl::setNySample);
        cls.def("setInterpStyle",
                (void (BackgroundControl::*)(Interpolate::Style const)) & BackgroundControl::setInterpStyle);
        cls.def("setInterpStyle",
                (void (BackgroundControl::*)(std::string const &)) & BackgroundControl::setInterpStyle);
        cls.def("setUndersampleStyle", (void (BackgroundControl::*)(UndersampleStyle const)) &
                                               BackgroundControl::setUndersampleStyle);
        cls.def("setUndersampleStyle",
                (void (BackgroundControl::*)(std::string const &)) & BackgroundControl::setUndersampleStyle);
        cls.def("getNxSample", &BackgroundControl::getNxSample);
        cls.def("getNySample", &BackgroundControl::getNySample);
        cls.def("getInterpStyle", &BackgroundControl::getInterpStyle);
        cls.def("getUndersampleStyle", &BackgroundControl::getUndersampleStyle);
        cls.def("getStatisticsControl", (std::shared_ptr<StatisticsControl>(BackgroundControl::*)()) &
                                                BackgroundControl::getStatisticsControl);
        cls.def("getStatisticsProperty", &BackgroundControl::getStatisticsProperty);
        cls.def("setStatisticsProperty",
                (void (BackgroundControl::*)(Property)) & BackgroundControl::setStatisticsProperty);
        cls.def("setStatisticsProperty",
                (void (BackgroundControl::*)(std::string)) & BackgroundControl::setStatisticsProperty);
        cls.def("setApproximateControl", &BackgroundControl::setApproximateControl);
        cls.def("getApproximateControl", (std::shared_ptr<ApproximateControl>(BackgroundControl::*)()) &
                                                 BackgroundControl::getApproximateControl);
    });
    using PyBackground = nb::class_<Background>;
    wrappers.wrapType(PyBackground(wrappers.module, "Background"), [](auto &mod, auto &cls) {
        /* Members */
        declareGetImage<float>(cls, "F");

        cls.def("getAsUsedInterpStyle", &Background::getAsUsedInterpStyle);
        cls.def("getAsUsedUndersampleStyle", &Background::getAsUsedUndersampleStyle);
        cls.def("getApproximate", &Background::getApproximate, "actrl"_a,
                "undersampleStyle"_a = THROW_EXCEPTION);
        cls.def("getBackgroundControl",
                (std::shared_ptr<BackgroundControl>(Background::*)()) & Background::getBackgroundControl);
    });

    using PyBackgroundMI = nb::class_<BackgroundMI, Background>;
    wrappers.wrapType(PyBackgroundMI(wrappers.module, "BackgroundMI"), [](auto &mod, auto &cls) {
        /* Constructors */
        cls.def(nb::init<lsst::geom::Box2I const,
                         image::MaskedImage<typename Background::InternalPixelT> const &>(),
                "imageDimensions"_a, "statsImage"_a);

        /* Operators */
        cls.def("__iadd__", &BackgroundMI::operator+=, nb::rv_policy::none);
        cls.def("__isub__", &BackgroundMI::operator-=, nb::rv_policy::none);

        /* Members */
        cls.def("getStatsImage", &BackgroundMI::getStatsImage);
        cls.def("getImageBBox", &BackgroundMI::getImageBBox);

        // Yes, really only float
    });
}
void wrapBackground(lsst::cpputils::python::WrapperCollection &wrappers) {
    // FIXME: review when lsst.afw.image is converted to python wrappers
    wrappers.addInheritanceDependency("lsst.afw.image");
    declareBackground(wrappers);
    declareMakeBackground<image::Image<float>>(wrappers);
    declareMakeBackground<image::MaskedImage<float>>(wrappers);
    wrappers.wrap(
            [](auto &mod) { mod.def("stringToUndersampleStyle", stringToUndersampleStyle, "style"_a); });
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
