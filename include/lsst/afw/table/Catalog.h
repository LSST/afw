// -*- lsst-c++ -*-
#ifndef AFW_TABLE_Catalog_h_INCLUDED
#define AFW_TABLE_Catalog_h_INCLUDED

#include <type_traits>
#include <vector>

#include "boost/iterator/iterator_adaptor.hpp"
#include "boost/iterator/transform_iterator.hpp"

#include "lsst/base.h"
#include "lsst/pex/exceptions.h"
#include "lsst/afw/fitsDefaults.h"
#include "lsst/afw/table/fwd.h"
#include "lsst/afw/table/io/FitsWriter.h"
#include "lsst/afw/table/io/FitsReader.h"
#include "lsst/afw/table/SchemaMapper.h"
#include "lsst/log/Log.h"

namespace lsst {
namespace afw {
namespace table {

/**
 *  Iterator class for CatalogT.
 *
 *  Iterators dereference to record references or const references, even though the CatalogT container
 *  is based on a vector of shared_ptr internally.  This is usually very convenient (and is one of
 *  the reasons for having a custom container class in the first place).
 *
 *  Sometimes, however, we'd like to get a shared_ptr from an iterator (especially because records
 *  are noncopyable).  With that in mind, CatalogIterator is implicitly convertible to the shared_ptr
 *  type it holds internally, and can also be assigned a shared_ptr to set the pointer in the
 *  underlying container.  This conversion makes sense from the perspective that both iterators
 *  and smart pointers mimic the interface of pointers and provide the same interface to get at
 *  the underlying record.
 */
template <typename BaseT>
class CatalogIterator : public boost::iterator_adaptor<CatalogIterator<BaseT>, BaseT,
                                                       typename BaseT::value_type::element_type> {
public:
    CatalogIterator() {}

    template <typename OtherBaseT>
    CatalogIterator(CatalogIterator<OtherBaseT> const& other)
            : CatalogIterator::iterator_adaptor_(other.base()) {}

    explicit CatalogIterator(BaseT const& base) : CatalogIterator::iterator_adaptor_(base) {}

    template <typename RecordT>
    operator std::shared_ptr<RecordT>() const {
        return *this->base();
    }

    template <typename RecordT>
    CatalogIterator& operator=(std::shared_ptr<RecordT> const& other) const {
        *this->base() = other;
        return *this;
    }

private:
    friend class boost::iterator_core_access;
    typename BaseT::value_type::element_type& dereference() const { return **this->base(); }
};

/**
 *  A custom container class for records, based on std::vector.
 *
 *  CatalogT wraps a std::vector<std::shared_ptr<RecordT>> in an interface that looks more
 *  like a std::vector<RecordT>; its iterators and accessors return references
 *  or const references, rather than pointers, making them easier to use.  It
 *  also holds a table, which is used to allocate new records and determine the
 *  schema, but no checking is done to ensure that records added to the catalog
 *  use the same table or indeed have the same schema.
 *
 *  Because a CatalogT is holds shared_ptrs internally, many of its operations
 *  can be either shallow or deep, with new deep copies allocated by the
 *  catalog's table object.  New records can be also be inserted by pointer
 *  (shallow) or by value (deep).
 *
 *  The constness of records is determined by the constness of the first
 *  template parameter to CatalogT; a container instance is always either const
 *  or non-const in that respect (like smart pointers).  Also like smart
 *  pointers, const member functions (and by extension, const_iterators) do not
 *  allow the underlying pointers to be changed, while non-const member
 *  functions and iterators do.
 *
 *  CatalogT does not permit empty (null) pointers as elements.  As a result,
 *  CatalogT has no resize member function.
 *
 *  CatalogT has a very different interface in Python; it mimics Python's list
 *  instead of C++'s std::vector.  It is also considerably simpler, because it
 *  doesn't need to deal with iterator ranges or the distinction between
 *  references and shared_ptrs to records.  See the Python docstring for more
 *  information.
 */
template <typename RecordT>
class CatalogT {
    using Internal = std::vector<std::shared_ptr<RecordT>>;

public:
    using Record = RecordT;
    using Table = typename Record::Table;
    using ColumnView = typename Record::ColumnView;

