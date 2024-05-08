// -*- LSST-C++ -*-

/*
 * LSST Data Management System
 * Copyright 2008-2016  AURA/LSST.
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

#ifndef LSST_AFW_MATH_KERNEL_H
#define LSST_AFW_MATH_KERNEL_H
/*
 * Declare the Kernel class and subclasses.
 */
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "boost/format.hpp"

#include "lsst/geom.h"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/math/Function.h"
#include "lsst/afw/math/traits.h"

#include "lsst/afw/table/io/Persistable.h"

namespace lsst {
namespace afw {

namespace math {

/**
 * Kernels are used for convolution with MaskedImages and (eventually) Images
 *
 * Kernel is a virtual base class; it cannot be instantiated. The following notes apply to
 * Kernel and to its subclasses.
 *
 * The template type should usually be float or double; integer kernels
 * should be used with caution because they do not normalize well.
 *
 * The center pixel of a Kernel is at index: (width-1)/2, (height-1)/2. Thus it is centered along
 * columns/rows if the kernel has an odd number of columns/rows and shifted 1/2 pixel towards 0 otherwise.
 * A kernel should have an odd number of columns and rows unless it is intended to shift an image.
 *
 * <b>Spatially Varying Kernels</b>
 *
 * Kernels may optionally vary spatially (so long as they have any kernel parameters).
 * To make a spatially varying kernel, specify a spatial function at construction
 * (you cannot change your mind once the kernel is constructed).
 * You must also specify a set of spatial parameters, and you may do this at construction
 * and/or later by calling setSpatialParameters. The spatial parameters are a vector
 * (one per kernel function parameter) of spatial function parameters. In other words
 * the spatial parameters are a vector of vectors indexed as [kernel parameter][spatial parameter].
 * The one spatial function is used to compute the kernel parameters at a given spatial position
 * by computing each kernel parameter using its associated vector of spatial function parameters.
 *
 * The convolve function computes the spatial function at the pixel position (not index) of the image.
 * See the convolve function for details.
 *
 * Note that if a kernel is spatially varying then you may not set the kernel parameters directly;
 * that is the job of the spatial function! However, you may change the spatial parameters at any time.
 *
 * <b>Design Notes</b>
 *
 * The basic design is to use the same kernel class for both spatially varying and spatially invariant
 * kernels. The user either does or does not supply a function describing the spatial variation
 * at creation time. In addition, analytic kernels are described by a user-supplied function
 * of the same basic type as the spatial variation function.
 *
 * Several other designs were considered, including:
 * A) Use different classes for spatially varying and spatially invariant versions of each kernel.
 * Thus instead of three basic kernel classes (FixedKernel, AnalyticKernel and LinearCombinationKernel)
 * we would have five (since FixedKernel cannot be spatially varying). Robert Lupton argued that
 * was a needless expansion of the class hiearchy and I agreed.
 * B) Construct analytic kernels by defining a subclass of AnalyticKernel that is specific to the
 * desired functional (e.g. GaussianAnalyticKernel). If spatial models are handled the same way
 * then this creates a serious proliferation of kernel classes (even if we only have two different
 * spatial models, e.g. polynomial and Chebyshev polynomial). I felt it made more sense to define
 * the spatial model by some kind of function class (often called a "functor"), and since we needed
 * such a class, I chose to use it for the analytic kernel as well.
 *
 * However, having a separate function class does introduce some potential inefficiencies.
 * If a function is part of the class it can potentially be evaluated more quickly than calling
 * a function for each pixel or spatial position.
 *
 * A possible variant on the current design is to define the spatial model and analytic kernel
 * by specifying the functions as template parameters. This has the potential to regain some efficiency
 * in evaluating the functions. However, it would be difficult or impossible to pre-instantiate
 * the desired template classes, a requirement of the LSST coding standards.
 *
 * @ingroup afw
 */
class Kernel : public afw::table::io::Persistable {
DECLARE_PERSISTABLE_FACADE(Kernel);
public:
    using Pixel = double;
    using SpatialFunctionPtr = std::shared_ptr<lsst::afw::math::Function2<double>>;
    using SpatialFunction = lsst::afw::math::Function2<double>;
    using NullSpatialFunction = lsst::afw::math::NullFunction2<double>;

    // Traits values for this class of Kernel
    using kernel_fill_factor = generic_kernel_tag;

    /**
     * Construct a null Kernel of size 0,0.
     *
     * A null constructor is primarily intended for persistence.
     */
    explicit Kernel();

