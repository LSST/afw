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
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/set.h>
#include <nanobind/ndarray.h>

#include "lsst/cpputils/python.h"

#include "ndarray/nanobind.h"

#include "lsst/pex/exceptions/Exception.h"
#include "lsst/pex/exceptions/Runtime.h"
#include "lsst/pex/exceptions/python/Exception.h"
#include "lsst/daf/base.h"
#include "lsst/afw/image/Image.h"

#include "lsst/afw/fits.h"

namespace nb = nanobind;

using namespace nanobind::literals;
namespace lsst {
namespace afw {
namespace fits {
namespace {
void declareImageCompression(lsst::cpputils::python::WrapperCollection &wrappers) {
    auto options = wrappers.wrapType(
            nb::class_<ImageCompressionOptions>(wrappers.module, "ImageCompressionOptions"),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<ImageCompressionOptions::CompressionAlgorithm,
                                 ImageCompressionOptions::Tiles, float>(),
                        "algorithm"_a, "tiles"_a, "quantizeLevel"_a = 0.0);
                cls.def(nb::init<ImageCompressionOptions::CompressionAlgorithm, int, float>(), "algorithm"_a,
                        "rows"_a = 1, "quantizeLevel"_a = 0.0);

                cls.def(nb::init<lsst::afw::image::Image<unsigned char> const &>());
                cls.def(nb::init<lsst::afw::image::Image<unsigned short> const &>());
                cls.def(nb::init<lsst::afw::image::Image<short> const &>());
                cls.def(nb::init<lsst::afw::image::Image<int> const &>());
                cls.def(nb::init<lsst::afw::image::Image<unsigned int> const &>());
                cls.def(nb::init<lsst::afw::image::Image<float> const &>());
                cls.def(nb::init<lsst::afw::image::Image<double> const &>());
                cls.def(nb::init<lsst::afw::image::Image<std::uint64_t> const &>());

                cls.def(nb::init<lsst::afw::image::Mask<unsigned char> const &>());
                cls.def(nb::init<lsst::afw::image::Mask<unsigned short> const &>());
                cls.def(nb::init<lsst::afw::image::Mask<short> const &>());
                cls.def(nb::init<lsst::afw::image::Mask<std::int32_t> const &>());

                cls.def_ro("algorithm", &ImageCompressionOptions::algorithm);
                cls.def_ro("tiles", &ImageCompressionOptions::tiles, nb::rv_policy::reference);
                cls.def_ro("quantizeLevel", &ImageCompressionOptions::quantizeLevel);
            });
    wrappers.wrapType(
            nb::enum_<ImageCompressionOptions::CompressionAlgorithm>(options, "CompressionAlgorithm"),
            [](auto &mod, auto &enm) {
                enm.value("NONE", ImageCompressionOptions::CompressionAlgorithm::NONE);
                enm.value("GZIP", ImageCompressionOptions::CompressionAlgorithm::GZIP);
                enm.value("GZIP_SHUFFLE", ImageCompressionOptions::CompressionAlgorithm::GZIP_SHUFFLE);
                enm.value("RICE", ImageCompressionOptions::CompressionAlgorithm::RICE);
                enm.value("PLIO", ImageCompressionOptions::CompressionAlgorithm::PLIO);
                enm.export_values();
            });
}

template <typename T>
void declareImageScalingOptionsTemplates(nb::class_<ImageScalingOptions> &cls) {
    cls.def(
        "determine",
        // It seems like nb::overload cast should work here, and I don't
        // understand why it doesn't.
        [](
            ImageScalingOptions const & self,
            image::ImageBase<T> const& image,
            image::Mask<image::MaskPixel> const * mask
        ) {
            return self.determine(image, mask);
        },
        "image"_a,
        "mask"_a=nullptr
    );
}

void declareImageScalingOptions(lsst::cpputils::python::WrapperCollection &wrappers) {
    auto options = wrappers.wrapType(
            nb::class_<ImageScalingOptions>(wrappers.module, "ImageScalingOptions"),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<>());
                cls.def(nb::init<ImageScalingOptions::ScalingAlgorithm, int, std::vector<std::string> const &,
                                 int, float, float, bool, double, double>(),
                        "algorithm"_a, "bitpix"_a, "maskPlanes"_a = std::vector<std::string>(), "seed"_a = 1,
                        "quantizeLevel"_a = 4.0, "quantizePad"_a = 5.0, "fuzz"_a = true, "bscale"_a = 1.0,
                        "bzero"_a = 0.0);

                cls.def_ro("algorithm", &ImageScalingOptions::algorithm);
                cls.def_ro("bitpix", &ImageScalingOptions::bitpix);
                cls.def_ro("maskPlanes", &ImageScalingOptions::maskPlanes);
                cls.def_ro("seed", &ImageScalingOptions::seed);
                cls.def_ro("quantizeLevel", &ImageScalingOptions::quantizeLevel);
                cls.def_ro("quantizePad", &ImageScalingOptions::quantizePad);
                cls.def_ro("fuzz", &ImageScalingOptions::fuzz);
                cls.def_ro("bscale", &ImageScalingOptions::bscale);
                cls.def_ro("bzero", &ImageScalingOptions::bzero);

                declareImageScalingOptionsTemplates<float>(cls);
                declareImageScalingOptionsTemplates<double>(cls);
            });
    wrappers.wrapType(nb::enum_<ImageScalingOptions::ScalingAlgorithm>(options, "ScalingAlgorithm"),
                      [](auto &mod, auto &enm) {
                          enm.value("NONE", ImageScalingOptions::ScalingAlgorithm::NONE);
                          enm.value("RANGE", ImageScalingOptions::ScalingAlgorithm::RANGE);
                          enm.value("STDEV_POSITIVE", ImageScalingOptions::ScalingAlgorithm::STDEV_POSITIVE);
                          enm.value("STDEV_NEGATIVE", ImageScalingOptions::ScalingAlgorithm::STDEV_NEGATIVE);
                          enm.value("STDEV_BOTH", ImageScalingOptions::ScalingAlgorithm::STDEV_BOTH);
                          enm.value("MANUAL", ImageScalingOptions::ScalingAlgorithm::MANUAL);
                          enm.export_values();
                      });
}

