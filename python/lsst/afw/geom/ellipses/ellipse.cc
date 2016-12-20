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
#include <pybind11/stl.h>

#include "numpy/arrayobject.h"
#include "ndarray/pybind11.h"
#include "ndarray/converter.h"

#include "lsst/afw/geom/ellipses/BaseCore.h"
#include "lsst/afw/geom/Point.h"
#include "lsst/afw/geom/ellipses/GridTransform.h"
#include "lsst/afw/geom/ellipses/Ellipse.h"

namespace py = pybind11;
using namespace pybind11::literals;

using namespace lsst::afw::geom;
using namespace ellipses;

PYBIND11_PLUGIN(_ellipse) {
    py::module mod("_ellipse", "Python wrapper for afw _ellipse library");

    if (_import_array() < 0) {
            PyErr_SetString(PyExc_ImportError, "numpy.core.multiarray failed to import");
            return nullptr;
        }

    /* Module level */
    py::class_<Ellipse, std::shared_ptr<Ellipse>> clsEllipse(mod, "Ellipse");

//    /* Member types and enums */
//    py::class_<Ellipse::GridTransform> clsEllipseGridTransform(clsEllipse, "GridTransform");
//
//    /* Constructors */
//    clsEllipseGridTransform.def(py::init<Ellipse const &>());
//
//    /* Members */
//    clsEllipseGridTransform.def("getMatrix", &Ellipse::GridTransform::getMatrix);
//    clsEllipseGridTransform.def("d", &Ellipse::GridTransform::d);
//    clsEllipseGridTransform.def("getDeterminant", &Ellipse::GridTransform::getDeterminant);
//    clsEllipseGridTransform.def("invert", &Ellipse::GridTransform::invert);

    /* Constructors */
    clsEllipse.def(py::init<BaseCore const &, Point2D const &>(), "core"_a, "center"_a=Point2D());
    clsEllipse.def(py::init<Ellipse const &>());

    /* Operators */

    /* Members */
    clsEllipse.def("getCore", [](Ellipse & ellipse){
        return ellipse.getCorePtr();
    });
    clsEllipse.def("getCenter", (Point2D & (Ellipse::*)())&Ellipse::getCenter);
    clsEllipse.def("setCenter", &Ellipse::setCenter);
    clsEllipse.def("setCore", &Ellipse::setCore);
    clsEllipse.def("normalize", &Ellipse::normalize);
    clsEllipse.def("grow", &Ellipse::grow);
    clsEllipse.def("scale", &Ellipse::scale);
    clsEllipse.def("shift", &Ellipse::shift);
    clsEllipse.def("getParameterVector", &Ellipse::getParameterVector);
    clsEllipse.def("setParameterVector", &Ellipse::setParameterVector);
//    clsEllipse.def("transform", (Transformer const (Ellipse::*)(AffineTransform const &) const) &Ellipse::transform);
//    clsEllipse.def("convolve", (Convolution const (Ellipse::*)(Ellipse const &) const) &Ellipse::convolve);
    clsEllipse.def("getGridTransform", [](Ellipse & self) -> AffineTransform {
        return self.getGridTransform(); // delibarate conversion to AffineTransform
    });
    clsEllipse.def("computeBBox", &Ellipse::computeBBox);

    return mod.ptr();
}