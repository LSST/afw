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
#ifndef AFW_TABLE_Exposure_h_INCLUDED
#define AFW_TABLE_Exposure_h_INCLUDED

#include "lsst/geom/Box.h"
#include "lsst/geom/SpherePoint.h"
#include "lsst/afw/fitsDefaults.h"
#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/BaseTable.h"
#include "lsst/afw/table/SortedCatalog.h"
#include "lsst/afw/table/BaseColumnView.h"
#include "lsst/afw/table/io/FitsWriter.h"
#include "lsst/afw/table/aggregates.h"

namespace lsst {
namespace afw {

namespace image {
class PhotoCalib;
class ApCorrMap;
class VisitInfo;
class TransmissionCurve;
}  // namespace image

namespace detection {
class Psf;
}  // namespace detection

namespace geom {
class SkyWcs;
namespace polygon {
class Polygon;
}
}  // namespace geom

namespace cameraGeom {
class Detector;
}  // namespace cameraGeom

namespace table {

class ExposureRecord;
class ExposureTable;

template <typename RecordT>
class ExposureCatalogT;

namespace io {

class OutputArchiveHandle;
class InputArchive;

}  // namespace io

/**
 *  Record class used to store exposure metadata.
 */
class ExposureRecord : public BaseRecord {
public:
    using Table = ExposureTable;
    using ColumnView = ColumnViewT<ExposureRecord>;
    using Catalog = ExposureCatalogT<ExposureRecord>;
    using ConstCatalog = ExposureCatalogT<const ExposureRecord>;

    /**
     *  Constructor used by ExposureTable.
     *
     *  While formally public, this constructor is conceptually and effectively
     *  protected, due to the (protected) ConstructionToken argument.
     *
     *  This is to allow make_shared to be used, as that cannot be used on a
     *  truly protected or private constructor.
     */
    ExposureRecord(ConstructionToken const & token, detail::RecordData && data) :
        BaseRecord(token, std::move(data))
    {}

    std::shared_ptr<ExposureTable const> getTable() const {
        return std::static_pointer_cast<ExposureTable const>(BaseRecord::getTable());
    }

    RecordId getId() const;
    void setId(RecordId id);

    lsst::geom::Box2I getBBox() const;
    void setBBox(lsst::geom::Box2I const& bbox);

    /**
     *  @brief Return true if the bounding box contains the given celestial coordinate point, taking
     *         into account the Wcs of the ExposureRecord.
     *
     *  If includeValidPolygon is true it will also check
     *  that the point is within the validPolygon of this ExpoureRecord if it has one; otherwise,
     *  this argument is ignored.
     *
     *  @throws pex::exceptions::LogicError if the ExposureRecord has no Wcs.
     */
    bool contains(lsst::geom::SpherePoint const& coord, bool includeValidPolygon = false) const;

    /**
     *  @brief Return true if the bounding box contains the given point, taking into account its Wcs
     *         (given) and the Wcs of the ExposureRecord.
     *
     *  If includeValidPolygon is true it will also check
     *         that the point is within the validPolygon of this ExposureRecord if present if it has one;
     *         otherwise, this argument is ignored.
     *
     *  @throws pex::exceptions::LogicError if the ExposureRecord has no Wcs.
     */
    bool contains(lsst::geom::Point2D const& point, geom::SkyWcs const& wcs,
                  bool includeValidPolygon = false) const;

    //@{
    /// Get/Set the the attached Wcs, Psf, PhotoCalib, or ApCorrMap.  No copies are made.
    std::shared_ptr<geom::SkyWcs const> getWcs() const { return _wcs; }
    void setWcs(std::shared_ptr<geom::SkyWcs const> wcs) { _wcs = wcs; }

    std::shared_ptr<detection::Psf const> getPsf() const { return _psf; }
    void setPsf(std::shared_ptr<detection::Psf const> psf) { _psf = psf; }

    std::shared_ptr<image::PhotoCalib const> getPhotoCalib() const { return _photoCalib; }
    void setPhotoCalib(std::shared_ptr<image::PhotoCalib const> photoCalib) { _photoCalib = photoCalib; }

    std::shared_ptr<image::ApCorrMap const> getApCorrMap() const { return _apCorrMap; }
    void setApCorrMap(std::shared_ptr<image::ApCorrMap const> apCorrMap) { _apCorrMap = apCorrMap; }

    std::shared_ptr<geom::polygon::Polygon const> getValidPolygon() const { return _validPolygon; }
    void setValidPolygon(std::shared_ptr<geom::polygon::Polygon const> polygon) { _validPolygon = polygon; }

