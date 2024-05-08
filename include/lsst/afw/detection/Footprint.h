/*
 * LSST Data Management System
 * Copyright 2008-2016  AURA/LSST.
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
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */
#if !defined(LSST_DETECTION_FOOTPRINT_H)
#define LSST_DETECTION_FOOTPRINT_H

#include <algorithm>
#include <list>
#include <set>
#include <cmath>
#include <cstdint>
#include <memory>
#include "ndarray.h"
#include "lsst/base.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/detection/Peak.h"
#include "lsst/afw/geom/Span.h"
#include "lsst/afw/geom/SpanSet.h"
#include "lsst/geom/Box.h"
#include "lsst/afw/geom/ellipses.h"
#include "lsst/geom/LinearTransform.h"
#include "lsst/afw/geom/SkyWcs.h"
#include "lsst/afw/geom/Transform.h"
#include "lsst/afw/table/fwd.h"
#include "lsst/afw/table/Schema.h"
#include "lsst/afw/table/Key.h"
#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/table/io/InputArchive.h"

namespace lsst {
namespace afw {
namespace detection {

/**
 * Class to describe the properties of a detected object from an image
 *
 * A Footprint is designed to be constructed with information about a detected
 * object in an image. Internally a Footprint holds a SpanSet which is used to
 * describe the location of the object in the image (the x, y pixel locations
 * which are considered part of the object). In addition a Footprint contains
 * a PeakCatalog which is used to store the location and intensity of peaks in
 * the detection.
 */
class Footprint : public afw::table::io::Persistable {
DECLARE_PERSISTABLE_FACADE(Footprint);
public:
    /*
     * As a note there is no constructor which accepts a unique_ptr to a SpanSet.
     * If someone did have a unique_ptr to a SpanSet it is possible to wrap the
     * unique_ptr in a std::move when calling the constructor of a Footprint and
     * the compiler will implicitly turn the unique_ptr into a shared_ptr and use
     * that to construct the Footprint.
     */

    /** @brief Constructor for Footprint object
     *
     * @param inputSpans Shared pointer to a SpanSet defining the pixels included in
                         the Footprint.
     * @param region Bounding box of the image in which the Footprint was created,
                     defaults to empty box.
     */
    explicit Footprint(std::shared_ptr<geom::SpanSet> inputSpans,
                       lsst::geom::Box2I const &region = lsst::geom::Box2I());

    /** @brief Constructor for the Footprint object
     *
     * @param inputSpans Shared pointer to a SpanSet defining the pixels included in
                         the Footprint.
     * @param peakSchema schema to be used in the PeakCatalog
     * @param region Bounding box of the image in which the Footprint was created,
                     defaults to empty box.
     */
    explicit Footprint(std::shared_ptr<geom::SpanSet> inputSpans, afw::table::Schema const &peakSchema,
                       lsst::geom::Box2I const &region = lsst::geom::Box2I());

    /** @brief Constructor of a empty Footprint object
     */
    explicit Footprint()
            : _spans(std::make_shared<geom::SpanSet>()),
              _peaks(PeakTable::makeMinimalSchema()),
              _region(lsst::geom::Box2I()) {}

    Footprint(Footprint const &other) = default;
    Footprint(Footprint &&) = default;

    Footprint &operator=(Footprint const &other) = default;
    Footprint &operator=(Footprint &&) = default;

    ~Footprint() override = default;

    /** Indicates if this object is a HeavyFootprint
     */
    virtual bool isHeavy() const { return false; }

    /** Return a shared pointer to the SpanSet
     */
    std::shared_ptr<geom::SpanSet> getSpans() const { return _spans; }

    /** Sets the shared pointer to the SpanSet in the Footprint
     *
     * @param otherSpanSet Shared pointer to a SpanSet
     */
    void setSpans(std::shared_ptr<geom::SpanSet> otherSpanSet);

    /**
     * Return the Peaks contained in this Footprint
     *
     * The peaks should be ordered by decreasing pixel intensity at the peak position (so the most negative
     * peak appears last).  Users that add new Peaks manually are responsible for maintaining this sorting.
     */
    PeakCatalog &getPeaks() { return _peaks; }
    const PeakCatalog &getPeaks() const { return _peaks; }

    /** Convenience function to add a peak
     *
     * @param fx Float containing the x position of a peak
     * @param fy Float containing the y position of a peak
     * @param value The intensity value of the peak
     */
    std::shared_ptr<PeakRecord> addPeak(float fx, float fy, float value);

    /**
     * Sort Peaks from most positive value to most negative.
     *
     * If the key passed is invalid (the default) PeakTable::getPeakValueKey() will be used.
     *
     * @param key A key corresponding to the field in the Schema the PeakCatalog is to be
     *            sorted by.
     */
    void sortPeaks(afw::table::Key<float> const &key = afw::table::Key<float>());

