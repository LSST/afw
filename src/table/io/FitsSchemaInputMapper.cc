// -*- lsst-c++ -*-

#include <cstdio>

#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/cstdint.hpp"
#include "boost/multi_index_container.hpp"
#include "boost/multi_index/sequenced_index.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/hashed_index.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/math/special_functions/round.hpp"

#include "lsst/pex/logging.h"
#include "lsst/afw/table/io/FitsSchemaInputMapper.h"

namespace lsst { namespace afw { namespace table { namespace io {

namespace {

// A quirk of Boost.MultiIndex (which we use for our container of FitsSchemaItems)
// that you have to use a special functor (like this one) to set data members
// in a container with set indices (because setting those values might require
// the element to be moved to a different place in the set).  Check out
// the Boost.MultiIndex docs for more information.
template <std::string FitsSchemaItem::*Member>
struct SetFitsSchemaString {
    void operator()(FitsSchemaItem & item) {
        item.*Member = _v;
    }
    explicit SetFitsSchemaString(std::string const & v) : _v(v) {}
private:
    std::string const & _v;
};

} // anonymous

class FitsSchemaInputMapper::Impl {
public:

    // A container class (based on Boost.MultiIndex) that provides three sort orders,
    // on column number, flag bit, and name (ttype).  This allows us to insert fields into the
    // schema in the correct order, regardless of which order they appear in the
    // FITS header.
    typedef boost::multi_index_container<
        FitsSchemaItem,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::member<FitsSchemaItem,int,&FitsSchemaItem::column>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::member<FitsSchemaItem,int,&FitsSchemaItem::bit>
            >,
            boost::multi_index::hashed_unique<
                boost::multi_index::member<FitsSchemaItem,std::string,&FitsSchemaItem::ttype>
            >,
            boost::multi_index::sequenced<>
        >
    > InputContainer;

    // Typedefs for the special functors used to set data members.
    typedef SetFitsSchemaString<&FitsSchemaItem::ttype> SetTTYPE;
    typedef SetFitsSchemaString<&FitsSchemaItem::tform> SetTFORM;
    typedef SetFitsSchemaString<&FitsSchemaItem::tccls> SetTCCLS;
    typedef SetFitsSchemaString<&FitsSchemaItem::tunit> SetTUNIT;
    typedef SetFitsSchemaString<&FitsSchemaItem::doc> SetDoc;

    // Typedefs for the different indices.
    typedef InputContainer::nth_index<0>::type ByColumn;
    typedef InputContainer::nth_index<1>::type ByBit;
    typedef InputContainer::nth_index<2>::type ByName;
    typedef InputContainer::nth_index<3>::type AsList;

    // Getters for the different indices.
    ByColumn & byColumn() { return inputs.get<0>(); }
    ByBit & byBit() { return inputs.get<1>(); }
    ByName & byName() { return inputs.get<2>(); }
    AsList & asList() { return inputs.get<3>(); }

    Impl() : version(0), flagColumn(0), archiveHdu(-1) {}

