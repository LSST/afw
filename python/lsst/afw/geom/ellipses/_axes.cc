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

#include "lsst/afw/geom/ellipses/Axes.h"

namespace nb = nanobind;

using namespace nb::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace ellipses {
void wrapAxes(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(
            nb::class_<Axes, BaseCore>(wrappers.module, "Axes"),
            [](auto &mod, auto &cls) {
                /* Constructors */
                cls.def(nb::init<double, double, double, bool>(), "a"_a = 1.0, "b"_a = 1.0, "theta"_a = 0.0,
                        "normalize"_a = false);
                cls.def(nb::init<Axes const &>());
                cls.def(nb::init<BaseCore const &>());

                /* Operators */
                cls.def(
                        "__eq__", [](Axes &self, Axes &other) { return self == other; }, nb::is_operator());
                cls.def(
                        "__ne__", [](Axes &self, Axes &other) { return self != other; }, nb::is_operator());

                /* Members */
                cls.def("getA", &Axes::getA);
                cls.def("setA", &Axes::setA);
                cls.def("getB", &Axes::getB);
                cls.def("setB", &Axes::setB);
                cls.def("getTheta", &Axes::getTheta);
                cls.def("setTheta", &Axes::setTheta);
                cls.def("clone", &Axes::clone);
                cls.def("getName", &Axes::getName);
                cls.def("normalize", &Axes::normalize);
                cls.def("readParameters", &Axes::readParameters);
                cls.def("writeParameters", &Axes::writeParameters);
                cls.def("assign", [](Axes &self, Axes &other) { self = other; });
                cls.def("assign", [](Axes &self, BaseCore &other) { self = other; });
                cls.def("transform", [](Axes &self, lsst::geom::LinearTransform const &t) {
                    return std::static_pointer_cast<Axes>(self.transform(t).copy());
                });
                cls.def("transformInPlace", [](Axes &self, lsst::geom::LinearTransform const &t) {
                    self.transform(t).inPlace();
                });
            });
}
}  // namespace ellipses
}  // namespace geom
}  // namespace afw
}  // namespace lsst
