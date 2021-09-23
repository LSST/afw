// -*- lsst-c++ -*-

/*
 * LSST Data Management System
 * Copyright 2008, 2009, 2010 LSST Corporation.
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

#ifndef LSST_AFW_GEOM_ELLIPSES_Convolution_h_INCLUDED
#define LSST_AFW_GEOM_ELLIPSES_Convolution_h_INCLUDED

/*
 *  Definitions for BaseEllipse::Convolution and BaseCore::Convolution.
 *
 *  Note: do not include directly; use the main ellipse header file.
 */

#include "lsst/afw/geom/ellipses/Ellipse.h"

namespace lsst {
namespace afw {
namespace geom {
namespace ellipses {

/**
 *  A temporary-only expression object for ellipse core convolution.
 */
class BaseCore::Convolution final {
public:
    /// Matrix type for derivative with respect to input ellipse parameters.
    using DerivativeMatrix = Eigen::Matrix3d;

    /// Standard constructor.
    Convolution(BaseCore &self, BaseCore const &other) : self(self), other(other) {}

    /// Return a new convolved ellipse core.
    std::shared_ptr<BaseCore> copy() const;

    /// Convolve the ellipse core in-place.
    void inPlace();

    /// Return the derivative of convolved core with respect to self.
    DerivativeMatrix d() const;

    void apply(BaseCore &result) const;

    BaseCore &self;
    BaseCore const &other;
};

/**
 *  A temporary-only expression object for ellipse convolution.
 */
class Ellipse::Convolution final {
public:
    /// Matrix type for derivative with respect to input ellipse parameters.
    using DerivativeMatrix = Eigen::Matrix<double, 5, 5>;

    /// Standard constructor.
    Convolution(Ellipse &self, Ellipse const &other) : self(self), other(other) {}

    /// Return a new convolved ellipse.
    std::shared_ptr<Ellipse> copy() const;

    /// Convolve the ellipse in-place.
    void inPlace();

    /// Return the derivative of convolved ellipse with respect to self.
    DerivativeMatrix d() const;

    Ellipse &self;
    Ellipse const &other;
};

inline BaseCore::Convolution BaseCore::convolve(BaseCore const &other) {
    return BaseCore::Convolution(*this, other);
}

inline BaseCore::Convolution const BaseCore::convolve(BaseCore const &other) const {
    return BaseCore::Convolution(const_cast<BaseCore &>(*this), other);
}

inline Ellipse::Convolution Ellipse::convolve(Ellipse const &other) {
    return Ellipse::Convolution(*this, other);
}

inline Ellipse::Convolution const Ellipse::convolve(Ellipse const &other) const {
    return Ellipse::Convolution(const_cast<Ellipse &>(*this), other);
}
}  // namespace ellipses
}  // namespace geom
}  // namespace afw
}  // namespace lsst

#endif  // !LSST_AFW_GEOM_ELLIPSES_Convolution_h_INCLUDED
