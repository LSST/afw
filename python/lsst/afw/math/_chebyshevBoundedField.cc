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
#include <lsst/utils/python.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

#include "ndarray/pybind11.h"

#include "lsst/pex/config/python.h"  // defines LSST_DECLARE_CONTROL_FIELD
#include "lsst/afw/table/io/python.h"

#include "lsst/afw/math/BoundedField.h"
#include "lsst/afw/math/ChebyshevBoundedField.h"

namespace py = pybind11;

using namespace lsst::afw::math;
namespace lsst {
namespace afw {
namespace math {
namespace {
using ClsField = py::class_<ChebyshevBoundedField, BoundedField>;

template <typename PixelT>
void declareTemplates(ClsField &cls) {
    cls.def_static("fit", (std::shared_ptr<ChebyshevBoundedField>(*)(lsst::afw::image::Image<PixelT> const &,
                                                                     ChebyshevBoundedFieldControl const &)) &
                                  ChebyshevBoundedField::fit);
}
void declareChebyshevBoundedField(lsst::utils::python::WrapperCollection &wrappers) {
    /* Module level */

    wrappers.wrapType(
            py::class_<ChebyshevBoundedFieldControl>(wrappers.module, "ChebyshevBoundedFieldControl"),
            [](auto &mod, auto &cls) {
                cls.def(py::init<>());
                LSST_DECLARE_CONTROL_FIELD(cls, ChebyshevBoundedFieldControl, orderX);
                LSST_DECLARE_CONTROL_FIELD(cls, ChebyshevBoundedFieldControl, orderY);
                LSST_DECLARE_CONTROL_FIELD(cls, ChebyshevBoundedFieldControl, triangular);
                cls.def("computeSize", &ChebyshevBoundedFieldControl::computeSize);
            });

    wrappers.wrapType(ClsField(wrappers.module, "ChebyshevBoundedField"), [](auto &mod, auto &cls) {
        using Control = ChebyshevBoundedFieldControl;
        cls.def(py::init<lsst::geom::Box2I const &, ndarray::Array<double const, 2, 2> const &>());
        cls.def("getCoefficients", &ChebyshevBoundedField::getCoefficients);
        cls.def_static("fit", (std::shared_ptr<ChebyshevBoundedField>(*)(
                                      lsst::geom::Box2I const &, ndarray::Array<double const, 1> const &,
                                      ndarray::Array<double const, 1> const &,
                                      ndarray::Array<double const, 1> const &, Control const &)) &
                                      ChebyshevBoundedField::fit);
        cls.def_static("fit", (std::shared_ptr<ChebyshevBoundedField>(*)(
                                      lsst::geom::Box2I const &, ndarray::Array<double const, 1> const &,
                                      ndarray::Array<double const, 1> const &,
                                      ndarray::Array<double const, 1> const &,
                                      ndarray::Array<double const, 1> const &, Control const &)) &
                                      ChebyshevBoundedField::fit);

        cls.def("truncate", &ChebyshevBoundedField::truncate);
        declareTemplates<double>(cls);
        declareTemplates<float>(cls);
    });
}
}  // namespace
void wrapChebyshevBoundedField(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.image");
    declareChebyshevBoundedField(wrappers);
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
