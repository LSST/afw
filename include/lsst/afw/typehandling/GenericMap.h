// -*- LSST-C++ -*-
/*
 * This file is part of afw.
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSST_AFW_TYPEHANDLING_GENERICMAP_H
#define LSST_AFW_TYPEHANDLING_GENERICMAP_H

#include <algorithm>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <variant>

#include "lsst/pex/exceptions.h"
#include "lsst/afw/typehandling/detail/type_traits.h"
#include "lsst/afw/typehandling/detail/refwrap_utils.h"
#include "lsst/afw/typehandling/Key.h"
#include "lsst/afw/typehandling/Storable.h"
#include "lsst/afw/typehandling/PolymorphicValue.h"

namespace lsst {
namespace afw {
namespace typehandling {

/**
 * Interface for a heterogeneous map.
 *
 * Objects of type GenericMap cannot necessarily have keys added or removed, although mutable values can be
 * modified as usual. In Python, a GenericMap behaves like a ``collections.abc.Mapping``. See
 * MutableGenericMap for a GenericMap that must allow insertions and deletions.
 *
 * @tparam K the key type of the map.
 *
 * A Key for the map is parameterized by both the key type `K` and a corresponding value type `V`. The map
 * is indexed uniquely by a value of type `K`; no two entries in the map may have identical values of
 * Key::getId().
 *
 * All operations are sensitive to the value type of the key: a @ref contains(Key<K,T> const&) const
 * "contains" call requesting an integer labeled "value", for example, will report no such integer if instead
 * there is a string labeled "value". For Python compatibility, a GenericMap does not store type information
 * internally, instead relying on RTTI for type checking.
 *
 * All subclasses **must** guarantee, as a class invariant, that every value in the map is implicitly
 * nothrow-convertible to the type indicated by its key. For example, MutableGenericMap ensures this by
 * appropriately templating all operations that create new key-value pairs.
 *
 * A GenericMap may contain primitive types, strings, Storable, and shared pointers to Storable as
 * values. It does not support unique pointers to Storable because such pointers are read destructively. For
 * safety reasons, it may not contain references, C-style pointers, or arrays to any type. Due to
 * implementation restrictions, `const` types (except pointers to `const` Storable) are not
 * currently supported.
 */
// TODO: correctly handling const vs. non-const keys should be possible in C++17 with std::variant
template <typename K>
class GenericMap {
public:
    using key_type = K;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    virtual ~GenericMap() noexcept = default;

    /**
     * Return a reference to the mapped value of the element with key equal to `key`.
     *
     * @tparam T the type of the element mapped to `key`. It may be the exact
     *           type of the element, if known, or any type to which its
     *           references or pointers can be implicitly converted (e.g.,
     *           a superclass). For example, references to built-in types are
     *           not convertible, so you can't retrieve an `int` with `T=long`.
     * @param key the key of the element to find
     *
     * @return a reference to the `T` mapped to `key`, if one exists
     *
     * @throws pex::exceptions::OutOfRangeError Thrown if the map does not
     *         have a `T` with the specified key
     * @exceptsafe Provides strong exception safety.
     *
     * @note This implementation calls @ref unsafeLookup once, then uses templates
     *       and RTTI to determine if the value is of the expected type.
     *
     * @{
     */
    template <typename T, typename std::enable_if_t<!detail::IS_SMART_PTR<T>, int> = 0>
    T& at(Key<K, T> const& key) {
        // Both casts are safe; see Effective C++, Item 3
        return const_cast<T&>(static_cast<const GenericMap&>(*this).at(key));
    }

    template <typename T, typename std::enable_if_t<!detail::IS_SMART_PTR<T>, int> = 0>
    T const& at(Key<K, T> const& key) const {
        // Delegate to private methods to hide further special-casing of T
        return _at(key);
    }

    template <typename T, typename std::enable_if_t<std::is_base_of<Storable, T>::value, int> = 0>
    std::shared_ptr<T> at(Key<K, std::shared_ptr<T>> const& key) const {
        // Delegate to private methods to hide further special-casing of T
        return _at(key);
    }

    /** @} */

    /// Return the number of key-value pairs in the map.
    virtual size_type size() const noexcept = 0;