    using value_type = RecordT;
    using reference = RecordT &;
    using pointer = std::shared_ptr<RecordT>;
    using size_type = typename Internal::size_type;
    using difference_type = typename Internal::difference_type;
    using iterator = CatalogIterator<typename Internal::iterator>;
    using const_iterator = CatalogIterator<typename Internal::const_iterator>;

    /// Return the table associated with the catalog.
    std::shared_ptr<Table> getTable() const { return _table; }

    /// Return the schema associated with the catalog's table.
    Schema getSchema() const { return _table->getSchema(); }

    /**
     *  Construct a catalog from a table (or nothing).
     *
     * A table is always initialized with a minimal schema.
     */
    explicit CatalogT(std::shared_ptr<Table> const& table = std::shared_ptr<Table>())
            : _table(table), _internal() {
        if(!_table) {
            auto schema=Table::makeMinimalSchema();
            _table=Table::make(schema);
        }
    }

    /// Construct a catalog from a schema, creating a table with Table::make(schema).
    explicit CatalogT(Schema const& schema) : _table(Table::make(schema)), _internal() {}

    /**
     *  Construct a catalog from a table and an iterator range.
     *
     *  If deep is true, new records will be created using table->copyRecord before being inserted.
     *  If deep is false, records will be not be copied, but they must already be associated with
     *  the given table.  The table itself is never deep-copied.
     *
     *  The iterator must dereference to a record reference or const reference rather than a pointer,
     *  but should be implicitly convertible to a record pointer as well (see CatalogIterator).
     */
    template <typename InputIterator>
    CatalogT(std::shared_ptr<Table> const& table, InputIterator first, InputIterator last, bool deep = false)
            : _table(table), _internal() {
        insert(end(), first, last, deep);
    }

    //@{
    /// Shallow copy and constructors.
    CatalogT(CatalogT const& other) = default;
    CatalogT(CatalogT&& other) noexcept = default;
    //@}

    ~CatalogT() = default;

    /**
     *  Shallow copy constructor from a container containing a related record type.
     *
     *  This conversion only succeeds if OtherRecordT is convertible to RecordT and OtherTable is
     *  convertible to Table.
     */
    template <typename OtherRecordT>
    CatalogT(CatalogT<OtherRecordT> const& other)
            : _table(other.getTable()), _internal(other.begin().base(), other.end().base()) {}

    /// Shallow assigment.
    CatalogT& operator=(CatalogT const& other) {
        if (&other != this) {
            _table = other._table;
            _internal = other._internal;
        }
        return *this;
    }
    // Delegate to copy assignment for backward compatibility
    CatalogT& operator=(CatalogT&& other) { return *this = other; }

    /**
     *  Return the subset of a catalog corresponding to the True values of the given mask array.
     *
     *  The returned array's records are shallow copies, and hence will not in general be contiguous.
     */
    CatalogT subset(ndarray::Array<bool const, 1> const& mask) const {
        if (size_type(mask.size()) != size()) {
            throw LSST_EXCEPT(
                    pex::exceptions::LengthError,
                    (boost::format("Mask array with %d elements applied to catalog with %d elements") %
                     mask.size() % size())
                            .str());
        }
        CatalogT<RecordT> result(getTable());
        ndarray::Array<bool const, 1>::Iterator maskIter = mask.begin();
        const_iterator catIter = begin();
        for (; maskIter != mask.end(); ++maskIter, ++catIter) {
            if (*maskIter) result.push_back(catIter);
        }
        return result;
    }

