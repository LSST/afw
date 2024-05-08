/*
 * LSST Data Management System
 * Copyright 2017 LSST/AURA.
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

#ifndef LSST_AFW_IMAGE_TRANSMISSIONCURVE_H_INCLUDED
#define LSST_AFW_IMAGE_TRANSMISSIONCURVE_H_INCLUDED

#include "ndarray_fwd.h"

#include "lsst/afw/geom/Transform.h"
#include "lsst/geom/Box.h"
#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/typehandling/Storable.h"

namespace lsst {
namespace afw {
namespace image {

/**
 *  A spatially-varying transmission curve as a function of wavelength.
 *
 *  TransmissionCurve can only be evaluated at discrete (albeit arbitrary)
 *  user-provided positions; it does not provide an interface for computing
 *  average transmission over regions or computing spatially-varying scalars
 *  from integrals over the wavelength dimension.
 *
 *  TransmissionCurves are immutable and are expected to be passed and held by
 *  shared_ptr<TransmissionCurve const>.  As such they are neither copyable
 *  nor movable (because there should be no need to copy or move).
 *
 *  All wavelength values should be in Angstroms.
 *
 *  The flux units and overall normalization of TransmissionCurves is
 *  unspecified by the class, but their normalization and units should always
 *  be consistent throughout the spatial area over which they are defined
 *  (an implementation should not e.g. re-normalize to unit bolometric flux
 *  at each position it is evaluated at).  Other classes and functions using
 *  TransmissionCurves should of course document the flux units and/or
 *  normalization expected/provided.
 *
 * @note TransmissionCurve is an interface to various C++-only implementation
 * classes. Because these classes are not mapped to Python, some references to
 * a TransmissionCurve object may appear in Python as a table::io::Persistable
 * or typehandling::Storable instead. The best way to avoid this at present is
 * to call functions that, in C++, explicitly return a TransmissionCurve.
 */
