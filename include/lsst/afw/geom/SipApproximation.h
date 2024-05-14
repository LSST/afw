// -*- lsst-c++ -*-
/*
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

#ifndef LSST_AFW_GEOM_SipApproximation_h_INCLUDED
#define LSST_AFW_GEOM_SipApproximation_h_INCLUDED

#include <memory>
#include <vector>
#include <lsst/geom/polynomials/PolynomialFunction2d.h>

#include "Eigen/Core"

#include "lsst/afw/geom/Transform.h"
#include "lsst/geom/Box.h"
#include "lsst/geom/LinearTransform.h"

namespace lsst { namespace afw { namespace geom {

/**
 *  A fitter and results class for approximating a general Transform in a form compatible with
 *  FITS WCS persistence.
 *
 *  The Simple Imaging Polynomial (SIP) convention (Shupe et al 2005) adds
 *  forward and reverse polynomial mappings to a standard projection FITS WCS
 *  projection (e.g. "TAN" for gnomonic) that relate Intermediate World
 *  Coordinates (see Calabretta & Greisen 2002) to image pixel coordinates.
 *  The SIP "forward" transform is defined by polynomial coeffients @f$A@f$
 *  and @f$B@f$ that map pixel coordinates @f$(u, v)@f$ to Intermediate World
 *  Coordinates @f$(x, y)@f$ via
 *  @f[
 *     \boldsymbol{S}\left[\begin{array}{c}
 *        x \\
 *        y
 *     \end{array}\right]
 *     \equiv
 *     \left[\begin{array}{c}
 *        x_s \\
 *        y_s
 *     \end{array}\right]
 *     =
 *     \left[\begin{array}{c}
 *        (u - u_0) + \displaystyle\sum_{p,q}^{0 \le p + q \le N} \mathrm{A}_{p,q} (u - u_0)^p (v - v_0)^q \\
 *        (v - v_0) + \displaystyle\sum_{p,q}^{0 \le p + q \le N} \mathrm{B}_{p,q} (u - u_0)^p (v - v_0)^q
 *     \end{array}\right]
 *  @f]
 *  The reverse transform has essentially the same form:
 *  @f[
 *     \left[\begin{array}{c}
 *        u - u_0 \\
 *        v - v_0
 *     \end{array}\right]
 *     =
 *     \left[\begin{array}{c}
 *        x_s + \displaystyle\sum_{p,q}^{0 \le p + q \le N} \mathrm{AP}_{p,q} x_s^p y_s^q \\
 *        y_s + \displaystyle\sum_{p,q}^{0 \le p + q \le N} \mathrm{BP}_{p,q} x_s^p y_s^q
 *     \end{array}\right]
 *  @f]
 *  In both cases, @f$(u_0, v_0)@f$ is the pixel origin (CRPIX in FITS WCS) and
 *  @f$\boldsymbol{S}@f$ is the *inverse* of the Jacobian "CD" matrix.  Both
 *  CRPIX and CD are considered fixed inputs, and we do not attempt to null
 *  the zeroth- and first-order terms of @f$A@f$ and @f$B@f$ (as some SIP
 *  fitters do); together, these conventions make solving for the coefficients
 *  a much simpler linear problem.
 *
 *  @note In the implementation, we typically refer to @f$(u-u_0, v-v_0)@f$ as
 *  @c dpix (for "pixel delta"), and @f$(x_s, y_s)@f$ as @c siwc (for "scaled
 *  intermediate world coordinates").
 *
 *  While LSST WCSs are in general too complex to be described exactly in FITS
 *  WCS, they can generally be closely approximated by standard FITS WCS
 *  projection with additional SIP distortions.  This class fits such an
 *  approximation, given a TransformPoint2ToPoint2 object that represents the
 *  exact mapping from pixels to Intermediate World Coordinates with a SIP
 *  distortion.
 */
class SipApproximation final {
public:

