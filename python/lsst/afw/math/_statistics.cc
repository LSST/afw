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
#include <nanobind/stl/shared_ptr.h>
#include <lsst/cpputils/python.h>
#include <nanobind/stl/vector.h>

#include "lsst/afw/math/Statistics.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace math {

template <typename Pixel>
void declareStatistics(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("makeStatistics",
                (Statistics(*)(image::Image<Pixel> const &, image::Mask<image::MaskPixel> const &, int const,
                               StatisticsControl const &))makeStatistics<Pixel>,
                "img"_a, "msk"_a, "flags"_a, "sctrl"_a = StatisticsControl());
        mod.def("makeStatistics",
                (Statistics(*)(image::MaskedImage<Pixel> const &, int const,
                               StatisticsControl const &))makeStatistics<Pixel>,
                "mimg"_a, "flags"_a, "sctrl"_a = StatisticsControl());
        mod.def("makeStatistics",
                (Statistics(*)(image::MaskedImage<Pixel> const &, image::Image<WeightPixel> const &,
                               int const, StatisticsControl const &))makeStatistics<Pixel>,
                "mimg"_a, "weights"_a, "flags"_a, "sctrl"_a = StatisticsControl());
        mod.def("makeStatistics",
                (Statistics(*)(image::Mask<image::MaskPixel> const &, int const, StatisticsControl const &))
                        makeStatistics,  // this is not a template, just a regular overload
                "msk"_a, "flags"_a, "sctrl"_a = StatisticsControl());
        mod.def("makeStatistics",
                (Statistics(*)(image::Image<Pixel> const &, int const,
                               StatisticsControl const &))makeStatistics<Pixel>,
                "img"_a, "flags"_a, "sctrl"_a = StatisticsControl());

        // interface with const Property flags instead of int for nanobind
        // interface should be fixed in the C++ code
        mod.def("makeStatistics", [](image::Image<Pixel> const & img, image::Mask<image::MaskPixel> const &mask,
                const Property flags, StatisticsControl const & ctrl) {
                return makeStatistics<Pixel>(img, mask, int(flags), ctrl);
        }, "img"_a, "msk"_a, "flags"_a, "sctrl"_a = StatisticsControl());

        mod.def("makeStatistics",[](image::MaskedImage<Pixel> const &mask, Property const flags,
                StatisticsControl const & ctrl) {
            return makeStatistics<Pixel>(mask, int(flags), ctrl);
                }, "mimg"_a, "flags"_a, "sctrl"_a = StatisticsControl());
        mod.def("makeStatistics", [](image::MaskedImage<Pixel> const &mask, image::Image<WeightPixel> const &weights,
                                     const Property flags, StatisticsControl const &ctrl) {
            return makeStatistics<Pixel>(mask, weights, int(flags), ctrl);
        }, "mimg"_a, "weights"_a, "flags"_a, "sctrl"_a = StatisticsControl());
        mod.def("makeStatistics", [](image::Mask<image::MaskPixel> const &mask , Property const flags,
                StatisticsControl const &ctrl) {
            return makeStatistics(mask, int(flags), ctrl);
        }, "msk"_a, "flags"_a, "sctrl"_a = StatisticsControl());
        mod.def("makeStatistics", [](image::Image<Pixel> const &img, Property const flags, StatisticsControl const & ctrl) {
            return makeStatistics<Pixel>(img, int(flags), ctrl);
        }, "img"_a, "flags"_a, "sctrl"_a = StatisticsControl());
    });
}

template <typename Pixel>
void declareStatisticsVectorOverloads(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("makeStatistics",
                (Statistics(*)(std::vector<Pixel> const &, int const,
                               StatisticsControl const &))makeStatistics<Pixel>,
                "v"_a, "flags"_a, "sctrl"_a = StatisticsControl());
        mod.def("makeStatistics",
                (Statistics(*)(std::vector<Pixel> const &, std::vector<WeightPixel> const &, int const,
                               StatisticsControl const &))makeStatistics<Pixel>,
                "v"_a, "vweights"_a, "flags"_a, "sctrl"_a = StatisticsControl());
        mod.def("makeStatistics", [](std::vector<Pixel> const &v, Property const flags, StatisticsControl const & ctrl) {
            return makeStatistics<Pixel>(v, int(flags), ctrl);
        }, "v"_a, "flags"_a, "sctrl"_a = StatisticsControl());
        mod.def("makeStatistics", [](std::vector<Pixel> const &v, std::vector<WeightPixel> const &weights,
                Property const flags, StatisticsControl const &ctrl) {
            return makeStatistics<Pixel>(v, weights, int(flags), ctrl);
        }, "v"_a, "vweights"_a, "flags"_a, "sctrl"_a = StatisticsControl());
    });
}

