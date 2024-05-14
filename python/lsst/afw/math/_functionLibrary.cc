/*
 * LSST Data Management System
 * Copyright 2008-2016  AURA/LSST.
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
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */
#include <memory>

#include <nanobind/nanobind.h>
#include <lsst/cpputils/python.h>

#include <nanobind/stl/vector.h>

#include "lsst/geom/Box.h"
#include "lsst/geom/Point.h"

#include "lsst/afw/math/FunctionLibrary.h"
#include "lsst/afw/math/Function.h"

namespace nb = nanobind;
using namespace nanobind::literals;

//NB_DECLARE_HOLDER_TYPE(MyType, std::shared_ptr<MyType>);

namespace lsst {
namespace afw {
namespace math {

template <typename ReturnT>
void declarePolynomialFunctions(lsst::cpputils::python::WrapperCollection &wrappers, const std::string &suffix) {
    /* PolynomialFunction1 */

    wrappers.wrapType(
            nb::class_<PolynomialFunction1<ReturnT>,
                       Function1<ReturnT>>(wrappers.module, ("PolynomialFunction1" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<std::vector<double> const &>(), "params"_a);
                cls.def(nb::init<unsigned int>(), "order"_a);

                cls.def("__call__", &PolynomialFunction1<ReturnT>::operator(), "x"_a);
                cls.def("clone", &PolynomialFunction1<ReturnT>::clone);
                cls.def("isLinearCombination", &PolynomialFunction1<ReturnT>::isLinearCombination);
                cls.def("getOrder", &PolynomialFunction1<ReturnT>::getOrder);
                cls.def("toString", &PolynomialFunction1<ReturnT>::toString, "prefix"_a = "");
            });
    /* PolynomialFunction2 */
    wrappers.wrapType(nb::class_<PolynomialFunction2<ReturnT>,
                                 BasePolynomialFunction2<ReturnT>>(wrappers.module,
                                                                   ("PolynomialFunction2" + suffix).c_str()),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<std::vector<double> const &>(), "params"_a);
                          cls.def(nb::init<unsigned int>(), "order"_a);

                          cls.def("__call__", &PolynomialFunction2<ReturnT>::operator(), "x"_a, "y"_a);
                          cls.def("clone", &PolynomialFunction2<ReturnT>::clone);
                          cls.def("getOrder", &PolynomialFunction2<ReturnT>::getOrder);
                          cls.def("getDFuncDParameters", &PolynomialFunction2<ReturnT>::getDFuncDParameters);
                          cls.def("toString", &PolynomialFunction2<ReturnT>::toString, "prefix"_a = "");
                          cls.def("isPersistable", &PolynomialFunction2<ReturnT>::isPersistable);
                      });
};

template <typename ReturnT>
void declareChebyshevFunctions(lsst::cpputils::python::WrapperCollection &wrappers, const std::string &suffix) {
    /* Chebyshev1Function1 */

    wrappers.wrapType(
            nb::class_<Chebyshev1Function1<ReturnT>,
                       Function1<ReturnT>>(wrappers.module, ("Chebyshev1Function1" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<std::vector<double>, double, double>(), "params"_a, "minX"_a = -1,
                        "maxX"_a = 1);
                cls.def(nb::init<unsigned int, double, double>(), "order"_a, "minX"_a = -1, "maxX"_a = 1);

                cls.def("__call__", &Chebyshev1Function1<ReturnT>::operator(), "x"_a);
                cls.def("clone", &Chebyshev1Function1<ReturnT>::clone);
                cls.def("getMinX", &Chebyshev1Function1<ReturnT>::getMinX);
                cls.def("getMaxX", &Chebyshev1Function1<ReturnT>::getMaxX);
                cls.def("getOrder", &Chebyshev1Function1<ReturnT>::getOrder);
                cls.def("isLinearCombination", &Chebyshev1Function1<ReturnT>::isLinearCombination);
                cls.def("toString", &Chebyshev1Function1<ReturnT>::toString, "prefix"_a = "");

                /* Chebyshev1Function2 */
            });
    wrappers.wrapType(nb::class_<Chebyshev1Function2<ReturnT>,
                                 BasePolynomialFunction2<ReturnT>>(wrappers.module,
                                                                   ("Chebyshev1Function2" + suffix).c_str()),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<std::vector<double>, lsst::geom::Box2D const &>(), "params"_a,
                                  "xyRange"_a = lsst::geom::Box2D(lsst::geom::Point2D(-1.0, -1.0),
                                                                  lsst::geom::Point2D(1.0, 1.0)));
                          cls.def(nb::init<unsigned int, lsst::geom::Box2D const &>(), "order"_a,
                                  "xyRange"_a = lsst::geom::Box2D(lsst::geom::Point2D(-1.0, -1.0),
                                                                  lsst::geom::Point2D(1.0, 1.0)));

                          cls.def("__call__", &Chebyshev1Function2<ReturnT>::operator(), "x"_a, "y"_a);
                          cls.def("clone", &Chebyshev1Function2<ReturnT>::clone);
                          cls.def("getXYRange", &Chebyshev1Function2<ReturnT>::getXYRange);
                          cls.def("truncate", &Chebyshev1Function2<ReturnT>::truncate, "order"_a);
                          cls.def("toString", &Chebyshev1Function2<ReturnT>::toString, "prefix"_a = "");
                          cls.def("isPersistable", &Chebyshev1Function2<ReturnT>::isPersistable);
                      });
};

