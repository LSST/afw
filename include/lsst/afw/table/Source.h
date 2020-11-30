// -*- lsst-c++ -*-
/*
 * LSST Data Management System
 * Copyright 2008-2014, 2011 LSST Corporation.
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

#ifndef AFW_TABLE_Source_h_INCLUDED
#define AFW_TABLE_Source_h_INCLUDED

#include "boost/array.hpp"
#include "boost/type_traits/is_convertible.hpp"

#include "lsst/afw/detection/Footprint.h"
#include "lsst/afw/table/Simple.h"
#include "lsst/afw/table/aggregates.h"
#include "lsst/afw/table/IdFactory.h"
#include "lsst/afw/table/Catalog.h"
#include "lsst/afw/table/BaseColumnView.h"
#include "lsst/afw/table/slots.h"
#include "lsst/afw/table/io/FitsWriter.h"

namespace lsst {
namespace afw {

namespace geom {
class SkyWcs;
}  // namespace geom

namespace table {

/**
 *  Bitflags to be passed to SourceCatalog::readFits and SourceCatalog::writeFits
 *
 *  Note that these flags may also be passed when reading/writing SourceCatalogs via the Butler,
 *  by passing a "flags" key/value pair as part of the data ID.
 */
enum SourceFitsFlags {
    SOURCE_IO_NO_FOOTPRINTS = 0x1,       ///< Do not read/write footprints at all
    SOURCE_IO_NO_HEAVY_FOOTPRINTS = 0x2  ///< Read/write heavy footprints as non-heavy footprints
};

typedef lsst::afw::detection::Footprint Footprint;

class SourceRecord;
class SourceTable;

template <typename RecordT>
class SourceColumnViewT;

/**
 *  Record class that contains measurements made on a single exposure.
 *
 *  Sources provide four additions to SimpleRecord / SimpleRecord:
 *   - Specific fields that must always be present, with specialized getters.
 *     The schema for a SourceTable should always be constructed by starting with the result of
 *     SourceTable::makeMinimalSchema.
 *   - A shared_ptr to a Footprint for each record.
 *   - A system of aliases (called slots) in which a SourceTable instance stores keys for particular
 *     measurements (a centroid, a shape, and a number of different fluxes) and SourceRecord uses
 *     this keys to provide custom getters and setters.  These are not separate fields, but rather
 *     aliases that can point to custom fields.  See the SlotDefinition hierarchy for more information.
 */
class SourceRecord : public SimpleRecord {
public:
    typedef SourceTable Table;
    typedef SourceColumnViewT<SourceRecord> ColumnView;
    typedef SortedCatalogT<SourceRecord> Catalog;
    typedef SortedCatalogT<SourceRecord const> ConstCatalog;

    /**
     *  Constructor used by SourceTable.
     *
     *  While formally public, this constructor is conceptually and effectively
     *  protected, due to the (protected) ConstructionToken argument.
     *
     *  This is to allow make_shared to be used, as that cannot be used on a
     *  truly protected or private constructor.
     */
    SourceRecord(ConstructionToken const & token, detail::RecordData && data) :
        SimpleRecord(token, std::move(data))
    {}

    std::shared_ptr<Footprint> getFootprint() const { return _footprint; }

    void setFootprint(std::shared_ptr<Footprint> const &footprint) { _footprint = footprint; }

    std::shared_ptr<SourceTable const> getTable() const {
        return std::static_pointer_cast<SourceTable const>(BaseRecord::getTable());
    }

    //@{
    /// Convenience accessors for the keys in the minimal source schema.
    RecordId getParent() const;
    void setParent(RecordId id);
    //@}

    /// Get the value of the PsfFlux slot measurement.
    FluxSlotDefinition::MeasValue getPsfInstFlux() const;

    /// Get the uncertainty on the PsfFlux slot measurement.
    FluxSlotDefinition::ErrValue getPsfInstFluxErr() const;

    /// Return true if the measurement in the PsfFlux slot failed.
    bool getPsfFluxFlag() const;

    /// Get the value of the ModelFlux slot measurement.
    FluxSlotDefinition::MeasValue getModelInstFlux() const;

    /// Get the uncertainty on the ModelFlux slot measurement.
    FluxSlotDefinition::ErrValue getModelInstFluxErr() const;

    /// Return true if the measurement in the ModelFlux slot failed.
    bool getModelFluxFlag() const;

    /// Get the value of the ApFlux slot measurement.
    FluxSlotDefinition::MeasValue getApInstFlux() const;

    /// Get the uncertainty on the ApFlux slot measurement.
    FluxSlotDefinition::ErrValue getApInstFluxErr() const;

