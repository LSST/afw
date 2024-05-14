// -*- lsst-c++ -*-
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

#if !defined(LSST_AFW_CAMERAGEOM_DETECTOR_H)
#define LSST_AFW_CAMERAGEOM_DETECTOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include "lsst/base.h"
#include "lsst/afw/geom/Transform.h"
#include "lsst/afw/cameraGeom/Amplifier.h"
#include "lsst/afw/cameraGeom/CameraSys.h"
#include "lsst/afw/cameraGeom/Orientation.h"
#include "lsst/afw/cameraGeom/TransformMap.h"
#include "lsst/afw/typehandling/Storable.h"

namespace lsst {
namespace afw {
namespace cameraGeom {

/**
 * Type of imaging detector
 */
enum class DetectorType {
    SCIENCE,
    FOCUS,
    GUIDER,
    WAVEFRONT,
};


/**
 * An abstract base class that provides common accessors for Detector and
 * Detector::Builder.
 */
class DetectorBase : public typehandling::Storable {
public:

    using CrosstalkMatrix = ndarray::Array<float const, 2>;

    virtual ~DetectorBase() noexcept = default;

    /** Get the detector name. */
    std::string getName() const { return getFields().name; }

    /** Get the detector ID. */
    int getId() const { return getFields().id; }

    /** Return the purpose of this detector. */
    DetectorType getType() const { return getFields().type; }

    /** Get the detector serial "number". */
    std::string getSerial() const { return getFields().serial; }

    /**
     * Get the detector's physical type.
     *
     * This may mean different things for different cameras; possibilities
     * include the manufacturer ("ITL" vs "E2V") or fundamental technology
     * ("CCD" vs "HgCdTe").
     */
    std::string getPhysicalType() const { return getFields().physicalType; }

    /** Get the bounding box. */
    lsst::geom::Box2I getBBox() const { return getFields().bbox; }

    /** Get detector's orientation in the focal plane */
    Orientation getOrientation() const { return getFields().orientation; }

    /** Get size of pixel along (mm) */
    lsst::geom::Extent2D getPixelSize() const { return getFields().pixelSize; }

    /** Have we got crosstalk coefficients? */
    bool hasCrosstalk() const {
        return !(getFields().crosstalk.isEmpty() ||
                 getFields().crosstalk.getShape() == ndarray::makeVector(0, 0));
    }

    /** Get the crosstalk coefficients */
    CrosstalkMatrix getCrosstalk() const { return getFields().crosstalk; }

    /**
     * Get a coordinate system from a coordinate system (return input unchanged and untested)
     *
     * @param[in] cameraSys  Camera coordinate system
     * @return `cameraSys` unchanged
     *
     * @note the CameraSysPrefix version needs the detector name, which is why this is not static.
     */
    CameraSys makeCameraSys(CameraSys const &cameraSys) const { return cameraSys; }

    /**
     * Get a coordinate system from a detector system prefix (add detector name)
     *
     * @param[in] cameraSysPrefix  Camera coordinate system prefix
     * @return `cameraSysPrefix` with the detector name added
     */
    CameraSys makeCameraSys(CameraSysPrefix const &cameraSysPrefix) const {
        return CameraSys(cameraSysPrefix, getFields().name);
    }

    /// The "native" coordinate system of this detector.
    CameraSys getNativeCoordSys() const { return CameraSys(PIXELS, getName()); }

protected:

    // Simple struct containing all simple fields (everything not related
    // to coordinate systems/transforms or associated with an Amplifier).
    // See docs for corresponding getters (each field has one) for
    // descriptions.
    struct Fields {
        std::string name = "";
        int id = 0;
        DetectorType type = DetectorType::SCIENCE;
        std::string serial = "";
        lsst::geom::Box2I bbox;
        Orientation orientation;
        lsst::geom::Extent2D pixelSize;
        CrosstalkMatrix crosstalk;
        std::string physicalType;
    };

    //@{
    /**
     *  DetectorBase has no state, and is hence default-constructable,
     *  copyable, and movable.
     */
    DetectorBase() = default;
    DetectorBase(DetectorBase const &) = default;
    DetectorBase(DetectorBase &&) = default;
    DetectorBase & operator=(DetectorBase const &) = default;
    DetectorBase & operator=(DetectorBase &&) = default;
    //@}