template <typename T>
void declareImageScaleTemplates(nb::class_<ImageScale> &cls, std::string const &suffix) {
    cls.def("toFits", &ImageScale::toFits<T>, "image"_a, "forceNonfiniteRemoval"_a = false, "fuzz"_a = true,
            "tiles"_a = ndarray::Array<long, 1, 1>(), "seed"_a = 1);
    cls.def("fromFits", &ImageScale::fromFits<T>);
}

void declareImageScale(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::class_<ImageScale>(wrappers.module, "ImageScale"), [](auto &mod, auto &cls) {
        cls.def(nb::init<int, double, double>(), "bitpix"_a, "bscale"_a, "bzero"_a);
        cls.def_ro("bitpix", &ImageScale::bitpix);
        cls.def_ro("bscale", &ImageScale::bscale);
        cls.def_ro("bzero", &ImageScale::bzero);
        cls.def_ro("blank", &ImageScale::blank);

        declareImageScaleTemplates<float>(cls, "F");
        declareImageScaleTemplates<double>(cls, "D");
    });
}

void declareImageWriteOptions(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::class_<ImageWriteOptions>(wrappers.module, "ImageWriteOptions"),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<lsst::afw::image::Image<std::uint16_t>>());
                          cls.def(nb::init<lsst::afw::image::Image<std::int32_t>>());
                          cls.def(nb::init<lsst::afw::image::Image<std::uint64_t>>());
                          cls.def(nb::init<lsst::afw::image::Image<float>>());
                          cls.def(nb::init<lsst::afw::image::Image<double>>());

                          cls.def(nb::init<lsst::afw::image::Mask<lsst::afw::image::MaskPixel>>());

                          cls.def(nb::init<ImageCompressionOptions const &, ImageScalingOptions const &>(),
                                  "compression"_a, "scaling"_a = ImageScalingOptions());
                          cls.def(nb::init<ImageScalingOptions const &>());

                          cls.def(nb::init<lsst::daf::base::PropertySet const &>());

                          cls.def_ro("compression", &ImageWriteOptions::compression);
                          cls.def_ro("scaling", &ImageWriteOptions::scaling);

                          cls.def_static("validate", &ImageWriteOptions::validate);
                      });
}

