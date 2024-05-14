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
#if !defined(LSST_AFW_CAMERAGEOM_TRANSFORMMAP_H)
#define LSST_AFW_CAMERAGEOM_TRANSFORMMAP_H

#include <vector>
#include <unordered_map>
#include <memory>

#include "boost/iterator/transform_iterator.hpp"
#include "astshim/FrameSet.h"

#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/cameraGeom/CameraSys.h"
#include "lsst/geom/Point.h"
#include "lsst/afw/geom/Transform.h"

namespace lsst {
namespace afw {
namespace cameraGeom {

/**
 * A registry of 2-dimensional coordinate transforms for a specific camera.
 *
 * Represents the interrelationships between camera coordinate systems for a particular camera.
 * It can be seen as a mapping between a pair of CameraSys and a Transform between them
 * (though it does not conform to either the C++ map or Python dictionary APIs).
 *
 * TransformMap supports:
 * * Transforming between any two supported CameraSys using the @ref transform methods.
 * * Retrieving a transform between any two supported CameraSys using @ref getTransform.
 * * Iteration over supported CameraSys using @ref begin and @ref end in C++
 *   and standard Python iteration in Python.
 *
 * TransformMap is immutable and must always be held by shared_ptr; this is
 * enforced by making all non-deleted constructors private, and instead
 * providing static `make` member functions for construction (in Python, these
 * are exposed as regular constructors).
 *
 * @exceptsafe Unless otherwise specified, all methods guarantee only basic
 *             exception safety.
 */
class TransformMap final : public table::io::PersistableFacade<TransformMap>, public table::io::Persistable {
private:

    // Functor for boost::transform_iterator: given an entry in a std::map or unordered_map, return the key
    struct GetKey {
        CameraSys const &operator()(std::pair<const CameraSys, int> const &p) const { return p.first; };
    };

    using CameraSysFrameIdMap = std::unordered_map<CameraSys, int>;

public:

    using Transforms = std::unordered_map<CameraSys, std::shared_ptr<geom::TransformPoint2ToPoint2>>;
    using CameraSysIterator = boost::transform_iterator<GetKey, CameraSysFrameIdMap::const_iterator>;

    /**
     * Representation of a single edge in the graph defined by a TransformMap.
     */
    struct Connection final {
        std::shared_ptr<geom::TransformPoint2ToPoint2 const> transform;
        CameraSys fromSys;
        CameraSys toSys;

        /**
         * Reverse the connection, by swapping fromSys and toSys and inverting
         * the transform.
         */
        void reverse();
    };

    /**
     * Construct a TransformMap with all transforms relative to a single reference CameraSys.
     *
     * @param reference  Coordinate system from which each Transform in `transforms` converts.
     * @param transforms  A map whose keys are camera coordinate systems, and whose values
     *                    point to Transforms that convert from `reference` to the corresponding key.
     *                    All Transforms must be invertible.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError Thrown if `transforms` contains
     *         the `reference` camera system as a key, or if any Transform is not invertible.
     *
     * This method is wrapped as a regular constructor in Python.
     */
    static std::shared_ptr<TransformMap const> make(
        CameraSys const &reference,
        Transforms const &transforms
    );

    static void make(
            TransformMap *transformMap,
            CameraSys const & reference,
            Transforms const & transforms
    );

    /**
     * Construct a TransformMap from a sequence of Connections.
     *
     * @param reference    Initial coordinate system that must be connected
     *                     to all other coordinate systems in the map.
     * @param connections  Sequence of Connection structs, each of which relates
     *                     two CameraSys via the Transform that connects them.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError Thrown if the graph
     *         defined by the given connections does not define a single unique
     *         path (which may involve multiple Connections) between any
     *         CameraSys and the reference CameraSys.
     *
     * This method is wrapped as a regular constructor in Python.
     */
    static std::shared_ptr<TransformMap const> make(
        CameraSys const &reference,
        std::vector<Connection> const & connections
    );

    static void make(
            TransformMap *transformMap,
            CameraSys const &reference,
            std::vector<Connection> const & connections
    );

    ///@{
    /// TransformMap is immutable, so both moving and copying are prohibited.
    /// It is also always held by shared_ptr, so there is no good reason to
    /// copy it.
    TransformMap(TransformMap const &other) = delete;
    TransformMap(TransformMap &&other) = delete;
    TransformMap &operator=(TransformMap const &) = delete;
    TransformMap &operator=(TransformMap &&) = delete;
    ///@}

