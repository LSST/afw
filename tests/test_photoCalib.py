#
# LSST Data Management System
# Copyright 2008-2016 LSST Corporation.
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

import os.path
import unittest

import numpy as np

import astropy.units as u

import lsst.utils.tests
import lsst.geom
import lsst.afw.image
import lsst.afw.image.testUtils
import lsst.afw.math
import lsst.daf.base
import lsst.pex.exceptions


def computeNanojanskyErr(instFluxErr, instFlux, calibrationErr, calibration, flux):
    """Return the error on the flux (nanojansky)."""
    return flux*np.hypot(instFluxErr/instFlux, calibrationErr/calibration)


def computeMagnitudeErr(instFluxErr, instFlux, calibrationErr, calibration, flux):
    """Return the error on the magnitude."""
    err = computeNanojanskyErr(instFluxErr, instFlux, calibrationErr, calibration, flux)
    return 2.5/np.log(10) * err / flux


def makeCalibratedMaskedImage(image, mask, variance, outImage, calibration, calibrationErr):
    """Return a MaskedImage using outImage, mask, and a computed variance image."""
    outErr = computeNanojanskyErr(np.sqrt(variance),
                                  image,
                                  calibrationErr,
                                  calibration,
                                  outImage).astype(np.float32)  # variance plane must be 32bit
    return lsst.afw.image.makeMaskedImageFromArrays(outImage,
                                                    mask,
                                                    outErr**2)


def makeCalibratedMaskedImageNoCalibrationError(image, mask, variance, outImage):
    """Return a MaskedImage using outImage, mask, and a computed variance image.

    Ignores the contributions from the uncertainty in the calibration.
    """
    outErr = computeNanojanskyErr(np.sqrt(variance),
                                  image,
                                  0,
                                  1,
                                  outImage).astype(np.float32)  # variance plane must be 32bit
    return lsst.afw.image.makeMaskedImageFromArrays(outImage,
                                                    mask,
                                                    outErr**2)


