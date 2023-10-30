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

# @file
# @brief Utilities to use with displaying images

import lsst.geom
import lsst.afw.image as afwImage

__all__ = (
    "Mosaic",
    "drawBBox", "drawFootprint", "drawCoaddInputs",
)


def _getDisplayFromDisplayOrFrame(display, frame=None):
    """Return an `lsst.afw.display.Display` given either a display or a frame ID.

    Notes
    -----
    If the two arguments are consistent, return the desired display; if they are not,
    raise a `RuntimeError` exception.

    If the desired display is `None`, return `None`;
    if ``(display, frame) == ("deferToFrame", None)``, return the default display
    """

    # import locally to allow this file to be imported by __init__
    import lsst.afw.display as afwDisplay

    if display in ("deferToFrame", None):
        if display is None and frame is None:
            return None

        # "deferToFrame" is the default value, and  means "obey frame"
        display = None

    if display and not hasattr(display, "frame"):
        raise RuntimeError(f"display == {display} doesn't support .frame")

    if frame and display and display.frame != frame:
        raise RuntimeError("Please specify display *or* frame")

    if display:
        frame = display.frame

    display = afwDisplay.getDisplay(frame, create=True)

    return display


class Mosaic:
    """A class to handle mosaics of one or more identically-sized images
    (or `~lsst.afw.image.Mask` or `~lsst.afw.image.MaskedImage`)

    Notes
    -----
    Note that this mosaic is a patchwork of the input images;  if you want to
    make a mosaic of a set images of the sky, you probably want to use the coadd code

    Examples
    --------

    .. code-block:: py

       m = Mosaic()
       m.setGutter(5)
       m.setBackground(10)
       m.setMode("square")                     # the default; other options are "x" or "y"

       mosaic = m.makeMosaic(im1, im2, im3)    # build the mosaic
       display = afwDisplay.getDisplay()
       display.mtv(mosaic)                         # display it
       m.drawLabels(["Label 1", "Label 2", "Label 3"], display) # label the panels

       # alternative way to build a mosaic
       images = [im1, im2, im3]
       labels = ["Label 1", "Label 2", "Label 3"]

       mosaic = m.makeMosaic(images)
       display.mtv(mosaic)
       m.drawLabels(labels, display)

       # Yet another way to build a mosaic (no need to build the images/labels lists)
       for i in range(len(images)):
           m.append(images[i], labels[i])
       # You may optionally include a colour, e.g. afwDisplay.YELLOW, as a third argument

       mosaic = m.makeMosaic()
       display.mtv(mosaic)
       m.drawLabels(display=display)

    Or simply:

    .. code-block:: py

       mosaic = m.makeMosaic(display=display)

    You can return the (ix, iy)th (or nth) bounding box (in pixels) with `getBBox()`
    """

    def __init__(self, gutter=3, background=0, mode="square"):
        self.gutter = gutter            # number of pixels between panels in a mosaic
        self.background = background    # value in gutters
        self.setMode(mode)              # mosaicing mode
        self.xsize = 0                  # column size of panels
        self.ysize = 0                  # row size of panels

        self.reset()

    def reset(self):
        """Reset the list of images to be mosaiced"""
        self.images = []                # images to mosaic together
        self.labels = []                # labels for images

    def append(self, image, label=None, ctype=None):
        """Add an image to the list of images to be mosaiced

        Returns
        -------
        index
            the index of this image (may be passed to `getBBox()`)

        Notes
        -----
        Set may be cleared with ``Mosaic.reset()``
        """
        if not self.xsize:
            self.xsize = image.getWidth()
            self.ysize = image.getHeight()

        self.images.append(image)
        self.labels.append((label, ctype))

        return len(self.images)

    def makeMosaic(self, images=None, display="deferToFrame", mode=None, nxMultiple=None,
                   background=None, title=""):
        """Return a mosaic of all the images provided.

        If none are specified, use the list accumulated with `Mosaic.append()`.

        If display is specified, display the mosaic.

        Parameters
        ----------
        images : `list` of `lsst.afw.image.MaskedImage`, optional
            List of images to mosaic.
        display : `str`, optional
            Display control string.
        mode : `str`, optional
            Mosaicing mode.  Allowed values include:
            "square" : Make mosaic as square as possible.
                       Obey ``nxMultiple``.
            "x" : Make mosaic one image high.
            "y" : Make mosaic one image wide.
        nxMultiple : `float`
            The number of associated images you want to line up (i.e. if your
            images list consists of an image then an image-model, then you
            would want an nxMultiple of 2.  Or, if you had an (image,
            model-imgae, image-model) triplet, you'd want an nxMultibple of 3.
            Ignored unless `mode`="square".
        background : `float`
           Value to set the gutters between images to.
        """
        if images:
            if self.images:
                raise RuntimeError(
                    f"You have already appended {len(self.images)} images to this Mosaic")

            try:
                len(images)             # check that it quacks like a list
            except TypeError:
                images = [images]

            self.images = images
        else:
            images = self.images

        if self.nImage == 0:
            raise RuntimeError("You must provide at least one image")

        self.xsize, self.ysize = 0, 0
        for im in images:
            w, h = im.getWidth(), im.getHeight()
            if w > self.xsize:
                self.xsize = w
            if h > self.ysize:
                self.ysize = h

        if background is None:
            background = self.background
        if mode is None:
            mode = self.mode

        if mode == "square":
            nx, ny = 1, self.nImage
            while nx*im.getWidth() < ny*im.getHeight():
                nx += 1
                ny = self.nImage//nx

                if nx*ny < self.nImage:
                    ny += 1
                if nx*ny < self.nImage:
                    nx += 1

            if nx > self.nImage:
                nx = self.nImage
            if nxMultiple and nx%nxMultiple != 0:
                nx += (nxMultiple - nx%nxMultiple)
                while nx*(ny - 1) >= self.nImage:
                    ny -= 1
            assert nx*ny >= self.nImage
        elif mode == "x":
            nx, ny = self.nImage, 1
        elif mode == "y":
            nx, ny = 1, self.nImage
        elif isinstance(mode, int):
            nx = mode
            ny = self.nImage//nx
            if nx*ny < self.nImage:
                ny += 1
        else:
            raise RuntimeError(f"Unknown mosaicing mode: {mode}")

        self.nx, self.ny = nx, ny

        mosaic = images[0].Factory(
            lsst.geom.Extent2I(nx*self.xsize + (nx - 1)*self.gutter,
                               ny*self.ysize + (ny - 1)*self.gutter)
        )
        try:
            mosaic.set(self.background)
        except AttributeError:
            raise RuntimeError(f"Attempt to mosaic images of type {type(mosaic)} which don't support set")

        for i in range(len(images)):
            smosaic = mosaic.Factory(
                mosaic, self.getBBox(i%nx, i//nx), afwImage.LOCAL)
            im = images[i]

            if smosaic.getDimensions() != im.getDimensions():  # im is smaller than smosaic
                llc = lsst.geom.PointI((smosaic.getWidth() - im.getWidth())//2,
                                       (smosaic.getHeight() - im.getHeight())//2)
                smosaic = smosaic.Factory(smosaic, lsst.geom.Box2I(
                    llc, im.getDimensions()), afwImage.LOCAL)

            smosaic[:] = im

        display = _getDisplayFromDisplayOrFrame(display)
        if display:
            display.mtv(mosaic, title=title)

            if images == self.images:
                self.drawLabels(display=display)

        return mosaic

    def setGutter(self, gutter):
        """Set the number of pixels between panels in a mosaic
        """
        self.gutter = gutter

    def setBackground(self, background):
        """Set the value in the gutters
        """
        self.background = background

    def setMode(self, mode):
        """Set mosaicing mode.

        Parameters
        ----------
        mode : {"square", "x", "y"}
            Valid options:

            square
                Make mosaic as square as possible
            x
                Make mosaic one image high
            y
                Make mosaic one image wide
        """

        if mode not in ("square", "x", "y"):
            raise RuntimeError(f"Unknown mosaicing mode: {mode}")

        self.mode = mode

    def getBBox(self, ix, iy=None):
        """Get the BBox for a panel

        Parameters
        ----------
        ix : `int`
            If ``iy`` is not `None`, this is the x coordinate of the panel.
            If ``iy`` is `None`, this is the number of the panel.
        iy : `int`, optional
            The y coordinate of the panel.
        """

        if iy is None:
            ix, iy = ix % self.nx, ix//self.nx

        return lsst.geom.Box2I(lsst.geom.PointI(ix*(self.xsize + self.gutter), iy*(self.ysize + self.gutter)),
                               lsst.geom.ExtentI(self.xsize, self.ysize))

    def drawLabels(self, labels=None, display="deferToFrame", frame=None):
        """Draw the list labels at the corners of each panel.

        Notes
        -----
        If labels is None, use the ones specified by ``Mosaic.append()``
        """

        if not labels:
            labels = self.labels

        if not labels:
            return

        if len(labels) != self.nImage:
            raise RuntimeError(f"You provided {len(labels)} labels for {self.nImage} panels")

        display = _getDisplayFromDisplayOrFrame(display, frame)
        if not display:
            return

        with display.Buffering():
            for i in range(len(labels)):
                if labels[i]:
                    label, ctype = labels[i], None
                    try:
                        label, ctype = label
                    except Exception:
                        pass

                    if not label:
                        continue

                    display.dot(str(label), self.getBBox(i).getMinX(),
                                self.getBBox(i).getMinY(), ctype=ctype)

    @property
    def nImage(self):
        """Number of images
        """
        return len(self.images)


def drawBBox(bbox, borderWidth=0.0, origin=None, display="deferToFrame", ctype=None, bin=1, frame=None):
    """Draw a bounding box on a display frame with the specified ctype.

    Parameters
    ----------
    bbox : `lsst.geom.Box2I` or `lsst.geom.Box2D`
        The box to draw
    borderWidth : `float`
        Include this many pixels
    origin
        If specified, the box is shifted by ``origin``
    display : `str`
    ctype : `str`
        The desired color, either e.g. `lsst.afw.display.RED` or a color name known to X11
    bin : `int`
        All BBox coordinates are divided by bin, as is right and proper for overlaying on a binned image
    frame
    """
    x0, y0 = bbox.getMinX(), bbox.getMinY()
    x1, y1 = bbox.getMaxX(), bbox.getMaxY()

    if origin:
        x0 += origin[0]
        x1 += origin[0]
        y0 += origin[1]
        y1 += origin[1]

    x0 /= bin
    y0 /= bin
    x1 /= bin
    y1 /= bin
    borderWidth /= bin

    display = _getDisplayFromDisplayOrFrame(display, frame)
    display.line([(x0 - borderWidth, y0 - borderWidth),
                  (x0 - borderWidth, y1 + borderWidth),
                  (x1 + borderWidth, y1 + borderWidth),
                  (x1 + borderWidth, y0 - borderWidth),
                  (x0 - borderWidth, y0 - borderWidth),
                  ], ctype=ctype)


def drawFootprint(foot, borderWidth=0.5, origin=None, XY0=None, frame=None, ctype=None, bin=1,
                  peaks=False, symb="+", size=0.4, ctypePeak=None, display="deferToFrame"):
    """Draw an `lsst.afw.detection.Footprint` on a display frame with the specified ctype.

    Parameters
    ----------
    foot : `lsst.afw.detection.Footprint`
    borderWidth : `float`
        Include an extra borderWidth pixels
    origin
        If ``origin`` is present, it's arithmetically added to the Footprint
    XY0
        if ``XY0`` is present is subtracted from the Footprint
    frame
    ctype : `str`
        The desired color, either e.g. `lsst.afw.display.RED` or a color name known to X11
    bin : `int`
        All Footprint coordinates are divided by bin, as is right and proper
        for overlaying on a binned image
    peaks : `bool`
        If peaks is `True`, also show the object's Peaks using the specified
        ``symb`` and ``size`` and ``ctypePeak``
    symb : `str`
    size : `float`
    ctypePeak : `str`
        The desired color for peaks, either e.g. `lsst.afw.display.RED` or a color name known to X11
    display : `str`
    """

    if XY0:
        if origin:
            raise RuntimeError("You may not specify both origin and XY0")
        origin = (-XY0[0], -XY0[1])

    display = _getDisplayFromDisplayOrFrame(display, frame)
    with display.Buffering():
        borderWidth /= bin
        for s in foot.getSpans():
            y, x0, x1 = s.getY(), s.getX0(), s.getX1()

            if origin:
                x0 += origin[0]
                x1 += origin[0]
                y += origin[1]

            x0 /= bin
            x1 /= bin
            y /= bin

            display.line([(x0 - borderWidth, y - borderWidth),
                          (x0 - borderWidth, y + borderWidth),
                          (x1 + borderWidth, y + borderWidth),
                          (x1 + borderWidth, y - borderWidth),
                          (x0 - borderWidth, y - borderWidth),
                          ], ctype=ctype)

        if peaks:
            for p in foot.getPeaks():
                x, y = p.getIx(), p.getIy()

                if origin:
                    x += origin[0]
                    y += origin[1]

                x /= bin
                y /= bin

                display.dot(symb, x, y, size=size, ctype=ctypePeak)


def drawCoaddInputs(exposure, frame=None, ctype=None, bin=1, display="deferToFrame"):
    """Draw the bounding boxes of input exposures to a coadd on a display
    frame with the specified ctype, assuming ``display.mtv()`` has already been
    called on the given exposure on this frame.

    All coordinates are divided by ``bin``, as is right and proper for overlaying on a binned image
    """
    coaddWcs = exposure.getWcs()
    catalog = exposure.getInfo().getCoaddInputs().ccds

    offset = lsst.geom.PointD() - lsst.geom.PointD(exposure.getXY0())

    display = _getDisplayFromDisplayOrFrame(display, frame)

    with display.Buffering():
        for record in catalog:
            ccdBox = lsst.geom.Box2D(record.getBBox())
            ccdCorners = ccdBox.getCorners()
            coaddCorners = [coaddWcs.skyToPixel(record.getWcs().pixelToSky(point)) + offset
                            for point in ccdCorners]
            display.line([(coaddCorners[i].getX()/bin, coaddCorners[i].getY()/bin)
                          for i in range(-1, 4)], ctype=ctype)
