// -*- lsst-c++ -*-
#ifndef AFW_TABLE_DETAIL_Access_h_INCLUDED
#define AFW_TABLE_DETAIL_Access_h_INCLUDED

#include <cstring>

#include "ndarray/Manager.h"
#include "lsst/afw/table/FieldBase.h"
#include "lsst/afw/table/Schema.h"
#include "lsst/afw/table/detail/SchemaImpl.h"

namespace lsst {
namespace afw {
namespace table {

class BaseRecord;
class BaseTable;

namespace detail {

/**
 *  @internal
 *
 *  Friendship-aggregation class for afw/table.
 *
 *  Access is a collection of static member functions that provide access to internals of other
 *  classes.  It allows many classes to just declare Access as a friend rather than a long list of
 *  related classes.  This is less secure, but it's obviously not part of the public interface,
 *  and that's good enough.
 */
class Access final {
public:
    /// @internal Return a sub-field key corresponding to the nth element.
    template <typename T>
    static Key<typename Key<T>::Element> extractElement(KeyBase<T> const &kb, std::size_t n) {
        if (!static_cast<Key<T> const &>(kb).isValid()) {
            throw LSST_EXCEPT(
                    pex::exceptions::LogicError,
                    (boost::format(
                             "Cannot extract subfield key from invalid key of type '%s' "
                             "(most often this is caused by failing to setup centroid or shape slots)") %
                     Key<T>::getTypeString())
                            .str());
        }
        return Key<typename Key<T>::Element>(static_cast<Key<T> const &>(kb).getOffset() +
                                             n * sizeof(typename Key<T>::Element));
    }

    /// @internal Return a sub-field key corresponding to a range
    template <typename T>
    static Key<Array<T> > extractRange(KeyBase<Array<T> > const &kb, std::size_t begin, std::size_t end) {
        if (!static_cast<Key<Array<T> > const &>(kb).isValid()) {
            throw LSST_EXCEPT(pex::exceptions::LogicError,
                              (boost::format("Cannot extract subfield key from invalid key of type '%s' ") %
                               Key<T>::getTypeString())
                                      .str());
        }
        return Key<Array<T> >(static_cast<Key<Array<T> > const &>(kb).getOffset() +
                                      begin * sizeof(typename Key<T>::Element),
                              end - begin);
    }

    /// @internal Access to the private Key constructor.
    template <typename T>
    static Key<T> makeKey(std::size_t offset) {
        return Key<T>(offset);
    }

    /// @internal Access to the private Key constructor.
    template <typename T>
    static void makeKey(Key<T> *key, std::size_t offset) {
        new (key) Key<T>(offset);
    }

    /// @internal Access to the private Key constructor.
    template <typename T>
    static Key<T> makeKey(Field<T> const &field, std::size_t offset) {
        return Key<T>(offset, field);
    }

    /// @internal Access to the private Key constructor.
    template <typename T>
    static Key<T> makeKey(Key<T> *key, Field<T> const &field, std::size_t offset) {
        new (key) Key<T>(offset, field);
    }

    /// @internal Access to the private Key constructor.
    static Key<Flag> makeKey(std::size_t offset, std::size_t bit) { return Key<Flag>(offset, bit); }

    /// @internal Access to the private Key constructor.
    static void makeKey(Key<Flag> *key, std::size_t offset, std::size_t bit) { new (key) Key<Flag>(offset, bit); }

    /// @internal Access to the private Key constructor.
    static Key<std::string> makeKeyString(std::size_t offset, std::size_t size) { return Key<std::string>(offset, size); }
    static void makeKeyString(Key<std::string> *key,std::size_t offset, std::size_t size) {new (key) Key<std::string>(offset, size); }

    /// @internal Access to the private Key constructor.
    template <typename T>
    static Key<Array<T>> makeKeyArray(std::size_t offset, std::size_t size) {
        return Key<Array<T>>(offset, size);
    }

    template <typename T>
    static void makeKeyArray(Key<Array<T>> *array, std::size_t offset, std::size_t size) {
        new (array) Key<Array<T>>(offset, size);
    }

    /// @internal Add some padding to a schema without adding a field.
    static void padSchema(Schema &schema, std::size_t bytes) {
        schema._edit();
        schema._impl->_recordSize += bytes;
    }
};
}  // namespace detail
}  // namespace table
}  // namespace afw
}  // namespace lsst

#endif  // !AFW_TABLE_DETAIL_Access_h_INCLUDED
