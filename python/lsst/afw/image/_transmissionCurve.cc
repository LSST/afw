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

#include <memory>

#include "ndarray/pybind11.h"

#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/image/TransmissionCurve.h"
#include "lsst/afw/table/io/python.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace image {

using PyTransmissionCurve =
        py::class_<TransmissionCurve, typehandling::Storable>;

void wrapTransmissionCurve(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.afw.typehandling");
    wrappers.addSignatureDependency("lsst.afw.geom");
    wrappers.wrapType(PyTransmissionCurve(wrappers.module, "TransmissionCurve"), [](auto &mod, auto &cls) {
        table::io::python::addPersistableMethods(cls);

        cls.def_static("makeIdentity", &TransmissionCurve::makeIdentity);
        cls.def_static("makeSpatiallyConstant", &TransmissionCurve::makeSpatiallyConstant, "throughput"_a,
                       "wavelengths"_a, "throughputAtMin"_a = 0.0, "throughputAtMax"_a = 0.0);
        cls.def_static("makeRadial", &TransmissionCurve::makeRadial, "throughput"_a, "wavelengths"_a,
                       "radii"_a, "throughputAtMin"_a = 0.0, "throughputAtMax"_a = 0.0);
        cls.def("__mul__", &TransmissionCurve::multipliedBy, py::is_operator());
        cls.def("multipliedBy", &TransmissionCurve::multipliedBy);
        cls.def("transformedBy", &TransmissionCurve::transformedBy, "transform"_a);
        cls.def("getWavelengthBounds", &TransmissionCurve::getWavelengthBounds);
        cls.def("getThroughputAtBounds", &TransmissionCurve::getThroughputAtBounds);
        cls.def("sampleAt",
                (void (TransmissionCurve::*)(lsst::geom::Point2D const &,
                                             ndarray::Array<double const, 1, 1> const &,
                                             ndarray::Array<double, 1, 1> const &) const) &
                        TransmissionCurve::sampleAt,
                "position"_a, "wavelengths"_a, "out"_a);
        cls.def("sampleAt",
                (ndarray::Array<double, 1, 1>(TransmissionCurve::*)(
                        lsst::geom::Point2D const &, ndarray::Array<double const, 1, 1> const &) const) &
                        TransmissionCurve::sampleAt,
                "position"_a, "wavelengths"_a);
    });
}

}  // namespace image
}  // namespace afw
}  // namespace lsst
