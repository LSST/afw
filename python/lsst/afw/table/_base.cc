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

/*
Unlike most nanobind wrapper classes, which have one .cc file per header file,
this module wraps both BaseRecord.h and BaseTable.h (as well as CatalogT<BaseRecord> from Catalog.h).

This allows us to define BaseCatalog.Table = clsBaseTable, which is needed to support `cast` in Python,
and makes wrapping Base catalogs more similar to all other types of catalog.
*/

#include "nanobind/nanobind.h"

#include "ndarray/nanobind.h"

#include "lsst/cpputils/python.h"

#include "lsst/afw/table/Flag.h"
#include "lsst/afw/table/SchemaMapper.h"
#include "lsst/afw/table/BaseColumnView.h"
#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/BaseTable.h"
#include "lsst/afw/table/python/catalog.h"
#include "lsst/afw/table/python/columnView.h"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace lsst {
namespace afw {
namespace table {

using cpputils::python::WrapperCollection;

namespace {

using PyBaseRecord = nb::class_<BaseRecord>;
using PyBaseTable = nb::class_<BaseTable>;

template <typename T>
void declareBaseRecordOverloads(PyBaseRecord &cls, std::string const &suffix) {
    using Getter = typename Field<T>::Value (BaseRecord::*)(const Key<T> &) const;
    using Setter = void (BaseRecord::*)(const Key<T> &, const typename Field<T>::Value &);
    cls.def(("get" + suffix).c_str(), (Getter)&BaseRecord::get);
    cls.def(("set" + suffix).c_str(), (Setter)&BaseRecord::set);
}

template <typename T>
void declareBaseRecordArrayOverloads(PyBaseRecord &cls, std::string const &suffix) {
    auto getter = [](BaseRecord &self, Key<Array<T>> const &key) -> ndarray::Array<T, 1, 1> {
        return self[key];
    };
    auto setter = [](BaseRecord &self, Key<Array<T>> const &key, nb::object const &value) {
        if (key.getSize() == 0) {
            // Variable-length array field: do a shallow copy, which requires a non-const
            // contiguous array.
            self.set(key, nb::cast<ndarray::Array<T, 1, 1>>(value));
        } else {
            // Fixed-length array field: do a deep copy, which can work with a const
            // noncontiguous array.  But we need to check the size first, since the
            // penalty for getting that wrong is assert->abort.
            auto v = nb::cast<ndarray::Array<T const, 1, 0>>(value);
            ndarray::ArrayRef<T, 1, 1> ref = self[key];
            if (v.size() != ref.size()) {
                throw LSST_EXCEPT(
                        pex::exceptions::LengthError,
                        (boost::format("Array sizes do not agree: %s != %s") % v.size() % ref.size()).str());
            }
            ref = v;
        }
        return;
    };
    cls.def(("get" + suffix).c_str(), getter);
    cls.def(("set" + suffix).c_str(), setter);
}

PyBaseRecord declareBaseRecord(WrapperCollection &wrappers) {
    return wrappers.wrapType(PyBaseRecord(wrappers.module, "BaseRecord"), [](auto &mod, auto &cls) {
        cpputils::python::addSharedPtrEquality<BaseRecord>(cls);
        cls.def("assign", (void (BaseRecord::*)(BaseRecord const &)) & BaseRecord::assign);
        cls.def("assign",
                (void (BaseRecord::*)(BaseRecord const &, SchemaMapper const &)) & BaseRecord::assign);
        cls.def("getSchema", &BaseRecord::getSchema);
        cls.def("getTable", &BaseRecord::getTable);
        cls.def_prop_ro("schema", &BaseRecord::getSchema);
        cls.def_prop_ro("table", &BaseRecord::getTable);

        declareBaseRecordOverloads<double>(cls, "D");
        declareBaseRecordOverloads<float>(cls, "F");
        declareBaseRecordOverloads<lsst::afw::table::Flag>(cls, "Flag");
        declareBaseRecordOverloads<std::uint8_t>(cls, "B");
        declareBaseRecordOverloads<std::uint16_t>(cls, "U");
        declareBaseRecordOverloads<std::int32_t>(cls, "I");
        declareBaseRecordOverloads<std::int64_t>(cls, "L");
        declareBaseRecordOverloads<std::string>(cls, "String");
        declareBaseRecordOverloads<lsst::geom::Angle>(cls, "Angle");
        declareBaseRecordArrayOverloads<std::uint8_t>(cls, "ArrayB");
        declareBaseRecordArrayOverloads<std::uint16_t>(cls, "ArrayU");
        declareBaseRecordArrayOverloads<int>(cls, "ArrayI");
        declareBaseRecordArrayOverloads<float>(cls, "ArrayF");
        declareBaseRecordArrayOverloads<double>(cls, "ArrayD");
        cpputils::python::addOutputOp(cls, "__str__");  // __repr__ is defined in baseContinued.py

        // These are master getters and setters that can take either strings, Keys, or
        // FunctorKeys, and dispatch to key.get.
        auto getter = [](nb::object const &self, nb::object key) -> nb::object {
            nb::object schema = self.attr("schema");
            if (nb::isinstance<nb::str>(key) || nb::isinstance<nb::bytes>(key)) {
                key = schema.attr("find")(key).attr("key");
            }
            return key.attr("get")(self);
        };
        auto setter = [](nb::object const &self, nb::object key, nb::object const &value) -> void {
            nb::object schema = self.attr("schema");
            if (nb::isinstance<nb::str>(key) || nb::isinstance<nb::bytes>(key)) {
                key = schema.attr("find")(key).attr("key");
            }
            key.attr("set")(self, value);
        };

        // The distinction between get/set and operator[] is meaningful in C++, because "record[k] = v"
        // operates by returning an object that can be assigned to.
        // But there's no meaningful difference between get/set and __getitem__/__setitem__.
        cls.def("get", getter);
        cls.def("__getitem__", getter, nb::rv_policy::reference);
        cls.def("set", setter);
        cls.def("__setitem__", setter);
    });
}

PyBaseTable declareBaseTable(WrapperCollection &wrappers) {
    return wrappers.wrapType(PyBaseTable(wrappers.module, "BaseTable"), [](auto &mod, auto &cls) {
        cpputils::python::addSharedPtrEquality<BaseTable>(cls);
        cls.def_static("make", &BaseTable::make);
        cls.def("getMetadata", &BaseTable::getMetadata);
        cls.def("setMetadata", &BaseTable::setMetadata, "metadata"_a);
        cls.def("popMetadata", &BaseTable::popMetadata);
        cls.def_prop_rw("metadata", &BaseTable::getMetadata, &BaseTable::setMetadata);
        cls.def("makeRecord", &BaseTable::makeRecord);
        cls.def("copyRecord",
                (std::shared_ptr<BaseRecord>(BaseTable::*)(BaseRecord const &)) & BaseTable::copyRecord);
        cls.def("copyRecord",
                (std::shared_ptr<BaseRecord>(BaseTable::*)(BaseRecord const &, SchemaMapper const &)) &
                        BaseTable::copyRecord);
        cls.def("getSchema", &BaseTable::getSchema);
        cls.def_prop_ro("schema", &BaseTable::getSchema);
        cls.def("getBufferSize", &BaseTable::getBufferSize);
        cls.def("clone", &BaseTable::clone);
        cls.def("preallocate", &BaseTable::preallocate);
    });
}

}  // namespace

void wrapBase(WrapperCollection &wrappers) {
    wrappers.addSignatureDependency("lsst.daf.base");

    auto clsBaseTable = declareBaseTable(wrappers);
    auto clsBaseRecord = declareBaseRecord(wrappers);
    auto clsBaseCatalog = table::python::declareCatalog<BaseRecord>(wrappers, "Base");
    auto clsBaseColumnView = table::python::declareColumnView<BaseRecord>(wrappers, "Base");

    clsBaseRecord.attr("Table") = clsBaseTable;
    clsBaseRecord.attr("ColumnView") = clsBaseColumnView;
    clsBaseRecord.attr("Catalog") = clsBaseCatalog;
    clsBaseTable.attr("Record") = clsBaseRecord;
    clsBaseTable.attr("ColumnView") = clsBaseColumnView;
    clsBaseTable.attr("Catalog") = clsBaseCatalog;
    clsBaseCatalog.attr("Record") = clsBaseRecord;
    clsBaseCatalog.attr("Table") = clsBaseTable;
    clsBaseCatalog.attr("ColumnView") = clsBaseColumnView;
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
