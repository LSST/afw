// -*- LSST-C++ -*-

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

#ifndef LSST_AFW_MATH_FUNCTIONLIBRARY_H
#define LSST_AFW_MATH_FUNCTIONLIBRARY_H
/*
 * Define a collection of useful Functions.
 */
#include <algorithm>
#include <cmath>

#include "lsst/geom.h"
#include "lsst/afw/math/Function.h"
#include "lsst/geom/Angle.h"

namespace lsst {
namespace afw {
namespace math {

/**
 * 1-dimensional integer delta function.
 *
 * f(x) = 1 if x == xo, 0 otherwise.
 *
 * For use as a kernel function be sure to handle the offset for row and column center.
 *
 * @ingroup afw
 */
template <typename ReturnT>
class IntegerDeltaFunction1 : public Function1<ReturnT> {
public:
    /**
     * Construct an integer delta function with specified xo
     */
    explicit IntegerDeltaFunction1(double xo) : Function1<ReturnT>(0), _xo(xo) {}

    IntegerDeltaFunction1(IntegerDeltaFunction1 const&) = default;
    IntegerDeltaFunction1(IntegerDeltaFunction1&&) = default;
    IntegerDeltaFunction1& operator=(IntegerDeltaFunction1 const&) = default;
    IntegerDeltaFunction1& operator=(IntegerDeltaFunction1&&) = default;
    ~IntegerDeltaFunction1() noexcept override = default;

    std::shared_ptr<Function1<ReturnT>> clone() const override {
        return std::shared_ptr<Function1<ReturnT>>(new IntegerDeltaFunction1(_xo));
    }

    ReturnT operator()(double x) const noexcept(IS_NOTHROW_INIT<ReturnT>) override {
        return static_cast<ReturnT>(x == _xo);
    }

    std::string toString(std::string const& prefix = "") const override {
        std::ostringstream os;
        os << "IntegerDeltaFunction1 [" << _xo << "]: ";
        os << Function1<ReturnT>::toString(prefix);
        return os.str();
    };

private:
    double _xo;

protected:
    /* Default constructor: intended only for serialization */
    explicit IntegerDeltaFunction1() : Function1<ReturnT>(0), _xo(0.0) {}
};

/**
 * 2-dimensional integer delta function.
 *
 * f(x) = 1 if x == xo and y == yo, 0 otherwise.
 *
 * For use as a kernel function be sure to handle the offset for row and column center.
 *
 * @ingroup afw
 */
template <typename ReturnT>
class IntegerDeltaFunction2 : public Function2<ReturnT> {
public:
    /**
     * Construct an integer delta function with specified xo, yo
     */
    explicit IntegerDeltaFunction2(double xo, double yo) : Function2<ReturnT>(0), _xo(xo), _yo(yo) {}

    IntegerDeltaFunction2(IntegerDeltaFunction2 const&) = default;
    IntegerDeltaFunction2(IntegerDeltaFunction2&&) = default;
    IntegerDeltaFunction2& operator=(IntegerDeltaFunction2 const&) = default;
    IntegerDeltaFunction2& operator=(IntegerDeltaFunction2&&) = default;
    ~IntegerDeltaFunction2() noexcept override = default;

    std::shared_ptr<Function2<ReturnT>> clone() const override {
        return std::shared_ptr<Function2<ReturnT>>(new IntegerDeltaFunction2(_xo, _yo));
    }

    ReturnT operator()(double x, double y) const noexcept(IS_NOTHROW_INIT<ReturnT>) override {
        return static_cast<ReturnT>((x == _xo) && (y == _yo));
    }

    std::string toString(std::string const& prefix) const override {
        std::ostringstream os;
        os << "IntegerDeltaFunction2 [" << _xo << ", " << _yo << "]: ";
        os << Function2<ReturnT>::toString(prefix);
        return os.str();
    }

private:
    double _xo;
    double _yo;

protected:
    /* Default constructor: intended only for serialization */
    explicit IntegerDeltaFunction2() : Function2<ReturnT>(0), _xo(0.0), _yo(0.0) {}
};

/**
 * 1-dimensional Gaussian
 *
 * f(x) = A e^(-x^2 / 2 sigma^2)
 * where:
 * * A = 1 / (sqrt(2 pi) xSigma)
 * coefficient c0 = sigma
 *
 * @ingroup afw
 */
template <typename ReturnT>
class GaussianFunction1 : public Function1<ReturnT> {
public:
    /**
     * Construct a Gaussian function with specified sigma
     */
    explicit GaussianFunction1(double sigma)  ///< sigma
            : Function1<ReturnT>(1), _multFac(1.0 / std::sqrt(lsst::geom::TWOPI)) {
        this->_params[0] = sigma;
    }
    GaussianFunction1(GaussianFunction1 const&) = default;
    GaussianFunction1(GaussianFunction1&&) = default;
    GaussianFunction1& operator=(GaussianFunction1 const&) = default;
    GaussianFunction1& operator=(GaussianFunction1&&) = default;
    ~GaussianFunction1() noexcept override = default;

    std::shared_ptr<Function1<ReturnT>> clone() const override {
        return std::shared_ptr<Function1<ReturnT>>(new GaussianFunction1(this->_params[0]));
    }

