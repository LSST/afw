// -*- lsst-c++ -*-
/*
 * LSST Data Management System
 * Copyright 2008-2017 LSST Corporation.
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

#ifndef LSST_AFW_GEOM_TRANSFORM_H
#define LSST_AFW_GEOM_TRANSFORM_H

#include <memory>
#include <vector>

#include "astshim.h"
#include "Eigen/Core"
#include "ndarray.h"

#include "lsst/afw/geom/Endpoint.h"
#include "lsst/afw/table/io/Persistable.h"

namespace lsst {
namespace afw {
namespace geom {

class SkyWcs;

/**
 * Transform LSST spatial data, such as lsst::geom::Point2D and lsst::geom::SpherePoint, using an AST mapping.
 *
 * This class contains two Endpoints, to specify the "from" and "to" LSST data type,
 * and an ast::Mapping to specify the transformation.
 *
 * Depending on the ast::FrameSet or ast::Mapping used to define it, a Transform may
 * provide either a forward transform, an inverse transform, or both. In particular, the
 * @ref inverted "inverse" of a forward-only transform is an inverse-only transform. The
 * @ref hasForward and @ref hasInverse methods can be used to check which transforms are available.
 *
 * Unless otherwise stated, all constructors and methods may throw `std::runtime_error` to indicate
 * internal errors within AST.
 *
 * Transforms are always immutable.
 *
 * @note You gain some safety by constructing a Transform from an ast::FrameSet,
 * since the base and current frames in the FrameSet can be checked against by the appropriate endpoint.
 *
 * @note "In place" versions of `applyForward` and `applyInverse` are not available
 * because data must be copied when converting from LSST data types to the type used by astshim,
 * so it didn't seem worth the bother.
 */
template <class FromEndpoint, class ToEndpoint>
class Transform final : public table::io::Persistable {
using TransformType = Transform<FromEndpoint, ToEndpoint>;
DECLARE_PERSISTABLE_FACADE(TransformType);
    // SkyWcs is a friend so it can call a protected Transform constructor
    friend class SkyWcs;

public:
    using FromArray = typename FromEndpoint::Array;
    using FromPoint = typename FromEndpoint::Point;
    using ToArray = typename ToEndpoint::Array;
    using ToPoint = typename ToEndpoint::Point;

    Transform(Transform const &) = default;
    Transform(Transform &&) = default;
    Transform &operator=(Transform const &) = delete;
    Transform &operator=(Transform &&) = delete;
    ~Transform() override = default;

    /**
     * Construct a Transform from a deep copy of an ast::Mapping
     *
     * @param[in] mapping  ast::Mapping describing the desired transformation
     * @param[in] simplify  Simplify the mapping? This combines component mappings
     *                      where it is possible to do so without affecting accuracy.
     */
    explicit Transform(ast::Mapping const &mapping, bool simplify = true);

    /**
     * Construct a Transform from a deep copy of a ast::FrameSet
     *
     * The result transforms from the "base" frame to the "current" frame of the provided FrameSet.
     * The "from" endpoint is used to normalize the "base" frame
     * and the "to" endpoint is used to normalize the "current" frame.
     *
     * This is pickier than the constructor that takes an ast::Mapping in that:
     * - SpherePointEndpoint must be associated with an ast::SkyFrame and the SkyFrame axes
     *   are transposed, if necessary, to give the standard order: longitude, latitude.
     * - Point2Endpoint must be associated with an ast::Frame (not a subclass),
     *   because Frame is the only kind of Frame that is sure to be Cartesian.
     *
     * @param[in] frameSet  ast::FrameSet describing the desired transformation in the usual way:
     *                      from "base" frame to "current" frame
     * @param[in] simplify  Simplify the mapping? This combines component mappings
     *                      where it is possible to do so without affecting accuracy.
     */
    explicit Transform(ast::FrameSet const &frameSet, bool simplify = true);

    /**
     * Test if this method has a forward transform.
     *
     * @exceptsafe Provides basic exception safety.
     */
    bool hasForward() const { return _mapping->hasForward(); }

    /**
     * Test if this method has an inverse transform.
     *
     * @exceptsafe Provides basic exception safety.
     */
    bool hasInverse() const { return _mapping->hasInverse(); }

    /**
     * Get the "from" endpoint
     */
    FromEndpoint getFromEndpoint() const { return _fromEndpoint; }

    /**
     * Get the contained mapping
     */
    std::shared_ptr<const ast::Mapping> getMapping() const { return _mapping; }

    /**
     * Get the "to" endpoint
     */
    ToEndpoint getToEndpoint() const { return _toEndpoint; }

    /**
     * Transform one point in the forward direction ("from" to "to")
     */
    ToPoint applyForward(FromPoint const &point) const;

    /**
     * Transform an array of points in the forward direction ("from" to "to")
     *
     * The first dimension of the array must match the number of input axes, and the data order is
     * values for the first axis, then values for the next axis, and so on, e.g. for 2 axes:
     *     x0, x1, x2, ..., y0, y1, y2...
     */
    ToArray applyForward(FromArray const &array) const;

    /**
     * Transform one point in the inverse direction ("to" to "from")
     */
    FromPoint applyInverse(ToPoint const &point) const;

    /**
     * Transform an array of points in the inverse direction ("to" to "from")
     *
     * The first dimension of the array must match the number of output axes, and the data order is
     * values for the first axis, then values for the next axis, and so on, e.g. for 2 axes:
     *     x0, x1, x2, ..., y0, y1, y2...
     */
    FromArray applyInverse(ToArray const &array) const;

