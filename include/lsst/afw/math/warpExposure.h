// -*- LSST-C++ -*- // fixed format comment for emacs

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

/*
 * Support for warping an image to a new WCS.
 */

#ifndef LSST_AFW_MATH_WARPEXPOSURE_H
#define LSST_AFW_MATH_WARPEXPOSURE_H

#include <memory>
#include <string>

#include "lsst/base.h"
#include "lsst/pex/exceptions.h"
#include "lsst/geom.h"
#include "lsst/afw/geom.h"
#include "lsst/afw/image/LsstImageTypes.h"
#include "lsst/afw/image/Exposure.h"
#include "lsst/afw/math/ConvolveImage.h"
#include "lsst/afw/math/Function.h"
#include "lsst/afw/math/FunctionLibrary.h"
#include "lsst/afw/math/Kernel.h"
#include "lsst/afw/table/io/Persistable.h"

namespace lsst {
namespace afw {
namespace geom {
class SkyWcs;
}
namespace math {

/**
 * Lanczos warping: accurate but slow and can introduce ringing artifacts.
 *
 * This kernel is the product of two 1-dimensional Lanczos functions.
 * The number of minima and maxima in the 1-dimensional Lanczos function is 2*order + 1.
 * The kernel has one pixel per function minimum or maximum; but as applied to warping,
 * the first or last pixel is always zero and can be omitted. Thus the kernel size is 2*order x 2*order.
 *
 * For more information about warping kernels see makeWarpingKernel
 *
 * @todo: make a new class WarpingKernel and make this a subclass.
 */
class LanczosWarpingKernel : public SeparableKernel {
public:
    explicit LanczosWarpingKernel(int order  ///< order of Lanczos function
                                  )
            : SeparableKernel(2 * order, 2 * order, LanczosFunction1<Kernel::Pixel>(order),
                              LanczosFunction1<Kernel::Pixel>(order)) {}

    LanczosWarpingKernel(const LanczosWarpingKernel &) = delete;
    LanczosWarpingKernel(LanczosWarpingKernel &&) = delete;
    LanczosWarpingKernel &operator=(const LanczosWarpingKernel &) = delete;
    LanczosWarpingKernel &operator=(LanczosWarpingKernel &&) = delete;

    ~LanczosWarpingKernel() override = default;

    std::shared_ptr<Kernel> clone() const override;

    /**
     * get the order of the kernel
     */
    int getOrder() const;

    bool isPersistable() const noexcept override { return true; }

protected:
    void setKernelParameter(unsigned int ind, double value) const override;

    std::string getPersistenceName() const override { return "LanczosWarpingKernel"; }

    std::string getPythonModule() const override { return "lsst.afw.math"; }

    void write(OutputArchiveHandle &handle) const override;
};

/**
 * Bilinear warping: fast; good for undersampled data.
 *
 * The kernel size is 2 x 2.
 *
 * For more information about warping kernels see makeWarpingKernel
 *
 * @todo: make a new class WarpingKernel and make this a subclass.
 */
class BilinearWarpingKernel : public SeparableKernel {
public:
    explicit BilinearWarpingKernel()
            : SeparableKernel(2, 2, BilinearFunction1(0.0), BilinearFunction1(0.0)) {}

    BilinearWarpingKernel(const BilinearWarpingKernel &) = delete;
    BilinearWarpingKernel(BilinearWarpingKernel &&) = delete;
    BilinearWarpingKernel &operator=(const BilinearWarpingKernel &) = delete;
    BilinearWarpingKernel &operator=(BilinearWarpingKernel &&) = delete;

    ~BilinearWarpingKernel() override = default;

    std::shared_ptr<Kernel> clone() const override;

    bool isPersistable() const noexcept override { return true; }

    /**
     * 1-dimensional bilinear interpolation function.
     *
     * Optimized for bilinear warping so only accepts two values: 0 and 1
     * (which is why it defined in the BilinearWarpingKernel class instead of
     * being made available as a standalone function).
     */
    class BilinearFunction1 : public Function1<Kernel::Pixel> {
    public:
        using Function1Ptr = std::shared_ptr<Function1<Kernel::Pixel>>;