    ReturnT operator()(double x) const noexcept(IS_NOTHROW_INIT<ReturnT>) override {
        return static_cast<ReturnT>((_multFac / this->_params[0]) *
                                    std::exp(-(x * x) / (2.0 * this->_params[0] * this->_params[0])));
    }

    std::string toString(std::string const& prefix) const override {
        std::ostringstream os;
        os << "GaussianFunction1 [" << _multFac << "]: ";
        os << Function1<ReturnT>::toString(prefix);
        return os.str();
    }

private:
    const double _multFac;  ///< precomputed scale factor

protected:
    /* Default constructor: intended only for serialization */
    explicit GaussianFunction1() : Function1<ReturnT>(1), _multFac(1.0 / std::sqrt(lsst::geom::TWOPI)) {}
};

/**
 * 2-dimensional Gaussian
 *
 * f(x,y) = A e^((-pos1^2 / 2 sigma1^2) - (pos2^2 / 2 sigma2^2))
 * where:
 * * A = 1 / (2 pi sigma1 sigma2)
 * * pos1 =  cos(angle) x + sin(angle) y
 * * pos2 = -sin(angle) x + cos(angle) y
 * coefficients c0 = sigma1, c1 = sigma2, c2 = angle
 *
 * @note if sigma1 > sigma2 then angle is the angle of the major axis
 *
 * @ingroup afw
 */
template <typename ReturnT>
class GaussianFunction2 : public Function2<ReturnT> {
public:
    /**
     * Construct a 2-dimensional Gaussian function
     */
    explicit GaussianFunction2(double sigma1,       ///< sigma along the pos1 axis
                               double sigma2,       ///< sigma along the pos2 axis
                               double angle = 0.0)  ///< angle of pos1 axis, in rad (along x=0, y=pi/2)
            : Function2<ReturnT>(3), _multFac(1.0 / (lsst::geom::TWOPI)) {
        this->_params[0] = sigma1;
        this->_params[1] = sigma2;
        this->_params[2] = angle;
        _updateCache();
    }

    GaussianFunction2(GaussianFunction2 const&) = default;
    GaussianFunction2(GaussianFunction2&&) = default;
    GaussianFunction2& operator=(GaussianFunction2 const&) = default;
    GaussianFunction2& operator=(GaussianFunction2&&) = default;
    ~GaussianFunction2() noexcept override = default;

    std::shared_ptr<Function2<ReturnT>> clone() const override {
        return std::shared_ptr<Function2<ReturnT>>(
                new GaussianFunction2(this->_params[0], this->_params[1], this->_params[2]));
    }

    ReturnT operator()(double x, double y) const noexcept(IS_NOTHROW_INIT<ReturnT>) override {
        if (_angle != this->_params[2]) {
            _updateCache();
        }
        double pos1 = (_cosAngle * x) + (_sinAngle * y);
        double pos2 = (-_sinAngle * x) + (_cosAngle * y);
        return static_cast<ReturnT>((_multFac / (this->_params[0] * this->_params[1])) *
                                    std::exp(-((pos1 * pos1) / (2.0 * this->_params[0] * this->_params[0])) -
                                             ((pos2 * pos2) / (2.0 * this->_params[1] * this->_params[1]))));
    }

    std::string toString(std::string const& prefix) const override {
        std::ostringstream os;
        os << "GaussianFunction2: ";
        os << Function2<ReturnT>::toString(prefix);
        return os.str();
    }

    bool isPersistable() const noexcept override { return true; }

protected:
    std::string getPersistenceName() const override;

    void write(afw::table::io::OutputArchiveHandle& handle) const override;

private:
    /**
     * Update cached values
     *
     * sin(angle) and cos(angle) are cached to speed computation
     * and angle is cached so one can check if an update is required
     *
     * The current design is to have operator() update the cache if needed.
     * An alternate design is to update the cache when the parameters are set,
     * not test in operator().
     * The main advantage to updating in operator() is safety and simplicity.
     * The test is performed in one place, and it is the place where it matters the most.
     * In contrast, there are multiple member functions to set parameters, and all must be overloaded
     * to update the cache; miss one and the function silently misbehaves.
     * There are trade-offs, of course. Testing the cache in operator() slows down operator() slightly.
     * The overhead is small, but the function is typically evaulated more often
     * than its parameters are changed.
     */
    void _updateCache() const noexcept {
        _angle = this->_params[2];
        _sinAngle = std::sin(_angle);
        _cosAngle = std::cos(_angle);
    }
    const double _multFac;     ///< precomputed scale factor
    mutable double _angle;     ///< cached angle
    mutable double _sinAngle;  ///< cached sin(angle)
    mutable double _cosAngle;  ///< cached cos(angle)

protected:
    /* Default constructor: intended only for serialization */
    explicit GaussianFunction2()
            : Function2<ReturnT>(3),
              _multFac(1.0 / (lsst::geom::TWOPI)),
              _angle(0.0),
              _sinAngle(0.0),
              _cosAngle(1.0) {}
};

/**
 * double Guassian (sum of two Gaussians)
 *
 * Intended for use as a PSF model: the main Gaussian represents the core
 * and the second Gaussian represents the wings.
 *
 * f(x,y) = A (e^(-r^2 / 2 sigma1^2) + ampl2 e^(-r^2 / 2 sigma2^2))
 * where:
 * * A = 1 / (2 pi (sigma1^2 + ampl2 sigma2^2))
 * * r^2 = x^2 + y^2
 * coefficients c[0] = sigma1, c[1] = sigma2, c[2] = ampl2
 *
 * @ingroup afw
 */
template <typename ReturnT>
class DoubleGaussianFunction2 : public Function2<ReturnT> {
public:
    /**
     * Construct a Gaussian function with specified x and y sigma
     */
    explicit DoubleGaussianFunction2(
            double sigma1,      ///< sigma of main Gaussian
            double sigma2 = 0,  ///< sigma of second Gaussian
            double ampl2 = 0)   ///< amplitude of second Gaussian as a fraction of main Gaussian at peak
            : Function2<ReturnT>(3), _multFac(1.0 / (lsst::geom::TWOPI)) {
        this->_params[0] = sigma1;
        this->_params[1] = sigma2;
        this->_params[2] = ampl2;
    }

