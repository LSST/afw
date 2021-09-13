// -*- LSST-C++ -*-

/*
 * This file is part of afw.
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <cmath>

#include "lsst/geom.h"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/math/Statistics.h"

using namespace std;
namespace image = lsst::afw::image;
namespace math = lsst::afw::math;

using ImageF = image::Image<float>;

int main() {
    // First we'll try a regular image
    ImageF img(lsst::geom::Extent2I(10, 40));
    img = 100000.0;

    {
        math::Statistics stats = math::makeStatistics(img, math::NPOINT | math::MEAN | math::STDEV);
        cout << "Npixel: " << stats.getValue(math::NPOINT) << endl;
        cout << "Mean: " << stats.getValue(math::MEAN) << endl;
        cout << "Error in mean: " << stats.getError(math::MEAN) << " (expect NaN)" << endl;
        cout << "Standard Deviation: " << stats.getValue(math::STDEV) << endl << endl;
    }

    {
        math::Statistics stats = math::makeStatistics(img, math::STDEV | math::MEAN | math::ERRORS);
        std::pair<double, double> mean = stats.getResult(math::MEAN);

        cout << "Mean: " << mean.first << " error in mean: " << mean.second << endl << endl;
    }

    {
        math::Statistics stats = math::makeStatistics(img, math::NPOINT);
        try {
            stats.getValue(math::MEAN);
        } catch (lsst::pex::exceptions::InvalidParameterError &e) {
            cout << "You didn't ask for the mean, so we caught an exception: " << e.what() << endl;
        }
    }

    return 0;
}
