// -*- LSST-C++ -*-
/*
 * LSST Data Management System
 * Copyright 2008-2013 LSST Corporation.
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
#ifndef LSST_AFW_DETECTION_Psf_h_INCLUDED
#define LSST_AFW_DETECTION_Psf_h_INCLUDED

#include <string>
#include <limits>

#include <memory>

#include "lsst/pex/exceptions.h"
#include "lsst/utils/CacheFwd.h"
#include "lsst/afw/geom/ellipses/Quadrupole.h"
#include "lsst/afw/math/Kernel.h"
#include "lsst/afw/image/Color.h"
#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/typehandling/Storable.h"

namespace lsst {
namespace afw {
namespace detection {
namespace detail {

/// Key for caching PSFs with lsst::utils::Cache
struct PsfCacheKey;

}  // namespace detail

/**
 *  An exception thrown when we have an invalid PSF.
 */
LSST_EXCEPTION_TYPE(InvalidPsfError, lsst::pex::exceptions::InvalidParameterError, lsst::afw::detection::InvalidPsfError)

/**
 *  A polymorphic base class for representing an image's Point Spread Function
 *
 *  Most of a Psf's functionality involves its evaluation at a position and color, either
 *  or both of which may be unspecified (which will result in evaluation at some average
 *  position or color).  Unlike the closely-related Kernel class, there is no requirement
 *  that a Psf have a well-defined spatial function or any parameters.  Psfs are not
 *  necessarily continuous, and the dimensions of image of the Psf at a point may not be
 *  fixed.
 *
 *  Psfs have two methods for getting at image at a point:
 *   - the image returned by computeImage() is in the same coordinate system as the pixelized image
 *   - the image returned by computeKernelImage() is in an offset coordinate system with the point
 *     P at (0,0); this implies that the image (x0,y0) will be negative
 *
 *  Because P does not need to have integer coordinates, these two images are fractionally offset
 *  from each other and we use interpolation to get (1) from (2).

 *  Psfs are immutable - derived classes should have no non-const methods, and hence
 *  should be fully-defined after construction.  This allows shared_ptrs to Psfs to be
 *  passed around and shared between objects without concern for whether they will be
 *  unexpectedly modified.
 *
 *  In most cases, Psf derived classes should inherit from meas::algorithms::ImagePsf
 *  or meas::algorithms::KernelPsf, as these will provide default implementions for
 *  several member functions.
 */
class Psf : public afw::typehandling::Storable {
    static lsst::geom::Point2D makeNullPoint() {
        return lsst::geom::Point2D(std::numeric_limits<double>::quiet_NaN());
    }
    DECLARE_PERSISTABLE_FACADE(Psf);

public:
    using Pixel = math::Kernel::Pixel;  ///< Pixel type of Image returned by computeImage
    using Image = image::Image<Pixel>;  ///< Image type returned by computeImage

    /// Enum passed to computeImage and computeKernelImage to determine image ownership.
    enum ImageOwnerEnum {
        COPY = 0,    ///< The image will be copied before returning; caller will own it.
        INTERNAL = 1 /**< An internal image will be returned without copying.  The caller must not modify
                      *   it, and it may be invalidated the next time a Psf member function is called with
                      *   different color and/or position.
                      */
    };

    Psf(Psf const&);
    Psf& operator=(Psf const&) = delete;
    Psf& operator=(Psf&&) = delete;

    Psf(Psf&&);
    ~Psf() override;

    /**
     *  Polymorphic deep-copy.
     *
     *  Because Psfs are immutable, clones should generally be unnecessary, but they may
     *  be useful in allowing Psfs to maintain separate caches for their most recently
     *  returned images.
     */
    virtual std::shared_ptr<Psf> clone() const = 0;

    /**
     * @copybrief clone
     *
     * This method is an alias of @ref clone that can be called from a
     * reference to @ref typehandling::Storable "Storable".
     */
    std::shared_ptr<typehandling::Storable> cloneStorable() const override { return clone(); }

    /**
     *  Return clone with specified kernel dimensions
     *
     *  @param[in]  width        Number of columns in pixels
     *  @param[in]  height       Number of rows in pixels
     *
     *  Must be implemented by derived classes.
     */
    virtual std::shared_ptr<Psf> resized(int width, int height) const = 0;

