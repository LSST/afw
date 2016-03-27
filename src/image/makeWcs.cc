// -*- lsst-c++ -*-

/* 
 * LSST Data Management System
 * See the COPYRIGHT and LICENSE files in the top-level directory of this
 * package for notices and licensing terms.
 */
 
#include "boost/make_shared.hpp"
#include "Eigen/Core"
#include "lsst/pex/logging.h"
#include "lsst/afw/image/Wcs.h"
#include "lsst/afw/image/TanWcs.h"

namespace except = lsst::pex::exceptions; 
namespace afwImg = lsst::afw::image;

/**
 * Create a Wcs object from a fits header.
 * It examines the header and determines the 
 * most suitable object to return, either a general Wcs object, or a more specific object specialised to a 
 * given coordinate system (e.g TanWcs)
 */
afwImg::Wcs::Ptr afwImg::makeWcs(
        PTR(lsst::daf::base::PropertySet) const& _metadata, ///< input metadata
        bool stripMetadata                              ///< Remove FITS keywords from metadata?
                                )
{
    //
    // _metadata is not const (it is probably meant to be), but we don't want to modify it.
    //
    auto metadata = _metadata;          // we'll make a copy and modify metadata if needs be
    auto modifyable = false;            // ... and set this variable to say that we did

    std::string ctype1, ctype2;
    if (metadata->exists("CTYPE1") && metadata->exists("CTYPE2")) {
        ctype1 = metadata->getAsString("CTYPE1");
        ctype2 = metadata->getAsString("CTYPE2");
    } else {
        return PTR(Wcs)();
    }
    //
    // SCAMP used to use PVi_j keys with a CTYPE of TAN to specify a "TPV" projection
    // (cf. https://github.com/astropy/astropy/issues/299
    // and the discussion from Dave Berry in https://jira.lsstcorp.org/browse/DM-2883)
    //
    // Follow Dave's AST and switch TAN to TPV
    //
    using pex::logging::Log;
    auto log = Log(Log::getDefaultLog(), "makeWcs");
    
    if (ctype1.substr(5, 3) == "TAN" &&
        (metadata->exists("PV1_5") || metadata->exists("PV2_1"))) {
        log.log(Log::INFO, str(boost::format("Interpreting %s/%s + PVi_j as TPV") % ctype1 % ctype2));

        if (!modifyable) {
            metadata = _metadata->deepCopy();
            modifyable = true;
        }

        ctype1.replace(5, 3, "TPV");
        metadata->set<std::string>("CTYPE1", ctype1);

        ctype2.replace(5, 3, "TPV");
        metadata->set<std::string>("CTYPE2", ctype2);
    }

    afwImg::Wcs::Ptr wcs;               // we can't use make_shared as ctor is private
    if (ctype1.substr(5, 3) == "TAN") {
        wcs = afwImg::Wcs::Ptr(new afwImg::TanWcs(metadata));
    } else if (ctype1.substr(5, 3) == "TPV") { // unfortunately we don't support TPV
        if (!modifyable) {
            metadata = _metadata->deepCopy();
            modifyable = true;
        }
        
        log.log(Log::WARN, str(boost::format("Stripping PVi_j keys from projection %s/%s") % ctype1 % ctype2));

        metadata->set<std::string>("CTYPE1", "RA---TAN");
        metadata->set<std::string>("CTYPE2", "DEC--TAN");
        metadata->set<bool>("TPV_WCS", true);
        // PV1_[1-4] are in principle legal, although Swarp reuses them as part of the TPV parameterisation.
        // It turns out that leaving PV1_[1-4] in the header breaks wcslib 4.14's ability to read
        // DECam headers (DM-3196) so we'll delete them all for now.
        //
        // John Swinbank points out the maximum value of j in TVi_j is 39;
        // http://fits.gsfc.nasa.gov/registry/tpvwcs/tpv.html
        for (int i = 0; i != 2; ++i) {
            for (int j = 1; j <= 39; ++j) { // 39's the max in the TPV standard
                char pvName[8];
                sprintf(pvName, "PV%d_%d", i, j);
                if (metadata->exists(pvName)) {
                    metadata->remove(pvName);
                }
            }
        }

        wcs = afwImg::Wcs::Ptr(new afwImg::TanWcs(metadata));
    } else {
        wcs = afwImg::Wcs::Ptr(new afwImg::Wcs(metadata));
    }

    //If keywords LTV[1,2] are present, the image on disk is already a subimage, so
    //we should shift the wcs to allow for this.
    std::string key = "LTV1";
    if (metadata->exists(key)) {
        wcs->shiftReferencePixel(-metadata->getAsDouble(key), 0);
    }

    key = "LTV2";
    if (metadata->exists(key) ) {
        wcs->shiftReferencePixel(0, -metadata->getAsDouble(key));
    }

    if (stripMetadata) {
        afwImg::detail::stripWcsKeywords(metadata, wcs);
    }

    return wcs;
}
    
/**
 * @brief Create a Wcs object from crval, crpix, CD, using CD elements (useful from python)
 */
afwImg::Wcs::Ptr afwImg::makeWcs(
    lsst::afw::coord::Coord const & crval, ///< CRVAL1,2 (ie. the sky origin)
    lsst::afw::geom::Point2D const & crpix, ///< CRPIX1,2 (ie. the pixel origin) in pixels
    double CD11,                   ///< CD matrix element 1,1                    
    double CD12,                   ///< CD matrix element 1,2                    
    double CD21,                   ///< CD matrix element 2,1                    
    double CD22                    ///< CD matrix element 2,2                    
    ) {
    Eigen::Matrix2d CD;
    CD << CD11, CD12, CD21, CD22;
    lsst::afw::geom::Point2D crvalTmp;
    crvalTmp[0] = crval.toIcrs().getLongitude().asDegrees();
    crvalTmp[1] = crval.toIcrs().getLatitude().asDegrees();
    return afwImg::Wcs::Ptr(new lsst::afw::image::TanWcs(crvalTmp, crpix, CD));
}
