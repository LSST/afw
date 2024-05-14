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
#include <lsst/cpputils/python.h>

#include <nanobind/stl/vector.h>

#include "lsst/afw/math/Kernel.h"
#include "lsst/afw/table/io/python.h"  // for addPersistableMethods

namespace nb = nanobind;

using namespace nb::literals;

using namespace lsst::afw::math;
namespace lsst {
namespace afw {
namespace math {
void wrapKernel(lsst::cpputils::python::WrapperCollection &wrappers) {
    using PyKernel = nb::class_<Kernel>;

    wrappers.addSignatureDependency("lsst.afw.table");
    wrappers.addSignatureDependency("lsst.afw.table.io");

    wrappers.wrapType(PyKernel(wrappers.module, "Kernel"), [](auto &mod, auto &cls) {
        lsst::afw::table::io::python::addPersistableMethods<Kernel>(cls);

        cls.def("clone", &Kernel::clone);
        cls.def("resized", &Kernel::resized, "width"_a, "height"_a);
        cls.def("computeImage", &Kernel::computeImage, "image"_a, "doNormalize"_a, "x"_a = 0.0, "y"_a = 0.0);
        cls.def("getDimensions", &Kernel::getDimensions);
        cls.def("setDimensions", &Kernel::setDimensions);
        cls.def("setWidth", &Kernel::setWidth);
        cls.def("setHeight", &Kernel::setHeight);
        cls.def("getWidth", &Kernel::getWidth);
        cls.def("getHeight", &Kernel::getHeight);
        cls.def("getCtr", &Kernel::getCtr);
        cls.def("getBBox", &Kernel::getBBox);
        cls.def("getNKernelParameters", &Kernel::getNKernelParameters);
        cls.def("getNSpatialParameters", &Kernel::getNSpatialParameters);
        cls.def("getSpatialFunction", &Kernel::getSpatialFunction);
        cls.def("getSpatialFunctionList", &Kernel::getSpatialFunctionList);
        cls.def("getKernelParameter", &Kernel::getKernelParameter);
        cls.def("getKernelParameters", &Kernel::getKernelParameters);
        cls.def("growBBox", &Kernel::growBBox);
        cls.def("shrinkBBox", &Kernel::shrinkBBox);
        cls.def("setCtr", &Kernel::setCtr);
        cls.def("getSpatialParameters", &Kernel::getSpatialParameters);
        cls.def("isSpatiallyVarying", &Kernel::isSpatiallyVarying);
        cls.def("setKernelParameters",
                (void (Kernel::*)(std::vector<double> const &)) & Kernel::setKernelParameters);
        cls.def("setKernelParameters",
                (void (Kernel::*)(std::pair<double, double> const &)) & Kernel::setKernelParameters);
        cls.def("setSpatialParameters", &Kernel::setSpatialParameters);
        cls.def("computeKernelParametersFromSpatialModel", &Kernel::computeKernelParametersFromSpatialModel);
        cls.def("toString", &Kernel::toString, "prefix"_a = "");
        cls.def("computeCache", &Kernel::computeCache);
        cls.def("getCacheSize", &Kernel::getCacheSize);
    });

    using PyFixedKernel = nb::class_<FixedKernel, Kernel>;
    wrappers.wrapType(PyFixedKernel(wrappers.module, "FixedKernel"), [](auto &mod, auto &cls) {
        cls.def(nb::init<>());
        cls.def(nb::init<lsst::afw::image::Image<Kernel::Pixel> const &>(), "image"_a);
        cls.def(nb::init<lsst::afw::math::Kernel const &, lsst::geom::Point2D const &>(), "kernel"_a,
                "pos"_a);
        cls.def("clone", &FixedKernel::clone);
        cls.def("resized", &FixedKernel::resized, "width"_a, "height"_a);
        cls.def("toString", &FixedKernel::toString, "prefix"_a = "");
        cls.def("getSum", &FixedKernel::getSum);
        cls.def("isPersistable", &FixedKernel::isPersistable);
    });

    using PyAnalyticKernel = nb::class_<AnalyticKernel, Kernel>;
    wrappers.wrapType(PyAnalyticKernel(wrappers.module, "AnalyticKernel"), [](auto &mod, auto &cls) {
        cls.def(nb::init<>());
        // Workaround for NullSpatialFunction and nb::arg not playing well with Citizen (TODO: no longer
        // needed?)
        cls.def(nb::init<int, int, AnalyticKernel::KernelFunction const &>(), "width"_a, "height"_a,
                "kernelFunction"_a);
        cls.def(nb::init<int, int, AnalyticKernel::KernelFunction const &, Kernel::SpatialFunction const &>(),
                "width"_a, "height"_a, "kernelFunction"_a, "spatialFunction"_a);
        cls.def(nb::init<int, int, AnalyticKernel::KernelFunction const &,
                         std::vector<Kernel::SpatialFunctionPtr> const &>(),
                "width"_a, "height"_a, "kernelFunction"_a, "spatialFunctionList"_a);
        cls.def("clone", &AnalyticKernel::clone);
        cls.def("resized", &AnalyticKernel::resized, "width"_a, "height"_a);
        cls.def("computeImage", &AnalyticKernel::computeImage, "image"_a, "doNormalize"_a, "x"_a = 0.0,
                "y"_a = 0.0);
        cls.def("getKernelParameters", &AnalyticKernel::getKernelParameters);
        cls.def("getKernelFunction", &AnalyticKernel::getKernelFunction);
        cls.def("toString", &AnalyticKernel::toString, "prefix"_a = "");
        cls.def("isPersistable", &AnalyticKernel::isPersistable);
    });

    using PyDeltaFunctionKernel =
            nb::class_<DeltaFunctionKernel, Kernel>;
    wrappers.wrapType(
            PyDeltaFunctionKernel(wrappers.module, "DeltaFunctionKernel"), [](auto &mod, auto &cls) {
                cls.def(nb::init<int, int, lsst::geom::Point2I const &>(), "width"_a, "height"_a, "point"_a);
                cls.def("clone", &DeltaFunctionKernel::clone);
                cls.def("resized", &DeltaFunctionKernel::resized, "width"_a, "height"_a);
                cls.def("getPixel", &DeltaFunctionKernel::getPixel);
                cls.def("toString", &DeltaFunctionKernel::toString, "prefix"_a = "");
                cls.def("isPersistable", &DeltaFunctionKernel::isPersistable);
            });

    using PyLinearCombinationKernel =
            nb::class_<LinearCombinationKernel, Kernel>;
    wrappers.wrapType(
            PyLinearCombinationKernel(wrappers.module, "LinearCombinationKernel"), [](auto &mod, auto &cls) {
                cls.def(nb::init<>());
                cls.def(nb::init<KernelList const &, std::vector<double> const &>(), "kernelList"_a,
                        "kernelParameters"_a);
                cls.def(nb::init<KernelList const &, Kernel::SpatialFunction const &>(), "kernelList"_a,
                        "spatialFunction"_a);
                cls.def(nb::init<KernelList const &, std::vector<Kernel::SpatialFunctionPtr> const &>(),
                        "kernelList"_a, "spatialFunctionList"_a);
                cls.def("clone", &LinearCombinationKernel::clone);
                cls.def("resized", &LinearCombinationKernel::resized, "width"_a, "height"_a);
                cls.def("getKernelParameters", &LinearCombinationKernel::getKernelParameters);
                cls.def("getKernelList", &LinearCombinationKernel::getKernelList);
                cls.def("getKernelSumList", &LinearCombinationKernel::getKernelSumList);
                cls.def("getNBasisKernels", &LinearCombinationKernel::getNBasisKernels);
                cls.def("checkKernelList", &LinearCombinationKernel::checkKernelList);
                cls.def("isDeltaFunctionBasis", &LinearCombinationKernel::isDeltaFunctionBasis);
                cls.def("refactor", &LinearCombinationKernel::refactor);
                cls.def("toString", &LinearCombinationKernel::toString, "prefix"_a = "");
                cls.def("isPersistable", &LinearCombinationKernel::isPersistable);
            });

    using PySeparableKernel = nb::class_<SeparableKernel, Kernel>;
    wrappers.wrapType(PySeparableKernel(wrappers.module, "SeparableKernel"), [](auto &mod, auto &cls) {
        cls.def(nb::init<>());
        // Workaround for NullSpatialFunction and nb::arg not playing well with Citizen (TODO: no longer
        // needed?)
        cls.def(nb::init<int, int, SeparableKernel::KernelFunction const &,
                         SeparableKernel::KernelFunction const &>(),
                "width"_a, "height"_a, "kernelColFunction"_a, "kernelRowFunction"_a);
        cls.def(nb::init<int, int, SeparableKernel::KernelFunction const &,
                         SeparableKernel::KernelFunction const &, Kernel::SpatialFunction const &>(),
                "width"_a, "height"_a, "kernelColFunction"_a, "kernelRowFunction"_a, "spatialFunction"_a);
        cls.def(nb::init<int, int, SeparableKernel::KernelFunction const &,
                         SeparableKernel::KernelFunction const &,
                         std::vector<Kernel::SpatialFunctionPtr> const &>(),
                "width"_a, "height"_a, "kernelColFunction"_a, "kernelRowFunction"_a, "spatialFunctionList"_a);
        cls.def("clone", &SeparableKernel::clone);
        cls.def("resized", &SeparableKernel::resized, "width"_a, "height"_a);
        cls.def("computeVectors", &SeparableKernel::computeVectors);
        cls.def("getKernelParameter", &SeparableKernel::getKernelParameter);
        cls.def("getKernelParameters", &SeparableKernel::getKernelParameters);
        cls.def("getKernelColFunction", &SeparableKernel::getKernelColFunction);
        cls.def("getKernelRowFunction", &SeparableKernel::getKernelRowFunction);
        cls.def("toString", &SeparableKernel::toString, "prefix"_a = "");
        cls.def("computeCache", &SeparableKernel::computeCache);
        cls.def("getCacheSize", &SeparableKernel::getCacheSize);
    });
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