    /**
     * Return a reference to a Fields struct.
     *
     * Must be implemented by all subclasses.
     */
    virtual Fields const & getFields() const = 0;

};


/**
 * A representation of a detector in a mosaic camera.
 *
 * Detector holds both simple data fields (see DetectorBase) and a set of
 * related coordinate systems and transforms, and acts as a container of
 * Amplifier objects.
 *
 * Detector is immutable, but copies can be modified via one of its Builder
 * classes.  A Detector must be created initially as part of a Camera
 * (see Camera::Builder::add), but can then be modified either individually
 * (see Detector::rebuild and Detector::PartialRebuilder) or as part of
 * modifying the full Camera (see Detector::InCameraBuilder).
 *
 * The coordinate systems and transforms known to a Detector are shared with
 * its parent Camera and all other Detectors in that Camera.
 */
class Detector final :
    public DetectorBase,
    public table::io::PersistableFacade<Detector>
{
public:

    class Builder;
    class PartialRebuilder;
    class InCameraBuilder;

    /**
     * Return a Builder object initialized with the state of this Detector.
     *
     * This is simply a shortcut for `Detector::PartialRebuilder(*this)`.
     */
    std::shared_ptr<PartialRebuilder> rebuild() const;

    /** Get the corners of the detector in the specified camera coordinate system */
    std::vector<lsst::geom::Point2D> getCorners(CameraSys const &cameraSys) const;

    /** Get the corners of the detector in the specified camera coordinate system prefix */
    std::vector<lsst::geom::Point2D> getCorners(CameraSysPrefix const &cameraSysPrefix) const;

    /** Get the center of the detector in the specified camera coordinate system */
    lsst::geom::Point2D getCenter(CameraSys const &cameraSys) const;

    /** Get the center of the detector in the specified camera coordinate system prefix */
    lsst::geom::Point2D getCenter(CameraSysPrefix const &cameraSysPrefix) const;

    /** Can this object convert between PIXELS and the specified camera coordinate system? */
    bool hasTransform(CameraSys const &cameraSys) const;

    /** Can this object convert between PIXELS and the specified camera coordinate system prefix? */
    bool hasTransform(CameraSysPrefix const &cameraSysPrefix) const;

    /**
     * Get a Transform from one camera coordinate system, or camera coordinate system prefix, to another.
     *
     * @tparam FromSysT, ToSysT  Type of `fromSys`, `toSys`: one of `CameraSys` or `CameraSysPrefix`
     *
     * @param fromSys, toSys camera coordinate systems or prefixes between which to transform
     * @returns a Transform that converts from `fromSys` to `toSys` in the forward direction.
     *      The Transform will be invertible.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError Thrown if either
     *         `fromSys` or `toSys` is not supported.
     */
    template <typename FromSysT, typename ToSysT>
    std::shared_ptr<afw::geom::TransformPoint2ToPoint2> getTransform(FromSysT const &fromSys,
                                                                     ToSysT const &toSys) const;

    /**
     * Transform a point from one camera system to another
     *
     * @tparam FromSysT  Class of fromSys: one of CameraSys or CameraSysPrefix
     * @tparam ToSysT  Class of toSys: one of CameraSys or CameraSysPrefix
     * @param[in] point  Camera point to transform
     * @param[in] fromSys  Camera coordinate system of `point`
     * @param[in] toSys  Camera coordinate system of returned point
     * @return The transformed point
     *
     * @throws pex::exceptions::InvalidParameterError if fromSys or toSys is unknown
     */
    template <typename FromSysT, typename ToSysT>
    lsst::geom::Point2D transform(lsst::geom::Point2D const &point, FromSysT const &fromSys,
                                  ToSysT const &toSys) const;

    /**
     * Transform a vector of points from one camera system to another
     *
     * @tparam FromSysT  Class of fromSys: one of CameraSys or CameraSysPrefix
     * @tparam ToSysT  Class of toSys: one of CameraSys or CameraSysPrefix
     * @param[in] points  Camera points to transform
     * @param[in] fromSys  Camera coordinate system of `points`
     * @param[in] toSys  Camera coordinate system of returned points
     * @return The transformed points
     *
     * @throws pex::exceptions::InvalidParameterError if fromSys or toSys is unknown
     */
    template <typename FromSysT, typename ToSysT>
    std::vector<lsst::geom::Point2D> transform(std::vector<lsst::geom::Point2D> const &points,
                                               FromSysT const &fromSys, ToSysT const &toSys) const;


    /** Get the transform registry */
    std::shared_ptr<TransformMap const> getTransformMap() const { return _transformMap; }

    /// Return the sequence of Amplifiers directly.
    std::vector<std::shared_ptr<Amplifier const>> const & getAmplifiers() const { return _amplifiers; }

    //@{
    /**
     * An iterator range over amplifers.
     *
     * Iterators dereference to `shared_ptr<Amplifier const>`.
     */
    auto begin() const { return _amplifiers.begin(); }
    auto end() const { return _amplifiers.end(); }
    //}

    /**
     * Get the amplifier specified by index.
     *
     * @throws std::out_of_range if index is out of range.
     */
    std::shared_ptr<Amplifier const> operator[](size_t i) const { return _amplifiers.at(i); }

    /**
     * Get the amplifier specified by name.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if no such amplifier.
     */
    std::shared_ptr<Amplifier const> operator[](std::string const &name) const;

    /**
     * Get the number of amplifiers. Renamed to `__len__` in Python.
     */
    std::size_t size() const { return _amplifiers.size(); }

    /// Detector is always persistable.
    bool isPersistable() const noexcept override { return true; }

protected:

    Fields const & getFields() const override { return _fields; }

private:

    class Factory;

    // Pass fields by value to move when we can and copy when we can't;
    // pass amplifiers by rvalue ref because we always move those.
    Detector(Fields fields, std::shared_ptr<TransformMap const> transformMap,
             std::vector<std::shared_ptr<Amplifier const>> && amplifiers);

    std::string getPersistenceName() const override;

    std::string getPythonModule() const override;

    void write(OutputArchiveHandle& handle) const override;

    Fields const _fields;
    std::shared_ptr<TransformMap const> _transformMap;
    // Given that the number of amplifiers in a detector is generally quite
    // small (even LSST only has 16), we just use a vector and do linear
    // searches for name lookups, as adding a map of some kind is definitely
    // more storage and code complexity, without necessarily being any faster.
    std::vector<std::shared_ptr<Amplifier const>> _amplifiers;
};


/**
 * A helper class for Detector that allows amplifiers and most fields to be
 * modified.
 *
 * Because Detector is immutable, creation and modification always go through
 * Builder, or more precisely, one of its two subclasses:
 *  - InCameraBuilder (obtained from Camera::Builder) is used to construct
 *    new detectors or modify them as part of Camera.
 *  - PartialRebuilder (obtained from Detector::rebuild) is used to modify
 *    an existing detector without changing its relationship to its camera.
 *
 * Detector::Builder itself provides functionality common to these:
 *  - setters for the simple data fields of Detector;
 *  - a container of Amplifier::Builders.
 * It is not intended define an interface independent of its subclasses.
 *
 * The name and ID of a detector (but not its "serial" string) are set at
 * initial construction and are an integral part of the relationship between
 * it and its Camera, and can never be changed, even by Builders.
 *
 * The fact that Amplifier::Builder inherits from Amplifier does not mean that
 * a container of Amplifier::Builder can inherit from a container of Amplifier,
 * and hence Detector::Builder (which has a container of Amplifer::Builder)
 * cannot inherit directly from Detector (which has a container of Amplifier).
 * But in both Python and templated C++ code, the container interfaces of
 * Detector and Detector::Builder are identical (i.e. they're "duck type"
 * equivalent), aside from the fact that Detector::Builder also permits
 * addition and removal of amplifiers.
 */
class Detector::Builder : public DetectorBase {
public:

