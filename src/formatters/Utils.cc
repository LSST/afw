// -*- lsst-c++ -*-

/*
 * LSST Data Management System
 * Copyright 2008, 2009, 2010 LSST Corporation.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
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
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

//
//##====----------------                                ----------------====##/
//
// Support for formatters
//
//##====----------------                                ----------------====##/

#include <cstdint>
#include <iostream>
#include <vector>

#include "boost/format.hpp"

#include "lsst/pex/exceptions.h"
#include "lsst/daf/base/PropertySet.h"
#include "lsst/daf/persistence/LogicalLocation.h"
#include "lsst/daf/persistence/DbTsvStorage.h"
#include "lsst/afw/formatters/Utils.h"

using std::int64_t;
namespace ex = lsst::pex::exceptions;
using lsst::daf::base::PropertySet;
using lsst::pex::policy::Policy;
using lsst::daf::persistence::LogicalLocation;

namespace lsst {
namespace afw {
namespace formatters {
namespace {

/**
Format a PropertySet into a FITS header string (exactly 80 characters per "card", no line terminator)

See @ref formatFitsProperties for details.

@param[in] paramNames  Names of properties to format
@param[in] prop  Properties to format
*/
std::string formatFitsPropertiesImpl(std::vector<std::string> const& paramNames,
                                     daf::base::PropertySet const& prop) {
    std::ostringstream result;
    for (auto const& fullName : paramNames) {
        std::size_t lastPeriod = fullName.rfind(char('.'));
        auto name = (lastPeriod == std::string::npos) ? fullName : fullName.substr(lastPeriod + 1);
        std::type_info const& type = prop.typeOf(name);

        std::string out = "";
        out.reserve(80);
        if (name.size() > 8) {
            continue;  // The name is too long for a FITS keyword; skip this item
        } else if (prop.isArray(name)) {
            continue;  // Data is an array; skip this item
        }
        out = (boost::format("%-8s= ") % name).str();

        if (type == typeid(bool)) {
            out += prop.get<bool>(name) ? "1" : "0";
        } else if (type == typeid(std::uint8_t)) {
            out += (boost::format("%20d") % static_cast<int>(prop.get<std::uint8_t>(name))).str();
        } else if (type == typeid(int)) {
            out += (boost::format("%20d") % prop.get<int>(name)).str();
        } else if (type == typeid(double)) {
            out += (boost::format("%20.15g") % prop.get<double>(name)).str();
        } else if (type == typeid(float)) {
            out += (boost::format("%20.15g") % prop.get<float>(name)).str();
        } else if (type == typeid(std::string)) {
            out += "'" + prop.get<std::string>(name) + "'";
            if (out.size() > 80) {
                continue;  // Formatted data is too long; skip this item
            }
        }

        int const len = out.size();
        if (len < 80) {
            out += std::string(80 - len, ' ');
        } else if (len > 80) {
            // non-string item has a formatted value that is too long; this should never happen
            throw LSST_EXCEPT(ex::LogicError,
                              "Formatted data too long: " + std::to_string(len) + " > 80: \"" + out + "\"");
        }

        result << out;
    }

    return result.str();
}

} // namespace

int extractSliceId(std::shared_ptr<PropertySet const> const& properties) {
    if (properties->isArray("sliceId")) {
        throw LSST_EXCEPT(ex::RuntimeError, "\"sliceId\" property has multiple values");
    }
    int sliceId = properties->getAsInt("sliceId");
    if (sliceId < 0) {
        throw LSST_EXCEPT(ex::RangeError, "negative \"sliceId\"");
    }
    if (properties->exists("universeSize") && !properties->isArray("universeSize")) {
        int universeSize = properties->getAsInt("universeSize");
        if (sliceId >= universeSize) {
            throw LSST_EXCEPT(ex::RangeError, "\"sliceId\" must be less than \"universeSize \"");
        }
    }
    return sliceId;
}

