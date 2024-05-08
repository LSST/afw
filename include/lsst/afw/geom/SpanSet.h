// -*- lsst-c++ -*-

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
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */

#ifndef LSST_AFW_GEOM_SPANSET_H
#define LSST_AFW_GEOM_SPANSET_H

#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include "lsst/pex/exceptions.h"
#include "lsst/afw/geom/Span.h"
#include "lsst/geom/Box.h"
#include "lsst/afw/image/Mask.h"
#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/geom/ellipses/Ellipse.h"
#include "lsst/afw/geom/ellipses/Quadrupole.h"
#include "lsst/afw/geom/SpanSetFunctorGetters.h"
#include "lsst/afw/geom/Transform.h"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"

namespace lsst {
namespace afw {
namespace geom {
namespace details {

/* Functor object to be used with fromMask function
 */
template <typename T>
class AnyBitSetFunctor {
public:
    bool operator()(T pixelValue) { return pixelValue != 0; }
};

}  // namespace details

/** An enumeration class which describes the shapes

A stencil is a shape used in creating SpanSets, erosion kernels,
or dilation kernels. CIRCLE creates a circle shape, BOX
creates a box shape, and MANHATTAN creates a diamond shape.
 */
enum class Stencil { CIRCLE, BOX, MANHATTAN };

/**
 * A compact representation of a collection of pixels
 *
 * A SpanSet is a collection of Span classes. As each Span encodes a range of pixels
 * on a given row, a SpanSet represents an arbitrary collection of pixels on an image.
 * The SpanSet class also contains mathematical set style operators, for working with
 * the collection of pixels, and helper functions which make use of the area defined
 * to perform localized actions
 */
class SpanSet : public afw::table::io::Persistable {
DECLARE_PERSISTABLE_FACADE(SpanSet);
public:
    using const_iterator = std::vector<Span>::const_iterator;
    using size_type = std::vector<Span>::size_type;
    using value_type = Span;
    using const_reference = const value_type &;

    // Expose properties of the underlying vector containing spans such that the
    // SpanSet can be considered a container.
    // Return the constant versions as SpanSets should be immutable
    const_iterator begin() const { return _spanVector.cbegin(); }
    const_iterator end() const { return _spanVector.cend(); }
    const_iterator cbegin() const { return _spanVector.cbegin(); }
    const_iterator cend() const { return _spanVector.cend(); }
    const_reference front() const { return const_cast<geom::Span &>(_spanVector.front()); }
    const_reference back() const { return const_cast<geom::Span &>(_spanVector.back()); }
    size_type size() const { return _spanVector.size(); }
    bool empty() const { return _spanVector.empty(); }

    /** Default constructor
     *
     * Construct a null SpanSet, with zero size. This is useful as a placeholder
     */
    SpanSet();

    /** Construct a SpanSet from a box
     *
     * @param box A integer box that defines the shape for which a span set should be created
     */
    explicit SpanSet(lsst::geom::Box2I const &box);

    /** Construct a SpanSet from an iterator
     *
     * This constructor accepts the begin and end points of an arbitrary iterator
     * of a container which contains previously created Spans. These Spans are
     * used to construct the SpanSet.
     *
     * @param begin Beginning iterator of a container of Spans
     * @param end End iterator of a container of Spans
     * @param normalize Controls if the constructor attempts to merge connected or
                        overlapping spans. Defaults to true. Set to false to save
                        computational time when the container is sure to already
                        be normalized.
     */
    template <typename iter>
    SpanSet(iter begin, iter end, bool normalize = true) : _spanVector(begin, end) {
        // Return a null SpanSet if spanVector is 0
        if (_spanVector.size() == 0) {
            _bbox = lsst::geom::Box2I();
            _area = 0;
        } else {
            if (normalize) {
                _runNormalize();
            }
            _initialize();
        }
    }