        /**
         * Construct a Bilinear interpolation function
         */
        explicit BilinearFunction1(double fracPos)  ///< fractional position; must be >= 0 and < 1
                : Function1<Kernel::Pixel>(1) {
            this->_params[0] = fracPos;
        }
        ~BilinearFunction1() override {}

        Function1Ptr clone() const override { return Function1Ptr(new BilinearFunction1(this->_params[0])); }

        /**
         * Solve bilinear equation
         *
         * Only the following arguments will give reliably meaningful values:
         * *  0.0 or 1.0 if the kernel center index is 0 in this axis
         * * -1.0 or 0.0 if the kernel center index is 1 in this axis
         */
        Kernel::Pixel operator()(double x) const override;

        /**
         * Return string representation.
         */
        std::string toString(std::string const & = "") const override;
    };

protected:
    void setKernelParameter(unsigned int ind, double value) const override;

    std::string getPersistenceName() const override { return "BillinearWarpingKernel"; }

    std::string getPythonModule() const override { return "lsst.afw.math"; }

    void write(OutputArchiveHandle &handle) const override;
};

/**
 * Nearest neighbor warping: fast; good for undersampled data.
 *
 * The kernel size is 2 x 2.
 *
 * For more information about warping kernels see makeWarpingKernel
 *
 * @todo: make a new class WarpingKernel and make this a subclass.
 */
class NearestWarpingKernel : public SeparableKernel {
public:
    explicit NearestWarpingKernel() : SeparableKernel(2, 2, NearestFunction1(0.0), NearestFunction1(0.0)) {}

    NearestWarpingKernel(const NearestWarpingKernel &) = delete;
    NearestWarpingKernel(NearestWarpingKernel &&) = delete;
    NearestWarpingKernel &operator=(const NearestWarpingKernel &) = delete;
    NearestWarpingKernel &operator=(NearestWarpingKernel &&) = delete;

    ~NearestWarpingKernel() override = default;

    std::shared_ptr<Kernel> clone() const override;

    bool isPersistable() const noexcept override { return true; }

    /**
     * 1-dimensional nearest neighbor interpolation function.
     *
     * Optimized for nearest neighbor warping so only accepts two values: 0 and 1
     * (which is why it defined in the NearestWarpingKernel class instead of
     * being made available as a standalone function).
     */
    class NearestFunction1 : public Function1<Kernel::Pixel> {
    public:
        using Function1Ptr = std::shared_ptr<Function1<Kernel::Pixel>>;

        /**
         * Construct a Nearest interpolation function
         */
        explicit NearestFunction1(double fracPos)  ///< fractional position
                : Function1<Kernel::Pixel>(1) {
            this->_params[0] = fracPos;
        }
        ~NearestFunction1() override {}

        Function1Ptr clone() const override { return Function1Ptr(new NearestFunction1(this->_params[0])); }

        /**
         * Solve nearest neighbor equation
         *
         * Only the following arguments will give reliably meaningful values:
         * *  0.0 or 1.0 if the kernel center index is 0 in this axis
         * * -1.0 or 0.0 if the kernel center index is 1 in this axis
         */
        Kernel::Pixel operator()(double x) const override;

        /**
         * Return string representation.
         */
        std::string toString(std::string const & = "") const override;
    };

protected:
    void setKernelParameter(unsigned int ind, double value) const override;

    std::string getPersistenceName() const override { return "NearestWarpingKernel"; }

    std::string getPythonModule() const override { return "lsst.afw.math"; }

