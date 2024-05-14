# This file is part of afw.
#
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (http://www.lsst.org).
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import unittest

import os
import numpy as np
import astropy.io.fits

import lsst.utils.tests
from lsst.daf.base import PropertyList
from lsst.geom import Box2I, Point2I, Extent2I, Point2D, Box2D, SpherePoint, degrees
from lsst.afw.geom import makeSkyWcs, Polygon
from lsst.afw.table import ExposureTable
from lsst.afw.image import (Image, Mask, MaskedImage, Exposure, LOCAL, PARENT, MaskPixel, VariancePixel,
                            ImageFitsReader, MaskFitsReader, MaskedImageFitsReader, ExposureFitsReader,
                            FilterLabel, PhotoCalib, ApCorrMap, VisitInfo, TransmissionCurve,
                            CoaddInputs, ExposureInfo, ExposureF)
from lsst.afw.detection import GaussianPsf
from lsst.afw.cameraGeom.testUtils import DetectorWrapper

TESTDIR = os.path.abspath(os.path.dirname(__file__))


class FitsReaderTestCase(lsst.utils.tests.TestCase):

    def setUp(self):
        self.dtypes = [np.dtype(t) for t in (np.uint16, np.int32, np.float32, np.float64)]
        self.bbox = Box2I(Point2I(2, 1), Extent2I(5, 7))
        self.args = [
            (),
            (Box2I(Point2I(3, 4), Extent2I(2, 1)),),
            (Box2I(Point2I(3, 4), Extent2I(2, 1)), PARENT),
            (Box2I(Point2I(1, 0), Extent2I(3, 2)), LOCAL),
        ]

    def testImageFitsReader(self):
        for n, dtypeIn in enumerate(self.dtypes):
            with self.subTest(dtypeIn=dtypeIn):
                imageIn = Image(self.bbox, dtype=dtypeIn)
                imageIn.array[:, :] = np.random.randint(low=1, high=5, size=imageIn.array.shape)
                with lsst.utils.tests.getTempFilePath(".fits") as fileName:
                    imageIn.writeFits(fileName)
                    reader = ImageFitsReader(fileName)
                    self.assertEqual(reader.readBBox(), self.bbox)
                    self.assertEqual(reader.readDType(), dtypeIn)
                    self.assertEqual(reader.fileName, fileName)
                    for args in self.args:
                        with self.subTest(args=args):
                            array1 = reader.readArray(*args)
                            image1 = reader.read(*args)
                            subIn = imageIn.subset(*args) if args else imageIn
                            self.assertEqual(dtypeIn, array1.dtype)
                            self.assertTrue(np.all(subIn.array == array1))
                            self.assertEqual(subIn.getXY0(), reader.readXY0(*args))
                            self.assertImagesEqual(subIn, image1)
                    for dtype2 in self.dtypes[n:]:
                        for args in self.args:
                            with self.subTest(dtype2=dtype2, args=args):
                                subIn = imageIn.subset(*args) if args else imageIn
                                array2 = reader.readArray(*args, dtype=dtype2)
                                image2 = reader.read(*args, dtype=dtype2)
                                self.assertEqual(dtype2, array2.dtype)
                                self.assertTrue(np.all(subIn.array == array2))
                                self.assertEqual(subIn.getXY0(), reader.readXY0(*args))
                                self.assertEqual(subIn.getBBox(), image2.getBBox())
                                self.assertTrue(np.all(image2.array == array2))

    def testMaskFitsReader(self):
        maskIn = Mask(self.bbox, dtype=MaskPixel)
        maskIn.array[:, :] = np.random.randint(low=1, high=5, size=maskIn.array.shape)
        with lsst.utils.tests.getTempFilePath(".fits") as fileName:
            maskIn.writeFits(fileName)
            reader = MaskFitsReader(fileName)
            self.assertEqual(reader.readBBox(), self.bbox)
            self.assertEqual(reader.readDType(), MaskPixel)
            self.assertEqual(reader.fileName, fileName)
            for args in self.args:
                with self.subTest(args=args):
                    array = reader.readArray(*args)
                    mask = reader.read(*args)
                    subIn = maskIn.subset(*args) if args else maskIn
                    self.assertEqual(MaskPixel, array.dtype)
                    self.assertTrue(np.all(subIn.array == array))
                    self.assertEqual(subIn.getXY0(), reader.readXY0(*args))
                    self.assertImagesEqual(subIn, mask)

    def testMaskedImageFitsReader(self):
        for n, dtypeIn in enumerate(self.dtypes):
            with self.subTest(dtypeIn=dtypeIn):
                maskedImageIn = MaskedImage(self.bbox, dtype=dtypeIn)
                maskedImageIn.image.array[:, :] = np.random.randint(low=1, high=5,
                                                                    size=maskedImageIn.image.array.shape
                                                                    )
                maskedImageIn.mask.array[:, :] = np.random.randint(low=1, high=5,
                                                                   size=maskedImageIn.mask.array.shape
                                                                   )
                maskedImageIn.variance.array[:, :] = np.random.randint(low=1, high=5,
                                                                       size=maskedImageIn.variance.array.shape
                                                                       )
                with lsst.utils.tests.getTempFilePath(".fits") as fileName:
                    maskedImageIn.writeFits(fileName)
                    reader = MaskedImageFitsReader(fileName)
                    self.assertEqual(reader.readBBox(), self.bbox)
                    self.assertEqual(reader.readImageDType(), dtypeIn)
                    self.assertEqual(reader.fileName, fileName)
                    self.checkMultiPlaneReader(reader, maskedImageIn, fileName, self.dtypes[n:],
                                               compare=self.assertMaskedImagesEqual)
                    self.checkMaskedImageFitsReader(maskedImageIn, fileName, self.dtypes[n:])

    def checkMultiPlaneReader(self, reader, objectIn, fileName, dtypesOut, compare):
        """Test operations common to MaskedImageFitsReader and ExposureFitsReader.

        Parameters
        ----------
        reader : `MaskedImageFitsReader` or `ExposureFitsReader` instance
            Reader object to test.
        objectIn : `MaskedImage` or `Exposure`
            Object originally saved, to compare against.
        fileName : `str`
            Name of the file the reader is reading.
        dtypesOut : sequence of `numpy.dype`
            Compatible image pixel types to try to read in.
        compare : callable
            Callable that compares objects of the same type as objectIn and
            asserts if they are not equal.
        """
        dtypeIn = objectIn.image.dtype
        self.assertEqual(reader.readBBox(), self.bbox)
        self.assertEqual(reader.readImageDType(), dtypeIn)
        self.assertEqual(reader.readMaskDType(), MaskPixel)
        self.assertEqual(reader.readVarianceDType(), VariancePixel)
        self.assertEqual(reader.fileName, fileName)
        for args in self.args:
            with self.subTest(args=args):
                object1 = reader.read(*args)
                subIn = objectIn.subset(*args) if args else objectIn
                self.assertEqual(object1.image.array.dtype, dtypeIn)
                self.assertEqual(object1.mask.array.dtype, MaskPixel)
                self.assertEqual(object1.variance.array.dtype, VariancePixel)
                self.assertImagesEqual(subIn.image, reader.readImage(*args))
                self.assertImagesEqual(subIn.mask, reader.readMask(*args))
                self.assertImagesEqual(subIn.variance, reader.readVariance(*args))
                compare(subIn, object1)
                for dtype2 in dtypesOut:
                    with self.subTest(dtype2=dtype2, args=args):
                        object2 = reader.read(*args, dtype=dtype2)
                        image2 = reader.readImage(*args, dtype=dtype2)
                        self.assertEqual(object2.image.array.dtype, dtype2)
                        self.assertEqual(object2.mask.array.dtype, MaskPixel)
                        self.assertEqual(object2.variance.array.dtype, VariancePixel)
                        self.assertImagesEqual(subIn.image, Image(image2, deep=True, dtype=dtypeIn))
                        self.assertImagesEqual(image2, object2.image)
                        compare(subIn, object2)

    def checkMaskedImageFitsReader(self, maskedImageIn, fileName, dtypesOut):
        """Test MaskedImageFitsReader.

        Parameters
        ----------
        maskedImageIn : `MaskedImage`
            Object originally saved, to compare against.
        fileName : `str`
            Name of the file the reader is reading.
        dtypesOut : sequence of `numpy.dype`
            Compatible image pixel types to try to read in.
        """
        reader = MaskedImageFitsReader(fileName)
        self.checkMultiPlaneReader(reader, maskedImageIn, fileName, dtypesOut,
                                   compare=self.assertMaskedImagesEqual)
        
    def checkExposureFitsReader(self, exposureIn, fileName, dtypesOut):
        """Test ExposureFitsReader.

        Parameters
        ----------
        exposureIn : `Exposure`
            Object originally saved, to compare against.
        fileName : `str`
            Name of the file the reader is reading.
        dtypesOut : sequence of `numpy.dype`
            Compatible image pixel types to try to read in.
        """
        reader = ExposureFitsReader(fileName)
        print(fileName)
        self.assertIn('EXPINFO_V', reader.readMetadata().toDict(), "metadata is automatically versioned")
        reader.readMetadata().remove('EXPINFO_V')
        # ensure EXTNAMEs can be read and make sense
        extnames = set(('PRIMARY', 'IMAGE', 'MASK', 'VARIANCE', 'ARCHIVE_INDEX',
                        'Detector', 'TransformMap', 'TransformPoint2ToPoint2',
                        'FilterLabel', 'SkyWcs', 'ApCorrMap', 'PhotoCalib',
                        'ChebyshevBoundedField', 'CoaddInputs', 'GaussianPsf',
                        'Polygon', 'VisitInfo'))
        with astropy.io.fits.open(fileName) as astropyReadFile:
            for hdu in astropyReadFile:
                self.assertIn(hdu.name, extnames)
        self.assertIn('EXTNAME', reader.readMetadata().toDict(), "EXTNAME is added upon writing")
        reader.readMetadata().remove('EXTNAME')
        self.assertGreaterEqual(reader.readSerializationVersion(), 0)
        self.assertEqual(exposureIn.info.id, reader.readExposureId())
        self.assertEqual(exposureIn.getMetadata().toDict(), reader.readMetadata().toDict())
        self.assertWcsAlmostEqualOverBBox(exposureIn.getWcs(), reader.readWcs(), self.bbox,
                                          maxDiffPix=0, maxDiffSky=0*degrees)
        self.assertWcsAlmostEqualOverBBox(exposureIn.getWcs(),
                                          reader.readComponent(ExposureInfo.KEY_WCS),
                                          self.bbox,
                                          maxDiffPix=0, maxDiffSky=0*degrees)
        self.assertEqual(exposureIn.getFilter(), reader.readFilter())
        self.assertEqual(exposureIn.getFilter(),
                         reader.readComponent(ExposureInfo.KEY_FILTER))
        self.assertEqual(exposureIn.getPhotoCalib(), reader.readPhotoCalib())
        self.assertEqual(exposureIn.getPhotoCalib(),
                         reader.readComponent(ExposureInfo.KEY_PHOTO_CALIB))
        center = exposureIn.getBBox().getCenter()
        self.assertImagesEqual(exposureIn.getPsf().computeImage(center),
                               reader.readPsf().computeImage(center))
        self.assertImagesEqual(exposureIn.getPsf().computeImage(center),
                               reader.readComponent('PSF').computeImage(center))
        self.assertEqual(exposureIn.getInfo().getValidPolygon(), reader.readValidPolygon())
        self.assertEqual(exposureIn.getInfo().getValidPolygon(),
                         reader.readComponent(ExposureInfo.KEY_VALID_POLYGON))
        self.assertCountEqual(exposureIn.getInfo().getApCorrMap(), reader.readApCorrMap())
        self.assertCountEqual(exposureIn.getInfo().getApCorrMap(),
                              reader.readComponent(ExposureInfo.KEY_AP_CORR_MAP))
        self.assertEqual(exposureIn.getInfo().getVisitInfo().getExposureTime(),
                         reader.readVisitInfo().getExposureTime())
        point = Point2D(2.3, 3.1)
        wavelengths = np.linspace(4000, 5000, 5)
        self.assertFloatsEqual(exposureIn.getInfo().getTransmissionCurve().sampleAt(point, wavelengths),
                               reader.readTransmissionCurve().sampleAt(point, wavelengths))
        # Note: readComponent(ExposureInfo.KEY_TRANSMISSION_CURVE) returns a generic Storable
        # rather than a TransmissionCurve object.

        # Because we persisted the same instances, we should get back the same
        # instances for *archive* components, and hence equality comparisons
        # should work even if it just amounts to C++ pointer equality.
        record = reader.readCoaddInputs().ccds[0]

        self.assertEqual(record.getWcs(), reader.readWcs())
        #print((record.getPsf()))
        #print((reader.readPsf()))
        a = record.getPsf()
        b = reader.readPsf()
        print(id(a),id(b))
        print(record.getPsf(), reader.readPsf())
        #self.assertEqual(a, b)
        self.assertEqual(record.getValidPolygon(), reader.readValidPolygon())
        #self.assertEqual(record.getApCorrMap(), reader.readApCorrMap())
        self.assertEqual(record.getPhotoCalib(), reader.readPhotoCalib())
        self.assertEqual(record.getDetector(), reader.readDetector())
        self.checkMultiPlaneReader(
            reader, exposureIn, fileName, dtypesOut,
            compare=lambda a, b: self.assertMaskedImagesEqual(a.maskedImage, b.maskedImage)
        )

    def testCompressedSinglePlaneExposureFitsReader(self):
        """Test that a compressed single plane image can be read as exposure.
        """
        uncompressed_file = os.path.join(TESTDIR, "data", "ticketdm26260.fits")
        compressed_file = os.path.join(TESTDIR, "data", "ticketdm26260.fits.fz")
        uncompressed = ExposureFitsReader(uncompressed_file).read()
        compressed = ExposureFitsReader(compressed_file).read()

        self.assertMaskedImagesEqual(uncompressed.maskedImage, compressed.maskedImage)

    def testMultiPlaneFitsReaders(self):
        """Run tests for MaskedImageFitsReader and ExposureFitsReader.
        """
        metadata = PropertyList()
        metadata.add("FIVE", 5)
        metadata.add("SIX", 6.0)
        wcs = makeSkyWcs(Point2D(2.5, 3.75), SpherePoint(40.0*degrees, 50.0*degrees),
                         np.array([[1E-5, 0.0], [0.0, -1E-5]]))
        calib = PhotoCalib(2.5E4)
        psf = GaussianPsf(21, 21, 8.0)
        polygon = Polygon(Box2D(self.bbox))
        apCorrMap = ApCorrMap()
        visitInfo = VisitInfo(exposureTime=5.0)
        transmissionCurve = TransmissionCurve.makeIdentity()
        coaddInputs = CoaddInputs(ExposureTable.makeMinimalSchema(), ExposureTable.makeMinimalSchema())
        detector = DetectorWrapper().detector
        record = coaddInputs.ccds.addNew()
        record.setWcs(wcs)
        record.setPhotoCalib(calib)
        record.setPsf(psf)
        record.setValidPolygon(polygon)
        record.setApCorrMap(apCorrMap)
        record.setVisitInfo(visitInfo)
        record.setTransmissionCurve(transmissionCurve)
        record.setDetector(detector)
        for n, dtypeIn in enumerate(self.dtypes):
            with self.subTest(dtypeIn=dtypeIn):
                exposureIn = Exposure(self.bbox, dtype=dtypeIn)
                shape = exposureIn.image.array.shape
                exposureIn.image.array[:, :] = np.random.randint(low=1, high=5, size=shape)
                exposureIn.mask.array[:, :] = np.random.randint(low=1, high=5, size=shape)
                exposureIn.variance.array[:, :] = np.random.randint(low=1, high=5, size=shape)
                exposureIn.setMetadata(metadata)
                exposureIn.setWcs(wcs)
                exposureIn.setFilter(FilterLabel(physical="test_readers_filter"))
                exposureIn.setPhotoCalib(calib)
                exposureIn.setPsf(psf)
                exposureIn.getInfo().setValidPolygon(polygon)
                exposureIn.getInfo().setApCorrMap(apCorrMap)
                exposureIn.getInfo().setVisitInfo(visitInfo)
                exposureIn.getInfo().setTransmissionCurve(transmissionCurve)
                exposureIn.getInfo().setCoaddInputs(coaddInputs)
                exposureIn.setDetector(detector)
                with lsst.utils.tests.getTempFilePath(".fits") as fileName:
                    exposureIn.writeFits(fileName)
                    self.checkMaskedImageFitsReader(exposureIn.maskedImage, fileName, self.dtypes[n:])
                    self.checkExposureFitsReader(exposureIn, fileName, self.dtypes[n:])

    def test31035(self):
        """Test that illegal values in the header can be round-tripped."""
        with lsst.utils.tests.getTempFilePath(".fits") as fileName:
            exp = ExposureF(width=100, height=100)
            md = exp.getMetadata()
            md['BORE-RA'] = 'NaN'
            md['BORE-DEC'] = 'NaN'
            md['BORE-AZ'] = 'NaN'
            md['BORE-ALT'] = 'NaN'
            md['BORE-AIRMASS'] = 'NaN'
            md['BORE-ROTANG'] = 'NaN'
            md['OBS-LONG'] = 'NaN'
            md['OBS-LAT'] = 'NaN'
            md['OBS-ELEV'] = 'NaN'
            md['AIRTEMP'] = 'NaN'
            md['AIRPRESS'] = 'NaN'
            md['HUMIDITY'] = 'NaN'

            exp.writeFits(fileName)

            _ = ExposureF.readFits(fileName)


class TestMemory(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    import sys
    setup_module(sys.modules[__name__])
    unittest.main()