    DoubleGaussianFunction2(DoubleGaussianFunction2 const&) = default;
    DoubleGaussianFunction2(DoubleGaussianFunction2&&) = default;
    DoubleGaussianFunction2& operator=(DoubleGaussianFunction2 const&) = default;
    DoubleGaussianFunction2& operator=(DoubleGaussianFunction2&&) = default;
    ~DoubleGaussianFunction2() noexcept override = default;

    std::shared_ptr<Function2<ReturnT>> clone() const override {
        return std::shared_ptr<Function2<ReturnT>>(
                new DoubleGaussianFunction2(this->_params[0], this->_params[1], this->_params[2]));
    }

    ReturnT operator()(double x, double y) const noexcept(IS_NOTHROW_INIT<ReturnT>) override {
        double radSq = (x * x) + (y * y);
        double sigma1Sq = this->_params[0] * this->_params[0];
        double sigma2Sq = this->_params[1] * this->_params[1];
        double b = this->_params[2];
        return static_cast<ReturnT>(
                (_multFac / (sigma1Sq + (b * sigma2Sq))) *
                (std::exp(-radSq / (2.0 * sigma1Sq)) + (b * std::exp(-radSq / (2.0 * sigma2Sq)))));
    }

    std::string toString(std::string const& prefix) const override {
        std::ostringstream os;
        os << "DoubleGaussianFunction2 [" << _multFac << "]: ";
        os << Function2<ReturnT>::toString(prefix);
        return os.str();
    }

    bool isPersistable() const noexcept override { return true; }

protected:
    std::string getPersistenceName() const override;

    void write(afw::table::io::OutputArchiveHandle& handle) const override;

private:
    const double _multFac;  ///< precomputed scale factor

protected:
    /* Default constructor: intended only for serialization */
    explicit DoubleGaussianFunction2() : Function2<ReturnT>(3), _multFac(1.0 / (lsst::geom::TWOPI)) {}
};

/**
 * 1-dimensional polynomial function.
 *
 * f(x) = c0 + c1 x + c2 x^2 + ... cn-1 x^(n-1)
 *
 * @ingroup afw
 */
template <typename ReturnT>
class PolynomialFunction1 : public Function1<ReturnT> {
public:
    /**
     * Construct a polynomial function of the specified order.
     *
     * The parameters are initialized to zero.
     */
    explicit PolynomialFunction1(unsigned int order)  ///< order of polynomial (0 for constant)
            : Function1<ReturnT>(order + 1) {}

    /**
     * Construct a polynomial function with the specified parameters.
     *
     * The order of the polynomial is set to the length of the params vector.
     *
     * @throws lsst::pex::exceptions::InvalidParameter if params is empty
     */
    explicit PolynomialFunction1(std::vector<double> params)  ///< polynomial coefficients (const, x, x^2...)
            : Function1<ReturnT>(params) {
        if (params.size() < 1) {
            throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterError,
                              "PolynomialFunction1 called with empty vector");
        }
    }

    PolynomialFunction1(PolynomialFunction1 const&) = default;
    PolynomialFunction1(PolynomialFunction1&&) = default;
    PolynomialFunction1& operator=(PolynomialFunction1 const&) = default;
    PolynomialFunction1& operator=(PolynomialFunction1&&) = default;
    ~PolynomialFunction1() noexcept override = default;

    std::shared_ptr<Function1<ReturnT>> clone() const override {
        return std::shared_ptr<Function1<ReturnT>>(new PolynomialFunction1(this->_params));
    }

    bool isLinearCombination() const noexcept override { return true; };

    ReturnT operator()(double x) const noexcept(IS_NOTHROW_INIT<ReturnT>) override {
        int const order = static_cast<int>(this->_params.size()) - 1;
        double retVal = this->_params[order];
        for (int ii = order - 1; ii >= 0; --ii) {
            retVal = (retVal * x) + this->_params[ii];
        }
        return static_cast<ReturnT>(retVal);
    }

