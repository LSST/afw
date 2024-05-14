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

#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "Rgb.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace display {

NB_MODULE(_rgb, mod) {
    /* Module level */
    lsst::cpputils::python::WrapperCollection wrappers(mod, "lsst.afw.display.rbg");
    wrappers.wrap([](auto &mod) {
        mod.def("replaceSaturatedPixels", replaceSaturatedPixels<lsst::afw::image::MaskedImage<float>>,
                "rim"_a, "gim"_a, "bim"_a, "borderWidth"_a = 2, "saturatedPixelValue"_a = 65535);
        mod.def("getZScale", getZScale<std::uint16_t>, "image"_a, "nsamples"_a = 1000, "contrast"_a = 0.25);
        mod.def("getZScale", getZScale<float>, "image"_a, "nsamples"_a = 1000, "contrast"_a = 0.25);
    });
    wrappers.finish();
}
}  // namespace display
}  // namespace afw
}  // namespace lsst
