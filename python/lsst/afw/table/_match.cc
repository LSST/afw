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
#include "pybind11/stl.h"

#include "lsst/utils/python.h"

#include "lsst/pex/config/python.h"  // for LSST_DECLARE_CONTROL_FIELD
#include "lsst/afw/table/Simple.h"
#include "lsst/afw/table/Source.h"
#include "lsst/afw/table/Match.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace table {

using utils::python::WrapperCollection;

namespace {

/// @internal Declare match code templated on two types of catalog
template <typename Catalog1, typename Catalog2>
void declareMatch2(WrapperCollection &wrappers, std::string const &prefix) {
    using Record1 = typename Catalog1::Record;
    using Record2 = typename Catalog2::Record;
    using MatchList = std::vector<Match<typename Catalog1::Record, typename Catalog2::Record>>;

    using Class = Match<Record1, Record2>;
    using PyClass = py::class_<Class>;
    wrappers.wrapType(PyClass(wrappers.module, (prefix + "Match").c_str()), [](auto &mod, auto &cls) {
        cls.def(py::init<>());
        cls.def(py::init<std::shared_ptr<Record1> const &, std::shared_ptr<Record2> const &, double>(),
                "first"_a, "second"_a, "distance"_a);

        // struct fields
        cls.def_readwrite("first", &Match<Record1, Record2>::first);
        cls.def_readwrite("second", &Match<Record1, Record2>::second);
        cls.def_readwrite("distance", &Match<Record1, Record2>::distance);
    });

    // Free Functions
    wrappers.wrap([](auto &mod) {
        mod.def("unpackMatches", &unpackMatches<Catalog1, Catalog2>, "matches"_a, "cat1"_a, "cat2"_a);

        mod.def("matchRaDec",
                (MatchList(*)(Catalog1 const &, Catalog2 const &, lsst::geom::Angle,
                              MatchControl const &))matchRaDec<Catalog1, Catalog2>,
                "cat1"_a, "cat2"_a, "radius"_a, "mc"_a = MatchControl());
    });
};

/// @internal Declare match code templated on one type of catalog
template <typename Catalog>
void declareMatch1(WrapperCollection &wrappers) {
    using MatchList = std::vector<Match<typename Catalog::Record, typename Catalog::Record>>;
    wrappers.wrap([](auto &mod) {
        mod.def("matchRaDec",
                (MatchList(*)(Catalog const &, lsst::geom::Angle, MatchControl const &))matchRaDec<Catalog>,
                "cat"_a, "radius"_a, "mc"_a = MatchControl());
    });
}

}  // namespace

void wrapMatch(WrapperCollection &wrappers) {
    wrappers.wrapType(py::class_<MatchControl>(wrappers.module, "MatchControl"), [](auto &mod, auto &cls) {
        cls.def(py::init<>());
        LSST_DECLARE_CONTROL_FIELD(cls, MatchControl, findOnlyClosest);
        LSST_DECLARE_CONTROL_FIELD(cls, MatchControl, symmetricMatch);
        LSST_DECLARE_CONTROL_FIELD(cls, MatchControl, includeMismatches);
    });

    declareMatch2<SimpleCatalog, SimpleCatalog>(wrappers, "Simple");
    declareMatch2<SimpleCatalog, SourceCatalog>(wrappers, "Reference");
    declareMatch2<SourceCatalog, SourceCatalog>(wrappers, "Source");
    declareMatch1<SimpleCatalog>(wrappers);
    declareMatch1<SourceCatalog>(wrappers);

    wrappers.wrap([](auto &mod) {
        mod.def("matchXy",
                (SourceMatchVector(*)(SourceCatalog const &, SourceCatalog const &, double,
                                      MatchControl const &))matchXy,
                "cat1"_a, "cat2"_a, "radius"_a, "mc"_a = MatchControl());
        mod.def("matchXy", (SourceMatchVector(*)(SourceCatalog const &, double, MatchControl const &))matchXy,
                "cat"_a, "radius"_a, "mc"_a = MatchControl());
    });
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
