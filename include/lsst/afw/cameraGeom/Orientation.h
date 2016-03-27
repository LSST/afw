/* 
 * LSST Data Management System
 * See the COPYRIGHT and LICENSE files in the top-level directory of this
 * package for notices and licensing terms.
 */
 
#if !defined(LSST_AFW_CAMERAGEOM_ORIENTATION_H)
#define LSST_AFW_CAMERAGEOM_ORIENTATION_H

#include <string>
#include <cmath>
#include "Eigen/Dense"
#include "lsst/afw/geom.h"
#include "lsst/afw/image/Utils.h"

/**
 * @file
 *
 * Describe a Detector's orientation
 */
namespace lsst {
namespace afw {
namespace cameraGeom {

/**
 * Describe a detector's orientation in the focal plane
 *
 * All rotations are about the reference point on the detector.
 * Rotations are intrinsic, meaning each rotation is applied in the coordinates system
 * produced by the previous rotation.
 * Rotations are applied in this order: yaw (Z), pitch (Y'), and roll (X'').
 *
 * @warning: default refPoint is -0.5, -0.5 (the lower left corner of a detector).
 * This means that the default-constructed Orientation is not a unity transform,
 * but instead includes a 1/2 pixel shift.
 */
class Orientation {
public:
    explicit Orientation(
        geom::Point2D const fpPosition=geom::Point2D(0, 0),
            ///< Focal plane position of detector reference point (mm)
        geom::Point2D const refPoint=geom::Point2D(-0.5, -0.5),
            ///< Reference point on detector (pixels).
            ///< Offset is measured to this point and all all rotations are about this point.
            ///< The default value (-0.5, -0.5) is the lower left corner of the detector.
        geom::Angle const yaw=geom::Angle(0),   ///< yaw: rotation about Z (X to Y), 1st rotation
        geom::Angle const pitch=geom::Angle(0),  ///< pitch: rotation about Y' (Z'=Z to X'), 2nd rotation
        geom::Angle const roll=geom::Angle(0)  ///< roll: rotation about X'' (Y''=Y' to Z''), 3rd rotation
    );

    /// Return focal plane position of detector reference point (mm)
    geom::Point2D getFpPosition() const { return _fpPosition; }

    /// Return detector reference point (pixels)
    geom::Point2D getReferencePoint() const { return _refPoint; }

    /// Return the yaw angle
    geom::Angle getYaw() const { return _yaw; }

    /// Return the pitch angle
    lsst::afw::geom::Angle getPitch() const { return _pitch; }

    /// Return the roll angle
    geom::Angle getRoll() const { return _roll; }

    /// Return the number of quarter turns (rounded to the closest quarter)
    int getNQuarter() const;

    /**
     * @brief Generate an XYTransform from pixel to focal plane coordinates
     *
     * @return lsst::afw::geom::AffineXYTransform from pixel to focal plane coordinates
     */
    geom::AffineXYTransform makePixelFpTransform(
            geom::Extent2D const pixelSizeMm ///< Size of the pixel in mm in X and Y
    ) const;

    /**
     * @brief Generate an XYTransform from focal plane to pixel coordinates
     *
     * @return lsst::afw::geom::AffineXYTransform from focal plane to pixel coordinates
     */
    geom::AffineXYTransform makeFpPixelTransform(
            geom::Extent2D const pixelSizeMm ///< Size of the pixel in mm in X and Y
    ) const;

private:
    geom::Point2D _fpPosition;          ///< focal plane position of reference point on detector
    geom::Point2D _refPoint;            ///< reference point on detector

    lsst::afw::geom::Angle _yaw;        ///< yaw
    lsst::afw::geom::Angle _pitch;      ///< pitch
    lsst::afw::geom::Angle _roll;       ///< roll

    // Elements of the Jacobian for three space rotation projected into XY plane.
    // Turn off alignment since this is dynamically allocated (via Detector)
    Eigen::Matrix<double,2,2,Eigen::DontAlign> _rotMat;
};

}}}

#endif
