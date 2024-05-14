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
#include <regex>

#include "lsst/cpputils/hashCombine.h"
#include "lsst/pex/exceptions.h"
#include "lsst/afw/image/FilterLabel.h"
#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/Flag.h"
#include "lsst/afw/table/FunctorKey.h"
#include "lsst/afw/table/Schema.h"
#include "lsst/afw/table/io/InputArchive.h"
#include "lsst/afw/table/io/OutputArchive.h"
#include "lsst/afw/table/io/CatalogVector.h"
#include "lsst/afw/table/io/Persistable.cc"  // Needed for PersistableFacade::dynamicCast

using namespace std::string_literals;

namespace lsst {
namespace afw {
namespace image {

std::string getDatabaseFilterLabel(std::string const &filterLabel) {
    static std::regex const unsafeCharacters("\\W"s);
    return std::regex_replace(filterLabel, unsafeCharacters, "_"s);
}

namespace impl {
// Hack to allow unit tests to test states that, while legal, are
// not produced by (and should not be required of) standard factories.
FilterLabel makeTestFilterLabel(bool hasBand, std::string const &band, bool hasPhysical,
                                std::string const &physical) {
    // private constructor accessible via friend
    return FilterLabel(hasBand, band, hasPhysical, physical);
}
}  // namespace impl

FilterLabel::FilterLabel(bool hasBand, std::string const &band, bool hasPhysical, std::string const &physical)
        : _hasBand(hasBand), _hasPhysical(hasPhysical), _band(band), _physical(physical) {
    // Guard against changes to factory methods or nanobind keyword constructor
    if (!hasBand && !hasPhysical) {
        throw LSST_EXCEPT(pex::exceptions::LogicError, "FilterLabel must have at least one label.");
    }
}

FilterLabel FilterLabel::fromBandPhysical(std::string const &band, std::string const &physical) {
    return FilterLabel(true, band, true, physical);
}

FilterLabel FilterLabel::fromBand(std::string const &band) { return FilterLabel(true, band, false, ""s); }

FilterLabel FilterLabel::fromPhysical(std::string const &physical) {
    return FilterLabel(false, ""s, true, physical);
}

void FilterLabel::fromBandPhysical(FilterLabel *filterLabel, std::string const &band, std::string const &physical) {
    new (filterLabel) FilterLabel(true, band, true, physical);
}

void FilterLabel::fromBand(FilterLabel *filterLabel, std::string const &band) {
    new (filterLabel) FilterLabel(true, band, false, ""s);
}

void FilterLabel::fromPhysical(FilterLabel *filterLabel, std::string const &physical) {
    new (filterLabel) FilterLabel(false, ""s, true, physical);
}

// defaults give the right behavior with bool-and-string implementation
FilterLabel::FilterLabel(FilterLabel const &) = default;
FilterLabel::FilterLabel(FilterLabel &&) noexcept = default;
FilterLabel &FilterLabel::operator=(FilterLabel const &) = default;
FilterLabel &FilterLabel::operator=(FilterLabel &&) noexcept = default;
FilterLabel::~FilterLabel() noexcept = default;

bool FilterLabel::hasBandLabel() const noexcept { return _hasBand; }

std::string FilterLabel::getBandLabel() const {
    // In no implementation I can think of will hasBandLabel() be an expensive test.
    if (hasBandLabel()) {
        return _band;
    } else {
        throw LSST_EXCEPT(pex::exceptions::LogicError, toString() + " has no band."s);
    }
}

bool FilterLabel::hasPhysicalLabel() const noexcept { return _hasPhysical; }

std::string FilterLabel::getPhysicalLabel() const {
    // In no implementation I can think of will hasBandLabel() be an expensive test.
    if (hasPhysicalLabel()) {
        return _physical;
    } else {
        throw LSST_EXCEPT(pex::exceptions::LogicError, toString() + " has no physical filter."s);
    }
}

bool FilterLabel::operator==(FilterLabel const &rhs) const noexcept {
    // Do not compare name unless _hasName for both
    if (_hasBand != rhs._hasBand) {
        return false;
    }
    if (_hasBand && _band != rhs._band) {
        return false;
    }
    if (_hasPhysical != rhs._hasPhysical) {
        return false;
    }
    if (_hasPhysical && _physical != rhs._physical) {
        return false;
    }
    return true;
}

// Storable support

std::size_t FilterLabel::hash_value() const noexcept {
    // Do not count _name unless _hasName
    // (_has=false, _name="A") and (_has=false, _name="B") compare equal, so must have same hash
    return cpputils::hashCombine(42, _hasBand, _hasBand ? _band : ""s, _hasPhysical,
                              _hasPhysical ? _physical : ""s);
}

/* The implementation is biased toward Python in its format, but I expect
 * the C++ calls to mostly be used for debugging rather than presentation.
 * This class is also too simple to need "long" and "short" string forms.
 */
std::string FilterLabel::toString() const {
    std::string buffer("FilterLabel(");
    bool comma = false;

    if (hasBandLabel()) {
        if (comma) buffer += ", "s;
        buffer += "band"s + "=\""s + getBandLabel() + "\""s;
        comma = true;
    }
    if (hasPhysicalLabel()) {
        if (comma) buffer += ", "s;
        buffer += "physical"s + "=\""s + getPhysicalLabel() + "\""s;
        comma = true;
    }
    buffer += ")"s;

    return buffer;
}

std::shared_ptr<typehandling::Storable> FilterLabel::cloneStorable() const {
    return std::make_shared<FilterLabel>(*this);
}

// Persistable support

namespace {

/* Abstract the representation of an optional string so that it's easy to
 * add/remove filter names later. The choice of pair as the key type was
 * dictated by the implementation of FilterLabel, and may be changed if
 * FilterLabel's internal representation changes. The persisted form
 * cannot be changed without breaking old files.
 */
class OptionalString : public table::FunctorKey<std::pair<bool, std::string>> {
public:
    static OptionalString addFields(table::Schema &schema, std::string const &name, std::string const &doc,
                                    int length) {
        table::Key<table::Flag> existsKey =
                schema.addField<table::Flag>(schema.join(name, "exists"), "Existence flag for "s + name);
        table::Key<std::string> valueKey = schema.addField<std::string>(name, doc, "", length);
        return OptionalString(existsKey, valueKey);
    }

