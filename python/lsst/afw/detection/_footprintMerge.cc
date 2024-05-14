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

#include "lsst/cpputils/python.h"

#include "lsst/afw/detection/FootprintMerge.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace lsst {
namespace afw {
namespace detection {

void wrapFootprintMerge(cpputils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.table");

    wrappers.wrapType(
            nb::class_<FootprintMergeList>(wrappers.module, "FootprintMergeList"), [](auto &mod, auto &cls) {
                cls.def(nb::init<afw::table::Schema &, std::vector<std::string> const &,
                                 afw::table::Schema const &>(),
                        "sourceSchema"_a, "filterList"_a, "initialPeakSchema"_a);
                cls.def(nb::init<afw::table::Schema &, std::vector<std::string> const &>(), "sourceSchema"_a,
                        "filterList"_a);

                cls.def("getPeakSchema", &FootprintMergeList::getPeakSchema);
                cls.def("addCatalog", &FootprintMergeList::addCatalog, "sourceTable"_a, "inputCat"_a,
                        "filter"_a, "minNewPeakDist"_a = -1., "doMerge"_a = true, "maxSamePeakDist"_a = -1.);
                cls.def("clearCatalog", &FootprintMergeList::clearCatalog);
                cls.def("getFinalSources", &FootprintMergeList::getFinalSources, "outputCat"_a);
            });
}

}  // namespace detection
}  // namespace afw
}  // namespace lsst
