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

#include <memory>
#include <limits>
#include <sstream>

#include "lsst/cpputils/python.h"

#include "lsst/daf/base/PropertySet.h"
#include "lsst/afw/coord/Observatory.h"
#include "lsst/afw/coord/Weather.h"
#include "lsst/geom/Angle.h"
#include "lsst/geom/SpherePoint.h"
#include "lsst/afw/table/io/python.h"  // for addPersistableMethods
#include "lsst/afw/table/misc.h"
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/image/VisitInfo.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace image {

namespace {

static double const nan(std::numeric_limits<double>::quiet_NaN());
static lsst::geom::Angle const nanAngle(nan);

}  // namespace

void declareVisitInfo(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(
            nb::class_<VisitInfo, typehandling::Storable>(wrappers.module,
                                                                                      "VisitInfo"),
            [](auto &mod, auto &cls) {
                /* Constructors */
                cls.def(nb::init<double, double, daf::base::DateTime const &, double,
                                 lsst::geom::Angle const &, lsst::geom::SpherePoint const &,
                                 lsst::geom::SpherePoint const &, double, lsst::geom::Angle const &,
                                 RotType const &, coord::Observatory const &, coord::Weather const &,
                                 std::string const &, table::RecordId const &, double, std::string const &,
                                 std::string const &, std::string const &, std::string const &, bool>(),
                        "exposureTime"_a = nan, "darkTime"_a = nan, "date"_a = daf::base::DateTime(),
                        "ut1"_a = nan, "era"_a = nanAngle,
                        "boresightRaDec"_a = lsst::geom::SpherePoint(nanAngle, nanAngle),
                        "boresightAzAlt"_a = lsst::geom::SpherePoint(nanAngle, nanAngle),
                        "boresightAirmass"_a = nan, "boresightRotAngle"_a = nanAngle,
                        "rotType"_a = RotType::UNKNOWN,
                        "observatory"_a = coord::Observatory(nanAngle, nanAngle, nan),
                        "weather"_a = coord::Weather(nan, nan, nan), "instrumentLabel"_a = "", "id"_a = 0,
                        "focusZ"_a = nan, "observationType"_a = "", "scienceProgram"_a = "",
                        // default hasSimulatedContent=false for backwards compatibility
                        "observationReason"_a = "", "object"_a = "", "hasSimulatedContent"_a = false);
                cls.def(nb::init<daf::base::PropertySet const &>(), "metadata"_a);
                cls.def(nb::init<VisitInfo const &>(), "visitInfo"_a);

                table::io::python::addPersistableMethods<VisitInfo>(cls);

                /* Operators */
                cls.def(
                        "__eq__", [](VisitInfo const &self, VisitInfo const &other) { return self == other; },
                        nb::is_operator());
                cls.def(
                        "__ne__", [](VisitInfo const &self, VisitInfo const &other) { return self != other; },
                        nb::is_operator());

                /* Members */
                cls.def("getExposureTime", &VisitInfo::getExposureTime);
                cls.def("getDarkTime", &VisitInfo::getDarkTime);
                cls.def("getDate", &VisitInfo::getDate);
                cls.def("getUt1", &VisitInfo::getUt1);
                cls.def("getEra", &VisitInfo::getEra);
                cls.def("getBoresightRaDec", &VisitInfo::getBoresightRaDec);
                cls.def("getBoresightAzAlt", &VisitInfo::getBoresightAzAlt);
                cls.def("getBoresightAirmass", &VisitInfo::getBoresightAirmass);
                cls.def("getBoresightParAngle", &VisitInfo::getBoresightParAngle);
                cls.def("getBoresightRotAngle", &VisitInfo::getBoresightRotAngle);
                cls.def("getRotType", &VisitInfo::getRotType);
                cls.def("getObservatory", &VisitInfo::getObservatory);
                cls.def("getWeather", &VisitInfo::getWeather);
                cls.def("isPersistable", &VisitInfo::isPersistable);
                cls.def("getLocalEra", &VisitInfo::getLocalEra);
                cls.def("getBoresightHourAngle", &VisitInfo::getBoresightHourAngle);
                cls.def("getInstrumentLabel", &VisitInfo::getInstrumentLabel);
                cls.def("getId", &VisitInfo::getId);
                cls.def("getFocusZ", &VisitInfo::getFocusZ);
                cls.def("getObservationType", &VisitInfo::getObservationType);
                cls.def("getScienceProgram", &VisitInfo::getScienceProgram);
                cls.def("getObservationReason", &VisitInfo::getObservationReason);
                cls.def("getObject", &VisitInfo::getObject);
                cls.def("getHasSimulatedContent", &VisitInfo::getHasSimulatedContent);

                /* readonly property accessors */
                cls.def_prop_ro("exposureTime", &VisitInfo::getExposureTime);
                cls.def_prop_ro("darkTime", &VisitInfo::getDarkTime);
                cls.def_prop_ro("date", &VisitInfo::getDate);
                cls.def_prop_ro("ut1", &VisitInfo::getUt1);
                cls.def_prop_ro("era", &VisitInfo::getEra);
                cls.def_prop_ro("boresightRaDec", &VisitInfo::getBoresightRaDec);
                cls.def_prop_ro("boresightAzAlt", &VisitInfo::getBoresightAzAlt);
                cls.def_prop_ro("boresightAirmass", &VisitInfo::getBoresightAirmass);
                cls.def_prop_ro("boresightParAngle", &VisitInfo::getBoresightParAngle);
                cls.def_prop_ro("boresightRotAngle", &VisitInfo::getBoresightRotAngle);
                cls.def_prop_ro("rotType", &VisitInfo::getRotType);
                cls.def_prop_ro("observatory", &VisitInfo::getObservatory);
                cls.def_prop_ro("weather", &VisitInfo::getWeather);
                cls.def_prop_ro("isPersistable", &VisitInfo::isPersistable);
                cls.def_prop_ro("localEra", &VisitInfo::getLocalEra);
                cls.def_prop_ro("boresightHourAngle", &VisitInfo::getBoresightHourAngle);
                cls.def_prop_ro("instrumentLabel", &VisitInfo::getInstrumentLabel);
                cls.def_prop_ro("id", &VisitInfo::getId);
                cls.def_prop_ro("focusZ", &VisitInfo::getFocusZ);
                cls.def_prop_ro("observationType", &VisitInfo::getObservationType);
                cls.def_prop_ro("scienceProgram", &VisitInfo::getScienceProgram);
                cls.def_prop_ro("observationReason", &VisitInfo::getObservationReason);
                cls.def_prop_ro("object", &VisitInfo::getObject);
                cls.def_prop_ro("hasSimulatedContent", &VisitInfo::getHasSimulatedContent);

                cpputils::python::addOutputOp(cls, "__repr__");
            });
}
void declareRotType(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(nb::enum_<RotType>(wrappers.module, "RotType"), [](auto &mod, auto &enm) {
        enm.value("UNKNOWN", RotType::UNKNOWN);
        enm.value("SKY", RotType::SKY);
        enm.value("HORIZON", RotType::HORIZON);
        enm.value("MOUNT", RotType::MOUNT);
        enm.export_values();
    });
}

void wrapVisitInfo(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.daf.base");
    wrappers.addInheritanceDependency("lsst.geom");
    wrappers.addInheritanceDependency("lsst.afw.coord");
    wrappers.addInheritanceDependency("lsst.afw.typehandling");
    declareRotType(wrappers);
    declareVisitInfo(wrappers);
    wrappers.wrap([](auto &mod) {
        /* Free Functions */
        mod.def("setVisitInfoMetadata", &detail::setVisitInfoMetadata, "metadata"_a, "visitInfo"_a);
        mod.def("stripVisitInfoKeywords", &detail::stripVisitInfoKeywords, "metadata"_a);
    });
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