    /**
     *  Return an Image of the PSF, in a form that can be compared directly with star images.
     *
     *  The specified position is a floating point number, and the resulting image will have a Psf
     *  centered on that point when the returned image's xy0 is taken into account.
     *
     *  The returned image is normalized to sum to unity.
     *
     *  @param[in]  position     Position at which to evaluate the PSF.
     *  @param[in]  color        Color of the source for which to evaluate the PSF; defaults to
     *                           getAverageColor().
     *  @param[in]  owner        Whether to copy the return value or return an internal image that
     *                           must be handled with care (see ImageOwnerEnum).
     *
     *  The Psf class caches the most recent return value of computeImage, so repeated calls
     *  with the same arguments will be highly optimized.
     *
     *  @note The real work is done in the virtual private member function Psf::doComputeImage;
     *        computeImage only handles caching and default arguments.
     */
    std::shared_ptr<Image> computeImage(lsst::geom::Point2D position,
                                        image::Color color = image::Color(),
                                        ImageOwnerEnum owner = COPY) const;


    /**
     *  Return an Image of the PSF, in a form suitable for convolution.
     *
     *  While the position need not be an integer, the center of the PSF image returned by
     *  computeKernelImage will in the center of the center pixel of the image, which will be
     *  (0,0) when the Image's xy0 is taken into account; this is the same behavior as
     *  Kernel::computeImage().
     *
     *  The returned image is normalized to sum to unity.
     *
     *  @param[in]  position     Position at which to evaluate the PSF.
     *  @param[in]  color        Color of the source for which to evaluate the PSF; defaults to
     *                           getAverageColor().
     *  @param[in]  owner        Whether to copy the return value or return an internal image that
     *                           must be handled with care (see ImageOwnerEnum).
     *
     *  The Psf class caches the most recent return value of computeKernelImage, so repeated calls
     *  with the same arguments will be highly optimized.
     *
     *  @note The real work is done in the virtual private member function Psf::doComputeKernelImage;
     *        computeKernelImage only handles caching and default arguments.
     */
    std::shared_ptr<Image> computeKernelImage(lsst::geom::Point2D position,
                                              image::Color color = image::Color(),
                                              ImageOwnerEnum owner = COPY) const;

    /**
     *   Return the peak value of the PSF image.
     *
     *  @param[in]  position     Position at which to evaluate the PSF.
     *  @param[in]  color        Color of the source for which to evaluate the PSF; defaults to
     *                           getAverageColor().
     *
     *  This calls computeKernelImage internally, but because this will usually be cached, it shouldn't
     *  be expensive (but be careful not to accidentally call it with no arguments when you actually
     *  want to call it with the same arguments just used to call computeImage or computeKernelImage).
     */
    double computePeak(lsst::geom::Point2D position,
                       image::Color color = image::Color()) const;

    /**
     *  Compute the "flux" of the Psf model within a circular aperture of the given radius.
     *
     *  @param[in]  radius       Radius of the aperture to measure.
     *  @param[in]  position     Position at which to evaluate the PSF.
     *  @param[in]  color        Color of the source for which to evaluate the PSF; defaults to
     *                           getAverageColor().
     *
     *  The flux is relative to a Psf image that has been normalized to unit integral, and the radius
     *  is in pixels.
     */
    double computeApertureFlux(double radius, lsst::geom::Point2D position,
                               image::Color color = image::Color()) const;

    /**
     *  Compute the ellipse corresponding to the second moments of the Psf.
     *
     *  @param[in]  position     Position at which to evaluate the PSF.
     *  @param[in]  color        Color of the source for which to evaluate the PSF; defaults to
     *                           getAverageColor().
     *
     *  The algorithm used to compute the moments is up to the derived class, and hence this
     *  method should not be used when a particular algorithm or weight function is required.
     */
    geom::ellipses::Quadrupole computeShape(lsst::geom::Point2D position,
                                            image::Color color = image::Color()) const;

    /**
     *  Return a FixedKernel corresponding to the Psf image at the given point.
     *
     *  @param[in]  position     Position at which to evaluate the PSF.
     *  @param[in]  color        Color of the source for which to evaluate the PSF; defaults to
     *                           getAverageColor().
     *
     *  This is implemented by calling computeKernelImage, and is simply provided for
     *  convenience.
     */
    std::shared_ptr<math::Kernel const> getLocalKernel(lsst::geom::Point2D position,
                                                       image::Color color = image::Color()) const;


    /**
     *  Return the average Color of the stars used to construct the Psf
     *
     *  This is also the Color used to return an image if you don't specify a Color.
     */
    image::Color getAverageColor() const { return image::Color(); }

    /**
     *  Return the average position of the stars used to construct the Psf.
     *
     *  This is also the position used to return an image if you don't specify a position.
     */
    virtual lsst::geom::Point2D getAveragePosition() const;

    /**
     *  Return the bounding box of the image returned by computeKernelImage()
     *
     *  @param[in]  position     Position at which to evaluate the PSF.
     *  @param[in]  color        Color of the source for which to evaluate the PSF; defaults to
     *                           getAverageColor().
     */
    lsst::geom::Box2I computeBBox(lsst::geom::Point2D position,
                                  image::Color color = image::Color()) const;

