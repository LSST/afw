// -*- lsst-c++ -*-
/*
 * LSST Data Management System
 * Copyright 2008, 2009, 2010, 2011 LSST Corporation.
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
#ifndef AFW_TABLE_Simple_h_INCLUDED
#define AFW_TABLE_Simple_h_INCLUDED

#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/BaseTable.h"
#include "lsst/afw/table/IdFactory.h"
#include "lsst/afw/table/Catalog.h"
#include "lsst/afw/table/BaseColumnView.h"
#include "lsst/afw/table/SortedCatalog.h"
#include "lsst/afw/table/aggregates.h"

namespace lsst {
namespace afw {
namespace table {

class SimpleRecord;
class SimpleTable;

/**
 *  Record class that must contain a unique ID field and a celestial coordinate field.
 *
 *  SimpleTable / SimpleRecord are intended to be the base class for records representing astronomical
 *  objects.  In additional to the minimal schema and the convenience accessors it allows, a SimpleTable
 *  may hold an IdFactory object that is used to assign unique IDs to new records.
 */
class SimpleRecord : public BaseRecord {
public:
    using Table = SimpleTable;
    using ColumnView = ColumnViewT<SimpleRecord>;
    using Catalog = SortedCatalogT<SimpleRecord>;
    using ConstCatalog = SortedCatalogT<const SimpleRecord>;

    /**
     *  Constructor used by SimpleTable.
     *
     *  While formally public, this constructor is conceptually and effectively
     *  protected, due to the (protected) ConstructionToken argument.
     *
     *  This is to allow make_shared to be used, as that cannot be used on a
     *  truly protected or private constructor.
     */
    SimpleRecord(ConstructionToken const & token, detail::RecordData && data) :
        BaseRecord(token, std::move(data))
    {}

    std::shared_ptr<SimpleTable const> getTable() const {
        return std::static_pointer_cast<SimpleTable const>(BaseRecord::getTable());
    }

    //@{
    /// Convenience accessors for the keys in the minimal reference schema.
    RecordId getId() const;
    void setId(RecordId id);

    lsst::geom::SpherePoint getCoord() const;
    void setCoord(lsst::geom::SpherePoint const& coord);

    lsst::geom::Angle getRa() const;
    void setRa(lsst::geom::Angle ra);

    lsst::geom::Angle getDec() const;
    void setDec(lsst::geom::Angle dec);
    //@}

    SimpleRecord(const SimpleRecord&) = delete;
    SimpleRecord& operator=(const SimpleRecord&) = delete;
    SimpleRecord(SimpleRecord&&) = delete;
    SimpleRecord& operator=(SimpleRecord&&) = delete;
    ~SimpleRecord() override;

private:
    friend class SimpleTable;
};

/**
 *  Table class that must contain a unique ID field and a celestial coordinate field.
 *
 *  @copydetails SimpleRecord
 */
class SimpleTable : public BaseTable {
public:
    using Record = SimpleRecord;
    using ColumnView = ColumnViewT<SimpleRecord>;
    using Catalog = SortedCatalogT<Record>;
    using ConstCatalog = SortedCatalogT<const Record>;

    /**
     *  Construct a new table.
     *
     *  @param[in] schema            Schema that defines the fields, offsets, and record size for the table.
     *  @param[in] idFactory         Factory class to generate record IDs when they are not explicitly given.
     *                               If null, record IDs will default to zero.
     *
     *  Note that not passing an IdFactory at all will call the other override of make(), which will
     *  set the ID factory to IdFactory::makeSimple().
     */
    static std::shared_ptr<SimpleTable> make(Schema const& schema,
                                             std::shared_ptr<IdFactory> const& idFactory);

    /**
     *  Construct a new table.
     *
     *  @param[in] schema            Schema that defines the fields, offsets, and record size for the table.
     *
     *  This overload sets the ID factory to IdFactory::makeSimple().
     */
    static std::shared_ptr<SimpleTable> make(Schema const& schema) {
        return make(schema, IdFactory::makeSimple());
    }

    /**
     *  Return a minimal schema for Simple tables and records.
     *
     *  The returned schema can and generally should be modified further,
     *  but many operations on SimpleRecords will assume that at least the fields
     *  provided by this routine are present.
     */
    static Schema makeMinimalSchema() {
        Schema r = getMinimalSchema().schema;
        r.disconnectAliases();
        return r;
    }