    /**
     * @brief Construct a spatially invariant Kernel or a spatially varying Kernel with one spatial function
     * that is duplicated as needed.
     *
     * @param width number of columns
     * @param height number of height
     * @param nKernelParams number of kernel parameters
     * @param spatialFunction spatial function, or NullSpatialFunction() if none specified
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if a spatial function is specified
     * and the kernel has no parameters.
     * @throws lsst::pex::exceptions::InvalidParameterError if a width or height < 1
     */
    explicit Kernel(int width, int height, unsigned int nKernelParams,
                    SpatialFunction const &spatialFunction = NullSpatialFunction());

    /**
     * Construct a spatially varying Kernel with a list of spatial functions (one per kernel parameter)
     *
     * Note: if the list of spatial functions is empty then the kernel is not spatially varying.
     *
     * @param width number of columns
     * @param height number of height
     * @param spatialFunctionList list of spatial function, one per kernel parameter
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if a width or height < 1
     */
    explicit Kernel(int width, int height, const std::vector<SpatialFunctionPtr> spatialFunctionList);

    // prevent copying and assignment (to avoid problems from type slicing)
    Kernel(const Kernel &) = delete;
    Kernel(Kernel &&) = delete;
    Kernel &operator=(const Kernel &) = delete;
    Kernel &operator=(Kernel &&) = delete;

    ~Kernel() override = default;

    /**
     * Return a pointer to a deep copy of this kernel
     *
     * This kernel exists instead of a copy constructor
     * so one can obtain a copy of an actual kernel
     * instead of a useless copy of the base class.
     *
     * Every kernel subclass must override this method.
     *
     * @returns a pointer to a deep copy of the kernel
     */
    virtual std::shared_ptr<Kernel> clone() const = 0;

    /**
     * Return a pointer to a clone with specified kernel dimensions
     *
     * @param width  Number of columns in pixels
     * @param height Number of rows in pixels
     *
     * @returns a pointer to a clone with new dimensions.
     *
     * Must be implemented by derived classes.
     */
    virtual std::shared_ptr<Kernel> resized(int width, int height) const = 0;

    /**
     * Compute an image (pixellized representation of the kernel) in place
     *
     * @param image image whose pixels are to be set (output); xy0 of the image will be
     *              set to -kernel.getCtr()
     * @param doNormalize normalize the image (so sum is 1)?
     * @param x x (column position) at which to compute spatial function
     * @param y y (row position) at which to compute spatial function
     *
     * @returns The kernel sum
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if the image is the wrong size
     * @throws lsst::pex::exceptions::OverflowError if doNormalize is true and the kernel
     *                                              sum is exactly 0
     *
     * @note computeNewImage has been retired; it doesn't need to be a member
     */
    double computeImage(lsst::afw::image::Image<Pixel> &image, bool doNormalize, double x = 0.0,
                        double y = 0.0) const;

    /**
     * Return the Kernel's dimensions (width, height)
     */
    lsst::geom::Extent2I const getDimensions() const { return lsst::geom::Extent2I(_width, _height); }

    void setDimensions(lsst::geom::Extent2I dims) {
        _width = dims.getX();
        _height = dims.getY();
    }
    inline void setWidth(int width) { _width = width; }
    inline void setHeight(int height) { _height = height; }

    /**
     * Return the Kernel's width
     */
    inline int getWidth() const { return _width; }

    /**
     * Return the Kernel's height
     */
    inline int getHeight() const { return _height; }

    /**
     * Return index of kernel's center
     */
    inline lsst::geom::Point2I getCtr() const { return lsst::geom::Point2I(_ctrX, _ctrY); }

    /**
     * return parent bounding box, with XY0 = -center
     */
    inline lsst::geom::Box2I getBBox() const {
        return lsst::geom::Box2I(lsst::geom::Point2I(-_ctrX, -_ctrY), lsst::geom::Extent2I(_width, _height));
    }

    /**
     * Return the number of kernel parameters (0 if none)
     */
    inline unsigned int getNKernelParameters() const { return _nKernelParams; }

    /**
     * Return the number of spatial parameters (0 if not spatially varying)
     */
    inline int getNSpatialParameters() const {
        return this->isSpatiallyVarying() ? _spatialFunctionList[0]->getNParameters() : 0;
    }

    /**
     * Return a clone of the specified spatial function (one component of the spatial model)
     *
     * @param index index of desired spatial function; must be in range [0, number spatial parameters - 1]
     * @returns a shared pointer to a spatial function. The function is a deep copy, so setting its parameters
     * has no effect on the kernel.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if kernel not spatially varying
     * @throws lsst::pex::exceptions::InvalidParameterError if index out of range
     */
    SpatialFunctionPtr getSpatialFunction(unsigned int index) const;