    void write(OutputArchiveHandle &handle) const override;
};

/**
 * Return a warping kernel given its name.
 *
 * Intended for use with warpImage() and warpExposure().
 *
 * Allowed names are:
 * - bilinear: return a BilinearWarpingKernel
 * - lanczos#: return a LanczosWarpingKernel of order #, e.g. lanczos4
 * - nearest: return a NearestWarpingKernel
 *
 * A warping kernel is a subclass of SeparableKernel with the following properties
 * (though for the sake of speed few, if any, of these are enforced):
 * - Width and height are even. This is unusual for a kernel, but it is more efficient
 *   because if the extra pixel was included it would always have value 0.
 * - The center pixels should be adjacent to the kernel center.
 *   Again, this avoids extra pixels that are sure to have value 0.
 * - It has two parameters: fractional x and fractional row position on the source %image.
 *   The fractional position is the offset of the pixel position on the source
 *   from the center of a nearby source pixel:
 *   - The pixel whose center is just below or to the left of the source position:
 *     0 <= fractional x and y < 0 and the kernel center is the default (size-1)/2.
 *   - The pixel whose center is just above or to the right of the source position:
 *     -1.0 < fractional x and y <= 0 and the kernel center must be set to (size+1)/2.
 */
std::shared_ptr<SeparableKernel> makeWarpingKernel(std::string name);

/**
 * Parameters to control convolution
 *
 * @note padValue is not member of this class to avoid making this a templated class.
 *
 * @ingroup afw
 */
class WarpingControl final : public table::io::Persistable {
DECLARE_PERSISTABLE_FACADE(WarpingControl);
public:
    /**
     * Construct a WarpingControl object
     *
     * @throws pex::exceptions::InvalidParameterError if the warping kernel
     * is smaller than the mask warping kernel.
     */
    explicit WarpingControl(
            std::string const &warpingKernelName,  ///< name of warping kernel;
            ///< used as the argument to makeWarpingKernel
            std::string const &maskWarpingKernelName = "",  ///< name of warping kernel used for
            ///< the mask plane; if "" then the regular warping kernel is used.
            ///< Intended so one can use a bilinear kernel or other compact kernel for the mask plane
            ///< to avoid smearing mask bits too far. The theory is that bad pixels are already
            ///< interpolated over, so we don't need to worry about bad values spreading very far.
            int cacheSize = 0,  ///< cache size for warping kernel; no cache if 0
            ///< (used as the argument to the warping kernels' computeCache method)
            int interpLength = 0,  ///< distance over which the WCS can be linearly interpolated
            lsst::afw::image::MaskPixel growFullMask = 0
            ///< mask bits to grow to full width of image/variance kernel
            )
            : _warpingKernelPtr(makeWarpingKernel(warpingKernelName)),
              _maskWarpingKernelPtr(),
              _cacheSize(cacheSize),
              _interpLength(interpLength),
              _growFullMask(growFullMask) {
        setMaskWarpingKernelName(maskWarpingKernelName);
    }

    virtual ~WarpingControl(){};

    /**
     * get the cache size for the interpolation kernel(s)
     */
    int getCacheSize() const { return _cacheSize; };

    /**
     * set the cache size for the interpolation kernel(s)
     *
     * A value of 0 disables the cache for maximum accuracy.
     * 10,000 typically results in a warping error of a fraction of a count.
     * 100,000 typically results in a warping error of less than 0.01 count.
     * Note the new cache is not computed until getWarpingKernel or getMaskWarpingKernel is called.
     */
    void setCacheSize(int cacheSize  ///< cache size
    ) {
        _cacheSize = cacheSize;
    };

    /**
     * get the interpolation length (pixels)
     */
    int getInterpLength() const { return _interpLength; };

    /**
     * set the interpolation length
     *
     * Interpolation length is the distance over which the WCS can be linearly interpolated, in pixels:
     * * 0 means no interpolation and uses an optimized branch of the code
     * * 1 also performs no interpolation but it runs the interpolation code branch
     *   (and so is only intended for unit tests)
     */
    void setInterpLength(int interpLength  ///< interpolation length (pixels)
    ) {
        _interpLength = interpLength;
    };

    /**
     * get the warping kernel
     */
    std::shared_ptr<SeparableKernel> getWarpingKernel() const;

    /**
     * set the warping kernel by name
     */
    void setWarpingKernelName(std::string const &warpingKernelName  ///< name of warping kernel
    );

