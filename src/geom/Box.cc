/*
 * LSST Data Management System
 * See the COPYRIGHT and LICENSE files in the top-level directory of this
 * package for notices and licensing terms.
 */

#include <cmath>
#include <limits>

#include "lsst/utils/ieee.h"
#include "lsst/afw/geom/Box.h"

namespace geom = lsst::afw::geom;

geom::Box2I::Box2I(Point2I const & minimum, Point2I const & maximum, bool invert) :
    _minimum(minimum), _dimensions(maximum - minimum)
{
    for (int n=0; n<2; ++n) {
        if (_dimensions[n] < 0) {
            if (invert) {
                _minimum[n] += _dimensions[n];
                _dimensions[n] = -_dimensions[n];
            } else {
                *this = Box2I();
                return;
            }
        }
    }
    _dimensions += Extent2I(1);
}

geom::Box2I::Box2I(Point2I const & minimum, Extent2I const & dimensions, bool invert) :
    _minimum(minimum), _dimensions(dimensions)
{
    for (int n=0; n<2; ++n) {
        if (_dimensions[n] == 0) {
            *this = Box2I();
            return;
        } else if (_dimensions[n] < 0) {
            if (invert) {
                _minimum[n] += (_dimensions[n] + 1);
                _dimensions[n] = -_dimensions[n];
            } else {
                *this = Box2I();
                return;
            }
        }
    }
    if (!isEmpty() && any(getMin().gt(getMax()))) {
        throw LSST_EXCEPT(
            pex::exceptions::OverflowError,
            "Box dimensions too large; integer overflow detected."
        );
    }
}

geom::Box2I::Box2I(Box2D const & other, EdgeHandlingEnum edgeHandling) : _minimum(), _dimensions() {
    if (other.isEmpty()) {
        *this = Box2I();
        return;
    }
    if (!utils::isfinite(other.getMinX()) || !utils::isfinite(other.getMinY())
        || !utils::isfinite(other.getMaxX()) || !utils::isfinite(other.getMaxY())) {
        throw LSST_EXCEPT(
            pex::exceptions::InvalidParameterError,
            "Cannot convert non-finite Box2D to Box2I"
        );
    }
    Point2D fpMin(other.getMin() + Extent2D(0.5));
    Point2D fpMax(other.getMax() - Extent2D(0.5));
    switch (edgeHandling) {
    case EXPAND:
        for (int n=0; n<2; ++n) {
            _minimum[n] = static_cast<int>(std::floor(fpMin[n]));
            _dimensions[n] = static_cast<int>(std::ceil(fpMax[n])) + 1 - _minimum[n];
        }
        break;
    case SHRINK:
        for (int n=0; n<2; ++n) {
            _minimum[n] = static_cast<int>(std::ceil(fpMin[n]));
            _dimensions[n] = static_cast<int>(std::floor(fpMax[n])) + 1 - _minimum[n];
        }
        break;
    }
}

ndarray::View< boost::fusion::vector2<ndarray::index::Range,ndarray::index::Range> >
geom::Box2I::getSlices() const {
    return ndarray::view(getBeginY(), getEndY())(getBeginX(), getEndX());
}

bool geom::Box2I::contains(Point2I const & point) const {
    return all(point.ge(this->getMin())) && all(point.le(this->getMax()));
}

bool geom::Box2I::contains(Box2I const & other) const {
    return other.isEmpty() ||
        (all(other.getMin().ge(this->getMin())) && all(other.getMax().le(this->getMax())));
}

bool geom::Box2I::overlaps(Box2I const & other) const {
    return !(
        other.isEmpty() || this->isEmpty()
        || any(other.getMax().lt(this->getMin()))
        || any(other.getMin().gt(this->getMax()))
    );
}

void geom::Box2I::grow(Extent2I const & buffer) {
    if (isEmpty()) return; // should we throw an exception here instead of a no-op?
    _minimum -= buffer;
    _dimensions += buffer * 2;
    if (any(_dimensions.le(0))) *this = Box2I();
}

void geom::Box2I::shift(Extent2I const & offset) {
    if (isEmpty()) return; // should we throw an exception here instead of a no-op?
    _minimum += offset;
}

void geom::Box2I::flipLR(int xextent) {
    if (isEmpty()) return; // should we throw an exception here instead of a no-op?
    // Apply flip about y-axis assumine parent coordinate system
    _minimum[0] = xextent - (_minimum[0] + _dimensions[0]);
    // _dimensions should remain unchanged
}

