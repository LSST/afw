// -*- lsst-c++ -*-
#ifndef AFW_TABLE_IO_Persistable_h_INCLUDED
#define AFW_TABLE_IO_Persistable_h_INCLUDED

#include <climits>
#include "lsst/base.h"
#include "lsst/pex/exceptions.h"
#include "lsst/afw/fitsDefaults.h"

namespace lsst {
namespace afw {

namespace fits {

class Fits;
class MemFileManager;

}  // namespace fits

namespace table {
namespace io {

class InputArchive;
class OutputArchive;
class OutputArchiveHandle;
class CatalogVector;

/**
 *  An exception thrown when problems occur during persistence.
 */
LSST_EXCEPTION_TYPE(PersistenceError, lsst::pex::exceptions::IoError, lsst::afw::table::io::PersistenceError)

/**
 *  An exception thrown when an InputArchive's contents do not make sense.
 *
 *  This is the exception thrown by the LSST_ARCHIVE_ASSERT macro.
 */
LSST_EXCEPTION_TYPE(MalformedArchiveError, lsst::afw::table::io::PersistenceError,
                    lsst::afw::table::io::MalformedArchiveError)

/**
 *  An assertion macro used to validate the structure of an InputArchive.
 *
 *  This assertion is not enabled/disabled by NDEBUG, and throws an exception rather than aborting,
 *  and should be reserved for errors that should only occur when an InputArchive is found to be
 *  in a state that could not have been produced by an OutputArchive.
 */
#define LSST_ARCHIVE_ASSERT(EXPR) \
    if (!(EXPR))                  \
    throw LSST_EXCEPT(lsst::afw::table::io::MalformedArchiveError, "Archive assertion failed: " #EXPR)

/**
 *  A base class for objects that can be persisted via afw::table::io Archive classes.
 *
 *  Inheriting from Persistable provides a public API for reading/writing individual objects to
 *  FITS that is fully defined in the base class, with derived classes only needing to implement
 *  persistence to catalogs.  It is expected that objects that contain multiple persistables
 *  (such as Exposures) will create their own InputArchives and OutputArchives, and use these
 *  to avoid writing the same object twice (which would otherwise be a big concern for future
 *  objects like ExposureCatalog and CoaddPsf).
 *
 *  Generally speaking, an abstract base class that inherits from Persistable should
 *  also inherit from PersistableFacade<Base>.
 *  A concrete class that inherits (possibly indirectly) from Persistable should inherit from
 *  PersistableFacade<Derived> (though this just provides a slightly nicer interface to users),
 *  implement isPersistable(), getPersistenceName(), getPythonModule(), and write(),
 *  and define a subclass of PersistableFactory.  Inheritance from PersistableFacade should
 *  always precede inheritance from Persistable.
 *
 *  Persistable has no pure virtual member functions, and instead contains a default implementation
 *  that throws LogicError when the user attempts to save an object for which persistence
 *  has not actually been implemented.
 */
class Persistable {
public:
    /**
     *  Write the object to a regular FITS file.
     *
     *  @param[in] fileName     Name of the file to write to.
     *  @param[in] mode         If "w", any existing file with the given name will be overwritten.  If
     *                          "a", new HDUs will be appended to an existing file.
     */
    void writeFits(std::string const& fileName, std::string const& mode = "w") const;

    /**
     *  Write the object to a FITS image in memory.
     *
     *  @param[in] manager      Name of the file to write to.
     *  @param[in] mode         If "w", any existing file with the given name will be overwritten.  If
     *                          "a", new HDUs will be appended to an existing file.
     */
    void writeFits(fits::MemFileManager& manager, std::string const& mode = "w") const;

    /**
     *  Write the object to an already-open FITS object.
     *
     *  @param[in] fitsfile     Open FITS object to write to.
     */
    void writeFits(fits::Fits& fitsfile) const;

    /// Return true if this particular object can be persisted using afw::table::io.
    virtual bool isPersistable() const noexcept { return false; }

    virtual ~Persistable() noexcept = default;

protected:
    // convenient for derived classes not in afw::table::io
    using OutputArchiveHandle = io::OutputArchiveHandle;

    /**
     *  Return the unique name used to persist this object and look up its factory.
     *
     *  Must be less than ArchiveIndexSchema::MAX_NAME_LENGTH characters.
     */
    virtual std::string getPersistenceName() const;

    /**
     *  @brief Return the fully-qualified Python module that should be imported to guarantee that its
     *         factory is registered.
     *
     *  Must be less than ArchiveIndexSchema::MAX_MODULE_LENGTH characters.
     *
     *  Will be ignored if empty.
     */
    virtual std::string getPythonModule() const;

    /**
     *  Write the object to one or more catalogs.
     *
     *  The handle object passed to this function provides an interface for adding new catalogs
     *  and adding nested objects to the same archive (while checking for duplicates).  See
     *  OutputArchiveHandle for more information.
     */
    virtual void write(OutputArchiveHandle& handle) const;

    Persistable() noexcept = default;

    Persistable(Persistable const& other) noexcept = default;
    Persistable(Persistable&& other) noexcept = default;

    Persistable& operator=(Persistable const& other) noexcept = default;
    Persistable& operator=(Persistable&& other) noexcept = default;

private:
    friend class io::OutputArchive;
    friend class io::InputArchive;

    template <typename T>
    friend class PersistableFacade;

    static std::shared_ptr<Persistable> _readFits(std::string const& fileName, int hdu = fits::DEFAULT_HDU);

    static std::shared_ptr<Persistable> _readFits(fits::MemFileManager& manager, int hdu = fits::DEFAULT_HDU);

    static std::shared_ptr<Persistable> _readFits(fits::Fits& fitsfile);
};

/**
 *  A CRTP facade class for subclasses of Persistable.
 *
 *  Derived classes should generally inherit from PersistableFacade at all levels,
 *  but only inherit from Persistable via the base class of each hierarchy.  For example,
 *  with Psfs:
 *
 *      class Psf: public PersistableFacade<Psf>, public Persistable { ... };
 *      class DoubleGaussianPsf: public PersistableFacade<DoubleGaussianPsf>, public Psf { ... };
 *
 *  Inheriting from PersistableFacade is not required for any classes but the base of
 *  each hierarchy, but doing so can save users from having to do some dynamic_casts.
 *
 *  @note PersistableFacade should usually be the first class in a list of base classes;
 *  if it appears after a base class that inherits from different specialization of
 *  PersistableFacade, those base class member functions will hide the desired ones.
 */
template <typename T>
class PersistableFacade {
public:
    /**
     *  Read an object from an already open FITS object.
     *
     *  @param[in]  fitsfile     FITS object to read from, already positioned at the desired HDU.
     */
    static std::shared_ptr<T> readFits(fits::Fits& fitsfile) {
        return dynamicCast(Persistable::_readFits(fitsfile));
    }

    /**
     *  Read an object from a regular FITS file.
     *
     *  @param[in]  fileName     Name of the file to read.
     *  @param[in]  hdu          HDU to read, where 0 is the primary.  The special value of
     *                           afw::fits::DEFAULT_HDU skips the primary HDU if it is empty.
     */
    static std::shared_ptr<T> readFits(std::string const& fileName, int hdu = fits::DEFAULT_HDU) {
        return dynamicCast(Persistable::_readFits(fileName, hdu));
    }

    /**
     *  Read an object from a FITS file in memory.
     *
     *  @param[in]  manager      Manager for the memory to read from.
     *  @param[in]  hdu          HDU to read, where 0 is the primary.  The special value of
     *                           afw::fits::DEFAULT_HDU skips the primary HDU if it is empty.
     */
    static std::shared_ptr<T> readFits(fits::MemFileManager& manager, int hdu = fits::DEFAULT_HDU) {
        return dynamicCast(Persistable::_readFits(manager, hdu));
    }

    /**
     * Dynamically cast a shared_ptr
     *
     * You must provide an explicit template instantiation in the .cc file
     * for each class that inherits from PersistableFacade.
     * Designed to work around RTTI issues on macOS with hidden symbols;
     *
     * @throw lsst::pex::exceptions::LogicError if the cast fails
     */
    static std::shared_ptr<T> dynamicCast(std::shared_ptr<Persistable> const &ptr);
};

#define DECLARE_PERSISTABLE_FACADE(CLASS) \
   private: \
       static afw::table::io::PersistableFacade<CLASS> facade_; \
   public: \
       static inline auto readFits = [](auto &&...args) { return facade_.readFits(args...);}; \
       static inline auto dynamicCast = [](auto &&...args) { return facade_.dynamicCast(args...);};

/**
 *  A base class for factory classes used to reconstruct objects from records.
 *
 *  Classes that inherit from Persistable should also subclass PersistableFactory,
 *  and instantiate exactly one instance of the derived factory with static duration (usually
 *  the class and instance are both defined in an anonymous namespace in a source file).
 */
class PersistableFactory {
protected:
    using InputArchive = io::InputArchive;  // convenient for derived classes not in afw::table::io
    using CatalogVector = io::CatalogVector;

public:
    /**
     *  Constructor for the factory.
     *
     *  This should be called only once, and only on an object with static duration,
     *  as a pointer to the object will be put in a singleton registry.
     *
     *  The name must be globally unique with respect to *all* Persistables and be the
     *  same as Persistable::getPersistenceName(); the Python module that a Persistable
     *  may also declare is not used to resolve names, but rather just to import the
     *  module that may install the necessary factory in the registry.
     */
    explicit PersistableFactory(std::string const& name);

    /// Construct a new object from the given InputArchive and vector of catalogs.
    virtual std::shared_ptr<Persistable> read(InputArchive const& archive,
                                              CatalogVector const& catalogs) const = 0;

    /**
     *  Return the factory that has been registered with the given name.
     *
     *  If the lookup fails and module is not an empty string, we will attempt to import a Python
     *  module with that name (this will only work when the C++ is being called from Python) and
     *  try again.
     */
    static PersistableFactory const& lookup(std::string const& name, std::string const& module = "");

    virtual ~PersistableFactory() noexcept = default;

    // No copying
    PersistableFactory(const PersistableFactory&) = delete;
    PersistableFactory& operator=(const PersistableFactory&) = delete;

    // No moving
    PersistableFactory(PersistableFactory&&) = delete;
    PersistableFactory& operator=(PersistableFactory&&) = delete;
};
}  // namespace io
}  // namespace table
}  // namespace afw
}  // namespace lsst

#endif  // !AFW_TABLE_IO_Persistable_h_INCLUDED
