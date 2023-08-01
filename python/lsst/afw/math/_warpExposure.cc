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

#include <cstdint>
#include <memory>
#include <string>

#include <pybind11/pybind11.h>
#include <lsst/utils/python.h>

#include "lsst/afw/geom/SkyWcs.h"
#include "lsst/afw/image/Exposure.h"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/math/Kernel.h"
#include "lsst/afw/math/warpExposure.h"
#include "lsst/afw/table/io/python.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace math {

namespace {
/**
@internal Declare a warping kernel class with no constructor

@tparam KernelT  class of warping kernel, e.g. LanczosWarpingKernel
@param[in] mod  pybind11 module to which to add the kernel
@param[in] name  Python name for class, e.g. "LanczosWarpingKernel"
@param[in] addConstructor  If true then add a default constructor.
*/
template <typename KernelT>
void declareWarpingKernel(lsst::utils::python::WrapperCollection &wrappers, std::string const &name) {
    using PyClass = py::class_<KernelT, SeparableKernel>;
    wrappers.wrapType(PyClass(wrappers.module, name.c_str()), [](auto &mod, auto &cls) {
        cls.def(py::init<int>(), "order"_a);
        cls.def("getOrder", &KernelT::getOrder);
        cls.def("clone", &KernelT::clone);
        table::io::python::addPersistableMethods(cls);
    });
}

/**
@internal Declare a warping kernel class with a defaut constructor

@tparam KernelT  class of warping kernel, e.g. LanczosWarpingKernel
@param[in] mod  pybind11 module to which to add the kernel
@param[in] name  Python name for class, e.g. "LanczosWarpingKernel"
@param[in] addConstructor  If true then add a default constructor.
*/
template <typename KernelT>
void declareSimpleWarpingKernel(lsst::utils::python::WrapperCollection &wrappers, std::string const &name) {
    using PyClass = py::class_<KernelT, SeparableKernel>;
    wrappers.wrapType(PyClass(wrappers.module, name.c_str()), [](auto &mod, auto &cls) {
        cls.def(py::init<>());
        cls.def("clone", &KernelT::clone);
        table::io::python::addPersistableMethods(cls);
    });
}

/**
@internal Declare wrappers for warpImage and warpCenteredImage
for a particular pair of image or masked image types

@tparam DestImageT  Desination image type, e.g. Image<int> or MaskedImage<float, MaskType, VarianceType>
@tparam SrcImageT  Source image type, e.g. Image<int> or MaskedImage<float, MaskType, VarianceType>
@param[in,out] mod  pybind11 module for which to declare the function wrappers
*/
template <typename DestImageT, typename SrcImageT>
void declareImageWarpingFunctions(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        typename DestImageT::SinglePixel const EdgePixel =
                edgePixel<DestImageT>(typename image::detail::image_traits<DestImageT>::image_category());
        mod.def("warpImage",
                (int (*)(DestImageT &, geom::SkyWcs const &, SrcImageT const &, geom::SkyWcs const &,
                         WarpingControl const &, typename DestImageT::SinglePixel)) &
                        warpImage<DestImageT, SrcImageT>,
                "destImage"_a, "destWcs"_a, "srcImage"_a, "srcWcs"_a, "control"_a, "padValue"_a = EdgePixel);

        mod.def("warpImage",
                (int (*)(DestImageT &, SrcImageT const &, geom::TransformPoint2ToPoint2 const &,
                         WarpingControl const &, typename DestImageT::SinglePixel)) &
                        warpImage<DestImageT, SrcImageT>,
                "destImage"_a, "srcImage"_a, "srcToDest"_a, "control"_a, "padValue"_a = EdgePixel);

        mod.def("warpCenteredImage", &warpCenteredImage<DestImageT, SrcImageT>, "destImage"_a, "srcImage"_a,
                "linearTransform"_a, "centerPoint"_a, "control"_a, "padValue"_a = EdgePixel);
    });
}

