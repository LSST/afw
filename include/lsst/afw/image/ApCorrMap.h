// -*- LSST-C++ -*-
/*
 * LSST Data Management System
 * See the COPYRIGHT and LICENSE files in the top-level directory of this
 * package for notices and licensing terms.
 */

#ifndef LSST_AFW_IMAGE_ApCorrMap_h_INCLUDED
#define LSST_AFW_IMAGE_ApCorrMap_h_INCLUDED

#include <string>
#include <map>

#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/math/BoundedField.h"

namespace lsst { namespace afw { namespace image {

/**
 *  @brief A thin wrapper around std::map to allow aperture corrections to be attached to Exposures.
 *
 *  ApCorrMap simply adds error handling accessors, persistence, and a bit of encapsulation to std::map
 *  (given the simplified interface, for instance, we could switch to unordered_map or some other
 *  underyling container in the future).
 */
class ApCorrMap : public table::io::PersistableFacade<ApCorrMap>, public table::io::Persistable {
    typedef std::map<std::string,PTR(math::BoundedField)> Internal;
public:

    /// Maximum number of characters for an aperture correction name (required for persistence).
    static std::size_t const MAX_NAME_LENGTH = 64;

    /// Iterator type returned by begin() and end().  Dereferences to a pair<string,PTR(BoundedField)>.
    typedef Internal::const_iterator Iterator;

    Iterator begin() const { return _internal.begin(); }
    Iterator end() const { return _internal.end(); }

    std::size_t size() const { return _internal.size(); }

    /// Return the field with the given name, throwing NotFoundError when the name is not present.
    PTR(math::BoundedField) const operator[](std::string const & name) const;

    /// Return the field with the given name, returning an empty pointer when the name is not present.
    PTR(math::BoundedField) const get(std::string const & name) const;

    /// Add or replace an aperture correction.
    void set(std::string const & name, PTR(math::BoundedField) field);

    /// Whether the map is persistable (true IFF all contained BoundedFields are persistable).
    virtual bool isPersistable() const;

    /// Scale all fields by a constant
    void operator*=(double const scale);
    void operator/=(double const scale) { *this *= 1.0/scale; }

private:

    virtual std::string getPersistenceName() const;

    virtual std::string getPythonModule() const;

    virtual void write(OutputArchiveHandle & handle) const;

    Internal _internal;
};

}}} // lsst::afw::image

#endif // !LSST_AFW_IMAGE_ApCorrMap_h_INCLUDED
