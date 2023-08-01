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
#include <pybind11/stl.h>
#include <lsst/utils/python.h>

#include <memory>

#include "ndarray/pybind11.h"

#include "lsst/utils/python.h"

#include "lsst/afw/table/io/Persistable.h"
#include "lsst/geom/Point.h"
#include "lsst/afw/math/BoundedField.h"
#include "lsst/afw/table/io/python.h"  // for addPersistableMethods

namespace py = pybind11;

using namespace lsst::afw::math;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace math {
namespace {

using PyClass = py::class_<BoundedField>;

template <typename PixelT>
void declareTemplates(PyClass &cls) {
    cls.def("fillImage", &BoundedField::fillImage<PixelT>, "image"_a, "overlapOnly"_a = false, "xStep"_a = 1,
            "yStep"_a = 1);
    cls.def("addToImage", &BoundedField::addToImage<PixelT>, "image"_a, "scaleBy"_a = 1.0,
            "overlapOnly"_a = false, "xStep"_a = 1, "yStep"_a = 1);
    cls.def("multiplyImage", &BoundedField::multiplyImage<PixelT>, "image"_a, "overlapOnly"_a = false,
            "xStep"_a = 1, "yStep"_a = 1);
    cls.def("divideImage", &BoundedField::divideImage<PixelT>, "image"_a, "overlapOnly"_a = false,
            "xStep"_a = 1, "yStep"_a = 1);
}
}  // namespace
void declareBoundedField(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyClass(wrappers.module, "BoundedField"), [](auto &mod, auto &cls) {
        table::io::python::addPersistableMethods<BoundedField>(cls);

        cls.def(
                "__rmul__", [](BoundedField &bf, double const scale) { return bf * scale; },
                py::is_operator());
        cls.def("__mul__", &BoundedField::operator*, py::is_operator());
        cls.def("__truediv__", &BoundedField::operator/, py::is_operator());
        cls.def("__eq__", &BoundedField::operator==, py::is_operator());
        cls.def("__ne__", &BoundedField::operator!=, py::is_operator());

        cls.def("evaluate", (double (BoundedField::*)(double, double) const) & BoundedField::evaluate);
        cls.def("evaluate", (ndarray::Array<double, 1, 1>(BoundedField::*)(
                                    ndarray::Array<double const, 1> const &,
                                    ndarray::Array<double const, 1> const &) const) &
                                    BoundedField::evaluate);
        cls.def("evaluate",
                (double (BoundedField::*)(lsst::geom::Point2D const &) const) & BoundedField::evaluate);
        cls.def("integrate", &BoundedField::integrate);
        cls.def("mean", &BoundedField::mean);
        cls.def("getBBox", &BoundedField::getBBox);

        // Pybind11 resolves overloads by picking the first one that might work
        declareTemplates<double>(cls);
        declareTemplates<float>(cls);

        utils::python::addOutputOp(cls, "__str__");
        cls.def("__repr__", [](BoundedField const &self) {
            std::ostringstream os;
            os << "BoundedField(" << self << ")";
            return os.str();
        });
    });
}
void wrapBoundedField(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.table.io");
    declareBoundedField(wrappers);
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
