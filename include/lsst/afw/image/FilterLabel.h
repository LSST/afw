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

#ifndef LSST_AFW_IMAGE_FILTERLABEL_H
#define LSST_AFW_IMAGE_FILTERLABEL_H

#include <memory>
#include <string>

#include "lsst/afw/table/io/Persistable.h"  // Needed for PersistableFacade
#include "lsst/afw/typehandling/Storable.h"

namespace lsst {
namespace afw {
namespace image {

#ifndef DOXYGEN
class FilterLabel;
namespace impl {
// Needed for some esoteric tests; do not use elsewhere!
FilterLabel makeTestFilterLabel(bool, std::string const &, bool, std::string const &);
}  // namespace impl
#endif

/**
 * A group of labels for a filter in an exposure or coadd.
 *
 * This class provides only identifiers for filters. Other filter information
 * can be retrieved from an Exposure object or a Butler repository.
 *
 * FilterLabel does not expose a public constructor in C++, except for copy
 * and move constructors. You can create a FilterLabel by calling one of the
 * `from*` factory methods, or (in Python) through a keyword-only constructor.
 *
 * Not all of a FilterLabel object's labels need be defined, but at least one
 * is guaranteed to be present.
 */
class FilterLabel final : public typehandling::Storable {
DECLARE_PERSISTABLE_FACADE(FilterLabel);
public:
    /**
     * Construct a FilterLabel from specific inputs.
     *
     * @{
     */
    static FilterLabel fromBandPhysical(std::string const &band, std::string const &physical);
    static FilterLabel fromBand(std::string const &band);
    static FilterLabel fromPhysical(std::string const &physical);
    /** @} */

    FilterLabel(FilterLabel const &);
    FilterLabel(FilterLabel &&) noexcept;
    FilterLabel &operator=(FilterLabel const &);  // Only way to modify a FilterLabel
    FilterLabel &operator=(FilterLabel &&) noexcept;
    ~FilterLabel() noexcept;

    /// Return whether the filter label names a band.
    bool hasBandLabel() const noexcept;

    /**
     * Return the band label.
     *
     * In Python, this is replaced by the `band` read-only property.
     *
     * @returns The band label.
     * @throws lsst::pex::exceptions::LogicError Thrown if hasBandLabel() is `false`.
     */
    std::string getBandLabel() const;

    /// Return whether the filter label names a physical filter.
    bool hasPhysicalLabel() const noexcept;

    /**
     * Return the physical filter label.
     *
     * In Python, this is replaced by the `physicalLabel` read-only property.
     *
     * @returns The physical filter label.
     * @throws lsst::pex::exceptions::LogicError Thrown if hasPhysicalLabel() is `false`.
     */
    std::string getPhysicalLabel() const;

    /**
     * Filter labels compare equal if their components are equal.
     *
     * @note This operation does not test whether two *filters* are the same.
     *       Two FilterLabels corresponding to identically-named filters on
     *       different instruments will compare equal.
     *
     * @{
     */
    bool operator==(FilterLabel const &rhs) const noexcept;
    bool operator!=(FilterLabel const &rhs) const noexcept { return !(*this == rhs); }
    /** @} */

    // Storable support

    /// Return a hash of this object.
    std::size_t hash_value() const noexcept override;
    /// Return a string representation of this object.
    std::string toString() const override;
    /// Create a new object that is a copy of this one.
    std::shared_ptr<Storable> cloneStorable() const override;
    bool equals(Storable const &other) const noexcept override { return singleClassEquals(*this, other); }

    // Persistable support

    /// All filter labels are always persistable.
    bool isPersistable() const noexcept override { return true; }

protected:
    // Persistable support

    std::string getPersistenceName() const noexcept override;
    std::string getPythonModule() const noexcept override;
    void write(table::io::OutputArchiveHandle &handle) const override;

private:
    FilterLabel(bool hasBand, std::string const &band, bool hasPhysical, std::string const &physical);
#ifndef DOXYGEN
    // Needed for some esoteric tests; do not use elsewhere!
    friend FilterLabel impl::makeTestFilterLabel(bool, std::string const &, bool, std::string const &);
#endif

    // A separate boolean leads to easier implementations (at the cost of more
    // memory) than a unique_ptr<string>.
    // _band and _physical are part of the object state iff _hasBand and _hasPhysical, respectively
    // Class invariant: at least one of _hasBand, _hasPhysical is true
    bool _hasBand, _hasPhysical;
    std::string _band, _physical;

    // Persistable support

    class Factory;
    static Factory factory;
};

/**
 * Remap special characters, etc. to "_" for database fields.
 *
 * @return The filter label in database-sanitized format.
 */
std::string getDatabaseFilterLabel(std::string const &filterLabel);

}  // namespace image
}  // namespace afw
}  // namespace lsst

namespace std {
template <>
struct hash<lsst::afw::image::FilterLabel> {
    using argument_type = lsst::afw::image::FilterLabel;
    using result_type = size_t;
    size_t operator()(argument_type const &obj) const noexcept { return obj.hash_value(); }
};
}  // namespace std

#endif