    /**
     * Return a list of clones of the spatial functions.
     *
     * @returns a list of shared pointers to spatial functions. The functions are deep copies,
     * so setting their parameters has no effect on the kernel.
     */
    std::vector<SpatialFunctionPtr> getSpatialFunctionList() const;

    /// Return a particular Kernel Parameter (no bounds checking).  This version is slow,
    /// but specialisations may be faster
    virtual double getKernelParameter(unsigned int i) const { return getKernelParameters()[i]; }

    /**
     * Return the current kernel parameters
     *
     * If the kernel is spatially varying then the parameters are those last computed.
     * See also computeKernelParametersFromSpatialModel.
     * If there are no kernel parameters then returns an empty vector.
     */
    virtual std::vector<double> getKernelParameters() const;

    /**
     * Given a bounding box for pixels one wishes to compute by convolving an image with this kernel,
     * return the bounding box of pixels that must be accessed on the image to be convolved.
     * Thus the box shifted by -kernel.getCtr() and its size is expanded by kernel.getDimensions()-1.
     *
     * @returns the bbox expanded by the kernel.
     */
    lsst::geom::Box2I growBBox(lsst::geom::Box2I const &bbox) const;

    /**
     * Given a bounding box for an image one wishes to convolve with this kernel,
     * return the bounding box for the region of pixels that can be computed.
     * Thus the box shifted by kernel.getCtr() and its size is reduced by kernel.getDimensions()-1.
     *
     * @returns the bbox shrunk by the kernel.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if the resulting box would have
     * dimension < 1 in either axis
     */
    lsst::geom::Box2I shrinkBBox(lsst::geom::Box2I const &bbox) const;

    /**
     * Set index of kernel's center
     */
    inline void setCtr(lsst::geom::Point2I ctr) {
        _ctrX = ctr.getX();
        _ctrY = ctr.getY();
        _setKernelXY();
    }

    /**
     * Return the spatial parameters parameters (an empty vector if not spatially varying)
     */
    inline std::vector<std::vector<double>> getSpatialParameters() const {
        std::vector<std::vector<double>> spatialParams;
        std::vector<SpatialFunctionPtr>::const_iterator spFuncIter = _spatialFunctionList.begin();
        for (; spFuncIter != _spatialFunctionList.end(); ++spFuncIter) {
            spatialParams.push_back((*spFuncIter)->getParameters());
        }
        return spatialParams;
    }

    /**
     * Return true iff the kernel is spatially varying (has a spatial function)
     */
    inline bool isSpatiallyVarying() const { return _spatialFunctionList.size() != 0; }

    /**
     * Set the kernel parameters of a spatially invariant kernel.
     *
     * @throws lsst::pex::exceptions::RuntimeError if the kernel has a spatial function
     * @throws lsst::pex::exceptions::InvalidParameterError if the params vector is the wrong length
     */
    inline void setKernelParameters(std::vector<double> const &params) {
        if (this->isSpatiallyVarying()) {
            throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeError, "Kernel is spatially varying");
        }
        const unsigned int nParams = this->getNKernelParameters();
        if (nParams != params.size()) {
            throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterError,
                              (boost::format("Number of parameters is wrong, saw %d expected %d") % nParams %
                               params.size())
                                      .str());
        }
        for (unsigned int ii = 0; ii < nParams; ++ii) {
            this->setKernelParameter(ii, params[ii]);
        }
    }

    /**
     * Set the kernel parameters of a 2-component spatially invariant kernel.
     *
     * @warning This is a low-level method intended for maximum efficiency when using warping kernels.
     * No error checking is performed. Use the std::vector<double> form if you want safety.
     */
    inline void setKernelParameters(std::pair<double, double> const &params) {
        this->setKernelParameter(0, params.first);
        this->setKernelParameter(1, params.second);
    }

    /**
     * Set the parameters of all spatial functions
     *
     * Params is indexed as [kernel parameter][spatial parameter]
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if params is the wrong shape
     *  (if this exception is thrown then no parameters are changed)
     */
    void setSpatialParameters(const std::vector<std::vector<double>> params);

    /**
     * Compute the kernel parameters at a specified point
     *
     * Warning: this is a low-level function that assumes kernelParams is the right length.
     * It will fail in unpredictable ways if that condition is not met.
     */
    void computeKernelParametersFromSpatialModel(std::vector<double> &kernelParams, double x, double y) const;

    /**
     * Return a string representation of the kernel
     */
    virtual std::string toString(std::string const &prefix = "") const;

