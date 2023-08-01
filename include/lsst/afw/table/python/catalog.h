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
#ifndef AFW_TABLE_PYTHON_CATALOG_H_INCLUDED
#define AFW_TABLE_PYTHON_CATALOG_H_INCLUDED

#include "pybind11/pybind11.h"

#include "ndarray/pybind11.h"
#include "lsst/utils/python.h"
#include "lsst/afw/table/BaseColumnView.h"
#include "lsst/afw/table/Catalog.h"

namespace lsst {
namespace afw {
namespace table {
namespace python {

template <typename Record>
using PyCatalog = pybind11::class_<CatalogT<Record>>;

/// Extract a column from a potentially non-contiguous Catalog
template <typename T, typename Record>
ndarray::Array<typename Field<T>::Value const, 1, 1> _getArrayFromCatalog(
        CatalogT<Record> const &catalog,  ///< Catalog
        Key<T> const &key                 ///< Key to column to extract
) {
    ndarray::Array<typename Field<T>::Value, 1, 1> out = ndarray::allocate(catalog.size());
    auto outIter = out.begin();
    auto inIter = catalog.begin();
    for (; inIter != catalog.end(); ++inIter, ++outIter) {
        *outIter = inIter->get(key);
    }
    return out;
}

/// Extract a column from a potentially non-contiguous Catalog (angle
/// specialization)
template <typename Record>
ndarray::Array<double const, 1, 1> _getArrayFromCatalog(
        CatalogT<Record> const &catalog,  ///< Catalog
        Key<Angle> const &key                 ///< Key to column to extract
) {
    ndarray::Array<double, 1, 1> out = ndarray::allocate(catalog.size());
    auto outIter = out.begin();
    auto inIter = catalog.begin();
    for (; inIter != catalog.end(); ++inIter, ++outIter) {
        *outIter = inIter->get(key).asRadians();
    }
    return out;
}

/// Extract an array-valued column from a potentially non-contiguous Catalog
/// into a 2-d array.
template <typename T, typename Record>
ndarray::Array<typename Field<T>::Value const, 2, 2> _getArrayFromCatalog(
        CatalogT<Record> const &catalog,  ///< Catalog
        Key<Array<T>> const &key          ///< Key to column to extract
) {
    ndarray::Array<typename Field<T>::Value, 2, 2> out = ndarray::allocate(catalog.size(), key.getSize());
    auto outIter = out.begin();
    auto inIter = catalog.begin();
    for (; inIter != catalog.end(); ++inIter, ++outIter) {
        *outIter = inIter->get(key);
    }
    return out;
}

template <typename Record>
void _setFlagColumnToArray(
    CatalogT<Record> & catalog,
    Key<Flag> const & key,
    ndarray::Array<bool const, 1> const & array
) {
    if (array.size() != catalog.size()) {
        throw LSST_EXCEPT(
            pex::exceptions::LengthError,
            (boost::format("Catalog has %d rows, while array has %d elements.")
             % catalog.size() % array.size()).str()
        );
    }
    auto catIter = catalog.begin();
    auto arrayIter = array.begin();
    for (; catIter != catalog.end(); ++catIter, ++arrayIter) {
        catIter->set(key, *arrayIter);
    }
}

template <typename Record>
void _setFlagColumnToScalar(
    CatalogT<Record> & catalog,
    Key<Flag> const & key,
    bool value
) {
    for (auto catIter = catalog.begin(); catIter != catalog.end(); ++catIter) {
        catIter->set(key, value);
    }
}

// Custom safe Python iterator for Catalog.  See Catalog.__iter__ wrapper
// for details.
template <typename Record>
class PyCatalogIndexIterator {
public:

    using value_type = std::shared_ptr<Record>;
    using reference = std::shared_ptr<Record>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;

    PyCatalogIndexIterator(CatalogT<Record> const * catalog, std::size_t index) : _catalog(catalog), _index(index) {}

    std::shared_ptr<Record> operator*() const {
        if (_index < _catalog->size()) {
            return _catalog->get(_index);
        }
        throw std::out_of_range(
            "Catalog shrunk during iteration, invalidating this iterator."
        );
    }

    PyCatalogIndexIterator & operator++() {
        ++_index;
        return *this;
    }

