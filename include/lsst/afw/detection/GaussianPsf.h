// -*- LSST-C++ -*-
/*
 * LSST Data Management System
 * Copyright 2008-2014 LSST Corporation.
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
#ifndef LSST_AFW_DETECTION_GaussianPsf_h_INCLUDED
#define LSST_AFW_DETECTION_GaussianPsf_h_INCLUDED

#include "lsst/geom.h"
#include "lsst/afw/detection/Psf.h"

namespace lsst {
namespace afw {
namespace detection {

/**
 *  A circularly symmetric Gaussian Psf class with no spatial variation, intended mostly for
 *         testing purposes.
 *
 *  This class is essentially an alternate implementation of meas::algorithms::SingleGaussianPsf;
 *  While SingleGaussianPsf inherits from ImagePsf and KernelPsf, and hence delegates to those
 *  various operations relating to the PSF model image (e.g. computeShape()), GaussianPsf computes
 *  these analytically.
 */
class GaussianPsf : public Psf {
DECLARE_PERSISTABLE_FACADE(Psf);
public:
    /**
     *  Constructor for a GaussianPsf
     *
     *  @param[in] width   Number of columns in realizations of the PSF at a point.
     *  @param[in] height  Number of rows in realizations of the PSF at a point.
     *  @param[in] sigma   Radius of the Gaussian.
     */
    GaussianPsf(int width, int height, double sigma);

    /**
     *  Constructor for a GaussianPsf
     *
     *  @param[in] dimensions     Number of columns, rows in realizations of the PSF at a point.
     *  @param[in] sigma   Radius of the Gaussian.
     */
    GaussianPsf(lsst::geom::Extent2I const& dimensions, double sigma);

    ~GaussianPsf() override;
    GaussianPsf(GaussianPsf const&);
    GaussianPsf(GaussianPsf&&);
    GaussianPsf& operator=(GaussianPsf const&) = delete;
    GaussianPsf& operator=(GaussianPsf&&) = delete;

    /// Polymorphic deep copy; should usually be unnecessary because Psfs are immutable.
    std::shared_ptr<afw::detection::Psf> clone() const override;

    /// Return a clone with specified kernel dimensions
    std::shared_ptr<afw::detection::Psf> resized(int width, int height) const override;

    /// Return the dimensions of the images returned by computeImage()
    lsst::geom::Extent2I getDimensions() const { return _dimensions; }

    /// Return the radius of the Gaussian.
    double getSigma() const { return _sigma; }

    /// Whether the Psf is persistable; always true.
    bool isPersistable() const noexcept override { return true; }

protected:
    std::string getPersistenceName() const override;

    std::string getPythonModule() const override;

    void write(OutputArchiveHandle& handle) const override;

private:
#if 0  // We could reimplement this more efficiently than what's in the base class,
      // but it's tricky to get the position right in all corner cases, and it's
      // not actually performance-critical, so we should just wait for #3116.
    virtual std::shared_ptr<Image> doComputeImage(
        lsst::geom::Point2D const & position, image::Color const& color
    ) const;
#endif

    std::shared_ptr<Image> doComputeKernelImage(lsst::geom::Point2D const& position,
                                                image::Color const& color) const override;

    double doComputeApertureFlux(double radius, lsst::geom::Point2D const& position,
                                 image::Color const& color) const override;

    geom::ellipses::Quadrupole doComputeShape(lsst::geom::Point2D const& position,
                                              image::Color const& color) const override;

    lsst::geom::Box2I doComputeBBox(lsst::geom::Point2D const& position,
                                    image::Color const& color) const override;

    lsst::geom::Extent2I _dimensions;
    double _sigma;
};
}  // namespace detection
}  // namespace afw
}  // namespace lsst

#endif  // !LSST_AFW_DETECTION_Psf_h_INCLUDED
