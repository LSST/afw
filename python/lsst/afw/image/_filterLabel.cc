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

#include "pybind11/pybind11.h"

#include <exception>

#include "lsst/utils/python.h"

#include "lsst/afw/image/FilterLabel.h"
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/table/io/python.h"

using namespace std::string_literals;

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace image {
namespace {

using PyFilterLabel = py::class_<FilterLabel, typehandling::Storable>;

// Macro to convert an exception without defining a global handler for it
#define _DELEGATE_EXCEPTION(call, cpp_ex, py_ex) \
    try {                                        \
        return call;                             \
    } catch (cpp_ex const &e) {                  \
        std::throw_with_nested(py_ex(e.what())); \
    }

void declareFilterLabel(lsst::utils::python::WrapperCollection &wrappers) {
    // Include Python-only constructor in class doc, since it's what people should use
    auto initDoc = R"delim(
        Attributes
        ----------
        band : str, optional
            The band associated with this label.
        physical : str, optional
            The physical filter associated with this label.
    )delim";
    wrappers.wrapType(PyFilterLabel(wrappers.module, "FilterLabel", initDoc), [](auto &mod, auto &cls) {
        table::io::python::addPersistableMethods(cls);

        cls.def_static("fromBandPhysical", &FilterLabel::fromBandPhysical, "band"_a, "physical"_a);
        cls.def_static("fromBand", &FilterLabel::fromBand, "band"_a);
        cls.def_static("fromPhysical", &FilterLabel::fromPhysical, "physical"_a);

        // Keyword constructor
        /* This is messy in C++, but it's hard to write a Python __init__ that delegates to a factory,
         * and the pybind11 docs imply that this way is less prone to multiple-definition errors.
         * In C++17, we should be able to replace py::object with std::optional<string>.
         */
        cls.def(py::init([](py::object band, py::object physical) {
                    try {
                        // Expand as we get more combinations of keywords
                        if (!band.is_none() && !physical.is_none()) {
                            return FilterLabel::fromBandPhysical(py::cast<std::string>(band),
                                                                 py::cast<std::string>(physical));
                        } else if (!band.is_none()) {
                            return FilterLabel::fromBand(py::cast<std::string>(band));
                        } else if (!physical.is_none()) {
                            return FilterLabel::fromPhysical(py::cast<std::string>(physical));
                        } else {
                            throw py::value_error("Need at least one of band, physical");
                        }
                    } catch (py::cast_error const &e) {
                        // By default cast_error is wrapped as RuntimeError
                        std::throw_with_nested(py::type_error(e.what()));
                    }
                }),
                // TODO: use py::kw_only() in pybind11 2.6 or later (DM-27247)
                "band"_a = py::none(), "physical"_a = py::none());

        cls.def("hasBandLabel", &FilterLabel::hasBandLabel);
        cls.def_property_readonly("bandLabel", [](FilterLabel const &label) {
            _DELEGATE_EXCEPTION(label.getBandLabel(), pex::exceptions::LogicError, std::runtime_error);
        });
        cls.def("hasPhysicalLabel", &FilterLabel::hasPhysicalLabel);
        cls.def_property_readonly("physicalLabel", [](FilterLabel const &label) {
            _DELEGATE_EXCEPTION(label.getPhysicalLabel(), pex::exceptions::LogicError, std::runtime_error);
        });
        cls.def("__eq__", &FilterLabel::operator==, py::is_operator());
        utils::python::addHash(cls);
        cls.def("__ne__", &FilterLabel::operator!=, py::is_operator());

        cls.def("__repr__", &FilterLabel::toString);
        // Neither __copy__ nor __deepcopy__ default to each other
        cls.def("__copy__", [](const FilterLabel &obj) { return obj.cloneStorable(); });
        cls.def("__deepcopy__", [](const FilterLabel &obj, py::dict &memo) { return obj.cloneStorable(); });
    });
}
}  // namespace
void wrapFilterLabel(lsst::utils::python::WrapperCollection &wrappers) {
    // import inheritance dependencies
    wrappers.addInheritanceDependency("lsst.afw.typehandling");
    declareFilterLabel(wrappers);
    wrappers.wrap(
            [](auto &mod) { mod.def("getDatabaseFilterLabel", &getDatabaseFilterLabel, "filterLabel"_a); });
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
