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
#include <nanobind/stl/vector.h>

#include "lsst/cpputils/python.h"

#include "lsst/afw/detection/FootprintCtrl.h"

namespace nb = nanobind;

using namespace nb::literals;

namespace lsst {
namespace afw {
namespace detection {

void wrapFootprintCtrl(cpputils::python::WrapperCollection& wrappers) {
    wrappers.wrapType(nb::class_<FootprintControl>(wrappers.module, "FootprintControl"),
                      [](auto& mod, auto& cls) {
                          cls.def(nb::init<>());
                          cls.def(nb::init<bool, bool>(), "circular"_a, "isotropic"_a = false);
                          cls.def(nb::init<bool, bool, bool, bool>(), "left"_a, "right"_a, "up"_a, "down"_a);

                          cls.def("growCircular", &FootprintControl::growCircular);
                          cls.def("growIsotropic", &FootprintControl::growIsotropic);
                          cls.def("growLeft", &FootprintControl::growLeft);
                          cls.def("growRight", &FootprintControl::growRight);
                          cls.def("growUp", &FootprintControl::growUp);
                          cls.def("growDown", &FootprintControl::growDown);

                          cls.def("isCircular", &FootprintControl::isCircular);
                          cls.def("isIsotropic", &FootprintControl::isIsotropic);
                          cls.def("isLeft", &FootprintControl::isLeft);
                          cls.def("isRight", &FootprintControl::isRight);
                          cls.def("isUp", &FootprintControl::isUp);
                          cls.def("isDown", &FootprintControl::isDown);
                      });

    auto clsHeavyFootprintCtrl = wrappers.wrapType(
            nb::class_<HeavyFootprintCtrl>(wrappers.module, "HeavyFootprintCtrl"), [](auto& mod, auto& cls) {
                cls.def(nb::init<HeavyFootprintCtrl::ModifySource>(),
                        "modifySource"_a = int(HeavyFootprintCtrl::ModifySource::NONE));

                cls.def("getModifySource", &HeavyFootprintCtrl::getModifySource);
                cls.def("setModifySource", &HeavyFootprintCtrl::setModifySource);
                cls.def("getImageVal", &HeavyFootprintCtrl::getImageVal);
                cls.def("setImageVal", &HeavyFootprintCtrl::setImageVal);
                cls.def("getMaskVal", &HeavyFootprintCtrl::getMaskVal);
                cls.def("setMaskVal", &HeavyFootprintCtrl::setMaskVal);
                cls.def("getVarianceVal", &HeavyFootprintCtrl::getVarianceVal);
                cls.def("setVarianceVal", &HeavyFootprintCtrl::setVarianceVal);
            });

    wrappers.wrapType(nb::enum_<HeavyFootprintCtrl::ModifySource>(clsHeavyFootprintCtrl, "ModifySource"),
                      [](auto& mod, auto& enm) {
                          enm.value("NONE", HeavyFootprintCtrl::ModifySource::NONE);
                          enm.value("SET", HeavyFootprintCtrl::ModifySource::SET);
                          enm.export_values();
                      });
}

}  // namespace detection
}  // namespace afw
}  // namespace lsst
