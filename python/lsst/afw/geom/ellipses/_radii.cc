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

#include "lsst/afw/geom/ellipses/radii.h"

namespace nb = nanobind;

using namespace nb::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace ellipses {
void wrapRadii(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(
            nb::class_<DeterminantRadius>(wrappers.module, "DeterminantRadius"), [](auto &mod, auto &cls) {
                cls.def(nb::init<double>(), "value"_a = 1.0);
                cls.def("normalize", &DeterminantRadius::normalize);
                cls.def_static("getName", DeterminantRadius::getName);
                cls.def("__str__", [](DeterminantRadius const &self) { return std::to_string(self); });
                cls.def("__repr__", [](DeterminantRadius const &self) {
                    return self.getName() + "(" + std::to_string(self) + ")";
                });
            });

    wrappers.wrapType(nb::class_<TraceRadius>(wrappers.module, "TraceRadius"), [](auto &mod, auto &cls) {
        cls.def(nb::init<double>(), "value"_a = 1.0);
        cls.def("normalize", &TraceRadius::normalize);
        cls.def_static("getName", TraceRadius::getName);
        cls.def("__str__", [](TraceRadius const &self) { return std::to_string(self); });
        cls.def("__repr__",
                [](TraceRadius const &self) { return self.getName() + "(" + std::to_string(self) + ")"; });
    });

    wrappers.wrapType(nb::class_<LogDeterminantRadius>(wrappers.module, "LogDeterminantRadius"),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<double>(), "value"_a = 0.0);
                          cls.def("normalize", &LogDeterminantRadius::normalize);
                          cls.def_static("getName", LogDeterminantRadius::getName);
                          cls.def("__str__",
                                  [](LogDeterminantRadius const &self) { return std::to_string(self); });
                          cls.def("__repr__", [](LogDeterminantRadius const &self) {
                              return self.getName() + "(" + std::to_string(self) + ")";
                          });
                      });

    wrappers.wrapType(nb::class_<LogTraceRadius>(wrappers.module, "LogTraceRadius"),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<double>(), "value"_a = 0.0);
                          cls.def("normalize", &LogTraceRadius::normalize);
                          cls.def_static("getName", LogTraceRadius::getName);
                          cls.def("__str__", [](LogTraceRadius const &self) { return std::to_string(self); });
                          cls.def("__repr__", [](LogTraceRadius const &self) {
                              return self.getName() + "(" + std::to_string(self) + ")";
                          });
                      });
}
}  // namespace ellipses
}  // namespace geom
}  // namespace afw
}  // namespace lsst
