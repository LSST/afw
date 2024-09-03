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
#include <nanobind/stl/vector.h>
#include <nanobind/stl/string.h>

#include "lsst/afw/image/Image.h"
#include "lsst/afw/math/Random.h"

namespace nb = nanobind;

using namespace lsst::afw::math;
using namespace nanobind::literals;
namespace lsst {
namespace afw {
namespace math {
template <typename ImageT>
void declareRandomImage(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("randomUniformImage", (void (*)(ImageT *, Random &))randomUniformImage<ImageT>);
        mod.def("randomUniformPosImage", (void (*)(ImageT *, Random &))randomUniformPosImage<ImageT>);
        mod.def("randomUniformIntImage",
                (void (*)(ImageT *, Random &, unsigned long))randomUniformIntImage<ImageT>);
        mod.def("randomFlatImage",
                (void (*)(ImageT *, Random &, double const, double const))randomFlatImage<ImageT>);
        mod.def("randomGaussianImage", (void (*)(ImageT *, Random &))randomGaussianImage<ImageT>);
        mod.def("randomChisqImage", (void (*)(ImageT *, Random &, double const))randomChisqImage<ImageT>);
        mod.def("randomPoissonImage", (void (*)(ImageT *, Random &, double const))randomPoissonImage<ImageT>);
    });
}

void declareRandom(lsst::cpputils::python::WrapperCollection &wrappers) {
    
    auto clsRandom = nb::class_<Random>(wrappers.module, "Random");
    wrappers.wrapType(nb::enum_<Random::Algorithm>(clsRandom, "Algorithm"), [](auto &mod, auto &enm) {
        enm.value("MT19937", Random::Algorithm::MT19937);
        enm.value("RANLXS0", Random::Algorithm::RANLXS0);
        enm.value("RANLXS1", Random::Algorithm::RANLXS1);
        enm.value("RANLXS2", Random::Algorithm::RANLXS2);
        enm.value("RANLXD1", Random::Algorithm::RANLXD1);
        enm.value("RANLXD2", Random::Algorithm::RANLXD2);
        enm.value("RANLUX", Random::Algorithm::RANLUX);
        enm.value("RANLUX389", Random::Algorithm::RANLUX389);
        enm.value("CMRG", Random::Algorithm::CMRG);
        enm.value("MRG", Random::Algorithm::MRG);
        enm.value("TAUS", Random::Algorithm::TAUS);
        enm.value("TAUS2", Random::Algorithm::TAUS2);
        enm.value("GFSR4", Random::Algorithm::GFSR4);
        enm.value("NUM_ALGORITHMS", Random::Algorithm::NUM_ALGORITHMS);
        enm.export_values();
    });

    wrappers.wrapType(clsRandom, [](auto &mod,
                                                                                         auto &cls) {
        /* Constructors */
        cls.def(nb::init<Random::Algorithm, unsigned long>(), "algorithm"_a = Random::Algorithm::MT19937,
                "seed"_a = 1);
        cls.def(nb::init<std::string const &, unsigned long>(), "algorithm"_a, "seed"_a = 1);

        /* Members */
        cls.def("deepCopy", &Random::deepCopy);
        cls.def("getAlgorithm", &Random::getAlgorithm);
        cls.def("getAlgorithmName", &Random::getAlgorithmName);
        cls.def_static("getAlgorithmNames", &Random::getAlgorithmNames);
        cls.def("getSeed", &Random::getSeed);
        cls.def("uniform", &Random::uniform);
        cls.def("uniformPos", &Random::uniformPos);
        cls.def("uniformInt", &Random::uniformInt);
        cls.def("flat", &Random::flat);
        cls.def("gaussian", &Random::gaussian);
        cls.def("chisq", &Random::chisq);
        cls.def("poisson", &Random::poisson);

        // getState and setState are special, their std::string cannot
        // be converted to a Python string (needs to go to bytes instead)
        // thus use the same solution as employed with Swig
        cls.def("getState", [](Random &self) -> nb::object {
            std::string state = self.getState();
            return nb::steal<nb::object>(PyBytes_FromStringAndSize(state.data(), state.size()));
        });
        cls.def("setState", [](Random &self, nb::bytes const &state) { self.setState(std::string(state.c_str(), state.size())); });
    });
}

void wrapRandom(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.afw.image");
    declareRandom(wrappers);
    declareRandomImage<lsst::afw::image::Image<double>>(wrappers);
    declareRandomImage<lsst::afw::image::Image<float>>(wrappers);
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