    /**
     * Get the polynomial order
     */
    unsigned int getOrder() const noexcept { return this->getNParameters() - 1; };

    std::string toString(std::string const& prefix) const override {
        std::ostringstream os;
        os << "PolynomialFunction1 []: ";
        os << Function1<ReturnT>::toString(prefix);
        return os.str();
    }

protected:
    /* Default constructor: intended only for serialization */
    explicit PolynomialFunction1() : Function1<ReturnT>(1) {}
};

/**
 * 2-dimensional polynomial function with cross terms
 *
 * f(x,y) = c0                                          (0th order)
 *          + c1 x   + c2 y                             (1st order)
 *          + c3 x^2 + c4 x y   + c5 y^2                (2nd order)
 *          + c6 x^3 + c7 x^2 y + c8 x y^2 + c9 y^3     (3rd order)
 *          + ...
 *
 * Intermediate products for the most recent y are cached,
 * so when computing for a set of x, y it is more efficient to change x before you change y.
 *
 * @ingroup afw
 */
template <typename ReturnT>
class PolynomialFunction2 : public BasePolynomialFunction2<ReturnT> {
public:
    /**
     * Construct a polynomial function of specified order.
     *
     * The polynomial will have (order + 1) * (order + 2) / 2 coefficients
     *
     * The parameters are initialized to zero.
     */
    explicit PolynomialFunction2(unsigned int order)  ///< order of polynomial (0 for constant)
            : BasePolynomialFunction2<ReturnT>(order), _oldY(0), _xCoeffs(this->_order + 1) {}

    /**
     * Construct a polynomial function with specified parameters.
     *
     * The order of the polynomial is determined from the length of the params vector:
     *   order = (sqrt(1 + 8 * length) - 3) / 2
     * and if this is not an integer then the length is unsuitable
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if params length is unsuitable
     * @throws lsst::pex::exceptions::Exception if an internal sanity check fails
     */
    explicit PolynomialFunction2(
            std::vector<double> params)  ///< polynomial coefficients (const, x, y, x^2, xy, y^2...);
                                         ///< length must be one of 1, 3, 6, 10, 15...
            : BasePolynomialFunction2<ReturnT>(params), _oldY(0), _xCoeffs(this->_order + 1) {}

    PolynomialFunction2(PolynomialFunction2 const&) = default;
    PolynomialFunction2(PolynomialFunction2&&) = default;
    PolynomialFunction2& operator=(PolynomialFunction2 const&) = default;
    PolynomialFunction2& operator=(PolynomialFunction2&&) = default;
    ~PolynomialFunction2() noexcept override = default;

    std::shared_ptr<Function2<ReturnT>> clone() const override {
        return std::shared_ptr<Function2<ReturnT>>(new PolynomialFunction2(this->_params));
    }

    ReturnT operator()(double x, double y) const noexcept(IS_NOTHROW_INIT<ReturnT>) override {
        /* Solve as follows:
        - f(x,y) = Cx0 + Cx1 x + Cx2 x^2 + Cx3 x^3 + ...
        where:
          Cx0 = P0 + P2 y + P5 y^2 + P9 y^3 + ...
          Cx1 = P1 + P4 y + P8 y2 + ...
          Cx2 = P3 + P7 y + ...
          Cx3 = P6 + ...
          ...

        Compute Cx0, Cx1...Cxn by solving 1-d polynomials in y in the usual way.
        These values are cached and only recomputed for new values of Y or if the parameters change.

        Then compute f(x,y) by solving the 1-d polynomial in x in the usual way.
        */
        const int maxXCoeffInd = this->_order;

        if ((y != _oldY) || !this->_isCacheValid) {
            // update _xCoeffs cache
            // note: paramInd is decremented in both of the following loops
            int paramInd = static_cast<int>(this->_params.size()) - 1;

            // initialize _xCoeffs to coeffs for pure y^n; e.g. for 3rd order:
            // _xCoeffs[0] = _params[9], _xCoeffs[1] = _params[8], ... _xCoeffs[3] = _params[6]
            for (int xCoeffInd = 0; xCoeffInd <= maxXCoeffInd; ++xCoeffInd, --paramInd) {
                _xCoeffs[xCoeffInd] = this->_params[paramInd];
            }

            // finish computing _xCoeffs
            for (int xCoeffInd = 0, endXCoeffInd = maxXCoeffInd; paramInd >= 0; --paramInd) {
                _xCoeffs[xCoeffInd] = (_xCoeffs[xCoeffInd] * y) + this->_params[paramInd];
                ++xCoeffInd;
                if (xCoeffInd >= endXCoeffInd) {
                    xCoeffInd = 0;
                    --endXCoeffInd;
                }
            }

            _oldY = y;
            this->_isCacheValid = true;
        }

        // use _xCoeffs to compute result
        double retVal = _xCoeffs[maxXCoeffInd];
        for (int xCoeffInd = maxXCoeffInd - 1; xCoeffInd >= 0; --xCoeffInd) {
            retVal = (retVal * x) + _xCoeffs[xCoeffInd];
        }
        return static_cast<ReturnT>(retVal);
    }