    /** Set the Schema used by the PeakCatalog (will throw if PeakCatalog is not empty).
     *
     * @param peakSchema The schema to use in the PeakCatalog
     *
     * @throws pex::exceptions::LogicError Thrown if if the PeakCatalog is not empty
     */
    void setPeakSchema(afw::table::Schema const &peakSchema);

    /** Set the peakCatalog to a copy of the supplied catalog
     *
     * PeakCatalog will be copied into the Footprint, but a PeakCatalog is a shallow
     * copy, so records will not be duplicated. This function will throw an error if
     * the PeakCatalog of *this is not empty.
     *
     * @param otherPeaks The PeakCatalog to copy
     */
    void setPeakCatalog(PeakCatalog const &otherPeaks);

    /**
     * Return the number of pixels in this Footprint
     *
     * This function returns the real number of pixels, not the area of the bbox.
     */
    std::size_t getArea() const { return _spans->getArea(); }

    /**
     * Return the Footprint's centroid
     *
     * The centroid is calculated as the mean of the pixel centers
     */
    lsst::geom::Point2D getCentroid() const { return _spans->computeCentroid(); }

    /**
     * Return the Footprint's shape (interpreted as an ellipse)
     *
     * The shape is determined by measuring the moments of the pixel
     * centers about its centroid (cf. getCentroid)
     */
    geom::ellipses::Quadrupole getShape() const { return _spans->computeShape(); }

    /**
     * Shift a Footprint by `(dx, dy)`
     *
     * @param dx How much to move Footprint in column direction
     * @param dy How much to move in row direction
     */
    void shift(int dx, int dy);

    /**
     * Shift a Footprint by a given extent
     *
     * @param d ExtentI object which gives the dimensions the Footprint should be shifted
     */
    void shift(lsst::geom::ExtentI const &d) { shift(d.getX(), d.getY()); }

    /**
     * Return the Footprint's bounding box
     */
    lsst::geom::Box2I getBBox() const { return _spans->getBBox(); }

    /**
     * Return the corners of the MaskedImage the footprints live in
     */
    lsst::geom::Box2I getRegion() const { return _region; }

    /**
     * Set the corners of the MaskedImage wherein the footprints dwell
     *
     * @param region A box describing the corners of the Image the Footprint derives from
     */
    void setRegion(lsst::geom::Box2I const &region) { _region = region; }

    /**
     * Clip the Footprint such that all values lie inside the supplied Bounding Box
     *
     * @param bbox Integer box object that defines the boundaries the footprint should be
     *             clipped to.
     */
    void clipTo(lsst::geom::Box2I const &bbox);

    /**
     * Tests if a pixel postion falls inside the Footprint
     *
     * @param pix Integer point object defining the position of a pixel to test
     */
    bool contains(lsst::geom::Point2I const &pix) const;

    /**
     *  Transform the footprint from one WCS to another
     *
     *  @param source Wcs that defines the coordinate system of the input footprint.
     *  @param target Wcs that defines that desired coordinate system of the returned footprint.
     *  @param region Used to set the "region" box of the returned footprint; note that this is
     *                NOT the same as the footprint's bounding box.
     *  @param doClip If true, clip the new footprint to the region bbox before returning it.
     */
    std::shared_ptr<Footprint> transform(std::shared_ptr<geom::SkyWcs> source,
                                         std::shared_ptr<geom::SkyWcs> target,
                                         lsst::geom::Box2I const &region, bool doClip = true) const;

    /** Return a new Footprint whose pixels are the product of applying the specified transformation
     *
     * @param t A linear transform object which will be used to map the pixels
     * @param region Used to set the "region" box of the returned footprint; note that this is
     *               NOT the same as the footprint's bounding box.
     * @param doClip If true, clip the new footprint to the region bbox before returning it.
     */
    std::shared_ptr<Footprint> transform(lsst::geom::LinearTransform const &t,
                                         lsst::geom::Box2I const &region, bool doClip = true) const;

    /** Return a new Footprint whose pixels are the product of applying the specified transformation
     *
     * @param t An affine transform object which will be used to map the pixels
     * @param region Used to set the "region" box of the returned footprint; note that this is
     *               NOT the same as the footprint's bounding box.
     * @param doClip If true, clip the new footprint to the region bbox before returning it.
     */
    std::shared_ptr<Footprint> transform(lsst::geom::AffineTransform const &t,
                                         lsst::geom::Box2I const &region, bool doClip = true) const;

    /** Return a new Footprint whose pixels are the product of applying the specified transformation
     *
     * @param t A 2-D transform which will be used to map the pixels
     * @param region Used to set the "region" box of the returned footprint; note that this is
     *               NOT the same as the footprint's bounding box.
     * @param doClip If true, clip the new footprint to the region bbox before returning it.
     */
    std::shared_ptr<Footprint> transform(geom::TransformPoint2ToPoint2 const &t,
                                         lsst::geom::Box2I const &region, bool doClip = true) const;

    /**
     * Report if this object is persistable
     */
    bool isPersistable() const noexcept override { return true; }

