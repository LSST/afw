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
from copy import deepcopy
import pickle

import numpy as np

import lsst.utils.tests
from lsst.afw.typehandling import StorableHelperFactory
from lsst.afw.detection import Psf, GaussianPsf
from lsst.afw.image import Image, ExposureF
from lsst.geom import Box2I, Extent2I, Point2I, Point2D
from lsst.afw.geom.ellipses import Quadrupole
import testPsfTrampolineLib as cppLib


# Subclass Psf in python.  Main tests here are that python virtual methods get
# resolved by trampoline class.  The test suite below calls python compute*
# methods which are implemented in c++ to call the _doCompute* methods defined
# in the PyGaussianPsf class.  We also test persistence and associated
# overloads.
class PyGaussianPsf(Psf):
    # We need to ensure a c++ StorableHelperFactory is constructed and available
    # before any unpersists of this class.  Placing this "private" class
    # attribute here accomplishes that.  Note the unusual use of `__name__` for
    # the module name, which is appropriate here where the class is defined in
    # the test suite.  In production code, this might be something like
    # `lsst.meas.extensions.piff`
    _factory = StorableHelperFactory(__name__, "PyGaussianPsf")

    def __init__(self, width, height, sigma):
        Psf.__init__(self, isFixed=True)
        self.dimensions = Extent2I(width, height)
        self.sigma = sigma

    # "public" virtual overrides
    def __deepcopy__(self, memo=None):
        return PyGaussianPsf(self.dimensions.x, self.dimensions.y, self.sigma)

    def resized(self, width, height):
        return PyGaussianPsf(width, height, self.sigma)

    def isPersistable(self):
        return True

    # "private" virtual overrides are underscored by convention
    def _doComputeKernelImage(self, position=None, color=None):
        bbox = self.computeBBox(self.getAveragePosition())
        img = Image(bbox, dtype=np.float64)
        x, y = np.ogrid[bbox.minY:bbox.maxY+1, bbox.minX:bbox.maxX+1]
        rsqr = x**2 + y**2
        img.array[:] = np.exp(-0.5*rsqr/self.sigma**2)
        img.array /= np.sum(img.array)
        return img

    def _doComputeBBox(self, position=None, color=None):
        print(type(self.dimensions/2))
        print(self.dimensions/2)
        return Box2I(Point2I(-self.dimensions/2), self.dimensions)

    def _doComputeShape(self, position=None, color=None):
        return Quadrupole(self.sigma**2, self.sigma**2, 0.0)

    def _doComputeApertureFlux(self, radius, position=None, color=None):
        return 1 - np.exp(-0.5*(radius/self.sigma)**2)

    def _getPersistenceName(self):
        return "PyGaussianPsf"

    def _getPythonModule(self):
        return __name__

    # _write and _read are not ordinary python overrides of the c++ Psf methods,
    # since the argument types required by the c++ methods are not available in
    # python.  Instead, we create methods that opaquely persist/unpersist
    # to/from a string via pickle.
    def _write(self):
        return pickle.dumps((self.dimensions, self.sigma))

    @staticmethod
    def _read(pkl):
        dimensions, sigma = pickle.loads(pkl)
        return PyGaussianPsf(dimensions.x, dimensions.y, sigma)

    def __eq__(self, rhs):
        if isinstance(rhs, PyGaussianPsf):
            return (
                self.dimensions == rhs.dimensions
                and self.sigma == rhs.sigma
            )
        return False