template <typename ReturnT>
void declareGaussianFunctions(lsst::cpputils::python::WrapperCollection &wrappers, const std::string &suffix) {
    /* GaussianFunction1 */
    wrappers.wrapType(nb::class_<GaussianFunction1<ReturnT>,
                                 Function1<ReturnT>>(wrappers.module, ("GaussianFunction1" + suffix).c_str()),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<double>(), "sigma"_a);

                          cls.def("__call__", &GaussianFunction1<ReturnT>::operator(), "x"_a);
                          cls.def("clone", &GaussianFunction1<ReturnT>::clone);
                          cls.def("toString", &GaussianFunction1<ReturnT>::toString, "prefix"_a = "");
                      });

    wrappers.wrapType(nb::class_<GaussianFunction2<ReturnT>,
                                 Function2<ReturnT>>(wrappers.module, ("GaussianFunction2" + suffix).c_str()),
                      [](auto &mod, auto &cls) {
                          /* GaussianFunction2 */

                          cls.def(nb::init<double, double, double>(), "sigma1"_a, "sigma2"_a,
                                  "angle"_a = 0.0);

                          cls.def("__call__", &GaussianFunction2<ReturnT>::operator(), "x"_a, "y"_a);
                          cls.def("clone", &GaussianFunction2<ReturnT>::clone);
                          cls.def("toString", &GaussianFunction2<ReturnT>::toString, "prefix"_a = "");
                          cls.def("isPersistable", &GaussianFunction2<ReturnT>::isPersistable);
                      });
    /* DoubleGaussianFunction2 */

    wrappers.wrapType(
            nb::class_<DoubleGaussianFunction2<ReturnT>,
                       Function2<ReturnT>>(wrappers.module, ("DoubleGaussianFunction2" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<double, double, double>(), "sigma1"_a, "sigma2"_a = 0, "ampl"_a = 0);

                cls.def("__call__", &DoubleGaussianFunction2<ReturnT>::operator(), "x"_a, "y"_a);
                cls.def("clone", &DoubleGaussianFunction2<ReturnT>::clone);
                cls.def("toString", &DoubleGaussianFunction2<ReturnT>::toString, "prefix"_a = "");
                cls.def("isPersistable", &DoubleGaussianFunction2<ReturnT>::isPersistable);
            });
};

