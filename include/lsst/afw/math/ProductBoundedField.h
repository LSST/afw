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

#ifndef LSST_AFW_MATH_ProductBoundedField_h_INCLUDED
#define LSST_AFW_MATH_ProductBoundedField_h_INCLUDED

#include "lsst/afw/math/BoundedField.h"

namespace lsst {
namespace afw {
namespace math {

/**
 *  A BoundedField that lazily multiplies a sequence of other BoundedFields.
 */
class ProductBoundedField final : public BoundedField {
DECLARE_PERSISTABLE_FACADE(ProductBoundedField);
public:

    /**
     *  Construct from a sequence of BoundedField factors.
     *
     *  @param[in] factors  BoundedFields to be multiplied together.  All
     *                      bounding boxes must be the same.
     *
     *  @throws pex::exceptions::LengthError Thrown if `factors.size() < 1`
     *  @throws pex::exceptions::InvalidParameterError Thrown if the bounding
     *      boxes of the factors are inconsistent.
     */
    explicit ProductBoundedField(std::vector<std::shared_ptr<BoundedField const>> const & factors);

    ProductBoundedField(ProductBoundedField const&);
    ProductBoundedField(ProductBoundedField&&);
    ProductBoundedField& operator=(ProductBoundedField const&) = delete;
    ProductBoundedField& operator=(ProductBoundedField&&) = delete;
    ~ProductBoundedField() override;

    /// @copydoc BoundedField::evaluate(lsst::geom::Point2D const & position) const
    double evaluate(lsst::geom::Point2D const& position) const override;

    /// @copydoc BoundedField::evaluate(ndarray::Array<double const, 1> const& x, ndarray::Array<double const, 1> const& y) const
    ndarray::Array<double, 1, 1> evaluate(ndarray::Array<double const, 1> const& x,
                                          ndarray::Array<double const, 1> const& y) const override;

    using BoundedField::evaluate;

    /**
     *  ProductBoundedField is persistable if and only if all of its factors
     *  are.
     */
    bool isPersistable() const noexcept override;

    /// @copydoc BoundedField::operator*
    std::shared_ptr<BoundedField> operator*(double const scale) const override;

    /// @copydoc BoundedField::operator==
    bool operator==(BoundedField const& rhs) const override;

protected:

    std::string getPersistenceName() const override;

    std::string getPythonModule() const override;

    void write(OutputArchiveHandle& handle) const override;

private:

    std::string toString() const override;

    std::vector<std::shared_ptr<BoundedField const>> _factors;
};
}  // namespace math
}  // namespace afw
}  // namespace lsst

#endif  // !LSST_AFW_MATH_ProductBoundedField_h_INCLUDED