    /**
     * Compute a cache of Kernel values, if desired
     *
     * @warning: few kernel classes actually support this,
     * in which case this is a no-op and getCacheSize always returns 0.
     */
    virtual void computeCache(int const  ///< desired cache size
    ) {}

    /**
     * Get the current size of the kernel cache (0 if none or if caches not supported)
     */
    virtual int getCacheSize() const { return 0; };

#if 0  // fails to compile with icc; is it actually used?
        virtual void toFile(std::string fileName) const;
#endif

    struct PersistenceHelper;

protected:
    std::string getPythonModule() const override;

    /**
     * Set one kernel parameter
     *
     * Classes that have kernel parameters must subclass this function.
     *
     * This function is marked "const", despite modifying unimportant internals,
     * so that computeImage can be const.
     *
     * @throws lsst::pex::exceptions::InvalidParameterError always (unless subclassed)
     */
    virtual void setKernelParameter(unsigned int ind, double value) const;

    /**
     * Set the kernel parameters from the spatial model (if any).
     *
     * This function has no effect if there is no spatial model.
     *
     * This function is marked "const", despite modifying unimportant internals,
     * so that computeImage can be const.
     */
    void setKernelParametersFromSpatialModel(double x, double y) const;

    /**
     * Low-level version of computeImage
     *
     * Before this is called the image dimensions are checked, the image's xy0 is set
     * and the kernel's parameters are set.
     * This routine sets the pixels, including normalization if requested.
     *
     * @param image image whose pixels are to be set (output)
     * @param doNormalize normalize the image (so sum is 1)?
     * @returns The kernel sum
     */
    virtual double doComputeImage(lsst::afw::image::Image<Pixel> &image, bool doNormalize) const = 0;

    std::vector<SpatialFunctionPtr> _spatialFunctionList;

private:
    int _width;
    int _height;
    int _ctrX;
    int _ctrY;
    unsigned int _nKernelParams;

    // Set the Kernel's ideas about the x- and y- coordinates
    virtual void _setKernelXY() {}
};

using KernelList = std::vector<std::shared_ptr<Kernel>>;

/**
 * A kernel created from an Image
 *
 * It has no adjustable parameters and so cannot be spatially varying.
 *
 * @ingroup afw
 */
class FixedKernel : public Kernel {
DECLARE_PERSISTABLE_FACADE(FixedKernel);
public:
    /**
     * Construct an empty FixedKernel of size 0x0
     */
    explicit FixedKernel();

    /**
     * Construct a FixedKernel from an image
     */
    explicit FixedKernel(lsst::afw::image::Image<Pixel> const &image  ///< image for kernel
    );

    /**
     * Construct a FixedKernel from a generic Kernel
     */
    explicit FixedKernel(lsst::afw::math::Kernel const &kernel,  ///< Kernel to convert to Fixed
                         lsst::geom::Point2D const &pos          ///< desired position
    );

    FixedKernel(const FixedKernel &) = delete;
    FixedKernel(FixedKernel &&) = delete;
    FixedKernel &operator=(const FixedKernel &) = delete;
    FixedKernel &operator=(FixedKernel &&) = delete;

    ~FixedKernel() override = default;

    std::shared_ptr<Kernel> clone() const override;

    std::shared_ptr<Kernel> resized(int width, int height) const override;

    std::string toString(std::string const &prefix = "") const override;

    virtual Pixel getSum() const { return _sum; }

    bool isPersistable() const noexcept override { return true; }

    class Factory;

protected:
    double doComputeImage(lsst::afw::image::Image<Pixel> &image, bool doNormalize) const override;

    std::string getPersistenceName() const override;

    void write(OutputArchiveHandle &handle) const override;

private:
    lsst::afw::image::Image<Pixel> _image;
    Pixel _sum;
};

/**
 * A kernel described by a function.
 *
 * The function's x, y arguments are as follows:
 * * -getCtr() for the lower left corner pixel
 * * 0, 0 for the center pixel
 * * (getDimensions() - 1) - getCtr() for the upper right pixel
 *
 * Note: each pixel is set to the value of the kernel function at the center of the pixel
 * (rather than averaging the function over the area of the pixel).
 *
 * @ingroup afw
 */
class AnalyticKernel : public Kernel {
DECLARE_PERSISTABLE_FACADE(AnalyticKernel);
public:
    using KernelFunction = lsst::afw::math::Function2<Pixel>;
    using KernelFunctionPtr = std::shared_ptr<lsst::afw::math::Function2<Pixel>>;

    /**
     * Construct an empty spatially invariant AnalyticKernel of size 0x0
     */
    explicit AnalyticKernel();