    int version;
    int flagColumn;
    int archiveHdu;
    Schema schema;
    std::vector<std::unique_ptr<FitsColumnReader>> readers;
    std::vector<Key<Flag>> flagKeys;
    std::unique_ptr<bool[]> flagWorkspace;
    PTR(io::InputArchive) archive;
    InputContainer inputs;
};

FitsSchemaInputMapper::FitsSchemaInputMapper(daf::base::PropertyList & metadata, bool stripMetadata) :
    _impl(boost::make_shared<Impl>())
{
    // Set the table version.  If AFW_TABLE_VERSION tag exists, use that
    // If not, set to 0 if it has an AFW_TYPE, Schema default otherwise (DM-590)
    if (!metadata.exists("AFW_TYPE")) {
        _impl->version = lsst::afw::table::Schema::VERSION;
    }
    _impl->version = metadata.get("AFW_TABLE_VERSION", _impl->version);
    if (stripMetadata) {
        metadata.remove("AFW_TABLE_VERSION");
    }

    // Find a key that indicates an Archive stored on other HDUs
    _impl->archiveHdu = metadata.get("AR_HDU", -1);
    if (stripMetadata && _impl->archiveHdu > 0) {
        metadata.remove("AR_HDU");
    }

    // read aliases stored in the new, expected way
    try {
        std::vector<std::string> rawAliases = metadata.getArray<std::string>("ALIAS");
        for (std::vector<std::string>::const_iterator i = rawAliases.begin(); i != rawAliases.end(); ++i) {
            std::size_t pos = i->find_first_of(':');
            if (pos == std::string::npos) {
                throw LSST_EXCEPT(
                    afw::fits::FitsError,
                    (boost::format("Malformed alias definition: '%s'") % (*i)).str()
                );
            }
            _impl->schema.getAliasMap()->set(i->substr(0, pos), i->substr(pos+1, std::string::npos));
        }
        if (stripMetadata) {
            metadata.remove("ALIAS");
        }
    } catch (pex::exceptions::NotFoundError &) {
        // if there are no aliases, just move on
    }

    if (_impl->version == 0) {
        // Read slots saved using an old mechanism in as aliases, since the new slot mechanism delegates
        // slot definition to the AliasMap.
        static std::array<std::pair<std::string,std::string>,6> oldSlotKeys = {
            std::make_pair("PSF_FLUX", "slot_PsfFlux"),
            std::make_pair("AP_FLUX", "slot_ApFlux"),
            std::make_pair("INST_FLUX", "slot_InstFlux"),
            std::make_pair("MODEL_FLUX", "slot_ModelFlux"),
            std::make_pair("CENTROID", "slot_Centroid"),
            std::make_pair("SHAPE", "slot_Shape")
        };
        for (std::size_t i = 0; i < oldSlotKeys.size(); ++i) {
            std::string target = metadata.get(oldSlotKeys[i].first + "_SLOT", std::string(""));
            std::replace(target.begin(), target.end(), '_', '.');
            if (!target.empty()) {
                _impl->schema.getAliasMap()->set(oldSlotKeys[i].second, target);
                if (stripMetadata) {
                    metadata.remove(oldSlotKeys[i].first);
                    metadata.remove(oldSlotKeys[i].first + "_ERR_SLOT");
                    metadata.remove(oldSlotKeys[i].first + "_FLAG_SLOT");
                }
            }
        }
    }

    // Read the rest of the header into the intermediate inputs container.
    std::vector<std::string> keyList = metadata.getOrderedNames();
    for (std::vector<std::string>::const_iterator key = keyList.begin(); key != keyList.end(); ++key) {
        if (key->compare(0, 5, "TTYPE") == 0) {
            int column = boost::lexical_cast<int>(key->substr(5)) - 1;
            auto iter = _impl->byColumn().lower_bound(column);
            if (iter == _impl->byColumn().end() || iter->column != column) {
                iter = _impl->byColumn().insert(iter, FitsSchemaItem(column, -1));

            }
            std::string v = metadata.get<std::string>(*key);
            _impl->byColumn().modify(iter, Impl::SetTTYPE(v));
            if (iter->doc.empty()) { // don't overwrite if already set with TDOCn
                _impl->byColumn().modify(iter, Impl::SetDoc(metadata.getComment(*key)));
            }
            if (stripMetadata) {
                metadata.remove(*key);
            }
        } else if (key->compare(0, 5, "TFLAG") == 0) {
            int bit = boost::lexical_cast<int>(key->substr(5)) - 1;
            auto iter = _impl->byBit().lower_bound(bit);
            if (iter == _impl->byBit().end() || iter->bit != bit) {
                iter = _impl->byBit().insert(iter, FitsSchemaItem(-1, bit));
            }
            std::string v = metadata.get<std::string>(*key);
            _impl->byBit().modify(iter, Impl::SetTTYPE(v));
            if (iter->doc.empty())  { // don't overwrite if already set with TFDOCn
                _impl->byBit().modify(iter, Impl::SetDoc(metadata.getComment(*key)));
            }
            if (stripMetadata) {
                metadata.remove(*key);
            }
        } else if (key->compare(0, 4, "TDOC") == 0) {
            int column = boost::lexical_cast<int>(key->substr(4)) - 1;
            auto iter = _impl->byColumn().lower_bound(column);
            if (iter == _impl->byColumn().end() || iter->column != column) {
                iter = _impl->byColumn().insert(iter, FitsSchemaItem(column, -1));
            }
            _impl->byColumn().modify(iter, Impl::SetDoc(metadata.get<std::string>(*key)));
            if (stripMetadata) {
                metadata.remove(*key);
            }
        } else if (key->compare(0, 5, "TFDOC") == 0) {
            int bit = boost::lexical_cast<int>(key->substr(5)) - 1;
            auto iter = _impl->byBit().lower_bound(bit);
            if (iter == _impl->byBit().end() || iter->bit != bit) {
                iter = _impl->byBit().insert(iter, FitsSchemaItem(-1, bit));
            }
            _impl->byBit().modify(iter, Impl::SetDoc(metadata.get<std::string>(*key)));
            if (stripMetadata) {
                metadata.remove(*key);
            }
        } else if (key->compare(0, 5, "TUNIT") == 0) {
            int column = boost::lexical_cast<int>(key->substr(5)) - 1;
            auto iter = _impl->byColumn().lower_bound(column);
            if (iter == _impl->byColumn().end() || iter->column != column) {
                iter = _impl->byColumn().insert(iter, FitsSchemaItem(column, -1));
            }
            _impl->byColumn().modify(iter, Impl::SetTUNIT(metadata.get<std::string>(*key)));
            if (stripMetadata) {
                metadata.remove(*key);
            }
        } else if (key->compare(0, 5, "TCCLS") == 0) {
            int column = boost::lexical_cast<int>(key->substr(5)) - 1;
            auto iter = _impl->byColumn().lower_bound(column);
            if (iter == _impl->byColumn().end() || iter->column != column) {
                iter = _impl->byColumn().insert(iter, FitsSchemaItem(column, -1));
            }
            _impl->byColumn().modify(iter, Impl::SetTCCLS(metadata.get<std::string>(*key)));
            if (stripMetadata) {
                metadata.remove(*key);
            }
        } else if (key->compare(0, 5, "TFORM") == 0) {
            int column = boost::lexical_cast<int>(key->substr(5)) - 1;
            auto iter = _impl->byColumn().lower_bound(column);
            if (iter == _impl->byColumn().end() || iter->column != column) {
                iter = _impl->byColumn().insert(iter, FitsSchemaItem(column, -1));
            }
            _impl->byColumn().modify(iter, Impl::SetTFORM(metadata.get<std::string>(*key)));
            if (stripMetadata) {
                metadata.remove(*key);
            }
        } else if (key->compare(0, 5, "TZERO") == 0) {
            if (stripMetadata) {
                metadata.remove(*key);
            }
        } else if (key->compare(0, 5, "TSCAL") == 0) {
            if (stripMetadata) {
                metadata.remove(*key);
            }
        } else if (key->compare(0, 5, "TNULL") == 0) {
            if (stripMetadata) {
                metadata.remove(*key);
            }
        } else if (key->compare(0, 5, "TDISP") == 0) {
            if (stripMetadata) {
                metadata.remove(*key);
            }
        }
    }

    // Find the column used to store flags, and setup the flag-handling data members from it.
    _impl->flagColumn = metadata.get("FLAGCOL", 0);
    if (_impl->flagColumn > 0) {
        if (stripMetadata) {
            metadata.remove("FLAGCOL");
        }
        --_impl->flagColumn; // switch from 1-indexed to 0-indexed
        auto iter = _impl->byColumn().find(_impl->flagColumn);
        if (iter == _impl->byColumn().end()) {
            throw LSST_EXCEPT(
                afw::fits::FitsError,
                (boost::format("Column for flag data not found; FLAGCOL=%d") % _impl->flagColumn).str()
            );
        }
        static boost::regex const regex("(\\d+)?X\\(?(\\d)*\\)?", boost::regex::perl);
        boost::smatch m;
        if (!boost::regex_match(iter->tform, m, regex)) {
            throw LSST_EXCEPT(
                afw::fits::FitsError,
                (boost::format("Invalid TFORM key for flags column: '%s'") % iter->tform).str()
            );
        }
        int nFlags = 1;
        if (m[1].matched) {
            nFlags = boost::lexical_cast<int>(m[1].str());
        }
        _impl->flagKeys.resize(nFlags);
        _impl->flagWorkspace.reset(new bool[nFlags]);
        // Delete the flag column from the input list so we don't interpret it as a
        // regular field.
        _impl->byColumn().erase(iter);
    }

    if (stripMetadata) {
        metadata.remove("AFW_TYPE");
    }
}

void FitsSchemaInputMapper::setArchive(PTR(InputArchive) archive) {
    _impl->archive = archive;
}

bool FitsSchemaInputMapper::readArchive(afw::fits::Fits & fits) {
    int oldHdu = fits.getHdu();
    if (_impl->archiveHdu < 0) _impl->archiveHdu = oldHdu + 1;
    try {
        fits.setHdu(_impl->archiveHdu);
        _impl->archive.reset(new io::InputArchive(InputArchive::readFits(fits)));
        fits.setHdu(oldHdu);
        return true;
    } catch (afw::fits::FitsError &) {
        fits.status = 0;
        fits.setHdu(oldHdu);
        _impl->archiveHdu = -1;
        return false;
    }
}

bool FitsSchemaInputMapper::hasArchive() const { return static_cast<bool>(_impl->archive); }

Schema & FitsSchemaInputMapper::editSchema() {
    return _impl->schema;
}

FitsSchemaItem const * FitsSchemaInputMapper::find(std::string const & ttype) const {
    auto iter = _impl->byName().find(ttype);
    if (iter == _impl->byName().end()) {
        return nullptr;
    }
    return &(*iter);
}

FitsSchemaItem const * FitsSchemaInputMapper::find(int column) const {
    auto iter = _impl->byColumn().lower_bound(column);
    if (iter == _impl->byColumn().end() || iter->column != column) {
        return nullptr;
    }
    return &(*iter);
}

void FitsSchemaInputMapper::erase(Item const * item) {
    auto iter = _impl->byColumn().lower_bound(item->column);
    assert(iter != _impl->byColumn().end() && iter->column == item->column);
    _impl->byColumn().erase(iter);
}

void FitsSchemaInputMapper::erase(std::string const & ttype) {
    auto iter = _impl->byName().find(ttype);
    if (iter != _impl->byName().end() && iter->ttype == ttype) {
        _impl->byName().erase(iter);
    }
}

void FitsSchemaInputMapper::erase(int column) {
    auto iter = _impl->byColumn().lower_bound(column);
    if (iter != _impl->byColumn().end() && iter->column == column) {
        _impl->byColumn().erase(iter);
    }
}

void erase(int column);

void FitsSchemaInputMapper::customize(std::unique_ptr<FitsColumnReader> reader) {
    _impl->readers.push_back(std::move(reader));
}

namespace {

template <typename T>
class StandardReader : public FitsColumnReader {
public:

    static std::unique_ptr<FitsColumnReader> make(
        Schema & schema,
        FitsSchemaItem const & item,
        FieldBase<T> const & base=FieldBase<T>()
    ) {
        return std::unique_ptr<FitsColumnReader>(new StandardReader(schema, item, base));
    }

    StandardReader(Schema & schema, FitsSchemaItem const & item, FieldBase<T> const & base) :
        _column(item.column), _key(schema.addField<T>(item.ttype, item.doc, item.tunit, base))
    {}

    virtual void readCell(
        BaseRecord & record, std::size_t row,
        afw::fits::Fits & fits,
        PTR(InputArchive) const & archive
    ) const {
        fits.readTableArray(row, _column, _key.getElementCount(), record.getElement(_key));
    }

private:
    int _column;
    Key<T> _key;
};

class StringReader : public FitsColumnReader {
public:

    static std::unique_ptr<FitsColumnReader> make(
        Schema & schema,
        FitsSchemaItem const & item,
        int size
    ) {
        return std::unique_ptr<FitsColumnReader>(new StringReader(schema, item, size));
    }

    StringReader(Schema & schema, FitsSchemaItem const & item, int size) :
        _column(item.column), _key(schema.addField<std::string>(item.ttype, item.doc, item.tunit, size))
    {}

