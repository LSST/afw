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

#include <pybind11/pybind11.h>
//#include <pybind11/operators.h>
//#include <pybind11/stl.h>

#include "lsst/afw/geom/ellipses/PixelRegion.h"

namespace py = pybind11;

using namespace lsst::afw::geom::ellipses;

PYBIND11_PLUGIN(_pixelRegion) {
    py::module mod("_pixelRegion", "Python wrapper for afw _pixelRegion library");

    py::class_<PixelRegion> clsPixelRegion(mod, "PixelRegion");

    /* Constructors */
    clsPixelRegion.def(py::init<Ellipse const &>());

    /* Members */
    clsPixelRegion.def("getBBox", &PixelRegion::getBBox);
    clsPixelRegion.def("getSpanAt", &PixelRegion::getSpanAt);
    clsPixelRegion.def("__iter__", [](const PixelRegion & self) {
            return py::make_iterator(self.begin(), self.end());
    }, py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */);

    return mod.ptr();
}