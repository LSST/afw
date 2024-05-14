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
#include "nanobind/eigen/dense.h"

#include <memory>

#include "astshim.h"
#include "nanobind/stl/vector.h"
#include "ndarray/nanobind.h"

#include "lsst/afw/table/io/python.h"
#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/geom/Endpoint.h"
#include "lsst/afw/geom/Transform.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace {

// Return a string consisting of "_pythonClassName_[_fromNAxes_->_toNAxes_]",
// for example "TransformGenericToPoint2[4->2]"
template <class Class>
std::string formatStr(Class const &self, std::string const &pyClassName) {
    std::ostringstream os;
    os << pyClassName;
    os << "[" << self.getFromEndpoint().getNAxes() << "->" << self.getToEndpoint().getNAxes() << "]";
    return os.str();
}

template <class FromEndpoint, class ToEndpoint, class NextToEndpoint, class PyClass>
void declareMethodTemplates(PyClass &cls) {
    using ThisTransform = Transform<FromEndpoint, ToEndpoint>;
    using NextTransform = Transform<ToEndpoint, NextToEndpoint>;
    using SeriesTransform = Transform<FromEndpoint, NextToEndpoint>;
    // Need Python-specific logic to give sensible errors for mismatched Transform types
    cls.def("_then",
            (std::shared_ptr<SeriesTransform>(ThisTransform::*)(NextTransform const &, bool) const) &
                    ThisTransform::template then<NextToEndpoint>,
            "next"_a, "simplify"_a = true);
}

// Declare Transform<FromEndpoint, ToEndpoint> using python class name Transform<X>To<Y>
// where <X> and <Y> are the prefix of the from endpoint and to endpoint class, respectively,
// for example TransformGenericToPoint2
template <class FromEndpoint, class ToEndpoint>
void declareTransform(lsst::cpputils::python::WrapperCollection &wrappers) {
    using Class = Transform<FromEndpoint, ToEndpoint>;
    using ToPoint = typename ToEndpoint::Point;
    using ToArray = typename ToEndpoint::Array;
    using FromPoint = typename FromEndpoint::Point;
    using FromArray = typename FromEndpoint::Array;

    std::string const pyClassName = Class::getShortClassName();
    wrappers.wrapType(
            nb::class_<Class, table::io::Persistable>(wrappers.module, pyClassName.c_str()),
            [](auto &mod, auto &cls) {
                std::string const pyClassName = Class::getShortClassName();
                cls.def(nb::init<ast::FrameSet const &, bool>(), "frameSet"_a, "simplify"_a = true);
                cls.def(nb::init<ast::Mapping const &, bool>(), "mapping"_a, "simplify"_a = true);

                cls.def_prop_ro("hasForward", &Class::hasForward);
                cls.def_prop_ro("hasInverse", &Class::hasInverse);
                cls.def_prop_ro("fromEndpoint", &Class::getFromEndpoint);
                cls.def_prop_ro("toEndpoint", &Class::getToEndpoint);

                // Return a copy of the contained Mapping in order to assure changing the returned Mapping
                // will not affect the contained Mapping (since Python ignores constness)
                cls.def("getMapping", [](Class const &self) { return self.getMapping()->copy(); });

                cls.def("applyForward",
                        nb::overload_cast<FromArray const &>(&Class::applyForward, nb::const_), "array"_a);
                cls.def("applyForward",
                        nb::overload_cast<FromPoint const &>(&Class::applyForward, nb::const_), "point"_a);
                cls.def("applyInverse", nb::overload_cast<ToArray const &>(&Class::applyInverse, nb::const_),
                        "array"_a);
                cls.def("applyInverse", nb::overload_cast<ToPoint const &>(&Class::applyInverse, nb::const_),
                        "point"_a);
                cls.def("inverted", &Class::inverted);
                /* Need some extra handling of ndarray return type in Python to prevent dimensions
                 * of length 1 from being deleted */
                cls.def("_getJacobian", &Class::getJacobian);
                // Do not wrap getShortClassName because it returns the name of the class;
                // use `<class>.__name__` or `type(<instance>).__name__` instead.
                // Do not wrap readStream or writeStream because C++ streams are not easy to wrap.
                cls.def_static("readString", &Class::readString);
                cls.def("writeString", &Class::writeString);

                declareMethodTemplates<FromEndpoint, ToEndpoint, GenericEndpoint>(cls);
                declareMethodTemplates<FromEndpoint, ToEndpoint, Point2Endpoint>(cls);
                declareMethodTemplates<FromEndpoint, ToEndpoint, SpherePointEndpoint>(cls);

                // str(self) = "<Python class name>[<nIn>-><nOut>]"
                cls.def("__str__", [pyClassName](Class const &self) { return formatStr(self, pyClassName); });
                // repr(self) = "lsst.afw.geom.<Python class name>[<nIn>-><nOut>]"
                cls.def("__repr__", [pyClassName](Class const &self) {
                    return "lsst.afw.geom." + formatStr(self, pyClassName);
                });

                table::io::python::addPersistableMethods<Class>(cls);
            });
}
}  // namespace
void wrapTransform(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.table.io");
    wrappers.addSignatureDependency("astshim");
    declareTransform<GenericEndpoint, GenericEndpoint>(wrappers);
    declareTransform<GenericEndpoint, Point2Endpoint>(wrappers);
    declareTransform<GenericEndpoint, SpherePointEndpoint>(wrappers);
    declareTransform<Point2Endpoint, GenericEndpoint>(wrappers);
    declareTransform<Point2Endpoint, Point2Endpoint>(wrappers);
    declareTransform<Point2Endpoint, SpherePointEndpoint>(wrappers);
    declareTransform<SpherePointEndpoint, GenericEndpoint>(wrappers);
    declareTransform<SpherePointEndpoint, Point2Endpoint>(wrappers);
    declareTransform<SpherePointEndpoint, SpherePointEndpoint>(wrappers);
}
}  // namespace geom
}  // namespace afw
}  // namespace lsst