    /**
     * @brief Returns a shallow copy of a subset of this Catalog.  The arguments
     * correspond to python's slice() syntax.
     */
    CatalogT subset(std::ptrdiff_t startd, std::ptrdiff_t stopd, std::ptrdiff_t step) const {
        /* Python's slicing syntax is weird and wonderful.

         Both the "start" and "stop" indices can be negative, which means the
         abs() of the index less than the size; [-1] means the last item.
         Moreover, it's possible to have a negative index less than -len(); it
         will get clipped.  That is in fact one way to slice *backward* through
         the array *and* include element 0;

         >>> range(10)[5:-20:-1]
         [5, 4, 3, 2, 1, 0]

         The clipping tests in this function look more complicated than they
         need to be, but that is partly because there are some weird edge cases.

         Also, ptrdiff_t vs size_t introduces some annoying complexity.  Note
         that the args are "startd"/"stopd" (not "start"/"stop").

         There is a fairly complete set of tests in tests/ticket2026.py; if you
         try to simplify this function, be sure they continue to pass.
         */
        size_type S = size();
        size_type start, stop = 0;
        // Python doesn't allow step == 0
        if (step == 0) {
            throw LSST_EXCEPT(pex::exceptions::InvalidParameterError, "Step cannot be zero");
        }
        // Basic negative indexing rule: first add size
        if (startd < 0) {
            startd += S;
        }
        if (stopd < 0) {
            stopd += S;
        }
        // Start gets clipped to zero; stop does not (yet).
        if (startd < 0) {
            startd = 0;
        }
        // Now start is non-negative, so can cast to size_t.
        start = (size_type)startd;
        if (start > S) {
            start = S;
        }
        if (step > 0) {
            // When stepping forward, stop gets clipped at zero,
            // so is non-negative and can get cast to size_t.
            if (stopd < 0) {
                stopd = 0;
            }
            stop = (size_type)stopd;
            if (stop > S) {
                stop = S;
            }
        } else if (step < 0) {
            // When stepping backward, stop gets clipped at -1 so that slices
            // including 0 are possible.
            if (stopd < 0) {
                stopd = -1;
            }
        }

        if (((step > 0) && (start >= stop)) || ((step < 0) && ((std::ptrdiff_t)start <= stopd))) {
            // Empty range
            return CatalogT<RecordT>(getTable(), begin(), begin());
        }

        if (step == 1) {
            // Use the iterator-based constructor for this simple case
            assert(start >= 0);
            assert(stop > 0);
            assert(start < S);
            assert(stop <= S);
            return CatalogT<RecordT>(getTable(), begin() + start, begin() + stop);
        }

        // Build a new CatalogT and copy records into it.
        CatalogT<RecordT> cat(getTable());
        size_type N = 0;
        if (step >= 0) {
            N = (stop - start) / step + (((stop - start) % step) ? 1 : 0);
        } else {
            N = (size_t)((stopd - (std::ptrdiff_t)start) / step +
                         (((stopd - (std::ptrdiff_t)start) % step) ? 1 : 0));
        }
        cat.reserve(N);
        if (step >= 0) {
            for (size_type i = start; i < stop; i += step) {
                cat.push_back(get(i));
            }
        } else {
            for (std::ptrdiff_t i = (std::ptrdiff_t)start; i > stopd; i += step) {
                cat.push_back(get(i));
            }
        }
        return cat;
    }

    /**
     *  Write a FITS binary table to a regular file.
     *
     *  @param[in] filename    Name of the file to write.
     *  @param[in] mode        "a" to append a new HDU, "w" to overwrite any existing file.
     *  @param[in] flags       Table-subclass-dependent bitflags that control the details of how to
     *                         read the catalogs.  See e.g. SourceFitsFlags.
     */
    void writeFits(std::string const& filename, std::string const& mode = "w", int flags = 0) const {
        io::FitsWriter::apply(filename, mode, *this, flags);
    }

    /**
     *  Write a FITS binary table to a RAM file.
     *
     *  @param[in,out] manager Object that manages the memory to write to.
     *  @param[in] mode        "a" to append a new HDU, "w" to overwrite any existing file.
     *  @param[in] flags       Table-subclass-dependent bitflags that control the details of how to
     *                         read the catalogs.  See e.g. SourceFitsFlags.
     */
    void writeFits(fits::MemFileManager& manager, std::string const& mode = "w", int flags = 0) const {
        io::FitsWriter::apply(manager, mode, *this, flags);
    }

