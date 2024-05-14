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

#include "nanobind/nanobind.h"
#include "nanobind/stl/vector.h"
#include "lsst/cpputils/python.h"

#include "lsst/afw/image/ImagePca.h"

namespace nb = nanobind;

using namespace nb::literals;

namespace lsst {
namespace afw {
namespace image {

namespace {

template <typename ImageT>
static void declareImagePca(lsst::cpputils::python::WrapperCollection &wrappers, std::string const &suffix) {
    std::string name = "ImagePca" + suffix;
    wrappers.wrapType(
            nb::class_<ImagePca<ImageT>>(wrappers.module, name.c_str()),
            [](auto &mod, auto &cls) {
                //  nb::class_<ImagePca<ImageT>, std::shared_ptr<ImagePca<ImageT>>> cls(mod, ("ImagePca" +
                //  suffix).c_str());

                cls.def(nb::init<bool>(), "constantWeight"_a = true);

                cls.def("addImage", &ImagePca<ImageT>::addImage, "img"_a, "flux"_a = 0.0);
                cls.def("getImageList", &ImagePca<ImageT>::getImageList);
                cls.def("getDimensions", &ImagePca<ImageT>::getDimensions);
                cls.def("getMean", &ImagePca<ImageT>::getMean);
                cls.def("analyze", &ImagePca<ImageT>::analyze);
                cls.def("updateBadPixels", &ImagePca<ImageT>::updateBadPixels);
                cls.def("getEigenValues", &ImagePca<ImageT>::getEigenValues);
                cls.def("getEigenImages", &ImagePca<ImageT>::getEigenImages);
            });
}

template <typename Image1T, typename Image2T>
static void declareInnerProduct(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("innerProduct",
                (double (*)(Image1T const &, Image2T const &, int const))innerProduct<Image1T, Image2T>,
                "lhs"_a, "rhs"_a, "border"_a = 0);
    });
}

}  // namespace

void wrapImagePca(lsst::cpputils::python::WrapperCollection &wrappers) {
    declareImagePca<Image<int>>(wrappers, "I");
    declareImagePca<Image<float>>(wrappers, "F");
    declareImagePca<Image<double>>(wrappers, "D");
    declareImagePca<Image<std::uint16_t>>(wrappers, "U");
    declareImagePca<Image<std::uint64_t>>(wrappers, "L");
    declareImagePca<MaskedImage<int>>(wrappers, "MI");
    declareImagePca<MaskedImage<float>>(wrappers, "MF");
    declareImagePca<MaskedImage<double>>(wrappers, "MD");
    declareImagePca<MaskedImage<std::uint16_t>>(wrappers, "MU");
    declareImagePca<MaskedImage<std::uint64_t>>(wrappers, "ML");

    declareInnerProduct<Image<int>, Image<int>>(wrappers);
    declareInnerProduct<Image<float>, Image<float>>(wrappers);
    declareInnerProduct<Image<double>, Image<double>>(wrappers);
    declareInnerProduct<Image<std::uint16_t>, Image<std::uint16_t>>(wrappers);
    declareInnerProduct<Image<std::uint64_t>, Image<std::uint64_t>>(wrappers);

    declareInnerProduct<Image<float>, Image<double>>(wrappers);
    declareInnerProduct<Image<double>, Image<float>>(wrappers);
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
