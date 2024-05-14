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
Tests for Stack

Run with:
   python test_stacker.py
or
   pytest test_stacker.py
"""
import unittest
from functools import reduce

import numpy as np

import lsst.geom
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.utils.tests
import lsst.pex.exceptions as pexEx
import lsst.afw.display as afwDisplay

display = False
afwDisplay.setDefaultMaskTransparency(75)

######################################
# main body of code
######################################


class StackTestCase(lsst.utils.tests.TestCase):

    def setUp(self):
        np.random.seed(1)
        self.nImg = 10
        self.nX, self.nY = 64, 64
        self.values = [1.0, 2.0, 2.0, 3.0, 8.0]

    def testMean(self):
        """ Test the statisticsStack() function for a MEAN"""

        knownMean = 0.0
        imgList = []
        for iImg in range(self.nImg):
            imgList.append(afwImage.ImageF(
                lsst.geom.Extent2I(self.nX, self.nY), iImg))
            knownMean += iImg

        imgStack = afwMath.statisticsStack(imgList, afwMath.MEAN)
        knownMean /= self.nImg
        self.assertEqual(imgStack[self.nX//2, self.nY//2, afwImage.LOCAL], knownMean)

        # Test in-place stacking
        afwMath.statisticsStack(imgStack, imgList, afwMath.MEAN)
        self.assertEqual(imgStack[self.nX//2, self.nY//2, afwImage.LOCAL], knownMean)

    def testStatistics(self):
        """ Test the statisticsStack() function """

        imgList = []
        for val in self.values:
            imgList.append(afwImage.ImageF(
                lsst.geom.Extent2I(self.nX, self.nY), val))

        imgStack = afwMath.statisticsStack(imgList, afwMath.MEAN)
        mean = reduce(lambda x, y: x+y, self.values)/float(len(self.values))
        self.assertAlmostEqual(imgStack[self.nX//2, self.nY//2, afwImage.LOCAL], mean)

        imgStack = afwMath.statisticsStack(imgList, afwMath.MEDIAN)
        median = sorted(self.values)[len(self.values)//2]
        self.assertEqual(imgStack[self.nX//2, self.nY//2, afwImage.LOCAL], median)

    def testWeightedStack(self):
        """ Test statisticsStack() function when weighting by a variance plane"""

        sctrl = afwMath.StatisticsControl()
        sctrl.setWeighted(True)
        mimgList = []
        for val in self.values:
            mimg = afwImage.MaskedImageF(lsst.geom.Extent2I(self.nX, self.nY))
            mimg.set(val, 0x0, val)
            mimgList.append(mimg)
        mimgStack = afwMath.statisticsStack(mimgList, afwMath.MEAN, sctrl)

        wvalues = [1.0/q for q in self.values]
        wmean = float(len(self.values)) / reduce(lambda x, y: x + y, wvalues)
        self.assertAlmostEqual(
            mimgStack.image[self.nX//2, self.nY//2, afwImage.LOCAL],
            wmean)

        # Test in-place stacking
        afwMath.statisticsStack(mimgStack, mimgList, afwMath.MEAN, sctrl)
        self.assertAlmostEqual(
            mimgStack.image[self.nX//2, self.nY//2, afwImage.LOCAL],
            wmean)

    def testConstantWeightedStack(self):
        """ Test statisticsStack() function when weighting by a vector of weights"""

        sctrl = afwMath.StatisticsControl()
        imgList = []
        weights = []
        for val in self.values:
            img = afwImage.ImageF(lsst.geom.Extent2I(self.nX, self.nY), val)
            imgList.append(img)
            weights.append(val)
        imgStack = afwMath.statisticsStack(
            imgList, afwMath.MEAN, sctrl, weights)

        wsum = reduce(lambda x, y: x + y, self.values)
        wvalues = [x*x for x in self.values]
        wmean = reduce(lambda x, y: x + y, wvalues)/float(wsum)
        self.assertAlmostEqual(imgStack[self.nX//2, self.nY//2, afwImage.LOCAL], wmean)

    def testRequestMoreThanOneStat(self):
        """ Make sure we throw an exception if someone requests more than one type of statistics. """

        sctrl = afwMath.StatisticsControl()
        imgList = []
        for val in self.values:
            img = afwImage.ImageF(lsst.geom.Extent2I(self.nX, self.nY), val)
            imgList.append(img)

        def tst():
            afwMath.statisticsStack(
                imgList,
                afwMath.Property(afwMath.MEAN | afwMath.MEANCLIP),
                sctrl)

        self.assertRaises(pexEx.InvalidParameterError, tst)

    def testReturnInputs(self):
        """ Make sure that a single file put into the stacker is returned unscathed"""

        imgList = []

        img = afwImage.MaskedImageF(lsst.geom.Extent2I(10, 20))
        for y in range(img.getHeight()):
            simg = img.Factory(
                img,
                lsst.geom.Box2I(lsst.geom.Point2I(0, y),
                                lsst.geom.Extent2I(img.getWidth(), 1)),
                afwImage.LOCAL)
            simg.set(y)

        imgList.append(img)

        imgStack = afwMath.statisticsStack(imgList, afwMath.MEAN)

        if display:
            afwDisplay.Display(frame=1).mtv(img, title="input")
            afwDisplay.Display(frame=2).mtv(imgStack, title="stack")

        self.assertEqual(img[0, 0, afwImage.LOCAL][0], imgStack[0, 0, afwImage.LOCAL][0])

    def testStackBadPixels(self):
        """Check that we properly ignore masked pixels, and set noGoodPixelsMask where there are
        no good pixels"""
        mimgVec = []

        DETECTED = afwImage.Mask.getPlaneBitMask("DETECTED")
        EDGE = afwImage.Mask.getPlaneBitMask("EDGE")
        INTRP = afwImage.Mask.getPlaneBitMask("INTRP")
        SAT = afwImage.Mask.getPlaneBitMask("SAT")

        sctrl = afwMath.StatisticsControl()
        sctrl.setNanSafe(False)
        sctrl.setAndMask(INTRP | SAT)
        sctrl.setNoGoodPixelsMask(EDGE)

        # set these pixels to EDGE
        edgeBBox = lsst.geom.Box2I(lsst.geom.Point2I(0, 0),
                                   lsst.geom.Extent2I(20, 20))
        width, height = 512, 512
        dim = lsst.geom.Extent2I(width, height)
        val, maskVal = 10, DETECTED
        for i in range(4):
            mimg = afwImage.MaskedImageF(dim)
            mimg.set(val, maskVal, 1)
            #
            # Set part of the image to NaN (with the INTRP bit set)
            #
            llc = lsst.geom.Point2I(width//2*(i//2), height//2*(i % 2))
            bbox = lsst.geom.Box2I(llc, dim//2)

            smimg = mimg.Factory(mimg, bbox, afwImage.LOCAL)
            del smimg
            #
            # And the bottom corner to SAT
            #
            smask = mimg.getMask().Factory(mimg.getMask(), edgeBBox, afwImage.LOCAL)
            smask |= SAT
            del smask

            mimgVec.append(mimg)

            if display > 1:
                afwDisplay.Display(frame=i).mtv(mimg, title=str(i))

        mimgStack = afwMath.statisticsStack(mimgVec, afwMath.MEAN, sctrl)
        print("-----------------", type(mimgStack))
        if display:
            i += 1
            afwDisplay.Display(frame=i).mtv(mimgStack, title="Stack")
            i += 1
            afwDisplay.Display(frame=i).mtv(mimgStack.getVariance(), title="var(Stack)")
        #
        # Check the output, ignoring EDGE pixels
        #
        sctrl = afwMath.StatisticsControl()
        sctrl.setAndMask(afwImage.Mask.getPlaneBitMask("EDGE"))
        print("fffffffffffffffff", type(mimgStack), type(afwMath.MIN | afwMath.MAX), type(sctrl))
        stats = afwMath.makeStatistics(
            mimgStack, (afwMath.MIN | afwMath.MAX).value, sctrl)
        self.assertEqual(stats.getValue(afwMath.MIN), val)
        self.assertEqual(stats.getValue(afwMath.MAX), val)
        #
        # We have to clear EDGE in the known bad corner to check the mask
        #
        smask = mimgStack.mask[edgeBBox, afwImage.LOCAL]
        self.assertEqual(smask[edgeBBox.getMin(), afwImage.LOCAL], EDGE)
        smask &= ~EDGE
        del smask

        self.assertEqual(
            afwMath.makeStatistics(mimgStack.getMask(),
                                   afwMath.SUM, sctrl).getValue(),
            maskVal)

    def testTicket1412(self):
        """Ticket 1412: ignored mask bits are propegated to output stack."""

        mimg1 = afwImage.MaskedImageF(lsst.geom.Extent2I(1, 1))
        mimg1[0, 0, afwImage.LOCAL] = (1, 0x4, 1)  # set 0100
        mimg2 = afwImage.MaskedImageF(lsst.geom.Extent2I(1, 1))
        mimg2[0, 0, afwImage.LOCAL] = (2, 0x3, 1)  # set 0010 and 0001

        imgList = []
        imgList.append(mimg1)
        imgList.append(mimg2)

        sctrl = afwMath.StatisticsControl()
        sctrl.setAndMask(0x1)  # andmask only 0001

        # try first with no sctrl (no andmask set), should see 0x0111 for all
        # output mask pixels
        imgStack = afwMath.statisticsStack(imgList, afwMath.MEAN)
        self.assertEqual(imgStack[0, 0, afwImage.LOCAL][1], 0x7)

        # now try with sctrl (andmask = 0x0001), should see 0x0100 for all
        # output mask pixels
        imgStack = afwMath.statisticsStack(imgList, afwMath.MEAN, sctrl)
        self.assertEqual(imgStack[0, 0, afwImage.LOCAL][1], 0x4)

    def test2145(self):
        """The how-to-repeat from #2145"""
        size = 5
        statsCtrl = afwMath.StatisticsControl()
        statsCtrl.setCalcErrorFromInputVariance(True)
        maskedImageList = []
        weightList = []
        for i in range(3):
            mi = afwImage.MaskedImageF(size, size)
            mi.image.array[:] = np.random.normal(10, 0.1, (size, size))
            mi.variance.array[:] = np.random.normal(10, 0.1, (size, size))
            maskedImageList.append(mi)
            weightList.append(1.0)

        stack = afwMath.statisticsStack(
            maskedImageList, afwMath.MEAN, statsCtrl, weightList)
        self.assertNotEqual(np.sum(stack.getVariance().getArray()), 0.0)

    def testMosaicMode(self):
        """Test that mosaic mode constructs a variance plane of mean of inputs
        """
        SIZE = 5
        MEAN = 10
        VAR = 0.1

        maskedImageList = []
        weightList = []
        for i in range(3):
            mi = afwImage.MaskedImageF(SIZE, SIZE)
            mi.image.array[:] = np.random.normal(MEAN*i, VAR, (SIZE, SIZE))
            mi.variance.array[:] = np.random.normal(MEAN*i, VAR, (SIZE, SIZE))
            maskedImageList.append(mi)
            # weight each image differently
            weightList.append(float(i))

        statsCtrl = afwMath.StatisticsControl()
        statsCtrl.setCalcErrorMosaicMode(True)

        # test unweighted mean
        stack = afwMath.statisticsStack(maskedImageList, afwMath.MEAN, statsCtrl, weightList)
        self.assertAlmostEqual(np.mean(stack.variance.array), np.mean(stack.image.array), delta=0.1)

        # test weighted mean
        statsCtrl.setWeighted(True)
        stack = afwMath.statisticsStack(maskedImageList, afwMath.MEAN, statsCtrl, weightList)
        self.assertAlmostEqual(np.mean(stack.variance.array), np.mean(stack.image.array), delta=0.1)

    def testRejectedMaskPropagation(self):
        """Test that we can propagate mask bits from rejected pixels, when the amount
        of rejection crosses a threshold."""
        rejectedBit = 1        # use this bit to determine whether to reject a pixel
        propagatedBit = 2  # propagate this bit if a pixel with it set is rejected
        statsCtrl = afwMath.StatisticsControl()
        statsCtrl.setMaskPropagationThreshold(propagatedBit, 0.3)
        statsCtrl.setAndMask(1 << rejectedBit)
        statsCtrl.setWeighted(True)
        maskedImageList = []

        # start with 4 images with no mask bits set
        partialSum = np.zeros((1, 4), dtype=np.float32)
        finalImage = np.array([12.0, 12.0, 12.0, 12.0], dtype=np.float32)
        for i in range(4):
            mi = afwImage.MaskedImageF(4, 1)
            mi.image.array[:, :] = np.ones((1, 4), dtype=np.float32)
            maskedImageList.append(mi)
            partialSum += mi.image.array
        # add one more image with all permutations of the first two bits set in
        # different pixels
        mi = afwImage.MaskedImageF(4, 1)
        mi.image.array[0, :] = finalImage
        mi.mask.array[0, 1] |= (1 << rejectedBit)
        mi.mask.array[0, 2] |= (1 << propagatedBit)
        mi.mask.array[0, 3] |= (1 << rejectedBit)
        mi.mask.array[0, 3] |= (1 << propagatedBit)
        maskedImageList.append(mi)

        # these will always be rejected
        finalImage[1] = 0.0
        finalImage[3] = 0.0

        # Uniform weights: we should only see pixel 2 set with propagatedBit, because it's not rejected;
        # pixel 3 is rejected, but its weight (0.2) below the propagation
        # threshold (0.3)
        stack1 = afwMath.statisticsStack(maskedImageList, afwMath.MEAN, statsCtrl, [
                                         1.0, 1.0, 1.0, 1.0, 1.0])
        self.assertEqual(stack1[0, 0, afwImage.LOCAL][1], 0x0)
        self.assertEqual(stack1[1, 0, afwImage.LOCAL][1], 0x0)
        self.assertEqual(stack1[2, 0, afwImage.LOCAL][1], 1 << propagatedBit)
        self.assertEqual(stack1[3, 0, afwImage.LOCAL][1], 0x0)
        self.assertFloatsAlmostEqual(stack1.getImage().getArray(),
                                     (partialSum + finalImage) / np.array([5.0, 4.0, 5.0, 4.0]), rtol=1E-7)

        # Give the masked image more weight: we should see pixel 2 and pixel 3 set with propagatedBit,
        # pixel 2 because it's not rejected, and pixel 3 because the weight of the rejection (0.3333)
        # is above the threshold (0.3)
        # Note that rejectedBit is never propagated, because we didn't include it in statsCtrl (of course,
        # normally the bits we'd propagate and the bits we'd reject would be
        # the same)
        stack2 = afwMath.statisticsStack(maskedImageList, afwMath.MEAN, statsCtrl, [
                                         1.0, 1.0, 1.0, 1.0, 2.0])
        self.assertEqual(stack2[0, 0, afwImage.LOCAL][1], 0x0)
        self.assertEqual(stack2[1, 0, afwImage.LOCAL][1], 0x0)
        self.assertEqual(stack2[2, 0, afwImage.LOCAL][1], 1 << propagatedBit)
        self.assertEqual(stack2[3, 0, afwImage.LOCAL][1], 1 << propagatedBit)
        self.assertFloatsAlmostEqual(stack2.getImage().getArray(),
                                     (partialSum + 2*finalImage) / np.array([6.0, 4.0, 6.0, 4.0]), rtol=1E-7)

    def testClipped(self):
        """Test that we set mask bits when pixels are clipped"""
        box = lsst.geom.Box2I(lsst.geom.Point2I(12345, 67890), lsst.geom.Extent2I(3, 3))
        num = 10
        maskVal = 0xAD
        value = 0.0

        images = [afwImage.MaskedImageF(box) for _ in range(num)]
        statsCtrl = afwMath.StatisticsControl()
        statsCtrl.setAndMask(maskVal)
        clipped = 1 << afwImage.Mask().addMaskPlane("CLIPPED")

        # No clipping: check that vanilla is working
        for img in images:
            img.getImage().set(value)
            img.getMask().set(0)
        stack = afwMath.statisticsStack(images, afwMath.MEANCLIP, clipped=clipped)
        self.assertFloatsAlmostEqual(stack.getImage().getArray(), 0.0, atol=0.0)
        self.assertFloatsAlmostEqual(stack.getMask().getArray(), 0, atol=0.0)  # Not floats, but that's OK

        # Clip a pixel; the CLIPPED bit should be set
        images[0].getImage()[1, 1, afwImage.LOCAL] = value + 1.0
        stack = afwMath.statisticsStack(images, afwMath.MEANCLIP, clipped=clipped)
        self.assertFloatsAlmostEqual(stack.getImage().getArray(), 0.0, atol=0.0)
        self.assertEqual(stack.mask[1, 1, afwImage.LOCAL], clipped)

        # Mask a pixel; the CLIPPED bit should be set
        images[0].getMask()[1, 1, afwImage.LOCAL] = maskVal
        stack = afwMath.statisticsStack(images, afwMath.MEAN, statsCtrl, clipped=clipped)
        self.assertFloatsAlmostEqual(stack.getImage().getArray(), 0.0, atol=0.0)
        self.assertEqual(stack.mask[1, 1, afwImage.LOCAL], clipped)

        # Excuse that mask; the CLIPPED bit should not be set
        stack = afwMath.statisticsStack(images, afwMath.MEAN, statsCtrl, clipped=clipped, excuse=maskVal)
        self.assertFloatsAlmostEqual(stack.getImage().getArray(), 0.0, atol=0.0)
        self.assertEqual(stack.mask[1, 1, afwImage.LOCAL], 0)

        # Map that mask value to a different one.
        rejected = 1 << afwImage.Mask().addMaskPlane("REJECTED")
        maskMap = [(maskVal, rejected)]
        images[0].mask[1, 1, afwImage.LOCAL] = 0        # only want to clip, not mask, this one
        images[1].mask[1, 2, afwImage.LOCAL] = maskVal  # only want to mask, not clip, this one
        stack = afwMath.statisticsStack(images, afwMath.MEANCLIP, statsCtrl, wvector=[], clipped=clipped,
                                        maskMap=maskMap)
        self.assertFloatsAlmostEqual(stack.getImage().getArray(), 0.0, atol=0.0)
        self.assertEqual(stack.mask[1, 1, afwImage.LOCAL], clipped)
        self.assertEqual(stack.mask[1, 2, afwImage.LOCAL], rejected)

#################################################################
# Test suite boiler plate
#################################################################


class TestMemory(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
