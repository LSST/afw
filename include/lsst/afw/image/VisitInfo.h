// -*- LSST-C++ -*- // fixed format comment for emacs
/*
 * LSST Data Management System
 * Copyright 2016 LSST Corporation.
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

#ifndef LSST_AFW_IMAGE_VISITINFO_H_INCLUDED
#define LSST_AFW_IMAGE_VISITINFO_H_INCLUDED

#include <cmath>
#include <limits>

#include "lsst/base.h"
#include "lsst/daf/base.h"
#include "lsst/afw/coord/Observatory.h"
#include "lsst/afw/coord/Weather.h"
#include "lsst/geom/Point.h"
#include "lsst/geom/SpherePoint.h"
#include "lsst/afw/table/misc.h"  // for RecordId
#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/typehandling/Storable.h"

namespace lsst {
namespace afw {
namespace image {

/// Type of rotation
enum class RotType {
    UNKNOWN,  ///< Rotation angle is unknown. Note: if there is no instrument rotator then it is better
              ///< to compute SKY or HORIZON and use that rotation type rather than specify UNKNOWN.
    SKY,      ///< Position angle of focal plane +Y, measured from N through E.
              ///< At 0 degrees, +Y is along N and +X is along E/W depending on handedness.
              ///< At 90 degrees, +Y is along E and +X is along S/N depending on handedness.
    HORIZON,  ///< Position angle of focal plane +Y, measured from +Alt through +Az.
              ///< At 0 degrees, +Y is along +Alt and +X is along +/-Az, depending on handedness.
              ///< At 90 degrees, +Y is along +Az and +X is along -/+Alt, depending on handedness.
    MOUNT     ///< The position sent to the instrument rotator; the details depend on the rotator.
};

/**
 * Information about a single exposure of an imaging camera.
 *
 * Includes exposure duration and date, and telescope pointing and orientation.
 *
 * All information is for the middle of the exposure
 * and at the boresight (center of the focal plane).
 * Thus for a mosaic camera VisitInfo is the same for all detectors in the mosaic.
 *
 * VisitInfo is immutable.
 */
class VisitInfo : public typehandling::Storable {
DECLARE_PERSISTABLE_FACADE(VisitInfo);
public:
    /**
     * Construct a VisitInfo
     *
     * @param[in] exposureTime  exposure duration (shutter open time); (sec)
     * @param[in] darkTime  time from CCD flush to readout, including shutter open time (despite the name);
                    (sec)
     * @param[in] date  TAI (international atomic time) MJD date at middle of exposure
     * @param[in] ut1  UT1 (universal time) MJD date at middle of exposure
     * @param[in] era  earth rotation angle at middle of exposure
     * @param[in] boresightRaDec  ICRS RA/Dec of boresight at middle of exposure
     * @param[in] boresightAzAlt  refracted apparent topocentric Az/Alt of boresight at middle of exposure
     * @param[in] boresightAirmass  airmass at the boresight, relative to zenith at sea level
     * @param[in] boresightRotAngle  rotation angle at boresight at middle of exposure;
                        see getBoresightRotAngle for details
     * @param[in] rotType  rotation type
     * @param[in] observatory  observatory longitude, latitude and altitude
     * @param[in] weather  basic weather information for computing air mass
     * @param[in] instrumentLabel  Short name of the instrument that took this data (e.g. "HSC")
     * @param[in] id  Identifier of this full focal plane data.
     * @param[in] focusZ Defocal distance of main-cam hexapod in mm. 0 is in focus.;
     *                   Extra-focal is negative while intra-focal is positive.
     * @param[in] observationType Type of this observation (e.g. science, dark, flat, bias, focus).
     * @param[in] scienceProgram Observing program (survey or proposal) identifier.
     * @param[in] observationReason Reason this observation was taken, or its purpose ('science' and
     *                              'calibration' are common values).
     * @param[in] object Object of interest or field name.
     * @param[in] hasSimulatedContent Was any part of this observation simulated?
     */
    explicit VisitInfo(double exposureTime, double darkTime, daf::base::DateTime const &date, double ut1,
                       lsst::geom::Angle const &era, lsst::geom::SpherePoint const &boresightRaDec,
                       lsst::geom::SpherePoint const &boresightAzAlt, double boresightAirmass,
                       lsst::geom::Angle const &boresightRotAngle, RotType const &rotType,
                       coord::Observatory const &observatory, coord::Weather const &weather,
                       std::string const &instrumentLabel, table::RecordId const &id, double focusZ,
                       std::string const &observationType, std::string const &scienceProgram,
                       std::string const &observationReason, std::string const &object,
                       bool hasSimulatedContent)
            : _exposureTime(exposureTime),
              _darkTime(darkTime),
              _date(date),
              _ut1(ut1),
              _era(era),
              _boresightRaDec(boresightRaDec),
              _boresightAzAlt(boresightAzAlt),
              _boresightAirmass(boresightAirmass),
              _boresightRotAngle(boresightRotAngle),
              _rotType(rotType),
              _observatory(observatory),
              _weather(weather),
              _instrumentLabel(instrumentLabel),
              _id(id),
              _focusZ(focusZ),
              _observationType(observationType),
              _scienceProgram(scienceProgram),
              _observationReason(observationReason),
              _object(object),
              _hasSimulatedContent(hasSimulatedContent) {}

    explicit VisitInfo(daf::base::PropertySet const &metadata);

    ~VisitInfo() override = default;

    VisitInfo(VisitInfo const &) = default;
    VisitInfo(VisitInfo &&) = default;
    VisitInfo &operator=(VisitInfo const &) = default;
    VisitInfo &operator=(VisitInfo &&) = default;

