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

#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"

#include <cstdint>

#include "ndarray/nanobind.h"

#include "lsst/cpputils/python.h"

#include "lsst/afw/image/Image.h"
#include "lsst/pex/exceptions.h"
#include "lsst/afw/table/io/python.h"  // for addPersistableMethods
#include "lsst/afw/detection/Footprint.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace detection {

using cpputils::python::WrapperCollection;

namespace {

template <typename MaskT>
void declareMaskFromFootprintList(WrapperCollection &wrappers) {
    auto maskSetter = [](lsst::afw::image::Mask<MaskT> *mask,
                         std::vector<std::shared_ptr<lsst::afw::detection::Footprint>> const &footprints,
                         MaskT const bitmask, bool doClip) {
        for (auto const &foot : footprints) {
            try {
                if (doClip) {
                    auto tmpSpan = foot->getSpans()->clippedTo(mask->getBBox());
                    tmpSpan->setMask(*mask, bitmask);
                } else {
                    foot->getSpans()->setMask(*mask, bitmask);
                }
            } catch (lsst::pex::exceptions::OutOfRangeError const &e) {
                throw LSST_EXCEPT(lsst::pex::exceptions::OutOfRangeError,
                                  "Bounds of a Footprint fall outside mask set doClip to force");
            }
        }
    };

    wrappers.wrap([&maskSetter](auto &mod) {
        mod.def("setMaskFromFootprintList", std::move(maskSetter), "mask"_a, "footprints"_a, "bitmask"_a,
                "doClip"_a = true);
    });
}

}  // end anonymous namespace

void wrapFootprint(WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.afw.table.io");
    wrappers.addSignatureDependency("lsst.afw.geom");
    wrappers.addSignatureDependency("lsst.afw.table");

    wrappers.wrapType(
            nb::class_<Footprint>(wrappers.module, "Footprint"),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<std::shared_ptr<geom::SpanSet>, lsst::geom::Box2I const &>(), "inputSpans"_a,
                        "region"_a = lsst::geom::Box2I());

                cls.def(nb::init<std::shared_ptr<geom::SpanSet>, afw::table::Schema const &,
                                 lsst::geom::Box2I const &>(),
                        "inputSpans"_a, "peakSchema"_a, "region"_a = lsst::geom::Box2I());
                cls.def(nb::init<Footprint const &>());
                cls.def(nb::init<>());

                table::io::python::addPersistableMethods<Footprint>(cls);

                cls.def("getSpans", &Footprint::getSpans);
                cls.def("setSpans", &Footprint::setSpans);
                cls.def("getPeaks", (PeakCatalog & (Footprint::*)()) & Footprint::getPeaks,
                        nb::rv_policy::reference_internal);
                cls.def("addPeak", &Footprint::addPeak);
                cls.def("sortPeaks", &Footprint::sortPeaks, "key"_a = afw::table::Key<float>());
                cls.def("setPeakSchema", &Footprint::setPeakSchema);
                cls.def("setPeakCatalog", &Footprint::setPeakCatalog, "otherPeaks"_a);
                cls.def("getArea", &Footprint::getArea);
                cls.def("getCentroid", &Footprint::getCentroid);
                cls.def("getShape", &Footprint::getShape);
                cls.def("shift", (void (Footprint::*)(int, int)) & Footprint::shift);
                cls.def("shift", (void (Footprint::*)(lsst::geom::ExtentI const &)) & Footprint::shift);
                cls.def("getBBox", &Footprint::getBBox);
                cls.def("getRegion", &Footprint::getRegion);
                cls.def("setRegion", &Footprint::setRegion);
                cls.def("clipTo", &Footprint::clipTo);
                cls.def("contains", &Footprint::contains);
                cls.def("transform",
                        (std::shared_ptr<Footprint>(Footprint::*)(std::shared_ptr<geom::SkyWcs>,
                                                                  std::shared_ptr<geom::SkyWcs>,
                                                                  lsst::geom::Box2I const &, bool) const) &
                                Footprint::transform,
                        "source"_a, "target"_a, "region"_a, "doClip"_a = true);
                cls.def("transform",
                        (std::shared_ptr<Footprint>(Footprint::*)(lsst::geom::LinearTransform const &,
                                                                  lsst::geom::Box2I const &, bool) const) &
                                Footprint::transform);
                cls.def("transform",
                        (std::shared_ptr<Footprint>(Footprint::*)(lsst::geom::AffineTransform const &,
                                                                  lsst::geom::Box2I const &, bool) const) &
                                Footprint::transform);
                cls.def("transform",
                        (std::shared_ptr<Footprint>(Footprint::*)(geom::TransformPoint2ToPoint2 const &,
                                                                  lsst::geom::Box2I const &, bool) const) &
                                Footprint::transform);
                cls.def("dilate", (void (Footprint::*)(int, geom::Stencil)) & Footprint::dilate, "r"_a,
                        "stencil"_a = geom::Stencil::CIRCLE);
                cls.def("dilate", (void (Footprint::*)(geom::SpanSet const &)) & Footprint::dilate);
                cls.def("erode", (void (Footprint::*)(int, geom::Stencil)) & Footprint::erode, "r"_a,
                        "stencil"_a = geom::Stencil::CIRCLE);
                cls.def("erode", (void (Footprint::*)(geom::SpanSet const &)) & Footprint::erode);
                cls.def("removeOrphanPeaks", &Footprint::removeOrphanPeaks);
                cls.def("updatePeakSignificance",
                        nb::overload_cast<double>(&Footprint::updatePeakSignificance));
                cls.def("updatePeakSignificance", nb::overload_cast<image::Image<float> const &, int>(
                                                          &Footprint::updatePeakSignificance));
                cls.def("isContiguous", &Footprint::isContiguous);
                cls.def("isHeavy", &Footprint::isHeavy);
                cls.def("assign", (Footprint & (Footprint::*)(Footprint const &)) & Footprint::operator=);

                cls.def("split", [](Footprint const &self) -> nb::list {
                    /* This is a work around for pybind not properly
                     * handling converting a vector of unique pointers
                     * to python lists of shared pointers */
                    nb::list l;
                    for (auto &ptr : self.split()) {
                        l.append(nb::cast(std::shared_ptr<Footprint>(std::move(ptr))));
                    }
                    return l;
                });

                cls.def_prop_rw("spans", &Footprint::getSpans, &Footprint::setSpans);
                cls.def_prop_ro("peaks", (PeakCatalog & (Footprint::*)()) & Footprint::getPeaks,
                                          nb::rv_policy::reference_internal);

                cls.def("__contains__", [](Footprint const &self, lsst::geom::Point2I const &point) -> bool {
                    return self.contains(point);
                });
                cls.def(
                        "__eq__",
                        [](Footprint const &self, Footprint const &other) -> bool { return self == other; },
                        nb::is_operator());
                cpputils::python::addOutputOp(cls, "__repr__");
            });

    declareMaskFromFootprintList<lsst::afw::image::MaskPixel>(wrappers);

    wrappers.wrap([](auto &mod) {
        mod.def("mergeFootprints", &mergeFootprints);
        mod.def("footprintToBBoxList", &footprintToBBoxList);
    });
}

}  // namespace detection
}  // namespace afw
}  // namespace lsst