    virtual void readCell(
        BaseRecord & record,
        std::size_t row,
        afw::fits::Fits & fits,
        PTR(InputArchive) const & archive
    ) const {
        std::string s;
        fits.readTableScalar(row, _column, s);
        record.set(_key, s);
    }

private:
    int _column;
    Key<std::string> _key;
};

template <typename T>
class VariableLengthArrayReader : public FitsColumnReader {
public:

    static std::unique_ptr<FitsColumnReader> make(
        Schema & schema,
        FitsSchemaItem const & item
    ) {
        return std::unique_ptr<FitsColumnReader>(new VariableLengthArrayReader(schema, item));
    }

    VariableLengthArrayReader(Schema & schema, FitsSchemaItem const & item) :
        _column(item.column), _key(schema.addField<Array<T>>(item.ttype, item.doc, item.tunit, 0))
    {}

    virtual void readCell(
        BaseRecord & record,
        std::size_t row,
        afw::fits::Fits & fits,
        PTR(InputArchive) const & archive
    ) const {
        int size = fits.getTableArraySize(row, _column);
        ndarray::Array<T,1,1> array = ndarray::allocate(size);
        fits.readTableArray(row, _column, size, array.getData());
        record.set(_key, array);
    }

private:
    int _column;
    Key<Array<T>> _key;
};

std::unique_ptr<FitsColumnReader> makeColumnReader(
    Schema & schema,
    FitsSchemaItem const & item
) {
    static boost::regex const regex("(\\d+)?([PQ])?(\\u)\\(?(\\d)*\\)?", boost::regex::perl);
    // start by parsing the format; this tells the element type of the field and the number of elements
    boost::smatch m;
    if (!boost::regex_match(item.tform, m, regex)) {
        return nullptr;
    }
    int size = 1;
    if (m[1].matched) {
        size = boost::lexical_cast<int>(m[1].str());
    }
    char code = m[3].str()[0];
    if (m[2].matched) {
        // P or Q presence indicates a variable-length array, which we can get by just setting the
        // size to zero and letting the rest of the logic run its course.
        size = 0;
    }
    // switch code over FITS codes that correspond to different element types
    switch (code) {
    case 'I': // 16-bit integers - can only be scalars or Arrays (we assume they're unsigned, since
              // that's all we ever write, and CFITSIO will complain later if they aren't)
        if (size == 1) {
            if (item.tccls == "Array") {
                return StandardReader<Array<boost::uint16_t>>::make(schema, item, size);
            }
            return StandardReader<boost::uint16_t>::make(schema, item);
        }
        if (size == 0) {
            return VariableLengthArrayReader<boost::uint16_t>::make(schema, item);
        }
        return StandardReader<Array<boost::uint16_t>>::make(schema, item, size);
    case 'J': // 32-bit integers - can only be scalars, Point fields, or Arrays
        if (size == 0) {
            return VariableLengthArrayReader<boost::int32_t>::make(schema, item);
        }
        if (item.tccls == "Point") {
            return StandardReader<Point<boost::int32_t>>::make(schema, item);
        }
        if (size > 1 || item.tccls == "Array") {
            return StandardReader<Array<boost::int32_t>>::make(schema, item, size);
        }
        return StandardReader<boost::int32_t>::make(schema, item);
    case 'K': // 64-bit integers - can only be scalars.
        if (size == 1) {
            return StandardReader<boost::int64_t>::make(schema, item);
        }
    case 'E': // floats
        if (size == 0) {
            return VariableLengthArrayReader<float>::make(schema, item);
        }
        if (size == 1) {
            if (item.tccls == "Covariance") {
                return StandardReader<Covariance<float>>::make(schema, item, 1);
            }
            if (item.tccls == "Array") {
                return StandardReader<Array<float>>::make(schema, item, 1);
            }
            return StandardReader<float>::make(schema, item);
        }
        if (size == 3 && item.tccls == "Covariance(Point)") {
            return StandardReader<Covariance<Point<float>>>::make(schema, item);
        }
        if (size == 6 && item.tccls == "Covariance(Moments)") {
            return StandardReader<Covariance<Moments<float>>>::make(schema, item);
        }
        if (item.tccls == "Covariance") {
            double v = 0.5 * (std::sqrt(1 + 8 * size) - 1);
            int n = boost::math::iround(v);
            if (n * (n + 1) != size * 2) {
                throw LSST_EXCEPT(
                    afw::fits::FitsError,
                    "Covariance field has invalid size."
                );
            }
            return StandardReader<Covariance<float>>::make(schema, item, size);
        }
        return StandardReader<Array<float>>::make(schema, item, size);
    case 'D': // doubles
        if (size == 0) {
            return VariableLengthArrayReader<double>::make(schema, item);
        }
        if (size == 1) {
            if (item.tccls == "Angle") {
                return StandardReader<Angle>::make(schema, item);
            }
            if (item.tccls == "Array") {
                return StandardReader<Array<double>>::make(schema, item, 1);
            }
            return StandardReader<double>::make(schema, item);
        }
        if (size == 2) {
            if (item.tccls == "Point") {
                return StandardReader<Point<double>>::make(schema, item);
            }
            if (item.tccls == "Coord") {
                return StandardReader<Coord>::make(schema, item);
            }
        }
        if (size ==3 && item.tccls == "Moments") {
            return StandardReader<Moments<double>>::make(schema, item);
        }
        return StandardReader<Array<double>>::make(schema, item, size);
    case 'A': // strings
        return StringReader::make(schema, item, size);
    default:
        return nullptr;
    }
}

} // anonymous

Schema FitsSchemaInputMapper::finalize() {
    for (auto iter = _impl->asList().begin(); iter != _impl->asList().end(); ++iter) {
        if (iter->bit < 0) { // not a Flag column
            std::unique_ptr<FitsColumnReader> reader = makeColumnReader(_impl->schema, *iter);
            if (reader) {
                _impl->readers.push_back(std::move(reader));
            } else {
                pex::logging::Log::getDefaultLog().log(
                    pex::logging::Log::WARN,
                    (boost::format("Format '%s' for column '%s' not supported; skipping.")
                    % iter->tform % iter->ttype).str()
                );
            }
        } else {  // is a Flag column
            if (static_cast<std::size_t>(iter->bit) >= _impl->flagKeys.size()) {
                throw LSST_EXCEPT(
                    afw::fits::FitsError,
                    (boost::format("Flag field '%s' is is in bit %d (0-indexed) of only %d")
                    % iter->ttype % iter->bit % _impl->flagKeys.size()).str()
                );
            }
            _impl->flagKeys[iter->bit] = _impl->schema.addField<Flag>(iter->ttype, iter->doc);
        }
    }
    _impl->asList().clear();
    return _impl->schema;
}

void FitsSchemaInputMapper::readRecord(
    BaseRecord & record,
    afw::fits::Fits & fits,
    std::size_t row
) {
    if (!_impl->flagKeys.empty()) {
        fits.readTableArray<bool>(
            row, _impl->flagColumn, _impl->flagKeys.size(), _impl->flagWorkspace.get()
        );
        for (std::size_t bit = 0; bit < _impl->flagKeys.size(); ++bit) {
            record.set(_impl->flagKeys[bit], _impl->flagWorkspace[bit]);
        }
    }
    for (auto iter = _impl->readers.begin(); iter != _impl->readers.end(); ++iter) {
        (**iter).readCell(record, row, fits, _impl->archive);
    }
}

}}}} // namespace lsst::afw::table::io