    /**
     *  Return the bounding box of the image returned by computeImage()
     *
     *  @param[in]  position     Position at which to evaluate the PSF.
     *  @param[in]  color        Color of the source for which to evaluate the PSF; defaults to
     *                           getAverageColor().
     */
    lsst::geom::Box2I computeImageBBox(lsst::geom::Point2D position,
                                       image::Color color = image::Color()) const;

    /**
     *  Return the bounding box of the image returned by computeImage()
     *
     *  @param[in]  position     Position at which to evaluate the PSF.
     *  @param[in]  color        Color of the source for which to evaluate the PSF; defaults to
     *                           getAverageColor().
     *  Alias for computeBBox
     */
    lsst::geom::Box2I computeKernelBBox(lsst::geom::Point2D position,
                                        image::Color color = image::Color()) const {
        return computeBBox(position, color);
    }

    /**
     * Helper function for Psf::doComputeImage(): converts a kernel image (centered at (0,0) when xy0
     * is taken into account) to an image centered at position when xy0 is taken into account.
     *
     * `warpAlgorithm` is passed to afw::math::makeWarpingKernel() and can be "nearest", "bilinear",
     * or "lanczosN"
     *
     * `warpBuffer` zero-pads the image before recentering.  Recommended value is 1 for bilinear,
     * N for lanczosN (note that it would be cleaner to infer this value from the warping algorithm
     * but this would require mild API changes; same issue occurs in e.g. afw::math::offsetImage()).
     *
     * The point with integer coordinates `(0,0)` in the source image (with xy0 taken into account)
     * corresponds to the point `position` in the destination image.  If `position` is not
     * integer-valued then we will need to fractionally shift the image using interpolation.
     *
     * Note: if fractional recentering is performed, then a new image will be allocated and returned.
     * If not, then the original image will be returned (after setting XY0).
     */
    static std::shared_ptr<Image> recenterKernelImage(std::shared_ptr<Image> im,
                                                      lsst::geom::Point2D const& position,
                                                      std::string const& warpAlgorithm = "lanczos5",
                                                      unsigned int warpBuffer = 5);

    /** Return the capacity of the caches
     *
     * Both the image and kernel image caches have the same capacity.
     */
    std::size_t getCacheCapacity() const;

    /** Set the capacity of the caches
     *
     * Both the image and kernel image caches will be set to this capacity.
     */
    void setCacheCapacity(std::size_t capacity);

protected:
    /**
     *  Main constructor for subclasses.
     *
     *  @param[in] isFixed  Should be true for Psf for which doComputeKernelImage always returns
     *                      the same image, regardless of color or position arguments.
     *  @param[in] capacity  Capacity of the caches.
     */
    explicit Psf(bool isFixed = false, std::size_t capacity = 100);

    //@{
    /**
     * These virtual members are protected (rather than private) so that python-implemented derived
     * classes may opt to use the default implementations.  C++ derived classes may override these
     * methods, but should not call them.  Derived classes should call the corresponding compute*
     * member functions instead so as to let the Psf base class handle caching properly.
     *
     * Derived classes are responsible for ensuring that returned images sum to one.
     */
    virtual std::shared_ptr<Image> doComputeImage(lsst::geom::Point2D const& position,
                                                  image::Color const& color) const;
    virtual lsst::geom::Box2I doComputeImageBBox(lsst::geom::Point2D const& position,
                                                 image::Color const& color) const;
    //@}

private:
    //@{
    /**
     *  These virtual member functions are private, not protected, because we only want derived classes
     *  to implement them, not call them; they should call the corresponding compute*Image member
     *  functions instead so as to let the Psf base class handle caching properly.
     *
     *  Derived classes are responsible for ensuring that returned images sum to one.
     */
    virtual std::shared_ptr<Image> doComputeKernelImage(lsst::geom::Point2D const& position,
                                                        image::Color const& color) const = 0;
    virtual double doComputeApertureFlux(double radius, lsst::geom::Point2D const& position,
                                         image::Color const& color) const = 0;
    virtual geom::ellipses::Quadrupole doComputeShape(lsst::geom::Point2D const& position,
                                                      image::Color const& color) const = 0;
    virtual lsst::geom::Box2I doComputeBBox(lsst::geom::Point2D const& position,
                                            image::Color const& color) const = 0;
    //@}

    bool const _isFixed;
    using PsfCache = utils::Cache<detail::PsfCacheKey, std::shared_ptr<Image>>;
    std::unique_ptr<PsfCache> _imageCache;
    std::unique_ptr<PsfCache> _kernelImageCache;
};
}  // namespace detection
}  // namespace afw
}  // namespace lsst

#endif  // !LSST_AFW_DETECTION_Psf_h_INCLUDED
