// -*- lsst-c++ -*-
/*
 * LSST Data Management System
 * Copyright 2017 AURA/LSST.
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

#ifndef LSST_AFW_GEOM_SKYWCS_H
#define LSST_AFW_GEOM_SKYWCS_H

#include <memory>
#include <utility>
#include <vector>

#include "Eigen/Core"
#include "astshim.h"
#include "ndarray.h"

#include "lsst/daf/base/PropertyList.h"
#include "lsst/geom/AffineTransform.h"
#include "lsst/geom/Angle.h"
#include "lsst/geom/Point.h"
#include "lsst/afw/geom/Endpoint.h"
#include "lsst/afw/geom/Transform.h"
#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/typehandling/Storable.h"

namespace lsst {
namespace afw {
namespace geom {

/**
 * Make a WCS CD matrix
 *
 * @param[in] scale  Pixel scale as an angle on sky/pixels
 * @param[in] orientation  Position angle of pixel +Y, measured from N through E.
 *                         At 0 degrees, +Y is along N and +X is along W/E if flipX false/true
 *                         At 90 degrees, +Y is along E and +X is along N/S if flipX false/true
 * @param[in] flipX  Flip x axis? See orientation for details.
 *
 * @return the CD matrix, where element (i-1, j-1) corresponds to FITS keyword CDi_j
 *                         and i, j have range [1, 2]
 *
 */
Eigen::Matrix2d makeCdMatrix(lsst::geom::Angle const &scale,
                             lsst::geom::Angle const &orientation = 0 * lsst::geom::degrees,
                             bool flipX = false);

/**
 * A 2-dimensional celestial WCS that transform pixels to ICRS RA/Dec, using the LSST standard for pixels.
 *
 * SkyWcs is an immutable object that can not only represent any standard FITS WCS, but can also contain
 * arbitrary Transforms, e.g. to model optical distortion or pixel imperfections.
 *
 * In order to make a SkyWcs that models optical distortion, say, it is usually simplest to start with
 * a standard FITS WCS (such as a TAN WCS) as an approximation, then insert a transform that models
 * optical distortion by calling makeModifiedWcs. However, it is also possible to build a SkyWcs
 * entirely from transforms, if you prefer, by building an ast::FrameDict and constructing the SkyWcs from
 * that.
 *
 * @anchor skywcs_frames **Frames of reference**
 *
 * SkyWcs internally keeps track of the following frames of reference:
 * - cameraGeom::ACTUAL_PIXELS (optional): "actual" pixel position using the @ref pixel_position_standards
 * "LSST standard". This has the same meaning as the cameraGeom::ACTUAL_PIXELS frame: actual pixels include
 * effects such as "tree ring" distortions and electrical effects at the edge of CCDs. This frame should
 * only be provided if there is a reasonable model for these imperfections.
 * - cameraGeom::PIXELS: nominal pixel position, using the @ref pixel_position_standards "LSST standard".
 *     This has the same meaning as the cameraGeom::PIXELS frame:
 *     nominal pixels may be rectangular, but are uniform in size and spacing.
 * - IWC: intermediate world coordinates (the FITS WCS concept).
 * - SKY: position on the sky as ICRS, with standard RA, Dec axis order.
 *
 * @anchor pixel_position_standards **Pixel position standards**
 *
 * The LSST standard for pixel position is: 0,0 is the center of the lower left image pixel.
 * The FITS standard for pixel position is: 1,1 is the center of the lower left image pixel.
 *
 * LSST and FITS also use a different origin for subimages:
 * - LSST pixel position is in the frame of reference of the parent image
 * - FITS pixel position is in the frame of reference of the subimage
 * However, SkyWcs does *not* keep track of this difference. Code that persists and unpersists SkyWcs
 * using FITS-WCS header cards must handle the offset, e.g. by calling `copyAtShiftedPixelOrigin`
 *
 * @anchor skywcs_frameDict **Internal details: the contained ast::FrameDict**
 *
 * SkyWcs contains an ast::FrameDict which transforms from pixels to sky (in radians) in the forward
 * direction.
 *
 * This FrameDict contains the named frames described in @ref skywcs_frames "frames of reference", e.g.
 * "SKY", "IWC", cameraGeom::PIXELS and possibly cameraGeom::ACTUAL_PIXELS. "SKY" is the current frame.
 *  If cameraGeom::ACTUAL_PIXELS is present then it is the base frame, otherwise cameraGeom::PIXELS is the
 * base frame.
 *
 * The "SKY" frame is of type ast::SkyFrame and has the following attributes:
 * - `SkyRef` is set to the sky origin of the WCS (ICRS RA, Dec) in radians.
 * - `SkyRefIs` is set to "Ignored" so that SkyRef is not used in transformations.
 *
 * The other frames are of type ast::Frame and have 2 axes.
 */
