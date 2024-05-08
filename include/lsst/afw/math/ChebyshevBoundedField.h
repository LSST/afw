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

#ifndef LSST_AFW_MATH_ChebyshevBoundedField_h_INCLUDED
#define LSST_AFW_MATH_ChebyshevBoundedField_h_INCLUDED

#include "ndarray.h"

#include "lsst/pex/config.h"
#include "lsst/geom/AffineTransform.h"
#include "lsst/afw/math/BoundedField.h"

namespace lsst {
namespace afw {
namespace math {

/// A control object used when fitting ChebyshevBoundedField to data (see ChebyshevBoundedField::fit)
class ChebyshevBoundedFieldControl {
public:
    ChebyshevBoundedFieldControl() : orderX(2), orderY(2), triangular(true) {}

    LSST_CONTROL_FIELD(orderX, int, "maximum Chebyshev function order in x");

    LSST_CONTROL_FIELD(orderY, int, "maximum Chebyshev function order in y");

    LSST_CONTROL_FIELD(triangular, bool,
                       "if true, only include terms where the sum of the x and y order "
                       "is less than or equal to max(orderX, orderY)");

    /// Return the number of nonzero coefficients in the Chebyshev function defined by this object
    int computeSize() const;
};

/**
 *  A BoundedField based on 2-d Chebyshev polynomials of the first kind.
 *
 *  The 2-d Chebyshev polynomial used here is defined as:
 *
 *  @f[
 *  f(x,y) = \sum_i \sum_j a_{i,j} T_i(x) T_j(y)
 *  @f]
 *
 *  where @f$T_n(x)@f$ is the n-th order Chebyshev polynomial of @f$x@f$ and
 *  @f$a_{i,j}@f$ is the corresponding coefficient of the (i,j) polynomial term.
 *
 *  ChebyshevBoundedField supports fitting to gridded and non-gridded data,
 *  as well coefficient matrices with different x- and y-order.
 *
 *  There is currently quite a bit of duplication of functionality between
 *  ChebyshevBoundedField, ApproximateChebyshev, and Chebyshev1Function2;
 *  the intent is that ChebyshevBoundedField will ultimately replace
 *  ApproximateChebyshev and should be preferred over Chebyshev1Function2
 *  when the parametrization interface that is part of the Function2 class
 *  is not needed.
 */
class ChebyshevBoundedField : public BoundedField {
DECLARE_PERSISTABLE_FACADE(ChebyshevBoundedField);
public:
    using Control = ChebyshevBoundedFieldControl;

    /**
     *  Initialize the field from its bounding box an coefficients.
     *
     *  This constructor is mostly intended for testing purposes and persistence,
     *  but it also provides a way to initialize the object from Chebyshev coefficients
     *  derived from some external source.
     *
     *  Note that because the bounding box provided is always an integer bounding box,
     *  and LSST convention puts the center of each pixel at an integer, the actual
     *  floating-point domain of the Chebyshev functions is lsst::geom::Box2D(bbox), that is, the
     *  box that contains the entirety of all the pixels included in the integer
     *  bounding box.
     *
     *  The coefficients are ordered [y,x], so the shape is (orderY+1, orderX+1),
     *  and the arguments to the Chebyshev functions are transformed such that
     *  the region lsst::geom::Box2D(bbox) is mapped to [-1, 1]x[-1, 1].
     *
     *  Example:
     *
     *      bbox = lsst::geom::Box2I(lsst::geom::Point2I(10, 20), lsst::geom::Point2I(30, 40));
     *      ndarray::Array<double, 2, 2> coeffs = ndarray::allocate(ndarray::makeVector(2, 2));
     *      coeffs[0][0] = 1;
     *      coeffs[1][0] = 2;
     *      coeffs[0][1] = 3;
     *      coeffs[1][1] = 4;
     *      ndarray::Array<double, 2, 2> coeffs = ndarray::external(data);
     *      poly = ChebyshevBoundedField(bbox, coeffs);
     *
     *  will result in the following polynomial:
     *
     *  @f[
     *  f(x,y) = 1 T_0(x) T_0(y) + 2 T_0(x) T_1(y) + 3 T_1(x) T_0(y) + 4 T_1(x) T_1(y)
     *  @f]
     */
    ChebyshevBoundedField(lsst::geom::Box2I const& bbox,
                          ndarray::Array<double const, 2, 2> const& coefficients);

    ChebyshevBoundedField(ChebyshevBoundedField const&);
    ChebyshevBoundedField(ChebyshevBoundedField&&);
    ChebyshevBoundedField& operator=(ChebyshevBoundedField const&) = delete;
    ChebyshevBoundedField& operator=(ChebyshevBoundedField&&) = delete;
    ~ChebyshevBoundedField() override;

