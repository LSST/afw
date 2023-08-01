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
#include <string>

#include <pybind11/pybind11.h>
#include <lsst/utils/python.h>
#include <pybind11/stl.h>

#include "lsst/afw/table/io/python.h"  // for addPersistableMethods
#include "lsst/afw/math/Function.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace afw {
namespace math {

namespace {

template <typename ReturnT>
void declareFunction(lsst::utils::python::WrapperCollection &wrappers, std::string const &suffix) {
    auto const name = "Function" + suffix;
    wrappers.wrapType(
            py::class_<Function<ReturnT>>(wrappers.module, name.c_str()),
            [](auto &mod, auto &cls) {
                cls.def(py::init<unsigned int>(), "nParams"_a);
                cls.def(py::init<std::vector<double> const &>(), "params"_a);

                table::io::python::addPersistableMethods<Function<ReturnT>>(cls);

                cls.def("getNParameters", &Function<ReturnT>::getNParameters);
                cls.def("getParameters", &Function<ReturnT>::getParameters, py::return_value_policy::copy);
                cls.def("getParameter", &Function<ReturnT>::getParameter, "index"_a);
                cls.def("isLinearCombination", &Function<ReturnT>::isLinearCombination);
                cls.def("setParameter", &Function<ReturnT>::setParameter, "index"_a, "value"_a);
                cls.def("setParameters", &Function<ReturnT>::setParameters);
                cls.def("toString", &Function<ReturnT>::toString, "prefix"_a = "");
            });
}

template <typename ReturnT>
void declareFunction1(lsst::utils::python::WrapperCollection &wrappers, const std::string &suffix) {
    auto const name = "Function1" + suffix;
    using PyClass = py::class_<Function1<ReturnT>, Function<ReturnT>>;
    wrappers.wrapType(PyClass(wrappers.module, name.c_str()), [](auto &mod, auto &cls) {
        table::io::python::addPersistableMethods<Function1<ReturnT>>(cls);

        cls.def("clone", &Function1<ReturnT>::clone);
        cls.def("__call__", &Function1<ReturnT>::operator(), "x"_a);
        cls.def("toString", &Function1<ReturnT>::toString, "prefix"_a = "");
        cls.def("computeCache", &Function1<ReturnT>::computeCache, "n"_a);
    });
}

template <typename ReturnT>
void declareFunction2(lsst::utils::python::WrapperCollection &wrappers, const std::string &suffix) {
    auto const name = "Function2" + suffix;
    using PyClass = py::class_<Function2<ReturnT>, Function<ReturnT>>;
    wrappers.wrapType(PyClass(wrappers.module, name.c_str()), [](auto &mod, auto &cls) {
        table::io::python::addPersistableMethods<Function2<ReturnT>>(cls);

        cls.def("clone", &Function2<ReturnT>::clone);
        cls.def("__call__", &Function2<ReturnT>::operator(), "x"_a, "y"_a);
        cls.def("toString", &Function2<ReturnT>::toString, "prefix"_a = "");
        cls.def("getDFuncDParameters", &Function2<ReturnT>::getDFuncDParameters, "x"_a, "y"_a);
    });
}

template <typename ReturnT>
void declareBasePolynomialFunction2(lsst ::utils::python::WrapperCollection &wrappers,
                                    const std::string &suffix) {
    auto const name = "BasePolynomialFunction2" + suffix;
    using PyClass = py::class_<BasePolynomialFunction2<ReturnT>, Function2<ReturnT>>;
    wrappers.wrapType(PyClass(wrappers.module, name.c_str()), [](auto &mod, auto &cls) {
        cls.def("getOrder", &BasePolynomialFunction2<ReturnT>::getOrder);
        cls.def("isLinearCombination", &BasePolynomialFunction2<ReturnT>::isLinearCombination);
        cls.def_static("nParametersFromOrder", &BasePolynomialFunction2<ReturnT>::nParametersFromOrder,
                       "order"_a);
        cls.def_static("orderFromNParameters", &BasePolynomialFunction2<ReturnT>::orderFromNParameters,
                       "nParameters"_a);
        cls.def("getDFuncDParameters", &BasePolynomialFunction2<ReturnT>::getDFuncDParameters, "x"_a, "y"_a);
    });
}

template <typename ReturnT>
void declareNullFunction1(lsst::utils::python::WrapperCollection &wrappers, const std::string &suffix) {
    auto const name = "NullFunction1" + suffix;
    using PyClass =
            py::class_<NullFunction1<ReturnT>, Function1<ReturnT>>;
    wrappers.wrapType(PyClass(wrappers.module, name.c_str()), [](auto &mod, auto &cls) {
        cls.def(py::init<>());

        cls.def("clone", &NullFunction1<ReturnT>::clone);
    });
}

template <typename ReturnT>
void declareNullFunction2(lsst::utils::python::WrapperCollection &wrappers, const std::string &suffix) {
    auto const name = "NullFunction2" + suffix;
    using PyClass =
            py::class_<NullFunction2<ReturnT>, Function2<ReturnT>>;
    wrappers.wrapType(PyClass(wrappers.module, name.c_str()), [](auto &mod, auto &cls) {
        cls.def(py::init<>());
        cls.def("clone", &NullFunction2<ReturnT>::clone);
    });
}

template <typename ReturnT>
void declareAllFunctions(lsst::utils::python::WrapperCollection &wrappers, const std::string &suffix) {
    declareFunction<ReturnT>(wrappers, suffix);
    declareFunction1<ReturnT>(wrappers, suffix);
    declareFunction2<ReturnT>(wrappers, suffix);
    declareBasePolynomialFunction2<ReturnT>(wrappers, suffix);
    declareNullFunction1<ReturnT>(wrappers, suffix);
    declareNullFunction2<ReturnT>(wrappers, suffix);
};

}  // namespace

void wrapFunction(lsst::utils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.table.io");
    declareAllFunctions<float>(wrappers, "F");
    declareAllFunctions<double>(wrappers, "D");
}

}  // namespace math
}  // namespace afw
}  // namespace lsst