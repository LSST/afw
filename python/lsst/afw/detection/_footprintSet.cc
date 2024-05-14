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

#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include "lsst/cpputils/python.h"

#include "lsst/afw/detection/FootprintSet.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace lsst {
namespace afw {
namespace detection {

namespace {

template <typename PixelT, typename PyClass>
void declareMakeHeavy(PyClass &cls) {
    //    cls.def("makeHeavy", [](FootprintSet & self, image::MaskedImage<PixelT, image::MaskPixel> const&
    //    mimg) {
    //            return self.makeHeavy(mimg);
    //            });
    //    cls.def("makeHeavy", [](FootprintSet & self, image::MaskedImage<PixelT, image::MaskPixel> const&
    //    mimg, HeavyFootprintCtrl const* ctrl) {
    //            return self.makeHeavy(mimg, ctrl);
    //            });
    cls.def("makeHeavy",
            (void (FootprintSet::*)(image::MaskedImage<PixelT, image::MaskPixel> const &,
                                    HeavyFootprintCtrl const *)) &
                    FootprintSet::makeHeavy<PixelT, image::MaskPixel>,
            "mimg"_a, "ctrl"_a = nullptr);
}

template <typename PixelT, typename PyClass>
void declareSetMask(PyClass &cls) {
    cls.def("setMask",
            (void (FootprintSet::*)(image::Mask<PixelT> *, std::string const &)) &
                    FootprintSet::setMask<PixelT>,
            "mask"_a, "planeName"_a);
}

template <typename PixelT, typename PyClass>
void declareTemplatedMembers(PyClass &cls) {
    /* Constructors */
    cls.def(nb::init<image::Image<PixelT> const &, Threshold const &, int const, bool const,
                     table::Schema const &>(),
            "img"_a, "threshold"_a, "npixMin"_a = 1, "setPeaks"_a = true,
            "peakSchema"_a = PeakTable::makeMinimalSchema());
    cls.def(nb::init<image::MaskedImage<PixelT, image::MaskPixel> const &, Threshold const &,
                     std::string const &, int const, bool const>(),
            "img"_a, "threshold"_a, "planeName"_a = "", "npixMin"_a = 1, "setPeaks"_a = true);

    /* Members */
    declareMakeHeavy<int>(cls);
    declareMakeHeavy<float>(cls);
    declareMakeHeavy<double>(cls);
    declareMakeHeavy<std::uint16_t>(cls);
    //    declareMakeHeavy<std::uint64_t>(cls);
    declareSetMask<image::MaskPixel>(cls);
}
}  // namespace

void wrapFootprintSet(cpputils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.image");
    wrappers.addSignatureDependency("lsst.afw.table");

    wrappers.wrapType(
            nb::class_<FootprintSet>(wrappers.module, "FootprintSet"),
            [](auto &mod, auto &cls) {
                declareTemplatedMembers<std::uint16_t>(cls);
                declareTemplatedMembers<int>(cls);
                declareTemplatedMembers<float>(cls);
                declareTemplatedMembers<double>(cls);

                cls.def(nb::init<image::Mask<image::MaskPixel> const &, Threshold const &, int const>(),
                        "img"_a, "threshold"_a, "npixMin"_a = 1);

                cls.def(nb::init<lsst::geom::Box2I>(), "region"_a);
                cls.def(nb::init<FootprintSet const &>(), "set"_a);
                cls.def(nb::init<FootprintSet const &, int, FootprintControl const &>(), "set"_a, "rGrow"_a,
                        "ctrl"_a);
                cls.def(nb::init<FootprintSet const &, int, bool>(), "set"_a, "rGrow"_a, "isotropic"_a);
                cls.def(nb::init<FootprintSet const &, FootprintSet const &, bool>(), "footprints1"_a,
                        "footprints2"_a, "includePeaks"_a);

                cls.def("swap", &FootprintSet::swap);
                // setFootprints takes shared_ptr<FootprintList> and getFootprints returns it,
                // but nanobind can't handle that type, so use a custom getter and setter
                cls.def("setFootprints", [](FootprintSet &self, FootprintSet::FootprintList footList) {
                    self.setFootprints(std::make_shared<FootprintSet::FootprintList>(std::move(footList)));
                });
                cls.def("getFootprints", [](FootprintSet &self) { return *(self.getFootprints()); });
                cls.def("makeSources", &FootprintSet::makeSources);
                cls.def("setRegion", &FootprintSet::setRegion);
                cls.def("getRegion", &FootprintSet::getRegion);
                cls.def("insertIntoImage", &FootprintSet::insertIntoImage);
                cls.def("setMask", (void (FootprintSet::*)(image::Mask<lsst::afw::image::MaskPixel> *,
                                                           std::string const &)) &
                                           FootprintSet::setMask<lsst::afw::image::MaskPixel>);
                cls.def("setMask",
                        (void (FootprintSet::*)(std::shared_ptr<image::Mask<lsst::afw::image::MaskPixel>>,
                                                std::string const &)) &
                                FootprintSet::setMask<lsst::afw::image::MaskPixel>);
                cls.def("merge", &FootprintSet::merge, "rhs"_a, "tGrow"_a = 0, "rGrow"_a = 0,
                        "isotropic"_a = true);
                cpputils::python::addOutputOp(cls, "__repr__");
            });
}

}  // namespace detection
}  // namespace afw
}  // namespace lsst
