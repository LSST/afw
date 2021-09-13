// -*- lsst-c++ -*-

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
 * An example executible which calls the statisticsStack function
 *
 */
#include <iostream>
#include <memory>

#include "lsst/geom.h"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/math/Stack.h"
#include "lsst/afw/image/ImageSlice.h"

namespace image = lsst::afw::image;
namespace math = lsst::afw::math;
using ImageF = image::Image<float>;
using ImageSliceF = image::ImageSlice<float>;
using MImageF = image::MaskedImage<float>;

int main(int argc, char **argv) {
    int const nX = 8;
    int const nY = 8;

    // fill an image with a gradient
    // - we want something different in x and y so we can see the different projections
    std::shared_ptr<ImageF> img = std::make_shared<ImageF>(lsst::geom::Extent2I(nX, nY), 0);
    for (int y = 0; y < img->getHeight(); ++y) {
        int x = 0;
        for (ImageF::x_iterator ptr = img->row_begin(y), end = img->row_end(y); ptr != end; ++ptr, ++x) {
            *ptr = 1.0 * x + 2.0 * y;
        }
    }

    // collapse with a MEAN over 'x' (ie. avg all columns to one), then 'y' (avg all rows to one)
    std::shared_ptr<MImageF> imgProjectCol = math::statisticsStack(*img, math::MEAN, 'x');
    std::shared_ptr<MImageF> imgProjectRow = math::statisticsStack(*img, math::MEAN, 'y');

    ImageSliceF slc = ImageSliceF(*(imgProjectCol->getImage()));

    std::shared_ptr<ImageF> opColPlus(new ImageF(*img, true));
    *opColPlus += slc;
    std::shared_ptr<ImageF> opColMinus = *img - slc;

    std::shared_ptr<ImageF> opColMult = *img * slc;
    std::shared_ptr<ImageF> opColDiv = *img / slc;

    ImageSliceF rowSlice = ImageSliceF(*(imgProjectRow->getImage()));
    std::vector<std::shared_ptr<ImageF>> rows;
    rows.push_back(*img + rowSlice);
    rows.push_back(*img - rowSlice);
    rows.push_back(*img * rowSlice);
    rows.push_back(*img / rowSlice);

    // output the pixel values and show the statistics projections

    for (auto const &row : rows) {
        ImageF::x_iterator end = row->row_end(0);
        printf("%26s", " ");
        for (ImageF::x_iterator ptr = row->row_begin(0); ptr != end; ++ptr) {
            printf("%5.2f ", static_cast<float>(*ptr));
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    int y = 0;
    MImageF::y_iterator colEnd = imgProjectCol->col_end(0);
    for (MImageF::y_iterator pCol = imgProjectCol->col_begin(0); pCol != colEnd; ++pCol, ++y) {
        printf("%5.1f %5.1f %5.1f %5.2f : ", (*opColPlus)(0, y), (*opColMinus)(0, y), (*opColMult)(0, y),
               (*opColDiv)(0, y));
        for (ImageF::x_iterator ptr = img->row_begin(y), end = img->row_end(y); ptr != end; ++ptr) {
            printf("%5.2f ", static_cast<float>(*ptr));
        }
        printf(" ==> %5.2f +/- %5.3f\n", pCol.image(), sqrt(pCol.variance()));
    }
    std::cout << std::endl;

    MImageF::x_iterator rowEnd = imgProjectRow->row_end(0);
    printf("%26s", " ");
    for (MImageF::x_iterator ptr = imgProjectRow->row_begin(0); ptr != rowEnd; ++ptr) {
        printf("%5.2f ", static_cast<float>(ptr.image()));
    }

    std::cout << std::endl;
}
