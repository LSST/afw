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
#include "nanobind/stl/string.h"
#include "nanobind/stl/shared_ptr.h"
#include "nanobind/stl/map.h"
#include "lsst/cpputils/python.h"

#include "lsst/afw/fits.h"
#include "lsst/afw/image/MaskedImage.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace image {

namespace {

template <typename ImagePixelT>  // only the image type varies; mask and variance are fixed
using PyMaskedImage = nb::class_<MaskedImage<ImagePixelT>>;

/**
@internal Declare a constructor that takes a MaskedImage of FromPixelT and returns a MaskedImage cast to
ToPixelT

The mask and variance must be of the standard types.

@param[in] cls  The nanobind class to which add the constructor
*/
template <typename FromPixelT, typename ToPixelT>
void declareCastConstructor(PyMaskedImage<ToPixelT> &cls) {
    cls.def(nb::init<MaskedImage<FromPixelT> const &, bool const>(), "src"_a, "deep"_a);
}

template <typename ImagePixelT>
PyMaskedImage<ImagePixelT> declareMaskedImage(lsst::cpputils::python::WrapperCollection &wrappers,
                                              const std::string &suffix) {
    using MI = MaskedImage<ImagePixelT>;

    return wrappers.wrapType(
            PyMaskedImage<ImagePixelT>(wrappers.module, ("MaskedImage" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                mod.def("makeMaskedImage", &makeMaskedImage<ImagePixelT, MaskPixel, VariancePixel>, "image"_a,
                        "mask"_a = nullptr, "variance"_a = nullptr);

                /* Member types and enums */

                /* Constructors */
                cls.def(nb::init<unsigned int, unsigned int, typename MI::MaskPlaneDict const &>(), "width"_a,
                        "height"_a, "planeDict"_a = typename MI::MaskPlaneDict());
                cls.def(nb::init<lsst::geom::Extent2I, typename MI::MaskPlaneDict const &>(), "dimensions"_a,
                        "planeDict"_a = typename MI::MaskPlaneDict());
                cls.def(nb::init<typename MI::ImagePtr, typename MI::MaskPtr, typename MI::VariancePtr>(),
                        "image"_a, "mask"_a = nullptr, "variance"_a = nullptr);
                cls.def(nb::init<lsst::geom::Box2I const &, typename MI::MaskPlaneDict const &>(), "bbox"_a,
                        "planeDict"_a = typename MI::MaskPlaneDict());
                cls.def(nb::init<std::string const &, std::shared_ptr<daf::base::PropertySet>,
                                 lsst::geom::Box2I const &, ImageOrigin, bool, bool,
                                 std::shared_ptr<daf::base::PropertySet>,
                                 std::shared_ptr<daf::base::PropertySet>,
                                 std::shared_ptr<daf::base::PropertySet>, bool>(),
                        "fileName"_a, "metadata"_a = nullptr, "bbox"_a = lsst::geom::Box2I(),
                        "origin"_a = PARENT, "conformMasks"_a = false, "needAllHdus"_a = false,
                        "imageMetadata"_a = nullptr, "maskMetadata"_a = nullptr,
                        "varianceMetadata"_a = nullptr, "allowUnsafe"_a = false);
                cls.def(nb::init<fits::MemFileManager &, std::shared_ptr<daf::base::PropertySet>,
                                 lsst::geom::Box2I const &, ImageOrigin, bool, bool,
                                 std::shared_ptr<daf::base::PropertySet>,
                                 std::shared_ptr<daf::base::PropertySet>,
                                 std::shared_ptr<daf::base::PropertySet>, bool>(),
                        "manager"_a, "metadata"_a = nullptr, "bbox"_a = lsst::geom::Box2I(),
                        "origin"_a = PARENT, "conformMasks"_a = false, "needAllHdus"_a = false,
                        "imageMetadata"_a = nullptr, "maskMetadata"_a = nullptr,
                        "varianceMetadata"_a = nullptr, "allowUnsafe"_a = false);
                cls.def(nb::init<MI const &, bool>(), "rhs"_a, "deep"_a = false);
                cls.def(nb::init<MI const &, lsst::geom::Box2I const &, ImageOrigin, bool>(), "rhs"_a,
                        "bbox"_a, "origin"_a = PARENT, "deep"_a = false);

                /* Operators */
                cls.def("swap", &MI::swap);
                cls.def("assign", &MI::assign, "rhs"_a, "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT,
                        nb::is_operator()  // nb::is_operator is a workaround for code in slicing.py
                                           // that expects NotImplemented to be returned on failure.
                );

                cls.def("subset", &MI::subset, "bbox"_a, "origin"_a = PARENT);

                cls.def("__iadd__", (MI & (MI::*)(ImagePixelT const)) & MI::operator+=);
                cls.def("__iadd__", (MI & (MI::*)(MI const &)) & MI::operator+=);
                cls.def("__iadd__", (MI & (MI::*)(Image<ImagePixelT> const &)) & MI::operator+=);
                cls.def("__iadd__", (MI & (MI::*)(math::Function2<double> const &)) & MI::operator+=);
                cls.def("scaledPlus", &MI::scaledPlus);
                cls.def("__isub__", (MI & (MI::*)(ImagePixelT const)) & MI::operator-=);
                cls.def("__isub__", (MI & (MI::*)(MI const &)) & MI::operator-=);
                cls.def("__isub__", (MI & (MI::*)(Image<ImagePixelT> const &)) & MI::operator-=);
                cls.def("__isub__", (MI & (MI::*)(math::Function2<double> const &)) & MI::operator-=);
                cls.def("scaledMinus", &MI::scaledMinus);
                cls.def("__imul__", (MI & (MI::*)(ImagePixelT const)) & MI::operator*=);
                cls.def("__imul__", (MI & (MI::*)(MI const &)) & MI::operator*=);
                cls.def("__imul__", (MI & (MI::*)(Image<ImagePixelT> const &)) & MI::operator*=);
                cls.def("scaledMultiplies", &MI::scaledMultiplies);
                cls.def("__itruediv__", (MI & (MI::*)(ImagePixelT const)) & MI::operator/=);
                cls.def("__itruediv__", (MI & (MI::*)(MI const &)) & MI::operator/=);
                cls.def("__itruediv__", (MI & (MI::*)(Image<ImagePixelT> const &)) & MI::operator/=);
                cls.def("scaledDivides", &MI::scaledDivides);

                /* Members */
                cls.def("writeFits",
                        (void (MI::*)(std::string const &, std::shared_ptr<daf::base::PropertySet const>,
                                      std::shared_ptr<daf::base::PropertySet const>,
                                      std::shared_ptr<daf::base::PropertySet const>,
                                      std::shared_ptr<daf::base::PropertySet const>) const) &
                                MI::writeFits,
                        "fileName"_a, "metadata"_a = std::shared_ptr<daf::base::PropertySet const>(),
                        "imageMetadata"_a = std::shared_ptr<daf::base::PropertySet const>(),
                        "maskMetadata"_a = std::shared_ptr<daf::base::PropertySet const>(),
                        "varianceMetadata"_a = std::shared_ptr<daf::base::PropertySet const>());
                cls.def("writeFits",
                        (void (MI::*)(fits::MemFileManager &, std::shared_ptr<daf::base::PropertySet const>,
                                      std::shared_ptr<daf::base::PropertySet const>,
                                      std::shared_ptr<daf::base::PropertySet const>,
                                      std::shared_ptr<daf::base::PropertySet const>) const) &
                                MI::writeFits,
                        "manager"_a, "metadata"_a = std::shared_ptr<daf::base::PropertySet const>(),
                        "imageMetadata"_a = std::shared_ptr<daf::base::PropertySet const>(),
                        "maskMetadata"_a = std::shared_ptr<daf::base::PropertySet const>(),
                        "varianceMetadata"_a = std::shared_ptr<daf::base::PropertySet const>());
                cls.def("writeFits",
                        (void (MI::*)(fits::Fits &, std::shared_ptr<daf::base::PropertySet const>,
                                      std::shared_ptr<daf::base::PropertySet const>,
                                      std::shared_ptr<daf::base::PropertySet const>,
                                      std::shared_ptr<daf::base::PropertySet const>) const) &
                                MI::writeFits,
                        "fitsfile"_a, "metadata"_a = std::shared_ptr<daf::base::PropertySet const>(),
                        "imageMetadata"_a = std::shared_ptr<daf::base::PropertySet const>(),
                        "maskMetadata"_a = std::shared_ptr<daf::base::PropertySet const>(),
                        "varianceMetadata"_a = std::shared_ptr<daf::base::PropertySet const>());

                cls.def(
                        "writeFits",
                        [](MI &self, std::string const &filename, fits::ImageWriteOptions const &imageOptions,
                           fits::ImageWriteOptions const &maskOptions,
                           fits::ImageWriteOptions const &varianceOptions,
                           std::shared_ptr<daf::base::PropertySet const> header) {
                            self.writeFits(filename, imageOptions, maskOptions, varianceOptions, header);
                        },
                        "filename"_a, "imageOptions"_a, "maskOptions"_a, "varianceOptions"_a,
                        "header"_a = std::shared_ptr<daf::base::PropertyList>());
                cls.def(
                        "writeFits",
                        [](MI &self, fits::MemFileManager &manager,
                           fits::ImageWriteOptions const &imageOptions,
                           fits::ImageWriteOptions const &maskOptions,
                           fits::ImageWriteOptions const &varianceOptions,
                           std::shared_ptr<daf::base::PropertySet const> header) {
                            self.writeFits(manager, imageOptions, maskOptions, varianceOptions, header);
                        },
                        "manager"_a, "imageOptions"_a, "maskOptions"_a, "varianceOptions"_a,
                        "header"_a = std::shared_ptr<daf::base::PropertyList>());
                cls.def(
                        "writeFits",
                        [](MI &self, fits::Fits &fits, fits::ImageWriteOptions const &imageOptions,
                           fits::ImageWriteOptions const &maskOptions,
                           fits::ImageWriteOptions const &varianceOptions,
                           std::shared_ptr<daf::base::PropertySet const> header) {
                            self.writeFits(fits, imageOptions, maskOptions, varianceOptions, header);
                        },
                        "fits"_a, "imageOptions"_a, "maskOptions"_a, "varianceOptions"_a,
                        "header"_a = std::shared_ptr<daf::base::PropertyList>());

                cls.def_static("readFits", (MI(*)(std::string const &))MI::readFits, "filename"_a);
                cls.def_static("readFits", (MI(*)(fits::MemFileManager &))MI::readFits, "manager"_a);
                cls.def("getImage", &MI::getImage);
                cls.def("setImage", &MI::setImage);
                cls.def_prop_rw("image", &MI::getImage, &MI::setImage);
                cls.def("getMask", &MI::getMask);
                cls.def("setMask", &MI::setMask);
                cls.def_prop_rw("mask", &MI::getMask, &MI::setMask);
                cls.def("getVariance", &MI::getVariance);
                cls.def("setVariance", &MI::setVariance);
                cls.def_prop_rw("variance", &MI::getVariance, &MI::setVariance);
                cls.def("getWidth", &MI::getWidth);
                cls.def("getHeight", &MI::getHeight);
                cls.def("getDimensions", &MI::getDimensions);
                cls.def("getBBox", &MI::getBBox, "origin"_a = PARENT);
                cls.def("getX0", &MI::getX0);
                cls.def("getY0", &MI::getY0);
                cls.def("getXY0", &MI::getXY0);
                cls.def("setXY0", (void (MI::*)(int const, int const)) & MI::setXY0, "x0"_a, "y0"_a);
                cls.def("setXY0", (void (MI::*)(lsst::geom::Point2I const)) & MI::setXY0, "origin"_a);
                cls.def("indexToPosition", &MI::indexToPosition);
                cls.def("positionToIndex", &MI::positionToIndex);
            });
}

template <typename ImagePixelT>  // addtional template types do not seem to be needed
void declareMakeMaskedImage(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("makeMaskedImage", makeMaskedImage<ImagePixelT, MaskPixel, VariancePixel>, "image"_a,
                "mask"_a = nullptr, "variance"_a = nullptr);
    });
}

