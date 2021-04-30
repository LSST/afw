// -*- lsst-c++ -*-
#ifndef AFW_TABLE_Key_h_INCLUDED
#define AFW_TABLE_Key_h_INCLUDED

#include <iostream>

#include "lsst/utils/hashCombine.h"

#include "lsst/afw/table/FieldBase.h"
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
        return utils::hashCombine(17, _offset, this->getElementCount());
    }

    /// Return the offset (in bytes) of this field within a record.
    int getOffset() const noexcept { return _offset; }

    /**
     *  Return true if the key was initialized to valid offset.
     *
     *  This does not guarantee that a key is valid with any particular schema, or even
     *  that any schemas still exist in which this key is valid.
     *
     *  A key that is default constructed will always be invalid.
     */
    bool isValid() const noexcept { return _offset >= 0; }

    /**
     *  Default construct a field.
     *
     *  The new field will be invalid until a valid Key is assigned to it.
     */
    Key() noexcept : FieldBase<T>(FieldBase<T>::makeDefault()), _offset(-1) {}

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

    explicit Key(int offset, FieldBase<T> const& fb = FieldBase<T>()) noexcept
            : FieldBase<T>(fb), _offset(offset) {}

    int _offset;
};

/**
 *  Key specialization for Flag.
 *
 *  Flag fields are special; their keys need to contain not only the offset to the
 *  integer element they share with other Flag fields, but also their position
 *  in that shared field.
 *
 *  Flag fields operate mostly like a bool field, but they do not support reference
 *  access, and internally they are packed into an integer shared by multiple fields
 *  so the marginal cost of each Flag field is only one bit.
 */
template <>
class Key<Flag> : public KeyBase<Flag>, public FieldBase<Flag> {
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
    bool operator==(Key<OtherT> const &other) const {
        return false;
    }
    template <typename OtherT>
    bool operator!=(Key<OtherT> const &other) const {
        return true;
    }

    bool operator==(Key const &other) const { return _offset == other._offset && _bit == other._bit; }
    bool operator!=(Key const &other) const { return !this->operator==(other); }
    //@}

    /// Return a hash of this object.
    std::size_t hash_value() const noexcept {
        // Completely arbitrary seed
        return utils::hashCombine(17, _offset, _bit);
    }

    /// Return the offset in bytes of the integer element that holds this field's bit.
    int getOffset() const { return _offset; }

    /// The index of this field's bit within the integer it shares with other Flag fields.
    int getBit() const { return _bit; }

    /**
     *  Return true if the key was initialized to valid offset.
     *
     *  This does not guarantee that a key is valid with any particular schema, or even
     *  that any schemas still exist in which this key is valid.
     *
     *  A key that is default constructed will always be invalid.
     */
    bool isValid() const { return _offset >= 0; }

    /**
     *  Default construct a field.
     *
     *  The new field will be invalid until a valid Key is assigned to it.
     */
    Key() : FieldBase<Flag>(), _offset(-1), _bit(0) {}

    Key(Key const &) = default;
    Key(Key &&) = default;
    Key &operator=(Key const &) = default;
    Key &operator=(Key &&) = default;
    ~Key() = default;

    /// Stringification.
    inline friend std::ostream &operator<<(std::ostream &os, Key<Flag> const &key) {
        return os << "Key['" << Key<Flag>::getTypeString() << "'](offset=" << key.getOffset()
                  << ", bit=" << key.getBit() << ")";
    }

private:
    friend class detail::Access;
    friend class BaseRecord;

    /// Used to implement BaseRecord::get.
    Value getValue(Element const *p, ndarray::Manager::Ptr const &) const {
        return (*p) & (Element(1) << _bit);
    }

    /// Used to implement BaseRecord::set.
    void setValue(Element *p, ndarray::Manager::Ptr const &, Value v) const {
        if (v) {
            *p |= (Element(1) << _bit);
        } else {
            *p &= ~(Element(1) << _bit);
        }
    }

    explicit Key(int offset, int bit) : _offset(offset), _bit(bit) {}

    int _offset;
    int _bit;
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

template <>
struct hash<lsst::afw::table::Key<lsst::afw::table::Flag>> {
    using argument_type = lsst::afw::table::Key<lsst::afw::table::Flag>;
    using result_type = size_t;
    size_t operator()(argument_type const &obj) const noexcept { return obj.hash_value(); }
};

}  // namespace std

#endif  // !AFW_TABLE_Key_h_INCLUDED