    /// Return true if the measurement in the ApFlux slot failed.
    bool getApFluxFlag() const;

    /// Get the value of the GaussianFlux slot measurement.
    FluxSlotDefinition::MeasValue getGaussianInstFlux() const;

    /// Get the uncertainty on the GaussianFlux slot measurement.
    FluxSlotDefinition::ErrValue getGaussianInstFluxErr() const;

    /// Return true if the measurement in the GaussianFlux slot failed.
    bool getGaussianFluxFlag() const;

    /// Get the value of the CalibFlux slot measurement.
    FluxSlotDefinition::MeasValue getCalibInstFlux() const;

    /// Get the uncertainty on the CalibFlux slot measurement.
    FluxSlotDefinition::ErrValue getCalibInstFluxErr() const;

    /// Return true if the measurement in the CalibFlux slot failed.
    bool getCalibFluxFlag() const;

    /// Get the value of the Centroid slot measurement.
    CentroidSlotDefinition::MeasValue getCentroid() const;

    /// Get the uncertainty on the Centroid slot measurement.
    CentroidSlotDefinition::ErrValue getCentroidErr() const;

    /// Return true if the measurement in the Centroid slot failed.
    bool getCentroidFlag() const;

    /// Get the value of the Shape slot measurement.
    ShapeSlotDefinition::MeasValue getShape() const;

    /// Get the uncertainty on the Shape slot measurement.
    ShapeSlotDefinition::ErrValue getShapeErr() const;

    /// Return true if the measurement in the Shape slot failed.
    bool getShapeFlag() const;

    /// Return the centroid slot x coordinate.
    double getX() const;

    /// Return the centroid slot y coordinate.
    double getY() const;

    /// Return the shape slot Ixx value.
    double getIxx() const;

    /// Return the shape slot Iyy value.
    double getIyy() const;

    /// Return the shape slot Ixy value.
    double getIxy() const;

    /// Update the coord field using the given Wcs and the field in the centroid slot.
    void updateCoord(geom::SkyWcs const &wcs);

    /// Update the coord field using the given Wcs and the image center from the given key.
    void updateCoord(geom::SkyWcs const &wcs, PointKey<double> const &key);

    SourceRecord(const SourceRecord &) = delete;
    SourceRecord &operator=(const SourceRecord &) = delete;
    SourceRecord(SourceRecord &&) = delete;
    SourceRecord &operator=(SourceRecord &&) = delete;
    ~SourceRecord();

protected:

    virtual void _assign(BaseRecord const &other);

private:
    friend class SourceTable;

    std::shared_ptr<Footprint> _footprint;
};

/**
 *  Table class that contains measurements made on a single exposure.
 *
 *  @copydetails SourceRecord
 */
class SourceTable : public SimpleTable {
public:
    typedef SourceRecord Record;
    typedef SourceColumnViewT<SourceRecord> ColumnView;
    typedef SortedCatalogT<Record> Catalog;
    typedef SortedCatalogT<Record const> ConstCatalog;

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
    static std::shared_ptr<SourceTable> make(Schema const &schema,
                                             std::shared_ptr<IdFactory> const &idFactory);

    /**
     *  Construct a new table.
     *
     *  @param[in] schema            Schema that defines the fields, offsets, and record size for the table.
     *
     *  This overload sets the ID factory to IdFactory::makeSimple().
     */
    static std::shared_ptr<SourceTable> make(Schema const &schema) {
        return make(schema, IdFactory::makeSimple());
    }

    /**
     *  Return a minimal schema for Source tables and records.
     *
     *  The returned schema can and generally should be modified further,
     *  but many operations on sources will assume that at least the fields
     *  provided by this routine are present.
     *
     *  Keys for the standard fields added by this routine can be obtained
     *  from other static member functions of the SourceTable class.
     */
    static Schema makeMinimalSchema() {
        Schema r = getMinimalSchema().schema;
        r.disconnectAliases();
        return r;
    }

    /**
     *  Return true if the given schema is a valid SourceTable schema.
     *
     *  This will always be true if the given schema was originally constructed
     *  using makeMinimalSchema(), and will rarely be true otherwise.
     */
    static bool checkSchema(Schema const &other) { return other.contains(getMinimalSchema().schema); }

    /// Key for the parent ID.
    static Key<RecordId> getParentKey() { return getMinimalSchema().parent; }

    /// @copydoc BaseTable::clone
    std::shared_ptr<SourceTable> clone() const { return std::static_pointer_cast<SourceTable>(_clone()); }

