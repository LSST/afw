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
#include "nanobind/stl/optional.h"
#include "lsst/cpputils/python.h"
#include <numpy/arrayobject.h>
#include "ndarray/nanobind.h"

#include "lsst/cpputils/python/TemplateInvoker.h"
#include "lsst/afw/image/ImageBaseFitsReader.h"
#include "lsst/afw/image/ImageFitsReader.h"
#include "lsst/afw/image/MaskFitsReader.h"
#include "lsst/afw/image/MaskedImageFitsReader.h"
#include "lsst/afw/image/ExposureFitsReader.h"
#include "lsst/afw/geom/SkyWcs.h"
#include "lsst/afw/geom/polygon/Polygon.h"
#include "lsst/afw/detection/Psf.h"
#include "lsst/afw/image/PhotoCalib.h"
#include "lsst/afw/image/CoaddInputs.h"
#include "lsst/afw/image/VisitInfo.h"
#include "lsst/afw/image/ApCorrMap.h"
#include "lsst/afw/image/TransmissionCurve.h"
#include "lsst/afw/cameraGeom/Detector.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace image {
namespace {

static nb::object create_dtype(const std::string &type) {
    // Import the numpy module
    PyObject* numpy_module = PyImport_ImportModule("numpy");
    if (!numpy_module) {
        throw std::runtime_error("Failed to import numpy module");
    }

    // Get the dtype attribute from the numpy module
    PyObject* dtype_attr = PyObject_GetAttrString(numpy_module, "dtype");
    Py_DECREF(numpy_module); // Decrement reference count of numpy_module
    if (!dtype_attr) {
        throw std::runtime_error("Failed to get numpy.dtype attribute");
    }
    PyObject* dtype = PyObject_CallFunction(dtype_attr, "s", type.c_str());
    Py_DECREF(dtype_attr); // Decrement reference count of dtype_attr
    if (!dtype) {
        throw std::runtime_error("Failed to create numpy dtype object");
    }
    return nb::object(nb::handle(dtype), nb::detail::steal_t());
}


// ImageBaseFitsReader is an implementation detail and is not exposed directly
// to Python, as we have better ways to share wrapper code between classes
// at the nanobind level (e.g. declareCommon below).
using PyImageFitsReader = nb::class_<ImageFitsReader>;
using PyMaskFitsReader = nb::class_<MaskFitsReader>;
using PyMaskedImageFitsReader = nb::class_<MaskedImageFitsReader>;
using PyExposureFitsReader = nb::class_<ExposureFitsReader>;

// Declare attributes common to all FitsReaders.  Excludes constructors
// because ExposureFitsReader's don't take an HDU argument.
template <typename Class, typename... Args>
void declareCommonMethods(nb::class_<Class, Args...> &cls) {
    cls.def("readBBox", &Class::readBBox, "origin"_a = PARENT);
    cls.def("readXY0", &Class::readXY0, "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT);
    cls.def("getFileName", &Class::getFileName);
    cls.def_prop_ro("fileName", &Class::getFileName);
}

// Declare attributes common to ImageFitsReader and MaskFitsReader
template <typename Class, typename... Args>
void declareSinglePlaneMethods(nb::class_<Class, Args...> &cls) {
    cls.def(nb::init<std::string const &, int>(), "fileName"_a, "hdu"_a = fits::DEFAULT_HDU);
    cls.def(nb::init<fits::MemFileManager &, int>(), "manager"_a, "hdu"_a = fits::DEFAULT_HDU);
    cls.def("readMetadata", &Class::readMetadata);
    cls.def("readDType", [](Class &self) { return create_dtype(self.readDType()); });
    cls.def("getHdu", &Class::getHdu);
    cls.def_prop_ro("hdu", &Class::getHdu);
    cls.def(
            "readArray",
            [](Class &self, lsst::geom::Box2I const &bbox, ImageOrigin origin, bool allowUnsafe,
               nb::object dtype) {
                if (dtype.is(nb::none())) {
                    dtype = create_dtype(self.readDType());
                }
                return cpputils::python::TemplateInvoker().apply(
                        [&](auto t) {
                            return self.template readArray<decltype(t)>(bbox, origin, allowUnsafe);
                        },
                        dtype,
                        cpputils::python::TemplateInvoker::Tag<std::uint16_t, int, float, double,
                                                            std::uint64_t>());
            },
            "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "allowUnsafe"_a = false,
            "dtype"_a = nb::none());
}

// Declare attributes shared by MaskedImageFitsReader and MaskedImageFitsReader.
template <typename Class, typename... Args>
void declareMultiPlaneMethods(nb::class_<Class, Args...> &cls) {
    cls.def("readImageDType", [](Class &self) { return create_dtype(self.readImageDType()); });
    cls.def("readMaskDType", [](Class &self) { return create_dtype(self.readMaskDType()); });
    cls.def("readVarianceDType", [](Class &self) { return create_dtype(self.readVarianceDType()); });
    cls.def(
            "readImage",
            [](Class &self, lsst::geom::Box2I const &bbox, ImageOrigin origin, bool allowUnsafe,
               nb::object dtype) {
                if (dtype.is(nb::none())) {
                    dtype = create_dtype(self.readImageDType());
                }
                return cpputils::python::TemplateInvoker().apply(
                        [&](auto t) {
                            return self.template readImage<decltype(t)>(bbox, origin, allowUnsafe);
                        },
                        dtype,
                        cpputils::python::TemplateInvoker::Tag<std::uint16_t, int, float, double,
                                                            std::uint64_t>());
            },
            "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "allowUnsafe"_a = false,
            "dtype"_a = nb::none());
    cls.def(
            "readImageArray",
            [](Class &self, lsst::geom::Box2I const &bbox, ImageOrigin origin, bool allowUnsafe,
               nb::object dtype) {
                if (dtype.is(nb::none())) {
                    dtype = create_dtype(self.readImageDType());
                }
                return cpputils::python::TemplateInvoker().apply(
                        [&](auto t) {
                            return self.template readImageArray<decltype(t)>(bbox, origin, allowUnsafe);
                        },
                        dtype,
                        cpputils::python::TemplateInvoker::Tag<std::uint16_t, int, float, double,
                                                            std::uint64_t>());
            },
            "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "allowUnsafe"_a = false,
            "dtype"_a = nb::none());
    cls.def(
            "readMask",
            [](Class &self, lsst::geom::Box2I const &bbox, ImageOrigin origin, bool conformMasks,
               bool allowUnsafe, nb::object dtype) {
                if (dtype.is(nb::none())) {
                    dtype = create_dtype(self.readMaskDType());
                }
                return cpputils::python::TemplateInvoker().apply(
                        [&](auto t) {
                            return self.template readMask<decltype(t)>(bbox, origin, conformMasks,
                                                                       allowUnsafe);
                        },
                        dtype, cpputils::python::TemplateInvoker::Tag<MaskPixel>());
            },
            "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "conformMasks"_a = false,
            "allowUnsafe"_a = false, "dtype"_a = nb::none());
    cls.def(
            "readMaskArray",
            [](Class &self, lsst::geom::Box2I const &bbox, ImageOrigin origin, bool allowUnsafe,
               nb::object dtype) {
                if (dtype.is(nb::none())) {
                    dtype = create_dtype(self.readMaskDType());
                }
                return cpputils::python::TemplateInvoker().apply(
                        [&](auto t) {
                            return self.template readMaskArray<decltype(t)>(bbox, origin, allowUnsafe);
                        },
                        dtype, cpputils::python::TemplateInvoker::Tag<MaskPixel>());
            },
            "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "allowUnsafe"_a = false,
            "dtype"_a = nb::none());
    cls.def(
            "readVariance",
            [](Class &self, lsst::geom::Box2I const &bbox, ImageOrigin origin, bool allowUnsafe,
               nb::object dtype) {
                if (dtype.is(nb::none())) {
                    dtype = create_dtype(self.readVarianceDType());
                }
                return cpputils::python::TemplateInvoker().apply(
                        [&](auto t) {
                            return self.template readVariance<decltype(t)>(bbox, origin, allowUnsafe);
                        },
                        dtype, cpputils::python::TemplateInvoker::Tag<VariancePixel>());
            },
            "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "allowUnsafe"_a = false,
            "dtype"_a = nb::none());
    cls.def(
            "readVarianceArray",
            [](Class &self, lsst::geom::Box2I const &bbox, ImageOrigin origin, bool allowUnsafe,
               nb::object dtype) {
                if (dtype.is(nb::none())) {
                    dtype = create_dtype(self.readVarianceDType());
                }
                return cpputils::python::TemplateInvoker().apply(
                        [&](auto t) {
                            return self.template readVarianceArray<decltype(t)>(bbox, origin, allowUnsafe);
                        },
                        dtype, cpputils::python::TemplateInvoker::Tag<VariancePixel>());
            },
            "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "allowUnsafe"_a = false,
            "dtype"_a = nb::none());
}

void declareImageFitsReader(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyImageFitsReader(wrappers.module, "ImageFitsReader"), [](auto &mod, auto &cls) {
        declareCommonMethods(cls);
        declareSinglePlaneMethods(cls);
        cls.def(
                "read",
                [](ImageFitsReader &self, lsst::geom::Box2I const &bbox, ImageOrigin origin, bool allowUnsafe,
                   nb::object dtype) {
                    if (dtype.is(nb::none())) {
                        dtype = create_dtype(self.readDType());
                    }
                    return cpputils::python::TemplateInvoker().apply(
                            [&](auto t) { return self.read<decltype(t)>(bbox, origin, allowUnsafe); },
                            dtype,
                            cpputils::python::TemplateInvoker::Tag<std::uint16_t, int, float, double,
                                                                std::uint64_t>());
                },
                "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "allowUnsafe"_a = false,
                "dtype"_a = nb::none());
    });
}

void declareMaskFitsReader(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyMaskFitsReader(wrappers.module, "MaskFitsReader"), [](auto &mod, auto &cls) {
        declareCommonMethods(cls);
        declareSinglePlaneMethods(cls);
        cls.def(
                "read",
                [](MaskFitsReader &self, lsst::geom::Box2I const &bbox, ImageOrigin origin, bool conformMasks,
                   bool allowUnsafe, nb::object dtype) {
                    if (dtype.is(nb::none())) {
                        dtype = create_dtype(self.readDType());
                    }
                    return cpputils::python::TemplateInvoker().apply(
                            [&](auto t) {
                                return self.read<decltype(t)>(bbox, origin, conformMasks, allowUnsafe);
                            },
                            dtype, cpputils::python::TemplateInvoker::Tag<MaskPixel>());
                },
                "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "conformMasks"_a = false,
                "allowUnsafe"_a = false, "dtype"_a = nb::none());
    });
    // all other methods provided by base class wrappers
}

