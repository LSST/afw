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

#include <nanobind/nanobind.h>
#include <lsst/cpputils/python.h>

#include "ndarray/nanobind.h"

#include "lsst/afw/math/GaussianProcess.h"

namespace nb = nanobind;

using namespace lsst::afw::math;
namespace lsst {
namespace afw {
namespace math {
namespace {
template <typename T>
void declareKdTree(lsst::cpputils::python::WrapperCollection &wrappers, const std::string &suffix) {
    wrappers.wrapType(
            nb::class_<KdTree<T>>(wrappers.module, ("KdTree" + suffix).c_str()), [](auto &mod, auto &cls) {
                cls.def(nb::init<>());
                cls.def("Initialize", &KdTree<T>::Initialize);
                cls.def("removePoint", &KdTree<T>::removePoint);
                cls.def("getData", (T(KdTree<T>::*)(int, int) const) & KdTree<T>::getData);
                cls.def("getData", (ndarray::Array<T, 1, 1>(KdTree<T>::*)(int) const) & KdTree<T>::getData);
                cls.def("addPoint", &KdTree<T>::addPoint);
                cls.def("getNPoints", &KdTree<T>::getNPoints);
                cls.def("getTreeNode", &KdTree<T>::getTreeNode);
                cls.def("findNeighbors", &KdTree<T>::findNeighbors);
            });
};

template <typename T>
void declareCovariograms(lsst::cpputils::python::WrapperCollection &wrappers, const std::string &suffix) {
    /* Covariogram */

    wrappers.wrapType(nb::class_<Covariogram<T>>(
                              wrappers.module, ("Covariogram" + suffix).c_str()),
                      [](auto &mod, auto &cls) {
                          cls.def(nb::init<>());
                          cls.def("__call__", &Covariogram<T>::operator());
                      });
    /* SquaredExpCovariogram */
    wrappers.wrapType(
            nb::class_<SquaredExpCovariogram<T>, Covariogram<T>>(
                    wrappers.module, ("SquaredExpCovariogram" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<>());
                cls.def("__call__", &SquaredExpCovariogram<T>::operator());
                cls.def("setEllSquared", &SquaredExpCovariogram<T>::setEllSquared);
            });
    /* NeuralNetCovariogram */
    wrappers.wrapType(
            nb::class_<NeuralNetCovariogram<T>, Covariogram<T>>(
                    wrappers.module, ("NeuralNetCovariogram" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<>());
                cls.def("setSigma0", &NeuralNetCovariogram<T>::setSigma0);
                cls.def("setSigma1", &NeuralNetCovariogram<T>::setSigma1);
            });
};

template <typename T>
void declareGaussianProcess(lsst::cpputils::python::WrapperCollection &wrappers, const std::string &suffix) {
    wrappers.wrapType(
            nb::class_<GaussianProcess<T>>(wrappers.module, ("GaussianProcess" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                /* Constructors */
                cls.def(nb::init<ndarray::Array<T, 2, 2> const &, ndarray::Array<T, 1, 1> const &,
                                 std::shared_ptr<Covariogram<T>> const &>());
                cls.def(nb::init<ndarray::Array<T, 2, 2> const &, ndarray::Array<T, 1, 1> const &,
                                 ndarray::Array<T, 1, 1> const &, ndarray::Array<T, 1, 1> const &,
                                 std::shared_ptr<Covariogram<T>> const &>());
                cls.def(nb::init<ndarray::Array<T, 2, 2> const &, ndarray::Array<T, 2, 2> const &,
                                 std::shared_ptr<Covariogram<T>> const &>());
                cls.def(nb::init<ndarray::Array<T, 2, 2> const &, ndarray::Array<T, 1, 1> const &,
                                 ndarray::Array<T, 1, 1> const &, ndarray::Array<T, 2, 2> const &,
                                 std::shared_ptr<Covariogram<T>> const &>());
                /* Members */
                cls.def("interpolate",
                        (T(GaussianProcess<T>::*)(ndarray::Array<T, 1, 1>, ndarray::Array<T, 1, 1> const &,
                                                  int) const) &
                                GaussianProcess<T>::interpolate);
                cls.def("interpolate",
                        (void (GaussianProcess<T>::*)(ndarray::Array<T, 1, 1>, ndarray::Array<T, 1, 1>,
                                                      ndarray::Array<T, 1, 1> const &, int) const) &
                                GaussianProcess<T>::interpolate);
                cls.def("selfInterpolate",
                        (T(GaussianProcess<T>::*)(ndarray::Array<T, 1, 1>, int, int) const) &
                                GaussianProcess<T>::selfInterpolate);
                cls.def("selfInterpolate",
                        (void (GaussianProcess<T>::*)(ndarray::Array<T, 1, 1>, ndarray::Array<T, 1, 1>, int,
                                                      int) const) &
                                GaussianProcess<T>::selfInterpolate);
                cls.def("setLambda", &GaussianProcess<T>::setLambda);
                cls.def("setCovariogram", &GaussianProcess<T>::setCovariogram);
                cls.def("addPoint", (void (GaussianProcess<T>::*)(ndarray::Array<T, 1, 1> const &, T)) &
                                            GaussianProcess<T>::addPoint);
                cls.def("addPoint", (void (GaussianProcess<T>::*)(ndarray::Array<T, 1, 1> const &,
                                                                  ndarray::Array<T, 1, 1> const &)) &
                                            GaussianProcess<T>::addPoint);
                cls.def("batchInterpolate",
                        (void (GaussianProcess<T>::*)(ndarray::Array<T, 1, 1>, ndarray::Array<T, 1, 1>,
                                                      ndarray::Array<T, 2, 2> const &) const) &
                                GaussianProcess<T>::batchInterpolate);
                cls.def("batchInterpolate",
                        (void (GaussianProcess<T>::*)(ndarray::Array<T, 1, 1>,
                                                      ndarray::Array<T, 2, 2> const &) const) &
                                GaussianProcess<T>::batchInterpolate);
                cls.def("batchInterpolate",
                        (void (GaussianProcess<T>::*)(ndarray::Array<T, 2, 2>, ndarray::Array<T, 2, 2>,
                                                      ndarray::Array<T, 2, 2> const &) const) &
                                GaussianProcess<T>::batchInterpolate);
                cls.def("batchInterpolate",
                        (void (GaussianProcess<T>::*)(ndarray::Array<T, 2, 2>,
                                                      ndarray::Array<T, 2, 2> const &) const) &
                                GaussianProcess<T>::batchInterpolate);
                cls.def("setKrigingParameter", &GaussianProcess<T>::setKrigingParameter);
                cls.def("removePoint", &GaussianProcess<T>::removePoint);
                cls.def("getNPoints", &GaussianProcess<T>::getNPoints);
                cls.def("getData",
                        (void (GaussianProcess<T>::*)(ndarray::Array<T, 2, 2>, ndarray::Array<T, 1, 1>,
                                                      ndarray::Array<int, 1, 1>) const) &
                                GaussianProcess<T>::getData);
                cls.def("getData",
                        (void (GaussianProcess<T>::*)(ndarray::Array<T, 2, 2>, ndarray::Array<T, 2, 2>,
                                                      ndarray::Array<int, 1, 1>) const) &
                                GaussianProcess<T>::getData);
            });
};
}  // namespace

void wrapGaussianProcess(lsst::cpputils::python::WrapperCollection &wrappers) {
    declareCovariograms<double>(wrappers, "D");
    declareGaussianProcess<double>(wrappers, "D");
    declareKdTree<double>(wrappers, "D");
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