    /// @copydoc BaseTable::makeRecord
    std::shared_ptr<SourceRecord> makeRecord() {
        return std::static_pointer_cast<SourceRecord>(_makeRecord());
    }

    /// @copydoc BaseTable::copyRecord
    std::shared_ptr<SourceRecord> copyRecord(BaseRecord const &other) {
        return std::static_pointer_cast<SourceRecord>(BaseTable::copyRecord(other));
    }

    /// @copydoc BaseTable::copyRecord
    std::shared_ptr<SourceRecord> copyRecord(BaseRecord const &other, SchemaMapper const &mapper) {
        return std::static_pointer_cast<SourceRecord>(BaseTable::copyRecord(other, mapper));
    }

    FluxSlotDefinition const &getPsfFluxSlot() const { return _slots.defPsfFlux; }

    /**
     *  Set the measurement used for the PsfFlux slot.
     *
     *  The definitions for slots are actually managed by the Schema object, and its associated
     *  AliasMap, so this simply sets the "slot_PsfFlux" alias
     *  to point to the given field name prefix.  See FluxSlotDefinition for more information.
     */
    void definePsfFlux(std::string const &name) {
        getSchema().getAliasMap()->set(getPsfFluxSlot().getAlias(), name);
    }

    FluxSlotDefinition const &getModelFluxSlot() const { return _slots.defModelFlux; }

    /**
     *  Set the measurement used for the ModelFlux slot.
     *
     *  The definitions for slots are actually managed by the Schema object, and its associated
     *  AliasMap, so this simply sets the "slot_ModelFlux" alias
     *  to point to the given field name prefix.  See FluxSlotDefinition for more information.
     */
    void defineModelFlux(std::string const &name) {
        getSchema().getAliasMap()->set(getModelFluxSlot().getAlias(), name);
    }

    FluxSlotDefinition const &getApFluxSlot() const { return _slots.defApFlux; }

    /**
     *  Set the measurement used for the ApFlux slot.
     *
     *  The definitions for slots are actually managed by the Schema object, and its associated
     *  AliasMap, so this simply sets the "slot_ApFlux" alias
     *  to point to the given field name prefix.  See FluxSlotDefinition for more information.
     */
    void defineApFlux(std::string const &name) {
        getSchema().getAliasMap()->set(getApFluxSlot().getAlias(), name);
    }

    FluxSlotDefinition const &getGaussianFluxSlot() const { return _slots.defGaussianFlux; }

    /**
     *  Set the measurement used for the GaussianFlux slot.
     *
     *  The definitions for slots are actually managed by the Schema object, and its associated
     *  AliasMap, so this simply sets the "slot_GaussianFlux" alias
     *  to point to the given field name prefix.  See FluxSlotDefinition for more information.
     */
    void defineGaussianFlux(std::string const &name) {
        getSchema().getAliasMap()->set(getGaussianFluxSlot().getAlias(), name);
    }

    FluxSlotDefinition const &getCalibFluxSlot() const { return _slots.defCalibFlux; }

    /**
     *  Set the measurement used for the CalibFlux slot.
     *
     *  The definitions for slots are actually managed by the Schema object, and its associated
     *  AliasMap, so this simply sets the "slot_CalibFlux" alias
     *  to point to the given field name prefix.  See FluxSlotDefinition for more information.
     */
    void defineCalibFlux(std::string const &name) {
        getSchema().getAliasMap()->set(getCalibFluxSlot().getAlias(), name);
    }

    CentroidSlotDefinition const &getCentroidSlot() const { return _slots.defCentroid; }

    /**
     *  Set the measurement used for the Centroid slot.
     *
     *  The definitions for slots are actually managed by the Schema object, and its associated
     *  AliasMap, so this simply sets the "slot_Centroid" alias
     *  to point to the given field name prefix.  See CentroidSlotDefinition for more information.
     */
    void defineCentroid(std::string const &name) {
        getSchema().getAliasMap()->set(getCentroidSlot().getAlias(), name);
    }

    ShapeSlotDefinition const &getShapeSlot() const { return _slots.defShape; }

    /**
     *  Set the measurement used for the Shape slot.
     *
     *  The definitions for slots are actually managed by the Schema object, and its associated
     *  AliasMap, so this simply sets the "slot_Shape" alias
     *  to point to the given field name prefix.  See ShapeSlotDefinition for more information.
     */
    void defineShape(std::string const &name) {
        getSchema().getAliasMap()->set(getShapeSlot().getAlias(), name);
    }

    SourceTable &operator=(SourceTable const &) = delete;
    SourceTable &operator=(SourceTable &&) = delete;

protected:
    SourceTable(Schema const &schema, std::shared_ptr<IdFactory> const &idFactory);

