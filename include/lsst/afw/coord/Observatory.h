// -*- lsst-c++ -*-

/*
 * LSST Data Management System
 * Copyright 2008-2016 LSST Corporation.
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
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

#if !defined(LSST_AFW_COORD_OBSERVATORY_H)
#define LSST_AFW_COORD_OBSERVATORY_H
/*
 * Class to hold observatory/telescope location
 */

#include <iostream>

#include "lsst/utils/hashCombine.h"
#include "lsst/geom/Angle.h"

namespace lsst {
namespace afw {
namespace coord {

/**
 * Hold the location of an observatory
 */
class Observatory final {
public:
    /**
     * Construct an Observatory with longitude and latitude specified as lsst::geom::Angle
     *
     * @param[in] longitude  telescope longitude (positive values are E of Greenwich)
     * @param[in] latitude  telescope latitude
     * @param[in] elevation  telescope elevation (meters above reference spheroid)
     */
    Observatory(lsst::geom::Angle const longitude, lsst::geom::Angle const latitude, double const elevation);

    /**
     * Construct an Observatory with longitude and latitude specified as sexagesimal strings
     *
     * @param[in] longitude  telescope longitude (dd:mm:ss.s, positive values are E of Greenwich)
     * @param[in] latitude  telescope latitude  (dd:mm:ss.s)
     * @param[in] elevation  telescope elevation (meters above reference spheroid)
     *
     */
    Observatory(std::string const& longitude, std::string const& latitude, double const elevation);

    ~Observatory() noexcept;
    Observatory(Observatory const&) noexcept;
    Observatory(Observatory&&) noexcept;
    Observatory& operator=(Observatory const&) noexcept;
    Observatory& operator=(Observatory&&) noexcept;

    /// set telescope longitude
    void setLongitude(lsst::geom::Angle const longitude);
    /// set telescope latitude (positive values are E of Greenwich)
    void setLatitude(lsst::geom::Angle const latitude);
    /// set telescope elevation (meters above reference spheroid)
    void setElevation(double const elevation);

    /// get telescope longitude (positive values are E of Greenwich)
    lsst::geom::Angle getLongitude() const noexcept;
    /// get telescope latitude
    lsst::geom::Angle getLatitude() const noexcept;
    /// get telescope elevation (meters above reference spheroid)
    double getElevation() const noexcept { return _elevation; }

    /// get string representation
    std::string toString() const;

    bool operator==(Observatory const& rhs) const noexcept {
        // Observatory may be initialized to NaN values as a placeholder, or to indicate "unknown".
        auto deltaLongitude = std::isnan(_latitude.asRadians()) && std::isnan(rhs._latitude.asRadians())
                                      ? 0.0 * lsst::geom::degrees
                                      : (_latitude - rhs._latitude).wrapCtr();
        auto deltaLatitude = std::isnan(_longitude.asRadians()) && std::isnan(rhs._longitude.asRadians())
                                     ? 0.0 * lsst::geom::degrees
                                     : (_longitude - rhs._longitude).wrapCtr();
        return (deltaLongitude == 0.0 * lsst::geom::degrees) &&
               (deltaLatitude == 0.0 * lsst::geom::degrees) &&
               ((std::isnan(_elevation) && std::isnan(rhs._elevation)) ||
                (_elevation - rhs._elevation) == 0.0);
    }
    bool operator!=(Observatory const& rhs) const noexcept { return !(*this == rhs); }

    /// Return a hash of this object
    std::size_t hash_value() const noexcept {
        // Completely arbitrary seed
        return utils::hashCombine(17, _latitude.wrapCtr(), _longitude.wrapCtr(), _elevation);
    }

private:
    lsst::geom::Angle _latitude;
    lsst::geom::Angle _longitude;
    double _elevation;
};

/**
 * Print an Observatory to the stream
 *
 * @param[in, out] os Stream to print to
 * @param[in] obs the Observatory to print
 */
std::ostream& operator<<(std::ostream& os, Observatory const& obs);
}  // namespace coord
}  // namespace afw
}  // namespace lsst

namespace std {
template <>
struct hash<lsst::afw::coord::Observatory> {
    using argument_type = lsst::afw::coord::Observatory;
    using result_type = size_t;
    size_t operator()(argument_type const& obj) const noexcept { return obj.hash_value(); }
};
}  // namespace std

#endif