class SkyWcs final : public typehandling::Storable {
DECLARE_PERSISTABLE_FACADE(SkyWcs);
public:
    SkyWcs(SkyWcs const &) = default;
    SkyWcs(SkyWcs &&) = default;
    SkyWcs &operator=(SkyWcs const &) = delete;
    SkyWcs &operator=(SkyWcs &&) = delete;
    ~SkyWcs() override = default;

    /**
     * Equality is based on the string representations being equal
     *
     * Two SkyWcs constructed the same way will be equal, and a SkyWcs that has been saved and restored
     * will be equal to the original. However, it is possible to construct two SkyWcs that behave identically
     * as far as transforming points go, but will compare as unequal due to subtle internal differences,
     * such as a contained ast::Mapping that has a different ID in one SkyWcs than another.
     *
     * Thus equality is primarily useful for testing persistence.
     */
    bool operator==(SkyWcs const &other) const;
    bool operator!=(SkyWcs const &other) const { return !(*this == other); }

    /**
     * Construct a SkyWcs from FITS keywords
     *
     * @param[in] metadata  FITS header metadata
     * @param[in] strip  If true: strip items from `metadata` used to create the WCS,
     *    such as RADESYS, EQUINOX, CTYPE12, CRPIX12, CRVAL12, etc.
     *    Always keep keywords that might be wanted for other purpposes, including NAXIS12
     *    and date-related keywords such as "DATE-OBS" and "TIMESYS" (but not "EQUINOX").
     *
     * @throws lsst::pex::exceptions::TypeError if the metadata does not describe a celestial WCS.
     */
    explicit SkyWcs(daf::base::PropertySet &metadata, bool strip = false);

    /**
     * Construct a SkyWcs from an ast::FrameDict
     *
     * This is the most general constructor; it can be used to define any celestial WCS.
     * Note that in many cases the result will not be exactly representable as a FITS WCS.
     *
     * @param[in] frameDict  An ast::FrameDict that describes the transformation from pixels to sky.
     *     It must meet the requirements outlined in @ref skywcs_frameDict "the contained ast::FrameDict".
     *
     * @throw lsst::pex::exceptions::TypeError if `frameDict` is missing
     * any of the required @ref skywcs_frames "frames of reference".
     */
    explicit SkyWcs(ast::FrameDict const &frameDict);

    /**
     * Return a copy of this SkyWcs with the pixel origin shifted by the specified amount.
     *
     *     new pixel origin = the old pixel origin + shift
     *
     * @param[in] shift  The amount by which to shift the pixel origin (pixels)
     */
    std::shared_ptr<SkyWcs> copyAtShiftedPixelOrigin(lsst::geom::Extent2D const &shift) const;

    /**
     * Return the WCS as FITS WCS metadata
     *
     * @param[in] precise  Fail if the WCS cannot be accurately represented as FITS metadata?
     *      If False then return an approximation. For now that approximation is pure TAN
     *      but as of DM-13170 it will be a fit TAN-SIP.
     *      The required precision is set by constant TIGHT_FITS_TOL in SkyWcs.cc
     *
     * FITS representations of WCS are described in "Representations of World Coordinates in FITS"
     * by Greisen and Calabretta and several related papers.
     *
     * The required precision is hard-coded as constant TIGHT_FITS_TOL in SkyWcs.cc
     *
     * @throws lsst::pex::exceptions::RuntimeError if precise is true and AST cannot represent
     * this WCS as a FITS WCS to sufficient precision.
     */
    std::shared_ptr<daf::base::PropertyList> getFitsMetadata(bool precise = false) const;

