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

#ifndef LSST_AFW_MATH_FUNCTION_H
#define LSST_AFW_MATH_FUNCTION_H
/*
 * Define the basic Function classes.
 */
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <vector>

#include "boost/format.hpp"

#include "lsst/pex/exceptions.h"

#include "lsst/afw/table/io/Persistable.h"

namespace lsst {
namespace afw {
namespace math {

/** Test that a Function's return value is nothrow-castable to T
 *
 * std::complex is an example of a numeric type that does not satisfy
 * this requirement.
 */
template <typename T>
bool constexpr IS_NOTHROW_INIT = noexcept(static_cast<T>(1.0));

/**
 * Basic Function class.
 *
 * Function objects are functions whose parameters may be read and changed using
 * getParameters and setParameters. They were designed for use with the Kernel class.
 *
 * These are simple functors with the restrictions that:
 * - Function arguments and parameters are double precision
 * - The return type is templated
 *
 * To create a function for a particular equation, subclass Function
 * or (much more likely) Function1 or Function2. Your subclass must:
 * - Have one or more constructors, all of which must initialize _params
 * - Define operator() with code to compute the function
 *   using this->_params or this->getParams() to reference the parameters
 * - If the function is a linear combination of parameters then override the function isLinearCombination.
 *
 * If you wish to cache any information you may use the _isCacheValid flag;
 * this is automatically set false whenever parameters are changed.
 *
 * Design Notes:
 * The reason these functions exist (rather than using a pre-existing function class,
 * such as Functor in VisualWorkbench) is because the Kernel class requires function
 * objects with a standard interface for setting and getting function parameters.
 *
 * The reason isLinearCombination exists is to support refactoring LinearCombinationKernels.
 *
 * @ingroup afw
 */
template <typename ReturnT>
class Function : public afw::table::io::Persistable {
using FunctionType = Function<ReturnT>;
DECLARE_PERSISTABLE_FACADE(FunctionType);
public:
    /**
     * Construct a Function given the number of function parameters.
     *
     * The function parameters are initialized to 0.
     */
    explicit Function(unsigned int nParams)  ///< number of function parameters
            : _params(nParams), _isCacheValid(false) {}

    /**
     * Construct a Function given the function parameters.
     */
    explicit Function(std::vector<double> const& params)  ///< function parameters
            : _params(params), _isCacheValid(false) {}

    Function(Function const&) = default;
    Function(Function&&) = default;
    Function& operator=(Function const&) = default;
    Function& operator=(Function&&) = default;

    ~Function() noexcept override = default;

    /**
     * Return the number of function parameters
     *
     * @returns the number of function parameters
     */
    unsigned int getNParameters() const noexcept { return _params.size(); }

    /**
     * Get one function parameter without range checking
     *
     * @returns the specified function parameter
     */
    virtual double getParameter(unsigned int ind)  ///< index of parameter
            const {
        return _params[ind];
    }

    /**
     * Return all function parameters
     *
     * @returns the function parameters as a vector
     */
    std::vector<double> const& getParameters() const noexcept { return _params; }

    /**
     * Is the function a linear combination of its parameters?
     *
     * @returns true if the function can be expressed as: sum over i of parameter_i * function_i(args)
     *
     * @warning: subclasses must override if true.
     */
    virtual bool isLinearCombination() const noexcept { return false; }

    /**
     * Set one function parameter without range checking
     */
    void setParameter(unsigned int ind,  ///< index of parameter
                      double newValue)   ///< new value for parameter
    {
        _isCacheValid = false;
        _params[ind] = newValue;
    }

    /**
     * Set all function parameters
     *
     * @throws lsst::pex::exceptions::InvalidParameterError
     *        if the wrong number of parameters is supplied.
     */
    void setParameters(std::vector<double> const& params)  ///< vector of function parameters
    {
        if (_params.size() != params.size()) {
            throw LSST_EXCEPT(
                    pexExcept::InvalidParameterError,
                    (boost::format("params has %d entries instead of %d") % params.size() % _params.size())
                            .str());
        }
        _isCacheValid = false;
        _params = params;
    }

    /**
     * Return a string representation of the function
     *
     * @returns a string representation of the function
     */
    virtual std::string toString(std::string const&) const {
        std::stringstream os;
        os << "parameters: [ ";
        for (std::vector<double>::const_iterator i = _params.begin(); i != _params.end(); ++i) {
            if (i != _params.begin()) os << ", ";
            os << *i;
        }
        os << " ]";
        return os.str();
    }

protected:
    std::vector<double> _params;
    mutable bool _isCacheValid;

    std::string getPythonModule() const override { return "lsst.afw.math"; }