    /**
     *  Construct a new approximation by fitting on a grid of points.
     *
     *  @param[in]  pixelToIwc      The true Transform to approximate.  Should go
     *                              from pixels to Intermediate World Coordinates
     *                              when applyForward is called.
     *  @param[in]  crpix           Pixel origin, using the LSST 0-indexed
     *                              convention rather than the FITS 1-indexed
     *                              convention; equal to (CRPIX1 - 1, CRPIX2 - 1).
     *  @param[in]  cd              Nominal Jacobian ("CD" in FITS WCS).
     *  @param[in]  bbox            Pixel-coordinate bounding box over which the
     *                              approximation should be valid.  Used to construct
     *                              the grid of points to fit.
     *  @param[in]  gridShape       Number of points in x and y for the grid of points.
     *  @param[in]  order           Order of the polynomial (same for forward and
     *                              reverse transforms).
     *  @param[in]  useInverse      If true, the inverse SIP transform will be fit
     *                              and compared to data points generated by calls to
     *                              pixelToIwc.applyInverse instead of
     *                              pixelToIwc.applyForward.
     *  @param[in]  svdThreshold    Fraction of the largest singular value at which to
     *                              declare smaller singular values zero in the least
     *                              squares solution.  Negative values use Eigen's
     *                              internal default.
     *
     *  @throws lsst::pex::exceptions::InvalidParameterError Thrown if order is negative
     *      or gridShape is non-positive.
     *
     *  @exceptsafe strong
     */
    SipApproximation(
        std::shared_ptr<TransformPoint2ToPoint2> pixelToIwc,
        lsst::geom::Point2D const & crpix,
        Eigen::Matrix2d const & cd,
        lsst::geom::Box2D const & bbox,
        lsst::geom::Extent2I const & gridShape,
        int order,
        bool useInverse=true,
        double svdThreshold=-1
    );

    /**
     *  Construct from existing SIP coefficients.
     *
     *  This constructor is primarily intended for testing purposes.
     *
     *  @param[in]  pixelToIwc      The true Transform to approximate.  Should go
     *                              from pixels to Intermediate World Coordinates
     *                              when applyForward is called.
     *  @param[in]  crpix           Pixel origin, using the LSST 0-indexed
     *                              convention rather than the FITS 1-indexed
     *                              convention; equal to (CRPIX1 - 1, CRPIX - 1).
     *  @param[in]  cd              Nominal Jacobian ("CD" in FITS WCS).
     *  @param[in]  bbox            Pixel-coordinate bounding box over which the
     *                              approximation should be valid.  Used to construct
     *                              the grid of points to fit.
     *  @param[in]  gridShape       Number of points in x and y for the grid of points.
     *  @param[in]  a               Matrix of A coefficients, with the first dimension
     *                              corresponding to powers of @f$(u - u_0)@f$ and the
     *                              second corresponding to powers of @f$(v - v_0)@f$.
     *  @param[in]  b               Matrix of B coefficients, with the first dimension
     *                              corresponding to powers of @f$(u - u_0)@f$ and the
     *                              second corresponding to powers of @f$(v - v_0)@f$.
     *  @param[in]  ap              Matrix of AP coefficients, with the first dimension
     *                              corresponding to powers of @f$x_s@f$ and the
     *                              second corresponding to powers of @f$y_s@f$.
     *  @param[in]  bp              Matrix of BP coefficients, with the first dimension
     *                              corresponding to powers of @f$x_s@f$ and the
     *                              second corresponding to powers of @f$y_s@f$.
     *  @param[in]  useInverse      If true, the inverse SIP transform will be compared
     *                              to data points generated by calls to
     *                              pixelToIwc.applyInverse instead of
     *                              pixelToIwc.applyForward.
     *
     *  @throws lsst::pex::exceptions::InvalidParameterError Thrown if gridShape
     *       is non-positive, or any matrix argument is non-square.
     *
     *  @exceptsafe strong
     */
    SipApproximation(
        std::shared_ptr<TransformPoint2ToPoint2> pixelToIwc,
        lsst::geom::Point2D const & crpix,
        Eigen::Matrix2d const & cd,
        lsst::geom::Box2D const & bbox,
        lsst::geom::Extent2I const & gridShape,
        ndarray::Array<double const, 2> const & a,
        ndarray::Array<double const, 2> const & b,
        ndarray::Array<double const, 2> const & ap,
        ndarray::Array<double const, 2> const & bp,
        bool useInverse=true
    );

