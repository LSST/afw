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

import unittest

import numpy as np

import lsst.utils.tests
import lsst.pex.exceptions
import lsst.geom
import lsst.afw.table
import lsst.afw.fits
import lsst.afw.image
import lsst.afw.detection


def makeGaussianImage(bbox, sigma, xc=0.0, yc=0.0):
    image = lsst.afw.image.ImageD(bbox)
    array = image.getArray()
    for yi, yv in enumerate(range(bbox.getBeginY(), bbox.getEndY())):
        for xi, xv in enumerate(range(bbox.getBeginX(), bbox.getEndX())):
            array[yi, xi] = np.exp(-0.5*((xv - xc)**2 + (yv - yc)**2)/sigma**2)
    array /= array.sum()
    return image


def computeNaiveApertureFlux(image, radius, xc=0.0, yc=0.0):
    bbox = image.getBBox()
    array = image.getArray()
    s = 0.0
    for yi, yv in enumerate(range(bbox.getBeginY(), bbox.getEndY())):
        for xi, xv in enumerate(range(bbox.getBeginX(), bbox.getEndX())):
            if (xv - xc)**2 + (yv - yc)**2 < radius**2:
                s += array[yi, xi]
    return s


class GaussianPsfTestCase(lsst.utils.tests.TestCase):

    def setUp(self):
        self.kernelSize = 51
        self.psf = lsst.afw.detection.GaussianPsf(
            self.kernelSize, self.kernelSize, 4.0)

    def tearDown(self):
        del self.psf

    def testKernelImage(self):
        image = self.psf.computeKernelImage(self.psf.getAveragePosition())
        check = makeGaussianImage(image.getBBox(), self.psf.getSigma())
        self.assertFloatsAlmostEqual(image.getArray(), check.getArray())
        self.assertFloatsAlmostEqual(image.getArray().sum(), 1.0, atol=1E-14)

    def testOffsetImage(self):
        image = self.psf.computeImage(lsst.geom.Point2D(0.25, 0.25))
        check = makeGaussianImage(
            image.getBBox(), self.psf.getSigma(), 0.25, 0.25)
        self.assertFloatsAlmostEqual(image.getArray(), check.getArray(), atol=1E-4, rtol=1E-4,
                                     plotOnFailure=True)

    def testApertureFlux(self):
        image = self.psf.computeKernelImage(lsst.geom.Point2D(0.0, 0.0))
        # test aperture implementation is very crude; can only test to about 10%
        self.assertFloatsAlmostEqual(self.psf.computeApertureFlux(5.0), computeNaiveApertureFlux(image, 5.0),
                                     rtol=0.1)
        self.assertFloatsAlmostEqual(self.psf.computeApertureFlux(7.0), computeNaiveApertureFlux(image, 7.0),
                                     rtol=0.1)

    def testShape(self):
        self.assertFloatsAlmostEqual(
            self.psf.computeShape(self.psf.getAveragePosition()).getDeterminantRadius(), 4.0)

    def testPersistence(self):
        with lsst.utils.tests.getTempFilePath(".fits") as filename:
            self.psf.writeFits(filename)
            psf = lsst.afw.detection.GaussianPsf.readFits(filename)
            self.assertEqual(self.psf.getSigma(), psf.getSigma())
            self.assertEqual(self.psf.getDimensions(), psf.getDimensions())

    def testBBox(self):

        self.assertEqual(self.psf.computeKernelImage(self.psf.getAveragePosition()).getBBox(),
                         self.psf.computeBBox(self.psf.getAveragePosition()))

        self.assertEqual(self.psf.computeBBox().getWidth(), self.kernelSize)

        # Test interface. GaussianPsf does not vary spatially
        self.assertEqual(self.psf.computeKernelImage(lsst.geom.Point2D(0.0, 0.0)).getBBox(),
                         self.psf.computeBBox(lsst.geom.Point2D(0.0, 0.0)))

        self.assertEqual(self.psf.computeImage(self.psf.getAveragePosition()).getBBox(),
                         self.psf.computeImageBBox(self.psf.getAveragePosition()))

    def testResized(self):
        for pad in [0, -10, 10]:
            newLen = self.kernelSize - pad
            resizedPsf = self.psf.resized(newLen, newLen)
            newBBox = resizedPsf.computeBBox(resizedPsf.getAveragePosition())
            self.assertEqual(newBBox.getWidth(), newLen)
            self.assertEqual(newBBox.getHeight(), newLen)
            self.assertEqual(resizedPsf.getSigma(), self.psf.getSigma())

            image = resizedPsf.computeKernelImage(resizedPsf.getAveragePosition())
            check = makeGaussianImage(newBBox, self.psf.getSigma())
            self.assertFloatsAlmostEqual(image.getArray(), check.getArray())
            # tolerance same as in self.testKernelImage
            self.assertFloatsAlmostEqual(image.getArray().sum(), 1.0, atol=1E-14)


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
