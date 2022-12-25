/*
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

#ifndef LSST_AFW_CAMERAGEOM_CAMERA_H
#define LSST_AFW_CAMERAGEOM_CAMERA_H

#include <string>
#include <memory>

#include "lsst/afw/cameraGeom/DetectorCollection.h"
#include "lsst/afw/cameraGeom/TransformMap.h"
#include "lsst/afw/cameraGeom/CameraSys.h"

namespace lsst {
namespace afw {
namespace cameraGeom {

/**
 * An immutable representation of a camera.
 *
 * Cameras are created (and modified, when necessary) via the Camera::Builder
 * helper class.
 */
class Camera : public DetectorCollection, public table::io::PersistableFacade<Camera> {
public:
    class Builder;

    using DetectorList = DetectorCollection::List;

    // Camera is immutable, so it cannot be moveable.  It is also always held
    // by shared_ptr, so there is no good reason to copy it.
    Camera(Camera const &) = delete;
    Camera(Camera &&) = delete;

    // Camera is immutable, so it cannot be assignable.
    Camera &operator=(Camera const &) = delete;
    Camera &operator=(Camera &&) = delete;

    virtual ~Camera() noexcept;

    /**
     * Create a Camera::Builder object initialized with this camera's state.
     *
     * This is simply a shortcut for `Camera::Builder(*this)`.
     */
    Camera::Builder rebuild() const;

    /**
     * Return the name of the camera
     */
    std::string getName() const { return _name; }

    /**
     * Return the fully-qualified name of the Python class that provides this Camera's PupilFactory.
     */
    std::string getPupilFactoryName() const { return _pupilFactoryName; }

    /**
     * Return True if there is an x-axis flip from FOCAL_PLANE to FIELD_ANGLE, False otherwise.
     *
     * @details Cameras with an even number of reflective surfaces (e.g. LATISS) or a particular choice for
     *          the PIXEL coordinates (e.g. DECam) require this x-axis flip for their coordinate system to
     *          match our conventions.
     */
    bool getFocalPlaneParity() const;

    /**
     * Find the detectors that cover a point in any camera system
     *
     * @param[in] point  position to use in lookup (lsst::geom::Point2D)
     * @param[in] cameraSys  camera coordinate system of `point`
     * @returns a list of zero or more Detectors that overlap the specified point
     */
    DetectorList findDetectors(lsst::geom::Point2D const &point, CameraSys const &cameraSys) const;

    /**
     * Find the detectors that cover a list of points in any camera system
     *
     * @param[in] pointList  a list of points (lsst::geom::Point2D)
     * @param[in] cameraSys the camera coordinate system of the points in `pointList`
     * @returns a list of lists; each list contains the names of all detectors
     *    which contain the corresponding point
     */
    std::vector<DetectorList> findDetectorsList(std::vector<lsst::geom::Point2D> const &pointList,
                                                CameraSys const &cameraSys) const;

    /**
     * Get a transform from one CameraSys to another
     *
     * @param[in] fromSys  From CameraSys
     * @param[in] toSys  To CameraSys
     * @returns an afw::geom::TransformPoint2ToPoint2 that transforms from
     *    `fromSys` to `toSys` in the forward direction
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if no transform is
     *    available.
     * @throws KeyError if an unknown detector is specified
     */
    std::shared_ptr<afw::geom::TransformPoint2ToPoint2> getTransform(CameraSys const &fromSys,
                                                                     CameraSys const &toSys) const;

    /**
     * Obtain the transform registry.
     *
     * @returns _transformMap a TransformMap
     *
     * @note _transformMap is immutable, so this should be safe.
     */
    std::shared_ptr<TransformMap const> getTransformMap() const noexcept { return _transformMap; }

    /**
     * Transform a point from one camera coordinate system to another
     *
     * @param[in] point  an lsst::geom::Point2d
     * @param[in] fromSys  transform from this CameraSys
     * @param[in] toSys  transform to this CameraSys
     * @returns point transformed to `toSys` (an lsst::geom::Point2D)
     */
    lsst::geom::Point2D transform(lsst::geom::Point2D const &point, CameraSys const &fromSys,
                                  CameraSys const &toSys) const;

    /**
     * Transform a vector of points from one camera coordinate system to another
     *
     * @param[in] points  an vector of lsst::geom::Point2d
     * @param[in] fromSys  transform from this CameraSys
     * @param[in] toSys  transform to this CameraSys
     * @returns points transformed to `toSys` (a vector of lsst::geom::Point2D)
     */
    std::vector<lsst::geom::Point2D> transform(std::vector<lsst::geom::Point2D> const &points,
                                               CameraSys const &fromSys, CameraSys const &toSys) const;

