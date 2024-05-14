# This file is part of afw.
#
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (https://www.lsst.org).
# See the COPYRIGHT file at the top-level directory of this distribution
# for details of code ownership.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

__all__ = ['assembleAmplifierImage', 'assembleAmplifierRawImage',
           'makeUpdatedDetector', 'AmplifierIsolator']

# dict of doFlip: slice
_SliceDict = {
    False: slice(None, None, 1),
    True: slice(None, None, -1),
}


def _insertPixelChunk(outView, inView, amplifier):
    # For the sake of simplicity and robustness, this code does not short-circuit the case flipX=flipY=False.
    # However, it would save a bit of time, including the cost of making numpy array views.
    # If short circuiting is wanted, do it here.

    xSlice = _SliceDict[amplifier.getRawFlipX()]
    ySlice = _SliceDict[amplifier.getRawFlipY()]
    if hasattr(inView, "image"):
        inArrList = (inView.image.array, inView.mask.array, inView.variance.array)
        outArrList = (outView.image.array, outView.mask.array, outView.variance.array)
    else:
        inArrList = [inView.array]
        outArrList = [outView.array]

    for inArr, outArr in zip(inArrList, outArrList):
        # y,x because numpy arrays are transposed w.r.t. afw Images
        outArr[:] = inArr[ySlice, xSlice]


def assembleAmplifierImage(destImage, rawImage, amplifier):
    """Assemble the amplifier region of an image from a raw image.

    Parameters
    ----------
    destImage : `lsst.afw.image.Image` or `lsst.afw.image.MaskedImage`
        Assembled image; the region amplifier.getBBox() is overwritten with
        the assembled amplifier image.
    rawImage : `lsst.afw.image.Image` or `lsst.afw.image.MaskedImage`
        Raw image (same type as destImage).
    amplifier : `lsst.afw.cameraGeom.Amplifier`
        Amplifier geometry, with raw amplifier info.

    Raises
    ------
    RuntimeError
        Raised if image types do not match or amplifier has no raw amplifier info.
    """
    if type(destImage.Factory) != type(rawImage.Factory):  # noqa: E721
        raise RuntimeError(f"destImage type = {type(destImage.Factory).__name__} != "
                           f"{type(rawImage.Factory).__name__} = rawImage type")
    inView = rawImage.Factory(rawImage, amplifier.getRawDataBBox())
    outView = destImage.Factory(destImage, amplifier.getBBox())

    _insertPixelChunk(outView, inView, amplifier)


def assembleAmplifierRawImage(destImage, rawImage, amplifier):
    """Assemble the amplifier region of a raw CCD image.

    For most cameras this is a no-op: the raw image already is an assembled
    CCD image.
    However, it is useful for camera such as LSST for which each amplifier
    image is a separate image.

    Parameters
    ----------
    destImage : `lsst.afw.image.Image` or `lsst.afw.image.MaskedImage`
        CCD Image; the region amplifier.getRawAmplifier().getBBox()
        is overwritten with the raw amplifier image.
    rawImage : `lsst.afw.image.Image` or `lsst.afw.image.MaskedImage`
        Raw image (same type as destImage).
    amplifier : `lsst.afw.cameraGeom.Amplifier`
        Amplifier geometry with raw amplifier info

    Raises
    ------
    RuntimeError
        Raised if image types do not match or amplifier has no raw amplifier info.
    """
    if type(destImage.Factory) != type(rawImage.Factory):  # noqa: E721
        raise RuntimeError(f"destImage type = {type(destImage.Factory).__name__} != "
                           f"{type(rawImage.Factory).__name__} = rawImage type")
    inBBox = amplifier.getRawBBox()
    inView = rawImage.Factory(rawImage, inBBox)
    outBBox = amplifier.getRawBBox()
    outBBox.shift(amplifier.getRawXYOffset())
    outView = destImage.Factory(destImage, outBBox)

    _insertPixelChunk(outView, inView, amplifier)


def makeUpdatedDetector(ccd):
    """Return a Detector that has had the definitions of amplifier geometry
    updated post assembly.

    Parameters
    ----------
    ccd : `lsst.afw.image.Detector`
        The detector to copy and update.
    """
    builder = ccd.rebuild()
    for amp in builder.getAmplifiers():
        amp.transform()
    return builder.finish()


class AmplifierIsolator:
    """A class that can extracts single-amplifier subimages from trimmed or
    untrimmed assembled images and transforms them to a particular orientation
    and offset.

    Callers who have a in-memory assembled `lsst.afw.image.Exposure` should
    generally just use the `apply` class method.  Other methods can be used to
    implement subimage loads of on on-disk images (e.g. formatter classes in
    ``obs_base``) or obtain subsets from other image classes.

    Parameters
    ----------
    amplifier : `Amplifier`
        Amplifier object that identifies the amplifier to load and sets the
        orientation and offset of the returned subimage.
    parent_bbox : `lsst.geom.Box2I`
        Bounding box of the assembled parent image.  This must be equal to
        either ``parent_detector.getBBox()`` or
        ``parent_detector.getRawBBox()``; which one is used to determine
        whether the parent image (and hence the amplifier subimages) is
        trimmed.
    parent_detector : `Detector`
        Detector object that describes the parent image.
    """

    def __init__(self, amplifier, parent_bbox, parent_detector):
        self._amplifier = amplifier
        self._parent_detector = parent_detector
        self._parent_amplifier = self._parent_detector[self._amplifier.getName()]
        self._is_parent_trimmed = (parent_bbox == self._parent_detector.getBBox())
        self._amplifier_comparison = self._amplifier.compareGeometry(self._parent_amplifier)
        if self._is_parent_trimmed:
            # We only care about the final bounding box; don't check e.g.
            # overscan regions for consistency.
            if self._parent_amplifier.getBBox() != self._amplifier.getBBox():
                raise ValueError(
                    f"The given amplifier's trimmed bounding box ({self._amplifier.getBBox()}) is not the "
                    "same as the trimmed bounding box of the same amplifier in the parent image "
                    f"({self._parent_amplifier.getBBox()})."
                )
        else:
            # Parent is untrimmed, so we need all regions to be consistent
            # between the amplifiers modulo flips and offsets.
            if self._amplifier_comparison & self._amplifier_comparison.REGIONS_DIFFER:
                raise ValueError(
                    "The given amplifier's subregions are fundamentally incompatible with those of the "
                    "parent image's amplifier."
                )

    @property
    def subimage_bbox(self):
        """The bounding box of the target amplifier in the parent image
        (`lsst.geom.Box2I`).
        """
        if self._is_parent_trimmed:
            return self._parent_amplifier.getBBox()
        else:
            return self._parent_amplifier.getRawBBox()

    def transform_subimage(self, subimage):
        """Transform an already-extracted subimage to match the orientation
        and offset of the target amplifier.

        Parameters
        ----------
        subimage : image-like
            The subimage to transform; may be any of `lsst.afw.image.Image`,
            `lsst.afw.image.Mask`, `lsst.afw.image.MaskedImage`, and
            `lsst.afw.image.Exposure`.

        Returns
        -------
        transformed : image-like
            Transformed image of the same type as ``subimage``.
        """
        from lsst.afw.math import flipImage
        if hasattr(subimage, "getMaskedImage"):
            # flipImage doesn't support Exposure natively.
            # And sadly, there's no way to write to an existing MaskedImage,
            # so we need to make yet another copy.
            result = subimage.clone()
            result.setMaskedImage(
                flipImage(
                    subimage.getMaskedImage(),
                    bool(self._amplifier_comparison & self._amplifier_comparison.FLIPPED_X),
                    bool(self._amplifier_comparison & self._amplifier_comparison.FLIPPED_Y),
                )
            )
        else:
            result = flipImage(
                subimage,
                bool(self._amplifier_comparison & self._amplifier_comparison.FLIPPED_X),
                bool(self._amplifier_comparison & self._amplifier_comparison.FLIPPED_Y),
            )
        if self._is_parent_trimmed:
            result.setXY0(self._amplifier.getBBox().getMin())
        else:
            result.setXY0(self._amplifier.getRawBBox().getMin() + self._amplifier.getRawXYOffset())
        return result

    def make_detector(self):
        """Create a single-amplifier detector that describes the transformed
        subimage.

        Returns
        -------
        detector : `Detector`
            Detector object with a single amplifier, a trimmed bounding box
            equal to the amplifier's trimmed bounding box, and no crosstalk.
        """
        detector = self._parent_detector.rebuild()
        detector.clear()
        detector.append(self._amplifier.rebuild())
        detector.setBBox(self._amplifier.getBBox())
        detector.unsetCrosstalk()
        return detector.finish()

    @classmethod
    def apply(cls, parent_exposure, amplifier):
        """Obtain a single-amplifier `lsst.afw.image.Exposure` subimage that
        masquerades as full-detector image for a single-amp detector.

        Parameters
        ----------
        parent_exposure : `lsst.afw.image.Exposure`
            Parent image to obtain a subset from.
            `~lsst.afw.image.Exposure.getDetector` must not return `None`.
        amplifier : `Amplifier`
            Target amplifier for the subimage.  May differ from the amplifier
            obtained by ``parent_exposure.getDetector()[amplifier.getName()]``
            only by flips and differences in `~Amplifier.getRawXYOffset`.

        Returns
        -------
        subimage : `lsst.afw.image.Exposure`
            Exposure subimage for the target amplifier, with the
            orientation and XY0 described by that amplifier, and a single-amp
            detector holding a copy of that amplifier.

        Notes
        -----
        Because we use the target amplifier's bounding box as the bounding box
        of the detector attached to the returned exposure, other exposure
        components that are passed through unmodified (e.g. the WCS) should
        still be valid for the single-amp exposure after it is trimmed and
        "assembled".  Unlike most trimmed+assembled images, however, it will
        have a nonzero XY0, and code that (incorrectly!) does not pay attention
        to XY0 may break.
        """
        instance = cls(amplifier, parent_bbox=parent_exposure.getBBox(),
                       parent_detector=parent_exposure.getDetector())
        result = instance.transform_subimage(parent_exposure[instance.subimage_bbox])
        result.setDetector(instance.make_detector())
        return result
