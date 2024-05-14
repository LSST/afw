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

#include "lsst/afw/math/detail/Convolve.h"

namespace nb = nanobind;

using namespace nb::literals;

namespace lsst {
namespace afw {
namespace math {
namespace detail {

namespace {
template <typename OutImageT, typename InImageT>
void declareByType(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("basicConvolve",
                (void (*)(OutImageT &, InImageT const &, lsst::afw::math::Kernel const &,
                          lsst::afw::math::ConvolutionControl const &))basicConvolve<OutImageT, InImageT>);
        mod.def("basicConvolve",
                (void (*)(OutImageT &, InImageT const &, lsst::afw::math::DeltaFunctionKernel const &,
                          lsst::afw::math::ConvolutionControl const &))basicConvolve<OutImageT, InImageT>);
        mod.def("basicConvolve",
                (void (*)(OutImageT &, InImageT const &, lsst::afw::math::LinearCombinationKernel const &,
                          lsst::afw::math::ConvolutionControl const &))basicConvolve<OutImageT, InImageT>);
        mod.def("basicConvolve",
                (void (*)(OutImageT &, InImageT const &, lsst::afw::math::SeparableKernel const &,
                          lsst::afw::math::ConvolutionControl const &))basicConvolve<OutImageT, InImageT>);
        mod.def("convolveWithBruteForce",
                (void (*)(OutImageT &, InImageT const &, lsst::afw::math::Kernel const &,
                          lsst::afw::math::ConvolutionControl const &))
                        convolveWithBruteForce<OutImageT, InImageT>);
    });
}
template <typename PixelType1, typename PixelType2>
void declareAll(lsst::cpputils::python::WrapperCollection &wrappers) {
    using M1 = image::MaskedImage<PixelType1, image::MaskPixel, image::VariancePixel>;
    using M2 = image::MaskedImage<PixelType2, image::MaskPixel, image::VariancePixel>;

    declareByType<image::Image<PixelType1>, image::Image<PixelType2>>(wrappers);
    declareByType<M1, M2>(wrappers);
}
}  // namespace

void declareConvolve(lsst::cpputils::python::WrapperCollection &wrappers) {
    using PyClass = nb::class_<KernelImagesForRegion>;
    auto clsKernelImagesForRegion =
            wrappers.wrapType(PyClass(wrappers.module, "KernelImagesForRegion"), [](auto &mod, auto &cls) {
                cls.def(nb::init<KernelImagesForRegion::KernelConstPtr, lsst::geom::Box2I const &,
                                 lsst::geom::Point2I const &, bool>(),
                        "kernelPtr"_a, "bbox"_a, "xy0"_a, "doNormalize"_a);
                cls.def(nb::init<KernelImagesForRegion::KernelConstPtr, lsst::geom::Box2I const &,
                                 lsst::geom::Point2I const &, bool, KernelImagesForRegion::ImagePtr,
                                 KernelImagesForRegion::ImagePtr, KernelImagesForRegion::ImagePtr,
                                 KernelImagesForRegion::ImagePtr>(),
                        "kernelPtr"_a, "bbox"_a, "xy0"_a, "doNormalize"_a, "bottomLeftImagePtr"_a,
                        "bottomRightImagePtr"_a, "topLeftImagePtr"_a, "topRightImagePtr"_a);

                cls.def("getBBox", &KernelImagesForRegion::getBBox);
                cls.def("getXY0", &KernelImagesForRegion::getXY0);
                cls.def("getDoNormalize", &KernelImagesForRegion::getDoNormalize);
                cls.def("getImage", &KernelImagesForRegion::getImage);
                cls.def("getKernel", &KernelImagesForRegion::getKernel);
                cls.def("getPixelIndex", &KernelImagesForRegion::getPixelIndex);
                cls.def("computeNextRow", &KernelImagesForRegion::computeNextRow);
                cls.def_static("getMinInterpolationSize", KernelImagesForRegion::getMinInterpolationSize);
            });

    wrappers.wrapType(nb::enum_<KernelImagesForRegion::Location>(clsKernelImagesForRegion, "Location"),
                      [](auto &mod, auto &enm) {
                          enm.value("BOTTOM_LEFT", KernelImagesForRegion::Location::BOTTOM_LEFT);
                          enm.value("BOTTOM_RIGHT", KernelImagesForRegion::Location::BOTTOM_RIGHT);
                          enm.value("TOP_LEFT", KernelImagesForRegion::Location::TOP_LEFT);
                          enm.value("TOP_RIGHT", KernelImagesForRegion::Location::TOP_RIGHT);
                          enm.export_values();
                      });

    wrappers.wrapType(nb::class_<RowOfKernelImagesForRegion>(
                              wrappers.module, "RowOfKernelImagesForRegion"),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<int, int>(), "nx"_a, "ny"_a);

                          cls.def("front", &RowOfKernelImagesForRegion::front);
                          cls.def("back", &RowOfKernelImagesForRegion::back);
                          cls.def("getNX", &RowOfKernelImagesForRegion::getNX);
                          cls.def("getNY", &RowOfKernelImagesForRegion::getNY);
                          cls.def("getYInd", &RowOfKernelImagesForRegion::getYInd);
                          cls.def("getRegion", &RowOfKernelImagesForRegion::getRegion);
                          cls.def("hasData", &RowOfKernelImagesForRegion::hasData);
                          cls.def("isLastRow", &RowOfKernelImagesForRegion::isLastRow);
                          cls.def("incrYInd", &RowOfKernelImagesForRegion::incrYInd);
                      });
}
void wrapConvolve(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.image");
    wrappers.addSignatureDependency("lsst.afw.math");
    declareConvolve(wrappers);
    declareAll<double, double>(wrappers);
    declareAll<double, float>(wrappers);
    declareAll<double, int>(wrappers);
    declareAll<double, std::uint16_t>(wrappers);
    declareAll<float, float>(wrappers);
    declareAll<float, int>(wrappers);
    declareAll<float, std::uint16_t>(wrappers);
    declareAll<int, int>(wrappers);
    declareAll<std::uint16_t, std::uint16_t>(wrappers);
}
}  // namespace detail
}  // namespace math
}  // namespace afw
}  // namespace lsst