    /**
     * set the warping kernel
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if new kernel pointer is empty.
     */
    void setWarpingKernel(SeparableKernel const &warpingKernel  ///< warping kernel
    );

    /**
     * get the mask warping kernel
     */
    std::shared_ptr<SeparableKernel> getMaskWarpingKernel() const;

    /**
     * return true if there is a mask kernel
     */
    bool hasMaskWarpingKernel() const { return static_cast<bool>(_maskWarpingKernelPtr); }

    /**
     * set or clear the mask warping kernel by name
     */
    void setMaskWarpingKernelName(std::string const &maskWarpingKernelName
                                  ///< name of mask warping kernel; use "" to clear the kernel
    );

    /**
     * set the mask warping kernel
     *
     * @note To clear the mask warping kernel use setMaskWarpingKernelName("").
     */
    void setMaskWarpingKernel(SeparableKernel const &maskWarpingKernel  ///< mask warping kernel
    );

    /**
     * get mask bits to grow to full width of image/variance kernel
     */
    lsst::afw::image::MaskPixel getGrowFullMask() const { return _growFullMask; };

    /**
     * set mask bits to grow to full width of image/variance kernel
     */
    void setGrowFullMask(lsst::afw::image::MaskPixel growFullMask  ///< mask bits to grow to full width
                                                                   ///< of image/variance kernel
    ) {
        _growFullMask = growFullMask;
    }

    bool isPersistable() const noexcept override;

protected:
    std::string getPersistenceName() const override;
    std::string getPythonModule() const override;
    void write(OutputArchiveHandle &handle) const override;

private:
    /**
     * Throw an exception if the two kernels are not compatible in shape
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if the two kernels
     * are not compatible in shape
     */
    void _testWarpingKernels(SeparableKernel const &warpingKernel,     ///< warping kernel
                             SeparableKernel const &maskWarpingKernel  ///< mask warping kernel
    ) const;

