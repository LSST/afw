// -*- lsst-c++ -*-
/*
 * LSST Data Management System
 * See the COPYRIGHT and LICENSE files in the top-level directory of this
 * package for notices and licensing terms.
 */
#ifndef AFW_DETECTION_Peak_h_INCLUDED
#define AFW_DETECTION_Peak_h_INCLUDED

#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/BaseTable.h"
#include "lsst/afw/table/IdFactory.h"
#include "lsst/afw/table/Catalog.h"
#include "lsst/afw/table/BaseColumnView.h"

namespace lsst { namespace afw { namespace detection {

class PeakRecord;
class PeakTable;

/**
 *  @brief Record class that represents a peak in a Footprint
 */
class PeakRecord : public afw::table::BaseRecord {
public:

    typedef PeakTable Table;
    typedef afw::table::ColumnViewT<PeakRecord> ColumnView;
    typedef afw::table::CatalogT<PeakRecord> Catalog;
    typedef afw::table::CatalogT<PeakRecord const> ConstCatalog;

    CONST_PTR(PeakTable) getTable() const {
        return boost::static_pointer_cast<PeakTable const>(afw::table::BaseRecord::getTable());
    }

    //@{
    /// @brief Convenience accessors for the keys in the minimal schema.
    afw::table::RecordId getId() const;
    void setId(afw::table::RecordId id);

    int getIx() const;
    int getIy() const;
    void setIx(int ix);
    void setIy(int iy);
    afw::geom::Point2I getI() const { return afw::geom::Point2I(getIx(), getIy()); }
    afw::geom::Point2I getCentroid(bool) const { return getI(); }

    float getFx() const;
    float getFy() const;
    void setFx(float fx);
    void setFy(float fy);
    afw::geom::Point2D getF() const { return afw::geom::Point2D(getFx(), getFy()); }
    afw::geom::Point2D getCentroid() const { return getF(); }

    float getPeakValue() const;
    void setPeakValue(float peakValue);
    //@}

protected:

    PeakRecord(PTR(PeakTable) const & table);

};

/**
 *  @brief Table class for Peaks in Footprints.
 */
class PeakTable : public afw::table::BaseTable {
public:

    typedef PeakRecord Record;
    typedef afw::table::ColumnViewT<PeakRecord> ColumnView;
    typedef afw::table::CatalogT<Record> Catalog;
    typedef afw::table::CatalogT<Record const> ConstCatalog;

    /**
     *  @brief Obtain a table that can be used to create records with given schema
     *
     *  @param[in] schema     Schema that defines the fields, offsets, and record size for the table.
     *  @param[in] forceNew   If true, guarantee that the returned PeakTable will be a new one, rather
     *                        than attempting to reuse an existing PeakTable with the same Schema.
     *
     *  If a PeakTable already exists that uses this Schema, that PeakTable will be returned instead
     *  of creating a new one.  This is different from how most Record/Table classes work, but it is
     *  an important memory optimization for Peaks, for which we expect to have very few distinct
     *  Schemas as well as many catalogs (one per Footprint) with a small number of Peaks; we don't want
     *  to have a different PeakTable for each one of those catalogs if they all share the same Schema.
     *  This behavior can be disabled by setting forceNewTable=true or by cloning an existing table
     *  (in both of these cases, the new table will not be reused in the future, either)
     */
    static PTR(PeakTable) make(afw::table::Schema const & schema, bool forceNew=false);

    /**
     *  @brief Return a minimal schema for Peak tables and records.
     *
     *  The returned schema can and generally should be modified further,
     *  but many operations on PeakRecords will assume that at least the fields
     *  provided by this routine are present.
     */
    static afw::table::Schema makeMinimalSchema() { return getMinimalSchema().schema; }

    /**
     *  @brief Return true if the given schema is a valid PeakTable schema.
     *
     *  This will always be true if the given schema was originally constructed
     *  using makeMinimalSchema(), and will rarely be true otherwise.
     */
    static bool checkSchema(afw::table::Schema const & other) {
        return other.contains(getMinimalSchema().schema);
    }

    /// @brief Return the object that generates IDs for the table (may be null).
    PTR(afw::table::IdFactory) getIdFactory() { return _idFactory; }

    /// @brief Return the object that generates IDs for the table (may be null).
    CONST_PTR(afw::table::IdFactory) getIdFactory() const { return _idFactory; }

