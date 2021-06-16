// -*- lsst-c++ -*-

#include <iostream>

#include "lsst/pex/exceptions.h"
#include "lsst/afw/table/BaseRecord.h"
#include "lsst/afw/table/SchemaMapper.h"

namespace lsst {
namespace afw {
namespace table {

namespace {

// A Schema Functor used to set floating point-fields to NaN and initialize variable-length arrays
// using placement new.  All other fields are left alone, as they should already be zero.
struct RecordInitializer {
    template <typename T>
    static void fill(T *element, int size) {}  // this matches all non-floating-point-element fields.

    static void fill(float *element, int size) {
        std::fill(element, element + size, std::numeric_limits<float>::quiet_NaN());
    }

    static void fill(double *element, int size) {
        std::fill(element, element + size, std::numeric_limits<double>::quiet_NaN());
    }

    static void fill(lsst::geom::Angle *element, int size) {
        fill(reinterpret_cast<double *>(element), size);
    }

    template <typename T>
    void operator()(SchemaItem<T> const &item) const {
        fill(reinterpret_cast<typename Field<T>::Element *>(data + item.key.getOffset()),
             item.key.getElementCount());
    }

    template <typename T>
    void operator()(SchemaItem<Array<T> > const &item) const {
        if (item.key.isVariableLength()) {
            // Use placement new because the memory (for one ndarray) is already allocated
            new (data + item.key.getOffset()) ndarray::Array<T, 1, 1>();
        } else {
            fill(reinterpret_cast<typename Field<T>::Element *>(data + item.key.getOffset()),
                 item.key.getElementCount());
        }
    }

    void operator()(SchemaItem<std::string> const &item) const {
        if (item.key.isVariableLength()) {
            // Use placement new because the memory (for one std::string) is already allocated
            new (reinterpret_cast<std::string *>(data + item.key.getOffset())) std::string();
        } else {
            fill(reinterpret_cast<char *>(data + item.key.getOffset()), item.key.getElementCount());
        }
    }

    void operator()(SchemaItem<Flag> const &item) const {}  // do nothing for Flag fields; already 0

    char *data;
};

// A Schema::forEach and SchemaMapper::forEach functor that copies data from one record to another.
struct CopyValue {
    template <typename U>
    void operator()(Key<U> const& inputKey, Key<U> const& outputKey) const {
        typename Field<U>::Element const* inputElem = _inputRecord->getElement(inputKey);
        std::copy(inputElem, inputElem + inputKey.getElementCount(), _outputRecord->getElement(outputKey));
    }

    template <typename U>
    void operator()(Key<Array<U> > const& inputKey, Key<Array<U> > const& outputKey) const {
        if (inputKey.isVariableLength() != outputKey.isVariableLength()) {
            throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterError,
                              "At least one input array field is variable-length"
                              " and the correponding output is not, or vice-versa");
        }
        if (inputKey.isVariableLength()) {
            ndarray::Array<U, 1, 1> value = ndarray::copy(_inputRecord->get(inputKey));
            _outputRecord->set(outputKey, value);
            return;
        }
        typename Field<U>::Element const* inputElem = _inputRecord->getElement(inputKey);
        std::copy(inputElem, inputElem + inputKey.getElementCount(), _outputRecord->getElement(outputKey));
    }

    void operator()(Key<std::string> const& inputKey, Key<std::string> const& outputKey) const {
        if (inputKey.isVariableLength() != outputKey.isVariableLength()) {
            throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterError,
                              "At least one input string field is variable-length "
                              "and the correponding output is not, or vice-versa");
        }
        if (inputKey.isVariableLength()) {
            std::string value = _inputRecord->get(inputKey);
            _outputRecord->set(outputKey, value);
            return;
        }
        char const* inputElem = _inputRecord->getElement(inputKey);
        std::copy(inputElem, inputElem + inputKey.getElementCount(), _outputRecord->getElement(outputKey));
    }

    void operator()(Key<Flag> const& inputKey, Key<Flag> const& outputKey) const {
        _outputRecord->set(outputKey, _inputRecord->get(inputKey));
    }

    template <typename U>
    void operator()(SchemaItem<U> const& item) const {
        (*this)(item.key, item.key);
    }

    CopyValue(BaseRecord const* inputRecord, BaseRecord* outputRecord)
            : _inputRecord(inputRecord), _outputRecord(outputRecord) {}

private:
    BaseRecord const* _inputRecord;
    BaseRecord* _outputRecord;
};

}  // namespace

void BaseRecord::assign(BaseRecord const& other) {
    if (this->getSchema() != other.getSchema()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LogicError, "Unequal schemas in record assignment.");
    }
    this->getSchema().forEach(CopyValue(&other, this));
    this->_assign(other);  // let derived classes assign their own stuff
}

void BaseRecord::assign(BaseRecord const& other, SchemaMapper const& mapper) {
    if (!other.getSchema().contains(mapper.getInputSchema())) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LogicError,
                          "Unequal schemas between input record and mapper.");
    }
    if (!this->getSchema().contains(mapper.getOutputSchema())) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LogicError,
                          "Unequal schemas between output record and mapper.");
    }
    mapper.forEach(CopyValue(&other, this));  // use the functor we defined above
    this->_assign(other);                     // let derived classes assign their own stuff
}

BaseRecord::BaseRecord(ConstructionToken const &, detail::RecordData && data) :
    _data(std::move(data.data)),
    _table(std::move(data.table)),
    _manager(std::move(data.manager))
{
    RecordInitializer f = {reinterpret_cast<char *>(_data)};
    _table->getSchema().forEach(f);
}

void BaseRecord::_stream(std::ostream& os) const {
    getSchema().forEach([&os, this](auto const& item) {
        os << item.field.getName() << ": " << this->get(item.key) << std::endl;
    });
}

std::ostream& operator<<(std::ostream& os, BaseRecord const& record) {
    record._stream(os);
    return os;
}

}  // namespace table
}  // namespace afw
}  // namespace lsst
