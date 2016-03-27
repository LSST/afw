/* 
 * LSST Data Management System
 * See the COPYRIGHT and LICENSE files in the top-level directory of this
 * package for notices and licensing terms.
 */

/*****************************************************************************/
/** \file
 *
 * \brief HeavyFootprint and associated classes
 */
#include <cassert>
#include <string>
#include <typeinfo>
#include <algorithm>
#include "boost/format.hpp"
#include "lsst/pex/logging/Trace.h"
#include "lsst/pex/exceptions.h"
#include "lsst/afw/detection/Peak.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/detection/HeavyFootprint.h"
#include "lsst/afw/detection/Footprint.h"
#include "lsst/afw/detection/FootprintCtrl.h"
#include "lsst/afw/detection/FootprintArray.h"
#include "lsst/afw/detection/FootprintArray.cc"
#include "lsst/afw/table/io/CatalogVector.h"
#include "lsst/afw/table/io/OutputArchive.h"
#include "lsst/afw/table/io/InputArchive.h"

namespace lsst {
namespace afw {
namespace detection {
namespace {
    template<typename T>
    struct setPixel {
        setPixel(T val) : _val(val) {}

        T operator()(T) const {
            return _val;
        }
    private:
        T _val;
    };

    template<>
    struct setPixel<boost::uint16_t> {
        typedef boost::uint16_t T;

        setPixel(T val) : _mask(~val) {}

