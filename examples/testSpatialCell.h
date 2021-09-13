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

#if !defined(TESTSPATIALCELL_H)
#define TESTSPATIALCELL_H
#include <memory>
#include "lsst/geom.h"
#include "lsst/afw/math.h"
#include "lsst/afw/image/Image.h"

/*
 * Test class for SpatialCellImageCandidate
 */
class ExampleCandidate : public lsst::afw::math::SpatialCellImageCandidate {
public:
    using PixelT = float;
    using MaskedImageT = lsst::afw::image::MaskedImage<PixelT>;

    /** Constructor
     *
     * @param xCenter The object's column-centre
     * @param yCenter The object's row-centre
     * @param parent the parent image
     * @param bbox The object's bounding box
     */
    ExampleCandidate(float const xCenter, float const yCenter, std::shared_ptr<MaskedImageT const> parent,
                     lsst::geom::Box2I bbox);

    lsst::geom::Box2I getBBox() const { return _bbox; }

    /// Return candidates rating
    double getCandidateRating() const;

    /// Return the %image
    std::shared_ptr<MaskedImageT const> getMaskedImage() const;

private:
    mutable std::shared_ptr<MaskedImageT> _image;
    std::shared_ptr<MaskedImageT const> _parent;
    lsst::geom::Box2I _bbox;
};

/*
 * Class to pass around to all our ExampleCandidates.  All this one does is count acceptable candidates
 */
class ExampleCandidateVisitor : public lsst::afw::math::CandidateVisitor {
public:
    ExampleCandidateVisitor() : lsst::afw::math::CandidateVisitor(), _n(0), _npix(0) {}

    // Called by SpatialCellSet::visitCandidates before visiting any Candidates
    void reset() { _n = _npix = 0; }

    // Called by SpatialCellSet::visitCandidates for each Candidate
    void processCandidate(lsst::afw::math::SpatialCellCandidate *candidate) {
        ++_n;

        lsst::geom::Box2I box = dynamic_cast<ExampleCandidate *>(candidate)->getBBox();
        _npix += box.getArea();
    }

    int getN() const { return _n; }
    int getNPix() const { return _npix; }

private:
    int _n;     // number of ExampleCandidates
    int _npix;  // number of pixels in ExampleCandidates's bounding boxes
};

#endif
