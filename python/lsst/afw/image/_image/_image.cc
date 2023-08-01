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

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "lsst/utils/python.h"
#include "ndarray/pybind11.h"

#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/ImageSlice.h"
#include "lsst/afw/image/Mask.h"
#include "lsst/afw/fits.h"
#include "lsst/afw/image/python/indexing.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace image {

namespace {

template <typename PixelT>
using PyImageBase = py::class_<ImageBase<PixelT>>;

template <typename PixelT>
using PyImage = py::class_<Image<PixelT>, ImageBase<PixelT>>;

template <typename PixelT>
using PyDecoratedImage = py::class_<DecoratedImage<PixelT>>;

template <typename MaskPixelT>
using PyMask = py::class_<Mask<MaskPixelT>, ImageBase<MaskPixelT>>;

/**
@internal Declare a constructor that takes a MaskedImage of FromPixelT and returns a MaskedImage cast to
ToPixelT

The mask and variance must be of the standard types.

@param[in] cls  The pybind11 class to which add the constructor
*/
template <typename FromPixelT, typename ToPixelT>
static void declareCastConstructor(PyImage<ToPixelT> &cls) {
    cls.def(py::init<Image<FromPixelT> const &, bool const>(), "src"_a, "deep"_a);
}

template <typename PixelT>
static void declareImageBase(lsst::utils::python::WrapperCollection &wrappers, std::string const &suffix) {
    using Array = typename ImageBase<PixelT>::Array;
    wrappers.wrapType(PyImageBase<PixelT>(wrappers.module, ("ImageBase" + suffix).c_str()), [](auto &mod,
                                                                                               auto &cls) {
        cls.def(py::init<lsst::geom::Extent2I const &>(), "dimensions"_a = lsst::geom::Extent2I());
        cls.def(py::init<ImageBase<PixelT> const &, bool>(), "src"_a, "deep"_a = false);
        cls.def(py::init<ImageBase<PixelT> const &, lsst::geom::Box2I const &, ImageOrigin, bool>(), "src"_a,
                "bbox"_a, "origin"_a = PARENT, "deep"_a = false);
        cls.def(py::init<Array const &, bool, lsst::geom::Point2I const &>(), "array"_a, "deep"_a = false,
                "xy0"_a = lsst::geom::Point2I());

        cls.def("assign", &ImageBase<PixelT>::assign, "rhs"_a, "bbox"_a = lsst::geom::Box2I(),
                "origin"_a = PARENT,
                py::is_operator());  // py::is_operator is a workaround for code in slicing.py
        // that expects NotImplemented to be returned on failure.
        cls.def("getWidth", &ImageBase<PixelT>::getWidth);
        cls.def("getHeight", &ImageBase<PixelT>::getHeight);
        cls.def("getX0", &ImageBase<PixelT>::getX0);
        cls.def("getY0", &ImageBase<PixelT>::getY0);
        cls.def("getXY0", &ImageBase<PixelT>::getXY0);
        cls.def("positionToIndex", &ImageBase<PixelT>::positionToIndex, "position"_a, "xOrY"_a);
        cls.def("indexToPosition", &ImageBase<PixelT>::indexToPosition, "index"_a, "xOrY"_a);
        cls.def("getDimensions", &ImageBase<PixelT>::getDimensions);
        cls.def("getArray", (Array(ImageBase<PixelT>::*)()) & ImageBase<PixelT>::getArray);
        cls.def_property("array", (Array(ImageBase<PixelT>::*)()) & ImageBase<PixelT>::getArray,
                         [](ImageBase<PixelT> &self, ndarray::Array<PixelT const, 2, 0> const &array) {
                             // Avoid self-assignment, which is invoked when a Python in-place operator is
                             // used.
                             if (array.shallow() != self.getArray().shallow()) {
                                 self.getArray().deep() = array;
                             }
                         });
        cls.def("setXY0",
                (void (ImageBase<PixelT>::*)(lsst::geom::Point2I const)) & ImageBase<PixelT>::setXY0,
                "xy0"_a);
        cls.def("setXY0", (void (ImageBase<PixelT>::*)(int const, int const)) & ImageBase<PixelT>::setXY0,
                "x0"_a, "y0"_a);
        cls.def("getBBox", &ImageBase<PixelT>::getBBox, "origin"_a = PARENT);

        cls.def("set", [](ImageBase<PixelT> &img, PixelT val) { img = val; });
        cls.def(
                "_set",
                [](ImageBase<PixelT> &img, geom::Point2I const &index, PixelT val, ImageOrigin origin) {
                    python::checkBounds(index, img.getBBox(origin));
                    img.get(index, origin) = val;
                },
                "index"_a, "value"_a, "origin"_a);
        cls.def(
                "_get",
                [](ImageBase<PixelT> &img, geom::Point2I const &index, ImageOrigin origin) {
                    python::checkBounds(index, img.getBBox(origin));
                    return img.get(index, origin);
                },
                "index"_a, "origin"_a);
    });
}

template <typename MaskPixelT>
static void declareMask(lsst::utils::python::WrapperCollection &wrappers, std::string const &suffix) {
    wrappers.wrapType(PyMask<MaskPixelT>(wrappers.module, ("Mask" + suffix).c_str()), [](auto &mod,
                                                                                         auto &cls) {
        /* Constructors */
        cls.def(py::init<unsigned int, unsigned int, typename Mask<MaskPixelT>::MaskPlaneDict const &>(),
                "width"_a, "height"_a, "planeDefs"_a = typename Mask<MaskPixelT>::MaskPlaneDict());
        cls.def(py::init<unsigned int, unsigned int, MaskPixelT,
                         typename Mask<MaskPixelT>::MaskPlaneDict const &>(),
                "width"_a, "height"_a, "initialValue"_a,
                "planeDefs"_a = typename Mask<MaskPixelT>::MaskPlaneDict());
        cls.def(py::init<lsst::geom::Extent2I const &, typename Mask<MaskPixelT>::MaskPlaneDict const &>(),
                "dimensions"_a = lsst::geom::Extent2I(),
                "planeDefs"_a = typename Mask<MaskPixelT>::MaskPlaneDict());
        cls.def(py::init<lsst::geom::Extent2I const &, MaskPixelT,
                         typename Mask<MaskPixelT>::MaskPlaneDict const &>(),
                "dimensions"_a = lsst::geom::Extent2I(), "initialValue"_a,
                "planeDefs"_a = typename Mask<MaskPixelT>::MaskPlaneDict());
        cls.def(py::init<lsst::geom::Box2I const &, typename Mask<MaskPixelT>::MaskPlaneDict const &>(),
                "bbox"_a, "planeDefs"_a = typename Mask<MaskPixelT>::MaskPlaneDict());
        cls.def(py::init<lsst::geom::Box2I const &, MaskPixelT,
                         typename Mask<MaskPixelT>::MaskPlaneDict const &>(),
                "bbox"_a, "initialValue"_a, "planeDefs"_a = typename Mask<MaskPixelT>::MaskPlaneDict());
        cls.def(py::init<const Mask<MaskPixelT> &, const bool>(), "src"_a, "deep"_a = false);
        cls.def(py::init<const Mask<MaskPixelT> &, const lsst::geom::Box2I &, ImageOrigin const,
                         const bool>(),
                "src"_a, "bbox"_a, "origin"_a = PARENT, "deep"_a = false);
        cls.def(py::init<ndarray::Array<MaskPixelT, 2, 1> const &, bool, lsst::geom::Point2I const &>(),
                "array"_a, "deep"_a = false, "xy0"_a = lsst::geom::Point2I());
        cls.def(py::init<std::string const &, int, std::shared_ptr<lsst::daf::base::PropertySet>,
                         lsst::geom::Box2I const &, ImageOrigin, bool, bool>(),
                "fileName"_a, "hdu"_a = fits::DEFAULT_HDU, "metadata"_a = nullptr,
                "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "conformMasks"_a = false,
                "allowUnsafe"_a = false);
        cls.def(py::init<fits::MemFileManager &, int, std::shared_ptr<lsst::daf::base::PropertySet>,
                         lsst::geom::Box2I const &, ImageOrigin, bool, bool>(),
                "manager"_a, "hdu"_a = fits::DEFAULT_HDU, "metadata"_a = nullptr,
                "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "conformMasks"_a = false,
                "allowUnsafe"_a = false);
        cls.def(py::init<fits::Fits &, std::shared_ptr<lsst::daf::base::PropertySet>,
                         lsst::geom::Box2I const &, ImageOrigin, bool, bool>(),
                "fitsFile"_a, "metadata"_a = nullptr, "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT,
                "conformMasks"_a = false, "allowUnsafe"_a = false);

        /* Operators */
        cls.def("__ior__", [](Mask<MaskPixelT> &self, Mask<MaskPixelT> &other) { return self |= other; });
        cls.def("__ior__", [](Mask<MaskPixelT> &self, MaskPixelT const other) { return self |= other; });
        cls.def("__ior__", [](Mask<MaskPixelT> &self, int other) { return self |= other; });
        cls.def("__iand__", [](Mask<MaskPixelT> &self, Mask<MaskPixelT> &other) { return self &= other; });
        cls.def("__iand__", [](Mask<MaskPixelT> &self, MaskPixelT const other) { return self &= other; });
        cls.def("__iand__", [](Mask<MaskPixelT> &self, int other) { return self &= other; });
        cls.def("__ixor__", [](Mask<MaskPixelT> &self, Mask<MaskPixelT> &other) { return self ^= other; });
        cls.def("__ixor__", [](Mask<MaskPixelT> &self, MaskPixelT const other) { return self ^= other; });
        cls.def("__ixor__", [](Mask<MaskPixelT> &self, int other) { return self ^= other; });

        /* Members */
        cls.def("swap", (void (Mask<MaskPixelT>::*)(Mask<MaskPixelT> &)) & Mask<MaskPixelT>::swap);
        cls.def("writeFits",
                (void (Mask<MaskPixelT>::*)(std::string const &,
                                            daf::base::PropertySet const *,
                                            std::string const &) const) &
                        Mask<MaskPixelT>::writeFits,
                "fileName"_a, "metadata"_a = nullptr, "mode"_a = "w");
        cls.def("writeFits",
                (void (Mask<MaskPixelT>::*)(fits::MemFileManager &,
                                            daf::base::PropertySet const *,
                                            std::string const &) const) &
                        Mask<MaskPixelT>::writeFits,
                "manager"_a, "metadata"_a = nullptr, "mode"_a = "w");
        cls.def("writeFits",
                (void (Mask<MaskPixelT>::*)(fits::Fits &, daf::base::PropertySet const *)
                         const) &
                        Mask<MaskPixelT>::writeFits,
                "fitsfile"_a, "metadata"_a = nullptr);
        cls.def("writeFits",
                (void (Mask<MaskPixelT>::*)(std::string const &, fits::ImageWriteOptions const &,
                                            std::string const &,
                                            daf::base::PropertySet const *) const) &
                        Mask<MaskPixelT>::writeFits,
                "filename"_a, "options"_a, "mode"_a = "w",
                "header"_a = nullptr);
        cls.def("writeFits",
                (void (Mask<MaskPixelT>::*)(fits::MemFileManager &, fits::ImageWriteOptions const &,
                                            std::string const &,
                                            daf::base::PropertySet const *) const) &
                        Mask<MaskPixelT>::writeFits,
                "manager"_a, "options"_a, "mode"_a = "w",
                "header"_a = nullptr);
        cls.def("writeFits",
                (void (Mask<MaskPixelT>::*)(fits::Fits &, fits::ImageWriteOptions const &,
                                            daf::base::PropertySet const *) const) &
                        Mask<MaskPixelT>::writeFits,
                "fits"_a, "options"_a, "header"_a = nullptr);
        cls.def_static("readFits", (Mask<MaskPixelT>(*)(std::string const &, int))Mask<MaskPixelT>::readFits,
                       "filename"_a, "hdu"_a = fits::DEFAULT_HDU);
        cls.def_static("readFits",
                       (Mask<MaskPixelT>(*)(fits::MemFileManager &, int))Mask<MaskPixelT>::readFits,
                       "manager"_a, "hdu"_a = fits::DEFAULT_HDU);
        cls.def_static("interpret", Mask<MaskPixelT>::interpret);
        cls.def("subset", &Mask<MaskPixelT>::subset, "bbox"_a, "origin"_a = PARENT);
        cls.def("getAsString", &Mask<MaskPixelT>::getAsString);
        cls.def("clearAllMaskPlanes", &Mask<MaskPixelT>::clearAllMaskPlanes);
        cls.def("clearMaskPlane", &Mask<MaskPixelT>::clearMaskPlane);
        cls.def("setMaskPlaneValues", &Mask<MaskPixelT>::setMaskPlaneValues);
        cls.def_static("parseMaskPlaneMetadata", Mask<MaskPixelT>::parseMaskPlaneMetadata);
        cls.def_static("clearMaskPlaneDict", Mask<MaskPixelT>::clearMaskPlaneDict);
        cls.def_static("removeMaskPlane", Mask<MaskPixelT>::removeMaskPlane);
        cls.def("removeAndClearMaskPlane", &Mask<MaskPixelT>::removeAndClearMaskPlane, "name"_a,
                "removeFromDefault"_a = false);
        cls.def_static("getMaskPlane", Mask<MaskPixelT>::getMaskPlane);
        cls.def_static("getPlaneBitMask",
                       (MaskPixelT(*)(const std::string &))Mask<MaskPixelT>::getPlaneBitMask);
        cls.def_static("getPlaneBitMask",
                       (MaskPixelT(*)(const std::vector<std::string> &))Mask<MaskPixelT>::getPlaneBitMask);
        cls.def_static("getNumPlanesMax", Mask<MaskPixelT>::getNumPlanesMax);
        cls.def_static("getNumPlanesUsed", Mask<MaskPixelT>::getNumPlanesUsed);
        cls.def("getMaskPlaneDict", &Mask<MaskPixelT>::getMaskPlaneDict);
        cls.def("printMaskPlanes", &Mask<MaskPixelT>::printMaskPlanes);
        cls.def_static("addMaskPlanesToMetadata", Mask<MaskPixelT>::addMaskPlanesToMetadata);
        cls.def("conformMaskPlanes", &Mask<MaskPixelT>::conformMaskPlanes);
        cls.def_static("addMaskPlane", (int (*)(const std::string &))Mask<MaskPixelT>::addMaskPlane);
    });
}

template <typename PixelT>
static PyImage<PixelT> declareImage(lsst::utils::python::WrapperCollection &wrappers,
                                    const std::string &suffix) {
    return wrappers.wrapType(PyImage<PixelT>(wrappers.module, ("Image" + suffix).c_str()), [](auto &mod,
                                                                                              auto &cls) {
        /* Constructors */
        cls.def(py::init<unsigned int, unsigned int, PixelT>(), "width"_a, "height"_a, "intialValue"_a = 0);
        cls.def(py::init<lsst::geom::Extent2I const &, PixelT>(), "dimensions"_a = lsst::geom::Extent2I(),
                "initialValue"_a = 0);
        cls.def(py::init<lsst::geom::Box2I const &, PixelT>(), "bbox"_a, "initialValue"_a = 0);
        cls.def(py::init<Image<PixelT> const &, lsst::geom::Box2I const &, ImageOrigin const, const bool>(),
                "rhs"_a, "bbox"_a, "origin"_a = PARENT, "deep"_a = false);
        cls.def(py::init<ndarray::Array<PixelT, 2, 1> const &, bool, lsst::geom::Point2I const &>(),
                "array"_a, "deep"_a = false, "xy0"_a = lsst::geom::Point2I());
        cls.def(py::init<std::string const &, int, std::shared_ptr<daf::base::PropertySet>,
                         lsst::geom::Box2I const &, ImageOrigin, bool>(),
                "fileName"_a, "hdu"_a = fits::DEFAULT_HDU, "metadata"_a = nullptr,
                "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "allowUnsafe"_a = false);
        cls.def(py::init<fits::MemFileManager &, int, std::shared_ptr<daf::base::PropertySet>,
                         lsst::geom::Box2I const &, ImageOrigin, bool>(),
                "manager"_a, "hdu"_a = fits::DEFAULT_HDU, "metadata"_a = nullptr,
                "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "allowUnsafe"_a = false);
        cls.def(py::init<fits::Fits &, std::shared_ptr<daf::base::PropertySet>, lsst::geom::Box2I const &,
                         ImageOrigin, bool>(),
                "fitsFile"_a, "metadata"_a = nullptr, "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT,
                "allowUnsafe"_a = false);

        /* Operators */
        cls.def("__iadd__", [](Image<PixelT> &self, PixelT const &other) { return self += other; });
        cls.def("__iadd__", [](Image<PixelT> &self, Image<PixelT> const &other) { return self += other; });
        cls.def("__iadd__", [](Image<PixelT> &self, lsst::afw::math::Function2<double> const &other) {
            return self += other;
        });
        cls.def("__isub__", [](Image<PixelT> &self, PixelT const &other) { return self -= other; });
        cls.def("__isub__", [](Image<PixelT> &self, Image<PixelT> const &other) { return self -= other; });
        cls.def("__isub__", [](Image<PixelT> &self, lsst::afw::math::Function2<double> const &other) {
            return self -= other;
        });
        cls.def("__imul__", [](Image<PixelT> &self, PixelT const &other) { return self *= other; });
        cls.def("__imul__", [](Image<PixelT> &self, Image<PixelT> const &other) { return self *= other; });
        cls.def("__itruediv__", [](Image<PixelT> &self, PixelT const &other) { return self /= other; });
        cls.def("__itruediv__",
                [](Image<PixelT> &self, Image<PixelT> const &other) { return self /= other; });

        /* Members */
        cls.def("scaledPlus", &Image<PixelT>::scaledPlus);
        cls.def("scaledMinus", &Image<PixelT>::scaledMinus);
        cls.def("scaledMultiplies", &Image<PixelT>::scaledMultiplies);
        cls.def("scaledDivides", &Image<PixelT>::scaledDivides);

        cls.def("subset", &Image<PixelT>::subset, "bbox"_a, "origin"_a = PARENT);

        cls.def("writeFits",
                (void (Image<PixelT>::*)(std::string const &, daf::base::PropertySet const *,
                                         std::string const &) const) &
                        Image<PixelT>::writeFits,
                "fileName"_a, "metadata"_a = nullptr, "mode"_a = "w");
        cls.def("writeFits",
                (void (Image<PixelT>::*)(fits::MemFileManager &,
                                         daf::base::PropertySet const *, std::string const &)
                         const) &
                        Image<PixelT>::writeFits,
                "manager"_a, "metadata"_a = nullptr, "mode"_a = "w");
        cls.def("writeFits",
                (void (Image<PixelT>::*)(fits::Fits &, daf::base::PropertySet const *) const) &
                        Image<PixelT>::writeFits,
                "fitsfile"_a, "metadata"_a = nullptr);
        cls.def("writeFits",
                (void (Image<PixelT>::*)(std::string const &, fits::ImageWriteOptions const &,
                                         std::string const &, daf::base::PropertySet const *,
                                         image::Mask<image::MaskPixel> const *) const) &
                        Image<PixelT>::writeFits,
                "filename"_a, "options"_a, "mode"_a = "w",
                "header"_a = nullptr,
                "mask"_a = nullptr);
        cls.def("writeFits",
                (void (Image<PixelT>::*)(fits::MemFileManager &, fits::ImageWriteOptions const &,
                                         std::string const &, daf::base::PropertySet const *,
                                         image::Mask<image::MaskPixel> const *) const) &
                        Image<PixelT>::writeFits,
                "manager"_a, "options"_a, "mode"_a = "w",
                "header"_a = nullptr,
                "mask"_a = nullptr);
        cls.def("writeFits",
                (void (Image<PixelT>::*)(fits::Fits &, fits::ImageWriteOptions const &,
                                         daf::base::PropertySet const *,
                                         image::Mask<image::MaskPixel> const *) const) &
                        Image<PixelT>::writeFits,
                "fits"_a, "options"_a, "header"_a = nullptr,
                "mask"_a = nullptr);

        cls.def_static("readFits", (Image<PixelT>(*)(std::string const &, int))Image<PixelT>::readFits,
                       "filename"_a, "hdu"_a = fits::DEFAULT_HDU);
        cls.def_static("readFits", (Image<PixelT>(*)(fits::MemFileManager &, int))Image<PixelT>::readFits,
                       "manager"_a, "hdu"_a = fits::DEFAULT_HDU);
        cls.def("sqrt", &Image<PixelT>::sqrt);
    });
}

template <typename PixelT>
static void declareDecoratedImage(lsst::utils::python::WrapperCollection &wrappers,
                                  std::string const &suffix) {
    wrappers.wrapType(
            PyDecoratedImage<PixelT>(wrappers.module, ("DecoratedImage" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def(py::init<const lsst::geom::Extent2I &>(), "dimensions"_a = lsst::geom::Extent2I());
                cls.def(py::init<const lsst::geom::Box2I &>(), "bbox"_a);
                cls.def(py::init<std::shared_ptr<Image<PixelT>>>(), "rhs"_a);
                cls.def(py::init<DecoratedImage<PixelT> const &, const bool>(), "rhs"_a, "deep"_a = false);
                cls.def(py::init<std::string const &, const int, lsst::geom::Box2I const &, ImageOrigin const,
                                 bool>(),
                        "fileName"_a, "hdu"_a = fits::DEFAULT_HDU, "bbox"_a = lsst::geom::Box2I(),
                        "origin"_a = PARENT, "allowUnsafe"_a = false);

                cls.def("getMetadata", &DecoratedImage<PixelT>::getMetadata);
                cls.def("setMetadata", &DecoratedImage<PixelT>::setMetadata);
                cls.def("getWidth", &DecoratedImage<PixelT>::getWidth);
                cls.def("getHeight", &DecoratedImage<PixelT>::getHeight);
                cls.def("getX0", &DecoratedImage<PixelT>::getX0);
                cls.def("getY0", &DecoratedImage<PixelT>::getY0);
                cls.def("getDimensions", &DecoratedImage<PixelT>::getDimensions);
                cls.def("swap", &DecoratedImage<PixelT>::swap);
                cls.def("writeFits",
                        py::overload_cast<std::string const &, daf::base::PropertySet const *,
                                          std::string const &>(&DecoratedImage<PixelT>::writeFits,
                                                               py::const_),
                        "filename"_a, "metadata"_a = nullptr,
                        "mode"_a = "w");
                cls.def("writeFits",
                        py::overload_cast<std::string const &, fits::ImageWriteOptions const &,
                                          daf::base::PropertySet const *, std::string const &>(
                                &DecoratedImage<PixelT>::writeFits, py::const_),
                        "filename"_a, "options"_a, "metadata"_a = nullptr,
                        "mode"_a = "w");
                cls.def("getImage", py::overload_cast<>(&DecoratedImage<PixelT>::getImage));
                cls.def_property_readonly("image", py::overload_cast<>(&DecoratedImage<PixelT>::getImage));
                cls.def("getGain", &DecoratedImage<PixelT>::getGain);
                cls.def("setGain", &DecoratedImage<PixelT>::setGain);
            });
}

/* Declare ImageSlice operators separately since they are only instantiated for float double */
template <typename PixelT>
static void addImageSliceOperators(
        py::class_<Image<PixelT>, ImageBase<PixelT>> &cls) {
    cls.def(
            "__add__",
            [](Image<PixelT> const &self, ImageSlice<PixelT> const &other) { return self + other; },
            py::is_operator());
    cls.def(
            "__sub__",
            [](Image<PixelT> const &self, ImageSlice<PixelT> const &other) { return self - other; },
            py::is_operator());
    cls.def(
            "__mul__",
            [](Image<PixelT> const &self, ImageSlice<PixelT> const &other) { return self * other; },
            py::is_operator());
    cls.def(
            "__truediv__",
            [](Image<PixelT> const &self, ImageSlice<PixelT> const &other) { return self / other; },
            py::is_operator());
    cls.def("__iadd__", [](Image<PixelT> &self, ImageSlice<PixelT> const &other) {
        self += other;
        return self;
    });
    cls.def("__isub__", [](Image<PixelT> &self, ImageSlice<PixelT> const &other) {
        self -= other;
        return self;
    });
    cls.def("__imul__", [](Image<PixelT> &self, ImageSlice<PixelT> const &other) {
        self *= other;
        return self;
    });
    cls.def("__itruediv__", [](Image<PixelT> &self, ImageSlice<PixelT> const &other) {
        self /= other;
        return self;
    });
}

template <typename PixelT, typename PyClass>
static void addGeneralizedCopyConstructors(PyClass &cls) {
    cls.def(py::init<Image<int> const &, const bool>(), "rhs"_a, "deep"_a = false);
    cls.def(py::init<Image<float> const &, const bool>(), "rhs"_a, "deep"_a = false);
    cls.def(py::init<Image<double> const &, const bool>(), "rhs"_a, "deep"_a = false);
    cls.def(py::init<Image<std::uint16_t> const &, const bool>(), "rhs"_a, "deep"_a = false);
    cls.def(py::init<Image<std::uint64_t> const &, const bool>(), "rhs"_a, "deep"_a = false);

    cls.def("convertI", [](Image<PixelT> const &self) { return Image<int>(self, true); });
    cls.def("convertF", [](Image<PixelT> const &self) { return Image<float>(self, true); });
    cls.def("convertD", [](Image<PixelT> const &self) { return Image<double>(self, true); });
    cls.def("convertU", [](Image<PixelT> const &self) { return Image<std::uint16_t>(self, true); });
    cls.def("convertL", [](Image<PixelT> const &self) { return Image<std::uint64_t>(self, true); });

    cls.def("convertFloat", [](Image<PixelT> const &self) { return Image<float>(self, true); });
    cls.def("convertDouble", [](Image<PixelT> const &self) { return Image<double>(self, true); });
}
}  // namespace
void wrapImage(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.daf.base");
    wrappers.addSignatureDependency("lsst.geom");
    wrappers.wrapType(py::enum_<ImageOrigin>(wrappers.module, "ImageOrigin"), [](auto &mod, auto &enm) {
        enm.value("PARENT", ImageOrigin::PARENT);
        enm.value("LOCAL", ImageOrigin::LOCAL);
        enm.export_values();
    });

    declareImageBase<int>(wrappers, "I");
    declareImageBase<float>(wrappers, "F");
    declareImageBase<double>(wrappers, "D");
    declareImageBase<std::uint16_t>(wrappers, "U");
    declareImageBase<std::uint64_t>(wrappers, "L");

    // Mask must be declared before Image because a mask is used as a default value in at least one method
    declareMask<MaskPixel>(wrappers, "X");

    auto clsImageI = declareImage<int>(wrappers, "I");
    auto clsImageF = declareImage<float>(wrappers, "F");
    auto clsImageD = declareImage<double>(wrappers, "D");
    auto clsImageU = declareImage<std::uint16_t>(wrappers, "U");
    auto clsImageL = declareImage<std::uint64_t>(wrappers, "L");

    // Add generalized copy constructors
    addGeneralizedCopyConstructors<int>(clsImageI);
    addGeneralizedCopyConstructors<float>(clsImageF);
    addGeneralizedCopyConstructors<double>(clsImageD);
    addGeneralizedCopyConstructors<std::uint16_t>(clsImageU);
    addGeneralizedCopyConstructors<std::uint64_t>(clsImageL);

    // Add slice operators only for float and double
    addImageSliceOperators<float>(clsImageF);
    addImageSliceOperators<double>(clsImageD);

    declareDecoratedImage<int>(wrappers, "I");
    declareDecoratedImage<float>(wrappers, "F");
    declareDecoratedImage<double>(wrappers, "D");
    declareDecoratedImage<std::uint16_t>(wrappers, "U");
    declareDecoratedImage<std::uint64_t>(wrappers, "L");

    // Declare constructors for casting all exposure types to to float and double
    // (the only two types of casts that Python supports)
    declareCastConstructor<int, float>(clsImageF);
    declareCastConstructor<int, double>(clsImageD);

    declareCastConstructor<float, double>(clsImageD);

    declareCastConstructor<double, float>(clsImageF);

    declareCastConstructor<std::uint16_t, float>(clsImageF);
    declareCastConstructor<std::uint16_t, double>(clsImageD);

    declareCastConstructor<std::uint64_t, float>(clsImageF);
    declareCastConstructor<std::uint64_t, double>(clsImageD);

    // Note: wrap both the Image and MaskedImage versions of imagesOverlap in the MaskedImage wrapper,
    // as wrapping the Image version here results in it being invisible in lsst.afw.image
    wrappers.wrap([](auto &mod) { mod.def("bboxFromMetadata", &bboxFromMetadata); });
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