    ~TransformMap() noexcept;

    /**
     * Convert a point from one camera coordinate system to another.
     *
     * @param point  Point from which to transform
     * @param fromSys, toSys  Camera coordinate systems between which to transform
     * @returns the transformed value. Equivalent to
     *          `getTransform(fromSys, toSys).applyForward(point)`.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError Thrown if either
     *         `fromSys` or `toSys` is not supported.
     */
    lsst::geom::Point2D transform(lsst::geom::Point2D const &point, CameraSys const &fromSys,
                                  CameraSys const &toSys) const;

    /**
     * Convert a list of points from one coordinate system to another.
     *
     * @overload
     */
    std::vector<lsst::geom::Point2D> transform(std::vector<lsst::geom::Point2D> const &pointList,
                                               CameraSys const &fromSys, CameraSys const &toSys) const;

    CameraSysIterator begin() const { return boost::make_transform_iterator(_frameIds.begin(), GetKey()); }

    CameraSysIterator end() const { return boost::make_transform_iterator(_frameIds.end(), GetKey()); }

    /**
     * Can this transform to and from the specified coordinate system?
     *
     * @param system  The coordinate system to search for
     * @returns `true` if `system` is supported, `false` otherwise
     *
     * @exceptsafe Shall not throw exceptions.
     */
    bool contains(CameraSys const &system) const noexcept;

    /**
     * Get a Transform from one camera coordinate system to another.
     *
     * @param fromSys, toSys  Camera coordinate systems between which to transform
     * @returns a Transform that converts from `fromSys` to `toSys` in the forward direction.
     *      The Transform will be invertible.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError Thrown if either
     *         `fromSys` or `toSys` is not supported.
     */
    std::shared_ptr<geom::TransformPoint2ToPoint2> getTransform(CameraSys const &fromSys,
                                                                CameraSys const &toSys) const;

    /**
     * Get the number of supported coordinate systems.
     *
     * @exceptsafe Shall not throw exceptions.
     */
    size_t size() const noexcept;

    /**
     * Return the sequence of connections used to construct this Transform.
     *
     * The order of the connections may differ from the original order, but is
     * guaranteed to yield the same `TransformMap` when passed to `make` with
     * the same reference system.
     */
    std::vector<Connection> getConnections() const;

    /**
     * TransformMaps should always be Persistable.
     */
    bool isPersistable() const noexcept override { return true; }

private:

    // Helper class used in persistence.
    class Factory;

    // Private ctor, only called by `make` static methods and `Factory`.
    explicit TransformMap(std::vector<Connection> && connections);

    /*
     * Return the internal frame ID corresponding to a coordinate system.
     *
     * @param system  The system to convert.
     * @return the ID by which the coordinate system can be found in transforms.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError Thrown if `system`
     *         is not in frameIds.
     *
     * @exceptsafe Provides strong exception guarantee.
     */
    int _getFrame(CameraSys const &system) const;

    /*
     * Return ast::Mapping that transforms between two coordinate systems.
     *
     * @param fromSys, toSys  Coordinate systems between which to transform
     * @return an invertible Mapping that converts from `fromSys` to `toSys`
     *
     * @throws lsst::pex::exceptions::InvalidParameterError Thrown if either
     *         `fromSys` or `toSys` is not supported.
     */
    std::shared_ptr<ast::Mapping const> _getMapping(CameraSys const &fromSys, CameraSys const &toSys) const;

    std::string getPersistenceName() const override;

    std::string getPythonModule() const override;

    void write(OutputArchiveHandle& handle) const override;

    /*
     * Sequence of connections that define the edges of the graph.
     * Ordered by distance from the reference system.
     */
    std::vector<Connection> _connections;

    // Stores information on all relationships between Transforms.
    std::unique_ptr<ast::FrameSet> _frameSet;

    /*
     * Translates from LSST coordinate ID to AST frame ID.
     *
     * Must have exactly one mapping for each Frame in `transforms`.
     */
    CameraSysFrameIdMap _frameIds;

};


std::ostream & operator<<(std::ostream & os, TransformMap::Connection const & connection);

}  // namespace cameraGeom
}  // namespace afw
}  // namespace lsst

#endif
