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
#include <pybind11/operators.h>
#include <pybind11/stl.h>

#include "numpy/arrayobject.h"
#include "ndarray/pybind11.h"
#include "ndarray/converter.h"

#include "lsst/afw/math/BoundedField.h"
#include "lsst/afw/math/ChebyshevBoundedField.h"

namespace py = pybind11;
PYBIND11_DECLARE_HOLDER_TYPE(MyType, std::shared_ptr<MyType>);

using namespace lsst::afw::math;

PYBIND11_PLUGIN(_chebyshevBoundedField) {
    py::module mod("_chebyshevBoundedField", "Python wrapper for afw _chebyshevBoundedField library");
    
    if (_import_array() < 0) {
            PyErr_SetString(PyExc_ImportError, "numpy.core.multiarray failed to import");
            return nullptr;
    };


    /* Module level */
    py::class_<ChebyshevBoundedFieldControl>
        clsChebyshevBoundedFieldControl(mod, "ChebyshevBoundedFieldControl");
    py::class_<ChebyshevBoundedField, std::shared_ptr<ChebyshevBoundedField>, BoundedField>
        clsChebyshevBoundedField(mod, "ChebyshevBoundedField");

    /* Member types and enums */
    using Control = ChebyshevBoundedFieldControl;

    /* Constructors */
    clsChebyshevBoundedFieldControl.def(py::init<>());
    clsChebyshevBoundedField.def(py::init<lsst::afw::geom::Box2I const &, ndarray::Array<double const,2,2>
        const &>());

    /* Operators */
    clsChebyshevBoundedField.def("__rmul__", [](ChebyshevBoundedField &bf, double const scale){
            return bf*scale;
    }, py::is_operator());
    clsChebyshevBoundedField.def("__mul__", [](ChebyshevBoundedField &bf, double const scale){
            return bf*scale;
    }, py::is_operator());

    /* Members */
    clsChebyshevBoundedFieldControl.def_readwrite("orderX", &ChebyshevBoundedFieldControl::orderX);
    clsChebyshevBoundedFieldControl.def_readwrite("orderY", &ChebyshevBoundedFieldControl::orderY);
    clsChebyshevBoundedFieldControl.def_readwrite("triangular", &ChebyshevBoundedFieldControl::triangular);
    
    clsChebyshevBoundedField.def("getCoefficients", &ChebyshevBoundedField::getCoefficients);
    clsChebyshevBoundedField.def_static("fit", (PTR(ChebyshevBoundedField) (*)
        (lsst::afw::geom::Box2I const &,
         ndarray::Array<double const,1> const &,
         ndarray::Array<double const,1> const &,
         ndarray::Array<double const,1> const &,
         Control const &)) &ChebyshevBoundedField::fit);
    clsChebyshevBoundedField.def_static("fit", (PTR(ChebyshevBoundedField) (*)
        (lsst::afw::geom::Box2I const &,
         ndarray::Array<double const,1> const &,
         ndarray::Array<double const,1> const &,
         ndarray::Array<double const,1> const &,
         ndarray::Array<double const,1> const &,
         Control const &)) &ChebyshevBoundedField::fit);
    
    clsChebyshevBoundedField.def("truncate", &ChebyshevBoundedField::truncate);

    return mod.ptr();
}