    /**
     * Get the pixel scale at the specified pixel position
     *
     * The scale is the square root of the area of the specified pixel on the sky.
     *
     * @warning Unlike getPixelScale() the value is not cached, even if pixel = pixel origin.
     */
    lsst::geom::Angle getPixelScale(lsst::geom::Point2D const &pixel) const;

    /**
     * Get the pixel scale at the pixel origin
     *
     * The scale is the square root of the area of the specified pixel on the sky.
     *
     * The value is cached, so this is a cheap call.
     */
    lsst::geom::Angle getPixelScale() const { return _pixelScaleAtOrigin; };

    /**
     * Get the pixel origin, in pixels, using the LSST convention
     *
     * This is CRPIX1 - 1, CRPIX2 -1 in FITS terminology
     */
    lsst::geom::Point2D getPixelOrigin() const { return _pixelOrigin; };

    /**
     * Get the sky origin, the celestial fiducial point
     *
     * This is CRVAL1, CRVAL2 in FITS terminology
     */
    lsst::geom::SpherePoint getSkyOrigin() const;

    /**
     * Get the 2x2 CD matrix at the specified pixel position
     *
     * The elements are in degrees
     */
    Eigen::Matrix2d getCdMatrix(lsst::geom::Point2D const &pixel) const;

    /**
     * Get the 2x2 CD matrix at the pixel origin
     *
     * The elements are in degrees
     */
    Eigen::Matrix2d getCdMatrix() const;

    /**
     * Get a local TAN WCS approximation to this WCS at the specified pixel position
     */
    std::shared_ptr<SkyWcs> getTanWcs(lsst::geom::Point2D const &pixel) const;

    /**
     * Get the contained FrameDict
     *
     * The base frame will be cameraGeom::PIXELS or cameraGeom::ACTUAL_PIXELS and the current frame will be
     * SKY, so the forward transform goes from pixels (using the LSST zero convention) to sky ICRS RA, Dec (in
     * radians). For more details see
     *  @ref skywcs_frameDict "the contained ast::FrameDict"
     */
    std::shared_ptr<const ast::FrameDict> getFrameDict() const;

    /**
     * Get a TransformPoint2ToSpherePoint that transforms pixels to sky in the forward direction
     * and sky to pixels in the inverse direction.
     */
    std::shared_ptr<const TransformPoint2ToSpherePoint> getTransform() const { return _transform; };

    /**
     * Does the WCS follow the convention of North=Up, East=Left?
     *
     * @returns False/true if E is along -X/+X when the N/E axes are rotated so that N is along image +Y.
     * @throws lsst::pex::exceptions::RuntimeError if the parity cannot be determined
     * because the CD matrix is singular.
     */
    bool isFlipped() const;

    bool isPersistable() const noexcept override { return true; }

    /**
     * Return the local linear approximation to pixelToSky at a point given in sky coordinates.
     *
     * The local linear approximation is defined such the following is true (ignoring floating-point errors):
     *
     *     wcs.linearizePixelToSky(sky, skyUnit)(wcs.skyToPixel(sky)) == sky.getPosition(skyUnit);
     *
     * (lsst::geom::AffineTransform::operator() applies the transform in the forward direction)
     *
     * @param[in] coord   Position in sky coordinates where transform is desired.
     * @param[in] skyUnit Units to use for sky coordinates; units of matrix elements will be skyUnits/pixel.
     */
    lsst::geom::AffineTransform linearizePixelToSky(lsst::geom::SpherePoint const &coord,
                                                    lsst::geom::AngleUnit const &skyUnit) const;