        T operator()(T pix) const {
            pix &= _mask;
            return pix;
        }
    private:
        T _mask;
    };
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT>::HeavyFootprint(
    Footprint const& foot,
    lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> const& mimage,
    HeavyFootprintCtrl const *ctrl
        ) : Footprint(foot),
            _image(ndarray::allocate(ndarray::makeVector(foot.getNpix()))),
            _mask(ndarray::allocate(ndarray::makeVector(foot.getNpix()))),
            _variance(ndarray::allocate(ndarray::makeVector(foot.getNpix())))
{
    HeavyFootprintCtrl ctrl_s = HeavyFootprintCtrl();

    if (!ctrl) {
        ctrl = &ctrl_s;
    }

    switch (ctrl->getModifySource()) {
      case HeavyFootprintCtrl::NONE:
        flattenArray(*this, mimage.getImage()->getArray(),    _image,    mimage.getXY0());
        flattenArray(*this, mimage.getMask()->getArray(),     _mask,     mimage.getXY0());
        flattenArray(*this, mimage.getVariance()->getArray(), _variance, mimage.getXY0());
        break;
      case HeavyFootprintCtrl::SET:
        {
        ImagePixelT const ival = ctrl->getImageVal();
        MaskPixelT const mval = ctrl->getMaskVal();
        VariancePixelT const vval = ctrl->getVarianceVal();

        flattenArray(*this, mimage.getImage()->getArray(),    _image,
            setPixel<ImagePixelT>(ival), mimage.getXY0());
        flattenArray(*this, mimage.getMask()->getArray(),     _mask,
            setPixel<MaskPixelT>(mval), mimage.getXY0());
        flattenArray(*this, mimage.getVariance()->getArray(), _variance,
            setPixel<VariancePixelT>(vval), mimage.getXY0());
        break;
        }
    }
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT>::HeavyFootprint(
    Footprint const& foot,              ///< The Footprint defining the pixels to set
    HeavyFootprintCtrl const* ctrl)
    : Footprint(foot),
      _image   (ndarray::allocate(ndarray::makeVector(foot.getNpix()))),
      _mask    (ndarray::allocate(ndarray::makeVector(foot.getNpix()))),
      _variance(ndarray::allocate(ndarray::makeVector(foot.getNpix())))
{
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT>::insert(
        lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> & mimage ///< Image to set
                                                                    ) const
{
    expandArray(*this, _image,    mimage.getImage()->getArray(),    mimage.getXY0());
    expandArray(*this, _mask,     mimage.getMask()->getArray(),     mimage.getXY0());
    expandArray(*this, _variance, mimage.getVariance()->getArray(), mimage.getXY0());
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT>::insert(
    lsst::afw::image::Image<ImagePixelT> & image) const
{
    expandArray(*this, _image,    image.getArray(),    image.getXY0());
}

template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
PTR(HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT>)
mergeHeavyFootprints(HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT> const& h1,
                     HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT> const& h2)
{
    // Merge the Footprints (by merging the Spans)
    PTR(Footprint) foot = mergeFootprints(h1, h2);

    // Find the union bounding-box
    geom::Box2I bbox(h1.getBBox());
    bbox.include(h2.getBBox());

    // Create union-bb-sized images and insert the heavies
    image::MaskedImage<ImagePixelT,MaskPixelT,VariancePixelT> im1(bbox);
    image::MaskedImage<ImagePixelT,MaskPixelT,VariancePixelT> im2(bbox);
    h1.insert(im1);
    h2.insert(im2);
    // Add the pixels
    im1 += im2;

    // Build new HeavyFootprint from the merged spans and summed pixels.
    return PTR(HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT>)
        (new HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT>(*foot, im1));
}


template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
double
HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT>::dot(
    HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT> const& rhs
    ) const
{
    // Require that footprints are sorted in ascending y
    assert(isNormalized());
    assert(rhs.isNormalized());

    // Coordinated cycling through the iterators while juggling the offsets into the arrays
    typedef typename ndarray::Array<ImagePixelT const, 1, 1>::Iterator ArrayIter;
    ArrayIter lhsArray = getImageArray().begin(), rhsArray = rhs.getImageArray().begin();
    SpanList::const_iterator lhsIter = getSpans().begin(), rhsIter = rhs.getSpans().begin();
    SpanList::const_iterator const lhsEnd = getSpans().end(), rhsEnd = rhs.getSpans().end();
    double sum = 0.0;
    while (lhsIter != lhsEnd && rhsIter != rhsEnd) {
        Span const& lhsSpan = **lhsIter, rhsSpan = **rhsIter;
        int const yLhs = lhsSpan.getY(), yRhs = rhsSpan.getY();
        if (yLhs == yRhs) {
            int const x0Lhs = lhsSpan.getX0(), x1Lhs = lhsSpan.getX1();
            int const x0Rhs = rhsSpan.getX0(), x1Rhs = rhsSpan.getX1();
            int const xMin = std::max(x0Lhs, x0Rhs), xMax = std::min(x1Lhs, x1Rhs);
            if (xMin <= xMax) {
                lhsArray += xMin - x0Lhs;
                rhsArray += xMin - x0Rhs;
                for (int x = xMin; x <= xMax; ++x, ++lhsArray, ++rhsArray) {
                    sum += (*lhsArray)*(*rhsArray);
                }
                // Rewind to the start of the span, for easier sync between spans and arrays
                lhsArray -= xMax + 1 - x0Lhs;
                rhsArray -= xMax + 1 - x0Rhs;
            }
            if (x1Lhs <= x1Rhs) {
                lhsArray += lhsSpan.getWidth();
                ++lhsIter;
            } else {
                rhsArray += rhsSpan.getWidth();
                ++rhsIter;
            }
            continue;
        } else if (yLhs < yRhs) {
            while (lhsIter != lhsEnd && (*lhsIter)->getY() < yRhs) {
                lhsArray += (*lhsIter)->getWidth();
                ++lhsIter;
            }
            continue;
        } else { // yLhs > yRhs
            while (rhsIter != rhsEnd && (*rhsIter)->getY() < yLhs) {
                rhsArray += (*rhsIter)->getWidth();
                ++rhsIter;
            }
            continue;
        }
    }
    return sum;
}


/************************************************************************************************************/
// Persistence (using afw::table::io)
//

namespace {

// Schema and Keys used to persist the pixels of a HeavyFootprint (Spans and Peaks are handled by the
// Footprint base class).  This is a singleton, but a different one for each template instantiation.
template <typename ImagePixelT,
          typename MaskPixelT=image::MaskPixel,
          typename VariancePixelT=image::VariancePixel>
struct HeavyFootprintPersistenceHelper {
    afw::table::Schema schema;
    afw::table::Key< afw::table::Array<ImagePixelT> > image;
    afw::table::Key< afw::table::Array<MaskPixelT> > mask;
    afw::table::Key< afw::table::Array<VariancePixelT> > variance;

    static HeavyFootprintPersistenceHelper const & get() {
        static HeavyFootprintPersistenceHelper const instance;
        return instance;
    }

private:

    HeavyFootprintPersistenceHelper() :
        schema(),
        image(schema.addField< afw::table::Array<ImagePixelT> >(
                  "image", "image pixels for HeavyFootprint", "dn"
              )),
        mask(schema.addField< afw::table::Array<MaskPixelT> >(
                 "mask", "mask pixels for HeavyFootprint"
             )),
        variance(schema.addField< afw::table::Array<VariancePixelT> >(
                     "variance", "variance pixels for HeavyFootprint", "dn^2"
                 ))
    {
        schema.getCitizen().markPersistent();
    }

};

// These suffix-computing structs are used to compute the string name associated with a HeavyFootprint
// for Persistence.
// We don't instantiate HeavyFootprints with anything other than defaults for Mask and Variance, so we
// don't bother figuring out what suffixes to use for them for now.  If we change that, we just need
// to add more explicit specializations of this template.
template <typename ImagePixelT,
          typename MaskPixelT=image::MaskPixel,
          typename VariancePixelT=image::VariancePixel>
struct ComputeSuffix;
template <> struct ComputeSuffix<boost::uint16_t> { static std::string apply() { return "U"; } };
template <> struct ComputeSuffix<float> { static std::string apply() { return "F"; } };
template <> struct ComputeSuffix<double> { static std::string apply() { return "D"; } };
template <> struct ComputeSuffix<int> { static std::string apply() { return "I"; } };

} // anonymous

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
std::string HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT>::getPersistenceName() const {
    return "HeavyFootprint" + ComputeSuffix<ImagePixelT,MaskPixelT,VariancePixelT>::apply();
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT>::write(OutputArchiveHandle & handle) const {
    HeavyFootprintPersistenceHelper<ImagePixelT,MaskPixelT,VariancePixelT> const & keys =
        HeavyFootprintPersistenceHelper<ImagePixelT,MaskPixelT,VariancePixelT>::get();
    // delegate to Footprint::write to handle spans and peaks
    Footprint::write(handle);
    // add one more catalog for pixel values
    afw::table::BaseCatalog cat = handle.makeCatalog(keys.schema);
    PTR(afw::table::BaseRecord) record = cat.addNew();
    // We could deep-copy the arrays instead of const-casting them, which might be marginally safer,
    // but we always save an OutputArchive to disk immediately after we create it, so there's really
    // no chance that we could get the HeavyFootprint in trouble by having this view modified.
    record->set(keys.image, ndarray::const_array_cast<ImagePixelT>(getImageArray()));
    record->set(keys.mask, ndarray::const_array_cast<MaskPixelT>(getMaskArray()));
    record->set(keys.variance, ndarray::const_array_cast<VariancePixelT>(getVarianceArray()));
    handle.saveCatalog(cat);
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
class HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT>::Factory :
    public afw::table::io::PersistableFactory
{
public:

    explicit Factory(std::string const & name) : afw::table::io::PersistableFactory(name) {}

    virtual PTR(afw::table::io::Persistable)
    read(InputArchive const & archive, CatalogVector const & catalogs) const {
        HeavyFootprintPersistenceHelper<ImagePixelT,MaskPixelT,VariancePixelT> const & keys =
            HeavyFootprintPersistenceHelper<ImagePixelT,MaskPixelT,VariancePixelT>::get();
        LSST_ARCHIVE_ASSERT(catalogs.size() == 3u);
        PTR(HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT>) result(
            new HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT>()
        );
        result->readSpans(catalogs[0]); // these read methods are inherited from Footprint
        result->readPeaks(catalogs[1]);
        afw::table::BaseRecord const & record = catalogs[2].front();
        result->_image = ndarray::const_array_cast<ImagePixelT>(record.get(keys.image));
        result->_mask = ndarray::const_array_cast<MaskPixelT>(record.get(keys.mask));
        result->_variance = ndarray::const_array_cast<VariancePixelT>(record.get(keys.variance));
        return result;
    }

    static Factory registration;

};

// initialize static instance, registering the factory with the persistence mechanism at the same time
template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
typename HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT>::Factory
HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT>::Factory::registration(
    "HeavyFootprint" + ComputeSuffix<ImagePixelT,MaskPixelT,VariancePixelT>::apply()
);

/************************************************************************************************************/
//
// Explicit instantiations
// \cond
//
//
#define INSTANTIATE(TYPE) \
    template class HeavyFootprint<TYPE>; \
    template PTR(HeavyFootprint<TYPE>) mergeHeavyFootprints<TYPE>( \
        HeavyFootprint<TYPE> const&, HeavyFootprint<TYPE> const&);

INSTANTIATE(boost::uint16_t);
INSTANTIATE(double);
INSTANTIATE(float);
INSTANTIATE(int);

}}}
// \endcond
