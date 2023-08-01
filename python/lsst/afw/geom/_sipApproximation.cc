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
#include <lsst/utils/python.h>
#include "pybind11/stl.h"
#include "pybind11/eigen.h"

#include "ndarray/pybind11.h"

#include "lsst/afw/geom/SipApproximation.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace {

using PySipApproximation = py::class_<SipApproximation>;

void declareSipApproximation(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PySipApproximation(wrappers.module, "SipApproximation"), [](auto &mod, auto &cls) {
        cls.def(py::init<std::shared_ptr<TransformPoint2ToPoint2>, lsst::geom::Point2D const &,
                         Eigen::MatrixXd const &, lsst::geom::Box2D const &, lsst::geom::Extent2I const &,
                         int, bool, double>(),
                "pixelToIwc"_a, "crpix"_a, "cd"_a, "bbox"_a, "gridShape"_a, "order"_a, "useInverse"_a = true,
                "svdThreshold"_a = -1);

        cls.def(py::init<std::shared_ptr<TransformPoint2ToPoint2>, lsst::geom::Point2D const &,
                         Eigen::MatrixXd const &, lsst::geom::Box2D const &, lsst::geom::Extent2I const &,
                         ndarray::Array<double const, 2> const &, ndarray::Array<double const, 2> const &,
                         ndarray::Array<double const, 2> const &, ndarray::Array<double const, 2> const &,
                         bool>(),
                "pixelToIwc"_a, "crpix"_a, "cd"_a, "bbox"_a, "gridShape"_a, "a"_a, "b"_a, "ap"_a, "bp"_a,
                "useInverse"_a = true);

        using ScalarTransform = lsst::geom::Point2D (SipApproximation::*)(lsst::geom::Point2D const &) const;
        using VectorTransform = std::vector<lsst::geom::Point2D> (SipApproximation::*)(
                std::vector<lsst::geom::Point2D> const &) const;

        cls.def("getOrder", &SipApproximation::getOrder);
        cls.def("getA", py::overload_cast<int, int>(&SipApproximation::getA, py::const_), "p"_a, "q"_a);
        cls.def("getB", py::overload_cast<int, int>(&SipApproximation::getB, py::const_), "p"_a, "q"_a);
        cls.def("getAP", py::overload_cast<int, int>(&SipApproximation::getAP, py::const_), "p"_a, "q"_a);
        cls.def("getBP", py::overload_cast<int, int>(&SipApproximation::getBP, py::const_), "p"_a, "q"_a);
        cls.def("getA", py::overload_cast<>(&SipApproximation::getA, py::const_));
        cls.def("getB", py::overload_cast<>(&SipApproximation::getB, py::const_));
        cls.def("getAP", py::overload_cast<>(&SipApproximation::getAP, py::const_));
        cls.def("getBP", py::overload_cast<>(&SipApproximation::getBP, py::const_));
        cls.def("applyForward", (ScalarTransform)&SipApproximation::applyForward);
        cls.def("applyForward", (VectorTransform)&SipApproximation::applyForward);
        cls.def("applyInverse", (ScalarTransform)&SipApproximation::applyInverse);
        cls.def("applyInverse", (VectorTransform)&SipApproximation::applyInverse);
        cls.def("getGridStep", &SipApproximation::getGridStep);
        cls.def("getGridShape", &SipApproximation::getGridShape);
        cls.def("getBBox", &SipApproximation::getBBox);
        cls.def("getPixelOrigin", &SipApproximation::getPixelOrigin);
        cls.def("getCdMatrix", &SipApproximation::getCdMatrix);
        cls.def("updateGrid", &SipApproximation::updateGrid, "shape"_a);
        cls.def("refineGrid", &SipApproximation::refineGrid, "factor"_a = 2);
        cls.def("fit", &SipApproximation::fit, "order"_a, "svdThreshold"_a = -1);
        cls.def("computeMaxDeviation", &SipApproximation::computeMaxDeviation);
    });
}
}  // namespace
void wrapSipApproximation(lsst::utils::python::WrapperCollection &wrappers) {
    declareSipApproximation(wrappers);
}
}  // namespace geom
}  // namespace afw
}  // namespace lsst
