// -*- lsst-c++ -*-

/* 
 * LSST Data Management System
 * See the COPYRIGHT and LICENSE files in the top-level directory of this
 * package for notices and licensing terms.
 */

/**
 * \file 
 * @brief Class representing a 2D transform for which the pixel
 * distortions in the x- and y-directions are separable.
 */

#ifndef LSST_AFW_GEOM_SEPARABLEXYTRANSFORM_H
#define LSST_AFW_GEOM_SEPARABLEXYTRANSFORM_H

#include "lsst/afw/geom/XYTransform.h"

namespace lsst {
namespace afw {
namespace geom {

class Functor;

/// @brief A 2D transform for which the pixel distortions in the in
/// the x- and y-directions are separable.  The transformations in
/// each direction are implemented as separate instances of concrete
/// subclasses of the Functor base class.
class SeparableXYTransform : public XYTransform {

public:

   /// @param xfunctor Functor describing the transformation from
   ///        nominal pixels to actual pixels in the x-direction.
   /// @param yfunctor Functor describing the transformation from
   ///        nominal pixels to actual pixels in the y-direction.
   SeparableXYTransform(Functor const & xfunctor, Functor const & yfunctor);

   virtual ~SeparableXYTransform() {}
   
   virtual PTR(XYTransform) clone() const;

   /// @return The transformed Point2D in sensor coordinates in units
   ///         of pixels.
   /// @param point The Point2D location in sensor coordinates in
   ///        units of pixels.  This corresponds to the location on
   ///        the sensor in the absence of the pixel distortions.
   virtual Point2D forwardTransform(Point2D const & point) const;

   /// @return The un-transformed Point2D in sensor coordinates.
   /// @param point The Point2D location in sensor coordinates.  This
   ///        corresponds to the actual location of charge deposition,
   ///        i.e., with the pixel distortions applied.
   virtual Point2D reverseTransform(Point2D const & point) const;

   /// @return Const reference to the xfunctor.
   Functor const & getXfunctor() const;

   /// @return Const reference to the yfunctor.
   Functor const & getYfunctor() const;

private:

   PTR(Functor) _xfunctor;
   PTR(Functor) _yfunctor;

};

} // namespace geom
} // namespace af
} // namespace lsst

#endif // LSST_AFW_GEOM_SEPARABLEXYTRANSFORM_H