    /**
     * Return the coefficients of the Function's parameters, evaluated at (x, y)
     * I.e. given c0, c1, c2, c3 ... return 1, x, y, x^2 ...
     */
    std::vector<double> getDFuncDParameters(double x, double y) const override;

    std::string toString(std::string const& prefix) const override {
        std::ostringstream os;
        os << "PolynomialFunction2 [" << this->_order << "]: ";
        os << Function2<ReturnT>::toString(prefix);
        return os.str();
    }

    bool isPersistable() const noexcept override { return true; }

protected:
    std::string getPersistenceName() const override;

    void write(afw::table::io::OutputArchiveHandle& handle) const override;

private:
    mutable double _oldY;                  ///< value of y for which _xCoeffs is valid
    mutable std::vector<double> _xCoeffs;  ///< working vector

protected:
    /* Default constructor: intended only for serialization */
    explicit PolynomialFunction2() : BasePolynomialFunction2<ReturnT>(), _oldY(0), _xCoeffs(0) {}
};

/**
 * 1-dimensional weighted sum of Chebyshev polynomials of the first kind.
 *
 * f(x) = c0 T0(x') + c1 T1(x') + c2 T2(x') + ...
 *      = c0 + c1 T1(x') + c2 T2(x') + ...
 * where:
 * * Tn(x) is the nth Chebyshev function of the first kind:
 *      T0(x) = 1
 *      T1(x) = 2
 *      Tn+1(x) = 2xTn(x) + Tn-1(x)
 * * x' is x offset and scaled to range [-1, 1] as x ranges over [minX, maxX]
 *
 * The function argument must be in the range [minX, maxX].
 *
 * @ingroup afw
 */
template <typename ReturnT>
class Chebyshev1Function1 : public Function1<ReturnT> {
public:
    /**
     * Construct a Chebyshev polynomial of specified order and range.
     *
     * The parameters are initialized to zero.
     */
    explicit Chebyshev1Function1(unsigned int order,  ///< order of polynomial (0 for constant)
                                 double minX = -1,    ///< minimum allowed x
                                 double maxX = 1)     ///< maximum allowed x
            : Function1<ReturnT>(order + 1) {
        _initialize(minX, maxX);
    }

    /**
     * Construct a Chebyshev polynomial with specified parameters and range.
     *
     * The order of the polynomial is set to the length of the params vector.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if params is empty
     */
    explicit Chebyshev1Function1(std::vector<double> params,  ///< polynomial coefficients
                                 double minX = -1,            ///< minimum allowed x
                                 double maxX = 1)             ///< maximum allowed x
            : Function1<ReturnT>(params) {
        if (params.size() < 1) {
            throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterError,
                              "Chebyshev1Function1 called with empty vector");
        }
        _initialize(minX, maxX);
    }

    Chebyshev1Function1(Chebyshev1Function1 const&) = default;
    Chebyshev1Function1(Chebyshev1Function1&&) = default;
    Chebyshev1Function1& operator=(Chebyshev1Function1 const&) = default;
    Chebyshev1Function1& operator=(Chebyshev1Function1&&) = default;
    ~Chebyshev1Function1() noexcept override = default;

    std::shared_ptr<Function1<ReturnT>> clone() const override {
        return std::shared_ptr<Function1<ReturnT>>(new Chebyshev1Function1(this->_params, _minX, _maxX));
    }

    /**
     * Get minimum allowed x
     */
    double getMinX() const noexcept { return _minX; };

    /**
     * Get maximum allowed x
     */
    double getMaxX() const noexcept { return _maxX; };

    /**
     * Get the polynomial order
     */
    unsigned int getOrder() const noexcept { return this->getNParameters() - 1; };

    bool isLinearCombination() const noexcept override { return true; };

    ReturnT operator()(double x) const override {
        double xPrime = (x + _offset) * _scale;

        // Clenshaw function for solving the Chebyshev polynomial
        // Non-recursive version from Kresimir Cosic
        int const order = _order;
        if (order == 0) {
            return this->_params[0];
        } else if (order == 1) {
            return this->_params[0] + (this->_params[1] * xPrime);
        }
        double cshPrev = this->_params[order];
        double csh = (2 * xPrime * this->_params[order]) + this->_params[order - 1];
        for (int i = order - 2; i > 0; --i) {
            double cshNext = (2 * xPrime * csh) + this->_params[i] - cshPrev;
            cshPrev = csh;
            csh = cshNext;
        }
        return (xPrime * csh) + this->_params[0] - cshPrev;
    }

    std::string toString(std::string const& prefix) const override {
        std::ostringstream os;
        os << "Chebyshev1Function1 [" << _minX << ", " << _maxX << "]: ";
        os << Function1<ReturnT>::toString(prefix);
        return os.str();
    }

private:
    double _minX;         ///< minimum allowed x
    double _maxX;         ///< maximum allowed x
    double _scale;        ///< x' = (x + _offset) * _scale
    double _offset;       ///< x' = (x + _offset) * _scale
    unsigned int _order;  ///< polynomial order

