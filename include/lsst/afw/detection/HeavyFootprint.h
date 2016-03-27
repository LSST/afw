/* 
 * LSST Data Management System
 * See the COPYRIGHT and LICENSE files in the top-level directory of this
 * package for notices and licensing terms.
 */
 
#if !defined(LSST_DETECTION_HEAVY_FOOTPRINT_H)
#define LSST_DETECTION_HEAVY_FOOTPRINT_H
/**
 * \file
 * \brief Represent a set of pixels of an arbitrary shape and size,
 *        including values for those pixels; a HeavyFootprint is a
 *        Footprint that also not only a description of a region, but
 *        values within that region.
 */
#include <algorithm>
#include <list>
#include <cmath>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include "lsst/afw/detection/Footprint.h"

namespace lsst {
namespace afw { 
namespace detection {

class HeavyFootprintCtrl;

/*!
 * \brief A set of pixels in an Image, including those pixels' actual values
 */
template <typename ImagePixelT, typename MaskPixelT=lsst::afw::image::MaskPixel,
          typename VariancePixelT=lsst::afw::image::VariancePixel>
class HeavyFootprint :
    public afw::table::io::PersistableFacade< HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT> >,
    public Footprint
{
public:

    /**
     * Create a HeavyFootprint from a regular Footprint and the image that
     * provides the pixel values
     *
     * \note: the HeavyFootprintCtrl is passed by const* not const& so
     * that we needn't provide a definition in the header.
     *
     * foot: The Footprint defining the pixels to set
     * mimage: The pixel values
     * ctrl: Control how we manipulate HeavyFootprints
     */
    explicit HeavyFootprint(
        Footprint const& foot,
        lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> const& mimage,
        HeavyFootprintCtrl const* ctrl=NULL
                           );

    /**
     * Create a HeavyFootprint from a regular Footprint, allocating space
     * to hold foot.getArea() pixels, but not initializing them.  This is
     * used when unpersisting a HeavyFootprint.
     */
    explicit HeavyFootprint(Footprint const& foot,
                            HeavyFootprintCtrl const* ctrl=NULL);

    /**
     * Is this a HeavyFootprint (yes!)
     */
    virtual bool isHeavy() const { return true; }

    /**
     * Replace all the pixels in the image with the values in the HeavyFootprint.
     */
    void insert(lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> & mimage) const;

    /**
     * Replace all the pixels in the image with the values in the HeavyFootprint.
     */
    void insert(lsst::afw::image::Image<ImagePixelT> & image) const;

    ndarray::Array<ImagePixelT,1,1>     getImageArray() { return _image; }
    ndarray::Array<MaskPixelT,1,1>      getMaskArray() { return _mask; }
    ndarray::Array<VariancePixelT,1,1>  getVarianceArray() { return _variance; }

    ndarray::Array<ImagePixelT const,1,1>     getImageArray() const { return _image; }
    ndarray::Array<MaskPixelT const,1,1>      getMaskArray() const { return _mask; }
    ndarray::Array<VariancePixelT const,1,1>  getVarianceArray() const { return _variance; }

    /* Returns the OR of all the mask pixels held in this HeavyFootprint. */
    MaskPixelT getMaskBitsSet() const {
        MaskPixelT maskbits = 0;
        for (typename ndarray::Array<MaskPixelT,1,1>::Iterator i = _mask.begin(); i != _mask.end(); ++i) {
            maskbits |= *i;
        }
        return maskbits;
    }

    /// Dot product between HeavyFootprints
    ///
    /// The mask and variance planes are ignored.
    double dot(HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT> const& other) const;

protected:

    class Factory;  // factory class used for persistence, public only so we can instantiate it in .cc file

    virtual std::string getPersistenceName() const;

    virtual void write(OutputArchiveHandle & handle) const;

private:
    HeavyFootprint() {}  // private constructor, only used for persistence.

    ndarray::Array<ImagePixelT, 1, 1> _image;
    ndarray::Array<MaskPixelT, 1, 1> _mask;
    ndarray::Array<VariancePixelT, 1, 1> _variance;
};

/**
 * Create a HeavyFootprint with footprint defined by the given
 * Footprint and pixel values from the given MaskedImage.
 */
template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT> makeHeavyFootprint(
    Footprint const& foot,
    lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> const& img,
    HeavyFootprintCtrl const* ctrl=NULL
                                                                          )
{
    return HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT>(foot, img, ctrl);
}

/**
 * Sum the two given HeavyFootprints *h1* and *h2*, returning a
 * HeavyFootprint with the union footprint, and summed pixels where
 * they overlap.  The peak list is the union of the two inputs.
 */
template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
boost::shared_ptr<HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT> >
mergeHeavyFootprints(
    HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT> const& h1,
    HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT> const& h2
);

}}}

#endif
