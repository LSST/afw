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
 * Bin an Image or MaskedImage by an integral factor (the same in x and y)
 */

#include "lsst/pex/exceptions.h"
#include "lsst/afw/math/offsetImage.h"

namespace pexExcept = lsst::pex::exceptions;

namespace lsst {
namespace afw {
namespace math {

template <typename ImageT>
std::shared_ptr<ImageT> binImage(ImageT const& in, int const binsize, lsst::afw::math::Property const flags) {
    return binImage(in, binsize, binsize, flags);
}

template <typename ImageT>
std::shared_ptr<ImageT> binImage(ImageT const& in, int const binX, int const binY,
                                 lsst::afw::math::Property const flags) {
    if (flags != lsst::afw::math::MEAN) {
        throw LSST_EXCEPT(pexExcept::InvalidParameterError,
                          (boost::format("Only afwMath::MEAN is supported, saw 0x%x") % flags).str());
    }
    if (binX <= 0 || binY <= 0) {
        throw LSST_EXCEPT(pexExcept::DomainError,
                          (boost::format("Binning must be >= 0, saw %dx%d") % binX % binY).str());
    }

    int const outWidth = in.getWidth() / binX;
    int const outHeight = in.getHeight() / binY;

    std::shared_ptr<ImageT> out =
            std::shared_ptr<ImageT>(new ImageT(lsst::geom::Extent2I(outWidth, outHeight)));
    out->setXY0(in.getXY0());
    *out = typename ImageT::SinglePixel(0);

    for (int oy = 0, iy = 0; oy < out->getHeight(); ++oy) {
        for (int i = 0; i != binY; ++i, ++iy) {
            typename ImageT::x_iterator optr = out->row_begin(oy);
            for (typename ImageT::x_iterator iptr = in.row_begin(iy), iend = iptr + binX * outWidth;
                 iptr < iend;) {
                typename ImageT::SinglePixel val = *iptr;
                ++iptr;
                for (int j = 1; j != binX; ++j, ++iptr) {
                    val += *iptr;
                }
                *optr += val;
                ++optr;
            }
        }
        for (typename ImageT::x_iterator ptr = out->row_begin(oy), end = out->row_end(oy); ptr != end;
             ++ptr) {
            *ptr /= binX * binY;
        }
    }

    return out;
}

//
// Explicit instantiations
//
/// @cond
#define INSTANTIATE(TYPE)                                                                                  \
    template std::shared_ptr<image::Image<TYPE>> binImage(image::Image<TYPE> const&, int,                  \
                                                          lsst::afw::math::Property const);                \
    template std::shared_ptr<image::Image<TYPE>> binImage(image::Image<TYPE> const&, int, int,             \
                                                          lsst::afw::math::Property const);                \
    template std::shared_ptr<image::MaskedImage<TYPE>> binImage(image::MaskedImage<TYPE> const&, int,      \
                                                                lsst::afw::math::Property const);          \
    template std::shared_ptr<image::MaskedImage<TYPE>> binImage(image::MaskedImage<TYPE> const&, int, int, \
                                                                lsst::afw::math::Property const);

INSTANTIATE(std::uint16_t)
INSTANTIATE(int)
INSTANTIATE(float)
INSTANTIATE(double)
/// @endcond
}  // namespace math
}  // namespace afw
}  // namespace lsst