    std::shared_ptr<SeparableKernel> _warpingKernelPtr;
    std::shared_ptr<SeparableKernel> _maskWarpingKernelPtr;
    int _cacheSize;
    int _interpLength;
    lsst::afw::image::MaskPixel _growFullMask;
};

/**
 * Warp (remap) one exposure to another.
 *
 * This is a convenience wrapper around warpImage().
 */
template <typename DestExposureT, typename SrcExposureT>
int warpExposure(
        DestExposureT &destExposure,  ///< Remapped exposure. Wcs and xy0 are read, MaskedImage is set,
                                      ///< and PhotoCalib, FilterLabel and VisitInfo are copied from
                                      ///< srcExposure.
                                      ///< All other attributes are left alone (including Detector and Psf)
        SrcExposureT const &srcExposure,  ///< Source exposure
        WarpingControl const &control,    ///< control parameters
        typename DestExposureT::MaskedImageT::SinglePixel padValue =
                lsst::afw::math::edgePixel<typename DestExposureT::MaskedImageT>(
                        typename lsst::afw::image::detail::image_traits<
                                typename DestExposureT::MaskedImageT>::image_category())
        ///< use this value for undefined (edge) pixels
);

/**
 * @brief Warp an Image or MaskedImage to a new Wcs. See also convenience function
 * warpExposure() to warp an Exposure.
 *
 * Edge pixels are set to padValue; these are pixels that cannot be computed because they
 * are too near the edge of srcImage or miss srcImage entirely.
 *
 * @returns the number of valid pixels in destImage (those that are not edge pixels).
 *
 * @b Algorithm Without Interpolation:
 *
 * For each integer pixel position in the remapped Exposure:
 * - The associated pixel position on srcImage is determined using the destination and source WCS
 * - The warping kernel's parameters are set based on the fractional part of the pixel position on srcImage
 * - The warping kernel is applied to srcImage at the integer portion of the pixel position
 *   to compute the remapped pixel value
 * - A flux-conservation factor is determined from the source and destination WCS
 *   and is applied to the remapped pixel
 *
 * The scaling of intensity for relative area of source and destination uses two minor approximations:
 * - The area of the sky marked out by a pixel on the destination %image
 *   corresponds to a parallellogram on the source %image.
 * - The area varies slowly enough across the %image that we can get away with computing
 *   the source area shifted by half a pixel up and to the left of the true area.
 *
 * @b Algorithm With Interpolation:
 *
 * Interpolation simply reduces the number of times WCS is used to map between destination and source
 * pixel position. This computation is only made at a grid of points on the destination image,
 * separated by interpLen pixels along rows and columns. All other source pixel positions are determined
 * by linear interpolation between those grid points. Everything else remains the same.
 *
 * @throws lsst::pex::exceptions::InvalidParameterError if destImage overlaps srcImage
 * @throws std::bad_alloc when allocation of CPU memory fails
 *
 * @todo Should support an additional color-based position correction in the remapping
 *   (differential chromatic refraction). This can be done either object-by-object or pixel-by-pixel.
 *
 * @todo Need to deal with oversampling and/or weight maps. If done we can use faster kernels than sinc.
 *
 * @warning The code that tests for image overlap is not guranteed to work correctly, based on the C++
 * standard. It is, in theory, possible for the code to report a "false positive", meaning that it may claim
 * that images overlap when they do not. We don't believe that any of our current compilers have this problem.
 * If, in the future, this becomes a problem then we will probably have to remove the test and rely on users
 * being careful.
 */
template <typename DestImageT, typename SrcImageT>
int warpImage(DestImageT &destImage,          ///< remapped %image
              geom::SkyWcs const &destWcs,    ///< WCS of remapped %image
              SrcImageT const &srcImage,      ///< source %image
              geom::SkyWcs const &srcWcs,     ///< WCS of source %image
              WarpingControl const &control,  ///< control parameters
              typename DestImageT::SinglePixel padValue = lsst::afw::math::edgePixel<DestImageT>(
                      typename lsst::afw::image::detail::image_traits<DestImageT>::image_category())
              ///< use this value for undefined (edge) pixels
);

/**
 * @brief A variant of warpImage that uses a Transform<Point2Endpoint, Point2Endpoint>
 * instead of a pair of WCS to describe the transformation.
 *
 * @param[in,out] destImage  Destination image; all pixels are set
 * @param[in] srcImage  Source image
 * @param[in] srcToDest  Transformation from source to destination pixels, in parent coordinates;
 *    the inverse must be defined (and is the only direction used).
 *    makeWcsPairTransform(srcWcs, destWcs) is one way to compute this transform.
 * @param[in] control  Warning control parameters
 * @param[in] padValue  Value used for pixels in the destination image that are outside
 *   the region of pixels that can be computed from the source image
 * @return the number of good pixels
 */
template <typename DestImageT, typename SrcImageT>
int warpImage(DestImageT &destImage, SrcImageT const &srcImage,
              geom::TransformPoint2ToPoint2 const &srcToDest, WarpingControl const &control,
              typename DestImageT::SinglePixel padValue = lsst::afw::math::edgePixel<DestImageT>(
                      typename lsst::afw::image::detail::image_traits<DestImageT>::image_category()));

/**
 * Warp an image with a LinearTranform about a specified point.
 *
 * This enables warping an image of e.g. a PSF without translating the centroid.
 */
template <typename DestImageT, typename SrcImageT>
int warpCenteredImage(
        DestImageT &destImage,                               ///< remapped %image
        SrcImageT const &srcImage,                           ///< source %image
        lsst::geom::LinearTransform const &linearTransform,  ///< linear transformation to apply
        lsst::geom::Point2D const &centerPosition,  ///< pixel position for location of linearTransform
        WarpingControl const &control,              ///< control parameters
        typename DestImageT::SinglePixel padValue = lsst::afw::math::edgePixel<DestImageT>(
                typename lsst::afw::image::detail::image_traits<DestImageT>::image_category())
        ///< use this value for undefined (edge) pixels
);

}  // namespace math
}  // namespace afw
}  // namespace lsst

#endif  // !defined(LSST_AFW_MATH_WARPEXPOSURE_H)
