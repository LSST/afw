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

#include "lsst/afw/image/Defect.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace image {
namespace {

using PyDefectBase = nb::class_<DefectBase>;

void declareDefects(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyDefectBase(wrappers.module, "DefectBase"), [](auto &mod, auto &cls) {
        /* Constructors */
        cls.def(nb::init<const lsst::geom::Box2I &>(), "bbox"_a);

        /* Members */
        cls.def("getBBox", &DefectBase::getBBox);
        cls.def("getX0", &DefectBase::getX0);
        cls.def("getX1", &DefectBase::getX1);
        cls.def("getY0", &DefectBase::getY0);
        cls.def("getY1", &DefectBase::getY1);
        cls.def("clip", &DefectBase::clip);
        cls.def("shift", (void (DefectBase::*)(int, int)) & DefectBase::shift, "dx"_a, "dy"_a);
        cls.def("shift", (void (DefectBase::*)(lsst::geom::Extent2I const &)) & DefectBase::shift, "d"_a);
    });
}
}  // namespace
void wrapDefect(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.geom");
    declareDefects(wrappers);
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
