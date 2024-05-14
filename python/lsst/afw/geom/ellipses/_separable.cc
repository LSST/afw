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

#include <nanobind/nanobind.h>
#include <lsst/cpputils/python.h>

#include "lsst/afw/geom/ellipses/Separable.h"
#include "lsst/afw/geom/ellipses/BaseCore.h"
#include "lsst/afw/geom/ellipses/Distortion.h"
#include "lsst/afw/geom/ellipses/ConformalShear.h"
#include "lsst/afw/geom/ellipses/ReducedShear.h"
#include "lsst/afw/geom/ellipses/radii.h"

namespace nb = nanobind;

using namespace nb::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace ellipses {
template <typename Ellipticity_, typename Radius_>
void declareSeparable(lsst::cpputils::python::WrapperCollection &wrappers, const std::string &suffix) {
    using Class = Separable<Ellipticity_, Radius_>;
    wrappers.wrapType(
            nb::class_<Class, BaseCore>(wrappers.module,
                                                                ("Separable" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<double, double, double, bool>(), "e1"_a = 0.0, "e2"_a = 0.0,
                        "radius"_a = Radius_(), "normalize"_a = true);
                cls.def(nb::init<Class const &>());
                cls.def(nb::init<BaseCore const &>());

                cls.def("getE1", &Class::getE1);
                cls.def("setE1", &Class::setE1);
                cls.def("getE2", &Class::getE2);
                cls.def("setE2", &Class::setE2);
                cls.def("getRadius", (Radius_ const &(Class::*)() const) & Class::getRadius);
                cls.def("setRadius", (void (Class::*)(double)) & Class::setRadius);
                cls.def("setRadius", (void (Class::*)(Radius_ const &)) & Class::setRadius);
                cls.def("getEllipticity", (Ellipticity_ const &(Class::*)() const) & Class::getEllipticity);
                cls.def("clone", &Class::clone);
                cls.def("getName", &Class::getName);
                cls.def("normalize", &Class::normalize);
                cls.def("assign", [](Class &self, Class &other) { self = other; });
                cls.def("assign", [](Class &self, BaseCore &other) { self = other; });
                cls.def("transform", [](Class &self, lsst::geom::LinearTransform const &t) {
                    return std::static_pointer_cast<Class>(self.transform(t).copy());
                });
                cls.def("transformInPlace", [](Class &self, lsst::geom::LinearTransform const &t) {
                    self.transform(t).inPlace();
                });
                cls.def("__str__", [](Class &self) {
                    return nb::str("({}, {})").format(self.getEllipticity(), self.getRadius());
                });
                cls.def("__repr__", [](Class &self) {
                    return nb::str("Separable({}, {})").format(self.getEllipticity(), self.getRadius());
                });
            });
}

void wrapSeparable(lsst::cpputils::python::WrapperCollection &wrappers) {
    declareSeparable<Distortion, DeterminantRadius>(wrappers, "DistortionDeterminantRadius");
    declareSeparable<Distortion, TraceRadius>(wrappers, "DistortionTraceRadius");
    declareSeparable<Distortion, LogDeterminantRadius>(wrappers, "DistortionLogDeterminantRadius");
    declareSeparable<Distortion, LogTraceRadius>(wrappers, "DistortionLogTraceRadius");

    declareSeparable<ConformalShear, DeterminantRadius>(wrappers, "ConformalShearDeterminantRadius");
    declareSeparable<ConformalShear, TraceRadius>(wrappers, "ConformalShearTraceRadius");
    declareSeparable<ConformalShear, LogDeterminantRadius>(wrappers, "ConformalShearLogDeterminantRadius");
    declareSeparable<ConformalShear, LogTraceRadius>(wrappers, "ConformalShearLogTraceRadius");

    declareSeparable<ReducedShear, DeterminantRadius>(wrappers, "ReducedShearDeterminantRadius");
    declareSeparable<ReducedShear, TraceRadius>(wrappers, "ReducedShearTraceRadius");
    declareSeparable<ReducedShear, LogDeterminantRadius>(wrappers, "ReducedShearLogDeterminantRadius");
    declareSeparable<ReducedShear, LogTraceRadius>(wrappers, "ReducedShearLogTraceRadius");
}
}  // namespace ellipses
}  // namespace geom
}  // namespace afw
}  // namespace lsst