void declareMaskedImageFitsReader(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyMaskedImageFitsReader(wrappers.module, "MaskedImageFitsReader"), [](auto &mod,
                                                                                            auto &cls) {
        cls.def(nb::init<std::string const &, int>(), "fileName"_a, "hdu"_a = fits::DEFAULT_HDU);
        cls.def(nb::init<fits::MemFileManager &, int>(), "manager"_a, "hdu"_a = fits::DEFAULT_HDU);
        declareCommonMethods(cls);
        declareMultiPlaneMethods(cls);
        cls.def("readPrimaryMetadata", &MaskedImageFitsReader::readPrimaryMetadata);
        cls.def("readImageMetadata", &MaskedImageFitsReader::readImageMetadata);
        cls.def("readMaskMetadata", &MaskedImageFitsReader::readMaskMetadata);
        cls.def("readVarianceMetadata", &MaskedImageFitsReader::readVarianceMetadata);
        cls.def(
                "read",
                [](MaskedImageFitsReader &self, lsst::geom::Box2I const &bbox, ImageOrigin origin,
                   bool conformMasks, bool needAllHdus, bool allowUnsafe, nb::object dtype) {
                    if (dtype.is(nb::none())) {
                        dtype = create_dtype(self.readImageDType());
                    }
                    return cpputils::python::TemplateInvoker().apply(
                            [&](auto t) {
                                return self.read<decltype(t)>(bbox, origin, conformMasks, allowUnsafe);
                            },
                            dtype,
                            cpputils::python::TemplateInvoker::Tag<std::uint16_t, int, float, double,
                                                                std::uint64_t>());
                },
                "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "conformMasks"_a = false,
                "needAllHdus"_a = false, "allowUnsafe"_a = false, "dtype"_a = nb::none());
    });
}