    // Builder's current subclasses have no need for copy or assignment, and
    // the hierarchy should be considered closed.
    Builder(Builder const &) = delete;
    Builder(Builder &&) = delete;
    Builder & operator=(Builder const &) = delete;
    Builder & operator=(Builder &&) = delete;

    ~Builder() noexcept override = 0;

    /** Set the bounding box */
    void setBBox(lsst::geom::Box2I const & bbox) { _fields.bbox = bbox; }

    /** Set the purpose of this detector. */
    void setType(DetectorType type) { _fields.type = type; }

    /** Set the detector serial "number". */
    void setSerial(std::string const & serial) { _fields.serial = serial; }

    /**
     * Set the detector's physical type.
     *
     * @copydetails DetectorBase::getPhysicalType
     */
    void setPhysicalType(std::string const & physicalType) { _fields.physicalType = physicalType; }

    /**
     * Set the crosstalk coefficients.
     *
     * The shape of the crosstalk matrix must be consistent with the set of
     * amplifiers, but is not checked until a Detector instance is actually
     * constructed.
     *
     * Setting with a zero-size matrix is equivalent to calling
     * `unsetCrosstalk()`.
     */
    void setCrosstalk(CrosstalkMatrix const & crosstalk) { _fields.crosstalk = crosstalk; }