void geom::Box2I::flipTB(int yextent) {
    if (isEmpty()) return; // should we throw an exception here instead of a no-op?
    // Apply flip about y-axis assumine parent coordinate system
    _minimum[1] = yextent - (_minimum[1] + _dimensions[1]);
    // _dimensions should remain unchanged
}

void geom::Box2I::include(Point2I const & point) {
    if (isEmpty()) {
        _minimum = point;
        _dimensions = Extent2I(1);
        return;
    }
    Point2I maximum(getMax());
    for (int n=0; n<2; ++n) {
        if (point[n] < _minimum[n]) {
            _minimum[n] = point[n];
        } else if (point[n] > maximum[n]) {
            maximum[n] = point[n];
        }
    }
    _dimensions = Extent2I(1) + maximum - _minimum;
}

void geom::Box2I::include(Box2I const & other) {
    if (other.isEmpty()) return;
    if (this->isEmpty()) {
        *this = other;
        return;
    }
    Point2I maximum(getMax());
    Point2I const & otherMin = other.getMin();
    Point2I const otherMax = other.getMax();
    for (int n=0; n<2; ++n) {
        if (otherMin[n] < _minimum[n]) {
            _minimum[n] = otherMin[n];
        }
        if (otherMax[n] > maximum[n]) {
            maximum[n] = otherMax[n];
        }
    }
    _dimensions = Extent2I(1) + maximum - _minimum;
}

void geom::Box2I::clip(Box2I const & other) {
    if (isEmpty()) return;
    if (other.isEmpty()) {
        *this = Box2I();
        return;
    }
    Point2I maximum(getMax());
    Point2I const & otherMin = other.getMin();
    Point2I const otherMax = other.getMax();
    for (int n=0; n<2; ++n) {
        if (otherMin[n] > _minimum[n]) {
            _minimum[n] = otherMin[n];
        }
        if (otherMax[n] < maximum[n]) {
            maximum[n] = otherMax[n];
        }
    }
    if (any(maximum.lt(_minimum))) {
        *this = Box2I();
        return;
    }
    _dimensions = Extent2I(1) + maximum - _minimum;
}

bool geom::Box2I::operator==(Box2I const & other) const {
    return other._minimum == this->_minimum && other._dimensions == this->_dimensions;
}

bool geom::Box2I::operator!=(Box2I const & other) const {
    return other._minimum != this->_minimum || other._dimensions != this->_dimensions;
}

std::vector<geom::Point2I> geom::Box2I::getCorners() const {
    std::vector<geom::Point2I> retVec;
    retVec.push_back(getMin());
    retVec.push_back(geom::Point2I(getMaxX(), getMinY()));
    retVec.push_back(getMax());
    retVec.push_back(geom::Point2I(getMinX(), getMaxY()));
    return retVec;
}

double const geom::Box2D::EPSILON = std::numeric_limits<double>::epsilon()*2;

double const geom::Box2D::INVALID = std::numeric_limits<double>::quiet_NaN();

geom::Box2D::Box2D() : _minimum(INVALID), _maximum(INVALID) {}

geom::Box2D::Box2D(Point2D const & minimum, Point2D const & maximum, bool invert) :
    _minimum(minimum), _maximum(maximum)
{
    for (int n=0; n<2; ++n) {
        if (_minimum[n] == _maximum[n]) {
            *this = Box2D();
            return;
        } else if (_minimum[n] > _maximum[n]) {
            if (invert) {
                std::swap(_minimum[n],_maximum[n]);
            } else {
                *this = Box2D();
                return;
            }
        }
    }
}

geom::Box2D::Box2D(Point2D const & minimum, Extent2D const & dimensions, bool invert) :
    _minimum(minimum), _maximum(minimum + dimensions)
{
    for (int n=0; n<2; ++n) {
        if (_minimum[n] == _maximum[n]) {
            *this = Box2D();
            return;
        } else if (_minimum[n] > _maximum[n]) {
            if (invert) {
                std::swap(_minimum[n],_maximum[n]);
            } else {
                *this = Box2D();
                return;
            }
        }
    }
}

geom::Box2D::Box2D(Box2I const & other) :
    _minimum(Point2D(other.getMin()) - Extent2D(0.5)),
    _maximum(Point2D(other.getMax()) + Extent2D(0.5))
{
    if (other.isEmpty()) *this = Box2D();
}

bool geom::Box2D::contains(Point2D const & point) const {
    return all(point.ge(this->getMin())) && all(point.lt(this->getMax()));
}

bool geom::Box2D::contains(Box2D const & other) const {
    return other.isEmpty() ||
        (all(other.getMin().ge(this->getMin())) && all(other.getMax().le(this->getMax())));
}