    /**
     * Return the local linear approximation to pixelToSky at a point given in pixel coordinates.
     *
     * The local linear approximation is defined such the following is true (ignoring floating-point errors):
     *
     *     wcs.linearizePixelToSky(pixel, skyUnit)(pixel) == wcs.pixelToSky(pixel).getPosition(skyUnit)
     *
     * (lsst::geom::AffineTransform::operator() applies the transform in the forward direction)
     *
     * @param[in] pixel     Position in pixel coordinates where transform is desired.
     * @param[in] skyUnit Units to use for sky coordinates; units of matrix elements will be skyUnits/pixel.
     */
    lsst::geom::AffineTransform linearizePixelToSky(lsst::geom::Point2D const &pixel,
                                                    lsst::geom::AngleUnit const &skyUnit) const;

    /**
     * Return the local linear approximation to skyToPixel at a point given in sky coordinates.
     *
     * The local linear approximation is defined such the following is true (ignoring floating-point errors):
     *
     *     wcs.linearizeSkyToPixel(sky, skyUnit)(sky.getPosition(skyUnit)) == wcs.skyToPixel(sky)
     *
     * (lsst::geom::AffineTransform::operator() applies the transform in the forward direction)
     *
     * @param[in] coord   Position in sky coordinates where transform is desired.
     * @param[in] skyUnit Units to use for sky coordinates; units of matrix elements will be pixels/skyUnit.
     */
    lsst::geom::AffineTransform linearizeSkyToPixel(lsst::geom::SpherePoint const &coord,
                                                    lsst::geom::AngleUnit const &skyUnit) const;

    /**
     * Return the local linear approximation to skyToPixel at a point given in pixel coordinates.
     *
     * The local linear approximation is defined such the following is true (ignoring floating-point errors):
     *
     *     wcs.linearizeSkyToPixel(pixel, skyUnit)(wcs.pixelToSky(pixel).getPosition(skyUnit)) == pixel
     *
     * (lsst::geom::AffineTransform::operator() applies the transform in the forward direction)
     *
     * @param[in] pixel     Position in pixel coordinates where transform is desired.
     * @param[in] skyUnit Units to use for sky coordinates; units of matrix elements will be pixels/skyUnit.
     */
    lsst::geom::AffineTransform linearizeSkyToPixel(lsst::geom::Point2D const &pixel,
                                                    lsst::geom::AngleUnit const &skyUnit) const;

    /**
     * Compute sky position(s) from pixel position(s)
     */
    //@{
    lsst::geom::SpherePoint pixelToSky(lsst::geom::Point2D const &pixel) const {
        return _transform->applyForward(pixel);
    }
    lsst::geom::SpherePoint pixelToSky(double x, double y) const {
        return pixelToSky(lsst::geom::Point2D(x, y));
    }
    std::vector<lsst::geom::SpherePoint> pixelToSky(std::vector<lsst::geom::Point2D> const &pixels) const {
        return _transform->applyForward(pixels);
    }
    //@}

    /**
     * Compute pixel position(s) from sky position(s)
     */
    //@{
    lsst::geom::Point2D skyToPixel(lsst::geom::SpherePoint const &sky) const {
        return _transform->applyInverse(sky);
    }
    std::vector<lsst::geom::Point2D> skyToPixel(std::vector<lsst::geom::SpherePoint> const &sky) const {
        return _transform->applyInverse(sky);
    }
    //@}

    static std::string getShortClassName();

    /**
     * Does this SkyWcs have an approximate SkyWcs that can be represented as standard FITS WCS?
     *
     * This feature is not yet implemented, so hasFitsApproximation is always false.
     */
    bool hasFitsApproximation() const { return false; }

    /**
     * Return true getFitsMetadata(true) will succeed, false if not.
     *
     * In other words, true indicates that the WCS can be accurately represented using FITS WCS metadata.
     */
    bool isFits() const;

    /**
     * Deserialize a SkyWcs from an input stream
     *
     * @param[in] is  input stream from which to deserialize this SkyWcs
     */
    static std::shared_ptr<SkyWcs> readStream(std::istream &is);

    /// Deserialize a SkyWcs from a string, using the same format as readStream
    static std::shared_ptr<SkyWcs> readString(std::string &str);

