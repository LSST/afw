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

#include "nanobind/nanobind.h"
#include <lsst/cpputils/python.h>
#include "nanobind/stl/vector.h"

#include "lsst/afw/math/ProductBoundedField.h"

namespace nb = nanobind;

using namespace lsst::afw::math;
namespace lsst {
namespace afw {
namespace math {
using PyClass = nb::class_<ProductBoundedField, BoundedField>;

void wrapProductBoundedField(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrapType(PyClass(wrappers.module, "ProductBoundedField"), [](auto &mod, auto &cls) {
        cls.def(nb::init<std::vector<std::shared_ptr<BoundedField const>>>());
        // All other operations are wrapped by the BoundedField base class.
    });
}
}  // namespace math
}  // namespace afw
}  // namespace lsst
