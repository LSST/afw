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

#ifndef LSST_AFW_GEOM_ELLIPSES_Separable_h_INCLUDED
#define LSST_AFW_GEOM_ELLIPSES_Separable_h_INCLUDED

/*
 *  Definitions and inlines for Separable.
 *
 *  Note: do not include directly; use the main ellipse header file.
 */

#include "lsst/afw/geom/ellipses/BaseCore.h"
#include "lsst/afw/geom/ellipses/Convolution.h"
#include "lsst/afw/geom/ellipses/Transformer.h"
#include "lsst/afw/geom/ellipses/GridTransform.h"

namespace lsst {
namespace afw {
namespace geom {
namespace ellipses {

/**
 *  An ellipse core with a complex ellipticity and radius parameterization.
 *
 *
 */
template <typename Ellipticity_, typename Radius_>
class Separable : public BaseCore {
public:
    enum ParameterEnum { E1 = 0, E2 = 1, RADIUS = 2 };  ///< Definitions for elements of a core vector.

    using Ellipticity = Ellipticity_;
    using Radius = Radius_;

    double const getE1() const { return _ellipticity.getE1(); }
    void setE1(double e1) { _ellipticity.setE1(e1); }

    double const getE2() const { return _ellipticity.getE2(); }
    void setE2(double e2) { _ellipticity.setE2(e2); }

    Radius const& getRadius() const { return _radius; }
    Radius& getRadius() { return _radius; }
    void setRadius(double radius) { _radius = radius; }
    void setRadius(Radius const& radius) { _radius = radius; }

    Ellipticity const& getEllipticity() const { return _ellipticity; }
    Ellipticity& getEllipticity() { return _ellipticity; }

    /// Deep copy the ellipse core.
    std::shared_ptr<Separable> clone() const { return std::static_pointer_cast<Separable>(_clone()); }

    /// Return a string that identifies this parametrization.
    std::string getName() const override;

    /**
     *  @brief Put the parameters into a "standard form", and throw InvalidParameterError
     *         if they cannot be normalized.
     */
    void normalize() override;

    void readParameters(double const* iter) override;

    void writeParameters(double* iter) const override;

    /// Standard assignment.
    Separable& operator=(Separable const& other);

    Separable& operator=(Separable&& other);

    /// Converting assignment.
    Separable& operator=(BaseCore const& other) {
        BaseCore::operator=(other);
        return *this;
    }

    /// Construct from parameter values.
    explicit Separable(double e1 = 0.0, double e2 = 0.0, double radius = Radius(), bool normalize = true);

    /// Construct from parameter values.
    explicit Separable(std::complex<double> const& complex, double radius = Radius(), bool normalize = true);

    /// Construct from parameter values.
    explicit Separable(Ellipticity const& ellipticity, double radius = Radius(), bool normalize = true);

    /// Construct from a parameter vector.
    explicit Separable(BaseCore::ParameterVector const& vector, bool normalize = false);

    /// Copy constructor.
    Separable(Separable const& other) : _ellipticity(other._ellipticity), _radius(other._radius) {}

    // Delegate to copy-constructor for backwards compatibility
    Separable(Separable&& other) : Separable(other) {}

    ~Separable() override = default;

    /// Converting copy constructor.
    Separable(BaseCore const& other) { *this = other; }

    /// Converting copy constructor.
    Separable(BaseCore::Transformer const& transformer) { transformer.apply(*this); }

    /// Converting copy constructor.
    Separable(BaseCore::Convolution const& convolution) { convolution.apply(*this); }

protected:
    std::shared_ptr<BaseCore> _clone() const override { return std::make_shared<Separable>(*this); }

    void _assignToQuadrupole(double& ixx, double& iyy, double& ixy) const override;
    void _assignFromQuadrupole(double ixx, double iyy, double ixy) override;

    void _assignToAxes(double& a, double& b, double& theta) const override;
    void _assignFromAxes(double a, double b, double theta) override;

    Jacobian _dAssignToQuadrupole(double& ixx, double& iyy, double& ixy) const override;
    Jacobian _dAssignFromQuadrupole(double ixx, double iyy, double ixy) override;

    Jacobian _dAssignToAxes(double& a, double& b, double& theta) const override;
    Jacobian _dAssignFromAxes(double a, double b, double theta) override;

private:
    static BaseCore::Registrar<Separable> registrar;

    Ellipticity _ellipticity;
    Radius _radius;
};
}  // namespace ellipses
}  // namespace geom
}  // namespace afw
}  // namespace lsst

#endif  // !LSST_AFW_GEOM_ELLIPSES_Separable_h_INCLUDED