    /** Construct a SpanSet from a std vector by copying
     *
     * This constructor accepts a standard vector which contains already created
     * Spans. These Spans are copied into the internal container of spans.
     *
     * @param vec Standard vector containing Spans
     * @param normalize Controls if the constructor attempts to merge connected or
                        overlapping spans. Defaults to true. Set to false to save
                        computational time when the container is sure to already
                        be normalized.
    */
    explicit SpanSet(std::vector<Span> const &vec, bool normalize = true);

    /** Construct a SpanSet from a std vector from a move
     *
     * This constructor accepts a standard vector r-reference which contains already
     * created Spans. These Spans become the internal container of spans.
     *
     * @param vec Standard vector containing Spans
     * @param normalize Controls if the constructor attempts to merge connected or
                        overlapping spans. Defaults to true. Set to false to save
                        computational time when the container is sure to already
                        be normalized.
    */
    explicit SpanSet(std::vector<Span> &&vec, bool normalize = true);

    // Explicitly delete copy and move constructors
    SpanSet(SpanSet const &other) = delete;
    SpanSet(SpanSet &&other) = delete;
    ~SpanSet() override = default;

    SpanSet &operator=(SpanSet const &) = default;
    // Delegate to copy-assignment for backwards compatibility
    SpanSet &operator=(SpanSet &&other) { return *this = other; }

    // Define class methods
    /** Return the number of pixels in the SpanSet
     */
    size_type getArea() const;

    /** Return a new integer box which is the minimum size to contain the pixels
     */
    lsst::geom::Box2I getBBox() const;

    /** Defines if the SpanSet is simply contiguous
     *
     * If the pixels can be traversed in such a way that every pixel can be reached
     * without going over a pixel not contained in the SpanSet this method will return
     * true. If the SpanSet is disjoint aka the above is not true and there is more
     * than one region, returns false.
     */
    bool isContiguous() const;

    /** Return a new SpanSet shifted by specified amount
     *
     * @param x number of pixels to shift in x dimension
     * @param y number of pixels to shift in y dimension
     */
    std::shared_ptr<SpanSet> shiftedBy(int x, int y) const;

    /** Return a new SpanSet shifted by specified amount
     *
     * @param offset integer extent which specifies amount to offset in x and y
     */
    std::shared_ptr<SpanSet> shiftedBy(lsst::geom::Extent2I const &offset) const;

    /** Return a new SpanSet which has all pixel values inside specified box
     *
     * @param box Integer box specifying the bounds for which all pixels must be within
     */
    std::shared_ptr<SpanSet> clippedTo(lsst::geom::Box2I const &box) const;

    /** Return a new SpanSet who's pixels are the product of applying the specified transformation
     *
     * @param t A linear transform object which will be used to map the pixels
     */
    std::shared_ptr<SpanSet> transformedBy(lsst::geom::LinearTransform const &t) const;

    /** Return a new SpanSet who's pixels are the product of applying the specified transformation
     *
     * @param t An affine transform object which will be used to map the pixels
     */
    std::shared_ptr<SpanSet> transformedBy(lsst::geom::AffineTransform const &t) const;

    /** Return a new SpanSet who's pixels are the product of applying the specified transformation
     *
     * @param t A 2-D transform which will be used to map the pixels
     */
    std::shared_ptr<SpanSet> transformedBy(TransformPoint2ToPoint2 const &t) const;

    /** Specifies if this SpanSet overlaps with another SpanSet
     *
     * @param other A SpanSet for which overlapping comparison will be made
     */
    bool overlaps(SpanSet const &other) const;

    /** Check if a SpanSet instance entirely contains another SpanSet
     *
     * @param other The SpanSet who's membership is to be tested for
     */
    bool contains(SpanSet const &other) const;

    /** Check if a point is contained within the SpanSet instance
     *
     * @param point An integer point object for which membership is to be tested
     */
    bool contains(lsst::geom::Point2I const &point) const;

    /** Compute the point about which the SpanSet's first moment is zero
     */
    lsst::geom::Point2D computeCentroid() const;

    /** Compute the shape parameters for the distribution of points in the SpanSet
     */
    ellipses::Quadrupole computeShape() const;