    std::shared_ptr<image::VisitInfo const> getVisitInfo() const { return _visitInfo; }
    void setVisitInfo(std::shared_ptr<image::VisitInfo const> visitInfo) { _visitInfo = visitInfo; }

    std::shared_ptr<image::TransmissionCurve const> getTransmissionCurve() const {
        return _transmissionCurve;
    }
    void setTransmissionCurve(std::shared_ptr<image::TransmissionCurve const> transmissionCurve) {
        _transmissionCurve = std::move(transmissionCurve);
    }

    std::shared_ptr<cameraGeom::Detector const> getDetector() const { return _detector; }
    void setDetector(std::shared_ptr<cameraGeom::Detector const> detector) {
        _detector = std::move(detector);
    }
    //@}

    ExposureRecord(ExposureRecord const&) = delete;
    ExposureRecord(ExposureRecord&&) = delete;
    ExposureRecord& operator=(ExposureRecord const&) = delete;
    ExposureRecord& operator=(ExposureRecord&&) = delete;
    ~ExposureRecord() override;

protected:

    void _assign(BaseRecord const& other) override;

private:
    friend class ExposureTable;

    std::shared_ptr<geom::SkyWcs const> _wcs;
    std::shared_ptr<detection::Psf const> _psf;
    std::shared_ptr<image::PhotoCalib const> _photoCalib;
    std::shared_ptr<image::ApCorrMap const> _apCorrMap;
    std::shared_ptr<geom::polygon::Polygon const> _validPolygon;
    std::shared_ptr<image::VisitInfo const> _visitInfo;
    std::shared_ptr<image::TransmissionCurve const> _transmissionCurve;
    std::shared_ptr<cameraGeom::Detector const> _detector;
};

/**
 *  Table class used to store exposure metadata.
 *
 *  @copydetails ExposureRecord
 */
class ExposureTable : public BaseTable {
public:
    using Record = ExposureRecord;
    using ColumnView = ColumnViewT<ExposureRecord>;
    using Catalog = ExposureCatalogT<Record>;
    using ConstCatalog = ExposureCatalogT<const Record>;

    /**
     *  Construct a new table.
     *
     *  @param[in] schema            Schema that defines the fields, offsets, and record size for the table.
     */
    static std::shared_ptr<ExposureTable> make(Schema const& schema);

    /**
     *  Return a minimal schema for Exposure tables and records.
     *
     *  The returned schema can and generally should be modified further,
     *  but many operations on ExposureRecords will assume that at least the fields
     *  provided by this routine are present.
     */
    static Schema makeMinimalSchema() {
        Schema r = getMinimalSchema().schema;
        r.disconnectAliases();
        return r;
    }

    /**
     *  Return true if the given schema is a valid ExposureTable schema.
     *
     *  This will always be true if the given schema was originally constructed
     *  using makeMinimalSchema(), and will rarely be true otherwise.
     */
    static bool checkSchema(Schema const& other) { return other.contains(getMinimalSchema().schema); }

    //@{
    /**
     *  Get keys for standard fields shared by all references.
     *
     *  These keys are used to implement getters and setters on ExposureRecord.
     */
    /// Key for the unique ID.
    static Key<RecordId> getIdKey() { return getMinimalSchema().id; }
    /// Key for the minimum point of the bbox.
    static PointKey<int> getBBoxMinKey() { return getMinimalSchema().bbox.getMin(); }
    /// Key for the maximum point of the bbox.
    static PointKey<int> getBBoxMaxKey() { return getMinimalSchema().bbox.getMax(); }
    /// Key for the full bbox.
    static Box2IKey getBBoxKey() { return getMinimalSchema().bbox; }
    //@}

    /// @copydoc BaseTable::clone
    std::shared_ptr<ExposureTable> clone() const { return std::static_pointer_cast<ExposureTable>(_clone()); }

    /// @copydoc BaseTable::makeRecord
    std::shared_ptr<ExposureRecord> makeRecord() {
        return std::static_pointer_cast<ExposureRecord>(_makeRecord());
    }

    /// @copydoc BaseTable::copyRecord
    std::shared_ptr<ExposureRecord> copyRecord(BaseRecord const& other) {
        return std::static_pointer_cast<ExposureRecord>(BaseTable::copyRecord(other));
    }

    /// @copydoc BaseTable::copyRecord
    std::shared_ptr<ExposureRecord> copyRecord(BaseRecord const& other, SchemaMapper const& mapper) {
        return std::static_pointer_cast<ExposureRecord>(BaseTable::copyRecord(other, mapper));
    }

    ExposureTable& operator=(ExposureTable const&) = delete;
    ExposureTable& operator=(ExposureTable&&) = delete;
    ~ExposureTable() override;

protected:
    explicit ExposureTable(Schema const& schema);

