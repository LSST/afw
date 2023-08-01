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
#include "pybind11/numpy.h"
#include <lsst/utils/python.h>

#include "lsst/afw/geom/Span.h"
#include "lsst/afw/geom/SpanPixelIterator.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace geom {
namespace {

using PySpan = py::class_<Span>;

// A thin wrapper around SpanPixelIterator.
// Unfortunately we cannot use py::make_iterator here, as we normally
// should, because for some reason the return values then all refer
// to the last element.
class SpanIterator {
public:
    SpanIterator(const Span &s) : _it{s.begin()}, _end{s.end()} {};
    lsst::geom::Point2I next() {
        if (_it == _end) {
            throw py::stop_iteration();
        }
        return *_it++;
    };

private:
    Span::Iterator _it;
    Span::Iterator _end;
};

static void declareSpanIterator(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(py::class_<SpanIterator>(wrappers.module, "SpanIterator"), [](auto &mod, auto &cls) {
        cls.def("__iter__", [](SpanIterator &it) -> SpanIterator & { return it; });
        cls.def("__next__", &SpanIterator::next);
    });
}

void declareSpan(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PySpan(wrappers.module, "Span"), [](auto &mod, auto &cls) {
        cls.def(py::init<int, int, int>());
        cls.def(py::init<Span::Interval const &, int>());
        cls.def(py::init<>());
        cls.def("__eq__", &Span::operator==, py::is_operator());
        cls.def("__ne__", &Span::operator!=, py::is_operator());
        cls.def("__lt__", &Span::operator<, py::is_operator());
        cls.def("__len__", &Span::getWidth);
        cls.def("__str__", &Span::toString);
        // unlike most iterators, SpanPixelIterator doesn't actually refer
        // back to its container (the Span), and there's no need to keep the
        // Span alive for the lifetime of the iterator.
        cls.def("__iter__", [](const Span &s) { return SpanIterator(s); });
        cls.def("getX0", (int (Span::*)() const) & Span::getX0);
        cls.def("getX1", (int (Span::*)() const) & Span::getX1);
        cls.def("getX", &Span::getX);
        cls.def("getY", (int (Span::*)() const) & Span::getY);
        cls.def("getWidth", &Span::getWidth);
        cls.def("getMinX", &Span::getMinX);
        cls.def("getMaxX", &Span::getMaxX);
        cls.def("getBeginX", &Span::getBeginX);
        cls.def("getEndX", &Span::getEndX);
        cls.def("getMin", &Span::getMin);
        cls.def("getMax", &Span::getMax);
        cls.def("contains", (bool (Span::*)(int) const) & Span::contains);
        cls.def("contains", py::vectorize((bool (Span::*)(int, int) const)&Span::contains), "x"_a, "y"_a);
        cls.def("contains", (bool (Span::*)(lsst::geom::Point2I const &) const) & Span::contains);
        cls.def("isEmpty", &Span::isEmpty);
        cls.def("toString", &Span::toString);
        cls.def("shift", &Span::shift);
    });
}
}  // namespace
void wrapSpan(lsst::utils::python::WrapperCollection &wrappers) {
    declareSpanIterator(wrappers);
    declareSpan(wrappers);
}
}  // namespace geom
}  // namespace afw
}  // namespace lsst