    /// Return `true` if this map contains no key-value pairs.
    virtual bool empty() const noexcept = 0;

    /**
     * Return the maximum number of elements the container is able to hold due to system or library
     * implementation limitations.
     *
     * @note This value typically reflects the theoretical limit on the size of the container. At runtime, the
     * size of the container may be limited to a value smaller than max_size() by the amount of RAM available.
     */
    virtual size_type max_size() const noexcept = 0;

    /**
     * Return the number of elements mapped to the specified key.
     *
     * @tparam T the value corresponding to `key`
     * @param key key value of the elements to count
     *
     * @return number of `T` with key `key`, that is, either 1 or 0.
     *
     * @exceptsafe Provides strong exception safety.
     *
     * @note This implementation calls @ref contains(Key<K,T> const&) const "contains".
     *
     */
    template <typename T>
    size_type count(Key<K, T> const& key) const {
        return contains(key) ? 1 : 0;
    }

    /**
     * Return `true` if this map contains a mapping whose key has the specified label.
     *
     * More formally, this method returns `true` if and only if this map contains a mapping with a key `k`
     * such that `k.getId() == key`. There can be at most one such mapping.
     *
     * @param key the weakly-typed key to search for
     *
     * @return `true` if this map contains a mapping for `key`, regardless of value type.
     *
     * @exceptsafe Provides strong exception safety.
     */
    virtual bool contains(K const& key) const = 0;

    /**
     * Return `true` if this map contains a mapping for the specified key.
     *
     * This is equivalent to testing whether `at(key)` would succeed.
     *
     * @tparam T the value corresponding to `key`
     * @param key the key to search for
     *
     * @return `true` if this map contains a mapping from the specified key to a `T`
     *
     * @exceptsafe Provides strong exception safety.
     *
     * @note This implementation calls contains(K const&) const. If the call returns
     *       `true`, it calls @ref unsafeLookup, then uses templates and RTTI to
     *       determine if the value is of the expected type. The performance of
     *       this method depends strongly on the performance of
     *       contains(K const&).
     */
    template <typename T>
    bool contains(Key<K, T> const& key) const {
        // Delegate to private methods to hide special-casing of T
        return _contains(key);
    }

    /**
     * Return the set of all keys, without type information.
     *
     * @return a read-only view of all keys currently in the map, in the same
     *         iteration order as this object. The view will be updated by
     *         changes to the underlying map.
     *
     * @warning Do not modify this map while iterating over its keys.
     * @warning Do not store the returned reference in variables that outlive
     *          the map; destroying the map will invalidate the reference.
     *
     * @note The keys are returned as a list, rather than a set, so that subclasses can give them a
     * well-defined iteration order.
     */
    virtual std::vector<K> const& keys() const noexcept = 0;