    ExposureTable(ExposureTable const& other);

    std::shared_ptr<BaseTable> _clone() const override;

    std::shared_ptr<BaseRecord> _makeRecord() override;

private:
    // Struct that holds the minimal schema and the special keys we've added to it.
    struct MinimalSchema {
        Schema schema;
        Key<RecordId> id;
        Box2IKey bbox;

        MinimalSchema();
    };

    // Return the singleton minimal schema.
    static MinimalSchema& getMinimalSchema();

    friend class io::FitsWriter;

    template <typename RecordT>
    friend class ExposureCatalogT;

    // Return a writer object that knows how to save in FITS format.  See also FitsWriter.
    std::shared_ptr<io::FitsWriter> makeFitsWriter(fits::Fits* fitsfile, int flags) const override;

    std::shared_ptr<io::FitsWriter> makeFitsWriter(fits::Fits* fitsfile,
                                                   std::shared_ptr<io::OutputArchive> archive,
                                                   int flags) const;
};

/**
 *  Custom catalog class for ExposureRecord/Table.
 *
 *  We don't expect to subclass ExposureRecord/Table, so unlike other Catalogs we can (and do) define
 *  some ExposureCatalogT member functions in Exposure.cc where the explicit instantiation is done.
 */
template <typename RecordT>
class ExposureCatalogT : public SortedCatalogT<RecordT> {
    using Base = SortedCatalogT<RecordT>;

public:
    using Record = RecordT;
    using Table = typename Record::Table;

    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;

    /**
     *  Construct a vector from a table (or nothing).
     *
     *  A vector with no table is considered invalid; a valid table must be assigned to it
     *  before it can be used.
     */
    explicit ExposureCatalogT(std::shared_ptr<Table> const& table = std::shared_ptr<Table>()) : Base(table) {}

    /// Construct a vector from a schema, creating a table with Table::make(schema).
    explicit ExposureCatalogT(Schema const& schema) : Base(schema) {}

    /**
     *  Construct a vector from a table and an iterator range.
     *
     *  If deep is true, new records will be created using table->copyRecord before being inserted.
     *  If deep is false, records will be not be copied, but they must already be associated with
     *  the given table.  The table itself is never deep-copied.
     *
     *  The iterator must dereference to a record reference or const reference rather than a pointer,
     *  but should be implicitly convertible to a record pointer as well (see CatalogIterator).
     */
    template <typename InputIterator>
    ExposureCatalogT(std::shared_ptr<Table> const& table, InputIterator first, InputIterator last,
                     bool deep = false)
            : Base(table, first, last, deep) {}

    /**
     *  Shallow copy constructor from a container containing a related record type.
     *
     *  This conversion only succeeds if OtherRecordT is convertible to RecordT and OtherTable is
     *  convertible to Table.
     */
    template <typename OtherRecordT>
    ExposureCatalogT(ExposureCatalogT<OtherRecordT> const& other) : Base(other) {}

    ExposureCatalogT(ExposureCatalogT const&) = default;
    ExposureCatalogT(ExposureCatalogT&&) = default;
    ExposureCatalogT& operator=(ExposureCatalogT const&) = default;
    ExposureCatalogT& operator=(ExposureCatalogT&&) = default;
    ~ExposureCatalogT() = default;

    using Base::writeFits;

    /**
     *  Write a FITS binary table to an open file object.
     *
     *  Instead of writing nested Persistables to an internal archive and appending it
     *  to the FITS file, this overload inserts nested Persistables into the given
     *  archive and does not save it, leaving it to the user to save it later.
     */
    void writeFits(fits::Fits& fitsfile, std::shared_ptr<io::OutputArchive> archive, int flags = 0) const {
        std::shared_ptr<io::FitsWriter> writer = this->getTable()->makeFitsWriter(&fitsfile, archive, flags);
        writer->write(*this);
    }

    /**
     *  Read a FITS binary table from a regular file.
     *
     *  @param[in] filename    Name of the file to read.
     *  @param[in] hdu         Number of the "header-data unit" to read (where 0 is the Primary HDU).
     *                         The default value of afw::fits::DEFAULT_HDU is interpreted as
     *                         "the first HDU with NAXIS != 0".
     *  @param[in] flags       Table-subclass-dependent bitflags that control the details of how to read
     *                         the catalog.  See e.g. SourceFitsFlags.
     */
    static ExposureCatalogT readFits(std::string const& filename, int hdu = fits::DEFAULT_HDU,
                                     int flags = 0) {
        return io::FitsReader::apply<ExposureCatalogT>(filename, hdu, flags);
    }