    /** Perform a set dilation operation, and return a new object
     *
     * Dilate a SpanSet with a kernel specified with the stencil parameter
     *
     * @param r radius of the stencil, the length is inclusive i.e. 3 ranges from -3 to 3
     * @param s must be an enumeration of type geom::Stencil. Specifies the shape of the
                dilation kernel. May be CIRCLE, MANHATTAN, or BOX
     */
    std::shared_ptr<SpanSet> dilated(int r, Stencil s = Stencil::CIRCLE) const;

    /** Perform a set dilation operation, and return a new object
     *
     * Dilate a SpanSet with a kernel specified by another SpanSet
     *
     * @param other A SpanSet which specifies the kernel to use for dilation
     */
    std::shared_ptr<SpanSet> dilated(SpanSet const &other) const;

    /** Perform a set erosion, and return a new object
     *
     * Erode a SpanSet with a kernel specified with the stencil parameter
     *
     * @param r radius of the stencil, the length is inclusive i.e. 3 ranges from -3 to 3
     * @param s must be an enumeration of type geom::Stencil. Specifies the shape of the
                erosion kernel. May be CIRCLE, MANHATTAN, or BOX
     */
    std::shared_ptr<SpanSet> eroded(int r, Stencil s = Stencil::CIRCLE) const;

    /** Perform a set erosion operation, and return a new object
     *
     * Erode a SpanSet with a kernel specified by another SpanSet
     *
     * @param other A SpanSet which specifies the kernel to use for erosion
     */
    std::shared_ptr<SpanSet> eroded(SpanSet const &other) const;

    /** Reduce the pixel dimensionality from 2 to 1 of an array at points given by SpanSet
     *
     * Take values from an array at points defined by SpanSet and use them to populate a new array
     * where the x,y coordinates of the SpanSet have been flattened to one dimension. First two
     * dimensions of the input array must be the h,w which correspond to the SpanSet coordinates. Any
     * number of remaining dimensions is permissible.
     *
     * @tparam Pixel The datatype for the ndarray
     * @tparam inN The number of dimensions in the array object
     * @tparam inC Number of guaranteed row-major contiguous dimensions, starting from the end
     *
     * @param input The ndarray from which the values will be taken
     * @param xy0 A point object with is used as the origin point for the SpanSet coordinate system
     */
    template <typename Pixel, int inN, int inC>
    ndarray::Array<typename std::remove_const<Pixel>::type, inN - 1, inN - 1> flatten(
            ndarray::Array<Pixel, inN, inC> const &input,
            lsst::geom::Point2I const &xy0 = lsst::geom::Point2I()) const {
        // Populate a lower dimensional array with the values from input taken at the points of SpanSet
        auto outputShape = ndarray::concatenate(ndarray::makeVector(getArea()),
                                                input.getShape().template last<inN - 2>());
        ndarray::Array<typename std::remove_const<Pixel>::type, inN - 1, inN - 1> outputArray =
                ndarray::allocate(outputShape);
        outputArray.deep() = 0;
        flatten(outputArray, input, xy0);
        return outputArray;
    }

    /** Reduce the pixel dimensionality from 2 to 1 of an array at points given by SpanSet
     *
     * Take values from an array at points defined by SpanSet and use them to populate a new array
     * where the x,y coordinates of the SpanSet have been flattened to one dimension. First two
     * dimensions of the input array must be the h,w which correspond to the SpanSet coordinates. Any
     * number of remaining dimensions is permissible.
     *
     * @tparam PixelOut The data-type for the output ndarray
     * @tparam PixelIn The data-type for the input ndarray
     * @tparam inA The number of dimensions in the input array
     * @tparam outC Number of guaranteed row-major contiguous dimensions in the output array,
                    starting from the end
     * @tparam inC Number of guaranteed row-major contiguous dimensions in the input array,
                   starting from the end
     *
     * @param[out] output The 1d ndarray which will be populated with output parameters, will happen in place
     * @param[in] input The ndarray from which the values will be taken
     * @param[in] xy0 A point object which is used as the origin point for the SpanSet coordinate system
     */
    template <typename PixelIn, typename PixelOut, int inA, int outC, int inC>
    void flatten(ndarray::Array<PixelOut, inA - 1, outC> const &output,
                 ndarray::Array<PixelIn, inA, inC> const &input,
                 lsst::geom::Point2I const &xy0 = lsst::geom::Point2I()) const {
        auto ndAssigner = [](lsst::geom::Point2I const &point,
                             typename details::FlatNdGetter<PixelOut, inA - 1, outC>::Reference out,
                             typename details::ImageNdGetter<PixelIn, inA, inC>::Reference in) { out = in; };
        // Populate array output with values from input at positions given by SpanSet
        applyFunctor(ndAssigner, ndarray::ndFlat(output), ndarray::ndImage(input, xy0));
    }