    /**
     * The inverse of this Transform.
     *
     * @returns a Transform whose `applyForward` is equivalent to this Transform's
     *          `applyInverse`, and vice versa.
     *
     * @exceptsafe Provides basic exception safety.
     */
    std::shared_ptr<Transform<ToEndpoint, FromEndpoint>> inverted() const;

    /**
     * The Jacobian matrix of this Transform.
     *
     * Radians are used for each axis of an SpherePointEndpoint.
     *
     * The matrix is defined only if this object has a forward transform.
     *
     * @param x the position at which the Jacobian shall be evaluated
     * @returns a matrix `J` with `getToEndpoint().getNAxes()` rows and
     *          `getFromEndpoint().getNAxes()` columns. `J(i,j)` shall be the
     *          rate of change of the `i`th output coordinate with respect to
     *          the `j`th input coordinate, or `NaN` if the derivative cannot
     *          be calculated.
     *
     * @exceptsafe Provides basic exception safety.
     *
     * @note The derivatives may be estimated by sampling and interpolating
     *       this Transform in the neighborhood of `x`. If the implementation
     *       requires interpolation, computation of the Jacobian may require
     *       hundreds of evaluations of @ref applyForward.
     */
    Eigen::MatrixXd getJacobian(FromPoint const &x) const;

    /**
     * Concatenate two Transforms.
     *
     * @tparam NextToEndpoint the "to" Endpoint of `next`
     * @param next the Transform to apply after this one
     * @param simplify if true then produce a transform containing a single simplified mapping
     *          with no intermediate frames.
     * @returns a Transform that first applies this transform to its input, and then
     *          `next` to the result. Its inverse shall first apply the
     *          inverse of `next`, and then the inverse of this transform.
     *
     * @throws pex::exceptions::InvalidParameterError Thrown if
     *         `getToEndpoint()` and `next.getFromEndpoint()` do not
     *         have the same number of axes.
     * @exceptsafe Provides basic exception safety.
     *
     * More than two Transforms can be combined in series. For example:
     *
     *     auto pixelsToSky = pixelsToFp.then(fpToField)->then(fieldToSky);
     */
    template <class NextToEndpoint>
    std::shared_ptr<Transform<FromEndpoint, NextToEndpoint>> then(
            Transform<ToEndpoint, NextToEndpoint> const &next, bool simplify = true) const;

    /**
     * Return a short version of the class name with no punctuation
     *
     * Used as the Python class name and for persistence as a string
     *
     * Returns "Transform" + fromEndpoint.getClassPrefix() + "To" + toEndpoint.getClassPrefix(),
     * for example "TransformPoint2ToSpherePoint" or "TransformPoint2ToGeneric".
     */
    static std::string getShortClassName();

    /**
     * Deserialize a Transform of this type from an input stream
     *
     * @param[in] is  input stream from which to deserialize this Transform
     */
    static std::shared_ptr<Transform<FromEndpoint, ToEndpoint>> readStream(std::istream &is);

    /// Deserialize a Transform of this type from a string, using the same format as readStream
    static std::shared_ptr<Transform<FromEndpoint, ToEndpoint>> readString(std::string &str);

    /**
     * Serialize this Transform to an output stream
     *
     * Version 1 format is as follows:
     * - The version number (an integer)
     * - A space
     * - The short class name, as obtained from getShortClassName
     * - A space
     * - The contained ast::FrameSet written using FrameSet.show(os, false)
     *
     * @param[out] os  outpu stream to which to serialize this Transform
     */
    void writeStream(std::ostream &os) const;

    /// Serialize this Transform to a string, using the same format as writeStream
    std::string writeString() const;

    /// Whether the Transform is persistable via afw::table::io (always true).
    bool isPersistable() const noexcept override { return true; }

protected:
    /**
     * Construct a Transform from a shared pointer to an ast::Mapping
     */
    explicit Transform(std::shared_ptr<ast::Mapping> mapping);

    // implement afw::table::io::Persistable interface
    std::string getPersistenceName() const override { return getShortClassName(); }

    // implement afw::table::io::Persistable interface
    std::string getPythonModule() const override { return "lsst.afw.geom"; }

    // implement afw::table::io::Persistable interface
    void write(OutputArchiveHandle &handle) const override;

private:
    FromEndpoint _fromEndpoint;
    std::shared_ptr<const ast::Mapping> _mapping;
    ToEndpoint _toEndpoint;
};

/**
 * Print a Transform to an ostream
 *
 * The format is "Transform<_fromEndpoint_, _toEndpoint_>"
 * where _fromEndpoint_ and _toEndpoint_ are the appropriate endpoint printed to the ostream;
 * for example "Transform<GenericEndpoint(4), Point2Endpoint()>"
 */
template <class FromEndpoint, class ToEndpoint>
std::ostream &operator<<(std::ostream &os, Transform<FromEndpoint, ToEndpoint> const &transform);

// typedefs for the most common transforms; names match Python names

using TransformPoint2ToPoint2 = Transform<Point2Endpoint, Point2Endpoint>;
using TransformPoint2ToGeneric = Transform<Point2Endpoint, GenericEndpoint>;
using TransformPoint2ToSpherePoint = Transform<Point2Endpoint, SpherePointEndpoint>;

}  // namespace geom
}  // namespace afw
}  // namespace lsst

#endif