    /**
     * Serialize this SkyWcs to an output stream
     *
     * Version 1 format is as follows:
     * - The version number (an integer)
     * - A space
     * - getShortClassName()
     * - A space
     * - hasFitsApproximation()
     * - A space
     * - The contained ast::FrameDict written using FrameDict.show(os, false)
     *
     * If and when fits approximation is supported, the approximate WCS will
     * be written as a second FrameDict immediately following the first.
     *
     * @param[out] os  output stream to which to serialize this SkyWcs
     */
    void writeStream(std::ostream &os) const;

    /// Serialize this SkyWcs to a string, using the same format as writeStream
    std::string writeString() const;

    // Override methods required by afw::typehandling::Storable

    /// Create a new SkyWcs that is a copy of this one.
    std::shared_ptr<typehandling::Storable> cloneStorable() const override;

    /**
     * Create a string representation of this object.
     *
     * An example of the output might look like this (note the newlines):
     *
     *     FITS standard SkyWcs:
     *     Sky Origin: (0.000000, +45.000000)
     *     Pixel Origin: (100, 100)
     *     Pixel Scale: 1 arcsec/pixel
     */
    std::string toString() const override;

    /**
     * Compare this object to another Storable.
     *
     * @returns `*this == other` if `other` is a SkyWcs; otherwise `false`.
     */
    bool equals(typehandling::Storable const &other) const noexcept override;

protected:
    // Override methods required by afw::table::io::Persistable
    std::string getPersistenceName() const override;
    std::string getPythonModule() const override;
    void write(OutputArchiveHandle &handle) const override;

private:
    /*
     * Construct a SkyWcs from a shared pointer to an ast::FrameDict
     *
     * The frameDict may be modified.
     */
    explicit SkyWcs(std::shared_ptr<ast::FrameDict> frameDict);

    /*
     * Check a FrameDict to see if it can safely be used for a SkyWcs
     * Return a copy so that it can be used as an argument to the SkyWcs(shared_ptr<FrameDict>) constructor
     */
    std::shared_ptr<ast::FrameDict> _checkFrameDict(ast::FrameDict const &frameDict) const;

    // the full FrameDict, for operations that need intermediate frames
    std::shared_ptr<const ast::FrameDict> _frameDict;
    // cached transform from _frameDict, for fast computation of pixels<->sky
    std::shared_ptr<const TransformPoint2ToSpherePoint> _transform;
    lsst::geom::Point2D _pixelOrigin;       // cached pixel origin
    lsst::geom::Angle _pixelScaleAtOrigin;  // cached pixel scale at pixel origin

    /*
     * Implementation for the overloaded public linearizePixelToSky methods, requiring both a pixel coordinate
     * and the corresponding sky coordinate.
     */
    lsst::geom::AffineTransform _linearizePixelToSky(lsst::geom::Point2D const &pix,
                                                     lsst::geom::SpherePoint const &coord,
                                                     lsst::geom::AngleUnit const &skyUnit) const;

    /*
     * Implementation for the overloaded public linearizeSkyToPixel methods, requiring both a pixel coordinate
     * and the corresponding sky coordinate.
     */
    lsst::geom::AffineTransform _linearizeSkyToPixel(lsst::geom::Point2D const &pix,
                                                     lsst::geom::SpherePoint const &coord,
                                                     lsst::geom::AngleUnit const &skyUnit) const;

    /// Compute _pixelOrigin and _pixelScaleAtOrigin
    void _computeCache() {
        _transform = std::make_shared<TransformPoint2ToSpherePoint>(*_frameDict->getMapping(), true);
        _pixelOrigin = skyToPixel(getSkyOrigin());
        _pixelScaleAtOrigin = getPixelScale(_pixelOrigin);
    }
};

/**
 * Return a copy of a FITS-WCS with pixel positions flipped around a specified center
 *
 * @param[in] wcs  The initial WCS
 * @param[in] flipLR  Flip pixel positions left/right about `center`
 * @param[in] flipTB  Flip pixel positions top/bottom about `center`
 * @param[in] center  Center pixel position of flip
 */
std::shared_ptr<SkyWcs> makeFlippedWcs(SkyWcs const &wcs, bool flipLR, bool flipTB,
                                       lsst::geom::Point2D const &center);