    // No copies just because they'd be a pain to implement and probably aren't necessary
    SipApproximation(SipApproximation const &) = delete;
    SipApproximation & operator=(SipApproximation const &) = delete;

    // Moves are fine.
    SipApproximation(SipApproximation &&) noexcept = default;
    SipApproximation & operator=(SipApproximation &&) noexcept = default;

    // Needs to be defined in .cc, where compiler can see the definitions of forward-declared
    // private implementation classes.
    ~SipApproximation() noexcept;

    /// Return the polynomial order of the current solution (same for forward and reverse).
    int getOrder() const noexcept;

    /**
     * Return a coefficient of the forward transform polynomial.
     *
     * Out-of-bounds arguments yields undefined behavior.
     *
     * @exceptsafe strong
     */
    double getA(int p, int q) const;

    /**
     * Return a coefficient of the forward transform polynomial.
     *
     * Out-of-bounds arguments yields undefined behavior.
     *
     * @exceptsafe strong
     */
    double getB(int p, int q) const;

    /**
     * Return a coefficient of the reverse transform polynomial.
     *
     * Out-of-bounds arguments yields undefined behavior.
     *
     * @exceptsafe strong
     */
    double getAP(int p, int q) const;

    /**
     * Return a coefficient of the reverse transform polynomial.
      *
     * Out-of-bounds arguments yields undefined behavior.
     *
     * @exceptsafe strong
    */
    double getBP(int p, int q) const;

    /// Return the coefficients of the forward transform polynomial.
    Eigen::MatrixXd getA() const noexcept;

    /// Return the coefficients of the forward transform polynomial.
    Eigen::MatrixXd getB() const noexcept;

    /// Return the coefficients of the reverse transform polynomial.
    Eigen::MatrixXd getAP() const noexcept;

    /// Return the coefficients of the reverse transform polynomial.
    Eigen::MatrixXd getBP() const noexcept;

    /**
     *  Convert a point from pixels to intermediate world coordinates.
     *
     *  This method is inefficient and should only be used for diagnostic purposes.
     *
     *  @exceptsafe strong
     */
    lsst::geom::Point2D applyForward(lsst::geom::Point2D const & pix) const;

    /**
     *  Convert an array of points from pixels to intermediate world coordinates.
     *
     *  @exceptsafe strong
     */
    std::vector<lsst::geom::Point2D> applyForward(std::vector<lsst::geom::Point2D> const & pix) const;

    /**
     *  Convert a point from intermediate world coordinates to pixels.
     *
     *  This method is inefficient and should only be used for diagnostic purposes.
     *
     *  @exceptsafe strong
     */
    lsst::geom::Point2D applyInverse(lsst::geom::Point2D const & iwcs) const;

    /**
     *  Convert an array of points from intermediate world coordinates to pixels.
     *
     *  @exceptsafe strong
     */
    std::vector<lsst::geom::Point2D> applyInverse(std::vector<lsst::geom::Point2D> const & iwcs) const;

    /// Return the distance between grid points in pixels.
    lsst::geom::Extent2D getGridStep() const noexcept;

    /// Return the number of grid points in x and y.
    lsst::geom::Extent2I getGridShape() const noexcept;

    /// Return the pixel-coordinate bounding box over which the approximation should be valid.
    lsst::geom::Box2D getBBox() const noexcept { return _bbox; }

    /// Return the pixel origin of the WCS being approximated.
    lsst::geom::Point2D getPixelOrigin() const noexcept { return lsst::geom::Point2D(_crpix); }

    /// Return the CD matrix of the WCS being approximated.
    Eigen::Matrix2d getCdMatrix() const noexcept { return _cdInv.inverted().getMatrix(); }

