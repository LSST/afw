#
# LSST Data Management System
# Copyright 2008, 2009, 2010 LSST Corporation.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
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
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <http://www.lsstcorp.org/LegalNotices/>.
#
import unittest

import numpy as np

import lsst.utils.tests
import lsst.daf.base as dafBase
import lsst.geom
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
from lsst.afw.image.testUtils import imagesDiffer
from lsst.afw.geom.utils import _compareWcsOverBBox


class TestTestUtils(lsst.utils.tests.TestCase):
    """Test test methods added to lsst.utils.tests.TestCase
    """
    def testAssertWcsAlmostEqualOverBBox(self):
        """Test assertWcsAlmostEqualOverBBox and wcsAlmostEqualOverBBox"""
        bbox = lsst.geom.Box2I(lsst.geom.Point2I(0, 0),
                               lsst.geom.Extent2I(3001, 3001))
        ctrPix = lsst.geom.Point2I(1500, 1500)
        metadata = dafBase.PropertySet()
        metadata.set("RADESYS", "FK5")
        metadata.set("EQUINOX", 2000.0)
        metadata.set("CTYPE1", "RA---TAN")
        metadata.set("CTYPE2", "DEC--TAN")
        metadata.set("CUNIT1", "deg")
        metadata.set("CUNIT2", "deg")
        metadata.set("CRVAL1", 215.5)
        metadata.set("CRVAL2", 53.0)
        metadata.set("CRPIX1", ctrPix[0] + 1)
        metadata.set("CRPIX2", ctrPix[1] + 1)
        metadata.set("CD1_1", 5.1e-05)
        metadata.set("CD1_2", 0.0)
        metadata.set("CD2_2", -5.1e-05)
        metadata.set("CD2_1", 0.0)
        wcs0 = lsst.afw.geom.makeSkyWcs(metadata, strip=False)
        metadata.set("CRVAL2", 53.000001)  # tweak CRVAL2 for wcs1
        wcs1 = lsst.afw.geom.makeSkyWcs(metadata)

        self.assertWcsAlmostEqualOverBBox(wcs0, wcs0, bbox,
                                          maxDiffSky=0*lsst.geom.arcseconds, maxDiffPix=0)
        self.assertTrue(afwGeom.wcsAlmostEqualOverBBox(wcs0, wcs0, bbox,
                                                       maxDiffSky=0*lsst.geom.arcseconds, maxDiffPix=0))

        self.assertWcsAlmostEqualOverBBox(wcs0, wcs1, bbox,
                                          maxDiffSky=0.04*lsst.geom.arcseconds, maxDiffPix=0.02)
        self.assertTrue(afwGeom.wcsAlmostEqualOverBBox(wcs0, wcs1, bbox,
                                                       maxDiffSky=0.04*lsst.geom.arcseconds, maxDiffPix=0.02))

        with self.assertRaises(AssertionError):
            self.assertWcsAlmostEqualOverBBox(wcs0, wcs1, bbox,
                                              maxDiffSky=0.001*lsst.geom.arcseconds, maxDiffPix=0.02)
        self.assertFalse(afwGeom.wcsAlmostEqualOverBBox(wcs0, wcs1, bbox,
                                                        maxDiffSky=0.001*lsst.geom.arcseconds,
                                                        maxDiffPix=0.02))

        with self.assertRaises(AssertionError):
            self.assertWcsAlmostEqualOverBBox(wcs0, wcs1, bbox,
                                              maxDiffSky=0.04*lsst.geom.arcseconds, maxDiffPix=0.001)
        self.assertFalse(afwGeom.wcsAlmostEqualOverBBox(wcs0, wcs1, bbox,
                                                        maxDiffSky=0.04*lsst.geom.arcseconds,
                                                        maxDiffPix=0.001))

        # check that doShortCircuit works in the private implementation
        errStr1 = _compareWcsOverBBox(wcs0, wcs1, bbox,
                                      maxDiffSky=0.001*lsst.geom.arcseconds, maxDiffPix=0.001,
                                      doShortCircuit=False)
        errStr2 = _compareWcsOverBBox(wcs0, wcs1, bbox,
                                      maxDiffSky=0.001*lsst.geom.arcseconds, maxDiffPix=0.001,
                                      doShortCircuit=True)
        self.assertNotEqual(errStr1, errStr2)

    def checkMaskedImage(self, mi):
        """Run assertImage-like function tests on a masked image

        Compare the masked image to itself, then alter copies and check that the altered copy
        is or is not nearly equal the original, depending on the amount of change, rtol and atol
        """
        epsilon = 1e-5  # margin to avoid roundoff error

        mi0 = mi.Factory(mi, True)  # deep copy
        mi1 = mi.Factory(mi, True)

        # a masked image should be exactly equal to itself
        self.assertMaskedImagesEqual(mi0, mi1)
        self.assertMaskedImagesEqual(mi1, mi0)
        self.assertMaskedImagesAlmostEqual(mi0, mi1, atol=0, rtol=0)
        self.assertMaskedImagesAlmostEqual(mi0, mi1, atol=0, rtol=0)
        self.assertMaskedImagesAlmostEqual(
            (mi0.image.array, mi0.mask.array, mi0.variance.array), mi1, atol=0, rtol=0)
        self.assertMaskedImagesAlmostEqual(
            mi0, (mi1.image.array, mi1.mask.array, mi1.variance.array), atol=0, rtol=0)
        self.assertMaskedImagesAlmostEqual(
            (mi0.image.array, mi0.mask.array, mi0.variance.array),
            (mi1.image.array, mi1.mask.array, mi1.variance.array), atol=0, rtol=0)
        for getName in ("getImage", "getVariance"):
            plane0 = getattr(mi0, getName)()
            plane1 = getattr(mi1, getName)()
            self.assertImagesEqual(plane0, plane1)
            self.assertImagesEqual(plane1, plane0)
            self.assertImagesAlmostEqual(plane0, plane1, atol=0, rtol=0)
            self.assertImagesAlmostEqual(plane1, plane0, atol=0, rtol=0)
            self.assertImagesAlmostEqual(
                plane0.getArray(), plane1, atol=0, rtol=0)
            self.assertImagesAlmostEqual(
                plane0, plane1.getArray(), atol=0, rtol=0)
            self.assertImagesAlmostEqual(
                plane0.getArray(), plane1.getArray(), atol=0, rtol=0)
            self.assertMasksEqual(plane0, plane1)
            self.assertMasksEqual(plane1, plane0)
            self.assertMasksEqual(plane0.getArray(), plane1)
            self.assertMasksEqual(plane0, plane1.getArray())
            self.assertMasksEqual(plane0.getArray(), plane1.getArray())
        self.assertMasksEqual(mi0.getMask(), mi1.getMask())
        self.assertMasksEqual(mi1.getMask(), mi0.getMask())

        # alter image and variance planes and check the results
        for getName in ("getImage", "getVariance"):
            isFloat = getattr(mi, getName)().getArray().dtype.kind == "f"
            if isFloat:
                for errVal in (np.nan, np.inf, -np.inf):
                    mi0 = mi.Factory(mi, True)
                    mi1 = mi.Factory(mi, True)
                    plane0 = getattr(mi0, getName)()
                    plane1 = getattr(mi1, getName)()
                    plane1[2, 2] = errVal
                    with self.assertRaises(Exception):
                        self.assertImagesAlmostEqual(plane0, plane1)
                    with self.assertRaises(Exception):
                        self.assertImagesAlmostEqual(plane0.getArray(), plane1)
                    with self.assertRaises(Exception):
                        self.assertImagesAlmostEqual(plane1, plane0)
                    with self.assertRaises(Exception):
                        self.assertMaskedImagesAlmostEqual(mi0, mi1)
                    with self.assertRaises(Exception):
                        self.assertMaskedImagesAlmostEqual(
                            mi0, (mi1.image.array, mi1.mask.array, mi1.variance.array))
                    with self.assertRaises(Exception):
                        self.assertMaskedImagesAlmostEqual(mi1, mi0)

                    skipMask = mi.getMask().Factory(mi.getMask(), True)
                    skipMaskArr = skipMask.getArray()
                    skipMaskArr[:] = 0
                    skipMaskArr[2, 2] = 1
                    self.assertImagesAlmostEqual(
                        plane0, plane1, skipMask=skipMaskArr, atol=0, rtol=0)
                    self.assertImagesAlmostEqual(
                        plane0, plane1, skipMask=skipMask, atol=0, rtol=0)
                    self.assertMaskedImagesAlmostEqual(
                        mi0, mi1, skipMask=skipMaskArr, atol=0, rtol=0)
                    self.assertMaskedImagesAlmostEqual(
                        mi0, mi1, skipMask=skipMask, atol=0, rtol=0)

                for dval in (0.001, 0.03):
                    mi0 = mi.Factory(mi, True)
                    mi1 = mi.Factory(mi, True)
                    plane0 = getattr(mi0, getName)()
                    plane1 = getattr(mi1, getName)()
                    plane1[2, 2] += dval
                    val1 = plane1[2, 2, afwImage.LOCAL]
                    self.assertImagesAlmostEqual(
                        plane0, plane1, rtol=0, atol=dval + epsilon)
                    self.assertImagesAlmostEqual(
                        plane0, plane1, rtol=dval/val1 + epsilon, atol=0)
                    self.assertMaskedImagesAlmostEqual(
                        mi0, mi1, rtol=0, atol=dval + epsilon)
                    self.assertMaskedImagesAlmostEqual(
                        mi1, mi0, rtol=0, atol=dval + epsilon)
                    with self.assertRaises(Exception):
                        self.assertImagesAlmostEqual(
                            plane0, plane1, rtol=0, atol=dval - epsilon)
                    with self.assertRaises(Exception):
                        self.assertImagesAlmostEqual(
                            plane0, plane1, rtol=dval/val1 - epsilon, atol=0)
                    with self.assertRaises(Exception):
                        self.assertMaskedImagesAlmostEqual(
                            mi0, mi1, rtol=0, atol=dval - epsilon)
                    with self.assertRaises(Exception):
                        self.assertMaskedImagesAlmostEqual(
                            mi0, mi1, rtol=dval/val1 - epsilon, atol=0)
            else:
                # plane is an integer of some type
                for dval in (1, 3):
                    mi0 = mi.Factory(mi, True)
                    mi1 = mi.Factory(mi, True)
                    plane0 = getattr(mi0, getName)()
                    plane1 = getattr(mi1, getName)()
                    plane1[2, 2] += dval
                    val1 = plane1[2, 2, afwImage.LOCAL]
                    # int value and test is <= so epsilon not required for atol
                    # but rtol is a fraction, so epsilon is still safest for
                    # the rtol test
                    self.assertImagesAlmostEqual(
                        plane0, plane1, rtol=0, atol=dval)
                    self.assertImagesAlmostEqual(
                        plane0, plane1, rtol=dval/val1 + epsilon, atol=0)
                    with self.assertRaises(Exception):
                        self.assertImagesAlmostEqual(
                            plane0, plane1, rtol=0, atol=dval - epsilon)
                    with self.assertRaises(Exception):
                        self.assertImagesAlmostEqual(
                            plane0, plane1, rtol=dval/val1 - epsilon, atol=0)

        # alter mask and check the results
        mi0 = mi.Factory(mi, True)
        mi1 = mi.Factory(mi, True)
        mask0 = mi0.getMask()
        mask1 = mi1.getMask()
        for dval in (1, 3):
            # getArray avoids "unsupported operand type" failure
            mask1.getArray()[2, 2] += 1
            with self.assertRaises(Exception):
                self.assertMasksEqual(mask0, mask1)
            with self.assertRaises(Exception):
                self.assertMasksEqual(mask1, mask0)
            with self.assertRaises(Exception):
                self.assertMaskedImagesEqual(mi0, mi1)
            with self.assertRaises(Exception):
                self.assertMaskedImagesEqual(mi1, mi0)

        skipMask = mi.getMask().Factory(mi.getMask(), True)
        skipMaskArr = skipMask.getArray()
        skipMaskArr[:] = 0
        skipMaskArr[2, 2] = 1
        self.assertMasksEqual(mask0, mask1, skipMask=skipMaskArr)
        self.assertMasksEqual(mask0, mask1, skipMask=skipMask)
        self.assertMaskedImagesAlmostEqual(
            mi0, mi1, skipMask=skipMaskArr, atol=0, rtol=0)
        self.assertMaskedImagesAlmostEqual(
            mi0, mi1, skipMask=skipMask, atol=0, rtol=0)

    def testAssertImagesAlmostEqual(self):
        """Test assertImagesAlmostEqual, assertMasksEqual and assertMaskedImagesAlmostEqual
        """
        width = 10
        height = 9

        for miType in (afwImage.MaskedImageF, afwImage.MaskedImageD, afwImage.MaskedImageI,
                       afwImage.MaskedImageU):
            mi = makeRampMaskedImageWithNans(width, height, miType)
            self.checkMaskedImage(mi)

        for invalidType in (np.zeros([width+1, height]), str, self.assertRaises):
            mi = makeRampMaskedImageWithNans(width, height, miType)
            with self.assertRaises(TypeError):
                self.assertMasksEqual(mi.getMask(), invalidType)
            with self.assertRaises(TypeError):
                self.assertMasksEqual(invalidType, mi.getMask())
            with self.assertRaises(TypeError):
                self.assertMasksEqual(
                    mi.getMask(), mi.getMask(), skipMask=invalidType)

            with self.assertRaises(TypeError):
                self.assertImagesAlmostEqual(mi.getImage(), invalidType)
            with self.assertRaises(TypeError):
                self.assertImagesAlmostEqual(invalidType, mi.getImage())
            with self.assertRaises(TypeError):
                self.assertImagesAlmostEqual(
                    mi.getImage(), mi.getImage(), skipMask=invalidType)

            with self.assertRaises(TypeError):
                self.assertMaskedImagesAlmostEqual(mi, invalidType)
            with self.assertRaises(TypeError):
                self.assertMaskedImagesAlmostEqual(invalidType, mi)
            with self.assertRaises(TypeError):
                self.assertMaskedImagesAlmostEqual(
                    mi, mi, skipMask=invalidType)

            with self.assertRaises(TypeError):
                self.assertMaskedImagesAlmostEqual(
                    mi.getImage(), mi.getImage())

    def testUnsignedImages(self):
        """Unsigned images can give incorrect differences unless the test code is careful
        """
        image0 = np.zeros([5, 5], dtype=np.uint8)
        image1 = np.zeros([5, 5], dtype=np.uint8)
        image0[0, 0] = 1
        image1[0, 1] = 2

        # arrays differ by a maximum of 2
        errMsg1 = imagesDiffer(image0, image1)
        self.assertIn("maximum absolute error: |0 - 2| = 2 at position (0, 1).", errMsg1)

        # arrays are equal to within 5
        self.assertImagesAlmostEqual(image0, image1, atol=5)