// Wrapping for lsst::afw::fits::Fits
//
// Not every feature is wrapped, only those that we guess might be useful.
// In particular, the header keyword read/write and table read/write are not wrapped.
void declareFits(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::class_<Fits>(wrappers.module, "Fits"), [](auto &mod, auto &cls) {
        cls.def(nb::init<std::string const &, std::string const &, int>(), "filename"_a, "mode"_a,
                "behavior"_a = Fits::AUTO_CLOSE | Fits::AUTO_CHECK);
        cls.def(nb::init<MemFileManager &, std::string const &, int>(), "manager"_a, "mode"_a,
                "behavior"_a = Fits::AUTO_CLOSE | Fits::AUTO_CHECK);

        cls.def("closeFile", &Fits::closeFile);
        cls.def("getFileName", &Fits::getFileName);
        cls.def("getHdu", &Fits::getHdu);
        cls.def("setHdu", nb::overload_cast<int, bool>(&Fits::setHdu), "hdu"_a, "relative"_a = false);
        cls.def(
                "setHdu", [](Fits &self, std::string const &name) { self.setHdu(name); }, "name"_a);
        cls.def("countHdus", &Fits::countHdus);

        cls.def("writeMetadata", &Fits::writeMetadata);
        cls.def(
                "readMetadata", [](Fits &self, bool strip = false) { return readMetadata(self, strip); },
                "strip"_a = false);
        cls.def("createEmpty", &Fits::createEmpty);

        cls.def("readImageI", [](Fits &self) {
            ndarray::Vector<int, 2> const offset;  // initialized to zero by default
            ndarray::Vector<ndarray::Size, 2> shape = self.getImageShape<2>();
            ndarray::Array<int, 2, 2> result = ndarray::allocate(shape[0], shape[1]);
            self.readImage(result, offset);
            return result;
        });

        cls.def("gotoFirstHdu", [](Fits &self) { self.setHdu(DEFAULT_HDU); });

        cls.def("setImageCompression", &Fits::setImageCompression);
        cls.def("getImageCompression", &Fits::getImageCompression);
        cls.def("checkCompressedImagePhu", &Fits::checkCompressedImagePhu);

        cls.def_ro("status", &Fits::status);
    });
}

void declareFitsModule(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        nb::class_<MemFileManager> clsMemFileManager(mod, "MemFileManager");

        clsMemFileManager.def(nb::init<>());
        clsMemFileManager.def(nb::init<size_t>());

        /* TODO: We should really revisit persistence and pickling as this is quite ugly.
         * But it is what Swig did (sort of, it used the cdata.i extension), so I reckon this
         * is cleaner because it does not expose casting to the Python side. */
        clsMemFileManager.def("getLength", &MemFileManager::getLength);
        clsMemFileManager.def("getData", [](MemFileManager &m) {
            return nb::bytes(static_cast<char *>(m.getData()), m.getLength());
        });
        clsMemFileManager.def("setData", [](MemFileManager &m, nb::bytes const &d, size_t size) {
            memcpy(m.getData(), PyBytes_AsString(d.ptr()), size);
        });
        clsMemFileManager.def(
                "readMetadata",
                [](MemFileManager &self, int hdu = DEFAULT_HDU, bool strip = false) {
                    return readMetadata(self, hdu, strip);
                },
                "hdu"_a = DEFAULT_HDU, "strip"_a = false);
        mod.attr("DEFAULT_HDU") = DEFAULT_HDU;
        mod.def(
            "combineMetadata",
            nb::overload_cast<daf::base::PropertyList const&, daf::base::PropertyList const &>(
                combineMetadata
            ),
            "first"_a, "second"_a
        );
        mod.def("makeLimitedFitsHeader", &makeLimitedFitsHeader, "metadata"_a,
                "excludeNames"_a = std::set<std::string>());
        mod.def(
                "readMetadata",
                [](std::string const &filename, int hdu = DEFAULT_HDU, bool strip = false) {
                    return readMetadata(filename, hdu, strip);
                },
                "fileName"_a, "hdu"_a = DEFAULT_HDU, "strip"_a = false);

        mod.def(
                "readMetadata",
                [](std::string const &filename, std::string const &hduname, bool strip = false) {
                    return readMetadata(filename, hduname, HduType::ANY, 0, strip);
                },
                "fileName"_a, "hduName"_a, "strip"_a = false);

        mod.def("setAllowImageCompression", &setAllowImageCompression, "allow"_a);
        mod.def("getAllowImageCompression", &getAllowImageCompression);

        mod.def("compressionAlgorithmFromString", &compressionAlgorithmFromString);
        mod.def("compressionAlgorithmToString", &compressionAlgorithmToString);
        mod.def("scalingAlgorithmFromString", &scalingAlgorithmFromString);
        mod.def("scalingAlgorithmToString", &scalingAlgorithmToString);
    });
}
}  // namespace
NB_MODULE(_fits, mod) {
    lsst::cpputils::python::WrapperCollection wrappers(mod, "lsst.afw.fits");
    wrappers.addInheritanceDependency("lsst.pex.exceptions");
    wrappers.addSignatureDependency("lsst.daf.base");
    // FIXME: after afw.image pybind wrappers are converted
    //wrappers.addSignatureDependency("lsst.afw.image");
    auto cls = wrappers.wrapException<FitsError, lsst::pex::exceptions::IoError>("FitsError", "IoError");
    cls.def(nb::init<std::string const &>());
    declareImageCompression(wrappers);
    declareImageScalingOptions(wrappers);
    declareImageScale(wrappers);
    declareImageWriteOptions(wrappers);
    declareFits(wrappers);
    declareFitsModule(wrappers);
    wrappers.finish();
}
}  // namespace fits
}  // namespace afw
}  // namespace lsst