void declareExposureFitsReader(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyExposureFitsReader(wrappers.module, "ExposureFitsReader"), [](auto &mod, auto &cls) {
        cls.def(nb::init<std::string const &>(), "fileName"_a);
        cls.def(nb::init<fits::MemFileManager &>(), "manager"_a);
        declareCommonMethods(cls);
        declareMultiPlaneMethods(cls);
        cls.def("readSerializationVersion", &ExposureFitsReader::readSerializationVersion);
        cls.def("readExposureId", &ExposureFitsReader::readExposureId);
        cls.def("readMetadata", &ExposureFitsReader::readMetadata);
        cls.def("readWcs", &ExposureFitsReader::readWcs);
        cls.def("readFilter", &ExposureFitsReader::readFilter);
        cls.def("readPhotoCalib", &ExposureFitsReader::readPhotoCalib);
        cls.def("readPsf", &ExposureFitsReader::readPsf);
        cls.def("readValidPolygon", &ExposureFitsReader::readValidPolygon);
        cls.def("readApCorrMap", &ExposureFitsReader::readApCorrMap);
        cls.def("readCoaddInputs", &ExposureFitsReader::readCoaddInputs);
        cls.def("readVisitInfo", &ExposureFitsReader::readVisitInfo);
        cls.def("readTransmissionCurve", &ExposureFitsReader::readTransmissionCurve);
        cls.def("readComponent", &ExposureFitsReader::readComponent);
        cls.def("readDetector", &ExposureFitsReader::readDetector);
        cls.def("readExposureInfo", &ExposureFitsReader::readExposureInfo);
        cls.def(
                "readMaskedImage",
                [](ExposureFitsReader &self, lsst::geom::Box2I const &bbox, ImageOrigin origin,
                   bool conformMasks, bool allowUnsafe, nb::object dtype) {
                    if (dtype.is(nb::none())) {
                        dtype = create_dtype(self.readImageDType());
                    }
                    return cpputils::python::TemplateInvoker().apply(
                            [&](auto t) {
                                return self.readMaskedImage<decltype(t)>(bbox, origin, conformMasks,
                                                                         allowUnsafe);
                            },
                            dtype,
                            cpputils::python::TemplateInvoker::Tag<std::uint16_t, int, float, double,
                                                                std::uint64_t>());
                },
                "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "conformMasks"_a = false,
                "allowUnsafe"_a = false, "dtype"_a = nb::none());
        cls.def(
                "read",
                [](ExposureFitsReader &self, lsst::geom::Box2I const &bbox, ImageOrigin origin,
                   bool conformMasks, bool allowUnsafe, nb::object dtype) {
                    if (dtype.is(nb::none())) {
                        dtype = create_dtype(self.readImageDType());
                    }
                    auto result = cpputils::python::TemplateInvoker().apply(
                            [&](auto t) {
                                return self.read<decltype(t)>(bbox, origin, conformMasks, allowUnsafe);
                            },
                            dtype,
                            cpputils::python::TemplateInvoker::Tag<std::uint16_t, int, float, double,
                                                                std::uint64_t>());
                    return result;

                },
                "bbox"_a = lsst::geom::Box2I(), "origin"_a = PARENT, "conformMasks"_a = false,
                "allowUnsafe"_a = false, "dtype"_a = nb::none());
    });
}
}  // namespace
void wrapReaders(lsst::cpputils::python::WrapperCollection &wrappers) {
    // wrappers.addInheritanceDependency("lsst.daf.base");
    wrappers.addSignatureDependency("lsst.geom");
    wrappers.addSignatureDependency("lsst.afw.image._image");
    wrappers.addSignatureDependency("lsst.afw.image._maskedImage");
    wrappers.addSignatureDependency("lsst.afw.image._exposure");
    declareImageFitsReader(wrappers);
    declareMaskFitsReader(wrappers);
    declareMaskedImageFitsReader(wrappers);
    declareExposureFitsReader(wrappers);
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
