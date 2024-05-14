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
#include "nanobind/eigen/dense.h"

#include "lsst/cpputils/python.h"

#include "lsst/afw/table/slots.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace table {

using cpputils::python::WrapperCollection;

namespace {

// tiny classes only used in afw::table: no need for shared_ptr.
using PySlotDefinition = nb::class_<SlotDefinition>;

void declareSlotDefinition(WrapperCollection &wrappers) {
    wrappers.wrapType(PySlotDefinition(wrappers.module, "SlotDefinition"), [](auto &mod, auto &cls) {
        cls.def("getName", &SlotDefinition::getName);
        cls.def("getAlias", &SlotDefinition::getAlias);
    });
}

/*
Declare standard methods for subclasses of SlotDefinition (but not SlotDefinition itself).
*/
template <typename Class>
void declareSlotDefinitionSubclass(WrapperCollection &wrappers, std::string const &name) {
    wrappers.wrapType(nb::class_<Class, SlotDefinition>(wrappers.module, name.c_str()),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<std::string const &>(), "name"_a);
                          cls.def("isValid", &Class::isValid);
                          cls.def("getMeasKey", &Class::getMeasKey);
                          cls.def("getErrKey", &Class::getErrKey);
                          cls.def("getFlagKey", &Class::getFlagKey);
                          cls.def("setKeys", &Class::setKeys, "alias"_a, "schema"_a);
                      });
}

}  // namespace

void wrapSlots(WrapperCollection &wrappers) {
    // TODO: uncomment once afw.geom uses WrapperCollection
    // wrappers.addSignatureDependency("lsst.afw.geom.ellipses");

    declareSlotDefinition(wrappers);
    declareSlotDefinitionSubclass<FluxSlotDefinition>(wrappers, "Flux");
    declareSlotDefinitionSubclass<CentroidSlotDefinition>(wrappers, "Centroid");
    declareSlotDefinitionSubclass<ShapeSlotDefinition>(wrappers, "Shape");
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
