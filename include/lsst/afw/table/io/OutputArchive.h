// -*- lsst-c++ -*-
#ifndef AFW_TABLE_IO_OutputArchive_h_INCLUDED
#define AFW_TABLE_IO_OutputArchive_h_INCLUDED

#include "lsst/base.h"
#include "lsst/afw/table/io/Persistable.h"

namespace lsst {
namespace afw {
namespace table {

class Schema;
class BaseRecord;
template <typename RecordT>
class CatalogT;
using BaseCatalog = CatalogT<BaseRecord>;

namespace io {

class Persistable;
class OutputArchiveHandle;

/**
 *  A multi-catalog archive object used to save table::io::Persistable objects.
 *
 *  OutputArchive should generally be used directly only by objects that do not themselves
 *  inherit from Persistable, but contain many objects that do (such as Exposure).  It provides
 *  an interface for adding objects to the archive (put()), transforming them into catalogs
 *  that can be retrieved directly or written to a FITS file.  The first catalog is an index
 *  that indicates which rows of the subsequent catalogs correspond to each object.
 *
 *  See getIndexCatalog() for a more detailed description of the index.
 */
class OutputArchive final {
public:
    friend class OutputArchiveHandle;

    /// Construct an empty OutputArchive containing no objects.
    OutputArchive();

    /// Copy-construct an OutputArchive.  Saved objects are not deep-copied.
    OutputArchive(OutputArchive const& other);
    OutputArchive(OutputArchive&& other);

    /// Assign from another OutputArchive.  Saved objects are not deep-copied.
    OutputArchive& operator=(OutputArchive const& other);
    OutputArchive& operator=(OutputArchive&& other);

    // (trivial) destructor must be defined in the source for pimpl idiom.
    ~OutputArchive();

    ///@{
    /**
     *  @brief Save an object to the archive and return a unique ID that can
     *         be used to retrieve it from an InputArchive.
     *
     *  If permissive is true and obj->isPersistable() is false, the object
     *  will not be saved but 0 will be returned instead of throwing an
     *  exception.
     *
     *  If the given pointer is null, the returned ID is always 0, which may
     *  be used to retrieve null pointers from an InputArchive.
     *
     *  It is expected that the `shared_ptr` form will usually be used, as
     *  Persistables are typically held by `shared_ptr`.  We also provide a
     *  const reference overload for temporaries as well as a const raw
     *  pointer overload for temporaries that may be null.
     *
     *  If the shared_ptr form is used and the given pointer has already been
     *  saved, it will not be written again and the same ID will be returned as
     *  the first time it was saved.
     *
     *  @exceptsafe Provides no exception safety for the archive itself - if
     *              the object being saved (or any nested object) throws an
     *              exception, the archive may be in an inconsistent state and
     *              should not be saved. The objects being saved are never
     *              modified during persistence, even when exceptions are
     *              thrown.
     */
    int put(std::shared_ptr<Persistable const> obj, bool permissive = false);
    int put(Persistable const * obj, bool permissive = false);
    int put(Persistable const & obj, bool permissive = false) { return put(&obj, permissive); }
    ///@}

    /**
     *  @brief Return the index catalog that specifies where objects are stored in the
     *         data catalogs.
     */
    BaseCatalog const& getIndexCatalog() const;

    /// Return the nth catalog.  Catalog 0 is always the index catalog.
    BaseCatalog const& getCatalog(int n) const;

    /// Return the total number of catalogs, including the index.
    std::size_t countCatalogs() const;

    /**
     *  Write the archive to an already-open FITS object.
     *
     *  Always appends new HDUs.
     *
     *  @param[in] fitsfile     Open FITS object to write to.
     */
    void writeFits(fits::Fits& fitsfile) const;

private:
    class Impl;

    std::shared_ptr<Impl> _impl;
};

/**
 *  An object passed to Persistable::write to allow it to persist itself.
 *
 *  OutputArchiveHandle provides an interface to add additional catalogs and save nested
 *  Persistables to the same archive.
 */
class OutputArchiveHandle final {
public:
    /**
     *  Return a new, empty catalog with the given schema.
     *
     *  All catalogs passed to saveCatalog should be originally
     *  created by makeCatalog, or at least share the same table.
     */
    BaseCatalog makeCatalog(Schema const& schema);

    /**
     *  Indicate that the object being persisted has no state,
     *  and hence will never call makeCatalog() or saveCatalog().
     */
    void saveEmpty();

    /**
     *  Save a catalog in the archive.
     *
     *  The catalog must have been created using makeCatalog,
     *  or be a shallow copy or subset of such a catalog.
     */
    void saveCatalog(BaseCatalog const& catalog);

    ///@{
    /// @copydoc OutputArchive::put(Persistable const*, bool)
    int put(Persistable const* obj, bool permissive = false);
    int put(std::shared_ptr<Persistable const> obj, bool permissive = false);
    int put(Persistable const & obj, bool permissive = false) { return put(&obj, permissive); }
    ///@}

    ~OutputArchiveHandle();

    // No copying
    OutputArchiveHandle(const OutputArchiveHandle&) = delete;
    OutputArchiveHandle& operator=(const OutputArchiveHandle&) = delete;

    // No moving
    OutputArchiveHandle(OutputArchiveHandle&&) = delete;
    OutputArchiveHandle& operator=(OutputArchiveHandle&&) = delete;

private:
    friend class OutputArchive::Impl;

    OutputArchiveHandle(int id, std::string const& name, std::string const& module,
                        std::shared_ptr<OutputArchive::Impl> impl);

    int _id;
    int _catPersistable;
    std::string _name;
    std::string _module;
    std::shared_ptr<OutputArchive::Impl> _impl;
};
}  // namespace io
}  // namespace table
}  // namespace afw
}  // namespace lsst

#endif  // !AFW_TABLE_IO_OutputArchive_h_INCLUDED