    /**
     * Cameras are always persistable.
     */
    bool isPersistable() const noexcept override { return true; }

protected:
    void write(OutputArchiveHandle &handle) const override;

private:
    // static methods below provide access to private methods of
    // Detector::InCameraBuilder for use by Camera::Builder, because
    // Detector::InCameraBuilder can only friend Camera.

    /*
     * Create a new Detector::InCameraBuilder for a completely new Detector.
     */
    static std::shared_ptr<Detector::InCameraBuilder> makeDetectorBuilder(std::string const &name, int id);

    /*
     * Create a new Detector::InCameraBuilder with the state of the given
     * Detector.
     */
    static std::shared_ptr<Detector::InCameraBuilder> makeDetectorBuilder(Detector const &detector);

    /*
     * Extract the sequence of TransformMap::Connections from a
     * Detector::InCameraBuilder.
     */
    static std::vector<TransformMap::Connection> const &getDetectorBuilderConnections(
            Detector::InCameraBuilder const &detector);

    // Deserialization factory.
    class Factory;

    // Constructor used by Camera::Builder.
    // Some arguments passed by value to make moves possible.
    Camera(std::string const &name, DetectorList detectors, std::shared_ptr<TransformMap const> transformMap,
           std::string const &pupilFactoryName);

    // Constructor used by persistence.
    Camera(table::io::InputArchive const &archive, table::io::CatalogVector const &catalogs);

    std::string getPersistenceName() const override;

    // getPythonModule implementation inherited from DetectorCollection.

    std::string _name;
    std::string _pupilFactoryName;
    std::shared_ptr<TransformMap const> _transformMap;
};

/**
 * A helper class for creating and modifying cameras.
 *
 * Camera and Camera::Builder have no direct inheritance relationship, but both
 * inherit from different specializations of DetectorCollectionBase, so their
 * container-of-detectors interfaces can generally be used the same way in
 * both Python and templated C++.
 */
class Camera::Builder : public DetectorCollectionBase<Detector::InCameraBuilder> {
    using BaseCollection = DetectorCollectionBase<Detector::InCameraBuilder>;

public:
    virtual ~Builder() noexcept;

    /**
     * Construct a Builder for a completely new Camera with the given name.
     */
    explicit Builder(std::string const &name);

    /**
     * Construct a Builder with the state of an existing Camera.
     */
    explicit Builder(Camera const &camera);

    /**
     * Construct a new Camera from the state of the Builder.
     */
    std::shared_ptr<Camera const> finish() const;

    /// @copydoc Camera::getName
    std::string getName() const { return _name; }

    /// Set the name of the camera.
    void setName(std::string const &name) { _name = name; }

    /// @copydoc Camera::getPupilFactoryName
    std::string getPupilFactoryName() const { return _pupilFactoryName; }

    /// Set the fully-qualified name of the Python class that provides this Camera's PupilFactory.
    void setPupilFactoryName(std::string const &pupilFactoryName) { _pupilFactoryName = pupilFactoryName; }

    /**
     * Set the transformation from FOCAL_PLANE to the given coordinate system.
     *
     * @param toSys     Coordinate system this transform returns points in.
     * @param transform Transform from FOCAL_PLANE to `toSys`.
     *
     * If a transform already exists from FOCAL_PLANE to `toSys`, it is
     * overwritten.
     */
    void setTransformFromFocalPlaneTo(CameraSys const &toSys,
                                      std::shared_ptr<afw::geom::TransformPoint2ToPoint2 const> transform);

    /**
     * Remove any transformation from FOCAL_PLANE to the given coordinate system.
     *
     * @param  toSys Coordinate system this transform returns points in.
     * @return true if a transform was removed; false otherwise.
     */
    bool discardTransformFromFocalPlaneTo(CameraSys const &toSys);

    /**
     * Add a new Detector with the given name and ID.
     *
     * This is the only way to create a completely new detector (as opposed to
     * a copy of an existing one), and it permanently sets that Detector's name
     * and ID.
     *
     * @throw pex::exceptions::RuntimeError  Thrown if the ID and/or name
     *     conflict with those of detectors already in the collection.
     *
     * @exceptsafe  Strong for pex::exceptions::RuntimeError, none otherwise.
     */
    std::shared_ptr<Detector::InCameraBuilder> add(std::string const &name, int id);

    //@{
    /**
     * Remove the detector with the given name or ID.
     *
     * Wrapped as `__delitem__` in Python.
     *
     * @throws pex::exceptions::NotFoundError if no such detector exists.
     */
    void remove(std::string const &name) { return BaseCollection::remove(name); }
    void remove(int id) { return BaseCollection::remove(id); }
    //@}

private:
    std::string _name;
    std::string _pupilFactoryName;
    std::vector<TransformMap::Connection> _connections;
};

}  // namespace cameraGeom
}  // namespace afw
}  // namespace lsst

#endif  // LSST_AFW_CAMERAGEOM_CAMERA_H