int extractVisitId(std::shared_ptr<PropertySet const> const& properties) {
    if (properties->isArray("visitId")) {
        throw LSST_EXCEPT(ex::RuntimeError, "\"visitId\" property has multiple values");
    }
    int visitId = properties->getAsInt("visitId");
    if (visitId < 0) {
        throw LSST_EXCEPT(ex::RangeError, "negative \"visitId\"");
    }
    return visitId;
}

int64_t extractFpaExposureId(std::shared_ptr<PropertySet const> const& properties) {
    if (properties->isArray("fpaExposureId")) {
        throw LSST_EXCEPT(ex::RuntimeError, "\"fpaExposureId\" property has multiple values");
    }
    int64_t fpaExposureId = properties->getAsInt64("fpaExposureId");
    if (fpaExposureId < 0) {
        throw LSST_EXCEPT(ex::RangeError, "negative \"fpaExposureId\"");
    }
    if ((fpaExposureId & 0xfffffffe00000000LL) != 0LL) {
        throw LSST_EXCEPT(ex::RangeError, "\"fpaExposureId\" is too large");
    }
    return fpaExposureId;
}

int extractCcdId(std::shared_ptr<PropertySet const> const& properties) {
    if (properties->isArray("ccdId")) {
        throw LSST_EXCEPT(ex::RuntimeError, "\"ccdId\" property has multiple values");
    }
    int ccdId = properties->getAsInt("ccdId");
    if (ccdId < 0) {
        throw LSST_EXCEPT(ex::RangeError, "negative \"ccdId\"");
    }
    if (ccdId > 255) {
        throw LSST_EXCEPT(ex::RangeError, "\"ccdId\" is too large");
    }
    return static_cast<int>(ccdId);
}

int extractAmpId(std::shared_ptr<PropertySet const> const& properties) {
    if (properties->isArray("ampId")) {
        throw LSST_EXCEPT(ex::RuntimeError, "\"ampId\" property has multiple values");
    }
    int ampId = properties->getAsInt("ampId");
    if (ampId < 0) {
        throw LSST_EXCEPT(ex::RangeError, "negative \"ampId\"");
    }
    if (ampId > 63) {
        throw LSST_EXCEPT(ex::RangeError, "\"ampId\" is too large");
    }
    return (extractCcdId(properties) << 6) + ampId;
}

int64_t extractCcdExposureId(std::shared_ptr<PropertySet const> const& properties) {
    if (properties->isArray("ccdExposureId")) {
        throw LSST_EXCEPT(ex::RuntimeError, "\"ccdExposureId\" property has multiple values");
    }
    int64_t ccdExposureId = properties->getAsInt64("ccdExposureId");
    if (ccdExposureId < 0) {
        throw LSST_EXCEPT(ex::RangeError, "negative \"ccdExposureId\"");
    }
    return ccdExposureId;
}

int64_t extractAmpExposureId(std::shared_ptr<PropertySet const> const& properties) {
    if (properties->isArray("ampExposureId")) {
        throw LSST_EXCEPT(ex::RuntimeError, "\"ampExposureId\" property has multiple values");
    }
    int64_t ampExposureId = properties->getAsInt64("ampExposureId");
    if (ampExposureId < 0) {
        throw LSST_EXCEPT(ex::RangeError, "negative \"ampExposureId\"");
    }
    return ampExposureId;
}

std::string const getItemName(std::shared_ptr<PropertySet const> const& properties) {
    if (!properties) {
        throw LSST_EXCEPT(ex::InvalidParameterError, "Null std::shared_ptr<PropertySet>");
    }
    if (properties->isArray("itemName")) {
        throw LSST_EXCEPT(ex::InvalidParameterError, "\"itemName\" property has multiple values");
    }
    return properties->getAsString("itemName");
}

bool extractOptionalFlag(std::shared_ptr<PropertySet const> const& properties, std::string const& name) {
    if (properties && properties->exists(name)) {
        return properties->getAsBool(name);
    }
    return false;
}