    /** Expand an array by one spatial dimension at points given by SpanSet
     *
     * Take values from a lower dimensional array and insert them in an output array with one additional
     * dimension at points defined by the SpanSet, offset by the lower left hand corner of the
     * bounding box of the SpanSet. The first two dimensions of the output array will correspond to the
     * y,x dimensions of the SpanSet
     *
     * @tparam Pixel The data-type for the ndarray
     * @tparam inA Number of dimension of the input array
     * @tparam inC Number of guaranteed row-major contiguous dimensions, starting from the end
     *
     * @param input The ndarray from which the values will be taken
     */
    template <typename Pixel, int inA, int inC>
    ndarray::Array<typename std::remove_const<Pixel>::type, inA + 1, inA + 1> unflatten(
            ndarray::Array<Pixel, inA, inC> const &input) const {
        // Create a higher dimensional array the size of the bounding box and extra dimensions of input.
        // Populate values from input, placed at locations corresponding to SpanSet, offset by the
        // lower corner of the bounding box
        auto existingShape = input.getShape();
        typename decltype(existingShape)::Element height = _bbox.getHeight();
        typename decltype(existingShape)::Element width = _bbox.getWidth();
        auto outputShape = ndarray::concatenate(ndarray::makeVector(height, width),
                                                input.getShape().template last<inA - 1>());
        ndarray::Array<typename std::remove_const<Pixel>::type, inA + 1, inA + 1> outputArray =
                ndarray::allocate(outputShape);
        outputArray.deep() = 0;
        unflatten(outputArray, input, lsst::geom::Point2I(_bbox.getMinX(), _bbox.getMinY()));
        return outputArray;
    }

    /** Expand an array by one spatial dimension at points given by SpanSet
    *
    * Take values from a lower dimensional array and insert them in an output array with one additional
    * dimension at points defined by the SpanSet, offset by the lower left hand corner of the
    * bounding box of the SpanSet. The first two dimensions of the output array will correspond to the
    * y,x dimensions of the SpanSet
    *
    * @tparam PixelOut The datatype for the output ndarray
    * @tparam PixelIn The datatype for the input ndarray
    * @tparam inA Number of dimensions of the input ndarray
    * @tparam outC Number of guaranteed row-major contiguous dimensions in the output array,
                   starting from the end
    * @tparam inC Number of guaranteed row-major contiguous dimensions in the input array,
                  starting from the end
    *
    * @param[out] output The 1d ndarray which will be populated with output parameters, will happen in place
    * @param[in] input The ndarray from which the values will be taken
    * @param[in] xy0 A point object with is used as the origin point for the SpanSet coordinate system
    */
    template <typename PixelIn, typename PixelOut, int inA, int outC, int inC>
    void unflatten(ndarray::Array<PixelOut, inA + 1, outC> const &output,
                   ndarray::Array<PixelIn, inA, inC> const &input,
                   lsst::geom::Point2I const &xy0 = lsst::geom::Point2I()) const {
        // Populate 2D ndarray output with values from input, at locations defined by SpanSet, optionally
        // offset by xy0
        auto ndAssigner = [](lsst::geom::Point2I const &point,
                             typename details::ImageNdGetter<PixelOut, inA + 1, outC>::Reference out,
                             typename details::FlatNdGetter<PixelIn, inA, inC>::Reference in) { out = in; };
        applyFunctor(ndAssigner, ndarray::ndImage(output, xy0), ndarray::ndFlat(input));
    }