    /**
     * @brief Construct a spatially invariant AnalyticKernel,
     * or a spatially varying AnalyticKernel where the spatial model
     * is described by one function (that is cloned to give one per analytic function parameter).
     *
     * @param width width of kernel
     * @param height height of kernel
     * @param kernelFunction kernel function; a deep copy is made
     * @param spatialFunction spatial function; one deep copy is made for each kernel function
     *                        parameter; if omitted or set to Kernel::NullSpatialFunction() then
     *                        the kernel is spatially invariant
     */
    explicit AnalyticKernel(int width, int height, KernelFunction const &kernelFunction,
                            Kernel::SpatialFunction const &spatialFunction = NullSpatialFunction());

    /**
     * @brief Construct a spatially varying AnalyticKernel, where the spatial model
     * is described by a list of functions (one per analytic function parameter).
     *
     * @param width width of kernel
     * @param height height of kernel
     * @param kernelFunction kernel function; a deep copy is made
     * @param spatialFunctionList list of spatial functions, one per kernel function parameter; a
     *                            deep copy is made of each function
     *
     * @throws lsst::pex::exceptions::InvalidParameterError
     *        if the length of spatialFunctionList != # kernel function parameters.
     */
    explicit AnalyticKernel(int width, int height, KernelFunction const &kernelFunction,
                            std::vector<Kernel::SpatialFunctionPtr> const &spatialFunctionList);

    AnalyticKernel(const AnalyticKernel &) = delete;
    AnalyticKernel(AnalyticKernel &&) = delete;
    AnalyticKernel &operator=(const AnalyticKernel &) = delete;
    AnalyticKernel &operator=(AnalyticKernel &&) = delete;

    ~AnalyticKernel() override = default;

    std::shared_ptr<Kernel> clone() const override;

    std::shared_ptr<Kernel> resized(int width, int height) const override;

    /**
     * Compute an image (pixellized representation of the kernel) in place
     *
     * This special version accepts any size image (though you can get in trouble
     * if the image is large enough that the image is evaluated outside its domain).
     *
     * @param image image whose pixels are to be set (output) xy0 of the image will be set to
     *              -kernel.getCtr() - border, where
     *              border = (image.getDimensions() - kernel.getDimensions()) / 2
     * @param doNormalize normalize the image (so sum is 1)?
     * @param x x (column position) at which to compute spatial function
     * @param y y (row position) at which to compute spatial function
     * @returns The kernel sum
     *
     * @note computeNewImage has been retired; it doesn't need to be a member
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if the image is the wrong size
     * @throws lsst::pex::exceptions::OverflowError if doNormalize is true and the kernel sum is
     * exactly 0
     */
    double computeImage(lsst::afw::image::Image<Pixel> &image, bool doNormalize, double x = 0.0,
                        double y = 0.0) const;

    std::vector<double> getKernelParameters() const override;

    /**
     * Get a deep copy of the kernel function
     */
    virtual KernelFunctionPtr getKernelFunction() const;

    std::string toString(std::string const &prefix = "") const override;

    bool isPersistable() const noexcept override { return true; }

    class Factory;

protected:
    double doComputeImage(lsst::afw::image::Image<Pixel> &image, bool doNormalize) const override;

    std::string getPersistenceName() const override;

    void write(OutputArchiveHandle &handle) const override;

protected:
    void setKernelParameter(unsigned int ind, double value) const override;

    KernelFunctionPtr _kernelFunctionPtr;
};

/**
 * A kernel that has only one non-zero pixel (of value 1)
 *
 * It has no adjustable parameters and so cannot be spatially varying.
 *
 * @ingroup afw
 */
class DeltaFunctionKernel : public Kernel {
DECLARE_PERSISTABLE_FACADE(DeltaFunctionKernel);
public:
    // Traits values for this class of Kernel
    using kernel_fill_factor = deltafunction_kernel_tag;

    /**
     * Construct a spatially invariant DeltaFunctionKernel
     *
     * @param width kernel size (columns)
     * @param height kernel size (rows)
     * @param point index of active pixel (where 0,0 is the lower left corner)
     *
     * @throws pex::exceptions::InvalidParameterError if active pixel is off the kernel
     */
    explicit DeltaFunctionKernel(int width, int height, lsst::geom::Point2I const &point);

    DeltaFunctionKernel(const DeltaFunctionKernel &) = delete;
    DeltaFunctionKernel(DeltaFunctionKernel &&) = delete;
    DeltaFunctionKernel &operator=(const DeltaFunctionKernel &) = delete;
    DeltaFunctionKernel &operator=(DeltaFunctionKernel &&) = delete;

