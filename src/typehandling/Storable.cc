// -*- LSST-C++ -*-
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

#include <memory>
#include <string>

#include "lsst/afw/typehandling/Storable.h"

namespace lsst {
namespace afw {
namespace typehandling {

Storable::~Storable() noexcept = default;

std::shared_ptr<Storable> Storable::cloneStorable() const {
    throw LSST_EXCEPT(UnsupportedOperationException, "Cloning is not supported.");
}

std::string Storable::toString() const {
    throw LSST_EXCEPT(UnsupportedOperationException, "No string representation available.");
}

std::size_t Storable::hash_value() const {
    throw LSST_EXCEPT(UnsupportedOperationException, "Hashes are not supported.");
}

bool Storable::equals(Storable const& other) const noexcept { return this == &other; }

}  // namespace typehandling
}  // namespace afw
}  // namespace lsst