void declareStatistics(lsst::cpputils::python::WrapperCollection &wrappers) {
    /* Module level */
    wrappers.wrapType(nb::enum_<Property>(wrappers.module, "Property", nb::is_flag()),
                      [](auto &mod, auto &enm) {
                          enm.value("NOTHING", Property::NOTHING);
                          enm.value("ERRORS", Property::ERRORS);
                          enm.value("NPOINT", Property::NPOINT);
                          enm.value("MEAN", Property::MEAN);
                          enm.value("STDEV", Property::STDEV);
                          enm.value("VARIANCE", Property::VARIANCE);
                          enm.value("MEDIAN", Property::MEDIAN);
                          enm.value("IQRANGE", Property::IQRANGE);
                          enm.value("MEANCLIP", Property::MEANCLIP);
                          enm.value("STDEVCLIP", Property::STDEVCLIP);
                          enm.value("VARIANCECLIP", Property::VARIANCECLIP);
                          enm.value("MIN", Property::MIN);
                          enm.value("MAX", Property::MAX);
                          enm.value("SUM", Property::SUM);
                          enm.value("MEANSQUARE", Property::MEANSQUARE);
                          enm.value("ORMASK", Property::ORMASK);
                          enm.value("NCLIPPED", Property::NCLIPPED);
                          enm.value("NMASKED", Property::NMASKED);
                          enm.export_values();
                      });

    wrappers.wrap([](auto &mod) {
        mod.def("stringToStatisticsProperty", stringToStatisticsProperty); });

    using PyClass = nb::class_<StatisticsControl>;
    auto control = PyClass(wrappers.module, "StatisticsControl");
    wrappers.wrapType(nb::enum_<StatisticsControl::WeightsBoolean>(control, "WeightsBoolean", nb::is_flag()),
                      [](auto &mod, auto &enm) {
                          enm.value("WEIGHTS_FALSE", StatisticsControl::WeightsBoolean::WEIGHTS_FALSE);
                          enm.value("WEIGHTS_TRUE", StatisticsControl::WeightsBoolean::WEIGHTS_TRUE);
                          enm.value("WEIGHTS_NONE", StatisticsControl::WeightsBoolean::WEIGHTS_NONE);
                          enm.export_values();
                      });
    wrappers.wrapType(control, [](auto &mod, auto &cls) {
        cls.def(nb::init<double, int, lsst::afw::image::MaskPixel, bool,
                         typename StatisticsControl::WeightsBoolean>(),
                "numSigmaClip"_a = 3.0, "numIter"_a = 3, "andMask"_a = 0x0, "isNanSafe"_a = true,
                "useWeights"_a = StatisticsControl::WeightsBoolean::WEIGHTS_NONE);

        cls.def("getMaskPropagationThreshold", &StatisticsControl::getMaskPropagationThreshold);
        cls.def("setMaskPropagationThreshold", &StatisticsControl::setMaskPropagationThreshold);
        cls.def("getNumSigmaClip", &StatisticsControl::getNumSigmaClip);
        cls.def("getNumIter", &StatisticsControl::getNumIter);
        cls.def("getAndMask", &StatisticsControl::getAndMask);
        cls.def("getNoGoodPixelsMask", &StatisticsControl::getNoGoodPixelsMask);
        cls.def("getNanSafe", &StatisticsControl::getNanSafe);
        cls.def("getWeighted", &StatisticsControl::getWeighted);
        cls.def("getWeightedIsSet", &StatisticsControl::getWeightedIsSet);
        cls.def("getCalcErrorFromInputVariance", &StatisticsControl::getCalcErrorFromInputVariance);
        cls.def("getCalcErrorMosaicMode", &StatisticsControl::getCalcErrorMosaicMode);
        cls.def("setNumSigmaClip", &StatisticsControl::setNumSigmaClip);
        cls.def("setNumIter", &StatisticsControl::setNumIter);
        cls.def("setAndMask", &StatisticsControl::setAndMask);
        cls.def("setNoGoodPixelsMask", &StatisticsControl::setNoGoodPixelsMask);
        cls.def("setNanSafe", &StatisticsControl::setNanSafe);
        cls.def("setWeighted", &StatisticsControl::setWeighted);
        cls.def("setCalcErrorFromInputVariance", &StatisticsControl::setCalcErrorFromInputVariance);
        cls.def("setCalcErrorMosaicMode", &StatisticsControl::setCalcErrorMosaicMode);
    });

    wrappers.wrapType(nb::class_<Statistics>(wrappers.module, "Statistics"), [](auto &mod, auto &cls) {
        cls.def("getResult", &Statistics::getResult, "prop"_a = Property::NOTHING);
        cls.def("getError", &Statistics::getError, "prop"_a = Property::NOTHING);
        cls.def("getValue", &Statistics::getValue, "prop"_a = Property::NOTHING);
        cls.def("getOrMask", &Statistics::getOrMask);
    });
}
void wrapStatistics(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.image");
    declareStatistics(wrappers);
    declareStatistics<unsigned short>(wrappers);
    declareStatistics<double>(wrappers);
    declareStatistics<float>(wrappers);
    declareStatistics<int>(wrappers);
    // Declare vector overloads separately to prevent casting errors
    // that otherwise (mysteriously) occur when overloads are tried
    // in order.
    declareStatisticsVectorOverloads<unsigned short>(wrappers);
    declareStatisticsVectorOverloads<double>(wrappers);
    declareStatisticsVectorOverloads<float>(wrappers);
    declareStatisticsVectorOverloads<int>(wrappers);
    wrappers.module.def("testProp", [](Property const flags) {return flags;});
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