    /** Copy contents of source Image into destination image at the positions defined in the SpanSet
     *
     * @tparam ImageT The pixel type of the Image
     *
     * @param[in] src The Image that pixel values will be taken from
     * @param[out] dest The Image where pixels will be copied
     */
    template <typename ImageT>
    void copyImage(image::Image<ImageT> const &src, image::Image<ImageT> &dest) {
        auto copyFunc = [](lsst::geom::Point2I const &point, ImageT const &srcPix, ImageT &destPix) {
            destPix = srcPix;
        };
        applyFunctor(copyFunc, src, dest);
    }

    /** Copy contents of source MaskedImage into destination image at the positions defined in the SpanSet
     *
     * @tparam ImageT The pixel type of the MaskedImage's Image
     * @tparam MaskT The pixel type of the MaskedImage's Mask
     * @tparam VarT The Pixel type of the MaskedImage's Variance Image
     *
     * @param[in] src The MaskedImage that pixel values will be taken from
     * @param[out] dest The MaskedImage where pixels will be copied
     */
    template <typename ImageT, typename MaskT, typename VarT>
    void copyMaskedImage(image::MaskedImage<ImageT, MaskT, VarT> const &src,
                         image::MaskedImage<ImageT, MaskT, VarT> &dest) {
        auto copyFunc = [](lsst::geom::Point2I const &point, ImageT const &srcPix, MaskT const &srcMask,
                           VarT const &srcVar, ImageT &destPix, MaskT &destMask, VarT &destVar) {
            destPix = srcPix;
            destMask = srcMask;
            destVar = srcVar;
        };
        applyFunctor(copyFunc, *(src.getImage()), *(src.getMask()), *(src.getVariance()), *(dest.getImage()),
                     *(dest.getMask()), *(dest.getVariance()));
    }

    /** Set the values of an Image at points defined by the SpanSet
     *
     * @tparam ImageT The pixel type of the Image to be set
     *
     * @param[out] image The Image in which pixels will be set
     * @param[in] val The value to set
     * @param[in] region A bounding box limiting the scope of the SpanSet, points defined
                     in the SpanSet which fall outside this box will be ignored if
                     the doClip parameter is set to true, defaults to the bounding
                     box of the image
     * @param[in] doClip Limit the copy operation to pixels in the SpanSet that lie within
                     the region parameter defaults to false
     */
    template <typename ImageT>
    void setImage(image::Image<ImageT> &image, ImageT val,
                  lsst::geom::Box2I const &region = lsst::geom::Box2I(), bool doClip = false) const;

