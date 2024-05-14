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

import math
import os.path
import unittest
import pickle
from functools import reduce

import numpy as np

import lsst.utils
import lsst.utils.tests
import lsst.pex.exceptions
from lsst.daf.base import PropertySet
import lsst.geom
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.display as afwDisplay

# Set to True to display debug messages and images
debugMode = False

afwDisplay.setDefaultMaskTransparency(75)
try:
    AfwdataDir = lsst.utils.getPackageDir("afwdata")
except LookupError:
    AfwdataDir = None


class BackgroundTestCase(lsst.utils.tests.TestCase):

    def setUp(self):
        np.random.seed(1)
        self.val = 10
        self.image = afwImage.ImageF(lsst.geom.Box2I(
            lsst.geom.Point2I(1000, 500), lsst.geom.Extent2I(100, 200)))
        self.image.set(self.val)

    def tearDown(self):
        del self.image

    def testOddSize(self):
        """Test for ticket #1781 -- without it, in oddly-sized images
        there is a chunk of pixels on the right/bottom that do not go
        into the fit and are extrapolated.  After this ticket, the
        subimage boundaries are spread more evenly so the last pixels
        get fit as well.  This slightly strange test case checks that
        the interpolant is close to the function at the end.  I could
        not think of an interpolant that would fit exactly, so this
        just puts a limit on the errors.
        """
        W, H = 2, 99
        image = afwImage.ImageF(lsst.geom.Extent2I(W, H))
        bgCtrl = afwMath.BackgroundControl(afwMath.Interpolate.LINEAR)
        bgCtrl.setNxSample(2)
        NY = 10
        bgCtrl.setNySample(NY)
        for y in range(H):
            for x in range(W):
                B = 89
                if y < B:
                    image[x, y, afwImage.LOCAL] = y
                else:
                    image[x, y, afwImage.LOCAL] = B + (y-B)*-1.
        bobj = afwMath.makeBackground(image, bgCtrl)
        back = bobj.getImageF()

        for iy, by in zip([image[0, y, afwImage.LOCAL] for y in range(H)],
                          [back[0, y, afwImage.LOCAL] for y in range(H)]):
            self.assertLess(abs(iy - by), 5)

    @unittest.skipIf(AfwdataDir is None, "afwdata not setup")
    def testBackgroundTestImages(self):
        """Tests Laher's afwdata/Statistics/*.fits images (doubles)"""
        imginfolist = []
        # cooked to known value
        imginfolist.append(["v1_i1_g_m400_s20_f.fits", 399.9912966583894])

        for imginfo in imginfolist:
            imgfile, centerValue = imginfo
            imgPath = os.path.join(AfwdataDir, "Statistics", imgfile)

            # get the image and header
            dimg = afwImage.DecoratedImageF(imgPath)
            img = dimg.getImage()
            fitsHdr = dimg.getMetadata()  # the FITS header

            # get the True values of the mean and stdev
            reqMean = fitsHdr.getAsDouble("MEANREQ")
            reqStdev = fitsHdr.getAsDouble("SIGREQ")
            naxis1 = img.getWidth()
            naxis2 = img.getHeight()

            # create a background control object
            bctrl = afwMath.BackgroundControl(afwMath.Interpolate.AKIMA_SPLINE)
            bctrl.setNxSample(5)
            bctrl.setNySample(5)

            # run the background constructor and call the getImage() function.
            backobj = afwMath.makeBackground(img, bctrl)

            pixPerSubimage = img.getWidth()*img.getHeight() / \
                (bctrl.getNxSample()*bctrl.getNySample())
            stdevInterp = reqStdev/math.sqrt(pixPerSubimage)

            # test getImage() by checking the center pixel
            bimg = backobj.getImageF()
            testImgval = bimg[naxis1//2, naxis2//2, afwImage.LOCAL]
            self.assertLess(abs(testImgval - reqMean), 2*stdevInterp)

    def testRamp(self):
        """tests Laher's afwdata/Statistics/*.fits images (doubles)"""
        # make a ramping image (spline should be exact for linear increasing
        # image
        nx = 512
        ny = 512
        x0, y0 = 9876, 54321
        box = lsst.geom.Box2I(lsst.geom.Point2I(x0, y0), lsst.geom.Extent2I(nx, ny))
        rampimg = afwImage.ImageF(box)
        dzdx, dzdy, z0 = 0.1, 0.2, 10000.0
        for x in range(nx):
            for y in range(ny):
                rampimg[x, y, afwImage.LOCAL] = dzdx*x + dzdy*y + z0

        # check corner, edge, and center pixels
        bctrl = afwMath.BackgroundControl(10, 10)
        bctrl.setInterpStyle(afwMath.Interpolate.CUBIC_SPLINE)
        bctrl.setNxSample(6)
        bctrl.setNySample(6)
        # large enough to entirely avoid clipping
        bctrl.getStatisticsControl().setNumSigmaClip(20.0)
        bctrl.getStatisticsControl().setNumIter(1)
        backobj = afwMath.makeBackground(rampimg, bctrl)

        if debugMode:
            print(rampimg.getArray())

        frame = 1
        for interp in ("CONSTANT", "LINEAR", "NATURAL_SPLINE", "AKIMA_SPLINE"):
            diff = backobj.getImageF(interp)
            if debugMode:
                afwDisplay.Display(frame=frame).mtv(diff, title=self._testMethodName + " diff")
                frame += 1
            diff -= rampimg
            if debugMode:
                print(interp, diff.getArray().mean(), diff.getArray().std())
            if debugMode:
                afwDisplay.Display(frame=frame).mtv(diff, title=self._testMethodName + " diff-ramping")
                frame += 1
        if debugMode:
            afwDisplay.Display(frame=frame).mtv(rampimg, title=self._testMethodName + " ramping")
            frame += 1
            afwDisplay.Display(frame=frame).mtv(backobj.getStatsImage(),
                                                title=self._testMethodName + " bkgd StatsImage")
            frame += 1

        xpixels = [0, nx//2, nx - 1]
        ypixels = [0, ny//2, ny - 1]
        for xpix in xpixels:
            for ypix in ypixels:
                testval = backobj.getImageF()[xpix, ypix, afwImage.LOCAL]
                self.assertAlmostEqual(testval/rampimg[xpix, ypix, afwImage.LOCAL], 1, 6)

        # Test pickle
        new = pickle.loads(pickle.dumps(backobj))
        self.assertBackgroundEqual(backobj, new)

        # Check creation of sub-image
        box = lsst.geom.Box2I(lsst.geom.Point2I(123, 45),
                              lsst.geom.Extent2I(45, 123))
        box.shift(lsst.geom.Extent2I(x0, y0))
        bgImage = backobj.getImageF("AKIMA_SPLINE")
        bgSubImage = afwImage.ImageF(bgImage, box)
        testImage = backobj.getImageF(box, "AKIMA_SPLINE")
        self.assertEqual(testImage.getXY0(), bgSubImage.getXY0())
        self.assertEqual(testImage.getDimensions(), bgSubImage.getDimensions())
        self.assertImagesEqual(testImage, bgSubImage)

    def getParabolaImage(self, nx, ny, pars=(1.0e-4, 1.0e-4, 0.1, 0.2, 10.0)):
        """Make sure a quadratic map is *well* reproduced by the spline model
        """
        parabimg = afwImage.ImageF(lsst.geom.Extent2I(nx, ny))
        d2zdx2, d2zdy2, dzdx, dzdy, z0 = pars  # no cross-terms
        x, y = np.meshgrid(np.arange(nx, dtype=int), np.arange(ny, dtype=int))
        parabimg.array[:, :] = d2zdx2*x**2 + d2zdy2*y**2 + dzdx*x + dzdy*y + z0
        return parabimg

    @unittest.skipIf(AfwdataDir is None, "afwdata not setup")
    def testTicket987(self):
        """This code used to abort; so the test is that it doesn't"""
        imagePath = os.path.join(
            AfwdataDir, "DC3a-Sim", "sci", "v5-e0", "v5-e0-c011-a00.sci.fits")
        mimg = afwImage.MaskedImageF(imagePath)
        binsize = 512
        bctrl = afwMath.BackgroundControl("NATURAL_SPLINE")

        # note: by default undersampleStyle is THROW_EXCEPTION
        bctrl.setUndersampleStyle(afwMath.REDUCE_INTERP_ORDER)

        nx = int(mimg.getWidth()/binsize) + 1
        ny = int(mimg.getHeight()/binsize) + 1

        bctrl.setNxSample(nx)
        bctrl.setNySample(ny)
        image = mimg.getImage()
        backobj = afwMath.makeBackground(image, bctrl)
        image -= backobj.getImageF()

    def testTicket1781(self):
        """Test an unusual-sized image"""
        nx = 526
        ny = 154

        parabimg = self.getParabolaImage(nx, ny)

        bctrl = afwMath.BackgroundControl(afwMath.Interpolate.CUBIC_SPLINE)
        bctrl.setNxSample(16)
        bctrl.setNySample(4)
        bctrl.getStatisticsControl().setNumSigmaClip(10.0)
        bctrl.getStatisticsControl().setNumIter(1)
        afwMath.makeBackground(parabimg, bctrl)

    def testParabola(self):
        """Test an image which varies parabolicly (spline should be exact for 2rd order polynomial)"""
        nx = 512
        ny = 512

        parabimg = self.getParabolaImage(nx, ny)

        # check corner, edge, and center pixels
        bctrl = afwMath.BackgroundControl(afwMath.Interpolate.CUBIC_SPLINE)
        bctrl.setNxSample(24)
        bctrl.setNySample(24)
        bctrl.getStatisticsControl().setNumSigmaClip(10.0)
        bctrl.getStatisticsControl().setNumIter(1)
        backobj = afwMath.makeBackground(parabimg, bctrl)

        segmentCenter = int(0.5*nx/bctrl.getNxSample())
        xpixels = [segmentCenter, nx//2, nx - segmentCenter]
        ypixels = [segmentCenter, ny//2, ny - segmentCenter]
        for xpix in xpixels:
            for ypix in ypixels:
                testval = backobj.getImageF(bctrl.getInterpStyle())[xpix, ypix, afwImage.LOCAL]
                realval = parabimg[xpix, ypix, afwImage.LOCAL]
                # quadratic terms skew the averages of the subimages and the clipped mean for
                # a subimage != value of center pixel.  1/20 counts on a 10000 count sky
                # is a fair (if arbitrary) test.
                self.assertLess(abs(testval - realval), 0.5)

    @unittest.skipIf(AfwdataDir is None, "afwdata not setup")
    def testCFHT_oldAPI(self):
        """Test background subtraction on some real CFHT data"""
        mi = afwImage.MaskedImageF(os.path.join(
            AfwdataDir, "CFHT", "D4", "cal-53535-i-797722_1.fits"))
        mi = mi.Factory(mi,
                        lsst.geom.Box2I(lsst.geom.Point2I(32, 2),
                                        lsst.geom.Point2I(2079, 4609)),
                        afwImage.LOCAL)

        bctrl = afwMath.BackgroundControl(afwMath.Interpolate.AKIMA_SPLINE)
        bctrl.setNxSample(16)
        bctrl.setNySample(16)
        bctrl.getStatisticsControl().setNumSigmaClip(3.0)
        bctrl.getStatisticsControl().setNumIter(2)
        backobj = afwMath.makeBackground(mi.getImage(), bctrl)

        if debugMode:
            afwDisplay.Display(frame=0).mtv(mi, title=self._testMethodName + " image")

        im = mi.getImage()
        im -= backobj.getImageF()

        if debugMode:
            afwDisplay.Display(frame=1).mtv(mi, title=self._testMethodName + " image-back")

    def getCfhtImage(self):
        """Get a portion of a CFHT image as a MaskedImageF"""
        bbox = lsst.geom.Box2I(lsst.geom.Point2I(500, 2000),
                               lsst.geom.Point2I(2079, 4609))
        imagePath = os.path.join(
            AfwdataDir, "CFHT", "D4", "cal-53535-i-797722_1.fits")
        return afwImage.MaskedImageF(imagePath, PropertySet(), bbox)

    @unittest.skipIf(AfwdataDir is None, "afwdata not setup")
    def testXY0(self):
        """Test fitting the background to an image with nonzero xy0

        The statsImage and background image should not vary with xy0
        """
        bgImageList = []  # list of background images, one per xy0
        statsImageList = []  # list of stats images, one per xy0
        for xy0 in (lsst.geom.Point2I(0, 0),
                    lsst.geom.Point2I(-100, -999),
                    lsst.geom.Point2I(1000, 500)):
            mi = self.getCfhtImage()
            mi.setXY0(xy0)

            bctrl = afwMath.BackgroundControl(
                mi.getWidth()//128, mi.getHeight()//128)
            backobj = afwMath.makeBackground(mi.getImage(), bctrl)
            bgImage = backobj.getImageF()
            self.assertEqual(bgImage.getBBox(), mi.getBBox())
            bgImageList.append(bgImage)

            statsImage = backobj.getStatsImage()
            statsImageList.append(statsImage)

        # changing the bounding box should make no difference to the pixel values,
        # so compare pixels using exact equality
        for bgImage in bgImageList[1:]:
            self.assertImagesEqual(bgImage, bgImageList[0])
        for statsImage in statsImageList[1:]:
            self.assertMaskedImagesEqual(statsImage, statsImageList[0])

    @unittest.skipIf(AfwdataDir is None, "afwdata not setup")
    def testSubImage(self):
        """Test getImage on a subregion of the full background image

        Using real image data is a cheap way to get a variable background
        """
        mi = self.getCfhtImage()

        bctrl = afwMath.BackgroundControl(
            mi.getWidth()//128, mi.getHeight()//128)
        backobj = afwMath.makeBackground(mi.getImage(), bctrl)
        subBBox = lsst.geom.Box2I(lsst.geom.Point2I(1000, 3000),
                                  lsst.geom.Extent2I(100, 100))

        bgFullImage = backobj.getImageF()
        self.assertEqual(bgFullImage.getBBox(), mi.getBBox())

        subFullArr = afwImage.ImageF(bgFullImage, subBBox).getArray()

        bgSubImage = backobj.getImageF(subBBox, bctrl.getInterpStyle())
        subArr = bgSubImage.getArray()

        # the pixels happen to be identical but it is safer not to rely on
        # that; close is good enough
        self.assertFloatsEqual(subArr, subFullArr)

    @unittest.skipIf(AfwdataDir is None, "afwdata not setup")
    def testCFHT(self):
        """Test background subtraction on some real CFHT data"""
        mi = self.getCfhtImage()

        bctrl = afwMath.BackgroundControl(
            mi.getWidth()//128, mi.getHeight()//128)
        bctrl.getStatisticsControl().setNumSigmaClip(3.0)
        bctrl.getStatisticsControl().setNumIter(2)
        backobj = afwMath.makeBackground(mi.getImage(), bctrl)

        if debugMode:
            afwDisplay.Display(frame=0).mtv(mi, title=self._testMethodName + " image")

        im = mi.getImage()
        im -= backobj.getImageF("AKIMA_SPLINE")

        if debugMode:
            afwDisplay.Display(frame=1).mtv(mi, title=self._testMethodName + " image-back")

        statsImage = backobj.getStatsImage()

        if debugMode:
            afwDisplay.Display(frame=2).mtv(statsImage, title=self._testMethodName + " bkgd StatsImage")
            afwDisplay.Display(frame=3).mtv(statsImage.getVariance(),
                                            title=self._testMethodName + " bkgd Variance")

    def testUndersample(self):
        """Test how the program handles nx,ny being too small for requested interp style."""
        nx = 64
        ny = 64
        img = afwImage.ImageF(lsst.geom.Extent2I(nx, ny))

        # make a background control object
        bctrl = afwMath.BackgroundControl(10, 10)
        bctrl.setInterpStyle(afwMath.Interpolate.CUBIC_SPLINE)
        bctrl.setNxSample(3)
        bctrl.setNySample(3)

        # put nx,ny back to 2 and see if it adjusts the interp style down to
        # linear
        bctrl.setNxSample(2)
        bctrl.setNySample(2)
        bctrl.setUndersampleStyle("REDUCE_INTERP_ORDER")
        backobj = afwMath.makeBackground(img, bctrl)
        # Need to interpolate background to discover what we actually needed
        backobj.getImageF()
        self.assertEqual(backobj.getAsUsedInterpStyle(),
                         afwMath.Interpolate.LINEAR)

        # put interp style back up to cspline and see if it throws an exception
        bctrl.setUndersampleStyle("THROW_EXCEPTION")

        def tst(img, bctrl):
            backobj = afwMath.makeBackground(img, bctrl)
            # only now do we see that we have too few points
            backobj.getImageF("CUBIC_SPLINE")
        self.assertRaises(lsst.pex.exceptions.InvalidParameterError,
                          tst, img, bctrl)

    def testOnlyOneGridCell(self):
        """Test how the program handles nxSample,nySample being 1x1."""
        # try a ramping image ... has an easy analytic solution
        nx = 64
        ny = 64
        img = afwImage.ImageF(lsst.geom.Extent2I(nx, ny), 10)

        dzdx, dzdy, z0 = 0.1, 0.2, 10000.0
        mean = z0 + dzdx*(nx - 1)/2 + dzdy*(ny - 1)/2  # the analytic solution
        for x in range(nx):
            for y in range(ny):
                img[x, y, afwImage.LOCAL] = dzdx*x + dzdy*y + z0

        # make a background control object
        bctrl = afwMath.BackgroundControl(10, 10)
        bctrl.setInterpStyle(afwMath.Interpolate.CONSTANT)
        bctrl.setNxSample(1)
        bctrl.setNySample(1)
        bctrl.setUndersampleStyle(afwMath.THROW_EXCEPTION)
        backobj = afwMath.makeBackground(img, bctrl)

        xpixels = [0, nx//2, nx - 1]
        ypixels = [0, ny//2, ny - 1]
        for xpix in xpixels:
            for ypix in ypixels:
                testval = backobj.getImageF(bctrl.getInterpStyle())[xpix, ypix]
                self.assertAlmostEqual(testval/mean, 1)
    
    #@unittest.skip("")
    def testAdjustLevel(self):
        """Test that we can adjust a background level"""
        sky = 100
        im = afwImage.ImageF(40, 40)
        im.set(sky)
        nx, ny = im.getWidth()//2, im.getHeight()//2
        bctrl = afwMath.BackgroundControl("LINEAR", nx, ny)
        bkd = afwMath.makeBackground(im, bctrl)
        self.assertEqual(
            afwMath.makeStatistics(bkd.getImageF(), afwMath.MEAN).getValue(),
            sky)

        delta = 123
        bkd += delta
        self.assertEqual(
            afwMath.makeStatistics(bkd.getImageF(), afwMath.MEAN).getValue(),
            sky + delta)
        bkd -= delta
        self.assertEqual(afwMath.makeStatistics(bkd.getImageF(), afwMath.MEAN).getValue(),
                         sky)

    def testNaNFromMaskedImage(self):
        """Check that an extensively masked image doesn't lead to NaNs in the background estimation"""
        image = afwImage.MaskedImageF(800, 800)
        msk = image.getMask()
        bbox = lsst.geom.BoxI(lsst.geom.PointI(560, 0), lsst.geom.PointI(799, 335))
        smsk = msk.Factory(msk, bbox)
        smsk.set(msk.getPlaneBitMask("DETECTED"))

        binSize = 256
        nx = image.getWidth()//binSize + 1
        ny = image.getHeight()//binSize + 1

        sctrl = afwMath.StatisticsControl()
        sctrl.setAndMask(reduce(lambda x, y: x | image.getMask().getPlaneBitMask(y),
                                ['EDGE', 'DETECTED', 'DETECTED_NEGATIVE'], 0x0))

        bctrl = afwMath.BackgroundControl(nx, ny, sctrl, "MEANCLIP")

        bkgd = afwMath.makeBackground(image, bctrl)
        bkgdImage = bkgd.getImageF("NATURAL_SPLINE", "THROW_EXCEPTION")
        if debugMode:
            afwDisplay.Display(frame=0).mtv(image, title=self._testMethodName + " image")
            afwDisplay.Display(frame=1).mtv(bkgdImage, title=self._testMethodName + " bkgdImage")

        self.assertFalse(np.isnan(bkgdImage[0, 0, afwImage.LOCAL]))

        # Check that the non-string API works too
        bkgdImage = bkgd.getImageF(
            afwMath.Interpolate.NATURAL_SPLINE, afwMath.THROW_EXCEPTION)

    def testBadAreaFailsSpline(self):
        """Check that a NaN in the stats image doesn't cause spline interpolation to fail (#2734)"""
        image = afwImage.ImageF(15, 9)
        for y in range(image.getHeight()):
            for x in range(image.getWidth()):
                # n.b. linear, which is what the interpolation will fall back
                # to
                image[x, y, afwImage.LOCAL] = 1 + 2*y

        # Set the right corner to NaN.  This will mean that we have too few
        # points for a spline interpolator
        binSize = 3
        image[-binSize:, -binSize:, afwImage.LOCAL] = np.nan

        nx = image.getWidth()//binSize
        ny = image.getHeight()//binSize

        sctrl = afwMath.StatisticsControl()
        bctrl = afwMath.BackgroundControl(nx, ny, sctrl, afwMath.MEANCLIP)

        bkgd = afwMath.makeBackground(image, bctrl)
        if debugMode:
            afwDisplay.Display(frame=0).mtv(image, title=self._testMethodName + " image")
            afwDisplay.Display(frame=1).mtv(bkgd.getStatsImage(),
                                            title=self._testMethodName + " bkgd StatsImage")
        # Should throw if we don't permit REDUCE_INTERP_ORDER
        self.assertRaises(lsst.pex.exceptions.OutOfRangeError,
                          bkgd.getImageF, afwMath.Interpolate.NATURAL_SPLINE)
        # The interpolation should fall back to linear for the right part of the image
        # where the NaNs don't permit spline interpolation (n.b. this happens
        # to be exact)
        bkgdImage = bkgd.getImageF(
            afwMath.Interpolate.NATURAL_SPLINE, afwMath.REDUCE_INTERP_ORDER)

        if debugMode:
            afwDisplay.Display(frame=2).mtv(bkgdImage, title=self._testMethodName + " bkgdImage")

        image -= bkgdImage
        self.assertEqual(afwMath.makeStatistics(image, afwMath.MEAN).getValue(),
                         0.0)

    def testBadPatch(self):
        """Test that a large bad patch of an image doesn't cause an absolute failure"""
        initialValue = 20
        mi = afwImage.MaskedImageF(500, 200)
        mi.set((initialValue, 0x0, 1.0))
        mi.image[0:200, :] = np.nan
        badBits = mi.mask.getPlaneBitMask(
            ['EDGE', 'DETECTED', 'DETECTED_NEGATIVE'])
        mi.mask[0:400, :] |= badBits

        if debugMode:
            afwDisplay.Display(frame=0).mtv(mi, title=self._testMethodName + " image")

        sctrl = afwMath.StatisticsControl()
        sctrl.setAndMask(badBits)
        nx, ny = 17, 17
        bctrl = afwMath.BackgroundControl(nx, ny, sctrl, afwMath.MEANCLIP)

        bkgd = afwMath.makeBackground(mi, bctrl)
        statsImage = bkgd.getStatsImage()
        if debugMode:
            afwDisplay.Display(frame=1).mtv(statsImage, title=self._testMethodName + " bkgd StatsImage")

        # the test is that this doesn't fail if the bug (#2297) is fixed
        bkgdImage = bkgd.getImageF(
            afwMath.Interpolate.NATURAL_SPLINE, afwMath.REDUCE_INTERP_ORDER)
        self.assertEqual(
            np.mean(bkgdImage[0:100, 0:100].array), initialValue)
        if debugMode:
            afwDisplay.Display(frame=2).mtv(bkgdImage, title=self._testMethodName + " bkgdImage")
        # Check that we can fix the NaNs in the statsImage
        sim = statsImage.getImage().getArray()
        sim[np.isnan(sim)] = initialValue  # replace NaN by initialValue
        bkgdImage = bkgd.getImageF(
            afwMath.Interpolate.NATURAL_SPLINE, afwMath.REDUCE_INTERP_ORDER)

        self.assertAlmostEqual(
            np.mean(bkgdImage[0:100, 0:100].array, dtype=np.float64),
            initialValue)

    def testBadRows(self):
        """Test that a bad set of rows in an image doesn't cause a failure"""
        initialValue = 20
        mi = afwImage.MaskedImageF(500, 200)
        mi.set((initialValue, 0x0, 1.0))
        mi.image[:, 0:100] = np.nan
        badBits = mi.mask.getPlaneBitMask(
            ['EDGE', 'DETECTED', 'DETECTED_NEGATIVE'])
        mi.mask[0:400, :] |= badBits

        if debugMode:
            afwDisplay.Display(frame=0).mtv(mi, title=self._testMethodName + " image")

        sctrl = afwMath.StatisticsControl()
        sctrl.setAndMask(badBits)
        nx, ny = 17, 17
        bctrl = afwMath.BackgroundControl(nx, ny, sctrl, afwMath.MEANCLIP)

        bkgd = afwMath.makeBackground(mi, bctrl)
        statsImage = bkgd.getStatsImage()
        if debugMode:
            afwDisplay.Display(frame=1).mtv(statsImage, title=self._testMethodName + " bkgd StatsImage")

        # the test is that this doesn't fail if the bug (#2297) is fixed
        for frame, interpStyle in enumerate([afwMath.Interpolate.CONSTANT, afwMath.Interpolate.LINEAR,
                                             afwMath.Interpolate.NATURAL_SPLINE,
                                             afwMath.Interpolate.AKIMA_SPLINE], 2):
            bkgdImage = bkgd.getImageF(
                interpStyle, afwMath.REDUCE_INTERP_ORDER)
            self.assertEqual(
                np.mean(bkgdImage[0:100, 0:100].array), initialValue)
            if debugMode:
                afwDisplay.Display(frame=frame).mtv(bkgdImage,
                                                    title=f"{self._testMethodName} bkgdImage: {interpStyle}")

    def testBadImage(self):
        """Test that an entirely bad image doesn't cause an absolute failure"""
        initialValue = 20
        mi = afwImage.MaskedImageF(500, 200)
        # Check that no good values don't crash (they return NaN), and that a single good value
        # is enough to redeem the entire image
        for pix00 in [np.nan, initialValue]:
            mi.image[:] = np.nan
            mi.image[0, 0] = pix00

            sctrl = afwMath.StatisticsControl()
            nx, ny = 17, 17
            bctrl = afwMath.BackgroundControl(nx, ny, sctrl, afwMath.MEANCLIP)

            bkgd = afwMath.makeBackground(mi, bctrl)

            for interpStyle in [afwMath.Interpolate.CONSTANT, afwMath.Interpolate.LINEAR,
                                afwMath.Interpolate.NATURAL_SPLINE, afwMath.Interpolate.AKIMA_SPLINE]:
                # the test is that this doesn't fail if the bug (#2297) is
                # fixed
                bkgdImage = bkgd.getImageF(
                    interpStyle, afwMath.REDUCE_INTERP_ORDER)
                val = np.mean(bkgdImage[0:100, 0:100].array)

                if np.isfinite(pix00):
                    self.assertEqual(val, pix00)
                else:
                    self.assertTrue(np.isnan(val))

    def testBackgroundFromStatsImage(self):
        """Check that we can rebuild a Background from a BackgroundMI.getStatsImage()"""
        bgCtrl = afwMath.BackgroundControl(10, 10)
        bkgd = afwMath.makeBackground(self.image, bgCtrl)

        interpStyle = afwMath.Interpolate.AKIMA_SPLINE
        undersampleStyle = afwMath.REDUCE_INTERP_ORDER
        bkgdImage = bkgd.getImageF(interpStyle, undersampleStyle)
        self.assertEqual(np.mean(bkgdImage.getArray()), self.val)
        self.assertEqual(interpStyle, bkgd.getAsUsedInterpStyle())
        self.assertEqual(undersampleStyle, bkgd.getAsUsedUndersampleStyle())

        # OK, we have our background.  Make a copy
        bkgd2 = afwMath.BackgroundMI(
            self.image.getBBox(), bkgd.getStatsImage())
        del bkgd           # we should be handling the memory correctly, but let's check
        bkgdImage2 = bkgd2.getImageF(interpStyle)

        self.assertEqual(np.mean(bkgdImage2.getArray()), self.val)

    @unittest.skip("")
    def testBackgroundList(self):
        """Test that a BackgroundLists behaves like a list"""
        bgCtrl = afwMath.BackgroundControl(10, 10)
        interpStyle = afwMath.Interpolate.AKIMA_SPLINE
        undersampleStyle = afwMath.REDUCE_INTERP_ORDER
        approxStyle = afwMath.ApproximateControl.UNKNOWN
        approxOrderX = 0
        approxOrderY = 0
        approxWeighting = False

        backgroundList = afwMath.BackgroundList()
        # An empty BackgroundList should eval to False, like an empty list.
        self.assertFalse(backgroundList)

        for i in range(2):
            bkgd = afwMath.makeBackground(self.image, bgCtrl)
            if i == 0:
                # no need to call getImage
                backgroundList.append((bkgd, interpStyle, undersampleStyle,
                                       approxStyle, approxOrderX, approxOrderY, approxWeighting))
            else:
                backgroundList.append(bkgd)

        def assertBackgroundList(bgl):
            self.assertTrue(bgl)
            self.assertEqual(len(bgl), 2)  # check that len() works
            for a in bgl:                 # check that we can iterate
                pass
            self.assertEqual(len(bgl[0]), 7)  # check that we can index
            # check that we always have a tuple (bkgd, interp, under,
            # approxStyle, orderX, orderY, weighting)
            self.assertEqual(len(bgl[1]), 7)

        assertBackgroundList(backgroundList)

        # Check pickling
        new = pickle.loads(pickle.dumps(backgroundList))
        assertBackgroundList(new)
        self.assertEqual(len(new), len(backgroundList))
        for i, j in zip(new, backgroundList):
            self.assertBackgroundEqual(i[0], j[0])
            self.assertEqual(i[1:], j[1:])

    def assertBackgroundEqual(self, lhs, rhs):
        lhsStats, rhsStats = lhs.getStatsImage(), rhs.getStatsImage()
        self.assertEqual(lhs.getImageBBox(), rhs.getImageBBox())
        self.assertMaskedImagesEqual(lhsStats, rhsStats)
        lhsImage, rhsImage = lhs.getImageF("LINEAR"), rhs.getImageF("LINEAR")
        self.assertImagesEqual(lhsImage, rhsImage)

    @unittest.skip("")
    def testApproximate(self):
        """Test I/O for BackgroundLists with Approximate"""
        # approx and interp should be very close, but not the same
        img = self.getParabolaImage(256, 256)

        # try regular interpolated image (the default)
        interpStyle = afwMath.Interpolate.AKIMA_SPLINE
        undersampleStyle = afwMath.REDUCE_INTERP_ORDER
        bgCtrl = afwMath.BackgroundControl(6, 6)
        bgCtrl.setInterpStyle(interpStyle)
        bgCtrl.setUndersampleStyle(undersampleStyle)
        bkgd = afwMath.makeBackground(img, bgCtrl)
        interpImage = bkgd.getImageF()

        with lsst.utils.tests.getTempFilePath("_bgi.fits") as bgiFile, \
                lsst.utils.tests.getTempFilePath("_bga.fits") as bgaFile:
            bglInterp = afwMath.BackgroundList()
            bglInterp.append((bkgd, interpStyle, undersampleStyle,
                              afwMath.ApproximateControl.UNKNOWN, 0, 0, True))
            bglInterp.writeFits(bgiFile)

            # try an approx background
            approxStyle = afwMath.ApproximateControl.CHEBYSHEV
            approxOrder = 2
            actrl = afwMath.ApproximateControl(approxStyle, approxOrder)
            bkgd.getBackgroundControl().setApproximateControl(actrl)
            approxImage = bkgd.getImageF()
            bglApprox = afwMath.BackgroundList()
            bglApprox.append((bkgd, interpStyle, undersampleStyle,
                              approxStyle, approxOrder, approxOrder, True))
            bglApprox.writeFits(bgaFile)

            # take a difference and make sure the two are very similar
            interpNp = interpImage.getArray()
            diff = np.abs(interpNp - approxImage.getArray())/interpNp

            # the image and interp/approx parameters are chosen so these limits
            # will be greater than machine precision for float.  The two methods
            # should be measurably different (so we know we're not just getting the
            # same thing from the getImage() method.  But they should be very close
            # since they're both doing the same sort of thing.
            tolSame = 1.0e-3  # should be the same to this order
            tolDiff = 1.0e-4  # should be different here
            self.assertLess(diff.max(), tolSame)
            self.assertGreater(diff.max(), tolDiff)

            # now see if we can reload them from files and get the same images
            # we wrote
            interpImage2 = afwMath.BackgroundList().readFits(bgiFile).getImage()
            approxImage2 = afwMath.BackgroundList().readFits(bgaFile).getImage()

            idiff = interpImage.getArray() - interpImage2.getArray()
            adiff = approxImage.getArray() - approxImage2.getArray()
            self.assertEqual(idiff.max(), 0.0)
            self.assertEqual(adiff.max(), 0.0)
    
    @unittest.skip("")
    def testBackgroundListIO(self):
        """Test I/O for BackgroundLists"""
        bgCtrl = afwMath.BackgroundControl(10, 10)
        interpStyle = afwMath.Interpolate.AKIMA_SPLINE
        undersampleStyle = afwMath.REDUCE_INTERP_ORDER
        approxOrderX = 6
        approxOrderY = 6
        approxWeighting = True

        im = self.image.Factory(
            self.image, self.image.getBBox())
        arr = im.getArray()
        arr += np.random.normal(size=(im.getHeight(), im.getWidth()))

        for astyle in afwMath.ApproximateControl.UNKNOWN, afwMath.ApproximateControl.CHEBYSHEV:
            actrl = afwMath.ApproximateControl(astyle, approxOrderX)
            bgCtrl.setApproximateControl(actrl)

            backgroundList = afwMath.BackgroundList()
            backImage = afwImage.ImageF(im.getDimensions())
            for i in range(2):
                bkgd = afwMath.makeBackground(im, bgCtrl)
                if i == 0:
                    # no need to call getImage
                    backgroundList.append((bkgd, interpStyle, undersampleStyle,
                                           astyle, approxOrderX, approxOrderY, approxWeighting))
                else:
                    backgroundList.append(bkgd)

                backImage += bkgd.getImageF(interpStyle, undersampleStyle)

            with lsst.utils.tests.getTempFilePath(".fits") as fileName:
                backgroundList.writeFits(fileName)

                backgrounds = afwMath.BackgroundList.readFits(fileName)

                img = backgrounds.getImage()
                # Check that the read-back image is identical to that generated from the backgroundList
                # round-tripped to disk
                backImage -= img

                self.assertEqual(np.min(backImage.getArray()), 0.0)
                self.assertEqual(np.max(backImage.getArray()), 0.0)


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
