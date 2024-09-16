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

#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"
#include "nanobind/stl/string.h"
#include <nanobind/stl/set.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/map.h>

#include <sstream>

#include "ndarray/nanobind.h"

#include "lsst/cpputils/python.h"

#include "lsst/afw/fits.h"
#include "lsst/afw/table/detail/Access.h"
#include "lsst/afw/table/Schema.h"
#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/SchemaMapper.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace table {

using cpputils::python::WrapperCollection;

namespace {

using PySchema = nb::class_<Schema>;

using PySubSchema = nb::class_<SubSchema>;

template <typename T>
using PyFieldBase = nb::class_<FieldBase<T>>;

template <typename T>
using PyKeyBase = nb::class_<KeyBase<T>>;

template <typename T>
using PyField = nb::class_<Field<T>, FieldBase<T>>;

template <typename T>
using PyKey = nb::class_<Key<T>, KeyBase<T>>;

template <typename T>
using PySchemaItem = nb::class_<SchemaItem<T>>;

// Specializations for FieldBase

template <typename T>
void declareFieldBaseSpecializations(PyFieldBase<T> &cls) {
    cls.def(nb::init<>());
}

template <typename T>
void declareFieldBaseSpecializations(PyFieldBase<Array<T>> &cls) {
    cls.def(nb::init<size_t>(), "size"_a = 0);
    cls.def("getSize", &FieldBase<Array<T>>::getSize);
    cls.def("isVariableLength", &FieldBase<Array<T>>::isVariableLength);
}

void declareFieldBaseSpecializations(PyFieldBase<std::string> &cls) {
    cls.def(nb::init<size_t>(), "size"_a = -1);
    cls.def("getSize", &FieldBase<std::string>::getSize);
}

// Specializations for Field

template <typename T>
void declareFieldSpecializations(PyField<T> &cls) {
    cls.def("__getstate__",
            [](Field<T> const &self) {
                /* Return a tuple that fully encodes the state of the object */
                return nb::make_tuple(self.getName(), self.getDoc(), self.getUnits());
            });
    cls.def("__setstate__",
            [](Field<T> *field, nb::tuple t) {
                int const NPARAMS = 3;
                if (t.size() != NPARAMS) {
                    std::ostringstream os;
                    os << "Invalid number of parameters (" << t.size() << ") when unpickling; expected "
                       << NPARAMS;
                    throw std::runtime_error(os.str());
                }
                new (field) Field<T>(nb::cast<std::string>(t[0]), nb::cast<std::string>(t[1]), nb::cast<std::string>(t[2]));
            });
}

// Field<Array<T>> and Field<std::string> have the same pickle implementation
template <typename T>
void _sequenceFieldSpecializations(PyField<T> &cls) {
    cls.def("__getstate__",
            [](Field<T> const &self) {
                /* Return a tuple that fully encodes the state of the object */
                return nb::make_tuple(self.getName(), self.getDoc(), self.getUnits(), self.getSize());
            });
    cls.def("__setstate__",
            [](Field<T> *field, nb::tuple t) {
                int const NPARAMS = 4;
                if (t.size() != NPARAMS) {
                    std::ostringstream os;
                    os << "Invalid number of parameters (" << t.size() << ") when unpickling; expected "
                       << NPARAMS;
                    throw std::runtime_error(os.str());
                }
                new (field) Field<T>(nb::cast<std::string>(t[0]), nb::cast<std::string>(t[1]), nb::cast<std::string>(t[2]),
                                nb::cast<int>(t[3]));
            });
}

template <typename T>
void declareFieldSpecializations(PyField<Array<T>> &cls) {
    _sequenceFieldSpecializations(cls);
}

void declareFieldSpecializations(PyField<std::string> &cls) { _sequenceFieldSpecializations(cls); }

// Specializations for KeyBase

template <typename T>
void declareKeyBaseSpecializations(PyKeyBase<T> &) {}

template <typename T>
void declareKeyBaseSpecializations(PyKeyBase<Array<T>> &cls) {
    cls.def("__getitem__", [](Key<Array<T>> const &self, nb::object const &index) -> nb::object {
        if (nb::isinstance<nb::slice>(index)) {
            nb::slice slice = (nanobind::slice &&) index;
            Py_ssize_t start = 0, stop = 0, step = 0;
            size_t length = 0;
            auto result  = slice.compute(self.getSize());
            start = result.template get<0>();
            stop = result.template get<1>();
            step = result.template get<2>();
            length = result.template get<3>();
            bool valid = true;
            if (!valid) throw nb::python_error();
            if (step != 1) {
                throw LSST_EXCEPT(pex::exceptions::InvalidParameterError,
                                  "Step for array Key indexing must be 1.");
            }
            return nb::cast(self.slice(start, stop));
        } else {
            return nb::cast(self[nb::cast<int>(index)]);
        }
    });
    cls.def("slice", &KeyBase<Array<T>>::slice);
}

// Specializations for Key

template <typename T>
void declareKeyAccessors(PyKey<T> &cls) {
    cls.def("get", [](Key<T> const &self, BaseRecord &record) { return record.get(self); });
    cls.def("set", [](Key<T> const &self, BaseRecord &record, typename Key<T>::Value const &value) {
        record.set(self, value);
    });
}

template <typename U>
void declareKeyAccessors(PyKey<Array<U>> &cls) {
    auto getter = [](Key<Array<U>> const &self, BaseRecord &record) -> ndarray::Array<U, 1, 1> {
        return record[self];
    };
    auto setter = [](Key<Array<U>> const &self, BaseRecord &record, nb::object const &value) {
        if (self.getSize() == 0) {
            // Variable-length array field: do a shallow copy, which requires a non-const
            // contiguous array.
            record.set(self, nb::cast<ndarray::Array<U, 1, 1>>(value));
        } else {
            // Fixed-length array field: do a deep copy, which can work with a const
            // noncontiguous array.  But we need to check the size first, since the
            // penalty for getting that wrong is assert->abort.
            auto v = nb::cast<ndarray::Array<U const, 1, 0>>(value);
            ndarray::ArrayRef<U, 1, 1> ref = record[self];
            if (v.size() != ref.size()) {
                throw LSST_EXCEPT(
                        pex::exceptions::LengthError,
                        (boost::format("Array sizes do not agree: %s != %s") % v.size() % ref.size()).str());
            }
            ref = v;
        }
        return;
    };
    cls.def("get", getter);
    cls.def("set", setter);
}

template <typename T>
void declareKeySpecializations(PyKey<T> &cls) {
    declareKeyAccessors(cls);
    cls.def_prop_ro("subfields", [](nb::object const &) { return nb::none(); });
    cls.def_prop_ro("subkeys", [](nb::object const &) { return nb::none(); });
    cls.def("__getstate__",
            [](Key<T> const &self) {
                /* Return a tuple that fully encodes the state of the object */
                return nb::make_tuple(self.getOffset());
            });
            cls.def("__getstate__",
            [](nb::tuple t) {
                int const NPARAMS = 1;
                if (t.size() != NPARAMS) {
                    std::ostringstream os;
                    os << "Invalid number of parameters (" << t.size() << ") when unpickling; expected "
                       << NPARAMS;
                    throw std::runtime_error(os.str());
                }
                return detail::Access::makeKey<T>(nb::cast<int>(t[0]));
            });
}

void declareKeySpecializations(PyKey<Flag> &cls) {
    declareKeyAccessors(cls);
    cls.def_prop_ro("subfields", [](nb::object const &) { return nb::none(); });
    cls.def_prop_ro("subkeys", [](nb::object const &) { return nb::none(); });
    cls.def("getBit", &Key<Flag>::getBit);
    cls.def("__getstate__",
            [](Key<Flag> const &self) {
                /* Return a tuple that fully encodes the state of the object */
                return nb::make_tuple(self.getOffset(), self.getBit());
            });
    cls.def("__setstate__",
            [](Key<Flag> *key, nb::tuple t) {
                int const NPARAMS = 2;
                if (t.size() != NPARAMS) {
                    std::ostringstream os;
                    os << "Invalid number of parameters (" << t.size() << ") when unpickling; expected "
                       << NPARAMS;
                    throw std::runtime_error(os.str());
                }
                detail::Access::makeKey(key, nb::cast<int>(t[0]), nb::cast<int>(t[1]));
            });
}

template <typename T>
void declareKeySpecializations(PyKey<Array<T>> &cls) {
    declareKeyAccessors(cls);
    cls.def("isVariableLength", [](Key<Array<T>> &self) -> bool {return self.isVariableLength();});
    cls.def("getSize", [](Key<Array<T>> &self) -> size_t {return self.getSize();});
    cls.def_prop_ro("subfields", [](Key<Array<T>> const &self) -> nb::object {
        nb::list result;
        for (std::size_t i = 0; i < self.getSize(); ++i) {
            result.append(nb::cast(i));
        }
        return nb::tuple(result);
    });
    cls.def_prop_ro("subkeys", [](Key<Array<T>> const &self) -> nb::object {
        nb::list result;
        for (std::size_t i = 0; i < self.getSize(); ++i) {
            result.append(nb::cast(self[i]));
        }
        return nb::tuple(result);
    });
    cls.def("__getstate__",
            [](Key<Array<T>> const &self) {
                /* Return a tuple that fully encodes the state of the object */
                return nb::make_tuple(self.getOffset(), self.getElementCount());
            });
    cls.def("__setstate__",
            [](Key<Array<T>> *key, nb::tuple t) {
                int const NPARAMS = 2;
                if (t.size() != NPARAMS) {
                    std::ostringstream os;
                    os << "Invalid number of parameters (" << t.size() << ") when unpickling; expected "
                       << NPARAMS;
                    throw std::runtime_error(os.str());
                }
                detail::Access::makeKeyArray<T>(key, nb::cast<int>(t[0]), nb::cast<int>(t[1]));
            });
}

void declareKeySpecializations(PyKey<std::string> &cls) {
    declareKeyAccessors(cls);
    cls.def("isVariableLength", [](Key<std::string> &self) -> bool {return self.isVariableLength();});
    cls.def("getSize", [](Key<std::string> &self) -> size_t {return self.getSize();});
    cls.def_prop_ro("subfields", [](nb::object const &) { return nb::none(); });
    cls.def_prop_ro("subkeys", [](nb::object const &) { return nb::none(); });
    cls.def("__getstate__",
            [](Key<std::string> const &self) {
                /* Return a tuple that fully encodes the state of the object */
                return nb::make_tuple(self.getOffset(), self.getElementCount());
            });
    cls.def("__setstate__",
            [](Key<std::string>  *key, nb::tuple t) {
                int const NPARAMS = 2;
                if (t.size() != NPARAMS) {
                    std::ostringstream os;
                    os << "Invalid number of parameters (" << t.size() << ") when unpickling; expected "
                       << NPARAMS;
                    throw std::runtime_error(os.str());
                }
                detail::Access::makeKeyString(key,nb::cast<int>(t[0]), nb::cast<int>(t[1]));
            });
}

// Wrap all helper classes (FieldBase, KeyBase, Key, Field, SchemaItem) declarefor a Schema field type.
template <typename T>
void declareSchemaType(WrapperCollection &wrappers) {
    std::string suffix = FieldBase<T>::getTypeString();
    nb::str pySuffix(suffix.c_str());

    nb::object astropyUnit = nb::module_::import_("astropy.units").attr("Unit");

    // FieldBase
    wrappers.wrapType(PyFieldBase<T>(wrappers.module, ("FieldBase" + suffix).c_str()),
                      [](auto &mod, auto &cls) {
                          cls.def_static("getTypeString", &FieldBase<T>::getTypeString);
                          declareFieldBaseSpecializations(cls);
                      });

    // KeyBase
    wrappers.wrapType(PyKeyBase<T>(wrappers.module, ("KeyBase" + suffix).c_str()), [](auto &mod, auto &cls) {
        declareKeyBaseSpecializations(cls);
    });

    // Field
    wrappers.wrapType(PyField<T>(wrappers.module, ("Field" + suffix).c_str()), [pySuffix, astropyUnit](
                                                                                       auto &mod, auto &cls) {
        declareFieldSpecializations(cls);

        mod.attr("_Field")[pySuffix] = cls;

	cls.def("__init__", [astropyUnit](Field<T> *field, // capture by value to refcount in Python instead of dangle in C++
                                 std::string const &name, std::string const &doc, std::string const &units, 
				 std::string const &parse_strict) {
                  astropyUnit(units, "parse_strict"_a = parse_strict);
                 
                        new (field) Field<T>(name, doc, units);
                   
                },
                "name"_a, "doc"_a = "", "units"_a = "", "parse_strict"_a = "raise");
        cls.def("_addTo", [](Field<T> const &self, Schema &schema, bool doReplace) -> Key<T> {
            return schema.addField(self, doReplace);
        });
        cls.def("__init__", [astropyUnit](Field<T> *field, // capture by value to refcount in Python instead of dangle in C++
                                 std::string const &name, std::string const &doc, std::string const &units,
                                 int size, std::string const &parse_strict) {
                    astropyUnit(units, "parse_strict"_a = parse_strict);
                        new (field) Field<T>(name, doc, units, size);
                },
                "name"_a, "doc"_a = "", "units"_a = "", "size"_a, "parse_strict"_a = "raise");
        cls.def("_addTo", [](Field<T> const &self, Schema &schema, bool doReplace) -> Key<T> {
            return schema.addField(self, doReplace);
        });
        cls.def("getName", &Field<T>::getName);
        cls.def("getDoc", &Field<T>::getDoc);
        cls.def("getUnits", &Field<T>::getUnits);
        cls.def("copyRenamed", &Field<T>::copyRenamed);
        cpputils::python::addOutputOp(cls, "__str__");
        cpputils::python::addOutputOp(cls, "__repr__");
    });

    // Key
    wrappers.wrapType(PyKey<T>(wrappers.module, ("Key" + suffix).c_str()), [pySuffix](auto &mod, auto &cls) {
        mod.attr("_Key")[pySuffix] = cls;
        cls.def(nb::init<>());
        cls.def_static("getTypeString", &Key<T>::getTypeString);
        cls.def("__eq__", [](Key<T> const &self, Key<T> const &other) -> bool { return self == other; },
                nb::is_operator());
        cpputils::python::addHash(cls);
        cls.def("__ne__", [](Key<T> const &self, Key<T> const &other) -> bool { return self != other; },
                nb::is_operator());
        cls.def("isValid", &Key<T>::isValid);
        cls.def("getOffset", &Key<T>::getOffset);
        cpputils::python::addOutputOp(cls, "__str__");
        cpputils::python::addOutputOp(cls, "__repr__");
        // The Key methods below actually wrap templated methods on Schema and
        // SchemaMapper.  Rather than doing many-type overload resolution by
        // wrapping those methods directly, we use the visitor pattern by having
        // the wrappers for those methods delegate back to these non-templated
        // methods on the templated Key classes.
        cls.def("_findIn", [](Key<T> const &self, Schema const &schema) { return schema.find(self); });
        cls.def("_addMappingTo", [](Key<T> const &self, SchemaMapper &mapper, Field<T> const &field,
                                    bool doReplace) { return mapper.addMapping(self, field, doReplace); });
        cls.def("_addMappingTo", [](Key<T> const &self, SchemaMapper &mapper, std::string const &name,
                                    bool doReplace) { return mapper.addMapping(self, name, doReplace); });
        cls.def("_addMappingTo", [](Key<T> const &self, SchemaMapper &mapper,
                                    bool doReplace) { return mapper.addMapping(self, doReplace); });
        declareKeySpecializations(cls);
    });

    // SchemaItem
    wrappers.wrapType(PySchemaItem<T>(wrappers.module, ("SchemaItem" + suffix).c_str()),
                      [pySuffix](auto &mod, auto &cls) {
                          mod.attr("_SchemaItem")[pySuffix] = cls;
                          cls.def_ro("key", &SchemaItem<T>::key);
                          cls.def_ro("field", &SchemaItem<T>::field);
                          cls.def("getKey", [](SchemaItem<T> const &self) { return self.key; });
                          cls.def("getField", [](SchemaItem<T> const &self) { return self.field; });
                          cls.def("__getitem__", [](nb::object const &self, int index) -> nb::object {
                              if (index == 0) {
                                  return self.attr("key");
                              } else if (index == 1) {
                                  return self.attr("field");
                              }
                              // Have to raise IndexError not some LSST exception to get the
                              // right behavior when unpacking.
                              throw nb::index_error("Index to SchemaItem must be 0 or 1.");
                          });
                          cls.def("__len__", [](nb::object const &self) -> int { return 2; });
                          cls.def("__str__",
                                  [](nb::object const &self) -> nb::str { return nb::str(nb::tuple(self)); });
                          cls.def("__repr__", [](nb::object const &self) -> nb::str {
                              return nb::str("SchemaItem(key={0.key}, field={0.field})").format(self);
                          });
                          cls.def("__getstate__",
                                  [](SchemaItem<T> const &self) {
                                      /* Return a tuple that fully encodes the state of the object */
                                      return nb::make_tuple(self.key, self.field);
                                  });
                          cls.def("__setstate__",
                                  [](SchemaItem<T> *schema, nb::tuple t) {
                                      int const NPARAMS = 2;
                                      if (t.size() != NPARAMS) {
                                          std::ostringstream os;
                                          os << "Invalid number of parameters (" << t.size()
                                             << ") when unpickling; expected " << NPARAMS;
                                          throw std::runtime_error(os.str());
                                      }
                                      new (schema)  SchemaItem<T>(nb::cast<Key<T>>(t[0]), nb::cast<Field<T>>(t[1]));
                                  });
                      });
}

// Helper class for Schema::find(name, func) that converts the result to Python.
// In C++14, this should be converted to a universal lambda.
struct MakePythonSchemaItem {
    template <typename T>
    void operator()(SchemaItem<T> const &item) {
        result = nb::cast(item);
    }

