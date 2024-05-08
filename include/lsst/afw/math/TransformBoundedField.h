// -*- LSST-C++ -*-
/*
 * LSST Data Management System
 * Copyright 2017 AURA/LSST.
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

#ifndef LSST_AFW_MATH_TransformBoundedField_h_INCLUDED
#define LSST_AFW_MATH_TransformBoundedField_h_INCLUDED

#include "ndarray.h"

#include "lsst/pex/config.h"
#include "lsst/afw/geom/Endpoint.h"
#include "lsst/afw/geom/Transform.h"
#include "lsst/afw/math/BoundedField.h"

namespace lsst {
namespace afw {
namespace math {

/**
 * A BoundedField based on geom::Transform<Poin2Endpoint, GenericEndpoint<1>>.
 *
 * TransformBoundedField supports arbitrary transforms.
 */
class TransformBoundedField : public BoundedField {
DECLARE_PERSISTABLE_FACADE(TransformBoundedField);
public:
    using Transform = afw::geom::Transform<afw::geom::Point2Endpoint, afw::geom::GenericEndpoint>;

    /**
     *  Create a TransformBoundedField from a bounding box and transform.
     */
    TransformBoundedField(lsst::geom::Box2I const &bbox, Transform const &transform);

    ~TransformBoundedField() override = default;

    TransformBoundedField(TransformBoundedField const &) = default;
    TransformBoundedField(TransformBoundedField &&) = default;
    TransformBoundedField &operator=(TransformBoundedField const &) = delete;
    TransformBoundedField &operator=(TransformBoundedField &&) = delete;

    /// Get the contained Transform
    Transform getTransform() const { return _transform; }

    /// @copydoc BoundedField::evaluate
    double evaluate(lsst::geom::Point2D const &position) const override;

    /// @copydoc BoundedField::evaluate
    ndarray::Array<double, 1, 1> evaluate(ndarray::Array<double const, 1> const &x,
                                          ndarray::Array<double const, 1> const &y) const override;

    using BoundedField::evaluate;

    /// TransformBoundedField is always persistable.
    bool isPersistable() const noexcept override { return true; }

    /// @copydoc BoundedField::operator*
    std::shared_ptr<BoundedField> operator*(double const scale) const override;

    /// @copydoc BoundedField::operator==
    bool operator==(BoundedField const &rhs) const override;

protected:
    std::string getPersistenceName() const override;

    std::string getPythonModule() const override;

    void write(OutputArchiveHandle &handle) const override;

private:
    // Internal constructor for fit() routines: just initializes the transform,
    // leaves coefficients empty.
    explicit TransformBoundedField(lsst::geom::Box2I const &bbox);

    std::string toString() const override;

    Transform _transform;
};
}  // namespace math
}  // namespace afw
}  // namespace lsst

#endif  // !LSST_AFW_MATH_TransformBoundedField_h_INCLUDED
