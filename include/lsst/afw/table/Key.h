// -*- lsst-c++ -*-
#ifndef AFW_TABLE_Key_h_INCLUDED
#define AFW_TABLE_Key_h_INCLUDED

#include "lsst/cpputils/hashCombine.h"

#include "lsst/afw/table/FieldBase.h"
#include "lsst/afw/table/Flag.h"
#include "lsst/afw/table/KeyBase.h"

namespace lsst {
namespace afw {
namespace table {

namespace detail {

class Access;

}  // namespace detail

/**
 *  A class used as a handle to a particular field in a table.
 *
 *  All access to table data ultimately goes through Key objects, which
 *  know (via an internal offset) how to address and cast the internal
 *  data buffer of a record or table.
 *
 *  Keys can be obtained from a Schema by name:
 *
 *      schema.find("myfield").key
 *
 *  and are also returned when a new field is added.  Compound and array keys also provide
 *  accessors to retrieve scalar keys to their elements (see the
 *  documentation for the KeyBase specializations), even though these
 *  element keys do not correspond to a field that exists in any Schema.
 *  For example:
 *
 *      Schema schema;
 *      Key< Array<float> > arrayKey = schema.addField< Array<float> >("array", "docs for array", 5);
 *      Key< Point<int> > pointKey = schema.addField< Point<int> >("point", "docs for point");
 *      Key<float> elementKey = arrayKey[3];
 *      Key<int> xKey = pointKey.getX();
 *      std::shared_ptr<BaseTable> table = BaseTable::make(schema);
 *      std::shared_ptr<BaseRecord> record = table.makeRecord();
 *      assert(&record[arrayKey][3] == &record[elementKey3]);
 *      assert(record.get(pointKey).getX() == record[xKey]);
 *
 *  Key inherits from FieldBase to allow a key for a dynamically-sized field
 *  to know its size without needing to specialize Key itself or hold a full
 *  Field object.
 */
template <typename T>
class Key : public KeyBase<T>, public FieldBase<T> {
public:
    //@{
    /**
     *  Equality comparison.
     *
     *  Two keys with different types are never equal.  Keys with the same type
     *  are equal if they point to the same location in a table, regardless of
     *  what Schema they were constructed from (for instance, if a field has a
     *  different name in one Schema than another, but is otherwise the same,
     *  the two keys will be equal).
     */
    template <typename OtherT>
    bool operator==(Key<OtherT> const& other) const noexcept {
        return false;
    }
    template <typename OtherT>
    bool operator!=(Key<OtherT> const& other) const noexcept {
        return true;
    }

    bool operator==(Key const& other) const noexcept {
        return _offset == other._offset && this->getElementCount() == other.getElementCount();
    }
    bool operator!=(Key const& other) const noexcept { return !this->operator==(other); }
    //@}

    /// Return a hash of this object.
    std::size_t hash_value() const noexcept {
        // Completely arbitrary seed
        return cpputils::hashCombine(17, _offset, this->getElementCount());
    }

    /// Return the offset (in bytes) of this field within a record.
    std::size_t getOffset() const noexcept { return _offset; }

    /**
     *  Return true if the key was initialized to valid offset.
     *
     *  This does not guarantee that a key is valid with any particular schema, or even
     *  that any schemas still exist in which this key is valid.
     *
     *  A key that is default constructed will always be invalid.
     */
    bool isValid() const noexcept { return _valid; }

    /**
     *  Default construct a field.
     *
     *  The new field will be invalid until a valid Key is assigned to it.
     */
    Key() noexcept : FieldBase<T>(FieldBase<T>::makeDefault()), _offset(0), _valid(false) {}

    Key(Key const&) noexcept = default;
    Key(Key&&) noexcept = default;
    Key& operator=(Key const&) noexcept = default;
    Key& operator=(Key&&) noexcept = default;
    ~Key() noexcept = default;

    /// Stringification.
    inline friend std::ostream& operator<<(std::ostream& os, Key<T> const& key) {
        return os << "Key<" << Key<T>::getTypeString() << ">(offset=" << key.getOffset()
                  << ", nElements=" << key.getElementCount() << ")";
    }

private:
    friend class detail::Access;
    friend class BaseRecord;

    explicit Key(std::size_t offset, FieldBase<T> const& fb = FieldBase<T>()) noexcept
            : FieldBase<T>(fb), _offset(offset), _valid(true) {}

    std::size_t _offset;
    bool _valid;
};
}  // namespace table
}  // namespace afw
}  // namespace lsst

namespace std {
template <typename T>
struct hash<lsst::afw::table::Key<T>> {
    using argument_type = lsst::afw::table::Key<T>;
    using result_type = size_t;
    size_t operator()(argument_type const& obj) const noexcept { return obj.hash_value(); }
};
}  // namespace std

#endif  // !AFW_TABLE_Key_h_INCLUDED