    SourceTable(SourceTable const &other);
    SourceTable(SourceTable &&other);

    void handleAliasChange(std::string const &alias) override;

    std::shared_ptr<BaseTable> _clone() const override;

    std::shared_ptr<BaseRecord> _makeRecord() override;

private:
    // Struct that holds the minimal schema and the special keys we've added to it.
    struct MinimalSchema {
        Schema schema;
        Key<RecordId> parent;

        MinimalSchema();
    };

    // Return the singleton minimal schema.
    static MinimalSchema &getMinimalSchema();

    friend class io::FitsWriter;
    friend class SourceRecord;

    // Return a writer object that knows how to save in FITS format.  See also FitsWriter.
    std::shared_ptr<io::FitsWriter> makeFitsWriter(fits::Fits *fitsfile, int flags) const override;

    SlotSuite _slots;
};

template <typename RecordT>
class SourceColumnViewT : public ColumnViewT<RecordT> {
public:
    typedef RecordT Record;
    typedef typename RecordT::Table Table;

    // See the documentation for BaseColumnView for an explanation of why these
    // accessors *appear* to violate const-correctness.

    /// Get the value of the PsfFlux slot measurement.
    ndarray::Array<double, 1> getPsfInstFlux() const {
        return this->operator[](this->getTable()->getPsfFluxSlot().getMeasKey());
    }
    /// Get the uncertainty on the PsfFlux slot measurement.
    ndarray::Array<double, 1> getPsfInstFluxErr() const {
        return this->operator[](this->getTable()->getPsfFluxSlot().getErrKey());
    }

    /// Get the value of the ApFlux slot measurement.
    ndarray::Array<double, 1> getApInstFlux() const {
        return this->operator[](this->getTable()->getApFluxSlot().getMeasKey());
    }
    /// Get the uncertainty on the ApFlux slot measurement.
    ndarray::Array<double, 1> getApInstFluxErr() const {
        return this->operator[](this->getTable()->getApFluxSlot().getErrKey());
    }

    /// Get the value of the ModelFlux slot measurement.
    ndarray::Array<double, 1> getModelInstFlux() const {
        return this->operator[](this->getTable()->getModelFluxSlot().getMeasKey());
    }
    /// Get the uncertainty on the ModelFlux slot measurement.
    ndarray::Array<double, 1> getModelInstFluxErr() const {
        return this->operator[](this->getTable()->getModelFluxSlot().getErrKey());
    }

    /// Get the value of the GaussianFlux slot measurement.
    ndarray::Array<double, 1> getGaussianInstFlux() const {
        return this->operator[](this->getTable()->getGaussianFluxSlot().getMeasKey());
    }
    /// Get the uncertainty on the GaussianFlux slot measurement.
    ndarray::Array<double, 1> getGaussianInstFluxErr() const {
        return this->operator[](this->getTable()->getGaussianFluxSlot().getErrKey());
    }

    /// Get the value of the CalibFlux slot measurement.
    ndarray::Array<double, 1> getCalibInstFlux() const {
        return this->operator[](this->getTable()->getCalibFluxSlot().getMeasKey());
    }
    /// Get the uncertainty on the CalibFlux slot measurement.
    ndarray::Array<double, 1> getCalibInstFluxErr() const {
        return this->operator[](this->getTable()->getCalibFluxSlot().getErrKey());
    }

    ndarray::Array<double, 1> const getX() const {
        return this->operator[](this->getTable()->getCentroidSlot().getMeasKey().getX());
    }
    ndarray::Array<double, 1> const getY() const {
        return this->operator[](this->getTable()->getCentroidSlot().getMeasKey().getY());
    }

    ndarray::Array<double, 1> const getIxx() const {
        return this->operator[](this->getTable()->getShapeSlot().getMeasKey().getIxx());
    }
    ndarray::Array<double, 1> const getIyy() const {
        return this->operator[](this->getTable()->getShapeSlot().getMeasKey().getIyy());
    }
    ndarray::Array<double, 1> const getIxy() const {
        return this->operator[](this->getTable()->getShapeSlot().getMeasKey().getIxy());
    }

    /// @copydoc BaseColumnView::make
    template <typename InputIterator>
    static SourceColumnViewT make(std::shared_ptr<Table> const &table, InputIterator first,
                                  InputIterator last) {
        return SourceColumnViewT(BaseColumnView::make(table, first, last));
    }