    /**
     *  Return true if the given schema is a valid SimpleTable schema.
     *
     *  This will always be true if the given schema was originally constructed
     *  using makeMinimalSchema(), and will rarely be true otherwise.
     */
    static bool checkSchema(Schema const& other) { return other.contains(getMinimalSchema().schema); }

    /// Return the object that generates IDs for the table (may be null).
    std::shared_ptr<IdFactory> getIdFactory() { return _idFactory; }

    /// Return the object that generates IDs for the table (may be null).
    std::shared_ptr<IdFactory const> getIdFactory() const { return _idFactory; }

    /// Switch to a new IdFactory -- object that generates IDs for the table (may be null).
    void setIdFactory(std::shared_ptr<IdFactory> f) { _idFactory = f; }

    //@{
    /**
     *  Get keys for standard fields shared by all references.
     *
     *  These keys are used to implement getters and setters on SimpleRecord.
     */
    /// Key for the unique ID.
    static Key<RecordId> getIdKey() { return getMinimalSchema().id; }
    /// Key for the celestial coordinates.
    static CoordKey getCoordKey() { return getMinimalSchema().coord; }
    //@}

    /// @copydoc BaseTable::clone
    std::shared_ptr<SimpleTable> clone() const { return std::static_pointer_cast<SimpleTable>(_clone()); }

    /// @copydoc BaseTable::makeRecord
    std::shared_ptr<SimpleRecord> makeRecord() {
        return std::static_pointer_cast<SimpleRecord>(_makeRecord());
    }

    /// @copydoc BaseTable::copyRecord
    std::shared_ptr<SimpleRecord> copyRecord(BaseRecord const& other) {
        return std::static_pointer_cast<SimpleRecord>(BaseTable::copyRecord(other));
    }

    /// @copydoc BaseTable::copyRecord
    std::shared_ptr<SimpleRecord> copyRecord(BaseRecord const& other, SchemaMapper const& mapper) {
        return std::static_pointer_cast<SimpleRecord>(BaseTable::copyRecord(other, mapper));
    }

    SimpleTable& operator=(SimpleTable const&) = delete;
    SimpleTable& operator=(SimpleTable&&) = delete;
    ~SimpleTable() override;

protected:
    SimpleTable(Schema const& schema, std::shared_ptr<IdFactory> const& idFactory);

    explicit SimpleTable(SimpleTable const& other);
    explicit SimpleTable(SimpleTable&& other);

    std::shared_ptr<BaseTable> _clone() const override;

    std::shared_ptr<BaseRecord> _makeRecord() override;

private:
    // Struct that holds the minimal schema and the special keys we've added to it.
    struct MinimalSchema {
        Schema schema;
        Key<RecordId> id;
        CoordKey coord;

        MinimalSchema();
    };

    // Return the singleton minimal schema.
    static MinimalSchema& getMinimalSchema();

    friend class io::FitsWriter;

    // Return a writer object that knows how to save in FITS format.  See also FitsWriter.
    std::shared_ptr<io::FitsWriter> makeFitsWriter(fits::Fits* fitsfile, int flags) const override;

    std::shared_ptr<IdFactory> _idFactory;  // generates IDs for new records
};

inline RecordId SimpleRecord::getId() const { return get(SimpleTable::getIdKey()); }
inline void SimpleRecord::setId(RecordId id) { set(SimpleTable::getIdKey(), id); }

inline lsst::geom::SpherePoint SimpleRecord::getCoord() const { return get(SimpleTable::getCoordKey()); }
inline void SimpleRecord::setCoord(lsst::geom::SpherePoint const& coord) {
    set(SimpleTable::getCoordKey(), coord);
}

inline lsst::geom::Angle SimpleRecord::getRa() const { return get(SimpleTable::getCoordKey().getRa()); }
inline void SimpleRecord::setRa(lsst::geom::Angle ra) { set(SimpleTable::getCoordKey().getRa(), ra); }

inline lsst::geom::Angle SimpleRecord::getDec() const { return get(SimpleTable::getCoordKey().getDec()); }
inline void SimpleRecord::setDec(lsst::geom::Angle dec) { set(SimpleTable::getCoordKey().getDec(), dec); }
}  // namespace table
}  // namespace afw
}  // namespace lsst

#endif  // !AFW_TABLE_Simple_h_INCLUDED