    /// Remove the crosstalk coefficient matrix.
    void unsetCrosstalk() { _fields.crosstalk = CrosstalkMatrix(); }

    // We return non-const Amplifier::Builder objects by shared_ptr, via const
    // methods.  That's a bit counterintuitive, but there's no solution to the
    // problem of constness in containers of pointers in C++ that *is*
    // intuitive. The alternative would be to have const methods that return
    // pointer-to-const and non-const methods that return
    // pointer-to-not-const.  The only gain from that would be that it'd be
    // possible to have a const reference to a Detector::Builder that would
    // prevent modifications to its amplifiers.  That's a lot more code
    // (writing the iterators would be especially unpleasant) for essentially
    // no gain, because users who want to prevent changes to the amplifiers
    // essentially always Detector itself, not Detector::Builder.

    /// Return the sequence of Amplifier::Builders directly.
    std::vector<std::shared_ptr<Amplifier::Builder>> const & getAmplifiers() const { return _amplifiers; }

    //@{
    /**
     * An iterator range over amplifers.
     *
     * Iterators dereference to `shared_ptr<Amplifier::Builder>`.
     */
    auto begin() { return _amplifiers.begin(); }
    auto end() { return _amplifiers.end(); }
    //@}

    /**
     * Get the amplifier builder specified by index
     *
     * @throws std::out_of_range if index is out of range.
     */
    std::shared_ptr<Amplifier::Builder> operator[](size_t i) const { return _amplifiers.at(i); }

    /**
     * Get a builder for the amplifier specified by name.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if no such amplifier.
     */
    std::shared_ptr<Amplifier::Builder> operator[](std::string const &name) const;

    /// Append a new amplifier.
    void append(std::shared_ptr<Amplifier::Builder> builder);

    /// Remove all amplifiers.
    void clear() { _amplifiers.clear(); }

    /// Return the number of amplifiers (renamed to __len__ in Python).
    std::size_t size() const { return _amplifiers.size(); }

protected:

    /**
     * Create a vector of Amplifier::Builders from the Amplifiers in a
     * Detector.
     */
    static std::vector<std::shared_ptr<Amplifier::Builder>> rebuildAmplifiers(Detector const & detector);

    /**
     * Construct a Detector::Builder with no amplifiers and the given name and
     * ID.
     */
    Builder(std::string const & name, int id);

    /**
     * Construct a Detector::Builder with the given field values and
     * amplifiers.
     */
    Builder(Fields fields, std::vector<std::shared_ptr<Amplifier::Builder>> && amplifiers) :
        _fields(std::move(fields)),
        _amplifiers(std::move(amplifiers))
    {}

    Fields const & getFields() const override { return _fields; }

    /**
     * Create a vector of Amplifiers from the Amplifier::Builder sequence.
     */
    std::vector<std::shared_ptr<Amplifier const>> finishAmplifiers() const;

    /**
     * Set the orientation of the detector in the focal plane.
     *
     * This is intended for use by InCameraBuilder only; the orientation is
     * used to set the coordinate transform from FOCAL_PLANE to PIXELS, and
     * hence cannot be modified unless the full Camera is being modified.
     */
    void setOrientation(Orientation const & orientation) { _fields.orientation = orientation; }

    /**
     * Set the pixel size (in mm).
     *
     * This is intended for use by InCameraBuilder only; the pixel size is
     * used to set the coordinate transform from FOCAL_PLANE to PIXELS, and
     * hence cannot be modified unless the full Camera is being modified.
     */
    void setPixelSize(lsst::geom::Extent2D const & pixelSize) { _fields.pixelSize = pixelSize; }

private:
    Fields _fields;
    std::vector<std::shared_ptr<Amplifier::Builder>> _amplifiers;
};


/**
 * A helper class that allows the properties of a single detector to be
 * modified in isolation.
 *
 * Detector::PartialRebuilder can be used without access to the Camera
 * instance the Detector was originally a part of (such as when the Detector
 * was obtained from an Exposure or ExposureRecord).  As this always creates a
 * new Detector, the original Camera is never updated.  PartialRebuilder
 * prohibits changes to coordinate systems and transforms (including the
 * orientation and pixel size fields that are used to define some transforms),
 * as these cannot be done self-consistently without access to the full
 * Camera.
 */
class Detector::PartialRebuilder final : public Detector::Builder {
public:

    /**
     * Construct a PartialRebuilder initialized to the state of the given
     * Detector.
     */
    PartialRebuilder(Detector const & detector);

    /**
     *  Construct a new Detector from the current state of the Builder.
     */
    std::shared_ptr<Detector const> finish() const;

private:
    std::shared_ptr<TransformMap const> _transformMap;
};

/**
 * A helper class that allows the properties of a detector to be modified
 * in the course of modifying a full camera.
 *
 * Detector::InCameraBuilder can only be constructed via Camera::Builder, and
 * all Detector::InCameraBuilder instances should always be owned by or shared
 * with a Camera::Builder.
 *
 * Unlike Detector::PartialRebuilder, InCameraBuilder can be used to set the
 * orientation, pixel size, and more general coordinate systems associated
 * with the detector.
 *
 * The transformation from FOCAL_PLANE to PIXELS that relates this detector's
 * coordinate systems to those of the full camera and other detectors is
 * created from the orientation and pixel size fields, and need not (and
 * cannot) be set explicitly.
 */
class Detector::InCameraBuilder final : public Detector::Builder {
public:

    /**
     * Set the orientation of the detector in the focal plane.
     */
    void setOrientation(Orientation const & orientation) { Detector::Builder::setOrientation(orientation); }

    /**
     * Set the pixel size (in mm).
     */
    void setPixelSize(lsst::geom::Extent2D const & pixelSize) { Detector::Builder::setPixelSize(pixelSize); }

    /**
     * Set the transformation from PIXELS to the given coordinate system.
     *
     * @param toSys     Coordinate system prefix this transform returns points
     *                  in.
     * @param transform Transform from PIXELS to `toSys`.
     *
     * If a transform already exists from PIXELS to `toSys`, it is overwritten.
     */
    void setTransformFromPixelsTo(
        CameraSysPrefix const & toSys,
        std::shared_ptr<afw::geom::TransformPoint2ToPoint2 const> transform
    );

    /**
     * Set the transformation from PIXELS to the given coordinate system.
     *
     * @param toSys     Coordinate system prefix this transform returns points
     *                  in.  Must be associated with this detector.
     * @param transform Transform from PIXELS to `toSys`.
     *
     * If a transform already exists from PIXELS to `toSys`, it is overwritten.
     *
     * @throws pex::exceptions::InvalidParameterError if
     *     `toSys.getDetectorName() != this->getName()`.
     */
    void setTransformFromPixelsTo(
        CameraSys const & toSys,
        std::shared_ptr<afw::geom::TransformPoint2ToPoint2 const> transform
    );

    /**
     * Remove any transformation from PIXELS to the given coordinate system.
     *
     * @param  toSys Coordinate system prefix this transform returns points
     *               in.
     * @return true if a transform was removed; false otherwise.
     */
    bool discardTransformFromPixelsTo(CameraSysPrefix const & toSys);

    /**
     * Remove any transformation from PIXELS to the given coordinate system.
     *
     * @param  toSys Coordinate system prefix this transform returns points
     *               in.  Must be associated with this detector.
     * @return true if a transform was removed; false otherwise.
     *
     * @throws pex::exceptions::InvalidParameterError if
     *     `toSys.getDetectorName() != this->getName()`.
     */
    bool discardTransformFromPixelsTo(CameraSys const & toSys);

    /**
     * Remove all coordinate transforms.
     */
    void clearTransforms() { _connections.clear(); }

private:

    // We'd really like to friend Camera::Builder, but can't forward declare
    // an inner class.  So instead we friend Camera, and it has static members
    // that make the needed functionality accessible to Camera::Builder.
    friend class Camera;

    // Construct from an existing Detector.  For use only by Camera.
    InCameraBuilder(Detector const & detector);

    // Construct a completely new detector.  For use only by Camera.
    InCameraBuilder(std::string const & name, int id);

    // Construct a Detector from the builder.  For use only by Camera.
    //
    // @param[in] transformMap  All transforms known to the entire Camera.
    //
    std::shared_ptr<Detector const> finish(std::shared_ptr<TransformMap const> transformMap) const;

    // Transforms and coordinate systems that are specific to this detector.
    // This does not include the FOCAL_PLANE<->PIXELS connection, as that's
    // derived from the orientation and pixel size.
    std::vector<TransformMap::Connection> _connections;
};

}  // namespace cameraGeom
}  // namespace afw
}  // namespace lsst

#endif
