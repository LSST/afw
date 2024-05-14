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

#include <ostream>
#include <memory>
#include <string>

#include <type_traits>

#include "nanobind/nanobind.h"
#include <lsst/cpputils/python.h>
#include "nanobind/stl/vector.h"
#include "ndarray/nanobind.h"

#include "lsst/geom/Point.h"
#include "lsst/geom/SpherePoint.h"
#include "lsst/afw/geom/Endpoint.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace {

/*
Add `__str__`, `__repr__` and `getClassPrefix` methods to an Endpoint nanobind wrapper

str(self) = "GenericEndpoint(_nAxes_)" for GenericEndpoint, e.g. "GenericEndpoint(4)";
            "_typeName_()" for all other Endpoint classes, e.g. "SpherePointEndpoint()",
repr(self) = "lsst.afw.geom." + str(self), e.g. "lsst.afw.geom.GenericEndpoint(4)"
*/
template <typename PyClass>
void addStrAndRepr(PyClass &cls) {
    using Class = typename PyClass::Type;  // C++ class associated with nanobind wrapper class
    cpputils::python::addOutputOp(cls, "__str__");
    cls.def("__repr__", [](Class const &self) {
        std::ostringstream os;
        os << "lsst.afw.geom." << self;
        return os.str();
    });
    cls.def_static("getClassPrefix", &Class::getClassPrefix);
}

/*
Add getNPoints, dataFromPoint, dataFromArray, pointFromData and arrayFromData
*/
template <typename PyClass>
void addDataConverters(PyClass &cls) {
    using Class = typename PyClass::Type;  // C++ class associated with nanobind wrapper class
    cls.def("getNPoints", &Class::getNPoints);
    cls.def("dataFromPoint", &Class::dataFromPoint);
    cls.def("dataFromArray", &Class::dataFromArray);
    cls.def("arrayFromData", &Class::arrayFromData);
    cls.def("pointFromData", &Class::pointFromData);
}

/*
Add makeFrame method
*/
template <typename PyClass>
void addMakeFrame(PyClass &cls) {
    using Class = typename PyClass::Type;  // C++ class associated with nanobind wrapper class
    // return a deep copy so Python cannot modify the internal state
    cls.def("makeFrame", [](Class const &self) {
        auto frame = self.makeFrame();
        return frame->copy();
    });
}

// Allow Python classes to be compared across different BaseEndpoints
template <typename SelfClass, typename OtherClass, typename PyClass>
std::enable_if_t<std::is_base_of<SelfClass, OtherClass>::value> addEquals(PyClass &cls) {
    cls.def("__eq__", &SelfClass::operator==);
    cls.def("__ne__", &SelfClass::operator!=);
}

template <typename SelfClass, typename OtherClass, typename PyClass>
std::enable_if_t<!std::is_base_of<SelfClass, OtherClass>::value> addEquals(PyClass &cls) {
    cls.def("__eq__", [](SelfClass const &self, OtherClass const &other) { return false; });
    cls.def("__ne__", [](SelfClass const &self, OtherClass const &other) { return true; });
}

template <typename SelfClass, typename PyClass>
void addAllEquals(PyClass &cls) {
    addEquals<SelfClass, GenericEndpoint>(cls);
    addEquals<SelfClass, Point2Endpoint>(cls);
    addEquals<SelfClass, SpherePointEndpoint>(cls);
}

/*
 * Declare BaseVectorEndpoint<Point, Array>;
 * this is meant to be called by other `declare...` functions;
 */
template <typename Point, typename Array>
void declareBaseEndpoint(lsst::cpputils::python::WrapperCollection &wrappers, std::string const &suffix) {
    using Class = BaseEndpoint<Point, Array>;
    std::string const pyClassName = "_BaseEndpoint" + suffix;
    wrappers.wrapType(nb::class_<Class>(wrappers.module, pyClassName.c_str()),
                      [](auto &mod, auto &cls) {
                          cls.def_prop_ro("nAxes", &Class::getNAxes);
                          addDataConverters(cls);
                          addMakeFrame(cls);
                          cls.def("normalizeFrame", &Class::normalizeFrame);
                          addAllEquals<Class>(cls);
                      });
}

// Declare BaseVectorEndpoint and all subclasses (the corresponding BaseEndpoint)
// This is meant to be called by other `declare...` functions;
template <typename Point>
void declareBaseVectorEndpoint(lsst::cpputils::python::WrapperCollection &wrappers, std::string const &suffix) {
    using Class = BaseVectorEndpoint<Point>;
    using Array = typename Class::Array;

    std::string const pyClassName = "_BaseVectorEndpoint" + suffix;

    declareBaseEndpoint<Point, Array>(wrappers, suffix);
    wrappers.wrapType(nb::class_<Class, BaseEndpoint<Point, Array>>(
                              wrappers.module, pyClassName.c_str()),
                      [](auto &mod, auto &cls) { addDataConverters(cls); });
}

// Declare GenericEndpoint and all subclasses
void declareGenericEndpoint(lsst::cpputils::python::WrapperCollection &wrappers) {
    using Class = GenericEndpoint;
    using Point = typename Class::Point;
    using Array = typename Class::Array;

    declareBaseEndpoint<Point, Array>(wrappers, "Generic");

    wrappers.wrapType(nb::class_<Class, BaseEndpoint<Point, Array>>(
                              wrappers.module, "GenericEndpoint"),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<int>(), "nAxes"_a);

                          addStrAndRepr(cls);
                      });
}

/// @internal declare PointNEndpoint (for N = 2 or 3) and all subclasses
void declarePoint2Endpoint(lsst::cpputils::python::WrapperCollection &wrappers) {
    using Class = Point2Endpoint;
    using Point = typename Class::Point;
    std::string const pointNumStr = "Point2";
    std::string const pyClassName = pointNumStr + "Endpoint";

    declareBaseVectorEndpoint<Point>(wrappers, pointNumStr);

    wrappers.wrapType(nb::class_<Class, BaseVectorEndpoint<Point>>(
                              wrappers.module, pyClassName.c_str()),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<>());
                          // do not wrap the constructor that takes nAxes; it is an implementation detail

                          cls.def("normalizeFrame", &Class::normalizeFrame);
                          addStrAndRepr(cls);
                      });
}

/// @internal declare SpherePointEndpoint and all subclasses
void declareSpherePointEndpoint(lsst::cpputils::python::WrapperCollection &wrappers) {
    using Class = SpherePointEndpoint;
    using Point = typename Class::Point;

    declareBaseVectorEndpoint<Point>(wrappers, "SpherePoint");

    wrappers.wrapType(nb::class_<Class, BaseVectorEndpoint<Point>>(
                              wrappers.module, "SpherePointEndpoint"),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<>());
                          // do not wrap the constructor that takes nAxes; it is an implementation detail

                          addMakeFrame(cls);
                          cls.def("normalizeFrame", &Class::normalizeFrame);
                          addStrAndRepr(cls);
                      });
}
}  // namespace
void wrapEndpoint(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.geom");
    declareGenericEndpoint(wrappers);
    declarePoint2Endpoint(wrappers);
    declareSpherePointEndpoint(wrappers);
}

}  // namespace geom
}  // namespace afw
}  // namespace lsst
