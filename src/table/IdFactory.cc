#include <memory>
#include "boost/format.hpp"

#include "lsst/pex/exceptions.h"
#include "lsst/afw/table/IdFactory.h"

namespace lsst {
namespace afw {
namespace table {

namespace {

class SimpleIdFactory : public IdFactory {
public:
    RecordId operator()() override { return ++_current; }

    void notify(RecordId id) override { _current = id; }

    std::shared_ptr<IdFactory> clone() const override { return std::make_shared<SimpleIdFactory>(*this); }

    SimpleIdFactory()  {}

private:
    RecordId _current{0};
};

class SourceIdFactory : public IdFactory {
public:
    RecordId operator()() override {
        if (++_lower & _upperMask) {
            --_lower;
            throw LSST_EXCEPT(pex::exceptions::LengthError,
                              (boost::format("Next ID '%s' is too large for the number of reserved bits") %
                               (_lower + 1))
                                      .str());
        }
        return _upper | _lower;
    }

    void notify(RecordId id) override {
        RecordId newLower = id & (~_upper);  // chop off the exact exposure ID
        if (newLower & _upperMask) {
            throw LSST_EXCEPT(
                    pex::exceptions::InvalidParameterError,
                    (boost::format("Explicit ID '%s' does not have the correct form.") % newLower).str());
        }
        _lower = newLower;
    }

    std::shared_ptr<IdFactory> clone() const override { return std::make_shared<SourceIdFactory>(*this); }

    SourceIdFactory(RecordId expId, int reserved)
            : _upper(expId << reserved),
              _upperMask(std::numeric_limits<RecordId>::max() << reserved),
              _lower(0) {
        if (_upper >> reserved != expId) {
            throw LSST_EXCEPT(pex::exceptions::InvalidParameterError,
                              (boost::format("Exposure ID '%s' is too large.") % expId).str());
        }
    }

private:
    RecordId const _upper;
    RecordId const _upperMask;
    RecordId _lower;
};

}  // namespace

std::shared_ptr<IdFactory> IdFactory::makeSimple() { return std::make_shared<SimpleIdFactory>(); }

std::shared_ptr<IdFactory> IdFactory::makeSource(RecordId expId, int reserved) {
    return std::make_shared<SourceIdFactory>(expId, reserved);
}
}  // namespace table
}  // namespace afw
}  // namespace lsst