    OptionalString() noexcept : _exists(), _value() {}
    OptionalString(table::Key<table::Flag> const &exists, table::Key<std::string> const &value) noexcept
            : _exists(exists), _value(value) {}

    std::pair<bool, std::string> get(table::BaseRecord const &record) const override {
        // Suppress any weird values if they don't matter
        bool exists = record.get(_exists);
        return std::make_pair(exists, exists ? record.get(_value) : ""s);
    }

    void set(table::BaseRecord &record, std::pair<bool, std::string> const &value) const override {
        // Suppress any weird values if they don't matter
        record.set(_exists, value.first);
        record.set(_value, value.first ? value.second : ""s);
    }

    bool operator==(OptionalString const &other) const noexcept {
        return _exists == other._exists && _value == other._value;
    }
    bool operator!=(OptionalString const &other) const noexcept { return !(*this == other); }

    bool isValid() const noexcept { return _exists.isValid() && _value.isValid(); }

    table::Key<table::Flag> getExists() const noexcept { return _exists; }
    table::Key<std::string> getValue() const noexcept { return _value; }

private:
    table::Key<table::Flag> _exists;
    table::Key<std::string> _value;
};

struct PersistenceHelper {
    table::Schema schema;
    OptionalString band;
    OptionalString physical;

    static PersistenceHelper const &get() {
        static PersistenceHelper const instance;
        return instance;
    }

private:
    PersistenceHelper()
            : schema(),
              band(OptionalString::addFields(schema, "band", "Name of the band.", 32)),
              physical(OptionalString::addFields(schema, "physical", "Name of the physical filter.", 32)) {}
};

std::string _getPersistenceName() noexcept { return "FilterLabel"s; }

}  // namespace

std::string FilterLabel::getPersistenceName() const noexcept { return _getPersistenceName(); }
std::string FilterLabel::getPythonModule() const noexcept { return "lsst.afw.image"s; }

void FilterLabel::write(table::io::OutputArchiveHandle &handle) const {
    PersistenceHelper const &keys = PersistenceHelper::get();
    table::BaseCatalog catalog = handle.makeCatalog(keys.schema);
    std::shared_ptr<table::BaseRecord> record = catalog.addNew();

    record->set(keys.band, std::make_pair(_hasBand, _band));
    record->set(keys.physical, std::make_pair(_hasPhysical, _physical));
    handle.saveCatalog(catalog);
}

class FilterLabel::Factory : public table::io::PersistableFactory {
public:
    std::shared_ptr<table::io::Persistable> read(table::io::InputArchive const &archive,
                                                 table::io::CatalogVector const &catalogs) const override {
        auto const &keys = PersistenceHelper::get();
        LSST_ARCHIVE_ASSERT(catalogs.size() == 1u);
        table::BaseRecord const &record = catalogs.front().front();
        LSST_ARCHIVE_ASSERT(record.getSchema() == keys.schema);

        // Use explicit new operator to access private constructor
        return std::shared_ptr<FilterLabel>(
                new FilterLabel(record.get(keys.band.getExists()), record.get(keys.band.getValue()),
                                record.get(keys.physical.getExists()), record.get(keys.physical.getValue())));
    }

    Factory(std::string const &name) : table::io::PersistableFactory(name) {}
};

// Adds FilterLabel::factory to a global registry.
FilterLabel::Factory FilterLabel::factory(_getPersistenceName());

}  // namespace image

template std::shared_ptr<image::FilterLabel> table::io::PersistableFacade<image::FilterLabel>::dynamicCast(
        std::shared_ptr<table::io::Persistable> const &);

}  // namespace afw
}  // namespace lsst