    /**
     * Test for map equality.
     *
     * Two GenericMap objects are considered equal if they map the same keys to
     * the same values. The two objects do not need to have the same
     * implementation class. If either class orders its keys, the order
     * is ignored.
     *
     * @note This implementation calls @ref keys on both objects and compares
     *       the results. If the two objects have the same keys, it calls
     *       @ref unsafeLookup for each key and compares the values.
     *
     * @{
     */
    virtual bool operator==(GenericMap const& other) const noexcept {
        auto keys1 = this->keys();
        auto keys2 = other.keys();
        if (!std::is_permutation(keys1.begin(), keys1.end(), keys2.begin(), keys2.end())) {
            return false;
        }
        for (K const& key : keys1) {
            // Can't use std::variant::operator== here because
            // std::reference_wrapper isn't equality-comparable.
            if (!detail::refwrap_equals()(this->unsafeLookup(key), other.unsafeLookup(key))) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(GenericMap const& other) const { return !(*this == other); }

    /** @} */

    /**
     * Apply an operation to each key-value pair in the map.
     *
     * @tparam Visitor a callable that takes a key and a value. See below for
     *                 exact requirements.
     * @param visitor the visitor to apply
     * @returns if `Visitor` has a return value, a `std::vector` of values
     *          returned from applying `visitor` to each key in @ref keys, in
     *          that order. Otherwise, `void`.
     *
     * @exceptsafe Provides the same level of exception safety as `Visitor`, or
     *             strong exception safety if `Visitor` cannot throw.
     *
     * A `Visitor` must define one or more `operator()` that take a
     * weakly-typed key and a value. Each `operator()` must return the same
     * type (which may be `void`). Through any combination of overloading or
     * templates, the visitor must accept values of the following types:
     *      * either `bool` or `bool const&`
     *      * either `int` or `int const&`
     *      * either `long` or `long const&`
     *      * either `long long` or `long long const&`
     *      * either `float` or `float const&`
     *      * either `double` or `double const&`
     *      * `std::string const&`
     *      * `Storable const&`
     *      * `std::shared_ptr<Storable const>`
     *
     * @note This implementation calls @ref keys, then calls @ref unsafeLookup
     *       for each key before passing the result to `visitor`.
     *
     * An example visitor that prints each key-value pair to standard output:
     *
     *     template <typename K>
     *     class Printer {
     *     public:
     *         template <typename V>
     *         void operator()(K const& key, V const& value) {
     *             std::cout << key << ": " << value << "," << std::endl;
     *         }
     *
     *         void operator()(K const& key, Storable const& value) {
     *             std::cout << key << ": ";
     *             try {
     *                 std::cout << value;
     *             } catch (UnsupportedOperationException const&) {
     *                 std::cout << "[unprintable]";
     *             }
     *             std::cout << "," << std::endl;
     *         }
     *
     *         void operator()(K const& key, std::shared_ptr<Storable const> value) {
     *             if (value != nullptr) {
     *                 operator()(key, *value);
     *             } else {
     *                 operator()(key, "null");
     *             }
     *         }
     *     };
     */
    template <class Visitor>
    auto apply(Visitor&& visitor) const {
        // Delegate to private methods to hide special-casing of Visitor
        return _apply(visitor);
    }

    /**
     * Apply a modifying operation to each key-value pair in the map.
     *
     * @tparam Visitor a callable that takes a key and a value. Requirements as for
     *                 @ref apply(Visitor&&) const, except that it may take
     *                 non-`const` references to values.
     * @param visitor the visitor to apply
     * @returns if `Visitor` has a return value, a `std::vector` of values
     *          returned from applying `visitor` to each key in @ref keys, in
     *          that order. Otherwise, `void`.
     *
     * @exceptsafe Provides basic exception safety if `Visitor` is exception-safe.
     *
     * @note This implementation calls @ref keys, then calls @ref unsafeLookup
     *       for each key before passing the result to `visitor`.
     */
    template <class Visitor>
    auto apply(Visitor&& visitor) {
        // Delegate to private methods to hide special-casing of Visitor
        return _apply(visitor);
    }

private:
    // Transformations between value/reference/ref-to-const versions of internal variant type, to let
    //     the set of value types supported by GenericMap be defined only once
    // Icky TMP, but I can't find another way to get at the template arguments for variant :(
    // Methods have no definition but can't be deleted without breaking type definitions below

    /// @cond
    template <typename... Types>
    static std::variant<std::reference_wrapper<Types>...> _typeToRef(
            std::variant<Types...> const&) noexcept;
    template <typename... Types>
    static std::variant<std::reference_wrapper<std::add_const_t<Types>>...> _typeToConstRef(
            std::variant<Types...> const&) noexcept;
    /// @endcond

protected:
    /**
     * The types that can be stored in a map.
     *
     * Keys of any subclass of Storable are implemented using PolymorphicValue to preserve type.
     */
    // Use int, long, long long instead of int32_t, int64_t because C++ doesn't
    // consider signed integers of the same length but different names equivalent
    using StorableType = std::variant<bool, int, long, long long, float, double, std::string,
                                      PolymorphicValue, std::shared_ptr<Storable const>>;

    /**
     * A type-agnostic reference to the value stored inside the map.
     *
     * These are the reference equivalents (`T const&` or `T&`) of @ref StorableType.
     * @{
     */
    // this mouthful is shorter than the equivalent expression with result_of
    using ConstValueReference = decltype(_typeToConstRef(std::declval<StorableType>()));
    using ValueReference = decltype(_typeToRef(std::declval<StorableType>()));

    /** @} */

    /**
     * Return a reference to the mapped value of the element with key equal to `key`.
     *
     * This method is the primary way to implement the GenericMap interface.
     *
     * @param key the key of the element to find
     *
     * @return the value mapped to `key`, if one exists
     *
     * @throws pex::exceptions::OutOfRangeError Thrown if the map does not have
     *         a value with the specified key
     * @exceptsafe Must provide strong exception safety.
     *
     * @{
     */
    virtual ConstValueReference unsafeLookup(K key) const = 0;

    ValueReference unsafeLookup(K key) {
        ConstValueReference constRef = static_cast<const GenericMap&>(*this).unsafeLookup(key);
        return std::visit(
            [](auto const & v) { return ValueReference(detail::refwrap_const_cast(v)); },
            constRef
        );
    }

    /** @} */

private:
    // Neither Storable nor shared_ptr<Storable>
    template <typename T,
              typename std::enable_if_t<
                      std::is_fundamental<T>::value || std::is_base_of<std::string, T>::value, int> = 0>
    T const& _at(Key<K, T> const& key) const {
        static_assert(!std::is_const<T>::value,
                      "Due to implementation constraints, const keys are not supported.");
        auto foo = unsafeLookup(key.getId());
        if (std::holds_alternative<std::reference_wrapper<T const>>(foo)) {
            return std::get<std::reference_wrapper<T const>>(foo);
        } else {
            std::stringstream message;
            message << "Key " << key << " not found, but a key labeled " << key.getId() << " is present.";
            throw LSST_EXCEPT(pex::exceptions::OutOfRangeError, message.str());
        }
    }

    // Storable and its subclasses
    template <typename T, typename std::enable_if_t<std::is_base_of<Storable, T>::value, int> = 0>
    T const& _at(Key<K, T> const& key) const {
        static_assert(!std::is_const<T>::value,
                      "Due to implementation constraints, const keys are not supported.");
        auto foo = unsafeLookup(key.getId());
        if (std::holds_alternative<std::reference_wrapper<PolymorphicValue const>>(foo)) {
            Storable const& value = std::get<std::reference_wrapper<PolymorphicValue const>>(foo).get();
            T const* typedPointer = dynamic_cast<T const*>(&value);
            if (typedPointer != nullptr) {
                return *typedPointer;
            } else {
                std::stringstream message;
                message << "Key " << key << " not found, but a key labeled " << key.getId() << " is present.";
                throw LSST_EXCEPT(pex::exceptions::OutOfRangeError, message.str());
            }
        } else {
            std::stringstream message;
            message << "Key " << key << " not found, but a key labeled " << key.getId() << " is present.";
            throw LSST_EXCEPT(pex::exceptions::OutOfRangeError, message.str());
        }
    }

    // shared_ptr<Storable>
    template <typename T, typename std::enable_if_t<std::is_base_of<Storable, T>::value, int> = 0>
    std::shared_ptr<T> _at(Key<K, std::shared_ptr<T>> const& key) const {
        static_assert(std::is_const<T>::value,
                      "Due to implementation constraints, pointers to non-const are not supported.");
        auto foo = unsafeLookup(key.getId());
        if (std::holds_alternative<std::reference_wrapper<std::shared_ptr<Storable const> const>>(foo)) {
            auto pointer = std::get<std::reference_wrapper<std::shared_ptr<Storable const> const>>(foo).get();
            if (pointer == nullptr) {  // dynamic_cast not helpful
                return nullptr;
            }

            std::shared_ptr<T> typedPointer = std::dynamic_pointer_cast<T>(pointer);
            // shared_ptr can be empty without being null. dynamic_pointer_cast
            // only promises result of failed cast is empty, so test for that
            if (typedPointer.use_count() > 0) {
                return typedPointer;
            } else {
                std::stringstream message;
                message << "Key " << key << " not found, but a key labeled " << key.getId() << " is present.";
                throw LSST_EXCEPT(pex::exceptions::OutOfRangeError, message.str());
            }
        } else {
            std::stringstream message;
            message << "Key " << key << " not found, but a key labeled " << key.getId() << " is present.";
            throw LSST_EXCEPT(pex::exceptions::OutOfRangeError, message.str());
        }
    }

    // Neither Storable nor shared_ptr<Storable>
    template <typename T,
              typename std::enable_if_t<
                      std::is_fundamental<T>::value || std::is_base_of<std::string, T>::value, int> = 0>
    bool _contains(Key<K, T> const& key) const {
        // Avoid actually getting and casting an object, if at all possible
        if (!contains(key.getId())) {
            return false;
        }

        auto foo = unsafeLookup(key.getId());
        return std::holds_alternative<std::reference_wrapper<T const>>(foo);
    }

    // Storable and its subclasses
    template <typename T, typename std::enable_if_t<std::is_base_of<Storable, T>::value, int> = 0>
    bool _contains(Key<K, T> const& key) const {
        // Avoid actually getting and casting an object, if at all possible
        if (!contains(key.getId())) {
            return false;
        }

        auto foo = unsafeLookup(key.getId());
        auto refwrap = std::get_if<std::reference_wrapper<PolymorphicValue const>>(&foo);
        if (refwrap) {
            Storable const & value = refwrap->get();
            auto asT = dynamic_cast<T const*>(&value);
            return asT != nullptr;
        } else {
            return false;
        }
    }

    // shared_ptr<Storable>
    template <typename T, typename std::enable_if_t<std::is_base_of<Storable, T>::value, int> = 0>
    bool _contains(Key<K, std::shared_ptr<T>> const& key) const {
        static_assert(std::is_const<T>::value,
                      "Due to implementation constraints, pointers to non-const are not supported.");
        // Avoid actually getting and casting an object, if at all possible
        if (!contains(key.getId())) {
            return false;
        }

        auto foo = unsafeLookup(key.getId());
        if (std::holds_alternative<std::reference_wrapper<std::shared_ptr<Storable const> const>>(foo)) {
            auto pointer = std::get<std::reference_wrapper<std::shared_ptr<Storable const> const>>(foo).get();
            if (pointer == nullptr) {  // Can't confirm type with dynamic_cast
                return true;
            }
            std::shared_ptr<T> typedPointer = std::dynamic_pointer_cast<T>(pointer);
            // shared_ptr can be empty without being null. dynamic_pointer_cast
            // only promises result of failed cast is empty, so test for that
            return typedPointer.use_count() > 0;
        } else {
            return false;
        }
    }

    // Type alias to properly handle Visitor output
    // Assume that each operator() has the same return type; variant will enforce it
    /// @cond
    template <class Visitor>
    using _VisitorResult = std::invoke_result_t<Visitor, K&&, bool&>;
    /// @endcond

    // No return value, const GenericMap
    template <class Visitor, typename std::enable_if_t<std::is_void<_VisitorResult<Visitor>>::value, int> = 0>
    void _apply(Visitor&& visitor) const {
        auto wrapped = detail::make_refwrap_visitor(std::forward<Visitor>(visitor));
        for (K const& key : keys()) {
            std::variant<K> varKey = key;
            std::visit(wrapped, varKey, unsafeLookup(key));
        }
    }

    // Return value, const GenericMap
    template <class Visitor,
              typename std::enable_if_t<!std::is_void<_VisitorResult<Visitor>>::value, int> = 0>
    auto _apply(Visitor&& visitor) const {
        std::vector<_VisitorResult<Visitor>> results;
        results.reserve(size());
        auto wrapped = detail::make_refwrap_visitor(std::forward<Visitor>(visitor));
        for (K const& key : keys()) {
            std::variant<K> varKey = key;
            results.emplace_back(std::visit(wrapped, varKey, unsafeLookup(key)));
        }
        return results;
    }

    // No return value, non-const GenericMap
    template <class Visitor, typename std::enable_if_t<std::is_void<_VisitorResult<Visitor>>::value, int> = 0>
    void _apply(Visitor&& visitor) {
        auto wrapped = detail::make_refwrap_visitor(std::forward<Visitor>(visitor));
        for (K const& key : keys()) {
            std::variant<K> varKey = key;
            std::visit(wrapped, varKey, unsafeLookup(key));
        }
    }

    // Return value, non-const GenericMap
    template <class Visitor,
              typename std::enable_if_t<!std::is_void<_VisitorResult<Visitor>>::value, int> = 0>
    auto _apply(Visitor&& visitor) {
        std::vector<_VisitorResult<Visitor>> results;
        results.reserve(size());
        auto wrapped = detail::make_refwrap_visitor(std::forward<Visitor>(visitor));

        for (K const& key : keys()) {
            std::variant<K> varKey = key;
            results.emplace_back(std::visit(wrapped, varKey, unsafeLookup(key)));
        }
        return results;
    }
};

/**
 * Interface for a GenericMap that allows element addition and removal.
 *
 * In Python, a MutableGenericMap behaves like a ``collections.abc.MutableMapping``.
 *
 * @note Unlike standard library maps, this class does not support `operator[]` or `insert_or_assign`. This is
 * because these operations would have surprising behavior when dealing with keys of different types but the
 * same Key::getId().
 *
 */
template <typename K>
class MutableGenericMap : public GenericMap<K> {
protected:
    using typename GenericMap<K>::StorableType;

public:
    virtual ~MutableGenericMap() noexcept = default;

    /**
     * Remove all of the mappings from this map.
     *
     * After this call, the map will be empty.
     */
    virtual void clear() noexcept = 0;

    /**
     * Insert an element into the map, if the map doesn't already contain a mapping with the same or a
     * conflicting key.
     *
     * @tparam T the type of value to insert
     * @param key key to insert
     * @param value value to insert
     *
     * @return `true` if the insertion took place, `false` otherwise
     *
     * @exceptsafe Provides strong exception safety.
     *
     * @note It is possible for a key with a value type other than `T` to prevent insertion. Callers can
     * safely assume `this->contains(key.getId())` as a postcondition, but not `this->contains(key)`.
     *
     * @note This implementation calls @ref contains(K const&) const "contains",
     *       then calls @ref unsafeInsert if there is no conflicting key.
     */
    template <typename T>
    bool insert(Key<K, T> const& key, T const& value) {
        if (this->contains(key.getId())) {
            return false;
        }

        return unsafeInsert(key.getId(), StorableType(value));
    }

    /**
     * Insert an element into the map, if the map doesn't already contain a mapping with a conflicting key.
     *
     * @tparam T the type of value to insert
     * @param key key to insert
     * @param value value to insert
     *
     * @return a pair consisting of a strongly-typed key for the value and a flag that is `true` if the
     * insertion took place and `false` otherwise
     *
     * @exceptsafe Provides strong exception safety.
     *
     * @warning the type of the compiler-generated key may not always be what you expect. Callers should save
     * the returned key if they wish to retrieve the value later.
     */
    template <typename T>
    std::pair<Key<K, T>, bool> insert(K const& key, T const& value) {
        auto strongKey = makeKey<T>(key);
        // Construct return value in advance, so that exception from copying/moving Key is atomic
        auto result = make_pair(strongKey, false);
        result.second = insert(strongKey, value);
        return result;
    }

    /**
     * Remove the mapping for a key from this map, if it exists.
     *
     * @tparam T the type of value the key maps to
     * @param key the key to remove
     *
     * @return `true` if `key` was removed, `false` if it was not present
     *
     * @exceptsafe Provides strong exception safety.
     *
     * @note This implementation calls @ref contains(Key<K,T> const&) const "contains",
     *       then calls @ref unsafeErase if the key is present.
     */
    template <typename T>
    bool erase(Key<K, T> const& key) {
        if (this->contains(key)) {
            return unsafeErase(key.getId());
        } else {
            return false;
        }
    }

protected:
    /**
     * Create a new mapping with key equal to `key` and value equal to `value`.
     *
     * This method is the primary way to implement the MutableGenericMap interface.
     *
     * @param key the key of the element to insert. The method may assume that the map does not contain `key`.
     * @param value a reference to the value to insert.
     *
     * @return `true` if the insertion took place, `false` otherwise
     *
     * @exceptsafe Must provide strong exception safety.
     */
    virtual bool unsafeInsert(K key, StorableType&& value) = 0;

    /**
     * Remove the mapping for a key from this map, if it exists.
     *
     * @param key the key to remove
     *
     * @return `true` if `key` was removed, `false` if it was not present
     *
     * @exceptsafe Must provide strong exception safety.
     */
    virtual bool unsafeErase(K key) = 0;
};

}  // namespace typehandling
}  // namespace afw
}  // namespace lsst

#endif
