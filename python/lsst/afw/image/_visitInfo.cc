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
#include "lsst/utils/python.h"

#include <memory>
#include <limits>
#include <sstream>

#include "lsst/utils/python.h"

#include "lsst/daf/base/PropertySet.h"
#include "lsst/afw/coord/Observatory.h"
#include "lsst/afw/coord/Weather.h"
#include "lsst/geom/Angle.h"
#include "lsst/geom/SpherePoint.h"
#include "lsst/afw/table/io/python.h"  // for addPersistableMethods
#include "lsst/afw/table/misc.h"
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/image/VisitInfo.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace image {

namespace {

static double const nan(std::numeric_limits<double>::quiet_NaN());
static lsst::geom::Angle const nanAngle(nan);

}  // namespace

void declareVisitInfo(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(
            py::class_<VisitInfo, typehandling::Storable>(wrappers.module,
                                                                                      "VisitInfo"),
            [](auto &mod, auto &cls) {
                /* Constructors */
                cls.def(py::init<double, double, daf::base::DateTime const &, double,
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
                cls.def(py::init<daf::base::PropertySet const &>(), "metadata"_a);
                cls.def(py::init<VisitInfo const &>(), "visitInfo"_a);

                table::io::python::addPersistableMethods<VisitInfo>(cls);

                /* Operators */
                cls.def(
                        "__eq__", [](VisitInfo const &self, VisitInfo const &other) { return self == other; },
                        py::is_operator());
                cls.def(
                        "__ne__", [](VisitInfo const &self, VisitInfo const &other) { return self != other; },
                        py::is_operator());

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
                cls.def_property_readonly("exposureTime", &VisitInfo::getExposureTime);
                cls.def_property_readonly("darkTime", &VisitInfo::getDarkTime);
                cls.def_property_readonly("date", &VisitInfo::getDate);
                cls.def_property_readonly("ut1", &VisitInfo::getUt1);
                cls.def_property_readonly("era", &VisitInfo::getEra);
                cls.def_property_readonly("boresightRaDec", &VisitInfo::getBoresightRaDec);
                cls.def_property_readonly("boresightAzAlt", &VisitInfo::getBoresightAzAlt);
                cls.def_property_readonly("boresightAirmass", &VisitInfo::getBoresightAirmass);
                cls.def_property_readonly("boresightParAngle", &VisitInfo::getBoresightParAngle);
                cls.def_property_readonly("boresightRotAngle", &VisitInfo::getBoresightRotAngle);
                cls.def_property_readonly("rotType", &VisitInfo::getRotType);
                cls.def_property_readonly("observatory", &VisitInfo::getObservatory);
                cls.def_property_readonly("weather", &VisitInfo::getWeather);
                cls.def_property_readonly("isPersistable", &VisitInfo::isPersistable);
                cls.def_property_readonly("localEra", &VisitInfo::getLocalEra);
                cls.def_property_readonly("boresightHourAngle", &VisitInfo::getBoresightHourAngle);
                cls.def_property_readonly("instrumentLabel", &VisitInfo::getInstrumentLabel);
                cls.def_property_readonly("id", &VisitInfo::getId);
                cls.def_property_readonly("focusZ", &VisitInfo::getFocusZ);
                cls.def_property_readonly("observationType", &VisitInfo::getObservationType);
                cls.def_property_readonly("scienceProgram", &VisitInfo::getScienceProgram);
                cls.def_property_readonly("observationReason", &VisitInfo::getObservationReason);
                cls.def_property_readonly("object", &VisitInfo::getObject);
                cls.def_property_readonly("hasSimulatedContent", &VisitInfo::getHasSimulatedContent);

                utils::python::addOutputOp(cls, "__repr__");
            });
}
void declareRotType(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(py::enum_<RotType>(wrappers.module, "RotType"), [](auto &mod, auto &enm) {
        enm.value("UNKNOWN", RotType::UNKNOWN);
        enm.value("SKY", RotType::SKY);
        enm.value("HORIZON", RotType::HORIZON);
        enm.value("MOUNT", RotType::MOUNT);
        enm.export_values();
    });
}

void wrapVisitInfo(lsst::utils::python::WrapperCollection &wrappers) {
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
