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
#include "nanobind/stl/vector.h"
#include "nanobind/stl/string.h"

#include "lsst/cpputils/python.h"

#include "lsst/afw/table/Field.h"
#include "lsst/afw/table/Key.h"
#include "lsst/afw/table/Schema.h"
#include "lsst/afw/table/detail/SchemaImpl.h"
#include "lsst/afw/table/SchemaMapper.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace table {

using utils::python::WrapperCollection;
using PySchemaMapper = nb::class_<SchemaMapper>;

namespace {

template <typename T>
void declareSchemaMapperOverloads(PySchemaMapper &cls, std::string const &suffix) {
    cls.def("getMapping", (Key<T>(SchemaMapper::*)(Key<T> const &) const) & SchemaMapper::getMapping);
    cls.def("isMapped", (bool (SchemaMapper::*)(Key<T> const &) const) & SchemaMapper::isMapped);
};

}  // namespace

void wrapSchemaMapper(WrapperCollection &wrappers) {
    wrappers.wrapType(PySchemaMapper(wrappers.module, "SchemaMapper"), [](auto &mod, auto &cls) {
        cls.def(nb::init<>());
        cls.def(nb::init<Schema const &, Schema const &>());
        cls.def(nb::init<Schema const &, bool>(), "input"_a, "shareAliasMap"_a = false);

        cls.def("getInputSchema", &SchemaMapper::getInputSchema);
        cls.def("getOutputSchema", &SchemaMapper::getOutputSchema);
        cls.def("editOutputSchema", &SchemaMapper::editOutputSchema,
                nb::rv_policy::reference_internal);
        cls.def("addMinimalSchema", &SchemaMapper::addMinimalSchema, "minimal"_a, "doMap"_a = true);
        cls.def_static("removeMinimalSchema", &SchemaMapper::removeMinimalSchema);
        cls.def_static("join", &SchemaMapper::join, "inputs"_a, "prefixes"_a = std::vector<std::string>());

        declareSchemaMapperOverloads<std::uint8_t>(cls, "B");
        declareSchemaMapperOverloads<std::uint16_t>(cls, "U");
        declareSchemaMapperOverloads<std::int32_t>(cls, "I");
        declareSchemaMapperOverloads<std::int64_t>(cls, "L");
        declareSchemaMapperOverloads<float>(cls, "F");
        declareSchemaMapperOverloads<double>(cls, "D");
        declareSchemaMapperOverloads<std::string>(cls, "String");
        declareSchemaMapperOverloads<lsst::geom::Angle>(cls, "Angle");
        declareSchemaMapperOverloads<lsst::afw::table::Flag>(cls, "Flag");
        declareSchemaMapperOverloads<lsst::afw::table::Array<std::uint8_t>>(cls, "ArrayB");
        declareSchemaMapperOverloads<lsst::afw::table::Array<std::uint16_t>>(cls, "ArrayU");
        declareSchemaMapperOverloads<lsst::afw::table::Array<int>>(cls, "ArrayI");
        declareSchemaMapperOverloads<lsst::afw::table::Array<float>>(cls, "ArrayF");
        declareSchemaMapperOverloads<lsst::afw::table::Array<double>>(cls, "ArrayD");
    });
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
