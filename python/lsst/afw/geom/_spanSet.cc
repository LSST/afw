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
#include <lsst/cpputils/python.h>
#include "nanobind/stl/vector.h"
#include "nanobind/stl/shared_ptr.h"
#include <cstdint>
#include <sstream>
#include <string>
#include <iostream>
#include <nanobind/make_iterator.h>

#include "ndarray/nanobind.h"
#include "ndarray/Array.h"

#include "lsst/afw/geom/ellipses/Quadrupole.h"
#include "lsst/afw/geom/SpanSet.h"
#include "lsst/afw/table/io/python.h"  // for addPersistableMethods

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace geom {

namespace {

using PySpanSet = nb::class_<SpanSet>;

template <typename Pixel, typename PyClass>
void declareFlattenMethod(PyClass &cls) {
    cls.def("flatten", [](SpanSet &self, ndarray::Array<Pixel, 2, 0> const &array, lsst::geom::Point2I const &point) {
            std::cout << "+++++++++++++++\n";
            static_assert(!std::is_same_v<Pixel, bool>);
            std::cout << array;
            auto result = self.flatten<Pixel, 2, 0>(array, point);
            std::cout << "+++++++++++++++\n";
            //std::cout << result;
            return result;
        }, "input"_a, "xy0"_a = lsst::geom::Point2I(), nb::rv_policy::reference);
    cls.def("flatten", [](SpanSet &self, ndarray::Array<Pixel, 3, 0> const & array, lsst::geom::Point2I const &point) {
            std::cout << "+++++++++++++++\n";
            std::cout << array;
            auto result =  self.flatten<Pixel, 3, 0>(array, point);
            std::cout << "+++++++++++++++\n";
            //std::cout << result;
            return result;
        },"input"_a, "xy0"_a = lsst::geom::Point2I(), nb::rv_policy::reference);
    cls.def("flatten",
            (void (SpanSet::*)(ndarray::Array<Pixel, 1, 0> const &, ndarray::Array<Pixel, 2, 0> const &,
                               lsst::geom::Point2I const &) const) &
                    SpanSet::flatten<Pixel, Pixel, 2, 0, 0>,
            "output"_a, "input"_a, "xy0"_a = lsst::geom::Point2I());
    cls.def("flatten",
            (void (SpanSet::*)(ndarray::Array<Pixel, 2, 0> const &, ndarray::Array<Pixel, 3, 0> const &,
                               lsst::geom::Point2I const &) const) &
                    SpanSet::flatten<Pixel, Pixel, 3, 0, 0>,
            "output"_a, "input"_a, "xy0"_a = lsst::geom::Point2I());
}

template <typename Pixel, typename PyClass>
void declareUnflattenMethod(PyClass &cls) {
    cls.def("unflatten",
            (ndarray::Array<Pixel, 2, 2>(SpanSet::*)(ndarray::Array<Pixel, 1, 0> const &input) const) &
                    SpanSet::unflatten<Pixel, 1, 0>, nb::rv_policy::move);
    cls.def("unflatten",
            (ndarray::Array<Pixel, 3, 3>(SpanSet::*)(ndarray::Array<Pixel, 2, 0> const &input) const) &
                    SpanSet::unflatten<Pixel, 2, 0>, nb::rv_policy::move);
    cls.def("unflatten",
            (void (SpanSet::*)(ndarray::Array<Pixel, 2, 0> const &, ndarray::Array<Pixel, 1, 0> const &,
                               lsst::geom::Point2I const &) const) &
                    SpanSet::unflatten<Pixel, Pixel, 1, 0, 0>,
            "output"_a, "input"_a, "xy0"_a = lsst::geom::Point2I());
    cls.def("unflatten",
            (void (SpanSet::*)(ndarray::Array<Pixel, 3, 0> const &, ndarray::Array<Pixel, 2, 0> const &,
                               lsst::geom::Point2I const &) const) &
                    SpanSet::unflatten<Pixel, Pixel, 2, 0, 0>,
            "output"_a, "input"_a, "xy0"_a = lsst::geom::Point2I());
}

template <typename Pixel, typename PyClass>
void declareSetMaskMethod(PyClass &cls) {
    cls.def("setMask", (void (SpanSet::*)(image::Mask<Pixel> &, Pixel) const) & SpanSet::setMask);
}

template <typename Pixel, typename PyClass>
void declareClearMaskMethod(PyClass &cls) {
    cls.def("clearMask", (void (SpanSet::*)(image::Mask<Pixel> &, Pixel) const) & SpanSet::clearMask);
}

template <typename Pixel, typename PyClass>
void declareIntersectMethod(PyClass &cls) {
    cls.def("intersect",
            (std::shared_ptr<SpanSet>(SpanSet::*)(image::Mask<Pixel> const &, Pixel) const) &
                    SpanSet::intersect,
            "other"_a, "bitmask"_a);
    // Default to compare any bit set
    cls.def(
            "intersect",
            [](SpanSet const &self, image::Mask<Pixel> const &mask) {
                auto tempSpanSet = SpanSet::fromMask(mask);
                return self.intersect(*tempSpanSet);
            },
            "other"_a);
}

template <typename Pixel, typename PyClass>
void declareIntersectNotMethod(PyClass &cls) {
    cls.def("intersectNot",
            (std::shared_ptr<SpanSet>(SpanSet::*)(image::Mask<Pixel> const &, Pixel) const) &
                    SpanSet::intersectNot,
            "other"_a, "bitmask"_a);
    // Default to compare any bit set
    cls.def(
            "intersectNot",
            [](SpanSet const &self, image::Mask<Pixel> const &mask) {
                auto tempSpanSet = SpanSet::fromMask(mask);
                return self.intersectNot(*tempSpanSet);
            },
            "other"_a);
}

template <typename Pixel, typename PyClass>
void declareUnionMethod(PyClass &cls) {
    cls.def("union",
            (std::shared_ptr<SpanSet>(SpanSet::*)(image::Mask<Pixel> const &, Pixel) const) & SpanSet::union_,
            "other"_a, "bitmask"_a);
    // Default to compare any bit set
    cls.def(
            "union",
            [](SpanSet const &self, image::Mask<Pixel> const &mask) {
                auto tempSpanSet = SpanSet::fromMask(mask);
                return self.union_(*tempSpanSet);
            },
            "other"_a);
}

template <typename ImageT, typename PyClass>
void declareCopyImage(PyClass &cls) {
    cls.def("copyImage", &SpanSet::copyImage<ImageT>);
}

template <typename ImageT, typename PyClass>
void declareCopyMaskedImage(PyClass &cls) {
    using MaskPixel = image::MaskPixel;
    using VariancePixel = image::VariancePixel;
    cls.def("copyMaskedImage", &SpanSet::copyMaskedImage<ImageT, MaskPixel, VariancePixel>);
}

template <typename ImageT, typename PyClass>
void declareSetImage(PyClass &cls) {
    cls.def("setImage",
            (void (SpanSet::*)(image::Image<ImageT> &, ImageT, lsst::geom::Box2I const &, bool) const) &
                    SpanSet::setImage,
            "image"_a, "val"_a, "region"_a = lsst::geom::Box2I(), "doClip"_a = false);
}

template <typename MaskPixel, typename PyClass>
void declarefromMask(PyClass &cls) {
    cls.def_static("fromMask", [](image::Mask<MaskPixel> mask) { return SpanSet::fromMask(mask); });
    cls.def_static("fromMask", [](image::Mask<MaskPixel> mask, MaskPixel const &bitmask) {
        return SpanSet::fromMask(mask, bitmask);
    });
}

template <typename Pixel, typename PyClass>
void declareMaskMethods(PyClass &cls) {
    declareSetMaskMethod<Pixel>(cls);
    declareClearMaskMethod<Pixel>(cls);
    declareIntersectMethod<Pixel>(cls);
    declareIntersectNotMethod<Pixel>(cls);
    declareUnionMethod<Pixel>(cls);
}

template <typename Pixel, typename PyClass>
void declareImageTypes(PyClass &cls) {
    declareFlattenMethod<Pixel>(cls);
    declareUnflattenMethod<Pixel>(cls);
    declareCopyImage<Pixel>(cls);
    declareCopyMaskedImage<Pixel>(cls);
    declareSetImage<Pixel>(cls);
}

void declareStencil(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::enum_<Stencil>(wrappers.module, "Stencil"), [](auto &mod, auto &enm) {
        enm.value("CIRCLE", Stencil::CIRCLE);
        enm.value("BOX", Stencil::BOX);
        enm.value("MANHATTAN", Stencil::MANHATTAN);
    });
}

void declareSpanSet(lsst::cpputils::python::WrapperCollection &wrappers) {
    using MaskPixel = image::MaskPixel;

    wrappers.wrapType(PySpanSet(wrappers.module, "SpanSet"), [](auto &mod, auto &cls) {
        /* SpanSet Constructors */
        cls.def(nb::init<>());
        cls.def(nb::init<lsst::geom::Box2I>(), "box"_a);
        cls.def(nb::init<std::vector<Span>, bool>(), "spans"_a, "normalize"_a = true);

        table::io::python::addPersistableMethods<SpanSet>(cls);

        /* SpanSet Methods */
        cls.def("getArea", &SpanSet::getArea);
        cls.def("getBBox", &SpanSet::getBBox);
        cls.def("isContiguous", &SpanSet::isContiguous);
        cls.def("shiftedBy", (std::shared_ptr<SpanSet>(SpanSet::*)(int, int) const) & SpanSet::shiftedBy);
        cls.def("shiftedBy", (std::shared_ptr<SpanSet>(SpanSet::*)(lsst::geom::Extent2I const &) const) &
                                     SpanSet::shiftedBy);
        cls.def("clippedTo", &SpanSet::clippedTo);
        cls.def("transformedBy",
                (std::shared_ptr<SpanSet>(SpanSet::*)(lsst::geom::LinearTransform const &) const) &
                        SpanSet::transformedBy);
        cls.def("transformedBy",
                (std::shared_ptr<SpanSet>(SpanSet::*)(lsst::geom::AffineTransform const &) const) &
                        SpanSet::transformedBy);
        cls.def("transformedBy",
                (std::shared_ptr<SpanSet>(SpanSet::*)(TransformPoint2ToPoint2 const &) const) &
                        SpanSet::transformedBy);
        cls.def("overlaps", &SpanSet::overlaps);
        cls.def("contains", (bool (SpanSet::*)(SpanSet const &) const) & SpanSet::contains);
        cls.def("contains", (bool (SpanSet::*)(lsst::geom::Point2I const &) const) & SpanSet::contains);
        cls.def("computeCentroid", &SpanSet::computeCentroid);
        cls.def("computeShape", &SpanSet::computeShape);
        cls.def("dilated", (std::shared_ptr<SpanSet>(SpanSet::*)(int, Stencil) const) & SpanSet::dilated,
                "radius"_a, "stencil"_a = Stencil::CIRCLE);
        cls.def("dilated", (std::shared_ptr<SpanSet>(SpanSet::*)(SpanSet const &) const) & SpanSet::dilated);
        cls.def("eroded", (std::shared_ptr<SpanSet>(SpanSet::*)(int, Stencil) const) & SpanSet::eroded,
                "radius"_a, "stencil"_a = Stencil::CIRCLE);
        cls.def("eroded", (std::shared_ptr<SpanSet>(SpanSet::*)(SpanSet const &) const) & SpanSet::eroded);
        cls.def("intersect",
                (std::shared_ptr<SpanSet>(SpanSet::*)(SpanSet const &) const) & SpanSet::intersect);
        cls.def("intersectNot",
                (std::shared_ptr<SpanSet>(SpanSet::*)(SpanSet const &) const) & SpanSet::intersectNot);
        cls.def("union", (std::shared_ptr<SpanSet>(SpanSet::*)(SpanSet const &) const) & SpanSet::union_);
        cls.def_static("fromShape",
                       (std::shared_ptr<SpanSet>(*)(int, Stencil, lsst::geom::Point2I)) & SpanSet::fromShape,
                       "radius"_a, "stencil"_a = Stencil::CIRCLE, "offset"_a = lsst::geom::Point2I());
        cls.def_static(
                "fromShape",
                [](int r, Stencil s, std::pair<int, int> point) {
                    return SpanSet::fromShape(r, s, lsst::geom::Point2I(point.first, point.second));
                },
                "radius"_a, "stencil"_a = Stencil::CIRCLE, "offset"_a = std::pair<int, int>(0, 0));
        cls.def_static("fromShape",
                       (std::shared_ptr<SpanSet>(*)(geom::ellipses::Ellipse const &)) & SpanSet::fromShape);
        cls.def("split", &SpanSet::split);
        cls.def("findEdgePixels", &SpanSet::findEdgePixels);
        cls.def("indices", [](SpanSet const &self) -> ndarray::Array<int, 2, 2> {
            unsigned long dims = 2;
            ndarray::Array<int, 2, 2> inds = ndarray::allocate(ndarray::makeVector(dims, self.getArea()));
            int element = 0;
            for (auto const &span : self) {
                auto y = span.getY();
                for (int x = span.getX0(); x <= span.getX1(); ++x) {
                    inds[0][element] = y;
                    inds[1][element] = x;
                    element++;
                }
            }
            return inds;
        });

        /* SpanSet Operators */
        cls.def(
                "__eq__", [](SpanSet const &self, SpanSet const &other) -> bool { return self == other; },
                nb::is_operator());
        cls.def(
                "__ne__", [](SpanSet const &self, SpanSet const &other) -> bool { return self != other; },
                nb::is_operator());
        cls.def(
                "__iter__", [](SpanSet const &self) { return nb::make_iterator(nb::type<SpanSet>(), "iterator", self.begin(), self.end()); },
                nb::keep_alive<0, 1>());
        cls.def("__len__", [](SpanSet const &self) -> decltype(self.size()) { return self.size(); });
        cls.def("__contains__",
                [](SpanSet &self, SpanSet const &other) -> bool { return self.contains(other); });
        cls.def("__contains__",
                [](SpanSet &self, lsst::geom::Point2I &other) -> bool { return self.contains(other); });
        cls.def("__repr__", [](SpanSet const &self) -> std::string {
            std::ostringstream os;
            image::Mask<MaskPixel> tempMask(self.getBBox());
            self.setMask(tempMask, static_cast<MaskPixel>(1));
            auto array = tempMask.getArray();
            auto dims = array.getShape();
            for (std::size_t i = 0; i < dims[0]; ++i) {
                os << "[";
                for (std::size_t j = 0; j < dims[1]; ++j) {
                    os << array[i][j];
                    if (j != dims[1] - 1) {
                        os << ", ";
                    }
                }
                os << "]" << std::endl;
            }
            return os.str();
        });
        cls.def("__str__", [](SpanSet const &self) -> std::string {
            std::ostringstream os;
            for (auto const &span : self) {
                os << span.getY() << ": " << span.getMinX() << ".." << span.getMaxX() << std::endl;
            }
            return os.str();
        });
        // Instantiate all the templates

        declareMaskMethods<MaskPixel>(cls);

        declareImageTypes<std::uint16_t>(cls);
        declareImageTypes<std::uint64_t>(cls);
        declareImageTypes<int>(cls);
        declareImageTypes<float>(cls);
        declareImageTypes<double>(cls);

        // Extra instantiation for flatten unflatten methods
        declareFlattenMethod<long>(cls);
        declareUnflattenMethod<long>(cls);

        declarefromMask<MaskPixel>(cls);
    });
}
}  // end anonymous namespace
void wrapSpanSet(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.table.io");
    wrappers.addSignatureDependency("lsst.afw.geom.ellipses");
    declareStencil(wrappers);
    declareSpanSet(wrappers);
}
}  // namespace geom
}  // namespace afw
}  // namespace lsst
