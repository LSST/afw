// -*- LSST-C++ -*-
#if !defined(LSST_AFW_MATH_APPROXIMATE_H)
#define LSST_AFW_MATH_APPROXIMATE_H

/*
 * LSST Data Management System
 * Copyright 2008-2015 AURA/LSST.
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
#include "lsst/base.h"
#include "lsst/geom.h"

namespace lsst {
namespace afw {
namespace image {
template <typename PixelT>
class Image;
template <typename PixelT, typename U, typename V>
class MaskedImage;
}  // namespace image
namespace math {
template <typename T>
class Approximate;

/**
 * Control how to make an approximation
 *
 * @note the x- and y-order must be the same, due to a limitation of Chebyshev1Function2
 *
 * @ingroup afw
 */
class ApproximateControl {
public:
    /// Choose the type of approximation to use
    enum Style {
        UNKNOWN = -1,
        CHEBYSHEV,  ///< Use a 2-D Chebyshev polynomial
        NUM_STYLES
    };

    /** ctor
     *
     * @param style Type of approximation
     * @param orderX Order of approximation to use in x-direction
     * @param orderY Order of approximation to use in y-direction
     * @param weighting Use inverse variance weighting?
     */
    ApproximateControl(Style style, int orderX, int orderY = -1, bool weighting = true);
    /// Return the Style
    Style getStyle() const { return _style; }
    /// Set the Style
    void setStyle(Style const style) { _style = style; }
    /// Return the order of approximation to use in the x-direction
    int getOrderX() const { return _orderX; }
    /// Set the order of approximation to use in the x-direction
    void setOrderX(int const orderX) { _orderX = orderX; }
    /// Return the order of approximation to use in the y-direction
    int getOrderY() const { return _orderY; }
    /// Set the order of approximation to use in the y-direction
    void setOrderY(int const orderY) { _orderY = orderY; }
    /// Return whether inverse variance weighting will be used in calculation
    bool getWeighting() const { return _weighting; }
    /// Set whether inverse variance weighting will be used in calculation
    void setWeighting(bool const weighting) { _weighting = weighting; }

private:
    Style _style;
    int _orderX;
    int _orderY;
    bool _weighting;
};

/**
 * Construct a new Approximate object, inferring the type from the type of the given MaskedImage.
 *
 * @param x the x-values of points
 * @param y the y-values of points
 * @param im The values at (x, y)
 * @param bbox Range where approximation should be valid
 * @param ctrl desired approximation algorithm
 */
template <typename PixelT>
std::shared_ptr<Approximate<PixelT>> makeApproximate(std::vector<double> const& x,
                                                     std::vector<double> const& y,
                                                     image::MaskedImage<PixelT> const& im,
                                                     lsst::geom::Box2I const& bbox,
                                                     ApproximateControl const& ctrl);

/**
 * Approximate values for a MaskedImage
 */
template <typename PixelT>
class Approximate {
public:
    using OutPixelT = float;  ///< The pixel type of returned images

    friend std::shared_ptr<Approximate<PixelT>> makeApproximate<>(std::vector<double> const& x,
                                                                  std::vector<double> const& y,
                                                                  image::MaskedImage<PixelT> const& im,
                                                                  lsst::geom::Box2I const& bbox,
                                                                  ApproximateControl const& ctrl);
    Approximate(Approximate const&) = delete;
    Approximate(Approximate&&) = delete;
    Approximate& operator=(Approximate const&) = delete;
    Approximate& operator=(Approximate&&) = delete;

    /// dtor
    virtual ~Approximate() = default;
    /// Return the approximate %image as a Image
    std::shared_ptr<image::Image<OutPixelT>> getImage(int orderX = -1, int orderY = -1) const {
        return doGetImage(orderX, orderY);
    }
    /// Return the approximate %image as a MaskedImage
    std::shared_ptr<image::MaskedImage<OutPixelT>> getMaskedImage(int orderX = -1, int orderY = -1) const {
        return doGetMaskedImage(orderX, orderY);
    }

protected:
    /**
     * Base class ctor
     */
    Approximate(std::vector<double> const& x,   ///< the x-values of points
                std::vector<double> const& y,   ///< the y-values of points
                lsst::geom::Box2I const& bbox,  ///< Range where approximation should be valid
                ApproximateControl const& ctrl  ///< desired approximation algorithm
                )
            : _xVec(x), _yVec(y), _bbox(bbox), _ctrl(ctrl) {}

    std::vector<double> const _xVec;  ///< the x-values of points
    std::vector<double> const _yVec;  ///< the y-values of points
    lsst::geom::Box2I const _bbox;    ///< Domain for approximation
    ApproximateControl const _ctrl;   ///< desired approximation algorithm
private:
    virtual std::shared_ptr<image::Image<OutPixelT>> doGetImage(int orderX, int orderY) const = 0;
    virtual std::shared_ptr<image::MaskedImage<OutPixelT>> doGetMaskedImage(int orderX, int orderY) const = 0;
};
}  // namespace math
}  // namespace afw
}  // namespace lsst

#endif  // LSST_AFW_MATH_APPROXIMATE_H
