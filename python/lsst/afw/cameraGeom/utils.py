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

"""
Support for displaying cameraGeom objects.
"""

__all__ = ['prepareWcsData', 'plotFocalPlane', 'makeImageFromAmp', 'calcRawCcdBBox', 'makeImageFromCcd',
           'FakeImageDataSource', 'ButlerImage', 'rawCallback', 'overlayCcdBoxes',
           'showAmp', 'showCcd', 'getCcdInCamBBoxList', 'getCameraImageBBox',
           'makeImageFromCamera', 'showCamera', 'makeFocalPlaneWcs', 'findAmp']

import logging
import math
import numpy

import lsst.geom
from lsst.afw.fits import FitsError
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.cameraGeom as afwCameraGeom
import lsst.daf.base as dafBase
import lsst.pex.exceptions as pexExceptions

from ._rotateBBoxBy90 import rotateBBoxBy90
from ._assembleImage import assembleAmplifierImage, assembleAmplifierRawImage
from ._cameraGeom import FIELD_ANGLE, FOCAL_PLANE
from lsst.afw.display.utils import _getDisplayFromDisplayOrFrame
from lsst.afw.cameraGeom import DetectorType

import lsst.afw.display as afwDisplay
import lsst.afw.display.utils as displayUtils

_LOG = logging.getLogger(__name__)


def prepareWcsData(wcs, amp, isTrimmed=True):
    """Put Wcs from an Amp image into CCD coordinates

    Parameters
    ----------
    wcs : `lsst.afw.geom.SkyWcs`
        The WCS object to start from.
    amp : `lsst.afw.table.AmpInfoRecord`
        Amp object to use
    isTrimmed : `bool`
        Is the image to which the WCS refers trimmed of non-imaging pixels?

    Returns
    -------
    ampWcs : `lsst.afw.geom.SkyWcs`
        The modified WCS.
    """
    if isTrimmed:
        ampBox = amp.getRawDataBBox()
    else:
        ampBox = amp.getRawBBox()
    ampCenter = lsst.geom.Point2D(ampBox.getDimensions()/2.0)
    wcs = afwGeom.makeFlippedWcs(wcs, amp.getRawFlipX(), amp.getRawFlipY(), ampCenter)
    # Shift WCS for trimming
    if isTrimmed:
        trim_shift = ampBox.getMin() - amp.getBBox().getMin()
        wcs = wcs.copyAtShiftedPixelOrigin(lsst.geom.Extent2D(-trim_shift.getX(), -trim_shift.getY()))
    # Account for shift of amp data in larger ccd matrix
    offset = amp.getRawXYOffset()
    return wcs.copyAtShiftedPixelOrigin(lsst.geom.Extent2D(offset))


def plotFocalPlane(camera, fieldSizeDeg_x=0, fieldSizeDeg_y=None, dx=0.1, dy=0.1, figsize=(10., 10.),
                   useIds=False, showFig=True, savePath=None):
    """Make a plot of the focal plane along with a set points that sample
    the field of view.

    Parameters
    ----------
    camera : `lsst.afw.cameraGeom.Camera`
        A camera object
    fieldSizeDeg_x : `float`
        Amount of the field to sample in x in degrees
    fieldSizeDeg_y : `float` or `None`
        Amount of the field to sample in y in degrees
    dx : `float`
        Spacing of sample points in x in degrees
    dy : `float`
        Spacing of sample points in y in degrees
    figsize : `tuple` containing two `float`
        Matplotlib style tuple indicating the size of the figure in inches
    useIds : `bool`
        Label detectors by name, not id?
    showFig : `bool`
        Display the figure on the screen?
    savePath : `str` or `None`
        If not `None`, save a copy of the figure to this name.
    """
    try:
        from matplotlib.patches import Polygon
        from matplotlib.collections import PatchCollection
        import matplotlib.pyplot as plt
    except ImportError:
        raise ImportError(
            "Can't run plotFocalPlane: matplotlib has not been set up")

    if fieldSizeDeg_x:
        if fieldSizeDeg_y is None:
            fieldSizeDeg_y = fieldSizeDeg_x

        field_gridx, field_gridy = numpy.meshgrid(
            numpy.arange(0., fieldSizeDeg_x + dx, dx) - fieldSizeDeg_x/2.,
            numpy.arange(0., fieldSizeDeg_y + dy, dy) - fieldSizeDeg_y/2.)
        field_gridx, field_gridy = field_gridx.flatten(), field_gridy.flatten()
    else:
        field_gridx, field_gridy = [], []

    xs = []
    ys = []
    pcolors = []

    # compute focal plane positions corresponding to field angles field_gridx, field_gridy
    posFieldAngleList = [lsst.geom.Point2D(x*lsst.geom.radians, y*lsst.geom.radians)
                         for x, y in zip(field_gridx, field_gridy)]
    posFocalPlaneList = camera.transform(posFieldAngleList, FIELD_ANGLE, FOCAL_PLANE)
    for posFocalPlane in posFocalPlaneList:
        xs.append(posFocalPlane.getX())
        ys.append(posFocalPlane.getY())
        dets = camera.findDetectors(posFocalPlane, FOCAL_PLANE)
        if len(dets) > 0:
            pcolors.append('w')
        else:
            pcolors.append('k')

    colorMap = {DetectorType.SCIENCE: 'b', DetectorType.FOCUS: 'y',
                DetectorType.GUIDER: 'g', DetectorType.WAVEFRONT: 'r'}

    patches = []
    colors = []
    plt.figure(figsize=figsize)
    ax = plt.gca()
    xvals = []
    yvals = []
    for det in camera:
        corners = [(c.getX(), c.getY()) for c in det.getCorners(FOCAL_PLANE)]
        for corner in corners:
            xvals.append(corner[0])
            yvals.append(corner[1])
        colors.append(colorMap[det.getType()])
        patches.append(Polygon(corners, True))
        center = det.getOrientation().getFpPosition()
        ax.text(center.getX(), center.getY(), det.getId() if useIds else det.getName(),
                horizontalalignment='center', size=6)

    patchCollection = PatchCollection(patches, alpha=0.6, facecolor=colors)
    ax.add_collection(patchCollection)
    ax.scatter(xs, ys, s=10, alpha=.7, linewidths=0., c=pcolors)
    ax.set_xlim(min(xvals) - abs(0.1*min(xvals)),
                max(xvals) + abs(0.1*max(xvals)))
    ax.set_ylim(min(yvals) - abs(0.1*min(yvals)),
                max(yvals) + abs(0.1*max(yvals)))
    ax.set_xlabel('Focal Plane X (mm)')
    ax.set_ylabel('Focal Plane Y (mm)')
    if savePath is not None:
        plt.savefig(savePath)
    if showFig:
        plt.show()


