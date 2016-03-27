// -*- LSST-C++ -*- // fixed format comment for emacs
/*
 * LSST Data Management System
 * See the COPYRIGHT and LICENSE files in the top-level directory of this
 * package for notices and licensing terms.
 */

#ifndef LSST_AFW_IMAGE_ExposureInfo_h_INCLUDED
#define LSST_AFW_IMAGE_ExposureInfo_h_INCLUDED

#include "lsst/base.h"
#include "lsst/daf/base.h"
#include "lsst/afw/geom/Point.h"
#include "lsst/afw/image/Filter.h"
#include "lsst/afw/table/io/OutputArchive.h"
#include "lsst/afw/image/CoaddInputs.h"

namespace lsst { namespace afw {

namespace cameraGeom {
class Detector;
}

namespace detection {
class Psf;
}

namespace geom { namespace polygon {
class Polygon;
}}

namespace fits {
class Fits;
}

namespace image {

class Calib;
class Wcs;
class ApCorrMap;

/**
 *  @brief A collection of all the things that make an Exposure different from a MaskedImage
 *
 *  The constness semantics of the things held by ExposureInfo are admittedly a bit of a mess,
 *  but they're that way to preserve backwards compatibility for now.  Eventually I'd like to make
 *  a lot of these things immutable, but in the meantime, here's the summary:
 *   - Wcs, Calib, and Psf are held by non-const pointer, and you can get a non-const pointer via a
 *     non-const member function accessor and a const pointer via a const member function accessor.
 *   - Detector is held by const pointer and only returned by const pointer (but if you're
 *     in Python, SWIG will have casted all that constness away).
 *   - Filter is held and returned by value.
 *   - Metadata is held by non-const pointer, and you can get a non-const pointer via a const
 *     member function accessor (i.e. constness is not propagated).
 *
 *  The setters for Wcs and Calib clone their input arguments (this is a departure from the
 *  previous behavior for Calib and Wcs but it's safer w.r.t. aliasing and it matches the old
 *  (and current) behavior of the Exposure and ExposureInfo constructors, which clone their
 *  arguments.  The setter for Psf and constructors do not clone the Psf, as Psfs are immutable
 *  and hence we don't need to ensure strict ownership.  The setter for Detector does *not*
 *  clone its input argument, because while it technically isn't, we can safely consider a
 *  Detector to be immutable once it's attached to an ExposureInfo.
 */
class ExposureInfo {
public:

    /// Does this exposure have a Wcs?
    bool hasWcs() const { return static_cast<bool>(_wcs); }

    /// Return the coordinate system of the exposure
    PTR(Wcs) getWcs() { return _wcs; }

    /// Return the coordinate system of the exposure
    CONST_PTR(Wcs) getWcs() const { return _wcs; }

    /// Set the coordinate system of the exposure
    void setWcs(CONST_PTR(Wcs) wcs) { _wcs = _cloneWcs(wcs); }

    /// Does this exposure have Detector information?
    bool hasDetector() const { return static_cast<bool>(_detector); }

    /// Return the exposure's Detector information
    CONST_PTR(cameraGeom::Detector) getDetector() const { return _detector; }

    /// Set the exposure's Detector information
    void setDetector(CONST_PTR(cameraGeom::Detector) detector) { _detector = detector; }

    /// Return the exposure's filter
    Filter getFilter() const { return _filter; }

    /// Set the exposure's filter
    void setFilter(Filter const& filter) { _filter = filter; }

    /// Does this exposure have a Calib?
    bool hasCalib() const { return static_cast<bool>(_calib); }

    /// Return the exposure's photometric calibration
    PTR(Calib) getCalib() { return _calib; }

    /// Return the exposure's photometric calibration
    CONST_PTR(Calib) getCalib() const { return _calib; }

    /// Set the Exposure's Calib object
    void setCalib(CONST_PTR(Calib) calib) { _calib = _cloneCalib(calib); }