/**
 * Create a new SkyWcs whose pixels are transformed by pixelTransform, as described below.
 *
 * If modifyActualPixels is true and the cameraGeom::ACTUAL_PIXELS frame exists then pixelTransform
 * is inserted just after the cameraGeom::ACTUAL_PIXELS frame:
 *
 *     newActualPixelsToPixels = pixelTransform -> oldActualPixelsToPixels
 *
 * This is appropriate for shifting a WCS, e.g. when writing FITS metadata for a subimage.
 *
 * If modifyActualPixels is false or the cameraGeom::ACTUAL_PIXELS frame does not exist then pixelTransform
 * is inserted just after the cameraGeom::PIXELS frame:
 *
 *     newPixelsToIwc = pixelTransform -> oldPixelsToIwc
 *
 * This is appropriate for inserting a model for optical distortion.
 *
 * Other than the change described above, the new SkyWcs will be just like the old one.
 *
 * @param[in] pixelTransform  Transform to insert
 * @param[in] wcs  Input WCS
 * @param[in] modifyActualPixels  Location at which to insert the transform;
 *    if true and the cameraGeom::ACTUAL_PIXELS frame is present then insert just after the
 * cameraGeom::ACTUAL_PIXELS frame, else insert just after the cameraGeom::PIXELS frame.
 * @return the new WCS
 */
std::shared_ptr<SkyWcs> makeModifiedWcs(TransformPoint2ToPoint2 const &pixelTransform, SkyWcs const &wcs,
                                        bool modifyActualPixels);

/**
 * Construct a SkyWcs from FITS keywords
 *
 * This function is preferred over calling the SkyWcs metadata constructor directly
 * because it allows us to change SkyWcs to an abstract base class in the future,
 * without affecting code that constructs a WCS from FITS metadata.
 *
 * @param[in] metadata  FITS header metadata
 * @param[in] strip  If true: strip items from `metadata` used to create the WCS,
 *    such as RADESYS, EQUINOX, CTYPE12, CRPIX12, CRVAL12, etc.
 *    Always keep keywords that might be wanted for other purpposes, including NAXIS12
 *    and date-related keywords such as "DATE-OBS" and "TIMESYS" (but not "EQUINOX").
 *
 * @throws lsst::pex::exceptions::TypeError if the metadata does not describe a celestial WCS.
 */
std::shared_ptr<SkyWcs> makeSkyWcs(daf::base::PropertySet &metadata, bool strip = false);

/**
 * Construct a simple FITS SkyWcs with no distortion
 *
 * @param[in] crpix  Center of projection on the CCD using the LSST convention:
 *                     0, 0 is the lower left pixel of the parent image
 * @param[in] crval  Center of projection on the sky
 * @param[in] cdMatrix  CD matrix, where element (i-1, j-1) corresponds to FITS keyword CDi_j
 *                     and i, j have range [1, 2]. May be computed by calling makeCdMatrix.
 * @param[in] projection  The name of the FITS WCS projection, e.g. "TAN" or "STG"
 */
std::shared_ptr<SkyWcs> makeSkyWcs(lsst::geom::Point2D const &crpix, lsst::geom::SpherePoint const &crval,
                                   Eigen::Matrix2d const &cdMatrix, std::string const &projection = "TAN");

/**
 * Construct a FITS SkyWcs from camera geometry
 *
 * @param[in] pixelsToFieldAngle  Transformation from @ref cameraGeom::PIXELS "pixels"
 *              to @ref cameraGeom::FIELD_ANGLE "field angle" (in radians).
 * @param[in] orientation  Position angle of focal plane +Y, measured from N through E at crval.
 *                         At 0 degrees, +Y is along N and +X is along W/E if flipX false/true.
 *                         At 90 degrees, +Y is along E and +X is along N/S if flipX false/true.
 * @param[in] flipX  Flip x axis? See orientation for details.
 * @param[in] boresight    ICRS sky position at the boresight (field angle (0, 0)).
 * @param[in] projection  The name of the FITS WCS projection, e.g. "TAN" or "STG".
 * @return a SkyWcs whose sky origin is the boresight and pixel origin is focal plane (0, 0).
 *
 * @note Unlike makeCdMatrix, `orientation` is with respect to the focal plane axes, not the CCD axes.
 *      This is because field angle is defined with respect to focal plane axes.
 */