    nb::object result;
};

void declareSchema(WrapperCollection &wrappers) {
    wrappers.wrapType(PySchema(wrappers.module, "Schema"), [](auto &mod, auto &cls) {
        // wrap ComparisonFlags values as ints since we use them as bitflags,
        // not true enums
        cls.attr("EQUAL_KEYS") = nb::cast(int(Schema::EQUAL_KEYS));
        cls.attr("EQUAL_NAMES") = nb::cast(int(Schema::EQUAL_NAMES));
        cls.attr("EQUAL_DOCS") = nb::cast(int(Schema::EQUAL_DOCS));
        cls.attr("EQUAL_UNITS") = nb::cast(int(Schema::EQUAL_UNITS));
        cls.attr("EQUAL_FIELDS") = nb::cast(int(Schema::EQUAL_FIELDS));
        cls.attr("EQUAL_ALIASES") = nb::cast(int(Schema::EQUAL_ALIASES));
        cls.attr("IDENTICAL") = nb::cast(int(Schema::IDENTICAL));

        cls.attr("VERSION") = nb::cast(int(Schema::VERSION));

        cls.def(nb::init<>());
        cls.def(nb::init<Schema const &>());
        cls.def("__getitem__", [](Schema &self, std::string const &name) { return self[name]; });
        cls.def("__eq__", [](Schema const &self, Schema const &other) { return self == other; },
                nb::is_operator());
        cls.def("__ne__", [](Schema const &self, Schema const &other) { return self != other; },
                nb::is_operator());
        cls.def("getRecordSize", &Schema::getRecordSize);
        cls.def("getFieldCount", &Schema::getFieldCount);
        cls.def("getFlagFieldCount", &Schema::getFlagFieldCount);
        cls.def("getNonFlagFieldCount", &Schema::getNonFlagFieldCount);
        cls.def("find", [](nb::object const &self, nb::object const &key) -> nb::object {
            try {
                if (nb::isinstance<nb::str>(key) || nb::isinstance<nb::bytes>(key)) {
                    Schema const &s = nb::cast<Schema const &>(self);
                    std::string name = nb::cast<std::string>(key);
                    MakePythonSchemaItem func;
                    s.findAndApply(name, func);
                    return func.result;
                }
                return key.attr("_findIn")(self);
            } catch (pex::exceptions::NotFoundError &err) {
                // Avoid API change by re-throwing as KeyError.
                PyErr_SetString(PyExc_KeyError, err.what());
                throw nb::python_error();
            }
        });
        cls.def("getNames", &Schema::getNames, "topOnly"_a = false);
        cls.def("getAliasMap", &Schema::getAliasMap);
        cls.def("setAliasMap", &Schema::setAliasMap, "aliases"_a = nb::none());
        cls.def("disconnectAliases", &Schema::disconnectAliases);
        cls.def("forEach", [](Schema &self, nb::object &obj) { self.forEach(obj); });
        cls.def("compare", &Schema::compare, "other"_a, "flags"_a = int(Schema::EQUAL_KEYS));
        cls.def("contains", (int (Schema::*)(Schema const &, int) const) & Schema::contains, "other"_a,
                "flags"_a = int(Schema::EQUAL_KEYS));
        cls.def("__contains__", [](nb::object const &self, nb::object const &key) {
            try {
                self.attr("find")(key);
            } catch (nb::python_error &err) {
                err.restore();
                PyErr_Clear();
                return false;
            }
            return true;
        });
        cls.def_static("readFits", (Schema(*)(std::string const &, int)) & Schema::readFits, "filename"_a,
                       "hdu"_a = fits::DEFAULT_HDU);
        cls.def_static("readFits", (Schema(*)(fits::MemFileManager &, int)) & Schema::readFits, "manager"_a,
                       "hdu"_a = fits::DEFAULT_HDU);

        cls.def("join",
                (std::string(Schema::*)(std::string const &, std::string const &) const) & Schema::join,
                "a"_a, "b"_a);
        cls.def("join",
                (std::string(Schema::*)(std::string const &, std::string const &, std::string const &)
                         const) &
                        Schema::join,
                "a"_a, "b"_a, "c"_a);
        cls.def("join",
                (std::string(Schema::*)(std::string const &, std::string const &, std::string const &,
                                        std::string const &) const) &
                        Schema::join,
                "a"_a, "b"_a, "c"_a, "d"_a);
        cpputils::python::addOutputOp(cls, "__str__");
        cpputils::python::addOutputOp(cls, "__repr__");
    });
}

void declareSubSchema(WrapperCollection &wrappers) {
    wrappers.wrapType(PySubSchema(wrappers.module, "SubSchema"), [](auto &mod, auto &cls) {
        cls.def("getNames", &SubSchema::getNames, "topOnly"_a = false);
        cls.def("getPrefix", &SubSchema::getPrefix);
        cls.def("asKey", [](SubSchema const &self) -> nb::object {
            MakePythonSchemaItem func;
            self.apply(func);
            return func.result.attr("key");
        });
        cls.def("asField", [](SubSchema const &self) -> nb::object {
            MakePythonSchemaItem func;
            self.apply(func);
            return func.result.attr("field");
        });
        cls.def("find", [](SubSchema const &self, std::string const &name) -> nb::object {
            MakePythonSchemaItem func;
            self.findAndApply(name, func);
            return func.result;
        });
        cls.def("__getitem__", [](SubSchema &self, std::string const &name) { return self[name]; });
    });
}

}  // namespace