    /// Return flexible metadata
    PTR(daf::base::PropertySet) getMetadata() const { return _metadata; }

    /// Set the flexible metadata
    void setMetadata(PTR(daf::base::PropertySet) metadata) { _metadata = metadata; }

    /// Does this exposure have a Psf?
    bool hasPsf() const { return static_cast<bool>(_psf); }

    /// Return the exposure's point-spread function
    PTR(detection::Psf) getPsf() const { return _psf; }

    /// Set the exposure's point-spread function
    void setPsf(CONST_PTR(detection::Psf) psf) {
        // Psfs are immutable, so this is always safe; it'd be better to always just pass around
        // const or non-const pointers, instead of both, but this is more backwards-compatible.
        _psf = boost::const_pointer_cast<detection::Psf>(psf);
    }

    /// Does this exposure have a valid Polygon
    bool hasValidPolygon() const { return static_cast<bool>(_validPolygon);}

    /// Return the valid Polygon
    CONST_PTR(geom::polygon::Polygon) getValidPolygon() const { return _validPolygon;}

    /// Set the exposure's valid Polygon
    void setValidPolygon(CONST_PTR(geom::polygon::Polygon) polygon) { _validPolygon = polygon; }

    /// Return true if the exposure has an aperture correction map
    bool hasApCorrMap() const { return static_cast<bool>(_apCorrMap); }

    /// Return the exposure's aperture correction map (null pointer if !hasApCorrMap())
    PTR(ApCorrMap) getApCorrMap() { return _apCorrMap; }

    /// Return the exposure's aperture correction map (null pointer if !hasApCorrMap())
    PTR(ApCorrMap const) getApCorrMap() const { return _apCorrMap; }

    /// Set the exposure's aperture correction map (null pointer if !hasApCorrMap())
    void setApCorrMap(PTR(ApCorrMap) apCorrMap) { _apCorrMap = apCorrMap; }

    /**
     *  Set the exposure's aperture correction map to a new, empty map
     *
     *  Note that the ExposureInfo constructors do not create an empty aperture correction map,
     *  so this method provide a convenient way to initialize one before filling it.
     */
    void initApCorrMap();

    /// Does this exposure have coadd provenance catalogs?
    bool hasCoaddInputs() const { return static_cast<bool>(_coaddInputs); }

    /// Set the exposure's coadd provenance catalogs.
    void setCoaddInputs(PTR(CoaddInputs) coaddInputs) { _coaddInputs = coaddInputs; }

    /// Return a pair of catalogs that record the inputs, if this Exposure is a coadd (otherwise null).
    PTR(CoaddInputs) getCoaddInputs() const { return _coaddInputs; }

    /**
     *  @brief Construct an ExposureInfo from its various components.
     *
     *  If a null Calib and/or PropertySet pointer is passed (the default),
     *  a new Calib and/or PropertyList will be created.  To set these pointers
     *  to null, you must explicitly call setCalib or setMetadata after construction.
     */
    explicit ExposureInfo(
        CONST_PTR(Wcs) const & wcs = CONST_PTR(Wcs)(),
        CONST_PTR(detection::Psf) const & psf = CONST_PTR(detection::Psf)(),
        CONST_PTR(Calib) const & calib = CONST_PTR(Calib)(),
        CONST_PTR(cameraGeom::Detector) const & detector = CONST_PTR(cameraGeom::Detector)(),
        CONST_PTR(geom::polygon::Polygon) const & polygon = CONST_PTR(geom::polygon::Polygon)(),
        Filter const & filter = Filter(),
        PTR(daf::base::PropertySet) const & metadata = PTR(daf::base::PropertySet)(),
        PTR(CoaddInputs) const & coaddInputs = PTR(CoaddInputs)(),
        PTR(ApCorrMap) const & apCorrMap = PTR(ApCorrMap)()
    );