    /**
     *  Write a FITS binary table to an open file object.
     *
     *  @param[in,out] fitsfile Fits file object to write to.
     *  @param[in] flags        Table-subclass-dependent bitflags that control the details of how to
     *                          read the catalogs.  See e.g. SourceFitsFlags.
     */
    void writeFits(fits::Fits& fitsfile, int flags = 0) const {
        io::FitsWriter::apply(fitsfile, *this, flags);
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
    static CatalogT readFits(std::string const& filename, int hdu = fits::DEFAULT_HDU, int flags = 0) {
        return io::FitsReader::apply<CatalogT>(filename, hdu, flags);
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
    static CatalogT readFits(fits::MemFileManager& manager, int hdu = fits::DEFAULT_HDU, int flags = 0) {
        return io::FitsReader::apply<CatalogT>(manager, hdu, flags);
    }

    /**
     *  Read a FITS binary table from a file object already at the correct extension.
     *
     *  @param[in] fitsfile    Fits file object to read from.
     *  @param[in] flags       Table-subclass-dependent bitflags that control the details of how to read
     *                         the catalog.  See e.g. SourceFitsFlags.
     */
    static CatalogT readFits(fits::Fits& fitsfile, int flags = 0) {
        return io::FitsReader::apply<CatalogT>(fitsfile, flags);
    }

    /**
     *  Return a ColumnView of this catalog's records.
     *
     *  Will throw RuntimeError if records are not contiguous.
     */
    ColumnView getColumnView() const {
        if (std::is_const<RecordT>::value) {
            throw LSST_EXCEPT(
                    pex::exceptions::LogicError,
                    "Cannot get a column view from a CatalogT<RecordT const> (as column views are always "
                    "non-const views).");
        }
        return ColumnView::make(_table, begin(), end());
    }

    /// Return true if all records are contiguous.
    bool isContiguous() const { return ColumnView::isRangeContiguous(_table, begin(), end()); }

    //@{
    /**
     *  Iterator access.
     *
     *  @see CatalogIterator
     */
    iterator begin() { return iterator(_internal.begin()); }
    iterator end() { return iterator(_internal.end()); }
    const_iterator begin() const { return const_iterator(_internal.begin()); }
    const_iterator end() const { return const_iterator(_internal.end()); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
    //@}

    /// Return true if the catalog has no records.
    bool empty() const { return _internal.empty(); }

    /// Return the number of elements in the catalog.
    size_type size() const { return _internal.size(); }

    /// Return the maximum number of elements allowed in a catalog.
    size_type max_size() const { return _internal.max_size(); }

    /**
     *  Return the capacity of the catalog.
     *
     *  This is computed as the sum of the current size and the unallocated space in the table.  It
     *  does not reflect the size of the internal vector, and hence cannot be used to judge when
     *  iterators may be invalidated.
     */
    size_type capacity() const { return _internal.size() + _table->getBufferSize(); }

    /**
     *  Increase the capacity of the catalog to the given size.
     *
     *  This can be used to guarantee that the catalog will be contiguous, but it only affects
     *  records constructed after reserve().
     */
    void reserve(size_type n) {
        if (n <= _internal.size()) return;
        _table->preallocate(n - _internal.size());
        _internal.reserve(n);
    }

    /// Change the size of the catalog, removing or adding empty records as needed.
    void resize(size_type n) {
        size_type old = size();
        _internal.resize(n);
        if (old < n) {
            _table->preallocate(n - old);
            for (size_type i = old; i != n; ++i) {
                _internal[i] = _table->makeRecord();
            }
        }
    }

    /// Return the record at index i.
    reference operator[](size_type i) const { return *_internal[i]; }

    /// Return the record at index i (throws std::out_of_range).
    reference at(size_type i) const { return *_internal.at(i); }

    /// Return the first record.
    reference front() const { return *_internal.front(); }

    /// Return the last record.
    reference back() const { return *_internal.back(); }

    /// Return a pointer to the record at index i.
    std::shared_ptr<RecordT> const get(size_type i) const { return _internal[i]; }

    /// Set the record at index i to a pointer.
    void set(size_type i, std::shared_ptr<RecordT> const& p) { _internal[i] = p; }

    /**
     *  Replace the contents of the table with an iterator range.
     *
     *  Delegates to insert(); look there for more information.
     */
    template <typename InputIterator>
    void assign(InputIterator first, InputIterator last, bool deep = false) {
        clear();
        insert(end(), first, last, deep);
    }

    /// Add a copy of the given record to the end of the catalog.
    void push_back(Record const& r) {
        std::shared_ptr<RecordT> p = _table->copyRecord(r);
        _internal.push_back(p);
    }

    /// Add the given record to the end of the catalog without copying.
    void push_back(std::shared_ptr<RecordT> const& p) { _internal.push_back(p); }

    /// Create a new record, add it to the end of the catalog, and return a pointer to it.
    std::shared_ptr<RecordT> addNew() {
        std::shared_ptr<RecordT> r = _table->makeRecord();
        _internal.push_back(r);
        return r;
    }

    /// Remove the last record in the catalog
    void pop_back() { _internal.pop_back(); }

    /// Deep-copy the catalog using a cloned table.
    CatalogT copy() const { return CatalogT(getTable()->clone(), begin(), end(), true); }

    /**
     *  Insert an iterator range into the table.
     *
     *  InputIterator must dereference to a record reference that is convertible to the record type
     *  held by the catalog, and must be implicitly convertible to a shared_ptr to a record.
     *
     *  If deep is true, new records will be created by calling copyRecord on the catalog's table.
     *  If deep is false, the new records will not be copied, but they must have been created
     *  with the catalog's table (note that a table may be shared by multiple catalogs).
     *
     *  If InputIterator models RandomAccessIterator (according to std::iterator_traits) and deep
     *  is true, table->preallocate will be used to ensure that the resulting records are
     *  contiguous in memory and can be used with ColumnView.  To ensure this is the case for
     *  other iterator types, the user must preallocate the table manually.
     */
    template <typename InputIterator>
    void insert(iterator pos, InputIterator first, InputIterator last, bool deep = false) {
        _maybeReserve(pos, first, last, deep,
                      (typename std::iterator_traits<InputIterator>::iterator_category*)nullptr);
        if (deep) {
            while (first != last) {
                pos = insert(pos, *first);
                ++pos;
                ++first;
            }
        } else {
            while (first != last) {
                pos = insert(pos, first);
                assert(pos != end());
                ++pos;
                ++first;
            }
        }
    }

    /// Insert a range of records into the catalog by copying them with a SchemaMapper.
    template <typename InputIterator>
    void insert(SchemaMapper const& mapper, iterator pos, InputIterator first, InputIterator last) {
        if (!_table->getSchema().contains(mapper.getOutputSchema())) {
            throw LSST_EXCEPT(pex::exceptions::InvalidParameterError,
                              "SchemaMapper's output schema does not match catalog's schema");
        }
        _maybeReserve(pos, first, last, true,
                      (typename std::iterator_traits<InputIterator>::iterator_category*)nullptr);
        while (first != last) {
            pos = insert(pos, _table->copyRecord(*first, mapper));
            ++pos;
            ++first;
        }
    }

    /// Insert a copy of the given record at the given position.
    iterator insert(iterator pos, Record const& r) {
        std::shared_ptr<RecordT> p = _table->copyRecord(r);
        return iterator(_internal.insert(pos.base(), p));
    }

    /// Insert the given record at the given position without copying.
    iterator insert(iterator pos, std::shared_ptr<RecordT> const& p) {
        return iterator(_internal.insert(pos.base(), p));
    }

    /// Erase the record pointed to by pos, and return an iterator the next record.
    iterator erase(iterator pos) { return iterator(_internal.erase(pos.base())); }

    /// Erase the records in the range [first, last).
    iterator erase(iterator first, iterator last) {
        return iterator(_internal.erase(first.base(), last.base()));
    }

    /// Shallow swap of two catalogs.
    void swap(CatalogT& other) noexcept {
        _table.swap(other._table);
        _internal.swap(other._internal);
    }

    /// Remove all records from the catalog.
    void clear() { _internal.clear(); }

    /// Return true if the catalog is in ascending order according to the given key.
    template <typename T>
    bool isSorted(Key<T> const& key) const;

    /**
     *  Return true if the catalog is in ascending order according to the given predicate.
     *
     *  cmp(a, b) should return true if record a is less than record b, and false otherwise.
     */
    template <typename Compare>
    bool isSorted(Compare cmp) const;

    /// Sort the catalog in-place by the field with the given key.
    template <typename T>
    void sort(Key<T> const& key);

    /**
     *  Sort the catalog in-place by the field with the given predicate.
     *
     *  cmp(a, b) should return true if record a is less than record b, and false otherwise.
     */
    template <typename Compare>
    void sort(Compare cmp);

    //@{
    /**
     *  Return an iterator to the record with the given value.
     *
     *  When the field being searched is not unique, which matching record will be returned
     *  is not defined.  In these cases, lower_bound, upper_bound, or equal_range should be
     *  used instead.
     *
     *  In Python, this method returns a Record, not an iterator.
     *
     *  @note The catalog must be sorted in ascending order according to the given key
     *        before calling find (i.e. isSorted(key) must be true) for maximal efficiency.
     *        If the value searched for is not found, it assumes that the catalog is unsorted
     *        and performs a brute-force search instead of failing immediately.
     *
     *  Returns end() if the Record cannot be found.
     */
    template <typename T>
    iterator find(typename Field<T>::Value const& value, Key<T> const& key);

    template <typename T>
    const_iterator find(typename Field<T>::Value const& value, Key<T> const& key) const;
    //@}

    //@{
    /**
     *  Performed binary searches on sorted fields.
     *
     *  These methods perform binary searches analogous to the STL algorithms of the same
     *  name; they simply create a comparison functor using the given value and Key.
     *
     *  In Python, the lower_bound and upper_bound methods return the position of the
     *  result record in the catalog, and equal_range returns a Python slice object
     *  that defines the range.
     *
     *  @note The catalog must be sorted in ascending order according to the given key
     *        before calling any of the search methods (i.e. isSorted(key) must be true).
     */
    template <typename T>
    iterator lower_bound(typename Field<T>::Value const& value, Key<T> const& key);

    template <typename T>
    const_iterator lower_bound(typename Field<T>::Value const& value, Key<T> const& key) const;

    template <typename T>
    iterator upper_bound(typename Field<T>::Value const& value, Key<T> const& key);

    template <typename T>
    const_iterator upper_bound(typename Field<T>::Value const& value, Key<T> const& key) const;

    template <typename T>
    std::pair<iterator, iterator> equal_range(typename Field<T>::Value const& value, Key<T> const& key);

    template <typename T>
    std::pair<const_iterator, const_iterator> equal_range(typename Field<T>::Value const& value,
                                                          Key<T> const& key) const;
    //@}

    //@{
    /**
     *  Return a reference to the internal vector-of-shared_ptr
     *
     *  While in most cases it is more convenient to use the Catalog's iterators, which dereference
     *  directly to Record objects (and hence allow iter->method() rather than (**iter).method()),
     *  direct access to the underlying vector-of-shared_ptr is provided here to allow complete use
     *  of the C++ STL.  In particular, in order to use a mutating STL algorithm on a Catalog in
     *  such a way that Records are shallow-copied (i.e. shared_ptr::operator= is invoked instead
     *  of Record::operator=), those algorithms should be called on the iterators of these internal
     *  containers.  When an algorithm should be called in such a way that records are deep-copied,
     *  the regular Catalog iterators should be used.
     */
    Internal& getInternal() { return _internal; }
    Internal const& getInternal() const { return _internal; }
    //@}

private:
    template <typename InputIterator>
    void _maybeReserve(iterator& pos, InputIterator first, InputIterator last, bool deep,
                       std::random_access_iterator_tag*) {
        if (deep) _table->preallocate(last - first);
    }

    template <typename InputIterator>
    void _maybeReserve(iterator pos, InputIterator first, InputIterator last, bool deep,
                       std::input_iterator_tag*) {}

    std::shared_ptr<Table> _table;
    Internal _internal;
};

namespace detail {

template <typename RecordT, typename T>
struct KeyComparisonFunctor {
    bool operator()(RecordT const& a, RecordT const& b) const { return a.get(key) < b.get(key); }

    Key<T> key;
};

template <typename RecordT, typename Adaptee>
struct ComparisonAdaptor {
    bool operator()(std::shared_ptr<RecordT> const& a, std::shared_ptr<RecordT> const& b) const {
        return adaptee(*a, *b);
    }

    Adaptee adaptee;
};

template <typename RecordT, typename T>
struct KeyExtractionFunctor {
    using result_type = typename Field<T>::Value;

    result_type operator()(RecordT const& r) const { return r.get(key); }

    Key<T> key;
};

}  // namespace detail

template <typename RecordT>
template <typename Compare>
bool CatalogT<RecordT>::isSorted(Compare cmp) const {
    /// TODO: C++11 has an is_sorted function we should use when available.
    detail::ComparisonAdaptor<RecordT, Compare> f = {cmp};
    if (empty()) return true;
    const_iterator last = this->begin();
    const_iterator i = last;
    ++i;
    for (; i != this->end(); ++i) {
        if (f(i, last)) return false;
        last = i;
    }
    return true;
}

template <typename RecordT>
template <typename Compare>
void CatalogT<RecordT>::sort(Compare cmp) {
    detail::ComparisonAdaptor<RecordT, Compare> f = {cmp};
    std::stable_sort(_internal.begin(), _internal.end(), f);
}

template <typename RecordT>
template <typename T>
bool CatalogT<RecordT>::isSorted(Key<T> const& key) const {
    detail::KeyComparisonFunctor<RecordT, T> f = {key};
    return isSorted(f);
}

template <typename RecordT>
template <typename T>
void CatalogT<RecordT>::sort(Key<T> const& key) {
    detail::KeyComparisonFunctor<RecordT, T> f = {key};
    return sort(f);
}

template <typename RecordT>
template <typename T>
typename CatalogT<RecordT>::iterator CatalogT<RecordT>::find(typename Field<T>::Value const& value,
                                                             Key<T> const& key) {
    detail::KeyExtractionFunctor<RecordT, T> f = {key};
    // Iterator adaptor that makes a CatalogT iterator work like an iterator over field values.
    using SearchIter = boost::transform_iterator<detail::KeyExtractionFunctor<RecordT, T>, iterator>;
    /* Try binary search for log n search assuming the table is sorted.
     * If the search is unsuccessful, try a brute-force search before quitting.
     */
    SearchIter i = std::lower_bound(SearchIter(begin(), f), SearchIter(end(), f), value);
    if (i.base() == end() || *i != value) {
        i = std::find(SearchIter(begin(), f), SearchIter(end(), f), value);
        if (i.base() == end()) {
            return end();
        }
        LOGL_DEBUG("lsst.afw.table.Catalog", "Catalog is not sorted by the key. Finding a record may be slow.");
    }
    return i.base();
}

template <typename RecordT>
template <typename T>
typename CatalogT<RecordT>::const_iterator CatalogT<RecordT>::find(typename Field<T>::Value const& value,
                                                                   Key<T> const& key) const {
    detail::KeyExtractionFunctor<RecordT, T> f = {key};
    // Iterator adaptor that makes a CatalogT iterator work like an iterator over field values.
    using SearchIter = boost::transform_iterator<detail::KeyExtractionFunctor<RecordT, T>, const_iterator>;
    /* Try binary search for log n search assuming the table is sorted.
     * If the search is unsuccessful, try a brute-force search before quitting.
     */
    SearchIter i = std::lower_bound(SearchIter(begin(), f), SearchIter(end(), f), value);
    if (i.base() == end() || *i != value) {
        i = std::find(SearchIter(begin(), f), SearchIter(end(), f), value);
        if (i.base() == end()) {
            return end();
        }
        LOGL_DEBUG("lsst.afw.table.Catalog", "Catalog is not sorted by the key. Finding a record may be slow.");
    }
    return i.base();
}

template <typename RecordT>
template <typename T>
typename CatalogT<RecordT>::iterator CatalogT<RecordT>::lower_bound(typename Field<T>::Value const& value,
                                                                    Key<T> const& key) {
    detail::KeyExtractionFunctor<RecordT, T> f = {key};
    // Iterator adaptor that makes a CatalogT iterator work like an iterator over field values.
    using SearchIter = boost::transform_iterator<detail::KeyExtractionFunctor<RecordT, T>, iterator>;
    SearchIter i = std::lower_bound(SearchIter(begin(), f), SearchIter(end(), f), value);
    return i.base();
}

template <typename RecordT>
template <typename T>
typename CatalogT<RecordT>::const_iterator CatalogT<RecordT>::lower_bound(
        typename Field<T>::Value const& value, Key<T> const& key) const {
    detail::KeyExtractionFunctor<RecordT, T> f = {key};
    // Iterator adaptor that makes a CatalogT iterator work like an iterator over field values.
    using SearchIter = boost::transform_iterator<detail::KeyExtractionFunctor<RecordT, T>, const_iterator>;
    SearchIter i = std::lower_bound(SearchIter(begin(), f), SearchIter(end(), f), value);
    return i.base();
}

template <typename RecordT>
template <typename T>
typename CatalogT<RecordT>::iterator CatalogT<RecordT>::upper_bound(typename Field<T>::Value const& value,
                                                                    Key<T> const& key) {
    detail::KeyExtractionFunctor<RecordT, T> f = {key};
    // Iterator adaptor that makes a CatalogT iterator work like an iterator over field values.
    using SearchIter = boost::transform_iterator<detail::KeyExtractionFunctor<RecordT, T>, iterator>;
    SearchIter i = std::upper_bound(SearchIter(begin(), f), SearchIter(end(), f), value);
    return i.base();
}

template <typename RecordT>
template <typename T>
typename CatalogT<RecordT>::const_iterator CatalogT<RecordT>::upper_bound(
        typename Field<T>::Value const& value, Key<T> const& key) const {
    detail::KeyExtractionFunctor<RecordT, T> f = {key};
    // Iterator adaptor that makes a CatalogT iterator work like an iterator over field values.
    using SearchIter = boost::transform_iterator<detail::KeyExtractionFunctor<RecordT, T>, const_iterator>;
    SearchIter i = std::upper_bound(SearchIter(begin(), f), SearchIter(end(), f), value);
    return i.base();
}

template <typename RecordT>
template <typename T>
std::pair<typename CatalogT<RecordT>::iterator, typename CatalogT<RecordT>::iterator>
CatalogT<RecordT>::equal_range(typename Field<T>::Value const& value, Key<T> const& key) {
    detail::KeyExtractionFunctor<RecordT, T> f = {key};
    // Iterator adaptor that makes a CatalogT iterator work like an iterator over field values.
    using SearchIter = boost::transform_iterator<detail::KeyExtractionFunctor<RecordT, T>, iterator>;
    std::pair<SearchIter, SearchIter> i =
            std::equal_range(SearchIter(begin(), f), SearchIter(end(), f), value);
    return std::make_pair(i.first.base(), i.second.base());
}

template <typename RecordT>
template <typename T>
std::pair<typename CatalogT<RecordT>::const_iterator, typename CatalogT<RecordT>::const_iterator>
CatalogT<RecordT>::equal_range(typename Field<T>::Value const& value, Key<T> const& key) const {
    detail::KeyExtractionFunctor<RecordT, T> f = {key};
    // Iterator adaptor that makes a CatalogT iterator work like an iterator over field values.
    using SearchIter = boost::transform_iterator<detail::KeyExtractionFunctor<RecordT, T>, const_iterator>;
    std::pair<SearchIter, SearchIter> i =
            std::equal_range(SearchIter(begin(), f), SearchIter(end(), f), value);
    return std::make_pair(i.first.base(), i.second.base());
}

//@{
/**
 *  @internal
 *
 *  This block is just a big, ugly workaround for a Swig deficiency: apparently we can't add
 *  templated methods to an %extend block (or at least we can't do that if the class is also templated),
 *  which we'd need to do to wrap these because they return iterators in C++ and other things in
 *  Python.  Instead we'll add doubly-templated free functions, and call them from a %pythoncode
 *  block in an %extend block in the .i file.
 *
 *  And we can't put these in an %inline block in the .i file because then the definitions
 *  wouldn't be available to downstream instantiations with catalog/record types defined in
 *  other packages.
 *
 *  But here's the best part: try putting these in 'namespace detail', and detectionLib_wrap.cc
 *  fails to build, because it causes Swig to starts putting "lsst::afw::table::lsst::afw::detection"
 *  in front of Footprint-related things!  I have no idea what unholy combination of chewing gum
 *  and rubber bands is holding this together, but I don't want to touch it.
 *
 *  Oh, how I hate Swig.
 */

template <typename RecordT, typename Catalog, typename T>
std::shared_ptr<RecordT> _Catalog_find(Catalog const& catalog, T const& value, Key<T> const& key) {
    typename Catalog::const_iterator iter = catalog.find(value, key);
    if (iter == catalog.end()) {
        return std::shared_ptr<RecordT>();
    }
    return iter;  // n.b. CatalogIterator is explicitly convertible to shared_ptr
}

template <typename Catalog, typename T>
int _Catalog_lower_bound(Catalog const& catalog, T const& value, Key<T> const& key) {
    return catalog.lower_bound(value, key) - catalog.begin();
}

template <typename Catalog, typename T>
int _Catalog_upper_bound(Catalog const& catalog, T const& value, Key<T> const& key) {
    return catalog.upper_bound(value, key) - catalog.begin();
}

template <typename Catalog, typename T>
std::pair<int, int> _Catalog_equal_range(Catalog const& catalog, T const& value, Key<T> const& key) {
    std::pair<typename Catalog::const_iterator, typename Catalog::const_iterator> p =
            catalog.equal_range(value, key);
    return std::pair<int, int>(p.first - catalog.begin(), p.second - catalog.begin());
}

//@}
}  // namespace table
}  // namespace afw
}  // namespace lsst

#endif  // !AFW_TABLE_Catalog_h_INCLUDED