    ~DeltaFunctionKernel() override = default;

    std::shared_ptr<Kernel> clone() const override;

    std::shared_ptr<Kernel> resized(int width, int height) const override;

    lsst::geom::Point2I getPixel() const { return _pixel; }

    std::string toString(std::string const &prefix = "") const override;

    bool isPersistable() const noexcept override { return true; }

    class Factory;

protected:
    double doComputeImage(lsst::afw::image::Image<Pixel> &image, bool doNormalize) const override;

    std::string getPersistenceName() const override;

    void write(OutputArchiveHandle &handle) const override;

private:
    lsst::geom::Point2I _pixel;
};

/**
 * A kernel that is a linear combination of fixed basis kernels.
 *
 * Convolution may be performed by first convolving the image
 * with each fixed kernel, then adding the resulting images using the (possibly
 * spatially varying) kernel coefficients.
 *
 * The basis kernels are cloned (deep copied) so you may safely modify your own copies.
 *
 * Warnings:
 * - This class does not normalize the individual basis kernels; they are used "as is".
 *
 * @ingroup afw
 */
class LinearCombinationKernel : public Kernel {
DECLARE_PERSISTABLE_FACADE(LinearCombinationKernel);
public:
    /**
     * Construct an empty LinearCombinationKernel of size 0x0
     */
    explicit LinearCombinationKernel();

    /**
     * Construct a spatially invariant LinearCombinationKernel
     *
     * @param kernelList list of (shared pointers to const) basis kernels
     * @param kernelParameters kernel coefficients
     */
    explicit LinearCombinationKernel(KernelList const &kernelList,
                                     std::vector<double> const &kernelParameters);

    /**
     * @brief Construct a spatially varying LinearCombinationKernel, where the spatial model
     * is described by one function (that is cloned to give one per basis kernel).
     *
     * @param kernelList list of (shared pointers to const) basis kernels
     * @param spatialFunction spatial function; one deep copy is made for each basis kernel
     */
    explicit LinearCombinationKernel(KernelList const &kernelList,
                                     Kernel::SpatialFunction const &spatialFunction);

    /**
     * @brief Construct a spatially varying LinearCombinationKernel, where the spatial model
     * is described by a list of functions (one per basis kernel).
     *
     * @param kernelList list of (shared pointers to const) kernels
     * @param spatialFunctionList list of spatial functions, one per basis kernel
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if the length of spatialFunctionList != # kernels
     */
    explicit LinearCombinationKernel(KernelList const &kernelList,
                                     std::vector<Kernel::SpatialFunctionPtr> const &spatialFunctionList);

    LinearCombinationKernel(const LinearCombinationKernel &) = delete;
    LinearCombinationKernel(LinearCombinationKernel &&) = delete;
    LinearCombinationKernel &operator=(const LinearCombinationKernel &) = delete;
    LinearCombinationKernel &operator=(LinearCombinationKernel &&) = delete;

    ~LinearCombinationKernel() override = default;

    std::shared_ptr<Kernel> clone() const override;

    std::shared_ptr<Kernel> resized(int width, int height) const override;

    std::vector<double> getKernelParameters() const override;

    /**
     * Get the fixed basis kernels
     */
    virtual KernelList const &getKernelList() const;

    /**
     * Get the sum of the pixels of each fixed basis kernel
     */
    std::vector<double> getKernelSumList() const;

    /**
     * Get the number of basis kernels
     */
    int getNBasisKernels() const { return static_cast<int>(_kernelList.size()); };

    /**
     * Check that all kernels have the same size and center and that none are spatially varying
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if the check fails
     */
    void checkKernelList(const KernelList &kernelList) const;

    /**
     * Return true if all basis kernels are instances of DeltaFunctionKernel
     */
    bool isDeltaFunctionBasis() const { return _isDeltaFunctionBasis; };