    /** Apply functor on individual elements from the supplied parameters
     *
     * Use a variadic template to take a functor object, and an arbitrary number of parameters.
     * Parameters may be of type(s) Image, MakedImage, ndarray, numeric value and generic iterators.
     * For most of these types bound checking is done to ensure execution safety, but because
     * a generic iterator can have any behavior, bounds checking is not possible. As such the iterator
     * must be valid for at least the number of pixels contained in the SpanSet. Numeric values are
     * also different in that they will not be iterated but the value will be passed to the functor
     * for each point in the SpanSet.

     * Because a ndarray can either be interpreted as either a two dimensional image, or a one
     * dimensional vector a user must break the ambiguity by using the functions ndarray::ndImage,
     * or ndarray::ndFlat respectively.
     *
     * The functor object must operate in-place on any data, no return values are captured. No exceptions
     * are handled, if a functor throws one it will propagate back out of the applyFunctor call, possibly
     * leaving the output array in an incomplete state. The first argument of the functor must be
     * a lsst::geom::Point2I which will be the point in the SpanSet where the operation is occurring. All the
     * remaining arguments to the functor will be individual values generated from the input arguments. For
     two
     * dimensional types (Image, MaskedImage, ndarray) arguments will be the value taken from the input type
     * at the location given by the SpanSet. I.e. if the SpanSet has the point (2,3) in it, then a 2
     dimensional
     * ndarray will have ndarray[2][3] passed to the functor. Thus all SpanSet coordinates must be addressable
     in
     * two dimensional types. For one dimensional types (ndarray, iterator) arguments will correspond to the
     * value at the location defined by number of pixels which have been iterated over. I.e. if there are 3
     pixels
     * defined in the SpanSet a one dimensional ndarray will have the value ndarray[0] passed to the functor
     on
     * on the first functor call, ndarray[1] passed on the second call, and ndarray[2] passed on the last
     functor
     * call. The total length of a one dimensional type will be equal to the area of the SpanSet, and
     therefore
     * the data-type must be at least that length. The order of the parameters supplied to the functor will
     * be the same order as they are passed to the applyFunctor method.
     *
     * @tparam ...Args Variadic type specification
     *
     * @param func Functor that is to be applied on each of the values taken from ...args.
     *
     * @param ...args Variadic arguments, may be of type Image, MaskedImage, ndarrays, numeric values
     *                and iterators. ndarrays must be specified as either image or vector with the
     *                ndarray::ndImage or ndarray::ndFlat functions
     */
    template <typename Functor, typename... Args>
    // Normally std::forward would be used with a universal reference, however
    // this function does not use one because without std::forward the
    // compiler is forced to keep any r-value references alive for the
    // duration of the function call
    void applyFunctor(Functor &&func, Args &&... args) const {
        /* Use a variadic template to take a functor object, and an arbitrary number
           of parameters. For each of the arguments, construct a Getter class using
           a function (makeGetter) which is overloaded to all the types applyFunctorImpl
           supports: Images, MaskedImages, Exposures, ndarrays, numeric values, and
           iterators. The functor and the getters are then passed to the implementation of
           applyFunctor where the values of the input arguments are intelligently
           generated at each point in SpanSet, and passed to the functor object for evaluation.
         */
        applyFunctorImpl(func, details::makeGetter(args)...);
    }

    /** Set a Mask at pixels defined by the SpanSet
     *
     * @tparam T data-type of a pixel in the Mask plane
     *
     * @param[in, out] target Mask in which values will be set
     * @param[in] bitmask The bit pattern to set in the mask
     */
    template <typename T>
    void setMask(lsst::afw::image::Mask<T> &target, T bitmask) const;

    /** Unset a Mask at pixels defined by the SpanSet
     *
     * @tparam T data-type of a pixel in the Mask plane
     *
     * @param[in, out] target Mask in which a bit pattern will be unset
     * @param[in] bitmask The bit pattern to clear in the mask
     */
    template <typename T>
    void clearMask(lsst::afw::image::Mask<T> &target, T bitmask) const;

    // SpanSet functions
    /** Determine the common points between two SpanSets, and create a new SpanSet
     *
     * @param other The other SpanSet with which to intersect with
     */
    std::shared_ptr<SpanSet> intersect(SpanSet const &other) const;

    /** Determine the common points between a SpanSet and a Mask with a given bit pattern
     *
     * @tparam T Pixel type of the Mask
     *
     * @param other Mask with which to calculate intersection
     * @param bitmask The bit value to consider when intersecting
     */
    template <typename T>
    std::shared_ptr<SpanSet> intersect(image::Mask<T> const &other, T bitmask) const;

    /** @brief Determine the common points between a SpanSet and the logical inverse of a second SpanSet
     *         and return them in a new SpanSet.
     *
     * @param other The spanset which will be logically inverted when computing the intersection
     */
    std::shared_ptr<SpanSet> intersectNot(SpanSet const &other) const;

    /** @brief Determine the common points between a SpanSet and the logical inverse of a Mask for a
     *  given bit pattern
     *
     * @param other Mask with which to calculate intersection
     * @param bitmask The bit value to consider when intersecting
     *
     * @tparam T Pixel type of the Mask
     */
    template <typename T>
    std::shared_ptr<SpanSet> intersectNot(image::Mask<T> const &other, T bitmask) const;

