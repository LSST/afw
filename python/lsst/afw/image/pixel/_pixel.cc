/*
 * LSST Data Management System
 * Copyright 2008-2017  AURA/LSST.
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

#include "nanobind/nanobind.h"
#include "lsst/cpputils/python.h"

#include <cstdint>
#include <string>

#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/image/Pixel.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace image {
namespace pixel {

namespace {

/**
@internal Declare a SinglePixel for a MaskedImage

(Note that SinglePixel for Image is just the pixel type)

@tparam  Image plane type
@param mod  nanobind module
@param[in] name  Name of Python class, e.g. "SinglePixelI" if PixelT is `int`.
*/
template <typename PixelT>
void declareSinglePixel(lsst::cpputils::python::WrapperCollection &wrappers, std::string const &name) {
    wrappers.wrapType(
            nb::class_<SinglePixel<PixelT, MaskPixel, VariancePixel>>(wrappers.module, name.c_str()),
            [](auto &mod, auto &cls) {
                mod.def("makeSinglePixel", &makeSinglePixel<PixelT, MaskPixel, VariancePixel>, "x"_a, "m"_a,
                        "v"_a);
                cls.def(nb::init<PixelT, MaskPixel, VariancePixel>(), "image"_a, "mask"_a = 0,
                        "variance"_a = 0);
            });
}

}  // namespace

NB_MODULE(_pixel, mod) {
    lsst::cpputils::python::WrapperCollection wrappers(mod, "lsst.afw.image.pixel");
    declareSinglePixel<float>(wrappers, "SinglePixelF");
    declareSinglePixel<double>(wrappers, "SinglePixelD");
    declareSinglePixel<int>(wrappers, "SinglePixelI");
    declareSinglePixel<std::uint16_t>(wrappers, "SinglePixelU");
    declareSinglePixel<std::uint64_t>(wrappers, "SinglePixelL");
    wrappers.finish();
}
}  // namespace pixel
}  // namespace image
}  // namespace afw
}  // namespace lsst