    /**
     * @brief Refactor the kernel as a linear combination of N bases where N is the number of parameters
     * for the spatial model.
     *
     * Refactoring is only possible if all of the following are true:
     *  * Kernel is spatially varying
     *  * The spatial functions are a linear combination of coefficients (return isLinearCombination() true).
     *  * The spatial functions all are the same class (and so have the same functional form)
     * Refactoring produces a kernel that is faster to compute only if the number of basis kernels
     * is greater than the number of parameters in the spatial model.
     *
     * Details:
     * A spatially varying LinearCombinationKernel consisting of M basis kernels
     * and using a spatial model that is a linear combination of N coefficients can be expressed as:
     * K(x,y) =   K0 (C00 F0(x,y) + C10 F1(x,y) + C20 F2(x,y) + ... + CN0 FN(x,y))
     *          + K1 (C01 F0(x,y) + C11 F1(x,y) + C21 F2(x,y) + ... + CN1 FN(x,y))
     *          + K2 (C02 F0(x,y) + C12 F1(x,y) + C22 F2(x,y) + ... + CN2 FN(x,y))
     *          + ...
     *          + KM (C0M F0(x,y) + C1M F1(x,y) + C2M F2(x,y) + ... + CNM FN(x,y))
     *
     * This is equivalent to the following linear combination of N basis kernels:
     *
     *         =      K0' F0(x,y) + K1' F1(x,y) + K2' F2(x,y) + ... + KN' FN(x,y)
     *
     *           where Ki' = sum over j of Kj Cij
     *
     * This is what refactor returns provided the required conditions are met. However, the spatial functions
     * for the refactored kernel are the same as those for the original kernel (for generality and simplicity)
     * with all coefficients equal to 0 except one that is set to 1; hence they are not computed optimally.
     *
     * Thanks to Kresimir Cosic for inventing or reinventing this useful technique.
     *
     * @returns a shared pointer to new kernel, or empty pointer if refactoring not possible
     */
    std::shared_ptr<Kernel> refactor() const;

    std::string toString(std::string const &prefix = "") const override;

    bool isPersistable() const noexcept override { return true; }

    class Factory;

protected:
    double doComputeImage(lsst::afw::image::Image<Pixel> &image, bool doNormalize) const override;

    std::string getPersistenceName() const override;

    void write(OutputArchiveHandle &handle) const override;

    void setKernelParameter(unsigned int ind, double value) const override;

private:
    /**
     * Set _kernelList by cloning each input kernel and update the kernel image cache.
     */
    void _setKernelList(KernelList const &kernelList);

    KernelList _kernelList;  ///< basis kernels
    /// image of each basis kernel (a cache)
    std::vector<std::shared_ptr<lsst::afw::image::Image<Pixel>>> _kernelImagePtrList;
    std::vector<double> _kernelSumList;  ///< sum of each basis kernel (a cache)
    mutable std::vector<double> _kernelParams;
    bool _isDeltaFunctionBasis;
};

/**
 * A kernel described by a pair of functions: func(x, y) = colFunc(x) * rowFunc(y)
 *
 * The function's x, y arguments are as follows:
 * * -getCtr() for the lower left corner pixel
 * * 0, 0 for the center pixel
 * * (getDimensions() - 1) - getCtr() for the upper right pixel
 *
 * Note: each pixel is set to the value of the kernel function at the center of the pixel
 * (rather than averaging the function over the area of the pixel).
 *
 * @ingroup afw
 */
class SeparableKernel : public Kernel {
DECLARE_PERSISTABLE_FACADE(SeparableKernel);
public:
    using KernelFunction = lsst::afw::math::Function1<Pixel>;
    using KernelFunctionPtr = std::shared_ptr<KernelFunction>;

    /**
     * Construct an empty spatially invariant SeparableKernel of size 0x0
     */
    explicit SeparableKernel();

    /**
     * @brief Construct a spatially invariant SeparableKernel, or a spatially varying SeparableKernel
     * that uses the same functional form to model each function parameter.
     *
     * @param width width of kernel
     * @param height height of kernel
     * @param kernelColFunction kernel column function
     * @param kernelRowFunction kernel row function
     * @param spatialFunction spatial function; one deep copy is made for each kernel column and row
     *                        function parameter; if omitted or set to Kernel::NullSpatialFunction then
     *                        the kernel is spatially invariant
     */
    explicit SeparableKernel(int width, int height, KernelFunction const &kernelColFunction,
                             KernelFunction const &kernelRowFunction,
                             Kernel::SpatialFunction const &spatialFunction = NullSpatialFunction());

    /**
     * Construct a spatially varying SeparableKernel
     *
     * @param width width of kernel
     * @param height height of kernel
     * @param kernelColFunction kernel column function
     * @param kernelRowFunction kernel row function
     * @param spatialFunctionList list of spatial funcs, one per kernel column and row function
     *                            parameter; a deep copy is made of each function
     *
     * @throws lsst::pex::exceptions::InvalidParameterError
     *  if the length of spatialFunctionList != # kernel function parameters.
     */
    explicit SeparableKernel(int width, int height, KernelFunction const &kernelColFunction,
                             KernelFunction const &kernelRowFunction,
                             std::vector<Kernel::SpatialFunctionPtr> const &spatialFunctionList);

