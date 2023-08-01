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

#include "lsst/afw/math/ConvolveImage.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace math {

namespace {
template <typename OutImageT, typename InImageT, typename KernelT>
void declareConvolve(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("convolve",
                (void (*)(OutImageT &, InImageT const &, KernelT const &,
                          ConvolutionControl const &))convolve<OutImageT, InImageT, KernelT>,
                "convolvedImage"_a, "inImage"_a, "kernel"_a, "convolutionControl"_a = ConvolutionControl());
        mod.def("convolve",
                (void (*)(OutImageT &, InImageT const &, KernelT const &, bool,
                          bool))convolve<OutImageT, InImageT, KernelT>,
                "convolvedImage"_a, "inImage"_a, "kernel"_a, "doNormalize"_a, "doCopyEdge"_a = false);
    });
}

template <typename ImageType1, typename ImageType2>
void declareScaledPlus(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("scaledPlus",
                (void (*)(ImageType1 &, double, ImageType2 const &, double, ImageType2 const &))scaledPlus);
    });
}

template <typename ImageType1, typename ImageType2>
void declareByType(lsst::utils::python::WrapperCollection &wrappers) {
    declareConvolve<ImageType1, ImageType2, AnalyticKernel>(wrappers);
    declareConvolve<ImageType1, ImageType2, DeltaFunctionKernel>(wrappers);
    declareConvolve<ImageType1, ImageType2, FixedKernel>(wrappers);
    declareConvolve<ImageType1, ImageType2, LinearCombinationKernel>(wrappers);
    declareConvolve<ImageType1, ImageType2, SeparableKernel>(wrappers);
    declareConvolve<ImageType1, ImageType2, Kernel>(wrappers);
    declareScaledPlus<ImageType1, ImageType2>(wrappers);
}

template <typename PixelType1, typename PixelType2>
void declareAll(lsst::utils::python::WrapperCollection &wrappers) {
    using M1 = image::MaskedImage<PixelType1, image::MaskPixel, image::VariancePixel>;
    using M2 = image::MaskedImage<PixelType2, image::MaskPixel, image::VariancePixel>;

    declareByType<image::Image<PixelType1>, image::Image<PixelType2>>(wrappers);
    declareByType<M1, M2>(wrappers);
}

void declareConvolveImage(lsst::utils::python::WrapperCollection &wrappers) {
    using PyClass = py::class_<ConvolutionControl>;
    wrappers.wrapType(PyClass(wrappers.module, "ConvolutionControl"), [](auto &mod, auto &clsl) {
        clsl.def(py::init<bool, bool, int>(), "doNormalize"_a = true, "doCopyEdge"_a = false,
                 "maxInterpolationDistance"_a = 10);

        clsl.def("getDoNormalize", &ConvolutionControl::getDoNormalize);
        clsl.def("getDoCopyEdge", &ConvolutionControl::getDoCopyEdge);
        clsl.def("getMaxInterpolationDistance", &ConvolutionControl::getMaxInterpolationDistance);
        clsl.def("setDoNormalize", &ConvolutionControl::setDoNormalize);
        clsl.def("setDoCopyEdge", &ConvolutionControl::setDoCopyEdge);
        clsl.def("setMaxInterpolationDistance", &ConvolutionControl::setMaxInterpolationDistance);
    });
}
}  // namespace

void wrapConvolveImage(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.image");
    declareConvolveImage(wrappers);
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
}  // namespace math
}  // namespace afw
}  // namespace lsst