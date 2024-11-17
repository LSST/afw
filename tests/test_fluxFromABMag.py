#
# LSST Data Management System
# Copyright 2014 LSST Corporation.
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
"""
Tests for lsst.afw.table.FluxFromABMagTable, etc.
"""
import unittest

import numpy as np

import lsst.utils.tests
import lsst.afw.image as afwImage


def refABMagFromFlux(flux):
    return -(5.0/2.0) * np.log10(flux/3631.0)


def refABMagErrFromFluxErr(fluxErr, flux):
    return np.fabs(fluxErr/(-0.4*flux*np.log(10)))


class FluxFromABMagTableTestCase(lsst.utils.tests.TestCase):

    def testBasics(self):
        for flux in (1, 210, 3210, 43210, 543210):
            abMag = afwImage.abMagFromFlux(flux)
            self.assertAlmostEqual(abMag, refABMagFromFlux(flux))
            fluxRoundTrip = afwImage.fluxFromABMag(abMag)
            self.assertAlmostEqual(flux, fluxRoundTrip)

            for fluxErrFrac in (0.001, 0.01, 0.1):
                fluxErr = flux * fluxErrFrac
                abMagErr = afwImage.abMagErrFromFluxErr(fluxErr, flux)
                self.assertAlmostEqual(
                    abMagErr, refABMagErrFromFluxErr(fluxErr, flux))
                fluxErrRoundTrip = afwImage.fluxErrFromABMagErr(
                    abMagErr, abMag)
                self.assertAlmostEqual(fluxErr, fluxErrRoundTrip)

    def testVector(self):
        flux = np.array([1.0, 210.0, 3210.0, 43210.0, 543210.0])
        # flux.flags.writeable = False  # Put the 'const' into ndarray::Array<double const, 1, 0>
        abMag = afwImage.abMagFromFlux(flux)
        self.assertFloatsAlmostEqual(abMag, refABMagFromFlux(flux))
        fluxRoundTrip = afwImage.fluxFromABMag(abMag)
        self.assertFloatsAlmostEqual(flux, fluxRoundTrip, rtol=1.0e-15)

        for fluxErrFrac in (0.001, 0.01, 0.1):
            fluxErr = flux * fluxErrFrac
            abMagErr = afwImage.abMagErrFromFluxErr(fluxErr, flux)
            self.assertFloatsAlmostEqual(abMagErr, refABMagErrFromFluxErr(fluxErr, flux))
            fluxErrRoundTrip = afwImage.fluxErrFromABMagErr(abMagErr, abMag)
            self.assertFloatsAlmostEqual(fluxErr, fluxErrRoundTrip, rtol=1.0e-15)


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
