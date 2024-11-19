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

#include <memory>

#include <nanobind/nanobind.h>

#include "lsst/cpputils/python.h"
#include "lsst/cpputils/python/PySharedPtr.h"

#include "lsst/pex/exceptions/Runtime.h"
#include "lsst/pex/exceptions/python/Exception.h"
#include "lsst/geom/Point.h"
#include "lsst/afw/image/Color.h"
#include "lsst/afw/table/io/python.h"  // for addPersistableMethods
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/typehandling/python.h"
#include "lsst/afw/detection/Psf.h"
#include "lsst/afw/detection/python.h"

namespace nb = nanobind;
using namespace nanobind::literals;

using lsst::cpputils::python::PySharedPtr;

namespace lsst {
namespace afw {
namespace detection {


void wrapPsf(utils::python::WrapperCollection& wrappers) {
    auto clsPsf = wrappers.wrapType(
            nb::class_<Psf, typehandling::Storable, PsfTrampoline<>>(
                wrappers.module, "Psf"
            ),
            [](auto& mod, auto& cls) {
                table::io::python::addPersistableMethods<Psf>(cls);
		        typehandling::python::addStorableMethods<Psf>(cls);
                cls.def(nb::init<bool, size_t>(), "isFixed"_a=false, "capacity"_a=100);  // Constructor for pure-Python subclasses
                cls.def("clone", &Psf::clone);
                cls.def("resized", &Psf::resized, "width"_a, "height"_a);

                cls.def("computeImage",
                        &Psf::computeImage,
                        "position"_a,
                        "color"_a = image::Color(),
                        "owner"_a = int(Psf::ImageOwnerEnum::COPY)
                );
                cls.def("computeKernelImage",
                        &Psf::computeKernelImage,
                        "position"_a,
                        "color"_a = image::Color(),
                        "owner"_a = int(Psf::ImageOwnerEnum::COPY)
                );
                cls.def("computePeak",
                        &Psf::computePeak,
                        "position"_a,
                        "color"_a = image::Color()
                );
                cls.def("computeApertureFlux",
                        &Psf::computeApertureFlux,
                        "radius"_a,
                        "position"_a,
                        "color"_a = image::Color()
                );
                cls.def("computeShape",
                        &Psf::computeShape,
                        "position"_a,
                        "color"_a = image::Color()
                );
                cls.def("computeBBox",
                        &Psf::computeBBox,
                        "position"_a,
                        "color"_a = image::Color()
                );
                cls.def("computeImageBBox",
                        &Psf::computeImageBBox,
                        "position"_a,
                        "color"_a = image::Color()
                );
                cls.def("computeKernelBBox",
                        &Psf::computeKernelBBox,
                        "position"_a,
                        "color"_a = image::Color()
                );
                cls.def("getLocalKernel",
                        &Psf::getLocalKernel,
                        "position"_a,
                        "color"_a = image::Color()
                );

                cls.def("getAverageColor", &Psf::getAverageColor);
                cls.def("getAveragePosition", &Psf::getAveragePosition);
                cls.def_static("recenterKernelImage", &Psf::recenterKernelImage, "im"_a, "position"_a,
                               "warpAlgorithm"_a = "lanczos5", "warpBuffer"_a = 5);
                cls.def("getCacheCapacity", &Psf::getCacheCapacity);
                cls.def("setCacheCapacity", &Psf::setCacheCapacity);
            }
    );

    wrappers.wrapType(nb::enum_<Psf::ImageOwnerEnum>(clsPsf, "ImageOwnerEnum"), [](auto& mod, auto& enm) {
        enm.value("COPY", Psf::ImageOwnerEnum::COPY);
        enm.value("INTERNAL", Psf::ImageOwnerEnum::INTERNAL);
        enm.export_values();
    });
}

}  // namespace detection
}  // namespace afw
}  // namespace lsst
