// -*- LSST-C++ -*-

/*
 * LSST Data Management System
 * Copyright 2008-2014 LSST Corporation.
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

#if !defined(LSST_AFW_GEOM_POLYGON_POLYGON_H)
#define LSST_AFW_GEOM_POLYGON_POLYGON_H

#include <vector>
#include <utility>  // for std::pair

#include <memory>

#include "lsst/base.h"
#include "lsst/pex/exceptions.h"
#include "lsst/afw/geom/Box.h"
#include "lsst/afw/geom/Point.h"
#include "lsst/afw/geom/AffineTransform.h"
#include "lsst/afw/geom/Transform.h"
#include "lsst/afw/geom/XYTransform.h"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"

namespace lsst {
namespace afw {
namespace geom {
namespace polygon {

/// An exception that indicates the single-polygon assumption has been violated
///
/// The single-polygon assumption is used in Polygon::intersectionSingle and
/// Polygon::unionSingle.
LSST_EXCEPTION_TYPE(SinglePolygonException, lsst::pex::exceptions::RuntimeError,
                    lsst::afw::geom::polygon::SinglePolygonException);

/// Cartesian polygons
///
/// Polygons are defined by a set of vertices

class Polygon : public afw::table::io::PersistableFacade<Polygon>, public afw::table::io::Persistable {
public:
    typedef Box2D Box;
    typedef Point2D Point;

    //@{
    /// Constructors
    explicit Polygon(Box const& box);
    Polygon(Box const& box,                           ///< Box to convert to polygon
            TransformPoint2ToPoint2 const& transform  ///< Transform from original to target frame
            );
    Polygon(Box const& box,                                      ///< Box to convert to polygon
            std::shared_ptr<XYTransform const> const& transform  ///< Transform from original to target frame
            );
    Polygon(Box const& box,                   ///< Box to convert to polygon
            AffineTransform const& transform  ///< Transform from original to target frame
            );
    explicit Polygon(std::vector<Point> const& vertices);
    //@}

    /// Swap two polygons
    void swap(Polygon& other) { std::swap(this->_impl, other._impl); }

    /// Return number of edges
    ///
    /// Identical with the number of points
    size_t getNumEdges() const;

    /// Return bounding box
    Box getBBox() const;

    Point calculateCenter() const;
    double calculateArea() const;
    double calculatePerimeter() const;

    /// Get vector of vertices
    ///
    /// Note that the "closed" polygon vertices are returned, so the first and
    /// last vertex are identical and there is one more vertex than otherwise
    /// expected.
    std::vector<Point> getVertices() const;

    //@{
    /// Iterator for vertices
    ///
    /// Iterates only over the "open" polygon vertices (i.e., same number as
    /// returned by "getNumEdges").
    std::vector<Point>::const_iterator begin() const;
    std::vector<Point>::const_iterator end() const;
    //@}

    /// Get vector of edges
    ///
    /// Returns edges, as pairs of vertices.
    std::vector<std::pair<Point, Point>> getEdges() const;

    bool operator==(Polygon const& other) const;
    bool operator!=(Polygon const& other) const { return !(*this == other); }

    /// Returns whether the polygon contains the point
    bool contains(Point const& point) const;

    //@{
    /// Returns whether the polygons overlap each other
    ///
    /// Note that there may be no intersection if the polygons
    /// only share a boundary.
    bool overlaps(Polygon const& other) const;
    bool overlaps(Box const& box) const;
    //@}

    //@{
    /// Returns the intersection of two polygons
    ///
    /// Does not handle non-convex polygons (which might have multiple independent
    /// intersections), and is provided as a convenience for when the polygons are
    /// known to be convex (e.g., image borders) and overlapping.
    std::shared_ptr<Polygon> intersectionSingle(Polygon const& other) const;
    std::shared_ptr<Polygon> intersectionSingle(Box const& box) const;
    //@}

    //@{
    /// Returns the intersection of two polygons
    ///
    /// Handles the full range of possibilities.
    std::vector<std::shared_ptr<Polygon>> intersection(Polygon const& other) const;
    std::vector<std::shared_ptr<Polygon>> intersection(Box const& box) const;
    //@}

    //@{
    /// Returns the union of two polygons
    ///
    /// Does not handle non-overlapping polygons, the union of which would be
    /// disjoint.
    std::shared_ptr<Polygon> unionSingle(Polygon const& other) const;
    std::shared_ptr<Polygon> unionSingle(Box const& box) const;
    //@}

    //@{
    /// Returns the union of two polygons
    ///
    /// Handles the full range of possibilities.
    ///
    /// Note the trailing underscore in C++, due to "union" being a reserved word.
    std::vector<std::shared_ptr<Polygon>> union_(Polygon const& other) const;
    std::vector<std::shared_ptr<Polygon>> union_(Box const& box) const;
    //@}

    //@{
    /// Return the symmetric difference of two polygons
    std::vector<std::shared_ptr<Polygon>> symDifference(Polygon const& other) const;
    std::vector<std::shared_ptr<Polygon>> symDifference(Box const& box) const;
    //@}

    /// Return a simplified polygon
    ///
    /// Removes unnecessary points (using the Douglas-Peucker algorithm).
    std::shared_ptr<Polygon> simplify(double const distance) const;

    //@{
    /// Operators for syntactic sugar
    std::vector<std::shared_ptr<Polygon>> operator&(Polygon const& rhs) const { return intersection(rhs); }
    std::vector<std::shared_ptr<Polygon>> operator&(Box const& rhs) const { return intersection(rhs); }
    std::vector<std::shared_ptr<Polygon>> operator|(Polygon const& rhs) const { return union_(rhs); }
    std::vector<std::shared_ptr<Polygon>> operator|(Box const& rhs) const { return union_(rhs); }
    std::vector<std::shared_ptr<Polygon>> operator^(Polygon const& rhs) const { return symDifference(rhs); }
    std::vector<std::shared_ptr<Polygon>> operator^(Box const& rhs) const { return symDifference(rhs); }
    //@}

    /// Produce a polygon from the convex hull
    std::shared_ptr<Polygon> convexHull() const;

    //@{
    /// Transform the polygon
    ///
    /// The transformation is only applied to the vertices.  If the transformation
    /// is non-linear, the edges will not reflect that, but simply join the vertices.
    /// Greater fidelity might be achieved by using "subSample" before transforming.
    std::shared_ptr<Polygon> transform(
            TransformPoint2ToPoint2 const& transform  ///< Transform from original to target frame
            ) const;
    std::shared_ptr<Polygon> transform(
            std::shared_ptr<XYTransform const> const& transform  ///< Transform from original to target frame
            ) const;
    std::shared_ptr<Polygon> transform(
            AffineTransform const& transform  ///< Transform from original to target frame
            ) const;
    //@}

    //@{
    /// Sub-sample each edge
    ///
    /// This should provide greater fidelity when transforming with a non-linear transform.
    std::shared_ptr<Polygon> subSample(size_t num) const;
    std::shared_ptr<Polygon> subSample(double maxLength) const;
    //@}

    //@{
    /// Create image of polygon
    ///
    /// Pixels entirely contained within the polygon receive value unity,
    /// pixels entirely outside the polygon receive value zero, and pixels
    /// on the border receive a value equal to the fraction of the pixel
    /// within the polygon.
    ///
    /// Note that the center of the lower-left pixel is 0,0.
    std::shared_ptr<afw::image::Image<float>> createImage(Box2I const& bbox) const;
    std::shared_ptr<afw::image::Image<float>> createImage(Extent2I const& extent) const {
        return createImage(Box2I(Point2I(0, 0), extent));
    }
    //@}

    //@{
    /// Whether Polygon is persistable which is always true
    virtual bool isPersistable() const { return true; }

protected:
    virtual std::string getPersistenceName() const;

    virtual void write(OutputArchiveHandle& handle) const;

private:
    //@{
    /// pImpl pattern to hide implementation
    struct Impl;
    std::shared_ptr<Impl> _impl;
    Polygon(std::shared_ptr<Impl> impl) : _impl(impl) {}
    //@}
};

/// Stream polygon
std::ostream& operator<<(std::ostream& os, Polygon const& poly);
}
}
}
}  // namespace lsst::afw::geom::polygon

#endif
