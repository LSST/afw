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

#include "ndarray/nanobind.h"

#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/cameraGeom/Amplifier.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace cameraGeom {

using PyAmplifier = nb::class_<Amplifier>;
using PyAmplifierBuilder = nb::class_<Amplifier::Builder, Amplifier>;

void wrapAmplifier(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.afw.table");
    wrappers.wrapType(nb::enum_<ReadoutCorner>(wrappers.module, "ReadoutCorner"), [](auto &mod, auto &enm) {
        enm.value("LL", ReadoutCorner::LL);
        enm.value("LR", ReadoutCorner::LR);
        enm.value("UR", ReadoutCorner::UR);
        enm.value("UL", ReadoutCorner::UL);
    });
    wrappers.wrapType(nb::enum_<AssemblyState>(wrappers.module, "AssemblyState"), [](auto &mod, auto &enm) {
        enm.value("RAW", AssemblyState::RAW);
        enm.value("SCIENCE", AssemblyState::SCIENCE);
    });
    auto amplifier = wrappers.wrapType(PyAmplifier(wrappers.module, "Amplifier"), [](auto &mod, auto &cls) {
        cls.def_static("getRecordSchema", &Amplifier::getRecordSchema);
        cls.def("toRecord", &Amplifier::toRecord);
        cls.def("rebuild", &Amplifier::rebuild);
        cls.def("getName", &Amplifier::getName);
        cls.def("getBBox", &Amplifier::getBBox);
        cls.def("getGain", &Amplifier::getGain);
        cls.def("getReadNoise", &Amplifier::getReadNoise);
        cls.def("getSaturation", &Amplifier::getSaturation);
        cls.def("getSuspectLevel", &Amplifier::getSuspectLevel);
        cls.def("getReadoutCorner", &Amplifier::getReadoutCorner);
        cls.def("getLinearityCoeffs", &Amplifier::getLinearityCoeffs);
        cls.def("getLinearityType", &Amplifier::getLinearityType);
        cls.def("getLinearityThreshold", &Amplifier::getLinearityThreshold);
        cls.def("getLinearityMaximum", &Amplifier::getLinearityMaximum);
        cls.def("getLinearityUnits", &Amplifier::getLinearityUnits);
        cls.def("getRawBBox", &Amplifier::getRawBBox);
        cls.def("getRawDataBBox", &Amplifier::getRawDataBBox);
        cls.def("getRawFlipX", &Amplifier::getRawFlipX);
        cls.def("getRawFlipY", &Amplifier::getRawFlipY);
        cls.def("getRawXYOffset", &Amplifier::getRawXYOffset);
        cls.def("getRawHorizontalOverscanBBox", &Amplifier::getRawHorizontalOverscanBBox);
        cls.def("getRawVerticalOverscanBBox", &Amplifier::getRawVerticalOverscanBBox);
        cls.def("getRawPrescanBBox", &Amplifier::getRawPrescanBBox);
        cls.def("getRawSerialOverscanBBox", &Amplifier::getRawSerialOverscanBBox);
        cls.def("getRawParallelOverscanBBox", &Amplifier::getRawParallelOverscanBBox);
        cls.def("getRawSerialPrescanBBox", &Amplifier::getRawSerialPrescanBBox);
        cls.def("getRawHorizontalPrescanBBox", &Amplifier::getRawHorizontalPrescanBBox);
    });
    wrappers.wrapType(PyAmplifierBuilder(amplifier, "Builder"), [](auto &mod, auto &cls) {
        cls.def_static("fromRecord", &Amplifier::Builder::fromRecord);
        cls.def(nb::init());
        cls.def("finish", &Amplifier::Builder::finish);
        cls.def("assign", [](Amplifier::Builder &self, Amplifier const &other) { self = other; });
        cls.def("setName", &Amplifier::Builder::setName, "name"_a);
        cls.def("setBBox", &Amplifier::Builder::setBBox, "bbox"_a);
        cls.def("setGain", &Amplifier::Builder::setGain, "gain"_a);
        cls.def("setReadNoise", &Amplifier::Builder::setReadNoise, "readNoise"_a);
        cls.def("setSaturation", &Amplifier::Builder::setSaturation, "saturation"_a);
        cls.def("setSuspectLevel", &Amplifier::Builder::setSuspectLevel, "suspectLevel"_a);
        cls.def("setReadoutCorner", &Amplifier::Builder::setReadoutCorner, "corner"_a);
        cls.def("setLinearityCoeffs", &Amplifier::Builder::setLinearityCoeffs, "coeffs"_a);
        // Backwards compatibility: accept std::vector (list in Python) in
        // addition to ndarray::Array (np.ndarray)
        cls.def("setLinearityCoeffs", [](Amplifier::Builder &self, std::vector<double> const &coeffs) {
            ndarray::Array<double, 1, 1> array = ndarray::allocate(coeffs.size());
            std::copy(coeffs.begin(), coeffs.end(), array.begin());
            self.setLinearityCoeffs(array);
        });
        cls.def("setLinearityType", &Amplifier::Builder::setLinearityType, "type"_a);
        cls.def("setLinearityThreshold", &Amplifier::Builder::setLinearityThreshold, "threshold"_a);
        cls.def("setLinearityMaximum", &Amplifier::Builder::setLinearityMaximum, "maximum"_a);
        cls.def("setLinearityUnits", &Amplifier::Builder::setLinearityUnits, "units"_a);
        cls.def("setRawBBox", &Amplifier::Builder::setRawBBox, "bbox"_a);
        cls.def("setRawDataBBox", &Amplifier::Builder::setRawDataBBox, "bbox"_a);
        cls.def("setRawFlipX", &Amplifier::Builder::setRawFlipX, "rawFlipX"_a);
        cls.def("setRawFlipY", &Amplifier::Builder::setRawFlipY, "rawFlipY"_a);
        cls.def("setRawXYOffset", &Amplifier::Builder::setRawXYOffset, "offset"_a);
        cls.def("setRawHorizontalOverscanBBox", &Amplifier::Builder::setRawHorizontalOverscanBBox, "bbox"_a);
        cls.def("setRawVerticalOverscanBBox", &Amplifier::Builder::setRawVerticalOverscanBBox, "bbox"_a);
        cls.def("setRawPrescanBBox", &Amplifier::Builder::setRawPrescanBBox, "bbox"_a);
        cls.def("setRawSerialOverscanBBox", &Amplifier::Builder::setRawSerialOverscanBBox, "bbox"_a);
        cls.def("setRawParallelOverscanBBox", &Amplifier::Builder::setRawParallelOverscanBBox, "bbox"_a);
        cls.def("setRawSerialPrescanBBox", &Amplifier::Builder::setRawSerialPrescanBBox, "bbox"_a);
        cls.def("setRawHorizontalPrescanBBox", &Amplifier::Builder::setRawHorizontalPrescanBBox, "bbox"_a);
    });
}

}  // namespace cameraGeom
}  // namespace afw
}  // namespace lsst
