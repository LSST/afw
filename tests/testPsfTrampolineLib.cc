/*
 * This file is part of afw.
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Testing helper module that let's us conveniently invoke c++ methods that should
// get overridden in python-derived subclasses of Psf.

#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include "lsst/afw/detection/Psf.h"
#include "lsst/afw/typehandling/Storable.h"

namespace lsst {
namespace afw {
namespace detection {

namespace {

std::shared_ptr<Psf> resizedPsf(const Psf& psf, int nx, int ny) {
    return psf.resized(nx, ny);
}

std::shared_ptr<Psf> clonedPsf(const Psf& psf) {
    return psf.clone();
}

std::shared_ptr<typehandling::Storable> clonedStorablePsf(const typehandling::Storable& psf) {
    return psf.cloneStorable();
}

bool isPersistable(const typehandling::Storable& psf) {
    return psf.isPersistable();
}

}

NB_MODULE(testPsfTrampolineLib, mod) {
    mod.def("resizedPsf", &resizedPsf);
    mod.def("clonedPsf", &clonedPsf);
    mod.def("clonedStorablePsf", &clonedStorablePsf);
    mod.def("isPersistable", &isPersistable);
}

}  // namespace detection
}  // namespace afw
}  // namespace lsst