class TransmissionCurve : public typehandling::Storable,
                          public std::enable_shared_from_this<TransmissionCurve> {
DECLARE_PERSISTABLE_FACADE(TransmissionCurve);
public:
    /**
     *  Create a new TranmissionCurve that has unit thoughput at all wavelengths everywhere.
     */
    static std::shared_ptr<TransmissionCurve const> makeIdentity();

    /**
     *  Create a new TransmissionCurve with spatially-constant throughput.
     *
     *  @param[in]  throughput      an Array of throughput values with the same
     *                              size as the wavelengths Array (will be copied).
     *  @param[in]  wavelengths     an Array of wavelengths in Angstroms (will be copied).
     *                              Must be monotonically increasing.
     *  @param[in]  throughputAtMin the throughput value used for wavelengths
     *                              below wavelengths.front().
     *  @param[in]  throughputAtMax the throughput value used for wavelengths
     *                              above wavelengths.back().
     *
     *  Throughput outside the given wavelength domain is assumed to be constant.
     */
    static std::shared_ptr<TransmissionCurve const> makeSpatiallyConstant(
            ndarray::Array<double const, 1> const &throughput,
            ndarray::Array<double const, 1> const &wavelengths, double throughputAtMin = 0.0,
            double throughputAtMax = 0.0);

    /**
     *  Create a new TransmissionCurve with throughput varying as function of radius.
     *
     *  @param[in]  throughput      an Array of throughput values with shape
     *                              (wavelengths.size(), radii.size()).  Will be
     *                              copied.
     *  @param[in]  wavelengths     an Array of wavelengths in Angstroms (will be copied).
     *                              Must be monotonically increasing.
     *  @param[in]  radii           an Array of radii (will be copied).
     *                              Must be monotonically increasing.
     *  @param[in]  throughputAtMin the throughput value used for wavelengths
     *                              below wavelengths.front().
     *  @param[in]  throughputAtMax the throughput value used for wavelengths
     *                              above wavelengths.back().
     *
     *  Throughput outside the given wavelength or radius domain is assumed
     *  to be constant.
     */
    static std::shared_ptr<TransmissionCurve const> makeRadial(
            ndarray::Array<double const, 2> const &throughput,
            ndarray::Array<double const, 1> const &wavelengths, ndarray::Array<double const, 1> const &radii,
            double throughputAtMin = 0.0, double throughputAtMax = 0.0);

    /**
     *  Return a new TransmissionCurve that simply multiplies the values of two others.
     *
     *  The new TransmissionCurve's "natural sampling" will be defined such
     *  that the spcaing is no larger than the spacing of either of the
     *  inputs. The minimum and maximum will be set to include the minimum and
     *  maximum of both operands except where one operand is exactly zero and
     *  hence the value of the other can be ignored.
     *
     *  @note This is also mapped to __mul__ in Python (overriding operator*
     *        in C++ would be problematic due to the use of shared_ptr).
     */
    std::shared_ptr<TransmissionCurve const> multipliedBy(TransmissionCurve const &other) const;

    /**
     *  Return a view of a TransmissionCurve in a different coordinate system.
     *
     *  The returned TransmissionCurve will be equivalent to one whose `sampleAt`
     *  method calls first calls `transform.applyInverse` on the given point
     *  and then calls `base.sampleAt` on the result.
     */
    std::shared_ptr<TransmissionCurve const> transformedBy(
            std::shared_ptr<geom::TransformPoint2ToPoint2> transform) const;

    // TransmissionCurve is not copyable.
    TransmissionCurve(TransmissionCurve const &) = delete;

    // TransmissionCurve is not movable.
    TransmissionCurve(TransmissionCurve &&) = delete;

    // TransmissionCurve is not copy-assignable.
    TransmissionCurve &operator=(TransmissionCurve const &) = delete;

    // TransmissionCurve is not move-assignable.
    TransmissionCurve &operator=(TransmissionCurve &&) = delete;

    ~TransmissionCurve() override = default;

    /**
     *  Return the wavelength interval on which this TransmissionCurve varies.
     *
     *  Throghputs beyond the min and max values will by set by `sampleAt` to
     *  the values returned by `getThroughputAtBounds()`.
     *
     *  Min and/or max values may be infinite to indicate an analytic curve
     *  with no wavelength bounds.
     */
    virtual std::pair<double, double> getWavelengthBounds() const = 0;

    /**
     *  Return the throughput value that will be returned for wavelengths
     *  below and above getWavelenthBounds().first and .second (respectively).
     */
    virtual std::pair<double, double> getThroughputAtBounds() const = 0;

    /**
     *  Evaluate the throughput at a position into a provided output array.
     *
     *  @param[in]  position     Spatial position at which to evaluate.
     *  @param[in]  wavelengths  Wavelengths at which to evaluate.
     *
     *  @param[in,out]  out      Computed throughput values.  Must be pre-
     *                           allocated to the same size as the wavelengths
     *                           array.
     *
     *  @throw Throws pex::exceptions::LengthError if the size of the
     *         `wavelengths` and `out` arrays differ.
     *
     *  @exceptsafe Provides basic exception safety: the `out` array values
     *              may be modified if an exception is thrown.
     */
    virtual void sampleAt(lsst::geom::Point2D const &position,
                          ndarray::Array<double const, 1, 1> const &wavelengths,
                          ndarray::Array<double, 1, 1> const &out) const = 0;

    /**
     *  Evaluate the throughput at a position into a new array.
     *
     *  @param[in]  position     Spatial position at which to evaluate.
     *  @param[in]  wavelengths  Wavelengths at which to evaluate.
     *
     *  @return  Computed throughput values, in an array with the same size as
     *           wavelengths.
     */
    ndarray::Array<double, 1, 1> sampleAt(lsst::geom::Point2D const &position,
                                          ndarray::Array<double const, 1, 1> const &wavelengths) const;

protected:
    /**
     *  Polymorphic implementation for transformedBy().
     *
     *  The default implementation of this method creates a new
     *  TransmissionCurve that lazily applies the given transform to points
     *  before evaluating the original TransmissionCurve, which should be
     *  appropriate for nearly all concrete TransmissionCurve subclases.
     *
     *  @param[in] transform  A transform to that maps the coordinate system
     *                        of the returned transform to that of this.
     */
    virtual std::shared_ptr<TransmissionCurve const> _transformedByImpl(
            std::shared_ptr<geom::TransformPoint2ToPoint2> transform) const;

    /**
     *  One-way polymorphic implementation for multipliedBy().
     *
     *  The default implementation simply returns `nullptr`, which indicates to
     *  `multiply()` that it should call
     *  `other->_multiplyImply(shared_from_this())`.  If that returns `nullptr`
     *  as well, `multiply` will construct a new `TransmisionCurve` whose
     *  `sampleAt` method delegates to both operands and then multiplies the
     *  results.
     *
     *  @param[in] other      The other TransmissionCurve to multiply with self.
     */
    virtual std::shared_ptr<TransmissionCurve const> _multipliedByImpl(
            std::shared_ptr<TransmissionCurve const> other) const;

    TransmissionCurve() = default;

    std::string getPythonModule() const override;
};

}  // namespace image
}  // namespace afw
}  // namespace lsst

#endif  // !LSST_AFW_IMAGE_TRANSMISSIONCURVE_H_INCLUDED