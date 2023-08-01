/*
 * This file is part of afw.
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "ndarray/pybind11.h"

#include "lsst/utils/python.h"

#include "lsst/afw/detection/Footprint.h"
#include "lsst/afw/detection/FootprintCtrl.h"
#include "lsst/afw/detection/HeavyFootprint.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace detection {

using utils::python::WrapperCollection;

namespace {
template <typename ImagePixelT, typename MaskPixelT = lsst::afw::image::MaskPixel,
          typename VariancePixelT = lsst::afw::image::VariancePixel>
void declareHeavyFootprint(WrapperCollection &wrappers, std::string const &suffix) {
    using Class = HeavyFootprint<ImagePixelT>;
    wrappers.wrapType(
            py::class_<Class, Footprint>(wrappers.module,
                                                                 ("HeavyFootprint" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def(py::init<Footprint const &,
                                 lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> const
                                         &,
                                 HeavyFootprintCtrl const *>(),
                        "foot"_a, "mimage"_a, "ctrl"_a = nullptr);
                cls.def(py::init<Footprint const &, HeavyFootprintCtrl const *>(), "foot"_a,
                        "ctrl"_a = nullptr);

                cls.def("isHeavy", &Class::isHeavy);
                cls.def("insert", (void (Class::*)(lsst::afw::image::MaskedImage<ImagePixelT> &) const) &
                                          Class::insert);
                cls.def("insert",
                        (void (Class::*)(lsst::afw::image::Image<ImagePixelT> &) const) & Class::insert);
                cls.def("getImageArray",
                        (ndarray::Array<ImagePixelT, 1, 1>(Class::*)()) & Class::getImageArray);
                cls.def("getMaskArray", (ndarray::Array<MaskPixelT, 1, 1>(Class::*)()) & Class::getMaskArray);
                cls.def("getVarianceArray",
                        (ndarray::Array<VariancePixelT, 1, 1>(Class::*)()) & Class::getVarianceArray);
                cls.def("getMaskBitsSet", &Class::getMaskBitsSet);
                cls.def("dot", &Class::dot);
            });

    wrappers.wrap([](auto &mod) {
        mod.def("makeHeavyFootprint",
                (Class(*)(Footprint const &,
                          lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> const &,
                          HeavyFootprintCtrl const *))
                        makeHeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT>,
                "foot"_a, "img"_a, "ctrl"_a = nullptr);

        mod.def("mergeHeavyFootprints", mergeHeavyFootprints<ImagePixelT, MaskPixelT, VariancePixelT>);
    });
}
}  // namespace

void wrapHeavyFootprint(WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.image");

    declareHeavyFootprint<int>(wrappers, "I");
    declareHeavyFootprint<std::uint16_t>(wrappers, "U");
    declareHeavyFootprint<float>(wrappers, "F");
    declareHeavyFootprint<double>(wrappers, "D");
}

}  // namespace detection
}  // namespace afw
}  // namespace lsst