    /* Default constructor: intended only for serialization */
    explicit Function() : _params(0), _isCacheValid(false) {}
};

/**
 * A Function taking one argument.
 *
 * Subclass and override operator() to do useful work.
 *
 * @ingroup afw
 */
template <typename ReturnT>
class Function1 : public Function<ReturnT> {
using FunctionType = Function<ReturnT>;
DECLARE_PERSISTABLE_FACADE(FunctionType);
public:
    /**
     * Construct a Function1 given the number of function parameters.
     *
     * The function parameters are initialized to 0.
     */
    explicit Function1(unsigned int nParams)  ///< number of function parameters
            : Function<ReturnT>(nParams) {}

    /**
     * Construct a Function1 given the function parameters.
     */
    explicit Function1(std::vector<double> const& params)  ///< function parameters
            : Function<ReturnT>(params) {}

    Function1(Function1 const&) = default;
    Function1(Function1&&) = default;
    Function1& operator=(Function1 const&) = default;
    Function1& operator=(Function1&&) = default;

    ~Function1() noexcept override = default;

    /**
     * Return a pointer to a deep copy of this function
     *
     * This function exists instead of a copy constructor
     * so one can obtain a copy of an actual function
     * instead of a useless copy of the base class.
     *
     * Every concrete subclass must override this method.
     *
     * @returns a pointer to a deep copy of the function
     */
    virtual std::shared_ptr<Function1<ReturnT>> clone() const = 0;

    virtual ReturnT operator()(double x) const = 0;

    std::string toString(std::string const& prefix = "") const override {
        return std::string("Function1: ") + Function<ReturnT>::toString(prefix);
    }

    virtual void computeCache(int const n) {}

protected:
    /* Default constructor: intended only for serialization */
    explicit Function1() : Function<ReturnT>() {}
};

/**
 * A Function taking two arguments.
 *
 * Subclass and override operator() to do useful work.
 *
 * @ingroup afw
 */
template <typename ReturnT>
class Function2 : public Function<ReturnT> {
using FunctionType = Function2<ReturnT>;
DECLARE_PERSISTABLE_FACADE(FunctionType);
public:
    /**
     * Construct a Function2 given the number of function parameters.
     *
     * The function parameters are initialized to 0.
     */
    explicit Function2(unsigned int nParams)  ///< number of function parameters
            : Function<ReturnT>(nParams) {}

    /**
     * Construct a Function2 given the function parameters.
     *
     * The number of function parameters is set to the length of params.
     */
    explicit Function2(std::vector<double> const& params)  ///< function parameters
            : Function<ReturnT>(params) {}

    Function2(Function2 const&) = default;
    Function2(Function2&&) = default;
    Function2& operator=(Function2 const&) = default;
    Function2& operator=(Function2&&) = default;

    ~Function2() noexcept override = default;

    /**
     * Return a pointer to a deep copy of this function
     *
     * This function exists instead of a copy constructor
     * so one can obtain a copy of an actual function
     * instead of a useless copy of the base class.
     *
     * Every non-virtual function must override this method.
     *
     * @returns a pointer to a deep copy of the function
     */
    virtual std::shared_ptr<Function2<ReturnT>> clone() const = 0;

    virtual ReturnT operator()(double x, double y) const = 0;

    std::string toString(std::string const& prefix = "") const override {
        return std::string("Function2: ") + Function<ReturnT>::toString(prefix);
    }
    /**
     * Return the derivative of the Function with respect to its parameters
     */
    virtual std::vector<double> getDFuncDParameters(double, double) const {
        throw LSST_EXCEPT(lsst::pex::exceptions::NotFoundError,
                          "getDFuncDParameters is not implemented for this class");
    }

protected:
    /* Default constructor: intended only for serialization */
    explicit Function2() : Function<ReturnT>() {}
};

/**
 * Base class for 2-dimensional polynomials of the form:
 *
 *     f(x,y) =   c0 f0(x) f0(y)                                        (0th order)
 *              + c1 f1(x) f0(x) + c2 f0(x) f1(y)                       (1st order)
 *              + c3 f2(x) f0(y) + c4 f1(x) f1(y) + c5 f0(x) f2(y)      (2nd order)
 *              + ...
 *
 * and typically f0(x) = 1
 */
template <typename ReturnT>
class BasePolynomialFunction2 : public Function2<ReturnT> {
public:
    /**
     * Construct a polynomial function of specified order.
     *
     * The polynomial will have (order + 1) * (order + 2) / 2 coefficients
     *
     * The parameters are initialized to zero.
     */
    explicit BasePolynomialFunction2(unsigned int order)  ///< order of polynomial (0 for constant)
            : Function2<ReturnT>(BasePolynomialFunction2::nParametersFromOrder(order)), _order(order) {}

