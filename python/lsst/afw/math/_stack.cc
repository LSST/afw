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

#include <memory>
#include <vector>

#include <nanobind/nanobind.h>
#include <lsst/cpputils/python.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/shared_ptr.h>

#include "lsst/afw/math/Stack.h"

namespace nb = nanobind;

using namespace nb::literals;

using namespace lsst::afw::math;
namespace lsst {
namespace afw {
namespace math {
namespace {

template <typename PixelT>
void declareStatisticsStack(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("statisticsStack",
                (std::shared_ptr<lsst::afw::image::MaskedImage<PixelT>>(*)(
                        lsst::afw::image::Image<PixelT> const &, Property, char,
                        StatisticsControl const &))statisticsStack<PixelT>,
                "image"_a, "flags"_a, "dimensions"_a, "sctrl"_a = StatisticsControl());
        mod.def("statisticsStack",
                (std::shared_ptr<lsst::afw::image::MaskedImage<PixelT>>(*)(
                        lsst::afw::image::MaskedImage<PixelT> const &, Property, char,
                        StatisticsControl const &))statisticsStack<PixelT>,
                "image"_a, "flags"_a, "dimensions"_a, "sctrl"_a = StatisticsControl());
        mod.def("statisticsStack",
                (void (*)(lsst::afw::image::Image<PixelT> &,
                          std::vector<std::shared_ptr<lsst::afw::image::Image<PixelT>>> &, Property,
                          StatisticsControl const &,
                          std::vector<lsst::afw::image::VariancePixel> const &))statisticsStack<PixelT>,
                "out"_a, "images"_a, "flags"_a, "sctrl"_a = StatisticsControl(),
                "wvector"_a = std::vector<lsst::afw::image::VariancePixel>(0));
        mod.def("statisticsStack",
                (void (*)(lsst::afw::image::MaskedImage<PixelT> &,
                          std::vector<std::shared_ptr<lsst::afw::image::MaskedImage<PixelT>>> &, Property,
                          StatisticsControl const &, std::vector<lsst::afw::image::VariancePixel> const &,
                          lsst::afw::image::MaskPixel, lsst::afw::image::MaskPixel))statisticsStack<PixelT>,
                "out"_a, "images"_a, "flags"_a, "sctrl"_a = StatisticsControl(),
                "wvector"_a = std::vector<lsst::afw::image::VariancePixel>(0), "clipped"_a = 0,
                "excuse"_a = 0);
        mod.def("statisticsStack",
                (void (*)(
                        lsst::afw::image::MaskedImage<PixelT> &,
                        std::vector<std::shared_ptr<lsst::afw::image::MaskedImage<PixelT>>> &, Property,
                        StatisticsControl const &, std::vector<lsst::afw::image::VariancePixel> const &,
                        lsst::afw::image::MaskPixel,
                        std::vector<std::pair<lsst::afw::image::MaskPixel, lsst::afw::image::MaskPixel>> const
                                &))statisticsStack<PixelT>,
                "out"_a, "images"_a, "flags"_a, "sctrl"_a, "wvector"_a, "clipped"_a, "maskMap"_a);
        mod.def("statisticsStack",
                (std::shared_ptr<lsst::afw::image::Image<PixelT>>(*)(
                        std::vector<std::shared_ptr<lsst::afw::image::Image<PixelT>>> &, Property,
                        StatisticsControl const &,
                        std::vector<lsst::afw::image::VariancePixel> const &))statisticsStack<PixelT>,
                "images"_a, "flags"_a, "sctrl"_a = StatisticsControl(),
                "wvector"_a = std::vector<lsst::afw::image::VariancePixel>(0));
        mod.def("statisticsStack",
                (std::shared_ptr<lsst::afw::image::MaskedImage<PixelT>>(*)(
                        std::vector<std::shared_ptr<lsst::afw::image::MaskedImage<PixelT>>> &, Property,
                        StatisticsControl const &, std::vector<lsst::afw::image::VariancePixel> const &,
                        lsst::afw::image::MaskPixel, lsst::afw::image::MaskPixel))statisticsStack<PixelT>,
                "images"_a, "flags"_a, "sctrl"_a = StatisticsControl(),
                "wvector"_a = std::vector<lsst::afw::image::VariancePixel>(0), "clipped"_a = 0,
                "excuse"_a = 0);
        mod.def("statisticsStack",
                (std::shared_ptr<lsst::afw::image::MaskedImage<PixelT>>(*)(
                        std::vector<std::shared_ptr<lsst::afw::image::MaskedImage<PixelT>>> &, Property,
                        StatisticsControl const &, std::vector<lsst::afw::image::VariancePixel> const &,
                        lsst::afw::image::MaskPixel,
                        std::vector<std::pair<lsst::afw::image::MaskPixel, lsst::afw::image::MaskPixel>> const
                                &))statisticsStack<PixelT>,
                "images"_a, "flags"_a, "sctrl"_a, "wvector"_a, "clipped"_a, "maskMap"_a);
        mod.def("statisticsStack",
                (std::vector<PixelT>(*)(
                        std::vector<std::vector<PixelT>> &, Property, StatisticsControl const &,
                        std::vector<lsst::afw::image::VariancePixel> const &))statisticsStack<PixelT>,
                "vectors"_a, "flags"_a, "sctrl"_a = StatisticsControl(),
                "wvector"_a = std::vector<lsst::afw::image::VariancePixel>(0));
    });
}

}  // namespace

void wrapStack(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.image");
    declareStatisticsStack<float>(wrappers);
    declareStatisticsStack<double>(wrappers);
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
