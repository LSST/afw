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

#include <iostream>
#include <iomanip>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "lsst/utils/python.h"

#include "lsst/afw/detection/Threshold.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace detection {

void wrapThreshold(utils::python::WrapperCollection& wrappers) {
    auto clsThreshold = wrappers.wrapType(
            py::class_<Threshold>(wrappers.module, "Threshold"),
            [](auto& mod, auto& cls) {
                cls.def(py::init<double const, typename Threshold::ThresholdType const, bool const,
                                 double const>(),
                        "value"_a, "type"_a = Threshold::VALUE, "polarity"_a = true,
                        "includeMultiplier"_a = 1.0);

                cls.def("getType", &Threshold::getType);
                cls.def_static("parseTypeString", Threshold::parseTypeString);
                cls.def_static("getTypeString", Threshold::getTypeString);
                cls.def("getValue", (double (Threshold::*)(const double) const) & Threshold::getValue,
                        "param"_a = -1);
                //
                //    template<typename ImageT>
                //    double getValue(ImageT const& image) const;
                //
                cls.def("getPolarity", &Threshold::getPolarity);
                cls.def("setPolarity", &Threshold::setPolarity);
                cls.def("getIncludeMultiplier", &Threshold::getIncludeMultiplier);
                cls.def("setIncludeMultiplier", &Threshold::setIncludeMultiplier);
                // Different str/repr so that the latter can be used to create a Threshold.
                cls.def("__str__", [](Threshold const& self) {
                    std::ostringstream os;
                    std::string polarity = (self.getPolarity() == true) ? "positive" : "negative";
                    os << py::cast(self.getType()).attr("name").cast<std::string>() << std::setprecision(8)
                       << " value=" << self.getValue(1.0) << " (" << polarity << ")";
                    if (self.getIncludeMultiplier() != 1.0) {
                        os << " multiplier=" << self.getIncludeMultiplier();
                    }
                    return os.str();
                });
                cls.def("__repr__", [](Threshold const& self) {
                    std::ostringstream os;
                    os << std::setprecision(16) << "Threshold(value=" << self.getValue(1.0)
                       << ", type=" << py::cast(self.getType()).attr("name").cast<std::string>()
                       << ", polarity=" << self.getPolarity()
                       << ", includeMultiplier=" << self.getIncludeMultiplier() << ")";
                    return os.str();
                });
            });

    wrappers.wrapType(py::enum_<Threshold::ThresholdType>(clsThreshold, "ThresholdType"),
                      [](auto& mod, auto& enm) {
                          enm.value("VALUE", Threshold::ThresholdType::VALUE);
                          enm.value("BITMASK", Threshold::ThresholdType::BITMASK);
                          enm.value("STDEV", Threshold::ThresholdType::STDEV);
                          enm.value("VARIANCE", Threshold::ThresholdType::VARIANCE);
                          enm.value("PIXEL_STDEV", Threshold::ThresholdType::PIXEL_STDEV);
                          enm.export_values();
                      });

    wrappers.wrap([](auto& mod) {
        mod.def("createThreshold", createThreshold, "value"_a, "type"_a = "value", "polarity"_a = true);
    });
}

}  // namespace detection
}  // namespace afw
}  // namespace lsst