    /**
     *  Fit a Chebyshev approximation to non-gridded data with equal weights.
     *
     *  @param[in]  bbox     Integer bounding box of the resulting approximation.  All
     *                       given points must lie within lsst::geom::Box2D(bbox).
     *  @param[in]  x        Array of x coordinate values.
     *  @param[in]  y        Array of y coordinate values.
     *  @param[in]  z        Array of field values to be fit at each (x,y) point.
     *  @param[in]  ctrl     Specifies the orders and triangularity of the coefficient matrix.
     */
    static std::shared_ptr<ChebyshevBoundedField> fit(lsst::geom::Box2I const& bbox,
                                                      ndarray::Array<double const, 1> const& x,
                                                      ndarray::Array<double const, 1> const& y,
                                                      ndarray::Array<double const, 1> const& z,
                                                      Control const& ctrl);

    /**
     *  Fit a Chebyshev approximation to non-gridded data with unequal weights.
     *
     *  @param[in]  bbox     Integer bounding box of the resulting approximation.  All
     *                       given points must lie within lsst::geom::Box2D(bbox).
     *  @param[in]  x        Array of x coordinate values.
     *  @param[in]  y        Array of y coordinate values.
     *  @param[in]  z        Array of field values to be fit at each (x,y) point.
     *  @param[in]  w        Array of weights for each point in the fit.  For points with Gaussian
     *                       noise, w = 1/sigma.
     *  @param[in]  ctrl     Specifies the orders and triangularity of the coefficient matrix.
     */
    static std::shared_ptr<ChebyshevBoundedField> fit(lsst::geom::Box2I const& bbox,
                                                      ndarray::Array<double const, 1> const& x,
                                                      ndarray::Array<double const, 1> const& y,
                                                      ndarray::Array<double const, 1> const& z,
                                                      ndarray::Array<double const, 1> const& w,
                                                      Control const& ctrl);

    /**
     *  Fit a Chebyshev approximation to gridded data with equal weights.
     *
     *  @param[in]  image    The Image containing the data to fit.  image.getBBox(PARENT) is
     *                       used as the bounding box of the BoundedField.
     *  @param[in]  ctrl     Specifies the orders and triangularity of the coefficient matrix.
     *
     *  Instantiated for float and double.
     *
     *  @note if the image to be fit is a binned version of the actual image the field should
     *        correspond to, call relocate() with the unbinned image's bounding box after
     *        fitting.
     */
    template <typename T>
    static std::shared_ptr<ChebyshevBoundedField> fit(image::Image<T> const& image, Control const& ctrl);

    /**
     *  Return the coefficient matrix.
     *
     *  The coefficients are ordered [y,x], so the shape is (orderY+1, orderX+1).
     */
    ndarray::Array<double const, 2, 2> getCoefficients() const { return _coefficients; }

    /// Return a new ChebyshevBoudedField with maximum orders set by the given control object.
    std::shared_ptr<ChebyshevBoundedField> truncate(Control const& ctrl) const;

    /**
     *  Return a new ChebyshevBoundedField with domain set to the given bounding box.
     *
     *  Because this leaves the coefficients unchanged, it is equivalent to transforming the function
     *  by the affine transform that maps the old box to the new one.
     */
    std::shared_ptr<ChebyshevBoundedField> relocate(lsst::geom::Box2I const& bbox) const;

    /// @copydoc BoundedField::evaluate
    double evaluate(lsst::geom::Point2D const& position) const override;

    using BoundedField::evaluate;

    /// @copydoc BoundedField::integrate
    double integrate() const override;

    /// @copydoc BoundedField::mean
    double mean() const override;

    /// ChebyshevBoundedField is always persistable.
    bool isPersistable() const noexcept override { return true; }

    /// @copydoc BoundedField::operator*
    std::shared_ptr<BoundedField> operator*(double const scale) const override;

    /// @copydoc BoundedField::operator==
    bool operator==(BoundedField const& rhs) const override;

protected:
    std::string getPersistenceName() const override;

    std::string getPythonModule() const override;

    void write(OutputArchiveHandle& handle) const override;

private:
    // Internal constructor for fit() routines: just initializes the transform,
    // leaves coefficients empty.
    explicit ChebyshevBoundedField(lsst::geom::Box2I const& bbox);

    lsst::geom::AffineTransform _toChebyshevRange;     // maps points from the bbox to [-1,1]x[-1,1]
    ndarray::Array<double const, 2, 2> _coefficients;  // shape=(orderY+1, orderX+1)

    std::string toString() const override;
};
}  // namespace math
}  // namespace afw
}  // namespace lsst

#endif  // !LSST_AFW_MATH_ChebyshevBoundedField_h_INCLUDED
