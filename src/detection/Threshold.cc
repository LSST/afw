/* 
 * LSST Data Management System 
 * See the COPYRIGHT and LICENSE files in the top-level directory of this
 * package for notices and licensing terms.
 */
#include <cmath>
#include <string>
#include <boost/format.hpp>
#include "lsst/pex/exceptions.h"
#include "lsst/pex/logging/Trace.h"
#include "lsst/afw/math/Statistics.h"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/detection/Threshold.h"

namespace image = lsst::afw::image;
namespace math = lsst::afw::math;
namespace pexLogging = lsst::pex::logging;

namespace lsst {
namespace afw {
namespace detection {

Threshold::ThresholdType Threshold::parseTypeString(std::string const & typeStr) {
    if (typeStr.compare("bitmask") == 0) {
        return Threshold::BITMASK;           
    } else if (typeStr.compare("value") == 0) {
        return Threshold::VALUE;           
    } else if (typeStr.compare("stdev") == 0) {
        return Threshold::STDEV;
    } else if (typeStr.compare("variance") == 0) {
        return Threshold::VARIANCE;
    } else if (typeStr.compare("pixel_stdev") == 0) {
        return Threshold::PIXEL_STDEV;
    } else {
        throw LSST_EXCEPT(
            lsst::pex::exceptions::InvalidParameterError,
            (boost::format("Unsupported Threshold type: %s") % typeStr).str()
        );
    }    
}

std::string Threshold::getTypeString(ThresholdType const & type) {
    if (type == VALUE) {
        return "value";
    } else if (type == STDEV) {
        return "stdev";
    } else if (type == VARIANCE) {
        return "variance";
    } else {
        throw LSST_EXCEPT(
            lsst::pex::exceptions::InvalidParameterError,
            (boost::format("Unsopported Threshold type: %d") % type).str()
        );
    }
}

/**
 * return value of threshold, to be interpreted via type
 * @param param value of variance/stdev if needed
 * @return value of threshold
 */
double Threshold::getValue(const double param) const {
    switch (_type) {
      case STDEV:
        if (param <= 0) {
            throw LSST_EXCEPT(
                lsst::pex::exceptions::InvalidParameterError,
                (boost::format("St. dev. must be > 0: %g") % param).str()
            );
        }
        return _value*param;
      case VALUE:
      case BITMASK:
      case PIXEL_STDEV:
        return _value;
      case VARIANCE:
        if (param <= 0) {
            throw LSST_EXCEPT(
                lsst::pex::exceptions::InvalidParameterError,
                (boost::format("Variance must be > 0: %g") % param).str()
            );
        }
        return _value*std::sqrt(param);
      default:
        throw LSST_EXCEPT(
            lsst::pex::exceptions::InvalidParameterError,
            (boost::format("Unsupported type: %d") % _type).str()
        );
    }
}

template<typename ImageT>
double Threshold::getValue(ImageT const& image) const {
    double param = -1;                  // Parameter for getValue()
    if (_type == STDEV || 
        _type == VARIANCE) {
        math::Statistics stats = math::makeStatistics(image, math::STDEVCLIP);
        double const sd = stats.getValue(math::STDEVCLIP);

        pexLogging::TTrace<3>("afw.detection", "St. Dev = %g", sd);
        
        if (_type == VARIANCE) {
            param = sd*sd;
        } else {
            param = sd;
        }
    }
    return getValue(param);
}

/**
 * \brief Factory method for creating Threshold objects
 *
 * @param value value of threshold
 * @param typeStr string representation of a ThresholdType. This parameter is 
 *                optional. Allowed values are: "variance", "value", "stdev", "pixel_stdev"
 * @param polarity If true detect positive objects, false for negative
 *
 * @return desired Threshold
 */
Threshold createThreshold(
    double const value,                  
    std::string const typeStr,
    bool const polarity
) {
    return Threshold(value, Threshold::parseTypeString(typeStr), polarity);
}


//
// Explicit instantiations
//
#define INSTANTIATE(TYPE) \
template double Threshold::getValue(image::TYPE<unsigned short> const&) const; \
template double Threshold::getValue(image::TYPE<int> const&) const; \
template double Threshold::getValue(image::TYPE<float> const&) const; \
template double Threshold::getValue(image::TYPE<double> const&) const; \
template double Threshold::getValue(image::TYPE<boost::uint64_t> const&) const;

#ifndef DOXYGEN
INSTANTIATE(Image);
INSTANTIATE(MaskedImage);
#endif

}}}