void wrapSchema(WrapperCollection &wrappers) {
    // We'll add instantiations of Field, Key, and SchemaItem to these private
    // dicts, and then in schemaContinued.py we'll add them to a TemplateMeta
    // ABC.
    auto &mod = wrappers.module;
    mod.attr("_Field") = nb::dict();
    mod.attr("_Key") = nb::dict();
    mod.attr("_SchemaItem") = nb::dict();

    declareSchemaType<std::uint8_t>(wrappers);
    declareSchemaType<std::uint16_t>(wrappers);
    declareSchemaType<std::int32_t>(wrappers);
    declareSchemaType<std::int64_t>(wrappers);
    declareSchemaType<float>(wrappers);
    declareSchemaType<double>(wrappers);
    declareSchemaType<std::string>(wrappers);
    declareSchemaType<lsst::geom::Angle>(wrappers);
    declareSchemaType<Array<std::uint8_t>>(wrappers);
    declareSchemaType<Array<std::uint16_t>>(wrappers);
    declareSchemaType<Array<int>>(wrappers);
    declareSchemaType<Array<float>>(wrappers);
    declareSchemaType<Array<double>>(wrappers);
    declareSchemaType<Flag>(wrappers);

    declareSchema(wrappers);
    declareSubSchema(wrappers);
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