bool geom::Box2D::overlaps(Box2D const & other) const {
    return !(
        other.isEmpty() || this->isEmpty()
        || any(other.getMax().le(this->getMin()))
        || any(other.getMin().ge(this->getMax()))
    );
}

void geom::Box2D::grow(Extent2D const & buffer) {
    if (isEmpty()) return; // should we throw an exception here instead of a no-op?
    _minimum -= buffer;
    _maximum += buffer;
    if (any(_minimum.ge(_maximum))) *this = Box2D();
}

void geom::Box2D::shift(Extent2D const & offset) {
    if (isEmpty()) return; // should we throw an exception here instead of a no-op?
    _minimum += offset;
    _maximum += offset;
}

void geom::Box2D::flipLR(float xextent) {
    if (isEmpty()) return; // should we throw an exception here instead of a no-op?
    // Swap min and max values for x dimension
    _minimum[0] += _maximum[0];
    _maximum[0] = _minimum[0] - _maximum[0];
    _minimum[0] -= _maximum[0];
    // Apply flip assuming coordinate system of parent.
    _minimum[0] = xextent - _minimum[0];
    _maximum[0] = xextent - _maximum[0];
    // _dimensions should remain unchanged
}

void geom::Box2D::flipTB(float yextent) {
    if (isEmpty()) return; // should we throw an exception here instead of a no-op?
    // Swap min and max values for y dimension
    _minimum[1] += _maximum[1];
    _maximum[1] = _minimum[1] - _maximum[1];
    _minimum[1] -= _maximum[1];
    // Apply flip assuming coordinate system of parent.
    _minimum[1] = yextent - _minimum[1];
    _maximum[1] = yextent - _maximum[1];
    // _dimensions should remain unchanged
}

void geom::Box2D::include(Point2D const & point) {
    if (isEmpty()) {
        _minimum = point;
        _maximum = point;
        _tweakMax(0);
        _tweakMax(1);
        return;
    }
    for (int n=0; n<2; ++n) {
        if (point[n] < _minimum[n]) {
            _minimum[n] = point[n];
        } else if (point[n] >= _maximum[n]) {
            _maximum[n] = point[n];
            _tweakMax(n);
        }
    }
}

void geom::Box2D::include(Box2D const & other) {
    if (other.isEmpty()) return;
    if (this->isEmpty()) {
        *this = other;
        return;
    }
    Point2D const & otherMin = other.getMin();
    Point2D const & otherMax = other.getMax();
    for (int n=0; n<2; ++n) {
        if (otherMin[n] < _minimum[n]) {
            _minimum[n] = otherMin[n];
        }
        if (otherMax[n] > _maximum[n]) {
            _maximum[n] = otherMax[n];
        }
    }
}

void geom::Box2D::clip(Box2D const & other) {
    if (isEmpty()) return;
    if (other.isEmpty()) {
        *this = Box2D();
        return;
    }
    Point2D const & otherMin = other.getMin();
    Point2D const & otherMax = other.getMax();
    for (int n=0; n<2; ++n) {
        if (otherMin[n] > _minimum[n]) {
            _minimum[n] = otherMin[n];
        }
        if (otherMax[n] < _maximum[n]) {
            _maximum[n] = otherMax[n];
        }
    }
    if (any(_maximum.le(_minimum))) {
        *this = Box2D();
        return;
    }
}

bool geom::Box2D::operator==(Box2D const & other) const {
    return (other.isEmpty() && this->isEmpty()) ||
        (other._minimum == this->_minimum && other._maximum == this->_maximum);
}

bool geom::Box2D::operator!=(Box2D const & other) const {
    return !(other.isEmpty() && other.isEmpty()) &&
        (other._minimum != this->_minimum || other._maximum != this->_maximum);
}

std::vector<geom::Point2D> geom::Box2D::getCorners() const {
    std::vector<geom::Point2D> retVec;
    retVec.push_back(getMin());
    retVec.push_back(geom::Point2D(getMaxX(), getMinY()));
    retVec.push_back(getMax());
    retVec.push_back(geom::Point2D(getMinX(), getMaxY()));
    return retVec;
}

std::ostream & geom::operator<<(std::ostream & os, geom::Box2I const & box) {
    if (box.isEmpty()) return os << "Box2I()";
    return os << "Box2I(Point2I" << box.getMin() << ", Extent2I" << box.getDimensions() << ")";
}

std::ostream & geom::operator<<(std::ostream & os, geom::Box2D const & box) {
    if (box.isEmpty()) return os << "Box2D()";
    return os << "Box2D(Point2D" << box.getMin() << ", Extent2D" << box.getDimensions() << ")";
}