    /** Create a new SpanSet that contains all points from two SpanSets
     *
     * @param other The SpanSet from which the union will be calculated
     */
    std::shared_ptr<SpanSet> union_(SpanSet const &other) const;

    /** Determine the union between a SpanSet and a Mask for a given bit pattern
     *
     * @tparam T Pixel type of the Mask
     *
     * @param other Mask with which to calculate intersection
     * @param bitmask The bit value to consider when intersecting
     */
    template <typename T>
    std::shared_ptr<SpanSet> union_(image::Mask<T> const &other, T bitmask) const;

    // Comparison Operators

    /** Compute equality between two SpanSets
     *
     * @param other The SpanSet for which equality will be computed
     */
    bool operator==(SpanSet const &other) const;

    /* Compute inequality between two SpanSets
     *
     * @param other The SpanSet for which inequality will be computed
     */
    bool operator!=(SpanSet const &other) const;

    /** Factory function for creating SpanSets from a Stencil
     *
     * @param r radius of the stencil, the length is inclusive i.e. 3 ranges from -3 to 3
     * @param s must be an enumeration of type geom::Stencil. Specifies the shape of the
     *          newly created SpanSet. May be CIRCLE, MANHATTAN, or BOX
     * @param offset This function usually creates a SpanSet centered about zero. This
                     parameter is a point2I object which specifies an offset from zero
                     to apply when creating the SpanSet.
    */
    static std::shared_ptr<geom::SpanSet> fromShape(int r, Stencil s = Stencil::CIRCLE,
                                                    lsst::geom::Point2I offset = lsst::geom::Point2I());

    /** Factory function for creating SpanSets from an ellipse object
     *
     * @param ellipse An ellipse defining the region to create a SpanSet from
     */
    static std::shared_ptr<geom::SpanSet> fromShape(geom::ellipses::Ellipse const &ellipse);

    /** Create a SpanSet from a mask.
     *
     * Create a SpanSet from a class. The default behavior is to include any pixels which have any
     * bits set. More complex selection/filtering of bit patterns can be done by supplying a comparator
     * function.
     *
     * @tparam T Pixel type of the Mask
     * @tparam F Type of the functor
     *
     * @param mask mask to convert to a SpanSet
     * @param comparator Functor object to use in the decision to include pixel in SpanSet. Should return
     *                   true when a given pixel in the mask should be part of the SpanSet, and false
     *                   otherwise. The functor takes a single value taken from the mask at the
     *                   pixel under consideration. Defaults to evaluating true if the mask has bits set,
     *                   and false otherwise.
     */
    template <typename T, typename UnaryPredicate = details::AnyBitSetFunctor<T>>
    static std::shared_ptr<geom::SpanSet> fromMask(
            image::Mask<T> const &mask, UnaryPredicate comparator = details::AnyBitSetFunctor<T>()) {
        // Create a vector which will hold all the spans created from the mask
        std::vector<Span> tempVec;
        // Grab some variables that will be used in the loop, so that they do not need to be fetched
        // every iteration.
        auto const maskArray = mask.getArray();
        auto const minPoint = mask.getBBox().getMin();
        auto const dimensions = maskArray.getShape();
        auto const minY = minPoint.getY();
        auto const minX = minPoint.getX();
        auto const dimMinusOne = dimensions[1] - 1;
        auto const yDim = dimensions[0];
        auto const xDim = dimensions[1];
        auto arrIter = maskArray.begin();
        for (size_t y = 0; y < yDim; ++y) {
            auto yWithOffset = y + minY;
            bool inSpan = false;  // are we currently in a span of interest?
            std::size_t start;    // starting x value of span we're currently in
            for (size_t x = 0; x < xDim; ++x) {
                bool compareValue = comparator((*arrIter)[x]);
                if (inSpan) {
                    // If we were in a span but now the condition isn't satisfied, it means the Span stops
                    // at the previous pixel.
                    if (!compareValue) {
                        tempVec.push_back(Span(yWithOffset, start + minX, x - 1 + minX));
                        inSpan = false;
                    }
                } else if (compareValue) {
                    // If we weren't in a span but now the condition is satisfied, we start a new span.
                    inSpan = true;
                    start = x;
                }
            }
            // Since the x loop is over, if we're still in a span, this means the Span was not
            // closed out and added to the vector. The last pixel should be included in the Span
            // and the Span should be closed and added to the vector of spans.
            if (inSpan) {
                tempVec.push_back(Span(yWithOffset, start + minX, dimMinusOne + minX));
            }
            ++arrIter;
        }

        // construct a SpanSet from the spans determined above
        return std::make_shared<SpanSet>(std::move(tempVec), false);
    }

