/*
 * LSST Data Management System
 * Copyright 2008-2016  AURA/LSST.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
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
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */

#include <memory>
#include <vector>

#include <nanobind/nanobind.h>
#include <lsst/cpputils/python.h>
#include <nanobind/stl/vector.h>

#include "lsst/geom/Box.h"
#include "lsst/afw/image.h"
#include "lsst/afw/math/SpatialCell.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace math {

namespace {

using CandidateList = std::vector<std::shared_ptr<SpatialCellCandidate>>;

// Wrap SpatialCellCandidate (an abstract class so no constructor is wrapped)
void declareSpatialCellCandidate(lsst::cpputils::python::WrapperCollection &wrappers) {
    using PyClass = nb::class_<SpatialCellCandidate>;
    auto clsSpatialCellCandidate =
            wrappers.wrapType(PyClass(wrappers.module, "SpatialCellCandidate"), [](auto &mod, auto &cls) {
                cls.def("getXCenter", &SpatialCellCandidate::getXCenter);
                cls.def("getYCenter", &SpatialCellCandidate::getYCenter);
                cls.def("instantiate", &SpatialCellCandidate::instantiate);
                cls.def("getCandidateRating", &SpatialCellCandidate::getCandidateRating);
                cls.def("setCandidateRating", &SpatialCellCandidate::setCandidateRating);
                cls.def("getId", &SpatialCellCandidate::getId);
                cls.def("getStatus", &SpatialCellCandidate::getStatus);
                cls.def("setStatus", &SpatialCellCandidate::setStatus);
                cls.def("isBad", &SpatialCellCandidate::isBad);
            });
    wrappers.wrapType(nb::enum_<SpatialCellCandidate::Status>(clsSpatialCellCandidate, "Status"),
                      [](auto &mod, auto &enm) {
                          enm.value("BAD", SpatialCellCandidate::Status::BAD);
                          enm.value("GOOD", SpatialCellCandidate::Status::GOOD);
                          enm.value("UNKNOWN", SpatialCellCandidate::Status::UNKNOWN);
                          enm.export_values();
                      });
}

// Wrap SpatialCellCandidateIterator
void declareSpatialCellCandidateIterator(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(
            nb::class_<SpatialCellCandidateIterator>(wrappers.module, "SpatialCellCandidateIterator"),
            [](auto &mod, auto &cls) {
                cls.def("__incr__", &SpatialCellCandidateIterator::operator++, nb::is_operator());
                cls.def(
                        "__deref__",
                        [](SpatialCellCandidateIterator &it) -> std::shared_ptr<SpatialCellCandidate> {
                            return *it;
                        },
                        nb::is_operator());
                cls.def("__eq__", &SpatialCellCandidateIterator::operator==, nb::is_operator());
                cls.def("__ne__", &SpatialCellCandidateIterator::operator!=, nb::is_operator());
                cls.def("__sub__", &SpatialCellCandidateIterator::operator-, nb::is_operator());
            });
}

// Wrap SpatialCell
void declareSpatialCell(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(
            nb::class_<SpatialCell>(wrappers.module, "SpatialCell"),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<std::string const &, lsst::geom::Box2I const &, CandidateList const &>(),
                        "label"_a, "bbox"_a = lsst::geom::Box2I(), "candidateList"_a = CandidateList());

                cls.def("empty", &SpatialCell::empty);
                cls.def("size", &SpatialCell::size);
                cls.def("__len__", &SpatialCell::size);
                cls.def("getLabel", &SpatialCell::getLabel);
                cls.def("begin", (SpatialCellCandidateIterator(SpatialCell::*)()) & SpatialCell::begin);
                cls.def("begin", (SpatialCellCandidateIterator(SpatialCell::*)(bool)) & SpatialCell::begin);
                cls.def("end", (SpatialCellCandidateIterator(SpatialCell::*)()) & SpatialCell::end);
                cls.def("end", (SpatialCellCandidateIterator(SpatialCell::*)(bool)) & SpatialCell::end);
                cls.def("insertCandidate", &SpatialCell::insertCandidate);
                cls.def("removeCandidate", &SpatialCell::removeCandidate);
                cls.def("setIgnoreBad", &SpatialCell::setIgnoreBad, "ignoreBad"_a);
                cls.def("getIgnoreBad", &SpatialCell::getIgnoreBad);
                cls.def("getCandidateById", &SpatialCell::getCandidateById, "id"_a, "noThrow"_a = false);
                cls.def("getLabel", &SpatialCell::getLabel);
                cls.def("getBBox", &SpatialCell::getBBox);
                cls.def("sortCandidates", &SpatialCell::sortCandidates);
                cls.def("visitCandidates",
                        (void (SpatialCell::*)(CandidateVisitor *, int const, bool const, bool const)) &
                                SpatialCell::visitCandidates,
                        "visitor"_a, "nMaxPerCell"_a = -1, "ignoreExceptions"_a = false, "reset"_a = true);
                cls.def("visitAllCandidates",
                        (void (SpatialCell::*)(CandidateVisitor *, bool const, bool const)) &
                                SpatialCell::visitAllCandidates,
                        "visitor"_a, "ignoreExceptions"_a = false, "reset"_a = true);
            });
}