    /**
     *  Update the grid to the given number of points in x and y.
     *
     *  This does not invalidate or modify the current solution; this allows
     *  the user to fit with a coarse grid and then check whether the solution
     *  still works well on a finer grid.
     *
     *  @throws lsst::pex::exceptions::InvalidParameterError Thrown if shape is
     *     non-positive.
     *
     *  @exceptsafe strong
     */
    void updateGrid(lsst::geom::Extent2I const & shape);

    /**
     *  Update the grid by making it finer by a given integer factor.
     *
     *  @throws lsst::pex::exceptions::InvalidParameterError Thrown if factor is
     *     non-positive.
     *
     *  @exceptsafe strong
     */
    void refineGrid(int factor=2);

    /**
     *  Obtain a new solution at the given order with the current grid.
     *
     *  @param[in]  order           Polynomial order to fit.
     *  @param[in]  svdThreshold    Fraction of the largest singular value at which to
     *                              declare smaller singular values zero in the least
     *                              squares solution.  Negative values use Eigen's
     *                              internal default.
     *
     *  @throws pex::exceptions::LogicError Thrown if the number of free
     *          parameters implied by order is larger than the number of
     *          data points defined by the grid.
     *
     *  @exceptsafe strong
     */
    void fit(int order, double svdThreshold=-1);

    /**
     *  Return the maximum deviation of the solution from the exact transform
     *  on the current grid.
     *
     *  The deviations are in scaled intermediate world coordinates
     *  @f$\sqrt{\delta x_s^2 \delta y_s^2}@f$ for the forward transform and in
     *  pixels @f$(\delta u^2, \delta v^2)@f$ for the reverse transform
     *  (respectively).  Note that in the common case where the CD matrix
     *  includes the scaling from angle units to pixel units, the *scaled*
     *  intermediate world coordinate values are also in (nominal) pixel
     *  units.
     */
    std::pair<double, double> computeMaxDeviation() const noexcept;

private:
    struct Grid {

        // Set up the grid.
        Grid(lsst::geom::Extent2I const & shape_, SipApproximation const & parent);

        lsst::geom::Extent2I const shape;  //  number of grid points in each dimension
        std::vector<lsst::geom::Point2D> dpix1; //  [pixel coords] - CRPIX
        std::vector<lsst::geom::Point2D> siwc;  //  CD^{-1}([intermediate world coords])
        std::vector<lsst::geom::Point2D> dpix2; //  round-tripped version of dpix1 if useInverse, or exactly dpix1
    };

    struct Solution {
        static std::unique_ptr<Solution> fit(int order_, double svdThreshold, SipApproximation const &parent);

        Solution(lsst::geom::polynomials::PolynomialFunction2dYX const &a_,
                 lsst::geom::polynomials::PolynomialFunction2dYX const &b_,
                 lsst::geom::polynomials::PolynomialFunction2dYX const &ap_,
                 lsst::geom::polynomials::PolynomialFunction2dYX const &bp_);

        using Workspace = lsst::geom::polynomials::PolynomialFunction2dYX::Workspace;

        Workspace makeWorkspace() const;

        lsst::geom::Point2D applyForward(lsst::geom::Point2D const & dpix, Workspace & ws) const;

        lsst::geom::Point2D applyInverse(lsst::geom::Point2D const & siwc, Workspace & ws) const;

        lsst::geom::polynomials::PolynomialFunction2dYX a;
        lsst::geom::polynomials::PolynomialFunction2dYX b;
        lsst::geom::polynomials::PolynomialFunction2dYX ap;
        lsst::geom::polynomials::PolynomialFunction2dYX bp;
    };

    bool _useInverse;
    std::shared_ptr<TransformPoint2ToPoint2> _pixelToIwc;
    lsst::geom::Box2D _bbox;
    lsst::geom::Extent2D _crpix;
    lsst::geom::LinearTransform _cdInv;
    std::unique_ptr<Grid const> _grid;
    std::unique_ptr<Solution const> _solution;
};

}}}  // namespace lsst::afw::geom

#endif // !LSST_AFW_GEOM_SipApproximation_h_INCLUDED