    bool operator==(VisitInfo const &other) const;
    bool operator!=(VisitInfo const &other) const { return !(*this == other); };

    /// Return a hash of this object.
    std::size_t hash_value() const noexcept override;

    /// get exposure duration (shutter open time); (sec)
    double getExposureTime() const { return _exposureTime; }

    /// get time from CCD flush to exposure readout, including shutter open time (despite the name); (sec)
    double getDarkTime() const { return _darkTime; }

    /// get uniform date and time at middle of exposure
    daf::base::DateTime getDate() const { return _date; }

    /// get UT1 (universal time) MJD date at middle of exposure
    double getUt1() const { return _ut1; }

    /// get earth rotation angle at middle of exposure
    lsst::geom::Angle getEra() const { return _era; }

    /// get ICRS RA/Dec position at the boresight
    /// (and at the middle of the exposure, if it varies with time)
    lsst::geom::SpherePoint getBoresightRaDec() const { return _boresightRaDec; }

    /// get refracted apparent topocentric Az/Alt position at the boresight
    /// (and at the middle of the exposure, if it varies with time)
    lsst::geom::SpherePoint getBoresightAzAlt() const { return _boresightAzAlt; }

    /// get airmass at the boresight, relative to zenith at sea level
    /// (and at the middle of the exposure, if it varies with time)
    double getBoresightAirmass() const { return _boresightAirmass; }

    /**
     * Get rotation angle at boresight at middle of exposure
     *
     * The meaning of rotation angle depends on @ref RotType "rotType".  For example, if `rotType` is SKY
     * the angle is the position angle of the focal plane +Y with respect to North.
     */
    lsst::geom::Angle getBoresightRotAngle() const { return _boresightRotAngle; }

    /// get rotation type of boresightRotAngle
    RotType getRotType() const { return _rotType; }

    /// get observatory longitude, latitude and elevation
    coord::Observatory getObservatory() const { return _observatory; }

    /// get basic weather information
    coord::Weather getWeather() const { return _weather; }

    bool isPersistable() const noexcept override { return true; }

    // get the local sidereal time on the meridian (equivalent, but not equal, to Local Mean Sidereal Time)
    lsst::geom::Angle getLocalEra() const;

    // get hour angle at the boresight
    lsst::geom::Angle getBoresightHourAngle() const;

    std::string getInstrumentLabel() const { return _instrumentLabel; }

    table::RecordId getId() const { return _id; }

    // get defocal distance (mm)
    double getFocusZ() const { return _focusZ; }

    std::string getObservationType() const { return _observationType; }
    std::string getScienceProgram() const { return _scienceProgram; }
    std::string getObservationReason() const { return _observationReason; }
    std::string getObject() const { return _object; }
    bool getHasSimulatedContent() const { return _hasSimulatedContent; }

    /**
     * Get parallactic angle at the boresight
     *
     * Equal to the angle between the North celestial pole and Zenith at the boresight.
     * Or, the angular separation between two great circle arcs that meet at the object:
     *   One passing through the North celestial pole, and the other through zenith.
     * For an object on the meridian the angle is zero if it is South of zenith and pi if it is North of
     * zenith The angle is positive for objects East of the meridian, and negative for objects to the West.
     */
    lsst::geom::Angle getBoresightParAngle() const;

    /// Create a new VisitInfo that is a copy of this one.
    std::shared_ptr<typehandling::Storable> cloneStorable() const override;

    /// Create a string representation of this object.
    std::string toString() const override;

    /**
     * Compare this object to another Storable.
     *
     * @returns `*this == other` if `other` is a VisitInfo; otherwise `false`.
     */
    bool equals(typehandling::Storable const &other) const noexcept override;

protected:
    std::string getPersistenceName() const override;

    void write(OutputArchiveHandle &handle) const override;

private:
    double _exposureTime;
    double _darkTime;
    daf::base::DateTime _date;
    double _ut1;
    lsst::geom::Angle _era;
    lsst::geom::SpherePoint _boresightRaDec;
    lsst::geom::SpherePoint _boresightAzAlt;
    double _boresightAirmass;
    lsst::geom::Angle _boresightRotAngle;
    RotType _rotType;
    coord::Observatory _observatory;
    coord::Weather _weather;
    std::string _instrumentLabel;
    table::RecordId _id;
    double _focusZ;
    std::string _observationType;
    std::string _scienceProgram;
    std::string _observationReason;
    std::string _object;
    bool _hasSimulatedContent;
};

std::ostream &operator<<(std::ostream &os, VisitInfo const &visitInfo);

namespace detail {

/**
 * Set FITS metadata from a VisitInfo
 *
 * @param[in,out] metadata  FITS keyword metadata to set
 * @param[in] visitInfo  instance of VisitInfo from which to set metadata
 */
void setVisitInfoMetadata(daf::base::PropertyList &metadata, VisitInfo const &visitInfo);

/**
 * Remove VisitInfo-related keywords from the metadata
 *
 * @param[in,out] metadata  FITS keyword metadata
 *
 * @returns Number of keywords stripped
 */
int stripVisitInfoKeywords(daf::base::PropertySet &metadata);

}  // namespace detail
}  // namespace image
}  // namespace afw
}  // namespace lsst

namespace std {
template <>
struct hash<lsst::afw::image::VisitInfo> {
    using argument_type = lsst::afw::image::VisitInfo;
    using result_type = size_t;
    size_t operator()(argument_type const &obj) const noexcept { return obj.hash_value(); }
};
}  // namespace std

#endif  // !LSST_AFW_IMAGE_VISITINFO_H_INCLUDED
