// -*- LSST-C++ -*-
/*
 * LSST Data Management System
 * Copyright 2008-2014 LSST Corporation.
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

#ifndef LSST_AFW_IMAGE_ApCorrMap_h_INCLUDED
#define LSST_AFW_IMAGE_ApCorrMap_h_INCLUDED

#include <string>
#include <map>

#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/math/BoundedField.h"

namespace lsst {
namespace afw {
namespace image {

/**
 *  A thin wrapper around std::map to allow aperture corrections to be attached to Exposures.
 *
 *  ApCorrMap simply adds error handling accessors, persistence, and a bit of encapsulation to std::map
 *  (given the simplified interface, for instance, we could switch to unordered_map or some other
 *  underyling container in the future).
 */
class ApCorrMap final : public typehandling::Storable {
DECLARE_PERSISTABLE_FACADE(ApCorrMap);
    using Internal = std::map<std::string, std::shared_ptr<math::BoundedField>>;

public:
    /// Maximum number of characters for an aperture correction name (required for persistence).
    static std::size_t const MAX_NAME_LENGTH = 64;

    /// Iterator type returned by begin() and end().  Dereferences to a
    /// pair<string,std::shared_ptr<BoundedField>>.
    using Iterator = Internal::const_iterator;

    ApCorrMap() = default;
    ApCorrMap(ApCorrMap const&) = default;
    ApCorrMap(ApCorrMap&&) = default;
    ApCorrMap& operator=(ApCorrMap const&) = default;
    ApCorrMap& operator=(ApCorrMap&&) = default;
    ~ApCorrMap() override = default;

    Iterator begin() const { return _internal.begin(); }
    Iterator end() const { return _internal.end(); }

    std::size_t size() const { return _internal.size(); }

    /// Return the field with the given name, throwing NotFoundError when the name is not present.
    std::shared_ptr<math::BoundedField> const operator[](std::string const& name) const;

    /// Return the field with the given name, returning an empty pointer when the name is not present.
    std::shared_ptr<math::BoundedField> const get(std::string const& name) const;

    /// Add or replace an aperture correction.
    void set(std::string const& name, std::shared_ptr<math::BoundedField> field);

    /// Whether the map is persistable (true IFF all contained BoundedFields are persistable).
    bool isPersistable() const noexcept override;

    /// Scale all fields by a constant
    ApCorrMap& operator*=(double const scale);
    ApCorrMap& operator/=(double const scale) { return *this *= 1.0 / scale; }

    /// Create a new ApCorrMap that is a copy of this one.
    std::shared_ptr<typehandling::Storable> cloneStorable() const override;

private:
    std::string getPersistenceName() const override;

    std::string getPythonModule() const override;

    void write(OutputArchiveHandle& handle) const override;

    Internal _internal;
};
}  // namespace image
}  // namespace afw
}  // namespace lsst

#endif  // !LSST_AFW_IMAGE_ApCorrMap_h_INCLUDED
