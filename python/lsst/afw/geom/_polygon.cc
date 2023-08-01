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

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <lsst/utils/python.h>

#include <pybind11/stl.h>

#include "lsst/pex/exceptions/Runtime.h"
#include "lsst/pex/exceptions/python/Exception.h"
#include "lsst/geom/Box.h"
#include "lsst/geom/Point.h"
#include "lsst/geom/AffineTransform.h"
#include "lsst/afw/geom/Transform.h"
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/geom/polygon/Polygon.h"
#include "lsst/afw/table/io/python.h"  // for addPersistableMethods

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace polygon {
namespace {
void declarePolygon(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(
            py::class_<Polygon, typehandling::Storable>(wrappers.module, "Polygon"),
            [](auto &mod, auto &cls) {
                /* Constructors */
                cls.def(py::init<Polygon::Box const &>());
                cls.def(py::init<Polygon::Box const &, TransformPoint2ToPoint2 const &>());
                cls.def(py::init<Polygon::Box const &, lsst::geom::AffineTransform const &>());
                cls.def(py::init<std::vector<Polygon::Point> const &>());

                table::io::python::addPersistableMethods<Polygon>(cls);

                /* Operators */
                cls.def(
                        "__eq__", [](Polygon const &self, Polygon const &other) { return self == other; },
                        py::is_operator());
                cls.def(
                        "__ne__", [](Polygon const &self, Polygon const &other) { return self != other; },
                        py::is_operator());

                /* Members */
                cls.def("getNumEdges", &Polygon::getNumEdges);
                cls.def("getBBox", &Polygon::getBBox);
                cls.def("calculateCenter", &Polygon::calculateCenter);
                cls.def("calculateArea", &Polygon::calculateArea);
                cls.def("calculatePerimeter", &Polygon::calculatePerimeter);
                cls.def("getVertices", &Polygon::getVertices);
                cls.def("getEdges", &Polygon::getEdges);

                cls.def("contains", (bool (Polygon::*)(Polygon::Point const&) const) &Polygon::contains);
                cls.def("contains", (std::vector<bool> (Polygon::*)(std::vector<Polygon::Point> const&) const) &Polygon::contains);
                cls.def("contains", (std::vector<bool> (Polygon::*)(std::vector<lsst::geom::Point2I> const&) const) &Polygon::contains);
                cls.def("contains", py::vectorize((bool (Polygon::*)(double x, double y) const) &Polygon::contains<double, double>));
                cls.def("contains", py::vectorize((bool (Polygon::*)(float x, float y) const) &Polygon::contains<float, float>));
                cls.def("contains", py::vectorize((bool (Polygon::*)(int x, int y) const) &Polygon::contains<int, int>));

                cls.def("overlaps", (bool (Polygon::*)(Polygon const &) const) & Polygon::overlaps);
                cls.def("overlaps", (bool (Polygon::*)(Polygon::Box const &) const) & Polygon::overlaps);
                cls.def("intersectionSingle", (std::shared_ptr<Polygon>(Polygon::*)(Polygon const &) const) &
                                                      Polygon::intersectionSingle);
                cls.def("intersectionSingle",
                        (std::shared_ptr<Polygon>(Polygon::*)(Polygon::Box const &) const) &
                                Polygon::intersectionSingle);
                cls.def("intersection",
                        (std::vector<std::shared_ptr<Polygon>>(Polygon::*)(Polygon const &) const) &
                                Polygon::intersection);
                cls.def("intersection",
                        (std::vector<std::shared_ptr<Polygon>>(Polygon::*)(Polygon::Box const &) const) &
                                Polygon::intersection);
                cls.def("unionSingle",
                        (std::shared_ptr<Polygon>(Polygon::*)(Polygon const &) const) & Polygon::unionSingle);
                cls.def("unionSingle", (std::shared_ptr<Polygon>(Polygon::*)(Polygon::Box const &) const) &
                                               Polygon::unionSingle);

                // Wrap Polygon::union_ (C++) as Polygon.union (Python)
                cls.def("union", (std::vector<std::shared_ptr<Polygon>>(Polygon::*)(Polygon const &) const) &
                                         Polygon::union_);
                cls.def("union",
                        (std::vector<std::shared_ptr<Polygon>>(Polygon::*)(Polygon::Box const &) const) &
                                Polygon::union_);
                cls.def("symDifference",
                        (std::vector<std::shared_ptr<Polygon>>(Polygon::*)(Polygon const &) const) &
                                Polygon::symDifference);
                cls.def("symDifference",
                        (std::vector<std::shared_ptr<Polygon>>(Polygon::*)(Polygon::Box const &) const) &
                                Polygon::symDifference);
                // cls.def("simplify", &Polygon::simplify);
                cls.def("convexHull", &Polygon::convexHull);
                cls.def("transform",
                        (std::shared_ptr<Polygon>(Polygon::*)(TransformPoint2ToPoint2 const &) const) &
                                Polygon::transform);
                cls.def("transform",
                        (std::shared_ptr<Polygon>(Polygon::*)(lsst::geom::AffineTransform const &) const) &
                                Polygon::transform);
                cls.def("subSample",
                        (std::shared_ptr<Polygon>(Polygon::*)(size_t) const) & Polygon::subSample);
                cls.def("subSample",
                        (std::shared_ptr<Polygon>(Polygon::*)(double) const) & Polygon::subSample);
                cls.def("createImage", (std::shared_ptr<afw::image::Image<float>>(Polygon::*)(
                                               lsst::geom::Box2I const &) const) &
                                               Polygon::createImage);
                cls.def("createImage", (std::shared_ptr<afw::image::Image<float>>(Polygon::*)(
                                               lsst::geom::Extent2I const &) const) &
                                               Polygon::createImage);
            });
}
}  // namespace
void wrapPolygon(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.pex.exceptions");
    wrappers.addInheritanceDependency("lsst.afw.typehandling");
    wrappers.addSignatureDependency("lsst.afw.table.io");
    wrappers.wrapException<SinglePolygonException, pex::exceptions::RuntimeError>("SinglePolygonException",
                                                                                  "RuntimeError");
    declarePolygon(wrappers);
}
}  // namespace polygon
}  // namespace geom
}  // namespace afw
}  // namespace lsst
