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

#include <pybind11/pybind11.h>
//#include <pybind11/stl.h>

#include "numpy/arrayobject.h"
#include "ndarray/pybind11.h"
#include "ndarray/converter.h"

#include "lsst/daf/base/Citizen.h"
#include "lsst/daf/base/PropertySet.h"
#include "lsst/afw/coord/Coord.h"
#include "lsst/afw/geom/AffineTransform.h"
#include "lsst/afw/geom/Extent.h"
#include "lsst/afw/geom/Point.h"
#include "lsst/afw/geom/XYTransform.h"
#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/table/io/pybind11.h"  // for declarePersistableFacade
#include "lsst/afw/image/Wcs.h"

namespace py = pybind11;
using namespace py::literals;

namespace lsst {
namespace afw {
namespace image {

namespace {

/// Create the pybind11 wrapper for Wcs
void declareWcs(py::module & mod) {
    table::io::declarePersistableFacade<Wcs>(mod, "Wcs");

    /* Module level */
    py::class_<Wcs,
               std::shared_ptr<Wcs>,
               daf::base::Citizen,
               table::io::Persistable,
               table::io::PersistableFacade<Wcs>> cls(mod, "Wcs");

    /* Constructors */
    cls.def(py::init<geom::Point2D const &, geom::Point2D const &,
        Eigen::Matrix2d const &, std::string const &, std::string const &,
        double, std::string const &, std::string const &, std::string const &>(),
        "crval"_a, "crpix"_a, "CD"_a, "ctype1"_a="RA---TAN",
        "ctype2"_a="DEC--TAN", "equinox"_a=2000, "raDecSys"_a="ICRS",
        "cunits1"_a="deg", "cunits2"_a="deg");

    /* Operators */
    cls.def("__eq__",
            [](Wcs const & self, Wcs const & other) { return self == other; },
            py::is_operator());
    cls.def("__ne__",
            [](Wcs const & self, Wcs const & other) { return self != other; },
            py::is_operator());

    /* Members */
    cls.def("clone", &Wcs::clone);
    cls.def("getSkyOrigin", &Wcs::getSkyOrigin);
    cls.def("getPixelOrigin", &Wcs::getPixelOrigin);
    cls.def("getCDMatrix", &Wcs::getCDMatrix);
    cls.def("flipImage", &Wcs::flipImage);
    cls.def("rotateImageBy90", &Wcs::rotateImageBy90);
    cls.def("getFitsMetadata", &Wcs::getFitsMetadata);
    cls.def("isFlipped", &Wcs::isFlipped);
    cls.def("pixArea", &Wcs::pixArea);
    cls.def("pixelScale", &Wcs::pixelScale);
    cls.def("pixelToSky", (std::shared_ptr<coord::Coord> (Wcs::*)(double, double) const) &Wcs::pixelToSky);
    cls.def("pixelToSky",
            (std::shared_ptr<coord::Coord> (Wcs::*)(geom::Point2D const &) const) &Wcs::pixelToSky);
    cls.def("pixelToSky", (void (Wcs::*)(double, double, geom::Angle&, geom::Angle&) const) &Wcs::pixelToSky);
    cls.def("skyToPixel", (geom::Point2D (Wcs::*)(geom::Angle, geom::Angle) const) &Wcs::skyToPixel);
    cls.def("skyToPixel", (geom::Point2D (Wcs::*)(coord::Coord const &) const) &Wcs::skyToPixel);
    cls.def("skyToIntermediateWorldCoord", &Wcs::skyToIntermediateWorldCoord);
    cls.def("hasDistortion", &Wcs::hasDistortion);
    cls.def("getCoordSystem", &Wcs::getCoordSystem);
    cls.def("getEquinox", &Wcs::getEquinox);
    cls.def("isSameSkySystem", &Wcs::isSameSkySystem);
    cls.def("getLinearTransform", &Wcs::getLinearTransform);
    cls.def("linearizePixelToSky",
            (geom::AffineTransform (Wcs::*)(coord::Coord const &, geom::AngleUnit) const)
                &Wcs::linearizePixelToSky,
            "coord"_a, "skyUnit"_a=geom::degrees);
    cls.def("linearizePixelToSky",
            (geom::AffineTransform (Wcs::*)(geom::Point2D const &, geom::AngleUnit) const)
                &Wcs::linearizePixelToSky,
            "coord"_a, "skyUnit"_a=geom::degrees);
    cls.def("linearizeSkyToPixel",
            (geom::AffineTransform (Wcs::*)(coord::Coord const &, geom::AngleUnit) const)
                &Wcs::linearizeSkyToPixel,
            "coord"_a, "skyUnit"_a=geom::degrees);
    cls.def("linearizeSkyToPixel",
            (geom::AffineTransform (Wcs::*)(geom::Point2D const &, geom::AngleUnit) const)
                &Wcs::linearizeSkyToPixel,
            "coord"_a, "skyUnit"_a=geom::degrees);
    cls.def("shiftReferencePixel", (void (Wcs::*)(double, double)) &Wcs::shiftReferencePixel);
    cls.def("shiftReferencePixel", (void (Wcs::*)(geom::Extent2D const &)) &Wcs::shiftReferencePixel);
    cls.def("isPersistable", &Wcs::isPersistable);
}


/// Create the pybind1`1 wrapper for XYTransformFromWcsPair
void declareXYTransformFromWcsPair(py::module & mod) {

    // TODO: Commented-out code is waiting until needed and is untested.
    // Add tests for it and enable it or remove it before the final pybind11 merge.

    /* Module level */
    py::class_<XYTransformFromWcsPair,
               std::shared_ptr<XYTransformFromWcsPair>,
               geom::XYTransform> cls(mod, "XYTransformFromWcsPair");

    /* Constructors */
    cls.def(py::init<std::shared_ptr<Wcs const>, std::shared_ptr<Wcs const>>(), "dst"_a, "src"_a);

    /* Members */
    // cls.def("invert", &XYTransformFromWcsPair::invert);
    // cls.def("clone", &XYTransformFromWcsPair::clone);
    // cls.def("forwardTransform", &XYTransformFromWcsPair::forwardTransform);
    // cls.def("reverseTransform", &XYTransformFromWcsPair::reverseTransform);
}

}  // anonymous namespace


PYBIND11_PLUGIN(_wcs) {
    py::module mod("_wcs", "Python wrapper for afw _wcs library");

    if (_import_array() < 0) {
            PyErr_SetString(PyExc_ImportError, "numpy.core.multiarray failed to import");
            return nullptr;
    };

    mod.def("makeWcs",
            (std::shared_ptr<Wcs> (*)(std::shared_ptr<daf::base::PropertySet> const &, bool)) &makeWcs,
            "fitsMetadata"_a, "stripMetadata"_a=false);
    mod.def("makeWcs",
            (std::shared_ptr<Wcs> (*)(coord::Coord const &, lsst::afw::geom::Point2D const &,
            double, double, double, double)) &makeWcs);

    declareWcs(mod);

    declareXYTransformFromWcsPair(mod);

    return mod.ptr();
}

}}}  // namespace lsst::afw::image