    bool operator==(PyCatalogIndexIterator const & other) const {
        return _catalog == other._catalog && _index == other._index;
    }

    bool operator!=(PyCatalogIndexIterator const & other) const {
        return !(*this == other);
    }

private:
    CatalogT<Record> const * _catalog;
    std::size_t _index;
};

/**
 * Declare field-type-specific overloaded catalog member functions for one field type
 *
 * @tparam T  Field type.
 * @tparam Record  Record type, e.g. BaseRecord or SimpleRecord.
 *
 * @param[in] cls  Catalog pybind11 class.
 */
template <typename T, typename Record>
void declareCatalogOverloads(PyCatalog<Record> &cls) {
    namespace py = pybind11;
    using namespace pybind11::literals;

    using Catalog = CatalogT<Record>;
    using Value = typename Field<T>::Value;
    using ColumnView = typename Record::ColumnView;

    cls.def("isSorted", (bool (Catalog::*)(Key<T> const &) const) & Catalog::isSorted);
    cls.def("sort", (void (Catalog::*)(Key<T> const &)) & Catalog::sort);
    cls.def("find", [](Catalog &self, Value const &value, Key<T> const &key) -> std::shared_ptr<Record> {
        auto iter = self.find(value, key);
        if (iter == self.end()) {
            return nullptr;
        };
        return iter;
    });
    cls.def("upper_bound", [](Catalog &self, Value const &value, Key<T> const &key) -> std::ptrdiff_t {
        return self.upper_bound(value, key) - self.begin();
    });
    cls.def("lower_bound", [](Catalog &self, Value const &value, Key<T> const &key) -> std::ptrdiff_t {
        return self.lower_bound(value, key) - self.begin();
    });
    cls.def("equal_range", [](Catalog &self, Value const &value, Key<T> const &key) {
        auto p = self.equal_range(value, key);
        return py::slice(p.first - self.begin(), p.second - self.begin(), 1);
    });
    cls.def("between", [](Catalog &self, Value const &lower, Value const &upper, Key<T> const &key) {
        std::ptrdiff_t a = self.lower_bound(lower, key) - self.begin();
        std::ptrdiff_t b = self.upper_bound(upper, key) - self.begin();
        return py::slice(a, b, 1);
    });

    cls.def("_get_column_from_key",
            [](Catalog const &self, Key<T> const &key, pybind11::object py_column_view) {
                std::shared_ptr<ColumnView> column_view = py_column_view.cast<std::shared_ptr<ColumnView>>();
                if (!column_view && self.isContiguous()) {
                    // If there's no column view cached, but there could be,
                    // make one (and we'll return it so it can be cached by
                    // the calling Python code).
                    column_view = std::make_shared<ColumnView>(self.getColumnView());
                    py_column_view = pybind11::cast(column_view);
                }
                if (column_view) {
                    // If there is a column view, use it to return a view.
                    if constexpr (std::is_same_v<T, Angle>) {
                        // numpy doesn't recognize our Angle type, so we return
                        // double radians.
                        return pybind11::make_tuple(column_view->get_radians_array(key), column_view);
                    } else {
                        return pybind11::make_tuple((*column_view)[key].shallow(), column_view);
                    }
                }
                // If we can't make a column view, extract a copy.
                return pybind11::make_tuple(_getArrayFromCatalog(self, key), column_view);
            });
}

/**
 * Declare field-type-specific overloaded catalog member functions for one array-valued field type
 *
 * @tparam T  Array element type.
 * @tparam Record  Record type, e.g. BaseRecord or SimpleRecord.
 *
 * @param[in] cls  Catalog pybind11 class.
 */
template <typename T, typename Record>
void declareCatalogArrayOverloads(PyCatalog<Record> &cls) {
    namespace py = pybind11;
    using namespace pybind11::literals;

    using Catalog = CatalogT<Record>;
    using Value = typename Field<T>::Value;
    using ColumnView = typename Record::ColumnView;

    cls.def("_get_column_from_key",
            [](Catalog const &self, Key<Array<T>> const &key, pybind11::object py_column_view) {
                std::shared_ptr<ColumnView> column_view = py_column_view.cast<std::shared_ptr<ColumnView>>();
                if (!column_view && self.isContiguous()) {
                    // If there's no column view cached, but there could be,
                    // make one (and we'll return it so it can be cached by
                    // the calling Python code).
                    column_view = std::make_shared<ColumnView>(self.getColumnView());
                    py_column_view = pybind11::cast(column_view);
                }
                if (column_view) {
                    // If there is a column view, use it to return view.
                    return pybind11::make_tuple((*column_view)[key].shallow(), column_view);
                }
                // If we can't make a column view, extract a copy.
                return pybind11::make_tuple(_getArrayFromCatalog(self, key), column_view);
            });
}

/**
 * Wrap an instantiation of lsst::afw::table::CatalogT<Record>.
 *
 * In addition to calling this method you must call addCatalogMethods on the
 * class object in Python.
 *
 * @tparam Record  Record type, e.g. BaseRecord or SimpleRecord.
 *
 * @param[in] wrappers Package manager class will be added to.
 * @param[in] name   Name prefix of the record type, e.g. "Base" or "Simple".
 * @param[in] isBase Whether this instantiation is only being used as a base class
 *                   (used to set the class name).
 */
template <typename Record>
PyCatalog<Record> declareCatalog(utils::python::WrapperCollection &wrappers, std::string const &name,
                                 bool isBase = false) {
    namespace py = pybind11;
    using namespace pybind11::literals;

    using Catalog = CatalogT<Record>;
    using Table = typename Record::Table;
    using ColumnView = typename Record::ColumnView;

    std::string fullName;
    if (isBase) {
        fullName = "_" + name + "CatalogBase";
    } else {
        fullName = name + "Catalog";
    }

    // We need py::dynamic_attr() in the class definition to support our Python-side caching
    // of the associated ColumnView.
    return wrappers.wrapType(
            PyCatalog<Record>(wrappers.module, fullName.c_str(), py::dynamic_attr()),
            [](auto &mod, auto &cls) {
                /* Constructors */
                cls.def(py::init<Schema const &>(), "schema"_a);
                cls.def(py::init<std::shared_ptr<Table> const &>(), "table"_a);
                cls.def(py::init<Catalog const &>(), "other"_a);

                /* Static Methods */
                cls.def_static("readFits", (Catalog(*)(std::string const &, int, int)) & Catalog::readFits,
                               "filename"_a, "hdu"_a = fits::DEFAULT_HDU, "flags"_a = 0);
                cls.def_static("readFits", (Catalog(*)(fits::MemFileManager &, int, int)) & Catalog::readFits,
                               "manager"_a, "hdu"_a = fits::DEFAULT_HDU, "flags"_a = 0);
                // readFits taking Fits objects not wrapped, because Fits objects are not wrapped.

                /* Methods */
                cls.def("getTable", &Catalog::getTable);
                cls.def_property_readonly("table", &Catalog::getTable);
                cls.def("getSchema", &Catalog::getSchema);
                cls.def_property_readonly("schema", &Catalog::getSchema);
                cls.def("capacity", &Catalog::capacity);
                cls.def("__len__", &Catalog::size);
                cls.def("resize", &Catalog::resize);

                // Use private names for the following so the public Python method
                // can manage the _column cache
                cls.def("_getColumnView", &Catalog::getColumnView);
                cls.def("_addNew", &Catalog::addNew);
                cls.def("_extend", [](Catalog &self, Catalog const &other, bool deep) {
                    self.insert(self.end(), other.begin(), other.end(), deep);
                });
                cls.def("_extend", [](Catalog &self, Catalog const &other, SchemaMapper const &mapper) {
                    self.insert(mapper, self.end(), other.begin(), other.end());
                });
                cls.def("_append",
                        [](Catalog &self, std::shared_ptr<Record> const &rec) { self.push_back(rec); });
                cls.def("_delitem_", [](Catalog &self, std::ptrdiff_t i) {
                    self.erase(self.begin() + utils::python::cppIndex(self.size(), i));
                });
                cls.def("_delslice_", [](Catalog &self, py::slice const &s) {
                    Py_ssize_t start = 0, stop = 0, step = 0, length = 0;
                    if (PySlice_GetIndicesEx(s.ptr(), self.size(), &start, &stop, &step, &length) != 0) {
                        throw py::error_already_set();
                    }
                    if (step != 1) {
                        throw py::index_error("Slice step must not exactly 1");
                    }
                    self.erase(self.begin() + start, self.begin() + stop);
                });
                cls.def("_clear", &Catalog::clear);

                cls.def("set", &Catalog::set);
                cls.def("_getitem_", [](Catalog &self, int i) {
                    return self.get(utils::python::cppIndex(self.size(), i));
                });
                cls.def("__iter__", [](Catalog & self) {
                    // We wrap a custom iterator class here for two reasons:
                    //
                    // - letting Python define an automatic iterator that
                    //   delegates to __getitem__(int) is super slow, because
                    //   __getitem__ is overloaded;
                    //
                    // - using pybind11::make_iterator on either Catalog's own
                    //   iterator type or Catalog.getInternal()'s iterator (a
                    //   std::vector iterator) opens us up to undefined
                    //   behavior if a modification to the container those
                    //   iterators during iteration.
                    //
                    // Our custom iterator holds a Catalog and an integer
                    // index, allowing it to do a bounds check at ever access,
                    // but unlike its Python equivalent there's no overloading
                    // in play (and even if it was, C++ overloading is resolved
                    // at compile-time).
                    //
                    // This custom iterator also yields `shared_ptr<Record>`.
                    // That should make the return value policy passed to
                    // `py::make_iterator` irrelevant; we don't need to keep
                    // the catalog alive in order to keep a record alive,
                    // because the `shared_ptr` manages the record's lifetime.
                    // But we still need keep_alive on the `__iter__` method
                    // itself to keep the raw catalog pointer alive as long as
                    // the iterator is alive.
                    return py::make_iterator(
                        PyCatalogIndexIterator<Record>(&self, 0),
                        PyCatalogIndexIterator<Record>(&self, self.size())
                    );
                }, py::keep_alive<0, 1>());
                cls.def("isContiguous", &Catalog::isContiguous);
                cls.def("writeFits",
                        (void (Catalog::*)(std::string const &, std::string const &, int) const) &
                                Catalog::writeFits,
                        "filename"_a, "mode"_a = "w", "flags"_a = 0);
                cls.def("writeFits",
                        (void (Catalog::*)(fits::MemFileManager &, std::string const &, int) const) &
                                Catalog::writeFits,
                        "manager"_a, "mode"_a = "w", "flags"_a = 0);
                cls.def("reserve", &Catalog::reserve);
                cls.def("subset",
                        (Catalog(Catalog::*)(ndarray::Array<bool const, 1> const &) const) & Catalog::subset);
                cls.def("subset",
                        (Catalog(Catalog::*)(std::ptrdiff_t, std::ptrdiff_t, std::ptrdiff_t) const) &
                                Catalog::subset);

                declareCatalogOverloads<std::int32_t>(cls);
                declareCatalogOverloads<std::int64_t>(cls);
                declareCatalogOverloads<float>(cls);
                declareCatalogOverloads<double>(cls);
                declareCatalogOverloads<lsst::geom::Angle>(cls);
                declareCatalogArrayOverloads<std::uint8_t>(cls);
                declareCatalogArrayOverloads<std::uint16_t>(cls);
                declareCatalogArrayOverloads<int>(cls);
                declareCatalogArrayOverloads<float>(cls);
                declareCatalogArrayOverloads<double>(cls);

                cls.def("_get_column_from_key",
                        [](Catalog const &self, Key<Flag> const &key, pybind11::object py_column_view) {
                            // Extra ColumnView arg and return value here are
                            // for consistency with the non-flag overload (up
                            // in declareCatalogOverloads).  Casting the array
                            // (from ndarray::Array to numpy.ndarray) before
                            // return is also for consistency with that, though
                            // it's not strictly necessary.
                            return pybind11::make_tuple(
                                _getArrayFromCatalog(self, key),
                                py_column_view
                            );
                        });
                cls.def(
                    "_set_flag",
                    [](Catalog &self, Key<Flag> const & key, ndarray::Array<bool const, 1> const & array) {
                        _setFlagColumnToArray(self, key, array);
                    }
                );
                cls.def(
                    "_set_flag",
                    [](Catalog &self, Key<Flag> const & key, bool value) {
                        _setFlagColumnToScalar(self, key, value);
                    }
                );

            });
}

}  // namespace python
}  // namespace table
}  // namespace afw
}  // namespace lsst

#endif  // !LSST_AFW_TABLE_PYTHON_CATALOG_H_INCLUDED