    SeparableKernel(const SeparableKernel &) = delete;
    SeparableKernel(SeparableKernel &&) = delete;
    SeparableKernel &operator=(const SeparableKernel &) = delete;
    SeparableKernel &operator=(SeparableKernel &&) = delete;

    ~SeparableKernel() override = default;

    std::shared_ptr<Kernel> clone() const override;

    std::shared_ptr<Kernel> resized(int width, int height) const override;

    /**
     * Compute the column and row arrays in place, where kernel(col, row) = colList(col) * rowList(row)
     *
     * x, y are ignored if there is no spatial function.
     *
     * @param colList column vector
     * @param rowList row vector
     * @param doNormalize normalize the image (so sum of each is 1)?
     * @param x x (column position) at which to compute spatial function
     * @param y y (row position) at which to compute spatial function
     * @returns the kernel sum (1.0 if doNormalize true)
     *
     * @throws lsst::pex::exceptions::InvalidParameterError if colList or rowList is the wrong size
     * @throws lsst::pex::exceptions::OverflowError if doNormalize is true and the kernel sum is
     * exactly 0
     */
    double computeVectors(std::vector<Pixel> &colList, std::vector<Pixel> &rowList, bool doNormalize,
                          double x = 0.0, double y = 0.0) const;

    double getKernelParameter(unsigned int i) const override {
        unsigned int const ncol = _kernelColFunctionPtr->getNParameters();
        if (i < ncol) {
            return _kernelColFunctionPtr->getParameter(i);
        } else {
            i -= ncol;
            return _kernelRowFunctionPtr->getParameter(i);
        }
    }
    std::vector<double> getKernelParameters() const override;

    /**
     * Get a deep copy of the col kernel function
     */
    KernelFunctionPtr getKernelColFunction() const;

    /**
     * Get a deep copy of the row kernel function
     */
    KernelFunctionPtr getKernelRowFunction() const;

    std::string toString(std::string const &prefix = "") const override;

    /***
     * Compute a cache of values for the x and y kernel functions
     *
     * A value of 0 disables the cache for maximum accuracy.
     * 10,000 typically results in a warping error of a fraction of a count.
     * 100,000 typically results in a warping error of less than 0.01 count.
     *
     * @param cacheSize cache size (number of double precision array elements in the x and y caches)
     */
    void computeCache(int const cacheSize) override;

    /**
     * Get the current cache size (0 if none)
     */
    int getCacheSize() const override;

protected:
    double doComputeImage(lsst::afw::image::Image<Pixel> &image, bool doNormalize) const override;

    void setKernelParameter(unsigned int ind, double value) const override;

private:
    /**
     * Compute the column and row arrays in place, where kernel(col, row) = colList(col) * rowList(row)
     *
     * @param colList column vector
     * @param rowList row vector
     * @param doNormalize normalize the arrays (so sum of each is 1)?
     * @returns the kernel sum (1.0 if doNormalize true)
     *
     * Warning: the length of colList and rowList are not verified!
     *
     * @throws lsst::pex::exceptions::OverflowError if doNormalize is true and the kernel sum is
     * exactly 0
     */
    double basicComputeVectors(std::vector<Pixel> &colList, std::vector<Pixel> &rowList,
                               bool doNormalize) const;

    KernelFunctionPtr _kernelColFunctionPtr;
    KernelFunctionPtr _kernelRowFunctionPtr;
    mutable std::vector<Pixel> _localColList;  // used by doComputeImage
    mutable std::vector<Pixel> _localRowList;
    mutable std::vector<double> _kernelX;  // used by SeparableKernel::basicComputeVectors
    mutable std::vector<double> _kernelY;
    //
    // Cached values of the row- and column- kernels
    //
    mutable std::vector<std::vector<double>> _kernelRowCache;
    mutable std::vector<std::vector<double>> _kernelColCache;

    virtual void _setKernelXY() override {
        lsst::geom::Extent2I const dim = getDimensions();
        lsst::geom::Point2I const ctr = getCtr();

        assert(dim[0] == static_cast<int>(_kernelX.size()));
        for (int i = 0; i != dim.getX(); ++i) {
            _kernelX[i] = i - ctr.getX();
        }

        assert(dim[1] == static_cast<int>(_kernelY.size()));
        for (int i = 0; i != dim.getY(); ++i) {
            _kernelY[i] = i - ctr.getY();
        }
    }
};
}  // namespace math
}  // namespace afw
}  // namespace lsst

#endif  // !defined(LSST_AFW_MATH_KERNEL_H)