std::shared_ptr<SkyWcs> makeSkyWcs(TransformPoint2ToPoint2 const &pixelsToFieldAngle,
                                   lsst::geom::Angle const &orientation, bool flipX,
                                   lsst::geom::SpherePoint const &boresight,
                                   std::string const &projection = "TAN");

/**
 * Construct a TAN-SIP SkyWcs with forward SIP distortion terms and an iterative inverse.
 *
 * @param[in] crpix    Center of projection on the CCD using the LSST convention:
 *                      0, 0 is the lower left pixel of the parent image
 * @param[in] crval    Center of projection on the sky
 * @param[in] cdMatrix CD matrix, where element (i-1, j-1) corresponds to FITS keyword CDi_j
 *                      and i, j have range [1, 2]. May be computed by calling makeCdMatrix.
 * @param[in] sipA     Forward distortion matrix for axis 1
 * @param[in] sipB     Forward distortion matrix for axis 2
 */
std::shared_ptr<SkyWcs> makeTanSipWcs(lsst::geom::Point2D const &crpix, lsst::geom::SpherePoint const &crval,
                                      Eigen::Matrix2d const &cdMatrix, Eigen::MatrixXd const &sipA,
                                      Eigen::MatrixXd const &sipB);

/**
 * Construct a TAN WCS with forward and inverse SIP distortion terms.
 *
 * @param[in] crpix    Center of projection on the CCD using the LSST convention:
 *                      0, 0 is the lower left pixel of the parent image
 * @param[in] crval    Center of projection on the sky
 * @param[in] cdMatrix CD matrix, where element (i-1, j-1) corresponds to FITS keyword CDi_j
 *                      and i, j have range [1, 2]. May be computed by calling makeCdMatrix.
 * @param[in] sipA     Forward distortion matrix for axis 1
 * @param[in] sipB     Forward distortion matrix for axis 2
 * @param[in] sipAp    Reverse distortion matrix for axis 1
 * @param[in] sipBp    Reverse distortion matrix for axis 2
 */
std::shared_ptr<SkyWcs> makeTanSipWcs(lsst::geom::Point2D const &crpix, lsst::geom::SpherePoint const &crval,
                                      Eigen::Matrix2d const &cdMatrix, Eigen::MatrixXd const &sipA,
                                      Eigen::MatrixXd const &sipB, Eigen::MatrixXd const &sipAp,
                                      Eigen::MatrixXd const &sipBp);
/**
 * A Transform obtained by putting two SkyWcs objects "back to back".
 *
 * @param src the WCS for the source pixels
 * @param dst the WCS for the destination pixels
 * @returns a Transform whose forward transformation converts from `src`
 *          pixels to `dst` pixels, and whose inverse transformation converts
 *          in the opposite direction.
 *
 * @exceptsafe Provides basic exception safety.
 */
std::shared_ptr<TransformPoint2ToPoint2> makeWcsPairTransform(SkyWcs const &src, SkyWcs const &dst);

/**
 * Return a transform from intermediate world coordinates to sky
 */
std::shared_ptr<TransformPoint2ToSpherePoint> getIntermediateWorldCoordsToSky(SkyWcs const &wcs,
                                                                              bool simplify = true);

/**
 * Return a transform from pixel coordinates to intermediate world coordinates
 *
 * The pixel frame is is the base frame: cameraGeom::ACTUAL_PIXELS, if present, else cameraGeom::PIXELS.
 */
std::shared_ptr<TransformPoint2ToPoint2> getPixelToIntermediateWorldCoords(SkyWcs const &wcs,
                                                                           bool simplify = true);

/**
 * Print a SkyWcs to an ostream (delegates to SkyWcs.toString()).
 */
std::ostream &operator<<(std::ostream &os, SkyWcs const &wcs);

}  // namespace geom
}  // namespace afw
}  // namespace lsst

#endif
