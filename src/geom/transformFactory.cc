// -*- LSST-C++ -*-
/*
 * LSST Data Management System
 * See COPYRIGHT file at the top of the source tree.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program. If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

#include <sstream>
#include <cmath>

#include "astshim.h"

#include "Eigen/Core"

#include "lsst/afw/geom/Endpoint.h"
#include "lsst/afw/geom/transformFactory.h"
#include "lsst/pex/exceptions.h"

#include "ndarray.h"
#include "ndarray/eigen.h"

namespace lsst {
namespace afw {
namespace geom {

namespace {
/**
 * Print a vector to a stream.
 *
 * The exact details of the representation are unspecified and subject to
 * change, but the following may be regarded as typical:
 *
 *     [1.0, -3.560, 42.0]
 *
 * @tparam T the element type. Must support stream output.
 */
template <typename T>
std::ostream &operator<<(std::ostream &os, std::vector<T> const &v) {
    os << '[';
    bool first = true;
    for (T element : v) {
        if (first) {
            first = false;
        } else {
            os << ", ";
        }
        os << element;
    }
    os << ']';
    return os;
}

/**
 * Convert a Matrix to the equivalent ndarray.
 *
 * @param matrix The matrix to convert.
 * @returns an ndarray containing a copy of the data in `matrix`
 */
template <class T, int Rows, int Cols>
ndarray::Array<T, 2, 2> toNdArray(Eigen::Matrix<T, Rows, Cols> const &matrix) {
    ndarray::Array<T, 2, 2> array = ndarray::allocate(ndarray::makeVector(matrix.rows(), matrix.cols()));
    array.asEigen() = matrix;
    return array;
}

/**
 * Tests whether polynomial coefficients match the expected format.
 *
 * @param coeffs radial polynomial coefficients.
 * @returns `true` if either `coeffs.size()` = 0, or `coeffs.size()` > 1,
 *          `coeffs[0]` = 0, and `coeffs[1]` &ne; 0. `false` otherwise.
 */
bool areRadialCoefficients(std::vector<double> const &coeffs) noexcept {
    if (coeffs.empty()) {
        return true;
    } else {
        return coeffs.size() > 1 && coeffs[0] == 0.0 && coeffs[1] != 0.0;
    }
}

/**
 * A one-dimensional polynomial distortion.
 *
 * The Mapping computes a scalar function
 * @f[ f(x) = \sum_{i=1}^{N} \mathrm{coeffs[i]} \ x^i @f]
 *
 * @param coeffs radial polynomial coefficients. Must have `size` > 1,
 *               `coeffs[0]` = 0, and `coeffs[1]` &ne; 0.
 * @returns the function represented by `coeffs`. The Mapping shall have an
 *          inverse, which may be approximate.
 *
 * @exceptsafe Provides basic exception safety.
 *
 * @warning Input to this function is not validated.
 */
ast::PolyMap makeOneDDistortion(std::vector<double> const &coeffs) {
    int const nCoeffs = coeffs.size() - 1;
    ndarray::Array<double, 2, 2> const polyCoeffs = ndarray::allocate(ndarray::makeVector(nCoeffs, 3));
    for (size_t i = 1; i < coeffs.size(); ++i) {
        polyCoeffs[i - 1][0] = coeffs[i];
        polyCoeffs[i - 1][1] = 1;
        polyCoeffs[i - 1][2] = i;
    }

    return ast::PolyMap(polyCoeffs, 1, "IterInverse=1, TolInverse=1e-8, NIterInverse=20");
}

/**
 * Wraps a one-dimensional mapping as a radial mapping.
 *
 * @param oneDMapping a 1D -> 1D mapping
 * @return a Transform that applies `oneDMapping` to the radius of
 *         two-dimensional points. It shall be invertible if and only if
 *         `oneDMapping` is.
 *
 * @warning Input to this function is not validated.
 */
Transform<Point2Endpoint, Point2Endpoint> wrapRadialMapping(ast::Mapping const &oneDMapping) {
    ast::UnitNormMap const splitNorm(std::vector<double>(2));
    auto const map =
            splitNorm.then(ast::ParallelMap(ast::UnitMap(2), oneDMapping)).then(*(splitNorm.getInverse()));
    return Transform<Point2Endpoint, Point2Endpoint>(map);
}

}  // namespace