    /**
     * initialize private constants
     */
    void _initialize(double minX, double maxX) {
        _minX = minX;
        _maxX = maxX;
        _scale = 2.0 / (_maxX - _minX);
        _offset = -(_minX + _maxX) * 0.5;
        _order = this->getNParameters() - 1;
    }

protected:
    /* Default constructor: intended only for serialization */
    explicit Chebyshev1Function1()
            : Function1<ReturnT>(1), _minX(0.0), _maxX(0.0), _scale(1.0), _offset(0.0), _order(0) {}
};

/**
 * 2-dimensional weighted sum of Chebyshev polynomials of the first kind.
 *
 * f(x,y) = c0 T0(x') T0(y')                                        # order 0
 *        + c1 T1(x') T0(y') + c2 T0(x') T1(y')                     # order 1
 *        + c3 T2(x') T0(y') + c4 T1(x') T1(y') + c5 T0(x') T2(y')  # order 2
 *        + ...
 *
 *        = c0                                                      # order 0
 *        + c1 T1(x') + c2 T1(y')                                   # order 1
 *        + c3 T2(x') + c4 T1(x') T1(y') + c5 T2(y')                # order 2
 *        + ...
 *
 * where:
 * * Tn(x) is the nth Chebyshev function of the first kind:
 *      T0(x) = 1
 *      T1(x) = x
 *      Tn+1(x) = 2xTn(x) + Tn-1(x)
 * * x' is x offset and scaled to range [-1, 1] as x ranges over [minX, maxX]
 * * y' is y offset and scaled to range [-1, 1] as y ranges over [minY, maxY]
 *
 * Return value is incorrect if function arguments are not in the range [minX, maxX], [minY, maxY].
 *
 * @ingroup afw
 */
template <typename ReturnT>
class Chebyshev1Function2 : public BasePolynomialFunction2<ReturnT> {
public:
    /**
     * Construct a Chebyshev polynomial of specified order and range.
     *
     * The parameters are initialized to zero.
     */
    explicit Chebyshev1Function2(unsigned int order,  ///< order of polynomial (0 for constant)
                                 lsst::geom::Box2D const& xyRange = lsst::geom::Box2D(
                                         lsst::geom::Point2D(-1.0, -1.0),
                                         lsst::geom::Point2D(1.0, 1.0)))  ///< allowed x,y range
            : BasePolynomialFunction2<ReturnT>(order),
              _oldYPrime(0),
              _yCheby(this->_order + 1),
              _xCoeffs(this->_order + 1) {
        _initialize(xyRange);
    }

    /**
     * Construct a Chebyshev polynomial with specified parameters and range.
     *
     * The order of the polynomial is set to the length of the params vector.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if params is empty
     */
    explicit Chebyshev1Function2(std::vector<double> params,  ///< polynomial coefficients
                                                              ///< length must be one of 1, 3, 6, 10, 15...
                                 lsst::geom::Box2D const& xyRange = lsst::geom::Box2D(
                                         lsst::geom::Point2D(-1.0, -1.0),
                                         lsst::geom::Point2D(1.0, 1.0)))  ///< allowed x,y range
            : BasePolynomialFunction2<ReturnT>(params),
              _oldYPrime(0),
              _yCheby(this->_order + 1),
              _xCoeffs(this->_order + 1) {
        _initialize(xyRange);
    }

    Chebyshev1Function2(Chebyshev1Function2 const&) = default;
    Chebyshev1Function2(Chebyshev1Function2&&) = default;
    Chebyshev1Function2& operator=(Chebyshev1Function2 const&) = default;
    Chebyshev1Function2& operator=(Chebyshev1Function2&&) = default;
    ~Chebyshev1Function2() noexcept override = default;

    std::shared_ptr<Function2<ReturnT>> clone() const override {
        return std::shared_ptr<Function2<ReturnT>>(
                new Chebyshev1Function2(this->_params, this->getXYRange()));
    }

    /**
     * Get x,y range
     */
    lsst::geom::Box2D getXYRange() const noexcept {
        return lsst::geom::Box2D(lsst::geom::Point2D(_minX, _minY), lsst::geom::Point2D(_maxX, _maxY));
    };

    /**
     * Return a truncated copy of lower (or equal) order
     *
     * @throws lsst::pex::exceptions::InvalidParameter if truncated order > original order
     */
    virtual Chebyshev1Function2 truncate(int truncOrder  ///< order of truncated polynomial
                                         ) const {
        if (truncOrder > this->_order) {
            std::ostringstream os;
            os << "truncated order=" << truncOrder << " must be <= original order=" << this->_order;
            throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterError, os.str());
        }
        int truncNParams = this->nParametersFromOrder(truncOrder);
        std::vector<double> truncParams(this->_params.begin(), this->_params.begin() + truncNParams);
        return Chebyshev1Function2(truncParams, this->getXYRange());
    }