    /**
     * Construct a polynomial function with specified parameters.
     *
     * The order of the polynomial is determined from the length of the params vector
     * (see orderFromNParameters) and only certain lengths are suitable: 1, 3, 6, 10, 15...
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if params length is unsuitable
     */
    explicit BasePolynomialFunction2(std::vector<double> params)  ///< polynomial coefficients
            : Function2<ReturnT>(params),
              _order(BasePolynomialFunction2::orderFromNParameters(static_cast<int>(params.size()))) {}

    BasePolynomialFunction2(BasePolynomialFunction2 const&) = default;
    BasePolynomialFunction2(BasePolynomialFunction2&&) = default;
    BasePolynomialFunction2& operator=(BasePolynomialFunction2 const&) = default;
    BasePolynomialFunction2& operator=(BasePolynomialFunction2&&) = default;

    ~BasePolynomialFunction2() noexcept override = default;

    /**
     * Get the polynomial order
     */
    int getOrder() const noexcept { return _order; }

    bool isLinearCombination() const noexcept override { return true; }

    /**
     * Compute number of parameters from polynomial order.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if order < 0
     */
    static int nParametersFromOrder(int order) {
        if (order < 0) {
            std::ostringstream os;
            os << "order=" << order << " invalid: must be >= 0";
            throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterError, os.str());
        }
        return (order + 1) * (order + 2) / 2;
    }

    /**
     * Compute polynomial order from the number of parameters
     *
     * Only certain values of nParameters are acceptable, including:
     * nParameters order
     *      1        0
     *      3        1
     *      6        2
     *     10        3
     *     15        4
     *    ...
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if nParameters is invalid
     */
    static int orderFromNParameters(int nParameters) {
        int order = static_cast<int>(
                0.5 + ((-3.0 + (std::sqrt(1.0 + (8.0 * static_cast<double>(nParameters))))) / 2.0));
        if (nParameters != BasePolynomialFunction2::nParametersFromOrder(order)) {
            std::ostringstream os;
            os << "nParameters=" << nParameters << " invalid: order is not an integer";
            throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterError, os.str());
        }
        return order;
    }

    /**
     * Return the derivative of the Function with respect to its parameters
     *
     * Because this is a polynomial, c0 F0(x,y) + c1 F1(x,y) + c2 F2(x,y) + ...
     * we can set ci = 0 for all i except the parameter of interest and evaluate.
     * This isn't necessarily the most efficient algorithm, but it's general,
     * and you can override it if it isn't suitable for your particular subclass.
     */
    std::vector<double> getDFuncDParameters(double x, double y) const override {
        unsigned int const numParams = this->getNParameters();  // Number of parameters
        std::vector<double> deriv(numParams);                   // Derivatives, to return

        std::shared_ptr<Function2<ReturnT>> dummy =
                this->clone();  // Dummy function to evaluate for derivatives
        for (unsigned int i = 0; i < numParams; ++i) {
            dummy->setParameter(i, 0.0);
        }

        for (unsigned int i = 0; i < numParams; ++i) {
            dummy->setParameter(i, 1.0);
            deriv[i] = (*dummy)(x, y);
            dummy->setParameter(i, 0.0);
        }

        return deriv;
    }

protected:
    int _order;  ///< order of polynomial

    /* Default constructor: intended only for serialization */
    explicit BasePolynomialFunction2() : Function2<ReturnT>(1), _order(0) {}
};

/**
 * a class used in function calls to indicate that no Function1 is being provided
 */
template <typename ReturnT>
class NullFunction1 : public Function1<ReturnT> {
public:
    explicit NullFunction1() : Function1<ReturnT>(0) {}
    std::shared_ptr<Function1<ReturnT>> clone() const override {
        return std::shared_ptr<Function1<ReturnT>>(new NullFunction1());
    }

private:
    ReturnT operator()(double) const noexcept(IS_NOTHROW_INIT<ReturnT>) override {
        return static_cast<ReturnT>(0);
    }
};

/**
 * a class used in function calls to indicate that no Function2 is being provided
 */
template <typename ReturnT>
class NullFunction2 : public Function2<ReturnT> {
public:
    explicit NullFunction2() : Function2<ReturnT>(0) {}
    std::shared_ptr<Function2<ReturnT>> clone() const override {
        return std::shared_ptr<Function2<ReturnT>>(new NullFunction2());
    }

private:
    ReturnT operator()(double, double) const noexcept(IS_NOTHROW_INIT<ReturnT>) override {
        return static_cast<ReturnT>(0);
    }
};
}  // namespace math
}  // namespace afw
}  // namespace lsst

#endif  // #ifndef LSST_AFW_MATH_FUNCTION_H
