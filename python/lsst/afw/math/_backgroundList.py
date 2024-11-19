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

__all__ = ["BackgroundList"]

import os
import lsst.daf.base as dafBase
import lsst.geom
import lsst.afw.image as afwImage
from lsst.afw.fits import MemFileManager, reduceToFits, Fits
from ._math import Interpolate, ApproximateControl, BackgroundMI, UndersampleStyle


class BackgroundList:
    """A list-like class to contain a list of (`lsst.afw.math.Background`,
    `lsst.afw.math.Interpolate.Style`, `~lsst.afw.math.UndersampleStyle`)
    tuples.

    Parameters
    ----------
    *args : `tuple` or `~lsst.afw.math.Background`
        A sequence of arguments, each of which becomes an element of the list.
        We also accept a single `lsst.afw.math.Background` and extract the
        ``interpStyle`` and ``undersampleStyle`` from the as-used values.
    """

    def __init__(self, *args):
        self._backgrounds = []
        for a in args:
            self.append(a)

    def __getitem__(self, *args):
        """Return an item

        Parameters
        ----------
        *args
            Any valid list index.
        """
        #
        # Set any previously-unknown Styles (they are set by bkgd.getImage())
        #
        for i, val in enumerate(self._backgrounds):
            bkgd, interpStyle, undersampleStyle, approxStyle, \
                approxOrderX, approxOrderY, approxWeighting = val
            if interpStyle is None or undersampleStyle is None:
                interpStyle = bkgd.getAsUsedInterpStyle()
                undersampleStyle = bkgd.getAsUsedUndersampleStyle()
                actrl = bkgd.getBackgroundControl().getApproximateControl()
                approxStyle = actrl.getStyle()
                approxOrderX = actrl.getOrderX()
                approxOrderY = actrl.getOrderY()
                approxWeighting = actrl.getWeighting()
                self._backgrounds[i] = (bkgd, interpStyle, undersampleStyle,
                                        approxStyle, approxOrderX, approxOrderY, approxWeighting)
        #
        # And return what they wanted
        #
        return self._backgrounds.__getitem__(*args)

    def __len__(self, *args):
        return self._backgrounds.__len__(*args)

    def append(self, val):
        try:
            bkgd, interpStyle, undersampleStyle, approxStyle, \
                approxOrderX, approxOrderY, approxWeighting = val
        except TypeError:
            bkgd = val
            interpStyle = None
            undersampleStyle = None
            approxStyle = None
            approxOrderX = None
            approxOrderY = None
            approxWeighting = None

        bgInfo = (bkgd, interpStyle, undersampleStyle, approxStyle,
                  approxOrderX, approxOrderY, approxWeighting)
        self._backgrounds.append(bgInfo)

    def clone(self):
        """Return a shallow copy

        Shallow copies do not share backgrounds that are appended after copying,
        but do share changes to contained background objects.
        """
        return BackgroundList(*self)

    def writeFits(self, fileName, flags=0):
        """Save our list of Backgrounds to a file.

        Parameters
        -----------
        fileName : `str`
            FITS file to write
        flags : `int`
            Flags to control details of writing; currently unused, but present
            for consistency with `lsst.afw.table.BaseCatalog.writeFits`.
        """

        for i, bkgd in enumerate(self):
            (bkgd, interpStyle, undersampleStyle, approxStyle, approxOrderX, approxOrderY,
             approxWeighting) = bkgd
            statsImage = bkgd.getStatsImage()

            md = dafBase.PropertyList()
            md.set("INTERPSTYLE", interpStyle.value)
            md.set("UNDERSAMPLESTYLE", undersampleStyle.value)
            md.set("APPROXSTYLE", approxStyle.value)
            md.set("APPROXORDERX", approxOrderX)
            md.set("APPROXORDERY", approxOrderY)
            md.set("APPROXWEIGHTING", approxWeighting)
            bbox = bkgd.getImageBBox()
            md.set("BKGD_X0", bbox.getMinX())
            md.set("BKGD_Y0", bbox.getMinY())
            md.set("BKGD_WIDTH", bbox.getWidth())
            md.set("BKGD_HEIGHT", bbox.getHeight())

            statsImage.getImage().writeFits(fileName, md, "w" if i == 0 else "a")
            statsImage.getMask().writeFits(fileName, md, "a")
            statsImage.getVariance().writeFits(fileName, md, "a")

    @staticmethod
    def readFits(fileName, hdu=0, flags=0):
        """Read our list of Backgrounds from a file.

        Parameters
        ----------
        fileName : `str`
            FITS file to read
        hdu : `int`
            First Header/Data Unit to attempt to read from
        flags : `int`
            Flags to control details of reading; currently unused, but present
            for consistency with `lsst.afw.table.BaseCatalog.readFits`.

        See Also
        --------
        getImage
        """
        if not isinstance(fileName, MemFileManager) and not os.path.exists(fileName):
            raise RuntimeError(f"File not found: {fileName}")

        self = BackgroundList()

        f = Fits(fileName, 'r')
        nHdus = f.countHdus()
        f.closeFile()
        if nHdus % 3 != 0:
            raise RuntimeError(f"BackgroundList FITS file {fileName} has {nHdus} HDUs;"
                               f"expected a multiple of 3 (compression is not supported).")

        for hdu in range(0, nHdus, 3):
            # It seems like we ought to be able to just use
            # MaskedImageFitsReader here, but it warns about EXTTYPE and still
            # doesn't work quite naturally when starting from a nonzero HDU.
            imageReader = afwImage.ImageFitsReader(fileName, hdu=hdu)
            maskReader = afwImage.MaskFitsReader(fileName, hdu=hdu + 1)
            varianceReader = afwImage.ImageFitsReader(fileName, hdu=hdu + 2)
            statsImage = afwImage.MaskedImageF(imageReader.read(), maskReader.read(), varianceReader.read())
            md = imageReader.readMetadata()

            x0 = md["BKGD_X0"]
            y0 = md["BKGD_Y0"]
            width = md["BKGD_WIDTH"]
            height = md["BKGD_HEIGHT"]
            imageBBox = lsst.geom.BoxI(lsst.geom.PointI(x0, y0), lsst.geom.ExtentI(width, height))

            interpStyle = Interpolate.Style(md["INTERPSTYLE"])
            undersampleStyle = UndersampleStyle(md["UNDERSAMPLESTYLE"])

            # Older outputs won't have APPROX* settings.  Provide alternative defaults.
            # Note: Currently X- and Y-orders must be equal due to a limitation in
            #       math::Chebyshev1Function2.  Setting approxOrderY = -1 is equivalent
            #       to saying approxOrderY = approxOrderX.
            approxStyle = md.get("APPROXSTYLE", ApproximateControl.UNKNOWN)
            approxStyle = ApproximateControl.Style(approxStyle)
            approxOrderX = md.get("APPROXORDERX", 1)
            approxOrderY = md.get("APPROXORDERY", -1)
            approxWeighting = md.get("APPROXWEIGHTING", True)

            bkgd = BackgroundMI(imageBBox, statsImage)
            bctrl = bkgd.getBackgroundControl()
            bctrl.setInterpStyle(interpStyle)
            bctrl.setUndersampleStyle(undersampleStyle)
            actrl = ApproximateControl(approxStyle, approxOrderX, approxOrderY, approxWeighting)
            bctrl.setApproximateControl(actrl)
            bgInfo = (bkgd, interpStyle, undersampleStyle, approxStyle,
                      approxOrderX, approxOrderY, approxWeighting)
            self.append(bgInfo)

        return self

    def getImage(self):
        """Compute and return a full-resolution image from our list of
        (Background, interpStyle, undersampleStyle).
        """

        bkgdImage = None
        for (bkgd, interpStyle, undersampleStyle, approxStyle,
             approxOrderX, approxOrderY, approxWeighting) in self:
            if not bkgdImage:
                if approxStyle != ApproximateControl.UNKNOWN:
                    bkgdImage = bkgd.getImageF()
                else:
                    bkgdImage = bkgd.getImageF(interpStyle, undersampleStyle)
            else:
                if approxStyle != ApproximateControl.UNKNOWN:
                    bkgdImage += bkgd.getImageF()
                else:
                    bkgdImage += bkgd.getImageF(interpStyle, undersampleStyle)

        return bkgdImage

    def __reduce__(self):
        return reduceToFits(self)