def makeImageFromAmp(amp, imValue=None, imageFactory=afwImage.ImageU, markSize=10, markValue=0,
                     scaleGain=lambda gain: (gain*1000)//10):
    """Make an image from an amp object.

    Since images are integer images by default, the gain needs to be scaled to
    give enough dynamic range to see variation from amp to amp.
    The scaling algorithm is assignable.

    Parameters
    ----------
    amp : `lsst.afw.table.AmpInfoRecord`
        Amp record to use for constructing the raw amp image.
    imValue : `float` or `None`
        Value to assign to the constructed image, or scaleGain(gain) if `None`.
    imageFactory : callable like `lsst.afw.image.Image`
        Type of image to construct.
    markSize : `float`
        Size of mark at read corner in pixels.
    markValue : `float`
        Value of pixels in the read corner mark.
    scaleGain : callable
        The function by which to scale the gain (must take a single argument).

    Returns
    -------
    ampImage : `lsst.afw.image`
        An untrimmed amp image, of the type produced by ``imageFactory``.
    """
    bbox = amp.getRawBBox()
    dbbox = amp.getRawDataBBox()
    img = imageFactory(bbox)
    if imValue is None:
        img.set(int(scaleGain(amp.getGain())))
    else:
        img.set(imValue)
    # Set the first pixel read to a different value
    markbbox = lsst.geom.Box2I()
    if amp.getReadoutCorner() == afwCameraGeom.ReadoutCorner.LL:
        markbbox.include(dbbox.getMin())
        markbbox.include(dbbox.getMin() + lsst.geom.Extent2I(markSize, markSize))
    elif amp.getReadoutCorner() == afwCameraGeom.ReadoutCorner.LR:
        cornerPoint = lsst.geom.Point2I(dbbox.getMaxX(), dbbox.getMinY())
        markbbox.include(cornerPoint)
        markbbox.include(cornerPoint + lsst.geom.Extent2I(-markSize, markSize))
    elif amp.getReadoutCorner() == afwCameraGeom.ReadoutCorner.UR:
        cornerPoint = lsst.geom.Point2I(dbbox.getMax())
        markbbox.include(cornerPoint)
        markbbox.include(cornerPoint + lsst.geom.Extent2I(-markSize, -markSize))
    elif amp.getReadoutCorner() == afwCameraGeom.ReadoutCorner.UL:
        cornerPoint = lsst.geom.Point2I(dbbox.getMinX(), dbbox.getMaxY())
        markbbox.include(cornerPoint)
        markbbox.include(cornerPoint + lsst.geom.Extent2I(markSize, -markSize))
    else:
        raise RuntimeError("Could not set readout corner")
    mimg = imageFactory(img, markbbox)
    mimg.set(markValue)
    return img


def calcRawCcdBBox(ccd):
    """Calculate the raw ccd bounding box.

    Parameters
    ----------
    ccd : `lsst.afw.cameraGeom.Detector`
        Detector for which to calculate the un-trimmed bounding box.

    Returns
    -------
    bbox : `lsst.geom.Box2I` or `None`
        Bounding box of the un-trimmed Detector, or `None` if there is not enough
        information to calculate raw BBox.
    """
    bbox = lsst.geom.Box2I()
    for amp in ccd:
        tbbox = amp.getRawBBox()
        tbbox.shift(amp.getRawXYOffset())
        bbox.include(tbbox)
    return bbox


def makeImageFromCcd(ccd, isTrimmed=True, showAmpGain=True, imageFactory=afwImage.ImageU, rcMarkSize=10,
                     binSize=1):
    """Make an Image of a CCD.

    Parameters
    ----------
    ccd : `lsst.afw.cameraGeom.Detector`
        Detector to use in making the image.
    isTrimmed : `bool`
        Assemble a trimmed Detector image.
    showAmpGain : `bool`
        Use the per-amp gain to color the pixels in the image?
    imageFactory : callable like `lsst.afw.image.Image`
        Image type to generate.
    rcMarkSize : `float`
        Size of the mark to make in the amp images at the read corner.
    binSize : `int`
        Bin the image by this factor in both dimensions.

    Returns
    -------
    image : `lsst.afw.image.Image`
        Image of the Detector (type returned by ``imageFactory``).
    """
    ampImages = []
    index = 0
    if isTrimmed:
        bbox = ccd.getBBox()
    else:
        bbox = calcRawCcdBBox(ccd)
    for amp in ccd:
        if showAmpGain:
            ampImages.append(makeImageFromAmp(
                amp, imageFactory=imageFactory, markSize=rcMarkSize))
        else:
            ampImages.append(makeImageFromAmp(amp, imValue=(index + 1)*1000,
                                              imageFactory=imageFactory, markSize=rcMarkSize))
        index += 1

    if len(ampImages) > 0:
        ccdImage = imageFactory(bbox)
        for ampImage, amp in zip(ampImages, ccd):
            if isTrimmed:
                assembleAmplifierImage(ccdImage, ampImage, amp)
            else:
                assembleAmplifierRawImage(ccdImage, ampImage, amp)
    else:
        if not isTrimmed:
            raise RuntimeError(
                "Cannot create untrimmed CCD without amps with raw information")
        ccdImage = imageFactory(ccd.getBBox())
    ccdImage = afwMath.binImage(ccdImage, binSize)
    return ccdImage


class FakeImageDataSource:
    """A class to retrieve synthetic images for display by the show* methods

    Parameters
    ----------
    isTrimmed : `bool`
        Should amps be trimmed?
    verbose : `bool`
        Be chatty?
    background : `float`
        The value of any pixels that lie outside the CCDs.
    showAmpGain : `bool`
        Color the amp segments with the gain of the amp?
    markSize : `float`
        Size of the side of the box used to mark the read corner.
    markValue : `float`
        Value to assign the read corner mark.
    ampImValue : `float` or `None`
        Value to assign to amps; scaleGain(gain) is used if `None`.
    scaleGain : callable
        Function to scale the gain by.
    """
    def __init__(self, isTrimmed=True, verbose=False, background=numpy.nan,
                 showAmpGain=True, markSize=10, markValue=0,
                 ampImValue=None, scaleGain=lambda gain: (gain*1000)//10):
        self.isTrimmed = isTrimmed
        self.verbose = verbose
        self.background = background
        self.showAmpGain = showAmpGain
        self.markSize = markSize
        self.markValue = markValue
        self.ampImValue = ampImValue
        self.scaleGain = scaleGain

    def getCcdImage(self, det, imageFactory, binSize):
        """Return a CCD image for the detector and the (possibly updated) Detector.

        Parameters
        ----------
        det : `lsst.afw.cameraGeom.Detector`
            Detector to use for making the image.
        imageFactory : callable like `lsst.afw.image.Image`
            Image constructor for making the image.
        binSize : `int`
            Bin the image by this factor in both dimensions.

        Returns
        -------
        ccdImage : `lsst.afw.image.Image`
            The constructed image.
        """
        ccdImage = makeImageFromCcd(det, isTrimmed=self.isTrimmed, showAmpGain=self.showAmpGain,
                                    imageFactory=imageFactory, binSize=binSize)
        return afwMath.rotateImageBy90(ccdImage, det.getOrientation().getNQuarter()), det

    def getAmpImage(self, amp, imageFactory):
        """Return an amp segment image.

        Parameters
        ----------
        amp : `lsst.afw.table.AmpInfoTable`
            AmpInfoTable for this amp.
        imageFactory : callable like `lsst.afw.image.Image`
            Image constructor for making the image.

        Returns
        -------
        ampImage : `lsst.afw.image.Image`
            The constructed image.
        """
        ampImage = makeImageFromAmp(amp, imValue=self.ampImValue, imageFactory=imageFactory,
                                    markSize=self.markSize, markValue=self.markValue,
                                    scaleGain=self.scaleGain)
        if self.isTrimmed:
            ampImage = ampImage.Factory(ampImage, amp.getRawDataBBox())
        return ampImage


class ButlerImage(FakeImageDataSource):
    """A class to return an Image of a given Ccd using the butler.

    Parameters
    ----------
    butler : `lsst.daf.butler.Butler` or `None`
        The butler to use. If `None`, an empty image is returned. Assumes that
        the instrument was specified during butler construction or is included
        in the ``kwargs`` parameter.
    type : `str`
        The type of image to read (e.g. raw, bias, flat, calexp).
    isTrimmed : `bool`
        If true, the showCamera command expects to be given trimmed images.
    verbose : `bool`
        Be chatty (in particular, log any error messages from the butler)?
    background : `float`
        The value of any pixels that lie outside the CCDs.
    callback : callable
        A function called with (image, detector, butler) for every image, which
        returns the image to be displayed (e.g. rawCallback). The image must
        be of the correct size, allowing for the value of isTrimmed.
    *args : `list`
        Passed to the base class constructor.
    **kwargs : `dict`
        Passed to the butler.

    Notes
    -----
    You can define a short named function as a callback::

        def callback(im, ccd, imageSource):
            return cameraGeom.utils.rawCallback(im, ccd, imageSource, correctGain=True)
    """
    def __init__(self, butler=None, type="raw",
                 isTrimmed=True, verbose=False, background=numpy.nan,
                 callback=None, *args, **kwargs):
        super().__init__(*args)
        self.isTrimmed = isTrimmed
        self.type = type
        self.butler = butler
        self.kwargs = kwargs
        self.isRaw = False
        self.background = background
        self.verbose = verbose
        self.callback = callback

    def _prepareImage(self, ccd, im, binSize, allowRotate=True):
        if binSize > 1:
            im = afwMath.binImage(im, binSize)

        if allowRotate:
            # if hasattr(ccd, "getOrientation"):
            im = afwMath.rotateImageBy90(
                im, ccd.getOrientation().getNQuarter())

        return im

    def getCcdImage(self, ccd, imageFactory=afwImage.ImageF, binSize=1, asMaskedImage=False):
        """Return an image of the specified ccd, and also the (possibly updated) ccd"""

        log = _LOG.getChild("ButlerImage")

        if self.isTrimmed:
            bbox = ccd.getBBox()
        else:
            bbox = calcRawCcdBBox(ccd)

        im = None
        if self.butler is not None:
            err = None
            try:
                im = self.butler.get(self.type, detector=ccd.getId(), **self.kwargs)
            except FitsError as e:
                err = IOError(e.args[0].split('\n')[0])  # It's a very chatty error
            except Exception as e:  # try a different dataId
                err = e
            else:
                if self.type == "raw":
                    ccd = im.getDetector()  # possibly modified by assembleCcdTask

            if im:
                if asMaskedImage:
                    im = im.getMaskedImage()
                else:
                    if hasattr(im, "getMaskedImage"):
                        im = im.getMaskedImage().getImage()
                    else:
                        im = im.getImage()
            else:
                if self.verbose:
                    # Lost by jupyterlab.
                    print(f"Reading {ccd.getId()}: {err}")

                log.warning("Reading %s: %s", ccd.getId(), err)

        if im is None:
            return self._prepareImage(ccd, imageFactory(*bbox.getDimensions()), binSize), ccd

        if self.type == "raw":
            if hasattr(im, 'convertF'):
                im = im.convertF()
            if False and self.callback is None:   # we need to trim the raw image
                self.callback = rawCallback

        allowRotate = True
        if self.callback:
            try:
                im = self.callback(im, ccd, imageSource=self)
            except Exception:
                if self.verbose:
                    log.exception("callback failed.")
                im = imageFactory(*bbox.getDimensions())
            else:
                allowRotate = False     # the callback was responsible for any rotations

        return self._prepareImage(ccd, im, binSize, allowRotate=allowRotate), ccd


def rawCallback(im, ccd=None, imageSource=None,
                correctGain=False, subtractBias=False, convertToFloat=False, obeyNQuarter=True):
    """A callback function that may or may not subtract bias/correct gain/trim
    a raw image.

    Parameters
    ----------
    im : `lsst.afw.image.Image` or `lsst.afw.image.MaskedImage` or `lsst.afw.image.Exposure`
       An image of a chip, ready to be binned and maybe rotated.
    ccd : `lsst.afw.cameraGeom.Detector` or `None`
        The Detector; if `None` assume that im is an exposure and extract its Detector.
    imageSource : `FakeImageDataSource` or `None`
        Source to get ccd images.  Must have a `getCcdImage()` method.
    correctGain : `bool`
        Correct each amplifier for its gain?
    subtractBias : `bool`
        Subtract the bias from each amplifier?
    convertToFloat : `bool`
        Convert ``im`` to floating point if possible.
    obeyNQuarter : `bool`
        Obey nQuarter from the Detector (default: True)

    Returns
    -------
    image : `lsst.afw.image.Image` like
        The constructed image (type returned by ``im.Factory``).

    Notes
    -----
    If imageSource is derived from ButlerImage, imageSource.butler is available.
    """
    if ccd is None:
        ccd = im.getDetector()
    if hasattr(im, "getMaskedImage"):
        im = im.getMaskedImage()
    if convertToFloat and hasattr(im, "convertF"):
        im = im.convertF()

    isTrimmed = imageSource.isTrimmed
    if isTrimmed:
        bbox = ccd.getBBox()
    else:
        bbox = calcRawCcdBBox(ccd)

    ampImages = []
    for a in ccd:
        if isTrimmed:
            data = im[a.getRawDataBBox()]
        else:
            data = im

        if subtractBias:
            bias = im[a.getRawHorizontalOverscanBBox()]
            data -= afwMath.makeStatistics(bias, afwMath.MEANCLIP).getValue()
        if correctGain:
            data *= a.getGain()

        ampImages.append(data)

    ccdImage = im.Factory(bbox)
    for ampImage, amp in zip(ampImages, ccd):
        if isTrimmed:
            assembleAmplifierImage(ccdImage, ampImage, amp)
        else:
            assembleAmplifierRawImage(ccdImage, ampImage, amp)

    if obeyNQuarter:
        nQuarter = ccd.getOrientation().getNQuarter()
        ccdImage = afwMath.rotateImageBy90(ccdImage, nQuarter)

    return ccdImage


def overlayCcdBoxes(ccd, untrimmedCcdBbox=None, nQuarter=0,
                    isTrimmed=False, ccdOrigin=(0, 0), display=None, binSize=1):
    """Overlay bounding boxes on an image display.

    Parameters
    ----------
    ccd : `lsst.afw.cameraGeom.Detector`
        Detector to iterate for the amp bounding boxes.
    untrimmedCcdBbox : `lsst.geom.Box2I` or `None`
        Bounding box of the un-trimmed Detector.
    nQuarter : `int`
        number of 90 degree rotations to apply to the bounding boxes (used for rotated chips).
    isTrimmed : `bool`
        Is the Detector image over which the boxes are layed trimmed?
    ccdOrigin : `tuple` of `float`
        Detector origin relative to the parent origin if in a larger pixel grid.
    display : `lsst.afw.display.Display`
        Image display to display on.
    binSize : `int`
        Bin the image by this factor in both dimensions.

    Notes
    -----
    The colours are:
    - Entire detector        GREEN
    - All data for amp       GREEN
    - HorizontalPrescan      YELLOW
    - HorizontalOverscan     RED
    - Data                   BLUE
    - VerticalOverscan       MAGENTA
    - VerticalOverscan       MAGENTA
    """
    if not display:                     # should be second parameter, and not defaulted!!
        raise RuntimeError("Please specify a display")

    if untrimmedCcdBbox is None:
        if isTrimmed:
            untrimmedCcdBbox = ccd.getBBox()
        else:
            untrimmedCcdBbox = lsst.geom.Box2I()
            for a in ccd.getAmplifiers():
                bbox = a.getRawBBox()
                untrimmedCcdBbox.include(bbox)

    with display.Buffering():
        ccdDim = untrimmedCcdBbox.getDimensions()
        ccdBbox = rotateBBoxBy90(untrimmedCcdBbox, nQuarter, ccdDim)
        for amp in ccd:
            if isTrimmed:
                ampbbox = amp.getBBox()
            else:
                ampbbox = amp.getRawBBox()
            if nQuarter != 0:
                ampbbox = rotateBBoxBy90(ampbbox, nQuarter, ccdDim)

            displayUtils.drawBBox(ampbbox, origin=ccdOrigin, borderWidth=0.49,
                                  display=display, bin=binSize)

            if not isTrimmed:
                for bbox, ctype in ((amp.getRawHorizontalOverscanBBox(), afwDisplay.RED),
                                    (amp.getRawDataBBox(), afwDisplay.BLUE),
                                    (amp.getRawVerticalOverscanBBox(),
                                     afwDisplay.MAGENTA),
                                    (amp.getRawPrescanBBox(), afwDisplay.YELLOW)):
                    if nQuarter != 0:
                        bbox = rotateBBoxBy90(bbox, nQuarter, ccdDim)
                    displayUtils.drawBBox(bbox, origin=ccdOrigin, borderWidth=0.49, ctype=ctype,
                                          display=display, bin=binSize)
            # Label each Amp
            xc, yc = ((ampbbox.getMin()[0] + ampbbox.getMax()[0])//2,
                      (ampbbox.getMin()[1] + ampbbox.getMax()[1])//2)
            #
            # Rotate the amp labels too
            #
            if nQuarter == 0:
                c, s = 1, 0
            elif nQuarter == 1:
                c, s = 0, -1
            elif nQuarter == 2:
                c, s = -1, 0
            elif nQuarter == 3:
                c, s = 0, 1
            c, s = 1, 0
            ccdHeight = ccdBbox.getHeight()
            ccdWidth = ccdBbox.getWidth()
            xc -= 0.5*ccdHeight
            yc -= 0.5*ccdWidth

            xc, yc = 0.5*ccdHeight + c*xc + s*yc, 0.5*ccdWidth + -s*xc + c*yc

            if ccdOrigin:
                xc += ccdOrigin[0]
                yc += ccdOrigin[1]
            display.dot(str(amp.getName()), xc/binSize,
                        yc/binSize, textAngle=nQuarter*90)

        displayUtils.drawBBox(ccdBbox, origin=ccdOrigin,
                              borderWidth=0.49, ctype=afwDisplay.MAGENTA, display=display, bin=binSize)


def showAmp(amp, imageSource=FakeImageDataSource(isTrimmed=False), display=None, overlay=True,
            imageFactory=afwImage.ImageU):
    """Show an amp in an image display.

    Parameters
    ----------
    amp : `lsst.afw.tables.AmpInfoRecord`
        Amp record to use in display.
    imageSource : `FakeImageDataSource` or `None`
        Source for getting the amp image.  Must have a ``getAmpImage()`` method.
    display : `lsst.afw.display.Display`
        Image display to use.
    overlay : `bool`
        Overlay bounding boxes?
    imageFactory : callable like `lsst.afw.image.Image`
        Type of image to display (only used if ampImage is `None`).
    """
    if not display:
        display = _getDisplayFromDisplayOrFrame(display)

    ampImage = imageSource.getAmpImage(amp, imageFactory=imageFactory)
    ampImSize = ampImage.getDimensions()
    title = amp.getName()
    display.mtv(ampImage, title=title)
    if overlay:
        with display.Buffering():
            if ampImSize == amp.getRawBBox().getDimensions():
                bboxes = [(amp.getRawBBox(), 0.49, afwDisplay.GREEN), ]
                xy0 = bboxes[0][0].getMin()
                bboxes.append(
                    (amp.getRawHorizontalOverscanBBox(), 0.49, afwDisplay.RED))
                bboxes.append((amp.getRawDataBBox(), 0.49, afwDisplay.BLUE))
                bboxes.append((amp.getRawPrescanBBox(),
                               0.49, afwDisplay.YELLOW))
                bboxes.append((amp.getRawVerticalOverscanBBox(),
                               0.49, afwDisplay.MAGENTA))
            else:
                bboxes = [(amp.getBBox(), 0.49, None), ]
                xy0 = bboxes[0][0].getMin()

            for bbox, borderWidth, ctype in bboxes:
                if bbox.isEmpty():
                    continue
                bbox = lsst.geom.Box2I(bbox)
                bbox.shift(-lsst.geom.ExtentI(xy0))
                displayUtils.drawBBox(
                    bbox, borderWidth=borderWidth, ctype=ctype, display=display)


def showCcd(ccd, imageSource=FakeImageDataSource(), display=None, overlay=True,
            imageFactory=afwImage.ImageF, binSize=1, inCameraCoords=False):
    """Show a CCD on display.

    Parameters
    ----------
    ccd : `lsst.afw.cameraGeom.Detector`
        Detector to use in display.
    imageSource : `FakeImageDataSource` or `None`
        Source to get ccd images.  Must have a ``getCcdImage()`` method.
    display : `lsst.afw.display.Display`
        image display to use.
    overlay : `bool`
        Show amp bounding boxes on the displayed image?
    imageFactory : callable like `lsst.afw.image.Image`
        The image factory to use in generating the images.
    binSize : `int`
        Bin the image by this factor in both dimensions.
    inCameraCoords : `bool`
        Show the Detector in camera coordinates?
    """
    display = _getDisplayFromDisplayOrFrame(display)

    ccdOrigin = lsst.geom.Point2I(0, 0)
    nQuarter = 0
    ccdImage, ccd = imageSource.getCcdImage(
        ccd, imageFactory=imageFactory, binSize=binSize)

    ccdBbox = ccdImage.getBBox()
    if ccdBbox.getDimensions() == ccd.getBBox().getDimensions():
        isTrimmed = True
    else:
        isTrimmed = False

    if inCameraCoords:
        nQuarter = ccd.getOrientation().getNQuarter()
        ccdImage = afwMath.rotateImageBy90(ccdImage, nQuarter)
    title = "{} [{}]".format(ccd.getName(), ccd.getId())
    if isTrimmed:
        title += "(trimmed)"

    if display:
        display.mtv(ccdImage, title=title)

        if overlay:
            overlayCcdBoxes(ccd, ccdBbox, nQuarter, isTrimmed,
                            ccdOrigin, display, binSize)

    return ccdImage


def getCcdInCamBBoxList(ccdList, binSize, pixelSize_o, origin):
    """Get the bounding boxes of a list of Detectors within a camera sized pixel grid

    Parameters
    ----------
    ccdList : `lsst.afw.cameraGeom.Detector`
        List of Detector.
    binSize : `int`
        Bin the image by this factor in both dimensions.
    pixelSize_o : `float`
        Size of the pixel in mm.
    origin : `int`
        Origin of the camera pixel grid in pixels.

    Returns
    -------
    boxList : `list` [`lsst.geom.Box2I`]
        A list of bounding boxes in camera pixel coordinates.
    """
    boxList = []
    for ccd in ccdList:
        if not pixelSize_o == ccd.getPixelSize():
            raise RuntimeError(
                "Cameras with detectors with different pixel scales are not currently supported")

        dbbox = lsst.geom.Box2D()
        for corner in ccd.getCorners(FOCAL_PLANE):
            dbbox.include(corner)
        llc = dbbox.getMin()
        nQuarter = ccd.getOrientation().getNQuarter()
        cbbox = ccd.getBBox()
        ex = cbbox.getDimensions().getX()//binSize
        ey = cbbox.getDimensions().getY()//binSize
        bbox = lsst.geom.Box2I(
            cbbox.getMin(), lsst.geom.Extent2I(int(ex), int(ey)))
        bbox = rotateBBoxBy90(bbox, nQuarter, bbox.getDimensions())
        bbox.shift(lsst.geom.Extent2I(int(llc.getX()//pixelSize_o.getX()/binSize),
                                      int(llc.getY()//pixelSize_o.getY()/binSize)))
        bbox.shift(lsst.geom.Extent2I(-int(origin.getX()//binSize),
                                      -int(origin.getY())//binSize))
        boxList.append(bbox)
    return boxList


def getCameraImageBBox(camBbox, pixelSize, bufferSize):
    """Get the bounding box of a camera sized image in pixels

    Parameters
    ----------
    camBbox : `lsst.geom.Box2D`
        Camera bounding box in focal plane coordinates (mm).
    pixelSize : `float`
        Size of a detector pixel in mm.
    bufferSize : `int`
        Buffer around edge of image in pixels.

    Returns
    -------
    box : `lsst.geom.Box2I`
        The resulting bounding box.
    """
    pixMin = lsst.geom.Point2I(int(camBbox.getMinX()//pixelSize.getX()),
                               int(camBbox.getMinY()//pixelSize.getY()))
    pixMax = lsst.geom.Point2I(int(camBbox.getMaxX()//pixelSize.getX()),
                               int(camBbox.getMaxY()//pixelSize.getY()))
    retBox = lsst.geom.Box2I(pixMin, pixMax)
    retBox.grow(bufferSize)
    return retBox


def makeImageFromCamera(camera, detectorNameList=None, background=numpy.nan, bufferSize=10,
                        imageSource=FakeImageDataSource(), imageFactory=afwImage.ImageU, binSize=1):
    """Make an Image of a Camera.

    Put each detector's image in the correct location and orientation on the
    focal plane. The input images can be binned to an integer fraction of their
    original bboxes.

    Parameters
    ----------
    camera : `lsst.afw.cameraGeom.Camera`
        Camera object to use to make the image.
    detectorNameList : `list` [`str`]
        List of detector names from ``camera`` to use in building the image.
        Use all Detectors if `None`.
    background : `float`
        Value to use where there is no Detector.
    bufferSize : `int`
        Size of border in binned pixels to make around the camera image.
    imageSource : `FakeImageDataSource` or `None`
        Source to get ccd images.  Must have a ``getCcdImage()`` method.
    imageFactory : callable like `lsst.afw.image.Image`
        Type of image to build.
    binSize : `int`
        Bin the image by this factor in both dimensions.

    Returns
    -------
    image : `lsst.afw.image.Image`
        Image of the entire camera.
    """
    log = _LOG.getChild("makeImageFromCamera")

    if detectorNameList is None:
        ccdList = camera
    else:
        ccdList = [camera[name] for name in detectorNameList]

    if detectorNameList is None:
        camBbox = camera.getFpBBox()
    else:
        camBbox = lsst.geom.Box2D()
        for detName in detectorNameList:
            for corner in camera[detName].getCorners(FOCAL_PLANE):
                camBbox.include(corner)

    pixelSize_o = camera[next(camera.getNameIter())].getPixelSize()
    camBbox = getCameraImageBBox(camBbox, pixelSize_o, bufferSize*binSize)
    origin = camBbox.getMin()

    camIm = imageFactory(int(math.ceil(camBbox.getDimensions().getX()/binSize)),
                         int(math.ceil(camBbox.getDimensions().getY()/binSize)))
    camIm[:] = imageSource.background

    assert imageSource.isTrimmed, "isTrimmed is False isn't supported by getCcdInCamBBoxList"

    boxList = getCcdInCamBBoxList(ccdList, binSize, pixelSize_o, origin)
    for det, bbox in zip(ccdList, boxList):
        im = imageSource.getCcdImage(det, imageFactory, binSize)[0]
        if im is None:
            continue

        imView = camIm.Factory(camIm, bbox, afwImage.LOCAL)
        try:
            imView[:] = im
        except pexExceptions.LengthError:
            nQuarter = det.getOrientation().getNQuarter()
            if nQuarter % 4:
                im = afwMath.rotateImageBy90(im, det.getOrientation().getNQuarter())
                imView[:] = im
            else:
                log.exception("Unable to fit image for detector \"%s\" [\"%d\"] into image of camera.",
                              det.getName(), det.getId())

    return camIm


def showCamera(camera, imageSource=FakeImageDataSource(), imageFactory=afwImage.ImageF,
               detectorNameList=None, binSize=10, bufferSize=10, overlay=True, title="",
               showWcs=None, ctype=afwDisplay.GREEN, textSize=1.25, originAtCenter=True, display=None,
               **kwargs):
    """Show a Camera on display, with the specified display.

    The rotation of the sensors is snapped to the nearest multiple of 90 deg.
    Also note that the pixel size is constant over the image array. The lower
    left corner (LLC) of each sensor amp is snapped to the LLC of the pixel
    containing the LLC of the image.

    Parameters
    ----------
    camera : `lsst.afw.cameraGeom.Camera`
        Camera object to use to make the image.
    imageSource : `FakeImageDataSource` or `None`
        Source to get ccd images.  Must have a ``getCcdImage()`` method.
    imageFactory : `lsst.afw.image.Image`
        Type of image to make
    detectorNameList : `list` [`str`] or `None`
        List of detector names from `camera` to use in building the image.
        Use all Detectors if `None`.
    binSize : `int`
        Bin the image by this factor in both dimensions.
    bufferSize : `int`
        Size of border in binned pixels to make around the camera image.
    overlay : `bool`
        Overlay Detector IDs and boundaries?
    title : `str`
        Title to use in display.
    showWcs : `bool`
        Include a WCS in the display?
    ctype : `lsst.afw.display.COLOR` or `str`
        Color to use when drawing Detector boundaries.
    textSize : `float`
        Size of detector labels
    originAtCenter : `bool`
        Put origin of the camera WCS at the center of the image?
        If `False`, the origin will be at the lower left.
    display : `lsst.afw.display`
        Image display on which to display.
    **kwargs :
        All remaining keyword arguments are passed to makeImageFromCamera

    Returns
    -------
    image : `lsst.afw.image.Image`
        The mosaic image.
    """
    display = _getDisplayFromDisplayOrFrame(display)

    if binSize < 1:
        binSize = 1
    cameraImage = makeImageFromCamera(camera, detectorNameList=detectorNameList, bufferSize=bufferSize,
                                      imageSource=imageSource, imageFactory=imageFactory, binSize=binSize,
                                      **kwargs)

    if detectorNameList is None:
        ccdList = [camera[name] for name in camera.getNameIter()]
    else:
        ccdList = [camera[name] for name in detectorNameList]

    if detectorNameList is None:
        camBbox = camera.getFpBBox()
    else:
        camBbox = lsst.geom.Box2D()
        for detName in detectorNameList:
            for corner in camera[detName].getCorners(FOCAL_PLANE):
                camBbox.include(corner)
    pixelSize = ccdList[0].getPixelSize()

    if showWcs:
        if originAtCenter:
            wcsReferencePixel = lsst.geom.Box2D(
                cameraImage.getBBox()).getCenter()
        else:
            wcsReferencePixel = lsst.geom.Point2I(0, 0)
        wcs = makeFocalPlaneWcs(pixelSize*binSize, wcsReferencePixel)
    else:
        wcs = None

    if display:
        if title == "":
            title = camera.getName()
        display.mtv(cameraImage, title=title, wcs=wcs)

        if overlay:
            with display.Buffering():
                camBbox = getCameraImageBBox(
                    camBbox, pixelSize, bufferSize*binSize)
                bboxList = getCcdInCamBBoxList(
                    ccdList, binSize, pixelSize, camBbox.getMin())
                for bbox, ccd in zip(bboxList, ccdList):
                    nQuarter = ccd.getOrientation().getNQuarter()
                    # borderWidth to 0.5 to align with the outside edge of the
                    # pixel
                    displayUtils.drawBBox(
                        bbox, borderWidth=0.5, ctype=ctype, display=display)
                    dims = bbox.getDimensions()
                    ccdLabel = "{}\n[{}]".format(ccd.getName(), ccd.getId())
                    display.dot(ccdLabel, bbox.getMinX() + dims.getX()/2, bbox.getMinY() + dims.getY()/2,
                                ctype=ctype, size=textSize, textAngle=nQuarter*90)

    return cameraImage


def makeFocalPlaneWcs(pixelSize, referencePixel):
    """Make a WCS for the focal plane geometry
    (i.e. one that returns positions in "mm")

    Parameters
    ----------
    pixelSize : `float`
        Size of the image pixels in physical units
    referencePixel : `lsst.geom.Point2D`
        Pixel for origin of WCS

    Returns
    -------
    `lsst.afw.geom.Wcs`
        Wcs object for mapping between pixels and focal plane.
    """
    md = dafBase.PropertySet()
    if referencePixel is None:
        referencePixel = lsst.geom.PointD(0, 0)
    for i in range(2):
        md.set("CRPIX%d"%(i + 1), referencePixel[i])
        md.set("CRVAL%d"%(i + 1), 0.)
    md.set("CDELT1", pixelSize[0])
    md.set("CDELT2", pixelSize[1])
    md.set("CTYPE1", "CAMERA_X")
    md.set("CTYPE2", "CAMERA_Y")
    md.set("CUNIT1", "mm")
    md.set("CUNIT2", "mm")

    return afwGeom.makeSkyWcs(md)


def findAmp(ccd, pixelPosition):
    """Find the Amp with the specified pixel position within the composite

    Parameters
    ----------
    ccd : `lsst.afw.cameraGeom.Detector`
        Detector to look in.
    pixelPosition : `lsst.geom.Point2I`
        The pixel position to find the amp for.

    Returns
    -------
    `lsst.afw.table.AmpInfoCatalog`
        Amp record in which ``pixelPosition`` falls or `None` if no Amp found.
    """
    for amp in ccd:
        if amp.getBBox().contains(pixelPosition):
            return amp

    return None