    /**
     *  Read a FITS binary table from a RAM file.
     *
     *  @param[in] manager     Object that manages the memory to be read.
     *  @param[in] hdu         Number of the "header-data unit" to read (where 0 is the Primary HDU).
     *                         The default value of afw::fits::DEFAULT_HDU is interpreted as
     *                         "the first HDU with NAXIS != 0".
     *  @param[in] flags       Table-subclass-dependent bitflags that control the details of how to read
     *                         the catalog.  See e.g. SourceFitsFlags.
     */
    static ExposureCatalogT readFits(fits::MemFileManager& manager, int hdu = fits::DEFAULT_HDU,
                                     int flags = 0) {
        return io::FitsReader::apply<ExposureCatalogT>(manager, hdu, flags);
    }

    /**
     *  Read a FITS binary table from a file object already at the correct extension.
     *
     *  @param[in] fitsfile    Fits file object to read from.
     *  @param[in] flags       Table-subclass-dependent bitflags that control the details of how to read
     *                         the catalog.  See e.g. SourceFitsFlags.
     */
    static ExposureCatalogT readFits(fits::Fits& fitsfile, int flags = 0) {
        return io::FitsReader::apply<ExposureCatalogT>(fitsfile, flags);
    }

    /**
     *  Read a FITS binary table from a file object already at the correct extension.
     *
     *  This overload reads nested Persistables from the given archive instead of loading
     *  a new archive from the HDUs following the catalog.
     */
    static ExposureCatalogT readFits(fits::Fits& fitsfile, std::shared_ptr<io::InputArchive> archive,
                                     int flags = 0) {
        return io::FitsReader::apply<ExposureCatalogT>(fitsfile, flags, archive);
    }

    /**
     *  Convenience output function for Persistables that contain an ExposureCatalog.
     *
     *  Unlike writeFits, this saves main catalog to one of the tables within the archive,
     *  as part of a Persistable's set of catalogs, rather than saving it to a separate HDU
     *  not managed by the archive.
     */
    void writeToArchive(io::OutputArchiveHandle& handle, bool ignoreUnpersistable = true) const;

    /**
     *  Convenience input function for Persistables that contain an ExposureCatalog.
     *
     *  Unlike the FITS read methods, this reader is not polymorphically aware - it always
     *  tries to create an ExposureTable rather than infer the type of table from the data.
     */
    static ExposureCatalogT readFromArchive(io::InputArchive const& archive, BaseCatalog const& catalog);

    /**
     *  Return the subset of a catalog corresponding to the True values of the given mask array.
     *
     *  The returned array's records are shallow copies, and hence will not in general be contiguous.
     */
    ExposureCatalogT<RecordT> subset(ndarray::Array<bool const, 1> const& mask) const {
        return ExposureCatalogT(Base::subset(mask));
    }

    /**
     * @brief Shallow copy a subset of another ExposureCatalog.  Mostly here for
     * use from python.
     */
    ExposureCatalogT subset(std::ptrdiff_t startd, std::ptrdiff_t stopd, std::ptrdiff_t step) const {
        return ExposureCatalogT(Base::subset(startd, stopd, step));
    }

    /**
     *  Return a shallow subset of the catalog with only those records that contain the given point.
     *
     *  If includeValidPolygon is true we check that the point is within the validPolygon of those
     *         records which have one; if they don't, this argument is ignored.
     *
     *  @see ExposureRecord::contains
     */
    ExposureCatalogT subsetContaining(lsst::geom::SpherePoint const& coord,
                                      bool includeValidPolygon = false) const;

    /**
     *  Return a shallow subset of the catalog with only those records that contain the given point.
     *
     *  If includeValidPolygon is true we check that the point is within the validPolygon of those
     *         records which have one; if they don't, this argument is ignored.
     *
     *  @see ExposureRecord::contains
     */
    ExposureCatalogT subsetContaining(lsst::geom::Point2D const& point, geom::SkyWcs const& wcs,
                                      bool includeValidPolygon = false) const;

protected:
    explicit ExposureCatalogT(Base const& other) : Base(other) {}
};

using ExposureColumnView = ColumnViewT<ExposureRecord>;
using ExposureCatalog = ExposureCatalogT<ExposureRecord>;
using ConstExposureCatalog = ExposureCatalogT<const ExposureRecord>;

inline RecordId ExposureRecord::getId() const { return get(ExposureTable::getIdKey()); }
inline void ExposureRecord::setId(RecordId id) { set(ExposureTable::getIdKey(), id); }
}  // namespace table
}  // namespace afw
}  // namespace lsst

#endif  // !AFW_TABLE_Exposure_h_INCLUDED