/**
@internal Declare wrappers for warpExposure, warpImage and warpCenteredImage
for a particular pair of source and destination pixel types.

Declares both image and masked image variants of warpImage and warpCenteredImage.

@tparam DestPixelT  Desination pixel type, e.g. `int` or `float`
@tparam SrcPixelT  Source pixel type, e.g. `int` or `float`
@param[in,out] mod  pybind11 module for which to declare the function wrappers
*/
template <typename DestPixelT, typename SrcPixelT>
void declareWarpingFunctions(lsst::utils::python::WrapperCollection &wrappers) {
    using DestExposureT = image::Exposure<DestPixelT, image::MaskPixel, image::VariancePixel>;
    using SrcExposureT = image::Exposure<SrcPixelT, image::MaskPixel, image::VariancePixel>;
    using DestImageT = image::Image<DestPixelT>;
    using SrcImageT = image::Image<SrcPixelT>;
    using DestMaskedImageT = image::MaskedImage<DestPixelT, image::MaskPixel, image::VariancePixel>;
    using SrcMaskedImageT = image::MaskedImage<SrcPixelT, image::MaskPixel, image::VariancePixel>;
    wrappers.wrap([](auto &mod) {
        mod.def("warpExposure", &warpExposure<DestExposureT, SrcExposureT>, "destExposure"_a, "srcExposure"_a,
                "control"_a,
                "padValue"_a = edgePixel<DestMaskedImageT>(
                        typename image::detail::image_traits<DestMaskedImageT>::image_category()));
    });
    declareImageWarpingFunctions<DestImageT, SrcImageT>(wrappers);
    declareImageWarpingFunctions<DestMaskedImageT, SrcMaskedImageT>(wrappers);
}

void declareWarpExposure(lsst::utils::python::WrapperCollection &wrappers) {
    using PyClass = py::class_<WarpingControl>;
    wrappers.wrapType(PyClass(wrappers.module, "WarpingControl"), [](auto &mod, auto cls) {
        cls.def(py::init<std::string, std::string, int, int, image::MaskPixel>(), "warpingKernelName"_a,
                "maskWarpingKernelName"_a = "", "cacheSize"_a = 0, "interpLength"_a = 0,
                "growFullMask"_a = 0);

        cls.def("getCacheSize", &WarpingControl::getCacheSize);
        cls.def("setCacheSize", &WarpingControl::setCacheSize, "cacheSize"_a);
        cls.def("getInterpLength", &WarpingControl::getInterpLength);
        cls.def("setInterpLength", &WarpingControl::setInterpLength, "interpLength"_a);
        cls.def("setWarpingKernelName", &WarpingControl::setWarpingKernelName, "warpingKernelName"_a);
        cls.def("getWarpingKernel", &WarpingControl::getWarpingKernel);
        cls.def("setWarpingKernel", &WarpingControl::setWarpingKernel, "warpingKernel"_a);
        cls.def("setMaskWarpingKernelName", &WarpingControl::setMaskWarpingKernelName,
                "maskWarpingKernelName"_a);
        cls.def("getMaskWarpingKernel", &WarpingControl::getMaskWarpingKernel);
        cls.def("hasMaskWarpingKernel", &WarpingControl::hasMaskWarpingKernel);
        cls.def("setMaskWarpingKernelName", &WarpingControl::setMaskWarpingKernelName,
                "maskWarpingKernelName"_a);
        cls.def("setMaskWarpingKernel", &WarpingControl::setMaskWarpingKernel, "maskWarpingKernel"_a);
        cls.def("getGrowFullMask", &WarpingControl::getGrowFullMask);
        cls.def("setGrowFullMask", &WarpingControl::setGrowFullMask, "growFullMask"_a);
        table::io::python::addPersistableMethods(cls);
    });
}
}  // namespace
void wrapWarpExposure(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.image");
    wrappers.addSignatureDependency("lsst.afw.geom.skyWcs");

    declareWarpExposure(wrappers);
    declareWarpingKernel<LanczosWarpingKernel>(wrappers, "LanczosWarpingKernel");
    declareSimpleWarpingKernel<BilinearWarpingKernel>(wrappers, "BilinearWarpingKernel");
    declareSimpleWarpingKernel<NearestWarpingKernel>(wrappers, "NearestWarpingKernel");
    declareWarpingFunctions<double, double>(wrappers);
    declareWarpingFunctions<double, float>(wrappers);
    declareWarpingFunctions<double, int>(wrappers);
    declareWarpingFunctions<double, std::uint16_t>(wrappers);
    declareWarpingFunctions<float, float>(wrappers);
    declareWarpingFunctions<float, int>(wrappers);
    declareWarpingFunctions<float, std::uint16_t>(wrappers);
    declareWarpingFunctions<int, int>(wrappers);
    declareWarpingFunctions<std::uint16_t, std::uint16_t>(wrappers);
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
