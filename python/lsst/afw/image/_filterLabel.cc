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

#include <exception>

#include "lsst/cpputils/python.h"

#include "lsst/afw/image/FilterLabel.h"
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/table/io/python.h"

using namespace std::string_literals;

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace image {
namespace {

using PyFilterLabel = nb::class_<FilterLabel, typehandling::Storable>;

// Macro to convert an exception without defining a global handler for it
#define _DELEGATE_EXCEPTION(call, cpp_ex, py_ex) \
    try {                                        \
        return call;                             \
    } catch (cpp_ex const &e) {                  \
        std::throw_with_nested(py_ex(e.what())); \
    }

void declareFilterLabel(lsst::cpputils::python::WrapperCollection &wrappers) {
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

        cls.def_static("fromBandPhysical", nb::overload_cast<std::string const &, std::string const &>(&FilterLabel::fromBandPhysical), "band"_a, "physical"_a);
        cls.def_static("fromBand", nb::overload_cast<std::string const &>(&FilterLabel::fromBand), "band"_a);
        cls.def_static("fromPhysical", nb::overload_cast<std::string const &>(&FilterLabel::fromPhysical), "physical"_a);

        // Keyword constructor
        /* This is messy in C++, but it's hard to write a Python __init__ that delegates to a factory,
         * and the nanobind docs imply that this way is less prone to multiple-definition errors.
         * In C++17, we should be able to replace nb::object with std::optional<string>.
         */
        cls.def("__init__", [](FilterLabel * filterLabel, nb::object band, nb::object physical) {
                    try {
                        // Expand as we get more combinations of keywords
                        if (!band.is_none() && !physical.is_none()) {
                            FilterLabel::fromBandPhysical(filterLabel, nb::cast<std::string>(band),
                                                                 nb::cast<std::string>(physical));
                        } else if (!band.is_none()) {
                            FilterLabel::fromBand(filterLabel, nb::cast<std::string>(band));
                        } else if (!physical.is_none()) {
                            FilterLabel::fromPhysical(filterLabel, nb::cast<std::string>(physical));
                        } else {
                            throw nb::value_error("Need at least one of band, physical");
                        }
                    } catch (nb::cast_error const &e) {
                        // By default cast_error is wrapped as RuntimeError
                        std::throw_with_nested(nb::type_error(e.what()));
                    }
                },
                // TODO: use nb::kw_only() in nanobind 2.6 or later (DM-27247)
                "band"_a = nb::none(), "physical"_a = nb::none());

        cls.def("hasBandLabel", &FilterLabel::hasBandLabel);
        cls.def_prop_ro("bandLabel", [](FilterLabel const &label) {
            _DELEGATE_EXCEPTION(label.getBandLabel(), pex::exceptions::LogicError, std::runtime_error);
        });
        cls.def("hasPhysicalLabel", &FilterLabel::hasPhysicalLabel);
        cls.def_prop_ro("physicalLabel", [](FilterLabel const &label) {
            _DELEGATE_EXCEPTION(label.getPhysicalLabel(), pex::exceptions::LogicError, std::runtime_error);
        });
        cls.def("__eq__", &FilterLabel::operator==, nb::is_operator());
        cpputils::python::addHash(cls);
        cls.def("__ne__", &FilterLabel::operator!=, nb::is_operator());

        cls.def("__repr__", &FilterLabel::toString);
        // Neither __copy__ nor __deepcopy__ default to each other
        cls.def("__copy__", [](const FilterLabel &obj) { return obj.cloneStorable(); });
        cls.def("__deepcopy__", [](const FilterLabel &obj, nb::dict &memo) { return obj.cloneStorable(); });
    });
}
}  // namespace
void wrapFilterLabel(lsst::cpputils::python::WrapperCollection &wrappers) {
    // import inheritance dependencies
    wrappers.addInheritanceDependency("lsst.afw.typehandling");
    declareFilterLabel(wrappers);
    wrappers.wrap(
            [](auto &mod) { mod.def("getDatabaseFilterLabel", &getDatabaseFilterLabel, "filterLabel"_a); });
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