    /** Create a SpanSet from a mask.
     *
     * Create a SpanSet from a Mask at pixels with the specified bit pattern
     *
     * @tparam T Pixel type of the Mask
     *
     * @param mask mask to convert to a SpanSet
     * @param bitmask bit pattern used to specify which pixel to include
     */
    template <typename T>
    static std::shared_ptr<geom::SpanSet> fromMask(image::Mask<T> const &mask, T bitmask) {
        return fromMask(mask, [bitmask](T const &bitPattern) { return bitPattern & bitmask; });
    }

    /** Split a discontinuous SpanSet into multiple SpanSets which are contiguous
     */
    std::vector<std::shared_ptr<geom::SpanSet>> split() const;

    bool isPersistable() const noexcept override { return true; }

    /**
     * Select pixels within the SpanSet which touch its edge
     *
     */
    std::shared_ptr<geom::SpanSet> findEdgePixels() const;

private:
    /* Returns the name used by the persistence layer to identify the SpanSet class
     */
    std::string getPersistenceName() const override;

    /* Return a string corresponding to the python module that SpanSets lives in
     */
    inline std::string getPythonModule() const override { return "lsst.afw.geom"; }

    /* Writes the representation of the class out to an output archive
     */
    void write(OutputArchiveHandle &handle) const override;

    /* A class which is used by the persistence layer to restore SpanSets from an archive
     */
    friend class SpansSetFactory;

    /* A function to combine overlapping Spans in a SpanSet into a single Span
     */
    void _runNormalize();

    /* Initializes the SpanSet class. Contains code that is common to multiple constructors
     */
    void _initialize();

    /* Label Spans according to contiguous group. If the SpanSet is contiguous, all Spans will be labeled 1.
     * If there is more than one group each group will receive a label one higher than the previous.
     */
    void _label(geom::Span const &spn, std::vector<std::size_t> &labelVector, std::size_t currentLabel,
                std::unordered_map<int, std::vector<std::pair<std::size_t, const Span *>>> &sortVector) const;
    std::pair<std::vector<std::size_t>, std::size_t> _makeLabels() const;

    std::shared_ptr<SpanSet> makeShift(int x, int y) const;

    template <typename F, typename... T>
    void applyFunctorImpl(F &&f, T... args) const {
        /* Implementation for applying functors, loop over each of the spans, and then
         * each point. Use the get method in the getters to fetch the value and pass
         * the point, and the values to the functor
         */
        // make sure that the SpanSet is within the bounds of functor arguments
        details::variadicBoundChecker(_bbox, _area, args...);
        for (auto const &spn : _spanVector) {
            // Set the current span in the getter, useful for optimizing value lookups
            details::variadicSpanSetter(spn, args...);
            for (int x = spn.getX0(); x <= spn.getX1(); ++x) {
                lsst::geom::Point2I point(x, spn.getY());
                f(point, args.get()...);
                details::variadicIncrementPosition(args...);
            }
        }
    }

    // Vector to hold the Spans contained in the SpanSet
    std::vector<Span> _spanVector;

    // Box that is large enough to bound all pixels in the SpanSet
    lsst::geom::Box2I _bbox;

    // Number of pixels in the SpanSet
    std::size_t _area;
};
}  // namespace geom
}  // namespace afw
}  // namespace lsst

#endif  // LSST_AFW_GEOM_SPANSET_H