class PhotoCalibTestCase(lsst.utils.tests.TestCase):

    def setUp(self):
        np.random.seed(100)

        self.point0 = lsst.geom.Point2D(0, 0)
        self.pointXShift = lsst.geom.Point2D(-10, 0)
        self.pointYShift = lsst.geom.Point2D(0, -10)
        self.bbox = lsst.geom.Box2I(lsst.geom.Point2I(-100, -100), lsst.geom.Point2I(100, 100))

        self.calibration = 1000.0
        # A 1% error on the calibration.
        self.calibrationErr = 10.0
        self.instFlux1 = 1.0
        self.instFluxErr1 = 0.1
        self.flux1 = 1000.0  # nJy
        self.mag1 = (self.flux1*u.nJy).to_value(u.ABmag)

        # useful reference points: 575.44 nJy ~= 24.5 mag, 3630.78 * 10^9 nJy ~= 0 mag
        self.flux2 = 575.44 * self.flux1
        self.instFlux2 = self.flux2/self.calibration
        self.mag2 = (self.flux2*u.nJy).to_value(u.ABmag)

        self.schema = lsst.afw.table.SourceTable.makeMinimalSchema()
        self.instFluxKeyName = "SomeFlux"
        lsst.afw.table.Point2DKey.addFields(self.schema, "centroid", "centroid", "pixels")
        self.schema.addField(self.instFluxKeyName+"_instFlux", type="D", units="count",
                             doc="post-ISR instrumental Flux")
        self.schema.addField(self.instFluxKeyName+"_instFluxErr", type="D", units="count",
                             doc="post-ISR instrumental flux error")
        self.schema.addField(self.instFluxKeyName+"_flux", type="D", units="nJy",
                             doc="calibrated flux")
        self.schema.addField(self.instFluxKeyName+"_fluxErr", type="D", units="nJy",
                             doc="calibrated flux error")
        self.schema.addField(self.instFluxKeyName+"_mag", type="D",
                             doc="calibrated magnitude")
        self.schema.addField(self.instFluxKeyName+"_magErr", type="D",
                             doc="calibrated magnitude error")
        self.otherInstFluxKeyName = "OtherFlux"
        self.schema.addField(self.otherInstFluxKeyName+"_instFlux", type="D", units="count",
                             doc="another instrumental Flux")
        self.schema.addField(self.otherInstFluxKeyName+"_instFluxErr", type="D", units="count",
                             doc="another instrumental flux error")
        self.noErrInstFluxKeyName = "NoErrFlux"
        self.schema.addField(self.noErrInstFluxKeyName+"_instFlux", type="D", units="count",
                             doc="instrumental Flux with no error")
        self.table = lsst.afw.table.SourceTable.make(self.schema)
        self.table.defineCentroid('centroid')
        self.catalog = lsst.afw.table.SourceCatalog(self.table)
        record = self.catalog.addNew()
        record.set('id', 1)
        record.set('centroid_x', self.point0[0])
        record.set('centroid_y', self.point0[1])
        record.set(self.instFluxKeyName+'_instFlux', self.instFlux1)
        record.set(self.instFluxKeyName+'_instFluxErr', self.instFluxErr1)
        record.set(self.otherInstFluxKeyName+'_instFlux', self.instFlux1)
        record.set(self.otherInstFluxKeyName+'_instFluxErr', self.instFluxErr1)
        record.set(self.noErrInstFluxKeyName+'_instFlux', self.instFlux1)
        record = self.catalog.addNew()
        record.set('id', 2)
        record.set('centroid_x', self.pointYShift[0])
        record.set('centroid_y', self.pointYShift[1])
        record.set(self.instFluxKeyName+'_instFlux', self.instFlux2)
        record.set(self.instFluxKeyName+'_instFluxErr', self.instFluxErr1)
        record.set(self.otherInstFluxKeyName+'_instFlux', self.instFlux2)
        record.set(self.otherInstFluxKeyName+'_instFluxErr', self.instFluxErr1)
        record.set(self.noErrInstFluxKeyName+'_instFlux', self.instFlux2)

        self.constantCalibration = lsst.afw.math.ChebyshevBoundedField(self.bbox,
                                                                       np.array([[self.calibration]]))
        self.linearXCalibration = lsst.afw.math.ChebyshevBoundedField(self.bbox,
                                                                      np.array([[self.calibration,
                                                                               self.calibration]]))

    def tearDown(self):
        del self.schema
        del self.table
        del self.catalog

    def _testPhotoCalibCenter(self, photoCalib, calibrationErr):
        """
        Test conversions of instFlux for the mean and (0,0) value of a photoCalib.
        Assumes those are the same, e.g. that the non-constant terms are all
        odd, and that the mean of the calib is self.calibration.

        calibrationErr is provided as an option to allow testing of photoCalibs
        that have no error specified, and those that do.
        """
        # test that the constructor set the calibrationMean and err correctly
        self.assertEqual(self.calibration, photoCalib.getCalibrationMean())
        self.assertEqual(photoCalib.instFluxToMagnitude(photoCalib.getInstFluxAtZeroMagnitude()), 0)
        self.assertEqual(calibrationErr, photoCalib.getCalibrationErr())

        # test with a "trivial" flux
        self.assertEqual(self.flux1, photoCalib.instFluxToNanojansky(self.instFlux1))
        self.assertEqual(self.mag1, photoCalib.instFluxToMagnitude(self.instFlux1))

        # a less trivial flux
        self.assertFloatsAlmostEqual(self.flux2, photoCalib.instFluxToNanojansky(self.instFlux2))
        self.assertFloatsAlmostEqual(self.mag2, photoCalib.instFluxToMagnitude(self.instFlux2))
        # test that (0,0) gives the same result as above
        self.assertFloatsAlmostEqual(self.flux2, photoCalib.instFluxToNanojansky(self.instFlux2, self.point0))
        self.assertFloatsAlmostEqual(self.mag2, photoCalib.instFluxToMagnitude(self.instFlux2, self.point0))

        # test that we get a correct nJy err for the base instFlux
        errFlux1 = computeNanojanskyErr(self.instFluxErr1,
                                        self.instFlux1,
                                        calibrationErr,
                                        self.calibration,
                                        self.flux1)
        result = photoCalib.instFluxToNanojansky(self.instFlux1, self.instFluxErr1)
        self.assertEqual(self.flux1, result.value)
        self.assertFloatsAlmostEqual(errFlux1, result.error)
        result = photoCalib.instFluxToNanojansky(self.instFlux1, self.instFluxErr1, self.point0)
        self.assertFloatsAlmostEqual(self.flux1, result.value)
        self.assertFloatsAlmostEqual(errFlux1, result.error)

        # test that we get a correct magnitude err for the base instFlux
        errMag1 = computeMagnitudeErr(self.instFluxErr1,
                                      self.instFlux1,
                                      calibrationErr,
                                      self.calibration,
                                      self.flux1)
        result = photoCalib.instFluxToMagnitude(self.instFlux1, self.instFluxErr1)
        self.assertEqual(self.mag1, result.value)
        self.assertFloatsAlmostEqual(errMag1, result.error)
        # and the same given an explicit point at the center
        result = photoCalib.instFluxToMagnitude(self.instFlux1, self.instFluxErr1, self.point0)
        self.assertFloatsAlmostEqual(self.mag1, result.value)
        self.assertFloatsAlmostEqual(errMag1, result.error)

        # test that we get a correct nJy err for flux2
        errFlux2 = computeNanojanskyErr(self.instFluxErr1,
                                        self.instFlux2,
                                        calibrationErr,
                                        self.calibration,
                                        self.flux2)
        result = photoCalib.instFluxToNanojansky(self.instFlux2, self.instFluxErr1)
        self.assertFloatsAlmostEqual(self.flux2, result.value)
        self.assertFloatsAlmostEqual(errFlux2, result.error)
        result = photoCalib.instFluxToNanojansky(self.instFlux2, self.instFluxErr1, self.point0)
        self.assertFloatsAlmostEqual(self.flux2, result.value)
        self.assertFloatsAlmostEqual(errFlux2, result.error)

        # test that we get a correct magnitude err for 575 nJy
        errMag2 = computeMagnitudeErr(self.instFluxErr1,
                                      self.instFlux2,
                                      calibrationErr,
                                      self.calibration,
                                      self.flux2)
        result = photoCalib.instFluxToMagnitude(self.instFlux2, self.instFluxErr1)
        self.assertFloatsAlmostEqual(self.mag2, result.value)
        self.assertFloatsAlmostEqual(errMag2, result.error)
        result = photoCalib.instFluxToMagnitude(self.instFlux2, self.instFluxErr1, self.point0)
        self.assertFloatsAlmostEqual(self.mag2, result.value)
        self.assertFloatsAlmostEqual(errMag2, result.error)

        # test calculations on a single sourceRecord
        record = self.catalog[0]
        result = photoCalib.instFluxToNanojansky(record, self.instFluxKeyName)
        self.assertEqual(self.flux1, result.value)
        self.assertFloatsAlmostEqual(errFlux1, result.error)
        result = photoCalib.instFluxToMagnitude(record, self.instFluxKeyName)
        self.assertEqual(self.mag1, result.value)
        self.assertFloatsAlmostEqual(errMag1, result.error)

        expectNanojansky = np.array([[self.flux1, errFlux1], [self.flux2, errFlux2]])
        expectMag = np.array([[self.mag1, errMag1], [self.mag2, errMag2]])
        self._testSourceCatalog(photoCalib, self.catalog, expectNanojansky, expectMag)

        # test reverse conversion: magnitude to instFlux (no position specified)
        self.assertFloatsAlmostEqual(self.instFlux1, photoCalib.magnitudeToInstFlux(self.mag1))
        self.assertFloatsAlmostEqual(self.instFlux2, photoCalib.magnitudeToInstFlux(self.mag2), rtol=1e-15)

        # test round-tripping instFlux->magnitude->instFlux (position specified)
        mag = photoCalib.instFluxToMagnitude(self.instFlux1, self.pointXShift)
        self.assertFloatsAlmostEqual(self.instFlux1,
                                     photoCalib.magnitudeToInstFlux(mag, self.pointXShift),
                                     rtol=1e-15)
        mag2 = photoCalib.instFluxToMagnitude(self.instFlux2, self.pointXShift)
        self.assertFloatsAlmostEqual(self.instFlux2,
                                     photoCalib.magnitudeToInstFlux(mag2, self.pointXShift),
                                     rtol=1e-15)

        # test reverse conversion: nanojansky to instFlux (no position specified)
        self.assertFloatsAlmostEqual(self.instFlux1, photoCalib.nanojanskyToInstFlux(self.flux1))
        self.assertFloatsAlmostEqual(self.instFlux2, photoCalib.nanojanskyToInstFlux(self.flux2), rtol=1e-15)

        # test round-tripping instFlux->nanojansky->instFlux (position specified)
        flux = photoCalib.instFluxToNanojansky(self.instFlux1, self.pointXShift)
        self.assertFloatsAlmostEqual(self.instFlux1,
                                     photoCalib.nanojanskyToInstFlux(flux, self.pointXShift),
                                     rtol=1e-15)
        flux2 = photoCalib.instFluxToNanojansky(self.instFlux2, self.pointXShift)
        self.assertFloatsAlmostEqual(self.instFlux2,
                                     photoCalib.nanojanskyToInstFlux(flux2, self.pointXShift),
                                     rtol=1e-15)

        # test round-tripping arrays (position specified)
        instFlux1Array = np.full(10, self.instFlux1)
        instFlux2Array = np.full(10, self.instFlux2)
        pointXShiftXArray = np.full(10, self.pointXShift.getX())
        pointXShiftYArray = np.full(10, self.pointXShift.getY())

        magArray = photoCalib.instFluxToMagnitudeArray(
            instFlux1Array,
            pointXShiftXArray,
            pointXShiftYArray
        )
        self.assertFloatsAlmostEqual(magArray.value, mag)
        self.assertFloatsAlmostEqual(photoCalib.magnitudeToInstFluxArray(magArray,
                                                                         pointXShiftXArray,
                                                                         pointXShiftYArray
                                                                         ),
                                     instFlux1Array,
                                     rtol=5e-15)
        mag2Array = photoCalib.instFluxToMagnitudeArray(
            np.full(10, self.instFlux2),
            np.full(10, self.pointXShift.getX()),
            np.full(10, self.pointXShift.getY())
        )
        self.assertFloatsAlmostEqual(mag2Array.value, mag2)
        self.assertFloatsAlmostEqual(photoCalib.magnitudeToInstFluxArray(mag2Array,
                                                                         pointXShiftXArray,
                                                                         pointXShiftYArray
                                                                         ),
                                     instFlux2Array,
                                     rtol=5e-15)

        # test getLocalCalibration.
        meas = photoCalib.instFluxToNanojansky(self.instFlux1, self.instFluxErr1, self.pointXShift)
        localCalib = photoCalib.getLocalCalibration(self.pointXShift)
        flux = localCalib * self.instFlux1
        self.assertAlmostEqual(meas.value, flux)

        # test getLocalCalibrationArray
        localCalib2 = photoCalib.getLocalCalibrationArray(
            pointXShiftXArray,
            pointXShiftYArray
        )
        self.assertFloatsAlmostEqual(localCalib2, localCalib)

    def _testSourceCatalog(self, photoCalib, catalog, expectNanojansky, expectMag):
        """Test instFluxTo*(sourceCatalog, ...), and calibrateCatalog()."""

        # test calculations on a sourceCatalog, returning the array
        result = photoCalib.instFluxToNanojansky(catalog, self.instFluxKeyName)
        self.assertFloatsAlmostEqual(expectNanojansky, result)
        result = photoCalib.instFluxToMagnitude(catalog, self.instFluxKeyName)
        self.assertFloatsAlmostEqual(expectMag, result)

        # Test modifying the catalog in-place with instFluxToNanojansky/instFluxToMagnitude
        # The original instFluxes shouldn't change: save them to test that.
        origInstFlux = catalog[self.instFluxKeyName+'_instFlux'].copy()
        origInstFluxErr = catalog[self.instFluxKeyName+'_instFluxErr'].copy()

        def checkCatalog(catalog, expect, keyName, outField):
            """Test that the fields in the catalog are correct."""
            self.assertFloatsAlmostEqual(catalog[keyName+'_%s' % outField], expect[:, 0])
            self.assertFloatsAlmostEqual(catalog[keyName+'_%sErr' % outField], expect[:, 1])
            self.assertFloatsAlmostEqual(catalog[keyName+'_instFlux'], origInstFlux)
            self.assertFloatsAlmostEqual(catalog[keyName+'_instFluxErr'], origInstFluxErr)

        testCat = catalog.copy(deep=True)
        photoCalib.instFluxToMagnitude(testCat, self.instFluxKeyName, self.instFluxKeyName)
        checkCatalog(testCat, expectMag, self.instFluxKeyName, "mag")

        testCat = catalog.copy(deep=True)
        photoCalib.instFluxToNanojansky(testCat, self.instFluxKeyName, self.instFluxKeyName)
        checkCatalog(testCat, expectNanojansky, self.instFluxKeyName, "flux")

        testCat = catalog.copy(deep=True)
        photoCalib.instFluxToMagnitude(testCat, self.instFluxKeyName, self.instFluxKeyName)
        checkCatalog(testCat, expectMag, self.instFluxKeyName, "mag")

        # test returning a calibrated catalog with calibrateCatalog

        # test that trying to calibrate a non-existent flux field raises
        with self.assertRaises(lsst.pex.exceptions.NotFoundError):
            photoCalib.calibrateCatalog(testCat, ["NotARealFluxFieldName"])

        # test calibrating just one flux field
        testCat = catalog.copy(deep=True)
        result = photoCalib.calibrateCatalog(testCat, [self.otherInstFluxKeyName])
        checkCatalog(result, expectNanojansky, self.otherInstFluxKeyName, "flux")
        checkCatalog(result, expectMag, self.otherInstFluxKeyName, "mag")

        # test calibrating all of the flux fields
        testCat = catalog.copy(deep=True)
        result = photoCalib.calibrateCatalog(testCat)
        checkCatalog(result, expectNanojansky, self.instFluxKeyName, "flux")
        checkCatalog(result, expectMag, self.instFluxKeyName, "mag")
        checkCatalog(result, expectNanojansky, self.otherInstFluxKeyName, "flux")
        checkCatalog(result, expectMag, self.otherInstFluxKeyName, "mag")
        self.assertFloatsAlmostEqual(result[self.noErrInstFluxKeyName+'_flux'], expectNanojansky[:, 0])
        self.assertFloatsAlmostEqual(result[self.noErrInstFluxKeyName+'_mag'], expectMag[:, 0])
        self.assertFloatsAlmostEqual(result[self.noErrInstFluxKeyName+'_instFlux'], origInstFlux)

    def testNonVarying(self):
        """Test constructing with a constant calibration factor."""
        photoCalib = lsst.afw.image.PhotoCalib(self.calibration)
        self._testPhotoCalibCenter(photoCalib, 0)

        # Test _isConstant
        self.assertTrue(photoCalib._isConstant)

        # test on positions off the center (position should not matter)
        self.assertEqual(self.flux1, photoCalib.instFluxToNanojansky(self.instFlux1, self.pointXShift))
        self.assertEqual(self.mag1, photoCalib.instFluxToMagnitude(self.instFlux1, self.pointXShift))
        result = photoCalib.instFluxToNanojansky(self.instFlux1, self.instFluxErr1)
        self.assertEqual(self.flux1, result.value)

        photoCalib = lsst.afw.image.PhotoCalib(self.calibration, self.calibrationErr)
        self._testPhotoCalibCenter(photoCalib, self.calibrationErr)

        # test converting to a photoCalib
        photoCalib = lsst.afw.image.PhotoCalib(self.calibration, bbox=self.bbox)
        self._testPhotoCalibCenter(photoCalib, 0)

    def testConstantBoundedField(self):
        """Test constructing with a spatially-constant bounded field."""
        photoCalib = lsst.afw.image.PhotoCalib(self.constantCalibration)
        self._testPhotoCalibCenter(photoCalib, 0)

        # test on positions off the center (position should not matter)
        self.assertEqual(self.flux1, photoCalib.instFluxToNanojansky(self.instFlux1, self.pointYShift))
        self.assertEqual(self.mag1, photoCalib.instFluxToMagnitude(self.instFlux1, self.pointYShift))
        self.assertFloatsAlmostEqual(self.flux2,
                                     photoCalib.instFluxToNanojansky(self.instFlux2, self.pointXShift))
        self.assertFloatsAlmostEqual(self.mag2,
                                     photoCalib.instFluxToMagnitude(self.instFlux2, self.pointXShift))

        # test converting to a photoCalib
        photoCalib = lsst.afw.image.PhotoCalib(self.constantCalibration, self.calibrationErr)
        self._testPhotoCalibCenter(photoCalib, self.calibrationErr)

        # test _isConstant (bounded field is not constant)
        self.assertFalse(photoCalib._isConstant)

    def testLinearXBoundedField(self):
        photoCalib = lsst.afw.image.PhotoCalib(self.linearXCalibration)
        self._testPhotoCalibCenter(photoCalib, 0)

        # test on positions off the center (Y position should not matter)
        self.assertEqual(self.flux1, photoCalib.instFluxToNanojansky(self.instFlux1, self.pointYShift))
        self.assertEqual(self.mag1, photoCalib.instFluxToMagnitude(self.instFlux1, self.pointYShift))

        # test on positions off the center (X position does matter)
        calibration = (self.calibration + self.pointXShift.getX()*self.calibration/(self.bbox.getWidth()/2.))
        expect = self.instFlux1*calibration
        self.assertFloatsAlmostEqual(expect,
                                     photoCalib.instFluxToNanojansky(self.instFlux1, self.pointXShift))
        self.assertFloatsAlmostEqual((expect*u.nJy).to_value(u.ABmag),
                                     photoCalib.instFluxToMagnitude(self.instFlux1, self.pointXShift))
        expect2 = self.instFlux2*calibration
        self.assertFloatsAlmostEqual(expect2,
                                     photoCalib.instFluxToNanojansky(self.instFlux2, self.pointXShift))
        self.assertFloatsAlmostEqual((expect2*u.nJy).to_value(u.ABmag),
                                     photoCalib.instFluxToMagnitude(self.instFlux2, self.pointXShift))

        # test converting to a photoCalib
        photoCalib = lsst.afw.image.PhotoCalib(self.linearXCalibration, self.calibrationErr)
        self._testPhotoCalibCenter(photoCalib, self.calibrationErr)

        # New catalog with a spatial component in the varying direction,
        # to ensure the calculations on a catalog properly handle non-constant BF.
        # NOTE: only the first quantity of the result (nJy or mags) should change.
        catalog = self.catalog.copy(deep=True)
        catalog[0].set('centroid_x', self.pointXShift[0])
        catalog[0].set('centroid_y', self.pointXShift[1])
        errFlux1 = computeNanojanskyErr(self.instFluxErr1,
                                        self.instFlux1,
                                        self.calibrationErr,
                                        calibration,
                                        expect)
        errMag1 = computeMagnitudeErr(self.instFluxErr1,
                                      self.instFlux1,
                                      self.calibrationErr,
                                      calibration,
                                      expect)
        # re-use the same instFluxErr1 for instFlux2.
        errFlux2 = computeNanojanskyErr(self.instFluxErr1,
                                        self.instFlux2,
                                        self.calibrationErr,
                                        self.calibration,
                                        self.flux2)
        errMag2 = computeMagnitudeErr(self.instFluxErr1,
                                      self.instFlux2,
                                      self.calibrationErr,
                                      self.calibration,
                                      self.flux2)
        expectNanojansky = np.array([[expect, errFlux1], [self.flux2, errFlux2]])
        expectMag = np.array([[(expect*u.nJy).to_value(u.ABmag), errMag1], [self.mag2, errMag2]])
        self._testSourceCatalog(photoCalib, catalog, expectNanojansky, expectMag)

    def testComputeScaledCalibration(self):
        photoCalib = lsst.afw.image.PhotoCalib(self.calibration, bbox=self.bbox)
        scaledCalib = lsst.afw.image.PhotoCalib(photoCalib.computeScaledCalibration())
        self.assertEqual(self.flux1,
                         scaledCalib.instFluxToNanojansky(self.instFlux1)*photoCalib.getCalibrationMean())
        self.assertEqual(photoCalib.instFluxToNanojansky(self.instFlux1),
                         scaledCalib.instFluxToNanojansky(self.instFlux1)*photoCalib.getCalibrationMean())

        photoCalib = lsst.afw.image.PhotoCalib(self.constantCalibration)
        scaledCalib = lsst.afw.image.PhotoCalib(photoCalib.computeScaledCalibration())

        self.assertEqual(self.flux1, scaledCalib.instFluxToNanojansky(self.instFlux1*self.calibration))
        self.assertEqual(photoCalib.instFluxToNanojansky(self.instFlux1),
                         scaledCalib.instFluxToNanojansky(self.instFlux1)*photoCalib.getCalibrationMean())

    @unittest.skip("Not yet implemented: see DM-10154")
    def testComputeScalingTo(self):
        photoCalib1 = lsst.afw.image.PhotoCalib(self.calibration, self.calibrationErr, bbox=self.bbox)
        photoCalib2 = lsst.afw.image.PhotoCalib(self.calibration*500, self.calibrationErr, bbox=self.bbox)
        scaling = photoCalib1.computeScalingTo(photoCalib2)(self.pointXShift)
        self.assertEqual(photoCalib1.instFluxToNanojansky(self.instFlux1, self.pointXShift)*scaling,
                         photoCalib2.instFluxToNanojansky(self.instFlux1, self.pointXShift))

        photoCalib3 = lsst.afw.image.PhotoCalib(self.constantCalibration, self.calibrationErr)
        scaling = photoCalib1.computeScalingTo(photoCalib3)(self.pointXShift)
        self.assertEqual(photoCalib1.instFluxToNanojansky(self.instFlux1, self.pointXShift)*scaling,
                         photoCalib3.instFluxToNanojansky(self.instFlux1, self.pointXShift))
        scaling = photoCalib3.computeScalingTo(photoCalib1)(self.pointXShift)
        self.assertEqual(photoCalib3.instFluxToNanojansky(self.instFlux1, self.pointXShift)*scaling,
                         photoCalib1.instFluxToNanojansky(self.instFlux1, self.pointXShift))

        photoCalib4 = lsst.afw.image.PhotoCalib(self.linearXCalibration, self.calibrationErr)
        scaling = photoCalib1.computeScalingTo(photoCalib4)(self.pointXShift)
        self.assertEqual(photoCalib1.instFluxToNanojansky(self.instFlux1, self.pointXShift)*scaling,
                         photoCalib4.instFluxToNanojansky(self.instFlux1, self.pointXShift))
        scaling = photoCalib4.computeScalingTo(photoCalib1)(self.pointXShift)
        self.assertEqual(photoCalib4.instFluxToNanojansky(self.instFlux1, self.pointXShift)*scaling,
                         photoCalib1.instFluxToNanojansky(self.instFlux1, self.pointXShift))

        # Don't allow division of BoundedFields with different bounding boxes
        photoCalibNoBBox = lsst.afw.image.PhotoCalib(self.calibration, self.calibrationErr)
        with self.assertRaises(lsst.pex.exceptions.DomainError):
            scaling = photoCalibNoBBox.computeScalingTo(photoCalib1)
        with self.assertRaises(lsst.pex.exceptions.DomainError):
            scaling = photoCalibNoBBox.computeScalingTo(photoCalib4)
        with self.assertRaises(lsst.pex.exceptions.DomainError):
            scaling = photoCalib1.computeScalingTo(photoCalibNoBBox)

    def _testPersistence(self, photoCalib):
        with lsst.utils.tests.getTempFilePath(".fits") as filename:
            photoCalib.writeFits(filename)
            result = lsst.afw.image.PhotoCalib.readFits(filename)
            self.assertEqual(result, photoCalib)

    def testPersistence(self):
        photoCalib = lsst.afw.image.PhotoCalib(self.calibration)
        self._testPersistence(photoCalib)

        photoCalib = lsst.afw.image.PhotoCalib(self.calibration, self.calibrationErr)
        self._testPersistence(photoCalib)

        photoCalib = lsst.afw.image.PhotoCalib(self.calibration, self.calibrationErr, self.bbox)
        self._testPersistence(photoCalib)

        photoCalib = lsst.afw.image.PhotoCalib(self.constantCalibration, self.calibrationErr)
        self._testPersistence(photoCalib)

        photoCalib = lsst.afw.image.PhotoCalib(self.linearXCalibration, self.calibrationErr)
        self._testPersistence(photoCalib)

    def testPersistenceVersions(self):
        """Test that different versions are handled appropriately."""
        # the values that were persisted in this photoCalib
        mean = 123
        err = 45
        dataDir = os.path.join(os.path.split(__file__)[0], "data")

        # implicit version 0 should raise (no longer compatible)
        filePath = os.path.join(dataDir, "photoCalib-noversion.fits")
        with self.assertRaises(RuntimeError):
            photoCalib = lsst.afw.image.PhotoCalib.readFits(filePath)

        # explicit version 0 should raise (no longer compatible)
        filePath = os.path.join(dataDir, "photoCalib-version0.fits")
        with self.assertRaises(RuntimeError):
            photoCalib = lsst.afw.image.PhotoCalib.readFits(filePath)

        # explicit version 1
        filePath = os.path.join(dataDir, "photoCalib-version1.fits")
        photoCalib = lsst.afw.image.PhotoCalib.readFits(filePath)
        self.assertEqual(photoCalib.getCalibrationMean(), mean)
        self.assertEqual(photoCalib.getCalibrationErr(), err)

    def testPhotoCalibEquality(self):
        photoCalib1 = lsst.afw.image.PhotoCalib(self.linearXCalibration, 0.5)
        photoCalib2 = lsst.afw.image.PhotoCalib(self.linearXCalibration, 0.5)
        photoCalib3 = lsst.afw.image.PhotoCalib(5, 0.5)
        photoCalib4 = lsst.afw.image.PhotoCalib(5, 0.5)
        photoCalib5 = lsst.afw.image.PhotoCalib(5)
        photoCalib6 = lsst.afw.image.PhotoCalib(self.linearXCalibration)
        photoCalib7 = lsst.afw.image.PhotoCalib(self.calibration, 0.5)
        photoCalib8 = lsst.afw.image.PhotoCalib(self.constantCalibration, 0.5)

        constantCalibration = lsst.afw.math.ChebyshevBoundedField(self.bbox, np.array([[self.calibration]]))
        photoCalib9 = lsst.afw.image.PhotoCalib(constantCalibration, 0.5)

        self.assertEqual(photoCalib1, photoCalib1)
        self.assertEqual(photoCalib1, photoCalib2)
        self.assertEqual(photoCalib3, photoCalib4)
        self.assertEqual(photoCalib8, photoCalib9)

        self.assertNotEqual(photoCalib1, photoCalib6)
        self.assertNotEqual(photoCalib1, photoCalib7)
        self.assertNotEqual(photoCalib1, photoCalib3)
        self.assertNotEqual(photoCalib3, photoCalib5)
        self.assertNotEqual(photoCalib1, photoCalib8)

        self.assertFalse(photoCalib1 != photoCalib2)  # using assertFalse to directly test != operator

    def setupImage(self):
        dim = (5, 6)
        npDim = (dim[1], dim[0])  # numpy and afw have a different x/y order
        sigma = 10.0
        # An image with increasing pixel values, to more easily check the
        # values in specific calculations.
        image = np.arange(dim[0]*dim[1]).astype(np.float32).reshape(npDim) + 1000
        mask = np.zeros(npDim, dtype=np.int32)
        variance = np.ones(npDim, dtype=np.float32)*sigma
        maskedImage = lsst.afw.image.makeMaskedImageFromArrays(image, mask, variance)
        maskedImage.mask[0, 0] = True  # set one mask bit to check propagation of mask bits.

        return npDim, maskedImage, image, mask, variance

    def testCalibrateImageConstant(self):
        """Test a spatially-constant calibration."""
        npDim, maskedImage, image, mask, variance = self.setupImage()
        outImage = maskedImage.image.getArray()*self.calibration
        expect = makeCalibratedMaskedImage(image, mask, variance, outImage,
                                           self.calibration, self.calibrationErr)
        photoCalib = lsst.afw.image.PhotoCalib(self.calibration, self.calibrationErr)
        result = photoCalib.calibrateImage(maskedImage)
        self.assertMaskedImagesAlmostEqual(expect, result)
        uncalibrated = photoCalib.uncalibrateImage(result)
        self.assertMaskedImagesAlmostEqual(maskedImage, uncalibrated)

        # same test, but without using the calibration error
        expect = makeCalibratedMaskedImageNoCalibrationError(image, mask, variance, outImage)
        result = photoCalib.calibrateImage(maskedImage, includeScaleUncertainty=False)
        self.assertMaskedImagesAlmostEqual(expect, result)
        uncalibrated = photoCalib.uncalibrateImage(result, includeScaleUncertainty=False)
        self.assertMaskedImagesAlmostEqual(maskedImage, uncalibrated)

    def testCalibrateImageNonConstant(self):
        """Test a spatially-varying calibration."""
        npDim, maskedImage, image, mask, variance = self.setupImage()
        xIndex, yIndex = np.indices(npDim, dtype=np.float64)
        # y then x, as afw order and np order are flipped
        calibration = self.linearXCalibration.evaluate(yIndex.flatten(), xIndex.flatten()).reshape(npDim)
        outImage = maskedImage.image.getArray()*calibration  # element-wise product, not matrix product
        expect = makeCalibratedMaskedImage(image, mask, variance, outImage, calibration, self.calibrationErr)

        photoCalib = lsst.afw.image.PhotoCalib(self.linearXCalibration, self.calibrationErr)
        result = photoCalib.calibrateImage(maskedImage)
        self.assertMaskedImagesAlmostEqual(expect, result)
        uncalibrated = photoCalib.uncalibrateImage(result)
        self.assertMaskedImagesAlmostEqual(maskedImage, uncalibrated)

        # same test, but without using the calibration error
        expect = makeCalibratedMaskedImageNoCalibrationError(image, mask, variance, outImage)
        result = photoCalib.calibrateImage(maskedImage, includeScaleUncertainty=False)
        self.assertMaskedImagesAlmostEqual(expect, result)
        uncalibrated = photoCalib.uncalibrateImage(result, includeScaleUncertainty=False)
        self.assertMaskedImagesAlmostEqual(maskedImage, uncalibrated)

    def testCalibrateImageNonConstantSubimage(self):
        """Test a non-constant calibration on a sub-image, to ensure we're
        handling xy0 correctly.
        """
        npDim, maskedImage, image, mask, variance = self.setupImage()
        xIndex, yIndex = np.indices(npDim, dtype=np.float64)
        calibration = self.linearXCalibration.evaluate(yIndex.flatten(), xIndex.flatten()).reshape(npDim)

        outImage = maskedImage.image.getArray()*calibration  # element-wise product, not matrix product
        expect = makeCalibratedMaskedImage(image, mask, variance, outImage, calibration, self.calibrationErr)

        subBox = lsst.geom.Box2I(lsst.geom.Point2I(2, 4), lsst.geom.Point2I(4, 5))
        subImage = maskedImage.subset(subBox)
        photoCalib = lsst.afw.image.PhotoCalib(self.linearXCalibration, self.calibrationErr)
        result = photoCalib.calibrateImage(subImage)
        self.assertMaskedImagesAlmostEqual(expect.subset(subBox), result)
        uncalibrated = photoCalib.uncalibrateImage(result)
        self.assertMaskedImagesAlmostEqual(subImage, uncalibrated)

        # same test, but without using the calibration error
        expect = makeCalibratedMaskedImageNoCalibrationError(image, mask, variance, outImage)
        result = photoCalib.calibrateImage(subImage, includeScaleUncertainty=False)
        self.assertMaskedImagesAlmostEqual(expect.subset(subBox), result)
        uncalibrated = photoCalib.uncalibrateImage(result, includeScaleUncertainty=False)
        self.assertMaskedImagesAlmostEqual(subImage, uncalibrated)

    def testNonPositiveMeans(self):
        # no negative calibrations
        with self.assertRaises(lsst.pex.exceptions.InvalidParameterError):
            lsst.afw.image.PhotoCalib(-1.0)
        # no negative errors
        with self.assertRaises(lsst.pex.exceptions.InvalidParameterError):
            lsst.afw.image.PhotoCalib(1.0, -1.0)

        # no negative calibration mean when computed from the bounded field
        negativeCalibration = lsst.afw.math.ChebyshevBoundedField(self.bbox,
                                                                  np.array([[-self.calibration]]))
        with self.assertRaises(lsst.pex.exceptions.InvalidParameterError):
            lsst.afw.image.PhotoCalib(negativeCalibration)
        # no negative calibration error
        with self.assertRaises(lsst.pex.exceptions.InvalidParameterError):
            lsst.afw.image.PhotoCalib(self.constantCalibration, -1.0)

        # no negative explicit calibration mean
        with self.assertRaises(lsst.pex.exceptions.InvalidParameterError):
            lsst.afw.image.PhotoCalib(-1.0, 0, self.constantCalibration, True)
        # no negative calibration error
        with self.assertRaises(lsst.pex.exceptions.InvalidParameterError):
            lsst.afw.image.PhotoCalib(1.0, -1.0, self.constantCalibration, True)

    def testPositiveErrors(self):
        """The errors should always be positive, regardless of whether the
        input flux is negative (as can happen in difference imaging).
        This tests and fixes tickets/DM-16696.
        """
        photoCalib = lsst.afw.image.PhotoCalib(self.calibration)
        result = photoCalib.instFluxToNanojansky(-100, 10)
        self.assertGreater(result.error, 0)

    def testMakePhotoCalibFromMetadata(self):
        """Test creating a PhotoCalib from the Calib FITS metadata.
        """
        fluxMag0 = 12345
        metadata = lsst.daf.base.PropertySet()
        metadata.set('FLUXMAG0', fluxMag0)

        photoCalib = lsst.afw.image.makePhotoCalibFromMetadata(metadata)
        self.assertEqual(photoCalib.getInstFluxAtZeroMagnitude(), fluxMag0)
        self.assertEqual(photoCalib.getCalibrationErr(), 0.0)
        # keys aren't deleted by default
        self.assertIn('FLUXMAG0', metadata)

        # Test reading with the error keyword
        fluxMag0Err = 6789
        metadata.set('FLUXMAG0ERR', fluxMag0Err)
        # The reference flux is "nanoJanskys at 0 magnitude".
        referenceFlux = (0*u.ABmag).to_value(u.nJy)
        calibrationErr = referenceFlux*fluxMag0Err/fluxMag0**2
        photoCalib = lsst.afw.image.makePhotoCalibFromMetadata(metadata)
        self.assertEqual(photoCalib.getInstFluxAtZeroMagnitude(), fluxMag0)
        self.assertFloatsAlmostEqual(photoCalib.getCalibrationErr(), calibrationErr)
        # keys aren't deleted by default
        self.assertIn('FLUXMAG0', metadata)
        self.assertIn('FLUXMAG0ERR', metadata)

        # test stripping keys from a new metadata
        metadata = lsst.daf.base.PropertySet()
        metadata.set('FLUXMAG0', fluxMag0)
        photoCalib = lsst.afw.image.makePhotoCalibFromMetadata(metadata, strip=True)
        self.assertEqual(photoCalib.getInstFluxAtZeroMagnitude(), fluxMag0)
        self.assertEqual(photoCalib.getCalibrationErr(), 0.0)
        self.assertNotIn('FLUXMAG0', metadata)

        metadata.set('FLUXMAG0', fluxMag0)
        metadata.set('FLUXMAG0ERR', fluxMag0Err)
        photoCalib = lsst.afw.image.makePhotoCalibFromMetadata(metadata, strip=True)
        self.assertEqual(photoCalib.getInstFluxAtZeroMagnitude(), fluxMag0)
        self.assertFloatsAlmostEqual(photoCalib.getCalibrationErr(), calibrationErr)
        self.assertNotIn('FLUXMAG0', metadata)
        self.assertNotIn('FLUXMAG0ERR', metadata)

    def testMakePhotoCalibFromMetadataNoKey(self):
        """Return None if the metadata does not contain a 'FLUXMAG0' key."""
        metadata = lsst.daf.base.PropertySet()
        metadata.set('something', 1000)
        metadata.set('FLUXMAG0ERR', 5)
        result = lsst.afw.image.makePhotoCalibFromMetadata(metadata)
        self.assertIsNone(result)

    def testMakePhotoCalibFromCalibZeroPoint(self):
        """Test creating from the Calib-style fluxMag0/fluxMag0Err values."""
        fluxMag0 = 12345
        fluxMag0Err = 67890

        referenceFlux = (0*u.ABmag).to_value(u.nJy)
        calibrationErr = referenceFlux*fluxMag0Err/fluxMag0**2

        # create with all zeros
        photoCalib = lsst.afw.image.makePhotoCalibFromCalibZeroPoint(0, 0)
        self.assertEqual(photoCalib.getInstFluxAtZeroMagnitude(), 0)
        self.assertEqual(photoCalib.getCalibrationMean(), np.inf)
        self.assertTrue(np.isnan(photoCalib.getCalibrationErr()))

        # create with non-zero fluxMag0, but zero err
        photoCalib = lsst.afw.image.makePhotoCalibFromCalibZeroPoint(fluxMag0, 0)
        self.assertEqual(photoCalib.getInstFluxAtZeroMagnitude(), fluxMag0)
        self.assertEqual(photoCalib.getCalibrationErr(), 0.0)

        # create with non-zero fluxMag0 and err
        photoCalib = lsst.afw.image.makePhotoCalibFromCalibZeroPoint(fluxMag0, fluxMag0Err)
        self.assertEqual(photoCalib.getInstFluxAtZeroMagnitude(), fluxMag0)
        self.assertFloatsAlmostEqual(photoCalib.getCalibrationErr(), calibrationErr)


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