    /// @brief Switch to a new IdFactory -- object that generates IDs for the table (may be null).
    void setIdFactory(PTR(afw::table::IdFactory) f) { _idFactory = f; }

    //@{
    /**
     *  Get keys for standard fields shared by all peaks.
     *
     *  These keys are used to implement getters and setters on PeakRecord.
     */
    static afw::table::Key<afw::table::RecordId> getIdKey() { return getMinimalSchema().id; }
    static afw::table::Key<int> getIxKey() { return getMinimalSchema().ix; }
    static afw::table::Key<int> getIyKey() { return getMinimalSchema().iy; }
    static afw::table::Key<float> getFxKey() { return getMinimalSchema().fx; }
    static afw::table::Key<float> getFyKey() { return getMinimalSchema().fy; }
    static afw::table::Key<float> getPeakValueKey() { return getMinimalSchema().peakValue; }
    //@}

    /// @copydoc BaseTable::clone
    PTR(PeakTable) clone() const { return boost::static_pointer_cast<PeakTable>(_clone()); }

    /// @copydoc BaseTable::makeRecord
    PTR(PeakRecord) makeRecord() { return boost::static_pointer_cast<PeakRecord>(_makeRecord()); }

    /// @copydoc BaseTable::copyRecord
    PTR(PeakRecord) copyRecord(afw::table::BaseRecord const & other) {
        return boost::static_pointer_cast<PeakRecord>(afw::table::BaseTable::copyRecord(other));
    }

    /// @copydoc BaseTable::copyRecord
    PTR(PeakRecord) copyRecord(
        afw::table::BaseRecord const & other,
        afw::table::SchemaMapper const & mapper
    ) {
        return boost::static_pointer_cast<PeakRecord>(afw::table::BaseTable::copyRecord(other, mapper));
    }

protected:

    PeakTable(afw::table::Schema const & schema, PTR(afw::table::IdFactory) const & idFactory);

    PeakTable(PeakTable const & other);

private:

    // Struct that holds the minimal schema and the special keys we've added to it.
    struct MinimalSchema {
        afw::table::Schema schema;
        afw::table::Key<afw::table::RecordId> id;
        afw::table::Key<float> fx;
        afw::table::Key<float> fy;
        afw::table::Key<int> ix;
        afw::table::Key<int> iy;
        afw::table::Key<float> peakValue;

        MinimalSchema();
    };

    // Return the singleton minimal schema.
    static MinimalSchema & getMinimalSchema();

    friend class afw::table::io::FitsWriter;

     // Return a writer object that knows how to save in FITS format.  See also FitsWriter.
    virtual PTR(afw::table::io::FitsWriter) makeFitsWriter(fits::Fits * fitsfile, int flags) const;

    PTR(afw::table::IdFactory) _idFactory;        // generates IDs for new records
};

#ifndef SWIG

std::ostream & operator<<(std::ostream & os, PeakRecord const & record);

inline afw::table::RecordId PeakRecord::getId() const { return get(PeakTable::getIdKey()); }
inline void PeakRecord::setId(afw::table::RecordId id) { set(PeakTable::getIdKey(), id); }

inline int PeakRecord::getIx() const { return get(PeakTable::getIxKey()); }
inline int PeakRecord::getIy() const { return get(PeakTable::getIyKey()); }
inline void PeakRecord::setIx(int ix) { set(PeakTable::getIxKey(), ix); }
inline void PeakRecord::setIy(int iy) { set(PeakTable::getIyKey(), iy); }

inline float PeakRecord::getFx() const { return get(PeakTable::getFxKey()); }
inline float PeakRecord::getFy() const { return get(PeakTable::getFyKey()); }
inline void PeakRecord::setFx(float fx) { set(PeakTable::getFxKey(), fx); }
inline void PeakRecord::setFy(float fy) { set(PeakTable::getFyKey(), fy); }

inline float PeakRecord::getPeakValue() const { return get(PeakTable::getPeakValueKey()); }
inline void PeakRecord::setPeakValue(float peakValue) { set(PeakTable::getPeakValueKey(), peakValue); }

#endif // !SWIG

typedef afw::table::ColumnViewT<PeakRecord> PeakColumnView;
typedef afw::table::CatalogT<PeakRecord> PeakCatalog;
typedef afw::table::CatalogT<PeakRecord const> ConstPeakCatalog;

}}} // namespace lsst::afw::detection

#endif // !AFW_DETECTION_Peak_h_INCLUDED