template <typename ReturnT>
void declareIntegerDeltaFunctions(lsst::cpputils::python::WrapperCollection &wrappers,
                                  const std::string &suffix) {
    /* IntegerDeltaFunction1 */

    wrappers.wrapType(
            nb::class_<IntegerDeltaFunction1<ReturnT>,
                       Function1<ReturnT>>(wrappers.module, ("IntegerDeltaFunction1" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<double>(), "xo"_a);

                cls.def("__call__", &IntegerDeltaFunction1<ReturnT>::operator(), "x"_a);
                cls.def("clone", &IntegerDeltaFunction1<ReturnT>::clone);
                cls.def("toString", &IntegerDeltaFunction1<ReturnT>::toString, "prefix"_a = "");
            });
    /* IntegerDeltaFunction2 */

    wrappers.wrapType(
            nb::class_<IntegerDeltaFunction2<ReturnT>,
                       Function2<ReturnT>>(wrappers.module, ("IntegerDeltaFunction2" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<double, double>(), "xo"_a, "yo"_a);

                cls.def("__call__", &IntegerDeltaFunction2<ReturnT>::operator(), "x"_a, "y"_a);
                cls.def("clone", &IntegerDeltaFunction2<ReturnT>::clone);
                cls.def("toString", &IntegerDeltaFunction2<ReturnT>::toString, "prefix"_a = "");
            });
};

template <typename ReturnT>
void declareLanczosFunctions(lsst::cpputils::python::WrapperCollection &wrappers, const std::string &suffix) {
    /* LanczosFunction1 */

    wrappers.wrapType(nb::class_<LanczosFunction1<ReturnT>,
                                 Function1<ReturnT>>(wrappers.module, ("LanczosFunction1" + suffix).c_str()),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<unsigned int, double>(), "n"_a, "xOffset"_a = 0.0);

                          cls.def("__call__", &LanczosFunction1<ReturnT>::operator(), "x"_a);
                          cls.def("clone", &LanczosFunction1<ReturnT>::clone);
                          cls.def("getOrder", &LanczosFunction1<ReturnT>::getOrder);
                          cls.def("toString", &LanczosFunction1<ReturnT>::toString, "prefix"_a = "");
                      });
    /* LanczosFunction2 */

    wrappers.wrapType(nb::class_<LanczosFunction2<ReturnT>,
                                 Function2<ReturnT>>(wrappers.module, ("LanczosFunction2" + suffix).c_str()),
                      [](auto &mod, auto &cls) {
                          /* LanczosFunction2 */
                          cls.def(nb::init<unsigned int, double, double>(), "n"_a, "xOffset"_a = 0.0,
                                  "yOffset"_a = 0.0);

                          cls.def("__call__", &LanczosFunction2<ReturnT>::operator(), "x"_a, "y"_a);
                          cls.def("clone", &LanczosFunction2<ReturnT>::clone);
                          cls.def("getOrder", &LanczosFunction2<ReturnT>::getOrder);
                          cls.def("toString", &LanczosFunction2<ReturnT>::toString, "prefix"_a = "");
                      });
};

void wrapFunctionLibrary(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addInheritanceDependency("lsst.geom");
    declarePolynomialFunctions<float>(wrappers, "F");
    declareChebyshevFunctions<float>(wrappers, "F");
    declareGaussianFunctions<float>(wrappers, "F");
    declareIntegerDeltaFunctions<float>(wrappers, "F");
    declareLanczosFunctions<float>(wrappers, "F");

    declarePolynomialFunctions<double>(wrappers, "D");
    declareChebyshevFunctions<double>(wrappers, "D");
    declareGaussianFunctions<double>(wrappers, "D");
    declareIntegerDeltaFunctions<double>(wrappers, "D");
    declareLanczosFunctions<double>(wrappers, "D");
}

}  // namespace math
}  // namespace afw
}  // namespace lsst