std::string const getTableName(std::shared_ptr<Policy const> const& policy,
                               std::shared_ptr<PropertySet const> const& properties) {
    std::string itemName(getItemName(properties));
    return LogicalLocation(policy->getString(itemName + ".tableNamePattern"), properties).locString();
}

std::vector<std::string> getAllSliceTableNames(std::shared_ptr<Policy const> const& policy,
                                               std::shared_ptr<PropertySet const> const& properties) {
    std::string itemName(getItemName(properties));
    std::string pattern(policy->getString(itemName + ".tableNamePattern"));
    int numSlices = 1;
    if (properties->exists(itemName + ".numSlices")) {
        numSlices = properties->getAsInt(itemName + ".numSlices");
    }
    if (numSlices <= 0) {
        throw LSST_EXCEPT(ex::RuntimeError, itemName + " \".numSlices\" property value must be positive");
    }
    std::vector<std::string> names;
    names.reserve(numSlices);
    std::shared_ptr<PropertySet> props = properties->deepCopy();
    for (int i = 0; i < numSlices; ++i) {
        props->set("sliceId", i);
        names.push_back(LogicalLocation(pattern, props).locString());
    }
    return names;
}

void createTable(lsst::daf::persistence::LogicalLocation const& location,
                 std::shared_ptr<lsst::pex::policy::Policy const> const& policy,
                 std::shared_ptr<PropertySet const> const& properties) {
    std::string itemName(getItemName(properties));
    std::string name(getTableName(policy, properties));
    std::string model(policy->getString(itemName + ".templateTableName"));

    lsst::daf::persistence::DbTsvStorage db;
    db.setPersistLocation(location);
    db.createTableFromTemplate(name, model);
}

void dropAllSliceTables(lsst::daf::persistence::LogicalLocation const& location,
                        std::shared_ptr<lsst::pex::policy::Policy const> const& policy,
                        std::shared_ptr<PropertySet const> const& properties) {
    std::vector<std::string> names = getAllSliceTableNames(policy, properties);

    lsst::daf::persistence::DbTsvStorage db;
    db.setPersistLocation(location);
    for (std::vector<std::string>::const_iterator i(names.begin()), end(names.end()); i != end; ++i) {
        db.dropTable(*i);
    }
}

std::string formatFitsProperties(daf::base::PropertySet const& prop,
                                 std::set<std::string> const& excludeNames) {
    daf::base::PropertyList const *pl = dynamic_cast<daf::base::PropertyList const *>(&prop);
    std::vector<std::string> allParamNames;
    if (pl) {
        allParamNames = pl->getOrderedNames();
    } else {
        allParamNames = prop.paramNames(false);
    }
    std::vector<std::string> desiredParamNames;
    for (auto const & name: allParamNames) {
        if (excludeNames.count(name) == 0) {
            desiredParamNames.push_back(name);
        }
    }
    return formatFitsPropertiesImpl(desiredParamNames, prop);
}

int countFitsHeaderCards(lsst::daf::base::PropertySet const& prop) {
    return prop.paramNames(false).size();
}

ndarray::Array<std::uint8_t, 1, 1> stringToBytes(std::string const& str) {
    auto nbytes = str.size() * sizeof(char) / sizeof(std::uint8_t);
    std::uint8_t const* byteCArr = reinterpret_cast<std::uint8_t const*>(str.data());
    auto shape = ndarray::makeVector(nbytes);
    auto strides = ndarray::makeVector(1);
    // Make an Array that shares memory with `str` (and does not free that memory when destroyed),
    // then return a copy; this is simpler than manually copying the data into a newly allocated array
    ndarray::Array<std::uint8_t const, 1, 1> localArray = ndarray::external(byteCArr, shape, strides);
    return ndarray::copy(localArray);
}

std::string bytesToString(ndarray::Array<std::uint8_t const, 1, 1> const& bytes) {
    auto nchars = bytes.size() * sizeof(std::uint8_t) / sizeof(char);
    char const* charCArr = reinterpret_cast<char const*>(bytes.getData());
    return std::string(charCArr, nchars);
}

}
}
}  // namespace lsst::afw::formatters
