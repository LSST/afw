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
#include "nanobind/stl/string.h"
#include "nanobind/stl/shared_ptr.h"
#include "lsst/cpputils/python.h"

#include "lsst/afw/cameraGeom/Detector.h"
#include "lsst/afw/geom/SkyWcs.h"
#include "lsst/afw/image/PhotoCalib.h"
#include "lsst/afw/image/FilterLabel.h"
#include "lsst/afw/image/Exposure.h"
#include "lsst/afw/detection/Psf.h"
#include "lsst/afw/image/ApCorrMap.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace image {
namespace {

template <typename PixelT>
using PyExposure = nb::class_<Exposure<PixelT>>;

/*
Declare a constructor that takes an Exposure of FromPixelT and returns an Exposure cast to ToPixelT

The mask and variance must be of the standard types.

@param[in] cls  The nanobind class to which add the constructor
*/
template <typename FromPixelT, typename ToPixelT>
void declareCastConstructor(PyExposure<ToPixelT> &cls) {
    cls.def(nb::init<Exposure<FromPixelT> const &, bool const>(), "src"_a, "deep"_a);
}

template <typename PixelT>  // only the image type varies; mask and variance are fixed to default tparams
PyExposure<PixelT> declareExposure(lsst::cpputils::python::WrapperCollection &wrappers,
                                   const std::string &suffix) {
    using ExposureT = Exposure<PixelT>;
    using MaskedImageT = typename ExposureT::MaskedImageT;
    wrappers.wrap([](auto &mod) {
        mod.def("makeExposure", &makeExposure<PixelT, MaskPixel, VariancePixel>, "maskedImage"_a,
                "wcs"_a = std::shared_ptr<geom::SkyWcs const>());
    });
    return wrappers.wrapType(
            PyExposure<PixelT>(wrappers.module, ("Exposure" + suffix).c_str()), [](auto &mod, auto &cls) {
                /* Constructors */
                cls.def(nb::init<unsigned int, unsigned int, std::shared_ptr<geom::SkyWcs const>>(),
                        "width"_a, "height"_a, "wcs"_a = std::shared_ptr<geom::SkyWcs const>());
                cls.def(nb::init<lsst::geom::Extent2I const &, std::shared_ptr<geom::SkyWcs const>>(),
                        "dimensions"_a = lsst::geom::Extent2I(),
                        "wcs"_a = std::shared_ptr<geom::SkyWcs const>());
                cls.def(nb::init<lsst::geom::Box2I const &, std::shared_ptr<geom::SkyWcs const>>(), "bbox"_a,
                        "wcs"_a = std::shared_ptr<geom::SkyWcs const>());
                cls.def(nb::init<MaskedImageT &, std::shared_ptr<geom::SkyWcs const>>(), "maskedImage"_a,
                        "wcs"_a = std::shared_ptr<geom::SkyWcs const>());
                cls.def(nb::init<MaskedImageT &, std::shared_ptr<ExposureInfo>>(), "maskedImage"_a,
                        "exposureInfo"_a);
                cls.def(nb::init<std::string const &, lsst::geom::Box2I const &, ImageOrigin, bool, bool>(),
                        "fileName"_a, "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT,
                        "conformMasks"_a = false, "allowUnsafe"_a = false);
                cls.def(nb::init<fits::MemFileManager &, lsst::geom::Box2I const &, ImageOrigin, bool,
                                 bool>(),
                        "manager"_a, "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT,
                        "conformMasks"_a = false, "allowUnsafe"_a = false);
                cls.def(nb::init<ExposureT const &, bool>(), "other"_a, "deep"_a = false);
                cls.def(nb::init<ExposureT const &, lsst::geom::Box2I const &, ImageOrigin, bool>(),
                        "other"_a, "bbox"_a, "origin"_a = PARENT, "deep"_a = false);

                /* Members */
                cls.def("getMaskedImage", (MaskedImageT(ExposureT::*)()) & ExposureT::getMaskedImage);
                cls.def("setMaskedImage", &ExposureT::setMaskedImage, "maskedImage"_a);
                cls.def_prop_rw("maskedImage", (MaskedImageT(ExposureT::*)()) & ExposureT::getMaskedImage,
                                 &ExposureT::setMaskedImage);
                cls.def("getMetadata", &ExposureT::getMetadata);
                cls.def("setMetadata", &ExposureT::setMetadata, "metadata"_a = nb::none());
                cls.def_prop_rw("metadata", &ExposureT::getMetadata, &ExposureT::setMetadata, nb::arg("metadata") = nb::none());
                cls.def("getWidth", &ExposureT::getWidth);
                cls.def("getHeight", &ExposureT::getHeight);
                cls.def_prop_ro("width", &ExposureT::getWidth);
                cls.def_prop_ro("height", &ExposureT::getHeight);
                cls.def("getDimensions", &ExposureT::getDimensions);
                cls.def("getX0", &ExposureT::getX0);
                cls.def("getY0", &ExposureT::getY0);
                cls.def_prop_ro("x0", &ExposureT::getX0);
                cls.def_prop_ro("y0", &ExposureT::getY0);
                cls.def("getXY0", &ExposureT::getXY0);
                cls.def("setXY0", &ExposureT::setXY0, "xy0"_a);
                cls.def("getBBox", &ExposureT::getBBox, "origin"_a = PARENT);
                cls.def("getWcs", (std::shared_ptr<geom::SkyWcs>(ExposureT::*)()) & ExposureT::getWcs);
                cls.def_prop_ro(
                        "wcs", (std::shared_ptr<geom::SkyWcs>(ExposureT::*)()) & ExposureT::getWcs);
                cls.def("setWcs", &ExposureT::setWcs, "wcs"_a);
                cls.def("hasWcs", &ExposureT::hasWcs);
                cls.def("getDetector", &ExposureT::getDetector);
                cls.def_prop_ro("detector", &ExposureT::getDetector);
                cls.def("setDetector", &ExposureT::setDetector, nb::arg("detector").none());
                cls.def("getFilter", &ExposureT::getFilter);
                cls.def_prop_ro("filter", &ExposureT::getFilter);
                cls.def("setFilter", &ExposureT::setFilter, "filterLabel"_a);

                cls.def("getPhotoCalib", &ExposureT::getPhotoCalib);
                cls.def_prop_ro("photoCalib", &ExposureT::getPhotoCalib);
                cls.def("setPhotoCalib", &ExposureT::setPhotoCalib, "photoCalib"_a);
                cls.def("getPsf", (std::shared_ptr<detection::Psf>(ExposureT::*)()) & ExposureT::getPsf);
                cls.def_prop_ro(
                        "psf", (std::shared_ptr<detection::Psf>(ExposureT::*)()) & ExposureT::getPsf);
                cls.def("setPsf", &ExposureT::setPsf, "psf"_a);
                cls.def("hasPsf", &ExposureT::hasPsf);
                cls.def("getInfo", (std::shared_ptr<ExposureInfo>(ExposureT::*)()) & ExposureT::getInfo);
                cls.def_prop_ro(
                        "info", (std::shared_ptr<ExposureInfo>(ExposureT::*)()) & ExposureT::getInfo);
                cls.def("setInfo", &ExposureT::setInfo, "exposureInfo"_a);

                cls.def_prop_ro("visitInfo",
                                          [](ExposureT &self) { return self.getInfo()->getVisitInfo(); });

                cls.def("setApCorrMap", &ExposureT::setApCorrMap, "apCorrMap"_a);
                cls.def_prop_ro("apCorrMap", [](ExposureT &self) { return self.getApCorrMap(); });

                cls.def("subset", &ExposureT::subset, "bbox"_a, "origin"_a = PARENT);

                cls.def("writeFits", (void(ExposureT::*)(std::string const &) const) & ExposureT::writeFits);
                cls.def("writeFits",
                        (void(ExposureT::*)(fits::MemFileManager &) const) & ExposureT::writeFits);
                cls.def("writeFits", [](ExposureT &self, fits::Fits &fits) { self.writeFits(fits); });

                cls.def(
                        "writeFits",
                        [](ExposureT &self, std::string const &filename,
                           fits::ImageWriteOptions const &imageOptions,
                           fits::ImageWriteOptions const &maskOptions,
                           fits::ImageWriteOptions const &varianceOptions) {
                            self.writeFits(filename, imageOptions, maskOptions, varianceOptions);
                        },
                        "filename"_a, "imageOptions"_a, "maskOptions"_a, "varianceOptions"_a);
                cls.def(
                        "writeFits",
                        [](ExposureT &self, fits::MemFileManager &manager,
                           fits::ImageWriteOptions const &imageOptions,
                           fits::ImageWriteOptions const &maskOptions,
                           fits::ImageWriteOptions const &varianceOptions) {
                            self.writeFits(manager, imageOptions, maskOptions, varianceOptions);
                        },
                        "manager"_a, "imageOptions"_a, "maskOptions"_a, "varianceOptions"_a);
                cls.def(
                        "writeFits",
                        [](ExposureT &self, fits::Fits &fits, fits::ImageWriteOptions const &imageOptions,
                           fits::ImageWriteOptions const &maskOptions,
                           fits::ImageWriteOptions const &varianceOptions) {
                            self.writeFits(fits, imageOptions, maskOptions, varianceOptions);
                        },
                        "fits"_a, "imageOptions"_a, "maskOptions"_a, "varianceOptions"_a);

                cls.def_static("readFits", (ExposureT(*)(std::string const &))ExposureT::readFits);
                cls.def_static("readFits", (ExposureT(*)(fits::MemFileManager &))ExposureT::readFits);

                cls.def("getCutout",
                        nb::overload_cast<lsst::geom::SpherePoint const &, lsst::geom::Extent2I const &>(
                                &ExposureT::getCutout, nb::const_),
                        "center"_a, "size"_a);
                cls.def("getCutout",
                        nb::overload_cast<lsst::geom::Point2D const &, lsst::geom::Extent2I const &>(
                                &ExposureT::getCutout, nb::const_),
                        "center"_a, "size"_a);
                cls.def("getCutout",
                        nb::overload_cast<lsst::geom::Box2I const &>(&ExposureT::getCutout, nb::const_),
                        "box"_a);
            });
}
}  // namespace
NB_MODULE(_exposure, mod) {
    lsst::cpputils::python::WrapperCollection wrappers(mod, "lsst.afw.image._exposure");
    wrappers.addSignatureDependency("lsst.afw.image._apCorrMap");
    wrappers.addSignatureDependency("lsst.afw.geom");
    wrappers.addSignatureDependency("lsst.afw.detection");
    wrappers.addSignatureDependency("lsst.afw.image._maskedImage");

    auto clsExposureF = declareExposure<float>(wrappers, "F");
    auto clsExposureD = declareExposure<double>(wrappers, "D");
    declareExposure<int>(wrappers, "I");
    declareExposure<std::uint16_t>(wrappers, "U");
    declareExposure<std::uint64_t>(wrappers, "L");

    // Declare constructors for casting all exposure types to to float and double
    // (the only two types of casts that Python supports)
    declareCastConstructor<int, float>(clsExposureF);
    declareCastConstructor<int, double>(clsExposureD);

    declareCastConstructor<float, double>(clsExposureD);
    declareCastConstructor<double, float>(clsExposureF);

    declareCastConstructor<std::uint16_t, float>(clsExposureF);
    declareCastConstructor<std::uint16_t, double>(clsExposureD);

    declareCastConstructor<std::uint64_t, float>(clsExposureF);
    declareCastConstructor<std::uint64_t, double>(clsExposureD);

    wrappers.finish();
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
