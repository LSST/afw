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
Tests for MaskedImages

Run with:
   python test_maskedImage.py
or
   pytest test_maskedImage.py
"""

import itertools
import os
import unittest

import numpy as np

import lsst.utils
import lsst.utils.tests
import lsst.pex.exceptions
import lsst.daf.base
import lsst.geom
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.display as afwDisplay

try:
    type(display)
except NameError:
    display = False
afwDisplay.setDefaultMaskTransparency(75)


def makeRampImage(width, height, imgClass=afwImage.MaskedImageF):
    """Make a ramp image of the specified size and image class

    Image values start from 0 at the lower left corner and increase by 1 along rows
    Variance values equal image values + 100
    Mask values equal image values modulo 8 bits (leaving plenty of unused values)
    """
    mi = imgClass(width, height)
    image = mi.image
    mask = mi.mask
    variance = mi.variance
    val = 0
    for yInd in range(height):
        for xInd in range(width):
            image[xInd, yInd] = val
            variance[xInd, yInd] = val + 100
            mask[xInd, yInd] = val % 0x100
            val += 1
    return mi


class MaskedImageTestCase(lsst.utils.tests.TestCase):
    """A test case for MaskedImage"""

    def setUp(self):
        self.imgVal1, self.varVal1 = 100.0, 10.0
        self.imgVal2, self.varVal2 = 200.0, 15.0
        self.mimage = afwImage.MaskedImageF(100, 200)

        self.mimage.image.set(self.imgVal1)
        #
        # Set center of mask to 0, with 2 pixel border set to EDGE
        #
        self.BAD = afwImage.Mask.getPlaneBitMask("BAD")
        self.EDGE = afwImage.Mask.getPlaneBitMask("EDGE")

        self.mimage.mask.set(self.EDGE)
        centre = afwImage.Mask(
            self.mimage.mask,
            lsst.geom.Box2I(lsst.geom.Point2I(2, 2),
                            self.mimage.getDimensions() - lsst.geom.Extent2I(4)),
            afwImage.LOCAL)
        centre.set(0x0)
        #
        self.mimage.variance.set(self.varVal1)
        #
        # Second MaskedImage
        #
        self.mimage2 = afwImage.MaskedImageF(self.mimage.getDimensions())
        self.mimage2.image.set(self.imgVal2)
        self.mimage2.variance.set(self.varVal2)
        #
        # a Function2
        #
        self.function = afwMath.PolynomialFunction2D(2)
        self.function.setParameters(
            list(range(self.function.getNParameters())))

    def tearDown(self):
        del self.mimage
        del self.mimage2
        del self.function

    def testProperties(self):
        self.assertImagesEqual(self.mimage.image, self.mimage.image)
        self.assertMasksEqual(self.mimage.mask, self.mimage.mask)
        self.assertImagesEqual(self.mimage.variance, self.mimage.variance)
        image2 = self.mimage.image.Factory(self.mimage.getDimensions())
        image2.array[:] = 5.0
        self.mimage.image = image2
        self.assertImagesEqual(self.mimage.image, image2)
        mask2 = self.mimage.mask.Factory(self.mimage.getDimensions())
        mask2.array[:] = 0x4
        self.mimage.mask = mask2
        self.assertMasksEqual(self.mimage.mask, mask2)
        var2 = self.mimage.image.Factory(self.mimage.getDimensions())
        var2.array[:] = 3.0
        self.mimage.variance = var2
        self.assertImagesEqual(self.mimage.variance, var2)

    def testSetGetValues(self):
        self.assertEqual(self.mimage[0, 0, afwImage.LOCAL],
                         (self.imgVal1, self.EDGE, self.varVal1))

        self.assertEqual(self.mimage.mask[1, 1, afwImage.LOCAL], self.EDGE)
        self.assertEqual(self.mimage.mask[2, 2, afwImage.LOCAL], 0x0)

    def testImagesOverlap(self):
        # make pairs of image, variance and mask planes
        # using the same dimensions for each so we can mix and match
        # while making masked images
        dim = lsst.geom.Extent2I(10, 8)
        # a set of bounding boxes, some of which overlap each other
        # and some of which do not, and include the full image bounding box
        bboxes = (
            lsst.geom.Box2I(lsst.geom.Point2I(0, 0), dim),
            lsst.geom.Box2I(lsst.geom.Point2I(0, 0), lsst.geom.Extent2I(3, 3)),
            lsst.geom.Box2I(lsst.geom.Point2I(2, 2), lsst.geom.Extent2I(6, 4)),
            lsst.geom.Box2I(lsst.geom.Point2I(4, 4), lsst.geom.Extent2I(6, 4)),
        )
        masks = [afwImage.Mask(dim), afwImage.Mask(dim)]
        variances = [afwImage.ImageF(dim), afwImage.ImageF(dim)]
        imageClasses = (afwImage.ImageF, afwImage.ImageD, afwImage.ImageI, afwImage.ImageU)
        for ImageClass1, ImageClass2 in itertools.product(imageClasses, imageClasses):
            images = [ImageClass1(dim), ImageClass2(dim)]
            for image1, mask1, variance1, image2, mask2, variance2 in itertools.product(
                    images, masks, variances, images, masks, variances):
                with self.subTest(ImageClass1=str(ImageClass1), ImageClass2=str(ImageClass2),
                                  image1=image1, mask1=mask1, variance1=variance1,
                                  image2=image2, mask2=mask2, variance2=variance2):
                    shouldOverlap = (image1 is image2) or (mask1 is mask2) or (variance1 is variance2)

                    mi1 = afwImage.makeMaskedImage(image=image1, mask=mask1, variance=variance1)
                    mi2 = afwImage.makeMaskedImage(image=image2, mask=mask2, variance=variance2)
                    self.assertEqual(afwImage.imagesOverlap(mi1, mi2), shouldOverlap)
                    self.assertEqual(afwImage.imagesOverlap(mi2, mi1), shouldOverlap)

                    for bbox1, bbox2 in itertools.product(bboxes, bboxes):
                        with self.subTest(bbox1=bbox1, bbox2=bbox2):
                            subMi1 = afwImage.makeMaskedImage(image=type(image1)(image1, bbox1),
                                                              mask=afwImage.Mask(mask1, bbox1),
                                                              variance=afwImage.ImageF(variance1, bbox1))
                            subMi2 = afwImage.makeMaskedImage(image=type(image2)(image2, bbox2),
                                                              mask=afwImage.Mask(mask2, bbox2),
                                                              variance=afwImage.ImageF(variance2, bbox2))
                            subregionsShouldOverlap = shouldOverlap and bbox1.overlaps(bbox2)
                            self.assertEqual(afwImage.imagesOverlap(subMi1, subMi2), subregionsShouldOverlap)
                            self.assertEqual(afwImage.imagesOverlap(subMi2, subMi1), subregionsShouldOverlap)

    def testMaskedImageFromImage(self):
        w, h = 10, 20
        dims = lsst.geom.Extent2I(w, h)
        im, mask, var = afwImage.ImageF(dims), \
            afwImage.Mask(dims), \
            afwImage.ImageF(dims)
        im.set(666)

        maskedImage = afwImage.MaskedImageF(im, mask, var)

        maskedImage = afwImage.makeMaskedImage(im, mask, var)

        maskedImage = afwImage.MaskedImageF(im)
        self.assertEqual(im.getDimensions(),
                         maskedImage.image.getDimensions())
        self.assertEqual(im.getDimensions(),
                         maskedImage.mask.getDimensions())
        self.assertEqual(im.getDimensions(),
                         maskedImage.variance.getDimensions())

        self.assertEqual(maskedImage[0, 0, afwImage.LOCAL], (im[0, 0, afwImage.LOCAL], 0x0, 0.0))

    def testMakeMaskedImageXY0(self):
        """Test that makeMaskedImage sets XY0 correctly"""
        im = afwImage.ImageF(200, 300)
        xy0 = lsst.geom.PointI(10, 20)
        im.setXY0(*xy0)
        mi = afwImage.makeMaskedImage(im)

        self.assertEqual(mi.image.getXY0(), xy0)
        self.assertEqual(mi.mask.getXY0(), xy0)
        self.assertEqual(mi.variance.getXY0(), xy0)

    def testCopyMaskedImage(self):
        """Test copy constructor"""
        #
        # shallow copy
        #
        mi = self.mimage.Factory(self.mimage, False)

        val00 = self.mimage[0, 0, afwImage.LOCAL]
        nval00 = (100, 0xff, -1)        # the new value we'll set
        self.assertNotEqual(val00, nval00)

        self.assertEqual(mi[0, 0, afwImage.LOCAL], val00)
        mi[0, 0, afwImage.LOCAL] = nval00

        self.assertEqual(self.mimage[0, 0, afwImage.LOCAL], nval00)
        self.assertEqual(mi[0, 0, afwImage.LOCAL], nval00)
        mi[0, 0, afwImage.LOCAL] = val00             # reinstate initial value
        #
        # deep copy
        #
        mi = self.mimage.Factory(self.mimage, True)

        self.assertEqual(mi[0, 0, afwImage.LOCAL], val00)
        mi[0, 0, afwImage.LOCAL] = nval00

        self.assertEqual(self.mimage[0, 0, afwImage.LOCAL], val00)
        self.assertEqual(mi[0, 0, afwImage.LOCAL], nval00)
        #
        # Copy with change of Image type
        #
        mi = self.mimage.convertD()

        self.assertEqual(mi[0, 0, afwImage.LOCAL], val00)
        mi[0, 0, afwImage.LOCAL] = nval00

        self.assertEqual(self.mimage[0, 0, afwImage.LOCAL], val00)
        self.assertEqual(mi[0, 0, afwImage.LOCAL], nval00)
        #
        # Convert from U to F
        #
        mi = afwImage.MaskedImageU(lsst.geom.Extent2I(10, 20))
        val00 = (10, 0x10, 1)
        mi.set(val00)
        self.assertEqual(mi[0, 0, afwImage.LOCAL], val00)

        fmi = mi.convertF()
        self.assertEqual(fmi[0, 0, afwImage.LOCAL], val00)

    def testAddImages(self):
        "Test addition"
        # add an image
        self.mimage2 += self.mimage

        self.assertEqual(self.mimage2[0, 0, afwImage.LOCAL], (self.imgVal1 + self.imgVal2, self.EDGE,
                                                              self.varVal1 + self.varVal2))

        # Add an Image<int> to a MaskedImage<int>
        mimage_i = afwImage.MaskedImageI(self.mimage2.getDimensions())
        mimage_i.set(900, 0x0, 1000.0)
        image_i = afwImage.ImageI(mimage_i.getDimensions(), 2)

        mimage_i += image_i

        self.assertEqual(mimage_i[0, 0, afwImage.LOCAL], (902, 0x0, 1000.0))

        # add a scalar
        self.mimage += self.imgVal1

        self.assertEqual(self.mimage[0, 0, afwImage.LOCAL],
                         (2*self.imgVal1, self.EDGE, self.varVal1))

        self.assertEqual(self.mimage.mask[1, 1, afwImage.LOCAL], self.EDGE)
        self.assertEqual(self.mimage.mask[2, 2, afwImage.LOCAL], 0x0)

        # add a function
        self.mimage.set(self.imgVal1, 0x0, 0.0)
        self.mimage += self.function

        for i, j in [(2, 3)]:
            self.assertEqual(self.mimage.image[i, j, afwImage.LOCAL],
                             self.imgVal1 + self.function(i, j))

    def testAddScaledImages(self):
        "Test addition by a scaled MaskedImage"
        # add an image
        c = 10.0
        mimage2_copy = self.mimage2.Factory(self.mimage2, True)  # make a copy
        self.mimage2.scaledPlus(c, self.mimage)
        #
        # Now repeat calculation using a temporary
        #
        tmp = self.mimage.Factory(self.mimage, True)
        tmp *= c
        mimage2_copy += tmp

        self.assertEqual(self.mimage2[0, 0, afwImage.LOCAL], mimage2_copy[0, 0, afwImage.LOCAL])

    def testAssignWithBBox(self):
        """Test assign(rhs, bbox) with non-empty bbox
        """
        for xy0 in (lsst.geom.Point2I(*val) for val in (
            (0, 0),
            (-100, 120),  # an arbitrary value that is off the image
        )):
            destMIDim = lsst.geom.Extent2I(5, 4)
            srcMIDim = lsst.geom.Extent2I(3, 2)
            destMI = afwImage.MaskedImageF(destMIDim)
            destImage = destMI.image
            destVariance = destMI.variance
            destMask = destMI.mask
            destMI.setXY0(xy0)
            srcMI = makeRampImage(*srcMIDim)
            srcMI.setXY0(55, -33)  # an arbitrary value that should be ignored
            self.assertRaises(Exception, destMI.set, srcMI)  # size mismatch

            for validMin in (lsst.geom.Point2I(*val) for val in (
                (0, 0),
                (2, 0),
                (0, 1),
                (1, 2),
            )):
                # None to omit the argument
                for origin in (None, afwImage.PARENT, afwImage.LOCAL):
                    destImage[:] = -1.0
                    destVariance[:] = -1.0
                    destMask[:] = 0xFFFF
                    bbox = lsst.geom.Box2I(validMin, srcMI.getDimensions())
                    if origin != afwImage.LOCAL:
                        bbox.shift(lsst.geom.Extent2I(xy0))
                    if origin is None:
                        destMI.assign(srcMI, bbox)
                        destMIView = afwImage.MaskedImageF(destMI, bbox)
                    else:
                        destMI.assign(srcMI, bbox, origin)
                        destMIView = afwImage.MaskedImageF(destMI, bbox, origin)
                    self.assertMaskedImagesEqual(destMIView, srcMI)
                    numPixNotAssigned = (
                        destMIDim[0] * destMIDim[1]) - (srcMIDim[0] * srcMIDim[1])
                    self.assertEqual(
                        np.sum(destImage.getArray() < -0.5), numPixNotAssigned)
                    self.assertEqual(
                        np.sum(destVariance.getArray() < -0.5), numPixNotAssigned)
                    self.assertEqual(
                        np.sum(destMask.getArray() == 0xFFFF), numPixNotAssigned)

            for badMin in (lsst.geom.Point2I(*val) + lsst.geom.Extent2I(xy0) for val in (
                (-1, 0),
                (3, 0),
                (0, -1),
                (1, 3),
            )):
                # None to omit the argument
                for origin in (None, afwImage.PARENT, afwImage.LOCAL):
                    bbox = lsst.geom.Box2I(validMin, srcMI.getDimensions())
                    if origin != afwImage.LOCAL:
                        bbox.shift(lsst.geom.Extent2I(xy0))
                    if origin is None:
                        self.assertRaises(Exception, destMI.set, srcMI, bbox)
                    else:
                        self.assertRaises(
                            Exception, destMI.set, srcMI, bbox, origin)

    def testAssignWithoutBBox(self):
        """Test assign(rhs, [bbox]) with an empty bbox and with no bbox specified; both set all pixels
        """
        for xy0 in (lsst.geom.Point2I(*val) for val in (
            (0, 0),
            (-100, 120),  # an arbitrary value that is off the image
        )):
            destMIDim = lsst.geom.Extent2I(5, 4)
            destMI = afwImage.MaskedImageF(destMIDim)
            destMI.setXY0(xy0)
            destImage = destMI.image
            destVariance = destMI.variance
            destMask = destMI.mask
            srcMI = makeRampImage(*destMIDim)
            srcMI.setXY0(55, -33)  # an arbitrary value that should be ignored

            destImage[:] = -1.0
            destVariance[:] = -1.0
            destMask[:] = 0xFFFF
            destMI.assign(srcMI)
            self.assertMaskedImagesEqual(destMI, srcMI)

            destImage[:] = -1.0
            destVariance[:] = -1.0
            destMask[:] = 0xFFFF
            destMI.assign(srcMI, lsst.geom.Box2I())
            self.assertMaskedImagesEqual(destMI, srcMI)

    def testSubtractImages(self):
        "Test subtraction"
        # subtract an image
        self.mimage2 -= self.mimage
        self.assertEqual(self.mimage2[0, 0, afwImage.LOCAL],
                         (self.imgVal2 - self.imgVal1, self.EDGE, self.varVal2 + self.varVal1))

        # Subtract an Image<int> from a MaskedImage<int>
        mimage_i = afwImage.MaskedImageI(self.mimage2.getDimensions())
        mimage_i.set(900, 0x0, 1000.0)
        image_i = afwImage.ImageI(mimage_i.getDimensions(), 2)

        mimage_i -= image_i

        self.assertEqual(mimage_i[0, 0, afwImage.LOCAL], (898, 0x0, 1000.0))

        # subtract a scalar
        self.mimage -= self.imgVal1
        self.assertEqual(self.mimage[0, 0, afwImage.LOCAL], (0.0, self.EDGE, self.varVal1))

    def testSubtractScaledImages(self):
        "Test subtraction by a scaled MaskedImage"
        # subtract a scaled image
        c = 10.0
        mimage2_copy = self.mimage2.Factory(self.mimage2, True)  # make a copy
        self.mimage2.scaledMinus(c, self.mimage)
        #
        # Now repeat calculation using a temporary
        #
        tmp = self.mimage.Factory(self.mimage, True)
        tmp *= c
        mimage2_copy -= tmp

        self.assertEqual(self.mimage2[0, 0, afwImage.LOCAL], mimage2_copy[0, 0, afwImage.LOCAL])

    def testArithmeticImagesMismatch(self):
        "Test arithmetic operations on MaskedImages of different sizes"
        i1 = afwImage.MaskedImageF(lsst.geom.Extent2I(100, 100))
        i1.set(100)
        i2 = afwImage.MaskedImageF(lsst.geom.Extent2I(10, 10))
        i2.set(10)

        def tst1(i1, i2):
            i1 -= i2

        def tst2(i1, i2):
            i1.scaledMinus(1.0, i2)

        def tst3(i1, i2):
            i1 += i2

        def tst4(i1, i2):
            i1.scaledPlus(1.0, i2)

        def tst5(i1, i2):
            i1 *= i2

        def tst6(i1, i2):
            i1.scaledMultiplies(1.0, i2)

        def tst7(i1, i2):
            i1 /= i2

        def tst8(i1, i2):
            i1.scaledDivides(1.0, i2)

        tsts12 = [tst1, tst3, tst5, tst7]
        for tst in tsts12:
            self.assertRaises(lsst.pex.exceptions.LengthError, tst, i1, i2)

        tsts21 = [tst2, tst4, tst6, tst8]
        for tst in tsts21:
            self.assertRaises(lsst.pex.exceptions.LengthError, tst, i2, i1)

    def testMultiplyImages(self):
        """Test multiplication"""
        # Multiply by a MaskedImage
        self.mimage2 *= self.mimage

        self.assertEqual(self.mimage2[0, 0, afwImage.LOCAL],
                         (self.imgVal2*self.imgVal1, self.EDGE,
                          self.varVal2*pow(self.imgVal1, 2) + self.varVal1*pow(self.imgVal2, 2)))

        # Divide a MaskedImage<int> by an Image<int>; this divides the variance Image<float>
        # by an Image<int> in C++
        mimage_i = afwImage.MaskedImageI(self.mimage2.getDimensions())
        mimage_i.set(900, 0x0, 1000.0)
        image_i = afwImage.ImageI(mimage_i.getDimensions(), 2)

        mimage_i *= image_i

        self.assertEqual(mimage_i[0, 0, afwImage.LOCAL], (1800, 0x0, 4000.0))

        # multiply by a scalar
        self.mimage *= self.imgVal1

        self.assertEqual(self.mimage[0, 0, afwImage.LOCAL],
                         (self.imgVal1*self.imgVal1, self.EDGE, self.varVal1*pow(self.imgVal1, 2)))

        self.assertEqual(self.mimage.mask[1, 1, afwImage.LOCAL], self.EDGE)
        self.assertEqual(self.mimage.mask[2, 2, afwImage.LOCAL], 0x0)

    def testScaledMultiplyImages(self):
        """Test multiplication by a scaled image"""
        # Multiply by an image
        c = 10.0
        mimage2_copy = self.mimage2.Factory(self.mimage2, True)  # make a copy
        self.mimage2.scaledMultiplies(c, self.mimage)
        #
        # Now repeat calculation using a temporary
        #
        tmp = self.mimage.Factory(self.mimage, True)
        tmp *= c
        mimage2_copy *= tmp

        self.assertEqual(self.mimage2[0, 0, afwImage.LOCAL], mimage2_copy[0, 0, afwImage.LOCAL])

    def testDivideImages(self):
        """Test division"""
        # Divide by a MaskedImage
        mimage2_copy = self.mimage2.Factory(self.mimage2, True)  # make a copy
        mimage2_copy /= self.mimage

        self.assertEqual(mimage2_copy.image[0, 0, afwImage.LOCAL],
                         self.imgVal2/self.imgVal1)
        self.assertEqual(mimage2_copy.mask[0, 0, afwImage.LOCAL], self.EDGE)
        self.assertAlmostEqual(mimage2_copy.variance[0, 0, afwImage.LOCAL],
                               (self.varVal2*pow(self.imgVal1, 2)
                                + self.varVal1*pow(self.imgVal2, 2))/pow(self.imgVal1, 4), 10)
        # Divide by an Image (of the same type as MaskedImage.image)
        mimage = self.mimage2.Factory(self.mimage2, True)
        mimage /= mimage.image

        self.assertEqual(mimage[0, 0, afwImage.LOCAL], (self.imgVal2 / self.imgVal2, 0x0, self.varVal2))

        # Divide by an Image (of a different type from MaskedImage.image)
        # this isn't supported from python (it's OK in C++)
        if False:
            mimage = self.mimage2.Factory(self.mimage2, True)
            image = afwImage.ImageI(mimage.getDimensions(), 1)
            mimage /= image

            self.assertEqual(mimage[0, 0, afwImage.LOCAL],
                             (self.imgVal2, 0x0, self.varVal2))

        # Divide a MaskedImage<int> by an Image<int>; this divides the variance Image<float>
        # by an Image<int> in C++
        mimage_i = afwImage.MaskedImageI(self.mimage2.getDimensions())
        mimage_i.set(900, 0x0, 1000.0)
        image_i = afwImage.ImageI(mimage_i.getDimensions(), 2)

        mimage_i /= image_i

        self.assertEqual(mimage_i[0, 0, afwImage.LOCAL], (450, 0x0, 250.0))

        # divide by a scalar
        self.mimage /= self.imgVal1

        self.assertEqual(self.mimage.image[0, 0, afwImage.LOCAL],
                         self.imgVal1/self.imgVal1)
        self.assertEqual(self.mimage.mask[0, 0, afwImage.LOCAL], self.EDGE)
        self.assertAlmostEqual(self.mimage.variance[0, 0, afwImage.LOCAL],
                               self.varVal1/pow(self.imgVal1, 2), 9)

        self.assertEqual(self.mimage.mask[1, 1, afwImage.LOCAL], self.EDGE)
        self.assertEqual(self.mimage.mask[2, 2, afwImage.LOCAL], 0x0)

    def testScaledDivideImages(self):
        """Test division by a scaled image"""
        # Divide by an image
        c = 10.0
        mimage2_copy = self.mimage2.Factory(self.mimage2, True)  # make a copy
        self.mimage2.scaledDivides(c, self.mimage)
        #
        # Now repeat calculation using a temporary
        #
        tmp = self.mimage.Factory(self.mimage, True)
        tmp *= c
        mimage2_copy /= tmp

        self.assertEqual(self.mimage2[0, 0, afwImage.LOCAL], mimage2_copy[0, 0, afwImage.LOCAL])

    def testCopyConstructors(self):
        dimage = afwImage.MaskedImageF(self.mimage, True)  # deep copy
        simage = afwImage.MaskedImageF(self.mimage)  # shallow copy

        self.mimage += 2                # should only change dimage
        self.assertEqual(dimage.image[0, 0, afwImage.LOCAL], self.imgVal1)
        self.assertEqual(simage.image[0, 0, afwImage.LOCAL], self.imgVal1 + 2)

    def checkImgPatch12(self, img, x0, y0):
        """Check that a patch of an image is correct; origin of patch is at (x0, y0) in full image
        N.b. This isn't a general routine!  Works only for testSubimages[12]"""

        self.assertEqual(img[x0 - 1, y0 - 1, afwImage.LOCAL],
                         (self.imgVal1, self.EDGE, self.varVal1))
        self.assertEqual(img[x0, y0, afwImage.LOCAL], (666, self.BAD, 0))
        self.assertEqual(img[x0 + 3, y0, afwImage.LOCAL],
                         (self.imgVal1, 0x0, self.varVal1))
        self.assertEqual(img[x0, y0 + 1, afwImage.LOCAL], (666, self.BAD, 0))
        self.assertEqual(img[x0 + 3, y0 + 1, afwImage.LOCAL],
                         (self.imgVal1, 0x0, self.varVal1))
        self.assertEqual(img[x0, y0 + 2, afwImage.LOCAL],
                         (self.imgVal1, 0x0, self.varVal1))

    def testOrigin(self):
        """Check that we can set and read the origin"""

        im = afwImage.MaskedImageF(lsst.geom.ExtentI(10, 20))
        x0 = y0 = 0

        self.assertEqual(im.getX0(), x0)
        self.assertEqual(im.getY0(), y0)
        self.assertEqual(im.getXY0(), lsst.geom.PointI(x0, y0))

        x0, y0 = 3, 5
        im.setXY0(x0, y0)
        self.assertEqual(im.getX0(), x0)
        self.assertEqual(im.getY0(), y0)
        self.assertEqual(im.getXY0(), lsst.geom.PointI(x0, y0))

        x0, y0 = 30, 50
        im.setXY0(lsst.geom.Point2I(x0, y0))
        self.assertEqual(im.getX0(), x0)
        self.assertEqual(im.getY0(), y0)
        self.assertEqual(im.getXY0(), lsst.geom.Point2I(x0, y0))

    def testSubimages1(self):
        smimage = afwImage.MaskedImageF(
            self.mimage,
            lsst.geom.Box2I(lsst.geom.Point2I(1, 1), lsst.geom.Extent2I(10, 5)),
            afwImage.LOCAL
        )

        simage = afwImage.MaskedImageF(
            smimage,
            lsst.geom.Box2I(lsst.geom.Point2I(1, 1), lsst.geom.Extent2I(3, 2)),
            afwImage.LOCAL
        )
        self.assertEqual(simage.getX0(), 2)
        self.assertEqual(simage.getY0(), 2)  # i.e. wrt self.mimage

        mimage2 = afwImage.MaskedImageF(simage.getDimensions())
        mimage2.image.set(666)
        mimage2.mask.set(self.BAD)
        simage[:] = mimage2

        del simage
        del mimage2

        self.checkImgPatch12(self.mimage, 2, 2)
        self.checkImgPatch12(smimage, 1, 1)

    def testSubimages2(self):
        """Test subimages when we've played with the (x0, y0) value"""

        self.mimage[9, 4, afwImage.LOCAL] = (888, 0x0, 0)

        smimage = afwImage.MaskedImageF(
            self.mimage,
            lsst.geom.Box2I(lsst.geom.Point2I(1, 1), lsst.geom.Extent2I(10, 5)),
            afwImage.LOCAL
        )
        # reset origin; doesn't affect pixel coordinate systems
        smimage.setXY0(lsst.geom.Point2I(0, 0))

        simage = afwImage.MaskedImageF(
            smimage, lsst.geom.Box2I(lsst.geom.Point2I(1, 1),
                                     lsst.geom.Extent2I(3, 2)),
            afwImage.LOCAL
        )
        self.assertEqual(simage.getX0(), 1)
        self.assertEqual(simage.getY0(), 1)

        mimage2 = afwImage.MaskedImageF(simage.getDimensions())
        mimage2.set(666, self.BAD, 0.0)
        simage[:] = mimage2
        del simage
        del mimage2

        self.checkImgPatch12(self.mimage, 2, 2)
        self.checkImgPatch12(smimage, 1, 1)

    def checkImgPatch3(self, img, deep):
        """Check that a patch of an image is correct; origin of patch is at (x0, y0) in full image
        N.b. This isn't a general routine!  Works only for testSubimages3"""

        # Include deep in comparison so we can see which test fails
        self.assertEqual(img[0, 0, afwImage.LOCAL] + (deep, ),
                         (100, 0x0, self.varVal1, deep))
        self.assertEqual(img[10, 10, afwImage.LOCAL] + (deep, ),
                         (200, 0xf, self.varVal1, deep))

    def testSubimages3(self):
        """Test subimages when we've played with the (x0, y0) value"""

        self.mimage.image[20, 20, afwImage.LOCAL] = 200
        self.mimage.mask[20, 20, afwImage.LOCAL] = 0xf

        for deep in (True, False):
            mimage = self.mimage.Factory(
                self.mimage,
                lsst.geom.Box2I(lsst.geom.Point2I(10, 10),
                                lsst.geom.Extent2I(64, 64)),
                afwImage.LOCAL,
                deep)
            mimage.setXY0(lsst.geom.Point2I(0, 0))
            mimage2 = mimage.Factory(mimage)

            if display:
                afwDisplay.Display(frame=0).mtv(mimage2, title="testSubimages3")

            self.checkImgPatch3(mimage2, deep)

    def testSetCopiedMask(self):
        """Check that we can set the Mask with a copied Mask"""

        crMask = self.mimage.mask.Factory(self.mimage.mask, True)
        msk = self.mimage.mask
        msk |= crMask
        del msk

    def testVariance(self):
        """Check that we can set the variance from the gain"""
        gain = 2

        var = self.mimage.variance
        var[:] = self.mimage.image
        var /= gain

    def testTicket653(self):
        """How-to-repeat for #653"""
        # The original ticket read this file, but it doesn't reproduce for me,
        # As I don't see how reading an exposure from disk could make a difference
        # it's easier to just build an Image
        if False:
            im = afwImage.ImageF(os.path.join(
                lsst.utils.getPackageDir("afwdata"), "med_img.fits"))
        else:
            im = afwImage.ImageF(lsst.geom.Extent2I(10, 10))
        mi = afwImage.MaskedImageF(im)
        afwImage.ExposureF(mi)

    def testTicket41478(self):
        """Test for DM-41478 fix"""
        masked_image = afwImage.MaskedImageF(afwImage.ImageF())
        image = afwImage.ImageF()
        self.assertEqual(masked_image.getBBox(), image.getBBox())

    def testMaskedImageInitialisation(self):
        dims = self.mimage.getDimensions()
        factory = self.mimage.Factory

        self.mimage.set(666)

        del self.mimage                 # tempt C++ to reuse the memory
        self.mimage = factory(dims)
        self.assertEqual(self.mimage[10, 10, afwImage.LOCAL], (0, 0x0, 0))

        del self.mimage
        self.mimage = factory(lsst.geom.Extent2I(20, 20))
        self.assertEqual(self.mimage[10, 10, afwImage.LOCAL], (0, 0x0, 0))

    def testImageSlices(self):
        """Test image slicing, which generate sub-images using Box2I under the covers"""
        im = afwImage.MaskedImageF(10, 20)
        im[4, 10] = (10, 0x2, 100)
        im[-3:, -2:, afwImage.LOCAL] = 100
        sim = im[1:4, 6:10]
        nan = -666  # a real NaN != NaN so tests fail
        sim[:] = (-1, 0x8, nan)
        im[0:4, 0:4] = im[2:6, 8:12]

        if display:
            afwDisplay.Display(frame=1).mtv(im, title="testImageSlices")

        self.assertEqual(im[0, 6, afwImage.LOCAL], (0, 0x0, 0))
        self.assertEqual(im[6, 17, afwImage.LOCAL], (0, 0x0, 0))
        self.assertEqual(im[7, 18, afwImage.LOCAL], (100, 0x0, 0))
        self.assertEqual(im[9, 19, afwImage.LOCAL], (100, 0x0, 0))
        self.assertEqual(im[1, 6, afwImage.LOCAL], (-1, 0x8, nan))
        self.assertEqual(im[3, 9, afwImage.LOCAL], (-1, 0x8, nan))
        self.assertEqual(im[4, 10, afwImage.LOCAL], (10, 0x2, 100))
        self.assertEqual(im[4, 9, afwImage.LOCAL], (0, 0x0, 0))
        self.assertEqual(im[2, 2, afwImage.LOCAL], (10, 0x2, 100))
        self.assertEqual(im[0, 0, afwImage.LOCAL], (-1, 0x8, nan))

    def testConversionToScalar(self):
        """Test that even 1-pixel MaskedImages can't be converted to scalars"""
        im = afwImage.MaskedImageF(10, 20)

        # only single pixel images may be converted
        self.assertRaises(TypeError, float, im)
        # actually, can't convert (img, msk, var) to scalar
        self.assertRaises(TypeError, float, im[0, 0])

    def testString(self):
        image = afwImage.MaskedImageF(100, 100)
        self.assertIn("image=", str(image))
        self.assertIn("mask=", str(image))
        self.assertIn("variance=", str(image))
        self.assertIn(str(np.zeros((100, 100), dtype=image.image.dtype)), str(image))
        self.assertIn(str(np.zeros((100, 100), dtype=image.mask.dtype)), str(image))
        self.assertIn(str(np.zeros((100, 100), dtype=image.variance.dtype)), str(image))
        self.assertIn("bbox=%s"%str(image.getBBox()), str(image))
        self.assertIn("maskPlaneDict=%s"%str(image.mask.getMaskPlaneDict()), str(image))

        self.assertIn("MaskedImageF=(", repr(image))


def printImg(img):
    print("%4s " % "", end=' ')
    for c in range(img.getWidth()):
        print("%7d" % c, end=' ')
    print()

    for r in range(img.getHeight() - 1, -1, -1):
        print("%4d " % r, end=' ')
        for c in range(img.getWidth()):
            print("%7.1f" % float(img[c, r, afwImage.LOCAL]), end=' ')
        print()


class TestMemory(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
