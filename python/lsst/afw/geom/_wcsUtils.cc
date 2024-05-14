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
#include "lsst/cpputils/python.h"
#include "nanobind/eigen/dense.h"

#include <memory>

#include "astshim.h"
#include "ndarray/nanobind.h"
#include "ndarray/eigen.h"

#include "lsst/daf/base.h"
#include "lsst/geom/Point.h"
#include "lsst/geom/SpherePoint.h"
#include "lsst/afw/geom/wcsUtils.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace lsst {
namespace afw {
namespace geom {

void wrapWcsUtils(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("createTrivialWcsMetadata", createTrivialWcsMetadata, "wcsName"_a, "xy0"_a);
        mod.def("deleteBasicWcsMetadata", deleteBasicWcsMetadata, "metadata"_a, "wcsName"_a);
        mod.def("getCdMatrixFromMetadata", getCdMatrixFromMetadata, "metadata"_a);
        mod.def("getImageXY0FromMetadata", getImageXY0FromMetadata, "metadata"_a, "wcsName"_a,
                "strip"_a = false);
        // getSipMatrixFromMetadata requires a pure python wrapper to return a matrix when order=0
        mod.def("_getSipMatrixFromMetadata", getSipMatrixFromMetadata, "metadata"_a, "name"_a);
        mod.def("hasSipMatrix", hasSipMatrix, "metadata"_a, "name"_a);
        mod.def("makeSipMatrixMetadata", makeSipMatrixMetadata, "matrix"_a, "name"_a);
        mod.def("makeSimpleWcsMetadata", makeSimpleWcsMetadata, "crpix"_a, "crval"_a, "cdMatrix"_a,
                "projection"_a = "TAN");
        mod.def("makeTanSipMetadata",
                (std::shared_ptr<daf::base::PropertyList>(*)(
                        lsst::geom::Point2D const &, lsst::geom::SpherePoint const &, Eigen::Matrix2d const &,
                        Eigen::MatrixXd const &, Eigen::MatrixXd const &))makeTanSipMetadata,
                "crpix"_a, "crval"_a, "cdMatrix"_a, "sipA"_a, "sipB"_a);
        mod.def("makeTanSipMetadata",
                (std::shared_ptr<daf::base::PropertyList>(*)(
                        lsst::geom::Point2D const &, lsst::geom::SpherePoint const &, Eigen::Matrix2d const &,
                        Eigen::MatrixXd const &, Eigen::MatrixXd const &, Eigen::MatrixXd const &,
                        Eigen::MatrixXd const &))makeTanSipMetadata,
                "crpix"_a, "crval"_a, "cdMatrix"_a, "sipA"_a, "sipB"_a, "sipAp"_a, "sipBp"_a);
        mod.def("stripWcsMetadata", stripWcsMetadata, "metadata"_a);
    });
}

}  // namespace geom
}  // namespace afw
}  // namespace lsst