    ReturnT operator()(double x, double y) const override {
        /* Solve as follows:
        - f(x,y) = Cy0 T0(y') + Cy1 T1(y') + Cy2 T2(y') + Cy3 T3(y') + ...
        where:
          Cy0 = P0 T0(x') + P1 T1(x') + P3 T2(x') + P6 T3(x') + ...
          Cy1 = P2 T0(x') + P4 T1(x') + P7 T2(x') + ...
          Cy2 = P5 T0(x') + P8 T1(x') + ...
          Cy3 = P9 T0(x') + ...
          ...

        First compute Tn(x') for each n
        Then use that to compute Cy0, Cy1, ...Cyn
        Then solve the y Chebyshev polynomial using the Clenshaw algorithm
        */
        double const xPrime = (x + _offsetX) * _scaleX;
        double const yPrime = (y + _offsetY) * _scaleY;

        const int nParams = static_cast<int>(this->_params.size());
        const int order = this->_order;

        if (order == 0) {
            return this->_params[0];  // No caching required
        }

        if ((yPrime != _oldYPrime) || !this->_isCacheValid) {
            // update cached _yCheby and _xCoeffs
            _yCheby[0] = 1.0;
            _yCheby[1] = yPrime;
            for (int chebyInd = 2; chebyInd <= order; chebyInd++) {
                _yCheby[chebyInd] = (2 * yPrime * _yCheby[chebyInd - 1]) - _yCheby[chebyInd - 2];
            }

            for (int coeffInd = 0; coeffInd <= order; coeffInd++) {
                _xCoeffs[coeffInd] = 0;
            }
            for (int coeffInd = 0, endCoeffInd = 0, paramInd = 0; paramInd < nParams; paramInd++) {
                _xCoeffs[coeffInd] += this->_params[paramInd] * _yCheby[endCoeffInd];
                --coeffInd;
                ++endCoeffInd;
                if (coeffInd < 0) {
                    coeffInd = endCoeffInd;
                    endCoeffInd = 0;
                }
            }

            _oldYPrime = yPrime;
            this->_isCacheValid = true;
        }

        // Clenshaw function for solving the Chebyshev polynomial
        // Non-recursive version from Kresimir Cosic
        if (order == 1) {
            return _xCoeffs[0] + (_xCoeffs[1] * xPrime);
        }
        double cshPrev = _xCoeffs[order];
        double csh = (2 * xPrime * _xCoeffs[order]) + _xCoeffs[order - 1];
        for (int i = order - 2; i > 0; --i) {
            double cshNext = (2 * xPrime * csh) + _xCoeffs[i] - cshPrev;
            cshPrev = csh;
            csh = cshNext;
        }
        return (xPrime * csh) + _xCoeffs[0] - cshPrev;
    }

    std::string toString(std::string const& prefix) const override {
        std::ostringstream os;
        os << "Chebyshev1Function2 [";
        os << this->_order << ", " << this->getXYRange() << "]";
        os << Function2<ReturnT>::toString(prefix);
        return os.str();
    }

    bool isPersistable() const noexcept override { return true; }

protected:
    std::string getPersistenceName() const override;

    void write(afw::table::io::OutputArchiveHandle& handle) const override;

private:
    mutable double _oldYPrime;
    mutable std::vector<double> _yCheby;   ///< working vector: value of Tn(y')
    mutable std::vector<double> _xCoeffs;  ///< working vector: transformed coeffs of x polynomial
    double _minX;                          ///< minimum allowed x
    double _minY;                          ///< minimum allowed y
    double _maxX;                          ///< maximum allowed x
    double _maxY;                          ///< maximum allowed y
    double _scaleX;                        ///< x' = (x + _offsetX) * _scaleX
    double _scaleY;                        ///< y' = (y + _offsetY) * _scaleY
    double _offsetX;                       ///< x' = (x + _offsetX) * _scaleX
    double _offsetY;                       ///< y' = (y + _offsetY) * _scaleY

    /**
     * initialize private constants
     */
    void _initialize(lsst::geom::Box2D const& xyRange) {
        _minX = xyRange.getMinX();
        _minY = xyRange.getMinY();
        _maxX = xyRange.getMaxX();
        _maxY = xyRange.getMaxY();
        _scaleX = 2.0 / (_maxX - _minX);
        _scaleY = 2.0 / (_maxY - _minY);
        _offsetX = -(_minX + _maxX) * 0.5;
        _offsetY = -(_minY + _maxY) * 0.5;
    }

protected:
    /* Default constructor: intended only for serialization */
    explicit Chebyshev1Function2()
            : BasePolynomialFunction2<ReturnT>(),
              _oldYPrime(0),
              _yCheby(0),
              _xCoeffs(0),
              _minX(0.0),
              _minY(0.0),
              _maxX(0.0),
              _maxY(0.0),
              _scaleX(1.0),
              _scaleY(1.0),
              _offsetX(0.0),
              _offsetY(0.0) {}
};

/**
 * 1-dimensional Lanczos function
 *
 * f(x) = sinc(pi x') sinc(pi x' / n)
 * where x' = x - xOffset
 * and coefficient c0 = xOffset
 *
 * Warning: the Lanczos function is sometimes forced to 0 if |x'| > n
 * but this implementation does not perform that truncation so as to improve Lanczos kernels.
 *
 * @ingroup afw
 */