    /// Copy constructor; deep-copies all components except the metadata.
    ExposureInfo(ExposureInfo const & other);

    /// Copy constructor; deep-copies everything, possibly including the metadata.
    ExposureInfo(ExposureInfo const & other, bool copyMetadata);

    /// Assignment; deep-copies all components except the metadata.
    ExposureInfo & operator=(ExposureInfo const & other);

    // Destructor defined in source file because we need access to destructors of forward-declared components
    ~ExposureInfo();

private:

    template <typename ImageT, typename MaskT, typename VarianceT> friend class Exposure;

    /**
     *  @brief A struct passed back and forth between Exposure and ExposureInfo when writing FITS files.
     *
     *  An ExposureInfo is generally held by an Exposure, and we implement much of Exposure persistence
     *  here in ExposureInfo.  FITS writing needs to take place in three steps:
     *   1. Exposure calls ExposureInfo::_startWriteFits to generate the image headers in the form of
     *      PropertyLists.  The headers  include archive IDs for the components of ExposureInfo, so we
     *      have to put those in the archive at this time, and transfer the PropertyLists and archive
     *      to the Exposure for the next step.
     *   2. Exposure calls MaskedImage::writeFits to save the Image, Mask, and Variance HDUs along
     *      with the headers.
     *   3. Exposure calls ExposureInfo::_finishWriteFits to save the archive to additional table HDUs.
     */
    struct FitsWriteData {
        PTR(daf::base::PropertyList) metadata;
        PTR(daf::base::PropertyList) imageMetadata;
        PTR(daf::base::PropertyList) maskMetadata;
        PTR(daf::base::PropertyList) varianceMetadata;
        table::io::OutputArchive archive;
    };

    /**
     *  @brief Start the process of writing an exposure to FITS.
     *
     *  @param[in]  xy0   The origin of the exposure associated with this object, used to
     *                    install a linear offset-only WCS in the FITS header.
     *
     *  @sa FitsWriteData
     */
    FitsWriteData _startWriteFits(geom::Point2I const & xy0=geom::Point2I()) const;

    /**
     *  @brief Write any additional non-image HDUs to a FITS file.
     *
     *  @param[in]  fitsfile   Open FITS object to write to.  Does not need to be positioned to any
     *                         particular HDU.
     *
     *  The additional HDUs will be appended to the FITS file, and should line up with the HDU index
     *  keys included in the result of getFitsMetadata() if this is called after writing the
     *  MaskedImage HDUs.
     *
     *  @sa FitsWriteData
     */
    void _finishWriteFits(fits::Fits & fitsfile, FitsWriteData const & data) const;

    /**
     *  @brief Read from a FITS file and metadata.
     *
     *  This operates in-place on this instead of returning a new object, because it will usually
     *  only be called by the exposure constructor, which starts by default-constructing the
     *  ExposureInfo.
     */
    void _readFits(
        fits::Fits & fitsfile,
        PTR(daf::base::PropertySet) metadata,
        PTR(daf::base::PropertySet) imageMetadata
    );

    static PTR(Calib) _cloneCalib(CONST_PTR(Calib) calib);
    static PTR(Wcs) _cloneWcs(CONST_PTR(Wcs) wcs);
    static PTR(ApCorrMap) _cloneApCorrMap(PTR(ApCorrMap const) apCorrMap);

    PTR(Wcs) _wcs;
    PTR(detection::Psf) _psf;
    PTR(Calib) _calib;
    CONST_PTR(cameraGeom::Detector) _detector;
    CONST_PTR(geom::polygon::Polygon) _validPolygon;
    Filter _filter;
    PTR(daf::base::PropertySet) _metadata;
    PTR(CoaddInputs) _coaddInputs;
    PTR(ApCorrMap) _apCorrMap;
};

}}} // lsst::afw::image

#endif // !LSST_AFW_IMAGE_ExposureInfo_h_INCLUDED