def makeRampMaskedImageWithNans(width, height, imgClass=afwImage.MaskedImageF):
    """Make a masked image that is a ramp with additional non-finite values

    Make a masked image with the following additional non-finite values
    in the variance plane and (if image is of some floating type) image plane:
    - nan at [0, 0]
    - inf at [1, 0]
    - -inf at [0, 1]
    """
    mi = makeRampMaskedImage(width, height, imgClass)

    var = mi.getVariance()
    var[0, 0] = np.nan
    var[1, 0] = np.inf
    var[0, 1] = -np.inf

    im = mi.getImage()
    try:
        np.array([np.nan], dtype=im.getArray().dtype)
    except Exception:
        # image plane does not support nan, etc. (presumably an int of some
        # variety)
        pass
    else:
        # image plane does support nan, etc.
        im[0, 0] = np.nan
        im[1, 0] = np.inf
        im[0, 1] = -np.inf
    return mi


def makeRampMaskedImage(width, height, imgClass=afwImage.MaskedImageF):
    """Make a ramp image of the specified size and image class

    Image values start from 0 at the lower left corner and increase by 1 along rows
    Variance values equal image values + 100
    Mask values equal image values modulo 8 bits (leaving plenty of unused values)
    """
    mi = imgClass(width, height)
    image = mi.getImage()
    mask = mi.getMask()
    variance = mi.getVariance()
    val = 0
    for yInd in range(height):
        for xInd in range(width):
            image[xInd, yInd, afwImage.LOCAL] = val
            variance[xInd, yInd, afwImage.LOCAL] = val + 100
            mask[xInd, yInd, afwImage.LOCAL] = val % 0x100
            val += 1
    return mi


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