template <class From, class To>
Transform<From, To> linearizeTransform(Transform<From, To> const &original,
                                       typename Transform<From, To>::FromPoint const &inPoint) {
    auto fromEndpoint = original.getFromEndpoint();
    auto toEndpoint = original.getToEndpoint();

    auto outPoint = original.applyForward(inPoint);
    auto jacobian = original.getJacobian(inPoint);
    for (int i = 0; i < toEndpoint.getNAxes(); ++i) {
        if (!std::isfinite(outPoint[i])) {
            std::ostringstream buffer;
            buffer << "Transform ill-defined: " << inPoint << " -> " << outPoint;
            throw LSST_EXCEPT(pex::exceptions::InvalidParameterError, buffer.str());
        }
    }
    if (!jacobian.allFinite()) {
        std::ostringstream buffer;
        buffer << "Transform not continuous at " << inPoint << ": J = " << jacobian;
        throw LSST_EXCEPT(pex::exceptions::InvalidParameterError, buffer.str());
    }

    // y(x) = J (x - x0) + y0
    auto map = ast::ShiftMap(fromEndpoint.dataFromPoint(inPoint))
                       .getInverse()
                       ->then(ast::MatrixMap(toNdArray(jacobian)))
                       .then(ast::ShiftMap(toEndpoint.dataFromPoint(outPoint)));
    return Transform<From, To>(map);
}

Transform<Point2Endpoint, Point2Endpoint> makeTransform(AffineTransform const &affine) {
    auto const offset = Point2D(affine.getTranslation());
    auto const jacobian = affine.getLinear().getMatrix();

    Point2Endpoint toEndpoint;
    auto const map =
            ast::MatrixMap(toNdArray(jacobian)).then(ast::ShiftMap(toEndpoint.dataFromPoint(offset)));
    return Transform<Point2Endpoint, Point2Endpoint>(map);
}

Transform<Point2Endpoint, Point2Endpoint> makeRadialTransform(std::vector<double> const &coeffs) {
    if (!areRadialCoefficients(coeffs)) {
        std::ostringstream buffer;
        buffer << "Invalid coefficient vector: " << coeffs;
        throw LSST_EXCEPT(pex::exceptions::InvalidParameterError, buffer.str());
    }

    if (coeffs.empty()) {
        return Transform<Point2Endpoint, Point2Endpoint>(ast::UnitMap(2));
    } else {
        // Easier to compute invertible transform in one dimension
        ast::PolyMap const distortion = makeOneDDistortion(coeffs);
        return wrapRadialMapping(distortion);
    }
}

Transform<Point2Endpoint, Point2Endpoint> makeRadialTransform(std::vector<double> const &forwardCoeffs,
                                                              std::vector<double> const &inverseCoeffs) {
    if (!areRadialCoefficients(forwardCoeffs)) {
        std::ostringstream buffer;
        buffer << "Invalid forward coefficient vector: " << forwardCoeffs;
        throw LSST_EXCEPT(pex::exceptions::InvalidParameterError, buffer.str());
    }
    if (!areRadialCoefficients(inverseCoeffs)) {
        std::ostringstream buffer;
        buffer << "Invalid inverse coefficient vector: " << inverseCoeffs;
        throw LSST_EXCEPT(pex::exceptions::InvalidParameterError, buffer.str());
    }
    if (forwardCoeffs.empty() != inverseCoeffs.empty()) {
        throw LSST_EXCEPT(
                pex::exceptions::InvalidParameterError,
                "makeRadialTransform must have either both empty or both non-empty coefficient vectors.");
    }

    if (forwardCoeffs.empty()) {
        return Transform<Point2Endpoint, Point2Endpoint>(ast::UnitMap(2));
    } else {
        ast::PolyMap const forward = makeOneDDistortion(forwardCoeffs);
        auto inverse = makeOneDDistortion(inverseCoeffs).getInverse();
        return wrapRadialMapping(ast::TranMap(forward, *inverse));
    }
}

Transform<GenericEndpoint, GenericEndpoint> makeIdentityTransform(int nDimensions) {
    if (nDimensions <= 0) {
        throw LSST_EXCEPT(pex::exceptions::InvalidParameterError,
                          "Cannot create identity Transform with dimension " + std::to_string(nDimensions));
    }

    return Transform<GenericEndpoint, GenericEndpoint>(ast::UnitMap(nDimensions));
}

#define INSTANTIATE_FACTORIES(From, To)                                                             \
    template Transform<From, To> linearizeTransform<From, To>(Transform<From, To> const &transform, \
                                                              Transform<From, To>::FromPoint const &point);

// explicit instantiations
INSTANTIATE_FACTORIES(GenericEndpoint, GenericEndpoint);
INSTANTIATE_FACTORIES(GenericEndpoint, Point2Endpoint);
INSTANTIATE_FACTORIES(Point2Endpoint, GenericEndpoint);
INSTANTIATE_FACTORIES(Point2Endpoint, Point2Endpoint);
}
}
} /* namespace lsst::afw::geom */