template <typename ReturnT>
class LanczosFunction1 : public Function1<ReturnT> {
public:
    /**
     * Construct a Lanczos function of specified order and x,y offset.
     */
    explicit LanczosFunction1(unsigned int n,        ///< order of Lanczos function
                              double xOffset = 0.0)  ///< x offset
            : Function1<ReturnT>(1), _invN(1.0 / static_cast<double>(n)) {
        this->_params[0] = xOffset;
    }

    LanczosFunction1(LanczosFunction1 const&) = default;
    LanczosFunction1(LanczosFunction1&&) = default;
    LanczosFunction1& operator=(LanczosFunction1 const&) = default;
    LanczosFunction1& operator=(LanczosFunction1&&) = default;
    ~LanczosFunction1() noexcept override = default;

    std::shared_ptr<Function1<ReturnT>> clone() const override {
        return std::shared_ptr<Function1<ReturnT>>(new LanczosFunction1(this->getOrder(), this->_params[0]));
    }

    ReturnT operator()(double x) const noexcept(IS_NOTHROW_INIT<ReturnT>) override {
        double xArg1 = (x - this->_params[0]) * lsst::geom::PI;
        double xArg2 = xArg1 * _invN;
        if (std::fabs(xArg1) > 1.0e-5) {
            return static_cast<ReturnT>(std::sin(xArg1) * std::sin(xArg2) / (xArg1 * xArg2));
        } else {
            return static_cast<ReturnT>(1);
        }
    }

    /**
     * Get the order of the Lanczos function
     */
    unsigned int getOrder() const noexcept { return static_cast<unsigned int>(0.5 + (1.0 / _invN)); };

    std::string toString(std::string const& prefix) const override {
        std::ostringstream os;
        os << "LanczosFunction1 [" << this->getOrder() << "]: ";
        ;
        os << Function1<ReturnT>::toString(prefix);
        return os.str();
    }

private:
    double _invN;  // == 1/n

protected:
    /* Default constructor: intended only for serialization */
    explicit LanczosFunction1() : Function1<ReturnT>(1), _invN(1.0) {}
};

/**
 * 2-dimensional separable Lanczos function
 *
 * f(x, y) = sinc(pi x') sinc(pi x' / n) sinc(pi y') sinc(pi y' / n)
 * where x' = x - xOffset and y' = y - yOffset
 * and coefficients c0 = xOffset, c1 = yOffset
 *
 * @warning the Lanczos function is sometimes forced to 0 if |x'| > n or |y'| > n
 * but this implementation does not perform that truncation so as to improve Lanczos kernels.
 *
 * @ingroup afw
 */
template <typename ReturnT>
class LanczosFunction2 : public Function2<ReturnT> {
public:
    /**
     * Construct a Lanczos function of specified order and x,y offset.
     */
    explicit LanczosFunction2(unsigned int n,        ///< order of Lanczos function
                              double xOffset = 0.0,  ///< x offset
                              double yOffset = 0.0)  ///< y offset
            : Function2<ReturnT>(2), _invN(1.0 / static_cast<double>(n)) {
        this->_params[0] = xOffset;
        this->_params[1] = yOffset;
    }

    LanczosFunction2(LanczosFunction2 const&) = default;
    LanczosFunction2(LanczosFunction2&&) = default;
    LanczosFunction2& operator=(LanczosFunction2 const&) = default;
    LanczosFunction2& operator=(LanczosFunction2&&) = default;
    ~LanczosFunction2() noexcept override = default;

    std::shared_ptr<Function2<ReturnT>> clone() const override {
        return std::shared_ptr<Function2<ReturnT>>(
                new LanczosFunction2(this->getOrder(), this->_params[0], this->_params[1]));
    }

    ReturnT operator()(double x, double y) const noexcept(IS_NOTHROW_INIT<ReturnT>) override {
        double xArg1 = (x - this->_params[0]) * lsst::geom::PI;
        double xArg2 = xArg1 * _invN;
        double xFunc = 1;
        if (std::fabs(xArg1) > 1.0e-5) {
            xFunc = std::sin(xArg1) * std::sin(xArg2) / (xArg1 * xArg2);
        }
        double yArg1 = (y - this->_params[1]) * lsst::geom::PI;
        double yArg2 = yArg1 * _invN;
        double yFunc = 1;
        if (std::fabs(yArg1) > 1.0e-5) {
            yFunc = std::sin(yArg1) * std::sin(yArg2) / (yArg1 * yArg2);
        }
        return static_cast<ReturnT>(xFunc * yFunc);
    }

    /**
     * Get the order of Lanczos function
     */
    unsigned int getOrder() const noexcept { return static_cast<unsigned int>(0.5 + (1.0 / _invN)); };

    std::string toString(std::string const& prefix) const override {
        std::ostringstream os;
        os << "LanczosFunction2 [" << this->getOrder() << "]: ";
        ;
        os << Function2<ReturnT>::toString(prefix);
        return os.str();
    }

private:
    double _invN;  ///< 1/n

protected:
    /* Default constructor: intended only for serialization */
    explicit LanczosFunction2() : Function2<ReturnT>(2), _invN(1.0) {}
};
}  // namespace math
}  // namespace afw
}  // namespace lsst

#endif  // #ifndef LSST_AFW_MATH_FUNCTIONLIBRARY_H
