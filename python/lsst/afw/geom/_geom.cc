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
#include "lsst/cpputils/python.h"

namespace lsst {
namespace afw {
namespace geom {

void wrapEndpoint(lsst::cpputils::python::WrapperCollection &);
namespace polygon {
void wrapPolygon(lsst::cpputils::python::WrapperCollection &);
}
void wrapSipApproximation(lsst::cpputils::python::WrapperCollection &);
void wrapSkyWcs(lsst::cpputils::python::WrapperCollection &);
void wrapSpan(lsst::cpputils::python::WrapperCollection &);
void wrapSpanSet(lsst::cpputils::python::WrapperCollection &);
void wrapTransform(lsst::cpputils::python::WrapperCollection &);
void wrapTransformFactory(lsst::cpputils::python::WrapperCollection &);
void wrapWcsUtils(lsst::cpputils::python::WrapperCollection &);
namespace detail {
void wrapFrameSetUtils(lsst::cpputils::python::WrapperCollection &);
}

NB_MODULE(_geom, mod) {
    lsst::cpputils::python::WrapperCollection wrappers(mod, "lsst.afw.geom");
    wrapEndpoint(wrappers);
    polygon::wrapPolygon(wrappers);
    wrapSipApproximation(wrappers);
    wrapSkyWcs(wrappers);
    wrapSpan(wrappers);
    wrapSpanSet(wrappers);
    wrapTransform(wrappers);
    wrapTransformFactory(wrappers);
    wrappers.makeSubmodule("wcsUtils");
    wrapWcsUtils(wrappers);
    wrappers.finish();
}
}  // namespace geom
}  // namespace afw
}  // namespace lsst