template <typename ImagePixelT1, typename ImagePixelT2>
void declareImagesOverlap(lsst::cpputils::python::WrapperCollection &wrappers) {
    // wrap both the Image and MaskedImage versions of imagesOverlap here, as wrapping
    // the Image version in the Image wrapper results in it being invisible in lsst.afw.image
    wrappers.wrap([](auto &mod) {
        mod.def("imagesOverlap",
                nb::overload_cast<ImageBase<ImagePixelT1> const &, ImageBase<ImagePixelT2> const &>(
                        &imagesOverlap<ImagePixelT1, ImagePixelT2>),
                "image1"_a, "image2"_a);

        mod.def("imagesOverlap",
                nb::overload_cast<MaskedImage<ImagePixelT1> const &, MaskedImage<ImagePixelT2> const &>(
                        &imagesOverlap<ImagePixelT1, ImagePixelT2>),
                "image1"_a, "image2"_a);
    });
}

}  // namespace

NB_MODULE(_maskedImage, mod) {
    lsst::cpputils::python::WrapperCollection wrappers(mod, "lsst.afw.image._maskedImage");
    wrappers.addSignatureDependency("lsst.afw.image._image");
    wrappers.addInheritanceDependency("lsst.daf.base");

    auto clsMaskedImageF = declareMaskedImage<float>(wrappers, "F");
    auto clsMaskedImageD = declareMaskedImage<double>(wrappers, "D");
    auto clsMaskedImageI = declareMaskedImage<int>(wrappers, "I");
    auto clsMaskedImageU = declareMaskedImage<std::uint16_t>(wrappers, "U");
    auto clsMaskedImageL = declareMaskedImage<std::uint64_t>(wrappers, "L");

    // Declare constructors for casting all exposure types to to float and double
    // (the only two types of casts that Python supports)
    declareCastConstructor<int, float>(clsMaskedImageF);
    declareCastConstructor<int, double>(clsMaskedImageD);
    declareCastConstructor<float, double>(clsMaskedImageD);
    declareCastConstructor<double, float>(clsMaskedImageF);
    declareCastConstructor<std::uint16_t, float>(clsMaskedImageF);
    declareCastConstructor<std::uint16_t, double>(clsMaskedImageD);
    declareCastConstructor<std::uint64_t, float>(clsMaskedImageF);
    declareCastConstructor<std::uint64_t, double>(clsMaskedImageD);

    /* Module level */
    declareMakeMaskedImage<int>(wrappers);
    declareMakeMaskedImage<float>(wrappers);
    declareMakeMaskedImage<double>(wrappers);
    declareMakeMaskedImage<std::uint16_t>(wrappers);
    declareMakeMaskedImage<std::uint64_t>(wrappers);

    declareImagesOverlap<int, int>(wrappers);
    declareImagesOverlap<int, float>(wrappers);
    declareImagesOverlap<int, double>(wrappers);
    declareImagesOverlap<int, std::uint16_t>(wrappers);
    declareImagesOverlap<int, std::uint64_t>(wrappers);

    declareImagesOverlap<float, int>(wrappers);
    declareImagesOverlap<float, float>(wrappers);
    declareImagesOverlap<float, double>(wrappers);
    declareImagesOverlap<float, std::uint16_t>(wrappers);
    declareImagesOverlap<float, std::uint64_t>(wrappers);

    declareImagesOverlap<double, int>(wrappers);
    declareImagesOverlap<double, float>(wrappers);
    declareImagesOverlap<double, double>(wrappers);
    declareImagesOverlap<double, std::uint16_t>(wrappers);
    declareImagesOverlap<double, std::uint64_t>(wrappers);

    declareImagesOverlap<std::uint16_t, int>(wrappers);
    declareImagesOverlap<std::uint16_t, float>(wrappers);
    declareImagesOverlap<std::uint16_t, double>(wrappers);
    declareImagesOverlap<std::uint16_t, std::uint16_t>(wrappers);
    declareImagesOverlap<std::uint16_t, std::uint64_t>(wrappers);

    declareImagesOverlap<std::uint64_t, int>(wrappers);
    declareImagesOverlap<std::uint64_t, float>(wrappers);
    declareImagesOverlap<std::uint64_t, double>(wrappers);
    declareImagesOverlap<std::uint64_t, std::uint16_t>(wrappers);
    declareImagesOverlap<std::uint64_t, std::uint64_t>(wrappers);
    wrappers.finish();
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