class PsfTrampolineTestSuite(lsst.utils.tests.TestCase):
    def setUp(self):
        self.pgps = []
        self.gps = []
        for width, height, sigma in [
            (5, 5, 1.1),
            (5, 3, 1.2),
            (7, 7, 1.3)
        ]:
            self.pgps.append(PyGaussianPsf(width, height, sigma))
            self.gps.append(GaussianPsf(width, height, sigma))

    def testImages(self):
        for pgp, gp in zip(self.pgps, self.gps):
            self.assertImagesAlmostEqual(
                pgp.computeImage(pgp.getAveragePosition()),
                gp.computeImage(gp.getAveragePosition())
            )
            self.assertImagesAlmostEqual(
                pgp.computeKernelImage(pgp.getAveragePosition()),
                gp.computeKernelImage(gp.getAveragePosition())
            )

    def testApertureFlux(self):
        for pgp, gp in zip(self.pgps, self.gps):
            for r in [0.1, 0.2, 0.3]:
                self.assertAlmostEqual(
                    pgp.computeApertureFlux(r, pgp.getAveragePosition()),
                    gp.computeApertureFlux(r, gp.getAveragePosition())
                )

    def testPeak(self):
        for pgp, gp in zip(self.pgps, self.gps):
            self.assertAlmostEqual(
                pgp.computePeak(pgp.getAveragePosition()),
                gp.computePeak(gp.getAveragePosition())
            )

    def testBBox(self):
        for pgp, gp in zip(self.pgps, self.gps):
            self.assertEqual(
                pgp.computeBBox(pgp.getAveragePosition()),
                gp.computeBBox(gp.getAveragePosition())
            )
            self.assertEqual(
                pgp.computeKernelBBox(pgp.getAveragePosition()),
                gp.computeKernelBBox(gp.getAveragePosition()),
            )
            self.assertEqual(
                pgp.computeImageBBox(pgp.getAveragePosition()),
                gp.computeImageBBox(gp.getAveragePosition()),
            )
            self.assertEqual(
                pgp.computeImage(pgp.getAveragePosition()).getBBox(),
                pgp.computeImageBBox(pgp.getAveragePosition())
            )
            self.assertEqual(
                pgp.computeKernelImage(pgp.getAveragePosition()).getBBox(),
                pgp.computeKernelBBox(pgp.getAveragePosition())
            )

    def testShape(self):
        for pgp, gp in zip(self.pgps, self.gps):
            self.assertAlmostEqual(
                pgp.computeShape(pgp.getAveragePosition()),
                gp.computeShape(gp.getAveragePosition())
            )

    def testResized(self):
        for pgp, gp in zip(self.pgps, self.gps):
            width, height = pgp.dimensions
            rpgp = pgp.resized(width+2, height+4)
            # cppLib.resizedPsf calls Psf::resized, which redirects to
            # PyGaussianPsf.resized above
            rpgp2 = cppLib.resizedPsf(pgp, width+2, height+4)
            rgp = gp.resized(width+2, height+4)
            self.assertImagesAlmostEqual(
                rpgp.computeImage(rpgp.getAveragePosition()),
                rgp.computeImage(rgp.getAveragePosition())
            )
            self.assertImagesAlmostEqual(
                rpgp2.computeImage(rpgp2.getAveragePosition()),
                rgp.computeImage(rgp.getAveragePosition())
            )

    def testClone(self):
        """Test different ways of invoking PyGaussianPsf.__deepcopy__
        """
        for pgp in self.pgps:
            # directly
            p1 = deepcopy(pgp)
            # cppLib::clonedPsf -> Psf::clone
            p2 = cppLib.clonedPsf(pgp)
            # cppLib::clonedStorablePsf -> Psf::cloneStorable
            p3 = cppLib.clonedStorablePsf(pgp)
            # Psf::clone()
            p4 = pgp.clone()

            for p in [p1, p2, p3, p4]:
                self.assertIsNot(pgp, p)
                self.assertImagesEqual(
                    pgp.computeImage(pgp.getAveragePosition()),
                    p.computeImage(p.getAveragePosition())
                )

    def testPersistence(self):
        for pgp in self.pgps:
            assert cppLib.isPersistable(pgp)
            im = ExposureF(10, 10)
            im.setPsf(pgp)
            self.assertEqual(im.getPsf(), pgp)
            with lsst.utils.tests.getTempFilePath(".fits") as tmpFile:
                im.writeFits(tmpFile)
                newIm = ExposureF(tmpFile)
                self.assertEqual(newIm.getPsf(), im.getPsf())


# Psf with position-dependent image, but nonetheless may use isFixed=True.
# When isFixed=True, first image returned is cached for all subsequent image
# queries
class TestPsf(Psf):
    __test__ = False  # Stop Pytest from trying to parse as a TestCase

    def __init__(self, isFixed):
        Psf.__init__(self, isFixed=isFixed)

    def _doComputeKernelImage(self, position=None, color=None):
        bbox = Box2I(Point2I(-3, -3), Extent2I(7, 7))
        img = Image(bbox, dtype=np.float64)
        x, y = np.ogrid[bbox.minY:bbox.maxY+1, bbox.minX:bbox.maxX+1]
        rsqr = x**2 + y**2
        if position.x >= 0.0:
            img.array[:] = np.exp(-0.5*rsqr)
        else:
            img.array[:] = np.exp(-0.5*rsqr/4)
        img.array /= np.sum(img.array)
        return img


class FixedPsfTestSuite(lsst.utils.tests.TestCase):
    def setUp(self):
        self.fixedPsf = TestPsf(isFixed=True)
        self.floatPsf = TestPsf(isFixed=False)

    def testFloat(self):
        pos1 = Point2D(1.0, 1.0)
        pos2 = Point2D(-1.0, -1.0)
        img1 = self.floatPsf.computeKernelImage(pos1)
        img2 = self.floatPsf.computeKernelImage(pos2)
        self.assertFloatsNotEqual(img1.array, img2.array)

    def testFixed(self):
        pos1 = Point2D(1.0, 1.0)
        pos2 = Point2D(-1.0, -1.0)
        img1 = self.fixedPsf.computeKernelImage(pos1)
        # Although _doComputeKernelImage would return a different image here due
        # do the difference between pos1 and pos2, for the fixed Psf, the
        # caching mechanism intercepts instead and _doComputeKernelImage is
        # never called with position=pos2.  So img1 == img2.
        img2 = self.fixedPsf.computeKernelImage(pos2)
        self.assertFloatsEqual(img1.array, img2.array)


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
