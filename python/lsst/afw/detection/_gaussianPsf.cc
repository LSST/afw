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

#include <pybind11/pybind11.h>

#include "lsst/afw/table/io/python.h"  // for addPersistableMethods
#include "lsst/afw/detection/GaussianPsf.h"

#include "lsst/utils/python.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace detection {

void wrapGaussianPsf(utils::python::WrapperCollection& wrappers) {
    wrappers.wrapType(
            py::class_<GaussianPsf, Psf>(wrappers.module, "GaussianPsf"),
            [](auto& mod, auto& cls) {
                table::io::python::addPersistableMethods<GaussianPsf>(cls);

                cls.def(py::init<int, int, double>(), "width"_a, "height"_a, "sigma"_a);
                cls.def(py::init<lsst::geom::Extent2I const&, double>(), "dimensions"_a, "sigma"_a);

                cls.def("clone", &GaussianPsf::clone);
                cls.def("resized", &GaussianPsf::resized, "width"_a, "height"_a);
                cls.def("getDimensions", &GaussianPsf::getDimensions);
                cls.def("getSigma", &GaussianPsf::getSigma);
                cls.def("isPersistable", &GaussianPsf::isPersistable);
            });
}

}  // namespace detection
}  // namespace afw
}  // namespace lsst
