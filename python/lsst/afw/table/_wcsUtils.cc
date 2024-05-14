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
#include <vector>

#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"

#include "lsst/cpputils/python.h"

#include "lsst/afw/table/fwd.h"
#include "lsst/afw/table/Simple.h"
#include "lsst/afw/table/Source.h"
#include "lsst/afw/table/wcsUtils.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace table {

using cpputils::python::WrapperCollection;

namespace {

template <typename ReferenceCollection>
void declareUpdateRefCentroids(WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("updateRefCentroids", updateRefCentroids<ReferenceCollection>, "wcs"_a, "refList"_a);
    });
}

template <typename SourceCollection>
void declareUpdateSourceCoords(WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("updateSourceCoords", updateSourceCoords<SourceCollection>, "wcs"_a, "sourceList"_a,
                "include_covariance"_a=true);
    });
}

}  // namespace

void wrapWcsUtils(WrapperCollection &wrappers) {
    declareUpdateRefCentroids<std::vector<std::shared_ptr<lsst::afw::table::SimpleRecord>>>(wrappers);
    declareUpdateRefCentroids<lsst::afw::table::SimpleCatalog>(wrappers);

    declareUpdateSourceCoords<std::vector<std::shared_ptr<lsst::afw::table::SourceRecord>>>(wrappers);
    declareUpdateSourceCoords<lsst::afw::table::SourceCatalog>(wrappers);
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
