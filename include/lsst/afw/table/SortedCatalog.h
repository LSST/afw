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
#ifndef AFW_TABLE_SortedCatalog_h_INCLUDED
#define AFW_TABLE_SortedCatalog_h_INCLUDED

#include "lsst/afw/fitsDefaults.h"
#include "lsst/afw/table/fwd.h"
#include "lsst/afw/table/Catalog.h"

namespace lsst {
namespace afw {
namespace table {

/**
 *  @brief Custom catalog class for record/table subclasses that are guaranteed to have an ID,
 *         and should generally be sorted by that ID.
 *
 *  For a record/table pair to be used with SortedCatalogT, the table class must provide a static
 *  getIdKey() member function that returns the key to the ID field.
 */
template <typename RecordT>
class SortedCatalogT : public CatalogT<RecordT> {
    using Base = CatalogT<RecordT>;

public:
    using Record = RecordT;
    using Table = typename Record::Table;

    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;

    using Base::isSorted;
    using Base::sort;
    using Base::find;

    SortedCatalogT(SortedCatalogT const&) = default;
    SortedCatalogT(SortedCatalogT&&) noexcept = default;
    SortedCatalogT& operator=(SortedCatalogT const&) = default;
    SortedCatalogT& operator=(SortedCatalogT&&) = default;
    ~SortedCatalogT() = default;

    /// Return true if the vector is in ascending ID order.
    bool isSorted() const { return this->isSorted(Table::getIdKey()); }

    /// Sort the vector in-place by ID.
    void sort() { this->sort(Table::getIdKey()); }

    //@{
    /**
     *  Return an iterator to the record with the given ID.
     *
     *  @note The vector must be sorted in ascending ID order before calling find (i.e.
     *        isSorted() must be true).
     *
     *  Returns end() if the Record cannot be found.
     */
    iterator find(RecordId id) { return this->find(id, Table::getIdKey()); }
    const_iterator find(RecordId id) const { return this->find(id, Table::getIdKey()); }
    //@}

    /**
     *  Construct a vector from a table (or nothing).
     *
     *  A vector with no table is considered invalid; a valid table must be assigned to it
     *  before it can be used.
     */
    explicit SortedCatalogT(std::shared_ptr<Table> const& table = std::shared_ptr<Table>()) : Base(table) {}

    /// Construct a vector from a schema, creating a table with Table::make(schema).
    explicit SortedCatalogT(Schema const& schema) : Base(schema) {}

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
    SortedCatalogT(std::shared_ptr<Table> const& table, InputIterator first, InputIterator last,
                   bool deep = false)
            : Base(table, first, last, deep) {}

    /**
     *  Shallow copy constructor from a container containing a related record type.
     *
     *  This conversion only succeeds if OtherRecordT is convertible to RecordT and OtherTable is
     *  convertible to Table.
     */
    template <typename OtherRecordT>
    SortedCatalogT(SortedCatalogT<OtherRecordT> const& other) : Base(other) {}

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
    static SortedCatalogT readFits(std::string const& filename, int hdu = fits::DEFAULT_HDU, int flags = 0) {
        return io::FitsReader::apply<SortedCatalogT>(filename, hdu, flags);
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
    static SortedCatalogT readFits(fits::MemFileManager& manager, int hdu = fits::DEFAULT_HDU,
                                   int flags = 0) {
        return io::FitsReader::apply<SortedCatalogT>(manager, hdu, flags);
    }

    /**
     *  Read a FITS binary table from a file object already at the correct extension.
     *
     *  @param[in] fitsfile    Fits file object to read from.
     *  @param[in] flags       Table-subclass-dependent bitflags that control the details of how to read
     *                         the catalog.  See e.g. SourceFitsFlags.
     */
    static SortedCatalogT readFits(fits::Fits& fitsfile, int flags = 0) {
        return io::FitsReader::apply<SortedCatalogT>(fitsfile, flags);
    }

    /**
     *  Return the subset of a catalog corresponding to the True values of the given mask array.
     *
     *  The returned array's records are shallow copies, and hence will not in general be contiguous.
     */
    SortedCatalogT<RecordT> subset(ndarray::Array<bool const, 1> const& mask) const {
        return SortedCatalogT(Base::subset(mask));
    }

    /**
     * Shallow copy a subset of another SortedCatalog.  Mostly here for
     * use from python.
     */
    SortedCatalogT subset(std::ptrdiff_t startd, std::ptrdiff_t stopd, std::ptrdiff_t step) const {
        return SortedCatalogT(Base::subset(startd, stopd, step));
    }

protected:
    explicit SortedCatalogT(Base const& other) : Base(other) {}
};
}  // namespace table
}  // namespace afw
}  // namespace lsst

#endif  // !AFW_TABLE_SortedCatalog_h_INCLUDED
