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
#include "lsst/cpputils/python.h"

#include "lsst/afw/image/ImageSlice.h"

namespace nb = nanobind;

using namespace nb::literals;

namespace lsst {
namespace afw {
namespace image {
namespace {

template <typename PixelT>
static void declareImageSlice(lsst::cpputils::python::WrapperCollection &wrappers, std::string const &suffix) {
    using Class = ImageSlice<PixelT>;

    wrappers.wrapType(
            nb::class_<Class, Image<PixelT>>(wrappers.module,
                                                                     ("ImageSlice" + suffix).c_str()),
            [](auto &mod, auto &cls) {
                cls.def(nb::init<Image<PixelT> const &>(), "img"_a);

                nb::enum_<typename Class::ImageSliceType>(cls, "ImageSliceType")
                        .value("ROW", Class::ImageSliceType::ROW)
                        .value("COLUMN", Class::ImageSliceType::COLUMN)
                        .export_values();

                cls.def("getImageSliceType", &Class::getImageSliceType);

                cls.def(
                        "__add__",
                        [](ImageSlice<PixelT> &self, Image<PixelT> const &other) { return self + other; },
                        nb::is_operator());
                cls.def(
                        "__mul__",
                        [](ImageSlice<PixelT> &self, Image<PixelT> const &other) { return self * other; },
                        nb::is_operator());
                cls.def("__iadd__", [](ImageSlice<PixelT> &self, Image<PixelT> const &other) {
                    self += other;
                    return self;
                });
                cls.def("__imul__", [](ImageSlice<PixelT> &self, Image<PixelT> const &other) {
                    self *= other;
                    return self;
                });

                cls.def(
                        "__add__",
                        [](Image<PixelT> const &self, ImageSlice<PixelT> const &other) {
                            return self + other;
                        },
                        nb::is_operator());
                cls.def(
                        "__sub__",
                        [](Image<PixelT> const &self, ImageSlice<PixelT> const &other) {
                            return self - other;
                        },
                        nb::is_operator());
                cls.def(
                        "__mul__",
                        [](Image<PixelT> const &self, ImageSlice<PixelT> const &other) {
                            return self * other;
                        },
                        nb::is_operator());
                cls.def(
                        "__truediv__",
                        [](Image<PixelT> const &self, ImageSlice<PixelT> const &other) {
                            return self / other;
                        },
                        nb::is_operator());
                cls.def("__iadd__", [](Image<PixelT> &self, ImageSlice<PixelT> const &other) {
                    self += other;
                    return self;
                });
                cls.def("__isub__", [](Image<PixelT> &self, ImageSlice<PixelT> const &other) {
                    self -= other;
                    return self;
                });
                cls.def("__imul__", [](Image<PixelT> &self, ImageSlice<PixelT> const &other) {
                    self *= other;
                    return self;
                });
                cls.def("__itruediv__", [](Image<PixelT> &self, ImageSlice<PixelT> const &other) {
                    self /= other;
                    return self;
                });
            });
}
}  // namespace
void wrapImageSlice(lsst::cpputils::python::WrapperCollection &wrappers) {
    declareImageSlice<float>(wrappers, "F");
    declareImageSlice<double>(wrappers, "D");
}
}  // namespace image
}  // namespace afw
}  // namespace lsst