    /**
     * Dilate the Footprint with a defined kernel
     *
     * This function enlarges the SpanSet which defines the area of the Footprint by
     * an amount governed by in input kernel
     *
     * @param r The radius of the stencil object used to create a dilation kernel
     * @param s The stencil object used to create the dilation kernel
     */
    void dilate(int r, geom::Stencil s = geom::Stencil::CIRCLE);

    /**
     * Dilate the Footprint with a defined kernel
     *
     * This function enlarges the SpanSet which defines the area of the Footprint by
     * an amount governed by the input kernel
     *
     * @param other SpanSet to use as the kernel in dilation
     */
    void dilate(geom::SpanSet const &other);

    /**
     * Erode the Footprint with a defined kernel
     *
     * This function reduces the size of the SpanSet which defines the area of the Footprint
     * by an amount governed by the input kernel
     *
     * @param r The radius of the stencil object used to create a erosion kernel
     * @param s The stencil object used to create the erosion kernel
     */
    void erode(int r, geom::Stencil s = geom::Stencil::CIRCLE);

    /**
     * Erode the Footprint with a defined kernel
     *
     * This function reduces the size of the SpanSet which defines the area of the Footprint
     * by an amount governed by the input kernel
     *
     * @param other SpanSet to use as the kernel in erosion
     */
    void erode(geom::SpanSet const &other);

    /**
     * Remove peaks from the PeakCatalog that fall outside the area of the Footprint
     */
    void removeOrphanPeaks();

    /**
     * Compute and update the significance of each peak, given a single value for sigma.
     *
     * The Footprint peak schema must already have a `significance` field in it.
     *
     * @param sigma The standard deviation for peak detection, positive or negative as appropriate.
     *
     * @throws pex::exceptions::NotFoundError Thrown if the the peaks do not have a `significance` field.
     */
    void updatePeakSignificance(double sigma);

    /**
     * Compute and update the significance of each peak, using the variance image.
     *
     * The Footprint peak schema must already have a `significance` field in it.
     *
     * @param variance The variance plane of the image that this footprint is in.
     * @param polarity +1 if this is detected as a positive footprint, -1 if negative.
     *
     * @throws pex::exceptions::NotFoundError Thrown if the the peaks do not have a significance field.
     */
    void updatePeakSignificance(image::Image<float> const &variance, int polarity);

    /**
     * Reports if the Footprint is simply connected or has multiple components
     */
    bool isContiguous() const { return getSpans()->isContiguous(); };

    /**
     * Split a multi-component Footprint into a vector of contiguous Footprints
     *
     * Split a multi-component Footprint such that each Footprint in the output vector
     * is contiguous and contains only peaks that can be found within the bounds of the
     * Footprint
     */
    std::vector<std::shared_ptr<Footprint>> split() const;

    /**
     * equality operator
     *
     * @param other The Footprint for which equality will be computed
     */
    bool operator==(Footprint const &other) const;

protected:
    /**
     * Return the name correspoinging ot the persistence type
     */
    std::string getPersistenceName() const override;

    /**
     * Return the python module the object will live in
     */
    inline std::string getPythonModule() const override { return "lsst.afw.detection"; }

    /**
     * Write an instance of a Footprint to an output Archive
     */
    void write(OutputArchiveHandle &handle) const override;

    friend class FootprintFactory;

    /**
     * Static method used to unpersist the SpanSet member of the Footprint class
     */
    static std::unique_ptr<Footprint> readSpanSet(afw::table::BaseCatalog const &,
                                                  afw::table::io::InputArchive const &);
    /**
     * Static method used to unpersist the PeakCatalog member of the Footprint class
     */
    static void readPeaks(afw::table::BaseCatalog const &, Footprint &);

private:
    friend class FootprintMerge;

    std::shared_ptr<geom::SpanSet> _spans;  ///< @internal The SpanSet representing area on image
    PeakCatalog _peaks;                     ///< @internal The peaks lying in this footprint
    lsst::geom::Box2I _region;  ///< @internal The corners of the MaskedImage the footprints live in
};

/**
 * Print a Footprint to the stream.
 *
 * @param os Stream to print to.
 * @param rhs Footprint to print.
 */
std::ostream &operator<<(std::ostream &os, Footprint const &rhs);

/**
 * @brief Merges two Footprints -- appends their peaks, and unions their
 * spans, returning a new Footprint. Region is not preserved, and is set to an empty
 * lsst::geom::Box2I object.
 */
std::shared_ptr<Footprint> mergeFootprints(Footprint const &footprint1, Footprint const &footprint2);

/**
 * @brief Return a list of BBox%s, whose union contains exactly the pixels in
 * the footprint, neither more nor less
 *
 * Useful in generating sets of meas::algorithms::Defects for the ISR
 *
 * @param footprint Footprint to turn into bounding box list
 */
std::vector<lsst::geom::Box2I> footprintToBBoxList(Footprint const &footprint);
}  // namespace detection
}  // namespace afw
}  // namespace lsst

#endif  // LSST_DETECTION_FOOTPRINT_H