// Wrap SpatialCellSet
void declareSpatialCellSet(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(
            nb::class_<SpatialCellSet>(wrappers.module, "SpatialCellSet"),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<lsst::geom::Box2I const &, int, int>(), "region"_a, "xSize"_a,
                        "ySize"_a = 0);

                cls.def("getCellList", &SpatialCellSet::getCellList);
                cls.def("getBBox", &SpatialCellSet::getBBox);
                cls.def("insertCandidate", &SpatialCellSet::insertCandidate);
                cls.def("sortCandidates", &SpatialCellSet::sortCandidates);
                cls.def("visitCandidates",
                        (void (SpatialCellSet::*)(CandidateVisitor *, int const, bool const)) &
                                SpatialCellSet::visitCandidates,
                        "visitor"_a, "nMaxPerCell"_a = -1, "ignoreExceptions"_a = false);
                cls.def("visitAllCandidates",
                        (void (SpatialCellSet::*)(CandidateVisitor *, bool const)) &
                                SpatialCellSet::visitAllCandidates,
                        "visitor"_a, "ignoreExceptions"_a = false);
                cls.def("getCandidateById", &SpatialCellSet::getCandidateById, "id"_a, "noThrow"_a = false);
                cls.def("setIgnoreBad", &SpatialCellSet::setIgnoreBad, "ignoreBad"_a);
            });
}

// Wrap CandidateVisitor
void declareCandidateVisitor(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::class_<CandidateVisitor>(wrappers.module,
                                                                                      "CandidateVisitor"),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<>());

                          cls.def("reset", &CandidateVisitor::reset);
                          cls.def("processCandidate", &CandidateVisitor::processCandidate);
                      });
}

// Wrap class SpatialCellImageCandidate (an abstract class, so no constructor is wrapped)
void declareSpatialCellImageCandidate(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::class_<SpatialCellImageCandidate,
                                 SpatialCellCandidate>(wrappers.module, "SpatialCellImageCandidate"),
                      [](auto &mod, auto &cls) {
                          cls.def_static("setWidth", &SpatialCellImageCandidate::setWidth, "width"_a);
                          cls.def_static("getWidth", &SpatialCellImageCandidate::getWidth);
                          cls.def_static("setHeight", &SpatialCellImageCandidate::setHeight, "height"_a);
                          cls.def_static("getHeight", &SpatialCellImageCandidate::getHeight);
                          cls.def("setChi2", &SpatialCellImageCandidate::setChi2, "chi2"_a);
                          cls.def("getChi2", &SpatialCellImageCandidate::getChi2);
                      });
}

void declareTestClasses(lsst::cpputils::python::WrapperCollection &wrappers) {
    /*
     * Test class for SpatialCellCandidate
     */
    class TestCandidate : public SpatialCellCandidate {
    public:
        TestCandidate(float const xCenter,  ///< @internal The object's column-centre
                      float const yCenter,  ///< @internal The object's row-centre
                      float const flux      ///< @internal The object's flux
                      )
                : SpatialCellCandidate(xCenter, yCenter), _flux(flux) {}

        /// @internal Return candidates rating
        double getCandidateRating() const override { return _flux; }
        void setCandidateRating(double flux) override { _flux = flux; }

    private:
        double _flux;
    };

    /// @internal A class to pass around to all our TestCandidates
    class TestCandidateVisitor : public CandidateVisitor {
    public:
        TestCandidateVisitor() : CandidateVisitor() {}

        // Called by SpatialCellSet::visitCandidates before visiting any Candidates
        void reset() override { _n = 0; }

        // Called by SpatialCellSet::visitCandidates for each Candidate
        void processCandidate(SpatialCellCandidate *candidate) override { ++_n; }

        int getN() const { return _n; }

    private:
        int _n{0};  // number of TestCandidates
    };

    class TestImageCandidate : public SpatialCellImageCandidate {
    public:
        using MaskedImageT = image::MaskedImage<float>;

        TestImageCandidate(float const xCenter,  ///< @internal The object's column-centre
                           float const yCenter,  ///< @internal The object's row-centre
                           float const flux      ///< @internal The object's flux
                           )
                : SpatialCellImageCandidate(xCenter, yCenter), _flux(flux) {}

        /// @internal Return candidates rating
        double getCandidateRating() const override { return _flux; }

        /// @internal Return the %image
        std::shared_ptr<MaskedImageT const> getMaskedImage() const {
            if (!_image) {
                _image = std::make_shared<MaskedImageT>(lsst::geom::ExtentI(getWidth(), getHeight()));
                *_image->getImage() = _flux;
            }
            return _image;
        }

    private:
        mutable std::shared_ptr<MaskedImageT> _image;
        double _flux;
    };

    wrappers.wrapType(nb::class_<TestCandidate, SpatialCellCandidate>(
                              wrappers.module, "TestCandidate"),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<float const, float const, float const>());
                          cls.def("getCandidateRating", &TestCandidate::getCandidateRating);
                          cls.def("setCandidateRating", &TestCandidate::setCandidateRating);
                      });
    wrappers.wrapType(
            nb::class_<TestCandidateVisitor, CandidateVisitor>(
                    wrappers.module, "TestCandidateVisitor"),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<>());
                cls.def("getN", &TestCandidateVisitor::getN);
            });
    wrappers.wrapType(
            nb::class_<TestImageCandidate, SpatialCellImageCandidate>(
                    wrappers.module, "TestImageCandidate"),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<float const, float const, float const>(), "xCenter"_a, "yCenter"_a,
                        "flux"_a);
                cls.def("getCandidateRating", &TestImageCandidate::getCandidateRating);
                cls.def("getMaskedImage", &TestImageCandidate::getMaskedImage);
            });
};
}  // namespace
void wrapSpatialCell(lsst::cpputils::python::WrapperCollection &wrappers) {
    declareSpatialCellCandidate(wrappers);
    declareSpatialCellCandidateIterator(wrappers);
    declareSpatialCell(wrappers);
    declareSpatialCellSet(wrappers);
    declareCandidateVisitor(wrappers);
    declareSpatialCellImageCandidate(wrappers);
    /* Test Members */
    declareTestClasses(wrappers);
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