    SourceColumnViewT(SourceColumnViewT const &) = default;
    SourceColumnViewT(SourceColumnViewT &&) = default;
    SourceColumnViewT &operator=(SourceColumnViewT const &) = default;
    SourceColumnViewT &operator=(SourceColumnViewT &&) = default;
    ~SourceColumnViewT() = default;

protected:
    explicit SourceColumnViewT(BaseColumnView const &base) : ColumnViewT<RecordT>(base) {}
};

typedef SourceColumnViewT<SourceRecord> SourceColumnView;

inline FluxSlotDefinition::MeasValue SourceRecord::getPsfInstFlux() const {
    return this->get(getTable()->getPsfFluxSlot().getMeasKey());
}

inline FluxSlotDefinition::ErrValue SourceRecord::getPsfInstFluxErr() const {
    return this->get(getTable()->getPsfFluxSlot().getErrKey());
}

inline bool SourceRecord::getPsfFluxFlag() const {
    return this->get(getTable()->getPsfFluxSlot().getFlagKey());
}

inline FluxSlotDefinition::MeasValue SourceRecord::getModelInstFlux() const {
    return this->get(getTable()->getModelFluxSlot().getMeasKey());
}

inline FluxSlotDefinition::ErrValue SourceRecord::getModelInstFluxErr() const {
    return this->get(getTable()->getModelFluxSlot().getErrKey());
}

inline bool SourceRecord::getModelFluxFlag() const {
    return this->get(getTable()->getModelFluxSlot().getFlagKey());
}

inline FluxSlotDefinition::MeasValue SourceRecord::getApInstFlux() const {
    return this->get(getTable()->getApFluxSlot().getMeasKey());
}

inline FluxSlotDefinition::ErrValue SourceRecord::getApInstFluxErr() const {
    return this->get(getTable()->getApFluxSlot().getErrKey());
}

inline bool SourceRecord::getApFluxFlag() const {
    return this->get(getTable()->getApFluxSlot().getFlagKey());
}

inline FluxSlotDefinition::MeasValue SourceRecord::getGaussianInstFlux() const {
    return this->get(getTable()->getGaussianFluxSlot().getMeasKey());
}

inline FluxSlotDefinition::ErrValue SourceRecord::getGaussianInstFluxErr() const {
    return this->get(getTable()->getGaussianFluxSlot().getErrKey());
}

inline bool SourceRecord::getGaussianFluxFlag() const {
    return this->get(getTable()->getGaussianFluxSlot().getFlagKey());
}

inline FluxSlotDefinition::MeasValue SourceRecord::getCalibInstFlux() const {
    return this->get(getTable()->getCalibFluxSlot().getMeasKey());
}

inline FluxSlotDefinition::ErrValue SourceRecord::getCalibInstFluxErr() const {
    return this->get(getTable()->getCalibFluxSlot().getErrKey());
}

inline bool SourceRecord::getCalibFluxFlag() const {
    return this->get(getTable()->getCalibFluxSlot().getFlagKey());
}

inline CentroidSlotDefinition::MeasValue SourceRecord::getCentroid() const {
    return this->get(getTable()->getCentroidSlot().getMeasKey());
}

inline CentroidSlotDefinition::ErrValue SourceRecord::getCentroidErr() const {
    return this->get(getTable()->getCentroidSlot().getErrKey());
}

inline bool SourceRecord::getCentroidFlag() const {
    return this->get(getTable()->getCentroidSlot().getFlagKey());
}

inline ShapeSlotDefinition::MeasValue SourceRecord::getShape() const {
    return this->get(getTable()->getShapeSlot().getMeasKey());
}

inline ShapeSlotDefinition::ErrValue SourceRecord::getShapeErr() const {
    return this->get(getTable()->getShapeSlot().getErrKey());
}

inline bool SourceRecord::getShapeFlag() const { return this->get(getTable()->getShapeSlot().getFlagKey()); }

inline RecordId SourceRecord::getParent() const { return get(SourceTable::getParentKey()); }
inline void SourceRecord::setParent(RecordId id) { set(SourceTable::getParentKey(), id); }
inline double SourceRecord::getX() const { return get(getTable()->getCentroidSlot().getMeasKey().getX()); }
inline double SourceRecord::getY() const { return get(getTable()->getCentroidSlot().getMeasKey().getY()); }
inline double SourceRecord::getIxx() const { return get(getTable()->getShapeSlot().getMeasKey().getIxx()); }
inline double SourceRecord::getIyy() const { return get(getTable()->getShapeSlot().getMeasKey().getIyy()); }
inline double SourceRecord::getIxy() const { return get(getTable()->getShapeSlot().getMeasKey().getIxy()); }

}  // namespace table
}  // namespace afw
}  // namespace lsst

#endif  // !AFW_TABLE_Source_h_INCLUDED
