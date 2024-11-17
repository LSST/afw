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
import os

import astshim as ast
import numpy as np

import lsst.utils.tests
import lsst.pex.exceptions as pexExcept
import lsst.geom
import lsst.afw.image as afwImage
import lsst.afw.display as afwDisplay
from lsst.afw.cameraGeom import (
    AmplifierIsolator,
    assembleAmplifierImage,
    assembleAmplifierRawImage,
    Camera,
    CameraSys,
    CameraSysPrefix,
    Detector,
    DetectorCollection,
    FIELD_ANGLE,
    FOCAL_PLANE,
    makeUpdatedDetector,
    Orientation,
    PIXELS,
)
import lsst.afw.cameraGeom.testUtils as testUtils
import lsst.afw.cameraGeom.utils as cameraGeomUtils
from lsst.afw.geom import TransformPoint2ToPoint2

try:
    type(display)
except NameError:
    display = False

testPath = os.path.abspath(os.path.dirname(__file__))


class CameraGeomTestCase(lsst.utils.tests.TestCase):
    """A test case for camera geometry"""

    def setUp(self):
        self.lsstCamWrapper = testUtils.CameraWrapper(isLsstLike=True)
        self.scCamWrapper = testUtils.CameraWrapper(isLsstLike=False)
        self.cameraList = (self.lsstCamWrapper, self.scCamWrapper)
        self.assemblyList = {}
        self.assemblyList[self.lsstCamWrapper.camera.getName()] =\
            [afwImage.ImageU(os.path.join(testPath, 'test_amp.fits.gz'))
             for i in range(8)]
        self.assemblyList[self.scCamWrapper.camera.getName()] =\
            [afwImage.ImageU(os.path.join(testPath, 'test.fits.gz'), allowUnsafe=True)]

    def tearDown(self):
        del self.lsstCamWrapper
        del self.scCamWrapper
        del self.cameraList
        del self.assemblyList

    def testConstructor(self):
        for cw in self.cameraList:
            self.assertIsInstance(cw.camera, Camera)
            self.assertEqual(cw.nDetectors, len(cw.camera))
            self.assertEqual(cw.nDetectors, len(cw.ampDataDict))
            self.assertEqual(sorted(cw.detectorNameList),
                             sorted(cw.camera.getNameIter()))
            self.assertEqual(sorted(cw.detectorIdList),
                             sorted(cw.camera.getIdIter()))
            for det in cw.camera:
                self.assertIsInstance(det, Detector)
                self.assertEqual(
                    cw.ampDataDict[det.getName()]['namps'], len(det))
            idList = [det.getId() for det in cw.camera]
            self.assertEqual(idList, sorted(idList))

    def testCameraSysRepr(self):
        """Test CameraSys.__repr__ and CameraSysPrefix.__repr__
        """
        for sysName in ("FocalPlane", "FieldAngle", "Pixels", "foo"):
            cameraSys = CameraSys(sysName)
            predRepr = f"CameraSys({sysName})"
            self.assertEqual(repr(cameraSys), predRepr)

            cameraSysPrefix = CameraSysPrefix(sysName)
            predCSPRepr = f"CameraSysPrefix({sysName})"
            self.assertEqual(repr(cameraSysPrefix), predCSPRepr)
            for detectorName in ("Detector 1", "bar"):
                cameraSys2 = CameraSys(sysName, detectorName)
                predRepr2 = f"CameraSys({sysName}, {detectorName})"
                self.assertEqual(repr(cameraSys2), predRepr2)

    def testAccessor(self):
        for cw in self.cameraList:
            camera = cw.camera
            for name in cw.detectorNameList:
                self.assertIsInstance(camera[name], Detector)
            for detId in cw.detectorIdList:
                self.assertIsInstance(camera[detId], Detector)

    def testTransformSlalib(self):
        """Test Camera.transform against data computed using SLALIB

        These test data come from SLALIB using SLA_PCD with 0.925 and
        a plate scale of 20 arcsec/mm
        """
        testData = [(-1.84000000, 1.04000000, -331.61689069, 187.43563387),
                    (-1.64000000, 0.12000000, -295.42491556, 21.61645724),
                    (-1.44000000, -0.80000000, -259.39818797, -144.11010443),
                    (-1.24000000, -1.72000000, -223.48275934, -309.99221457),
                    (-1.08000000, 1.36000000, -194.56520533, 245.00803635),
                    (-0.88000000, 0.44000000, -158.44320430, 79.22160215),
                    (-0.68000000, -0.48000000, -122.42389383, -86.41686623),
                    (-0.48000000, -1.40000000, -86.45332534, -252.15553224),
                    (-0.32000000, 1.68000000, -57.64746955, 302.64921514),
                    (-0.12000000, 0.76000000, -21.60360306, 136.82281940),
                    (0.08000000, -0.16000000, 14.40012984, -28.80025968),
                    (0.28000000, -1.08000000, 50.41767773, -194.46818554),
                    (0.48000000, -2.00000000, 86.50298919, -360.42912163),
                    (0.64000000, 1.08000000, 115.25115701, 194.48632746),
                    (0.84000000, 0.16000000, 151.23115189, 28.80593369),
                    (1.04000000, -0.76000000, 187.28751874, -136.86395600),
                    (1.24000000, -1.68000000, 223.47420612, -302.77150507),
                    (1.40000000, 1.40000000, 252.27834478, 252.27834478),
                    (1.60000000, 0.48000000, 288.22644118, 86.46793236),
                    (1.80000000, -0.44000000, 324.31346653, -79.27662515), ]

        for cw in self.cameraList:
            camera = cw.camera
            for point in testData:
                fpGivenPos = lsst.geom.Point2D(point[2], point[3])
                fieldGivenPos = lsst.geom.Point2D(
                    lsst.geom.degToRad(point[0]), lsst.geom.degToRad(point[1]))

                fieldAngleToFocalPlane = camera.getTransform(FIELD_ANGLE, FOCAL_PLANE)
                fpComputedPos = fieldAngleToFocalPlane.applyForward(fieldGivenPos)
                self.assertPairsAlmostEqual(fpComputedPos, fpGivenPos)

                focalPlaneToFieldAngle = camera.getTransform(FOCAL_PLANE, FIELD_ANGLE)
                fieldComputedPos = focalPlaneToFieldAngle.applyForward(fpGivenPos)
                self.assertPairsAlmostEqual(fieldComputedPos, fieldGivenPos)

    def testTransformDet(self):
        """Test Camera.getTransform with detector-based coordinate systems (PIXELS)
        """
        for cw in self.cameraList:
            numOffUsable = 0  # number of points off one detector but on another
            camera = cw.camera
            detNameList = list(camera.getNameIter())
            for detName in detNameList:
                det = camera[detName]

                # test transforms using an arbitrary point on the detector
                posPixels = lsst.geom.Point2D(10, 10)
                pixSys = det.makeCameraSys(PIXELS)
                pixelsToFocalPlane = camera.getTransform(pixSys, FOCAL_PLANE)
                pixelsToFieldAngle = camera.getTransform(pixSys, FIELD_ANGLE)
                focalPlaneToFieldAngle = camera.getTransform(FOCAL_PLANE, FIELD_ANGLE)
                posFocalPlane = pixelsToFocalPlane.applyForward(posPixels)
                posFieldAngle = pixelsToFieldAngle.applyForward(posPixels)
                posFieldAngle2 = focalPlaneToFieldAngle.applyForward(posFocalPlane)
                self.assertPairsAlmostEqual(posFieldAngle, posFieldAngle2)

                posFieldAngle3 = camera.transform(posPixels, pixSys, FIELD_ANGLE)
                self.assertPairsAlmostEqual(posFieldAngle, posFieldAngle3)

                for intermedPos, intermedSys in (
                    (posPixels, pixSys),
                    (posFocalPlane, FOCAL_PLANE),
                    (posFieldAngle, FIELD_ANGLE),
                ):
                    pixelSys = det.makeCameraSys(PIXELS)
                    intermedSysToPixels = camera.getTransform(intermedSys, pixelSys)
                    posPixelsRoundTrip = intermedSysToPixels.applyForward(intermedPos)
                    self.assertPairsAlmostEqual(posPixels, posPixelsRoundTrip)

                    posPixelsRoundTrip2 = camera.transform(intermedPos, intermedSys, pixelSys)
                    self.assertPairsAlmostEqual(posPixels, posPixelsRoundTrip2)

                # Test finding detectors for a point off this detector.
                # The point off the detector may be on one other detector,
                # depending if the detector has neighbor on the correct edge.
                pixOffDet = lsst.geom.Point2D(0, -10)
                pixCoordSys = det.makeCameraSys(PIXELS)
                detList = camera.findDetectors(pixOffDet, pixCoordSys)
                self.assertIn(len(detList), (0, 1))
                if len(detList) == 1:
                    numOffUsable += 1

                    otherDet = detList[0]
                    self.assertNotEqual(otherDet, det)
                    otherCoordPixSys = otherDet.makeCameraSys(PIXELS)

                    pixelsToOtherPixels = camera.getTransform(pixCoordSys, otherCoordPixSys)
                    otherPixels = pixelsToOtherPixels.applyForward(pixOffDet)
                    with self.assertRaises(AssertionError):
                        self.assertPairsAlmostEqual(otherPixels, pixOffDet)

                    # convert back
                    otherPixelsToPixels = camera.getTransform(otherCoordPixSys, pixCoordSys)
                    pixOffDetRoundTrip = otherPixelsToPixels.applyForward(otherPixels)
                    self.assertPairsAlmostEqual(pixOffDet, pixOffDetRoundTrip)
            self.assertEqual(numOffUsable, 5)

    def testAACustomDetectorTransform(self):
        """Test that user-set detector to focal plane transforms are not
        overriden.
        """
        cameraBuilder = Camera.Builder("CameraWithCustomTransforms")

        pixelSize = lsst.geom.Extent2D(0.02, 0.02)
        # Detector with custom map:
        detectorBuilder = cameraBuilder.add("Det0", 0)
        detectorBuilder.setPixelSize(pixelSize)
        detectorBuilder.setOrientation(Orientation())

        customMap = ast.ZoomMap(2, 1.5)
        transform = TransformPoint2ToPoint2(customMap)
        detectorBuilder.setTransformFromPixelsTo(FOCAL_PLANE, transform)

        # Detector with default map:
        detectorBuilder = cameraBuilder.add("Det1", 1)
        detectorBuilder.setPixelSize(pixelSize)
        detectorBuilder.setOrientation(Orientation())

        camera = cameraBuilder.finish()

        testPoint = lsst.geom.Point2D(1, 1)
        testPointFPDet0 = camera[0].getTransform(PIXELS, FOCAL_PLANE).applyForward(testPoint)
        testPointFPDet1 = camera[1].getTransform(PIXELS, FOCAL_PLANE).applyForward(testPoint)

        self.assertEqual(testPointFPDet0.x, 1.5)
        self.assertNotEqual(testPointFPDet0.x, testPointFPDet1.x)

    def testFindDetectors(self):
        for cw in self.cameraList:
            detCtrFocalPlaneList = []
            for det in cw.camera:
                # This currently assumes there is only one detector at the center
                # position of any detector.  That is not enforced and multiple detectors
                # at a given FIELD_ANGLE position is supported.  Change this if the default
                # camera changes.
                detCtrFocalPlane = det.getCenter(FOCAL_PLANE)
                detCtrFocalPlaneList.append(detCtrFocalPlane)
                detList = cw.camera.findDetectors(detCtrFocalPlane, FOCAL_PLANE)
                self.assertEqual(len(detList), 1)
                self.assertEqual(det.getName(), detList[0].getName())
            detList = cw.camera.findDetectorsList(detCtrFocalPlaneList, FOCAL_PLANE)
            self.assertEqual(len(cw.camera), len(detList))
            for dets in detList:
                self.assertEqual(len(dets), 1)

    def testFpBbox(self):
        for cw in self.cameraList:
            camera = cw.camera
            bbox = lsst.geom.Box2D()
            for name in cw.detectorNameList:
                for corner in camera[name].getCorners(FOCAL_PLANE):
                    bbox.include(corner)
            self.assertEqual(bbox.getMin(), camera.getFpBBox().getMin())
            self.assertEqual(bbox.getMax(), camera.getFpBBox().getMax())

    def testLinearity(self):
        """Test if we can set/get Linearity parameters"""
        for cw in self.cameraList:
            camera = cw.camera
            for det in camera:
                for amp in det:
                    self.assertEqual(cw.ampDataDict[det.getName()]['linInfo'][amp.getName()]['linthresh'],
                                     amp.getLinearityThreshold())
                    self.assertEqual(cw.ampDataDict[det.getName()]['linInfo'][amp.getName()]['linmax'],
                                     amp.getLinearityMaximum())
                    self.assertEqual(cw.ampDataDict[det.getName()]['linInfo'][amp.getName()]['linunits'],
                                     amp.getLinearityUnits())
                    self.assertEqual(cw.ampDataDict[det.getName()]['linInfo'][amp.getName()]['lintype'],
                                     amp.getLinearityType())
                    for c1, c2 in zip(cw.ampDataDict[det.getName()]['linInfo'][amp.getName()]['lincoeffs'],
                                      amp.getLinearityCoeffs()):
                        if np.isfinite(c1) and np.isfinite(c2):
                            self.assertEqual(c1, c2)

    def testAssembly(self):
        ccdNames = ('R:0,0 S:1,0', 'R:0,0 S:0,1')
        detectorImageMap = {True: afwImage.ImageU(os.path.join(testPath, 'test_comp_trimmed.fits.gz'),
                                                  allowUnsafe=True),
                            False: afwImage.ImageU(os.path.join(testPath, 'test_comp.fits.gz'),
                                                   allowUnsafe=True)}
        for cw in self.cameraList:
            camera = cw.camera
            imList = self.assemblyList[camera.getName()]
            for ccdName in ccdNames:
                det = camera[ccdName]
                if len(imList) == 1:
                    # There's one test image because it's the same for all
                    # amplifiers, not because there's only one amplifier.
                    imList *= len(det)
                # Test going from possibly-separate amp images to detector
                # images.
                for trim, assemble in ((False, assembleAmplifierRawImage), (True, assembleAmplifierImage)):
                    if not trim:
                        outBbox = cameraGeomUtils.calcRawCcdBBox(det)
                    else:
                        outBbox = det.getBBox()
                    outImage = afwImage.ImageU(outBbox)
                    for amp, im in zip(det, imList):
                        assemble(outImage, im, amp)
                    self.assertImagesEqual(outImage, detectorImageMap[trim])
                    # Test going from detector images back to single-amplifier
                    # images.
                    detector_exposure = afwImage.ExposureU(afwImage.MaskedImageU(detectorImageMap[trim]))
                    detector_exposure.setDetector(makeUpdatedDetector(det))
                    for amp, im in zip(det, imList):
                        amp_exposure = AmplifierIsolator.apply(detector_exposure, amp)
                        self.assertEqual(len(amp_exposure.getDetector()), 1)
                        self.assertEqual(amp_exposure.getDetector().getBBox(), amp.getBBox())
                        self.assertAmplifiersEqual(amp, amp_exposure.getDetector()[0])
                        if not trim:
                            self.assertEqual(cameraGeomUtils.calcRawCcdBBox(amp_exposure.getDetector()),
                                             amp_exposure.getBBox())
                            self.assertImagesEqual(im[amp.getRawBBox()], amp_exposure.image)
                        else:
                            self.assertEqual(amp_exposure.getDetector().getBBox(),
                                             amp_exposure.getBBox())
                            self.assertImagesEqual(im[amp.getRawDataBBox()], amp_exposure.image)

    @unittest.skipIf(not display, "display variable not set; skipping cameraGeomUtils test")
    def testCameraGeomUtils(self):
        for cw in self.cameraList:
            camera = cw.camera
            disp = afwDisplay.Display()
            cameraGeomUtils.showCamera(camera, display=disp)
            disp.incrDefaultFrame()
            for det in (camera[10], camera[20]):
                cameraGeomUtils.showCcd(det, inCameraCoords=False)
                disp.incrDefaultFrame()
                cameraGeomUtils.showCcd(det, inCameraCoords=True)
                disp.incrDefaultFrame()
                cameraGeomUtils.showCcd(det, inCameraCoords=False)
                disp.incrDefaultFrame()
                cameraGeomUtils.showCcd(det, inCameraCoords=True)
                disp.incrDefaultFrame()
                for amp in det:
                    cameraGeomUtils.showAmp(amp, display=disp, imageFactory=afwImage.ImageF)
                    disp.incrDefaultFrame()

    def testCameraRaises(self):
        for cw in self.cameraList:
            camera = cw.camera
            point = lsst.geom.Point2D(0, 0)
            # non-existant source camera system
            with self.assertRaises(pexExcept.InvalidParameterError):
                camera.getTransform(CameraSys("badSystem"), FOCAL_PLANE)
            with self.assertRaises(pexExcept.InvalidParameterError):
                camera.transform(point, CameraSys("badSystem"), FOCAL_PLANE)
            # non-existant destination camera system
            with self.assertRaises(pexExcept.InvalidParameterError):
                camera.getTransform(FOCAL_PLANE, CameraSys("badSystem"))
            with self.assertRaises(pexExcept.InvalidParameterError):
                camera.transform(point, FOCAL_PLANE, CameraSys("badSystem"))
            # non-existent source detector
            with self.assertRaises(pexExcept.InvalidParameterError):
                camera.getTransform(CameraSys("pixels", "invalid"), FOCAL_PLANE)
            with self.assertRaises(pexExcept.InvalidParameterError):
                camera.transform(point, CameraSys("pixels", "invalid"), FOCAL_PLANE)
            # non-existent destination detector
            with self.assertRaises(pexExcept.InvalidParameterError):
                camera.getTransform(FOCAL_PLANE, CameraSys("pixels", "invalid"))
            with self.assertRaises(pexExcept.InvalidParameterError):
                camera.transform(point, FOCAL_PLANE, CameraSys("pixels", "invalid"))

    def testDetectorCollectionPersistence(self):
        """Test that we can round-trip a DetectorCollection through FITS I/O.
        """
        for wrapper in self.cameraList:
            camera = wrapper.camera
            detectors = list(camera)
            collectionIn = DetectorCollection(detectors)
            with lsst.utils.tests.getTempFilePath(".fits") as filename:
                collectionIn.writeFits(filename)
                collectionOut = DetectorCollection.readFits(filename)
            self.assertDetectorCollectionsEqual(collectionIn, collectionOut)

    def testCameraPersistence(self):
        """Test that we can round-trip a Camera through FITS I/O.
        """
        for wrapper in self.cameraList:
            cameraIn = wrapper.camera
            with lsst.utils.tests.getTempFilePath(".fits") as filename:
                cameraIn.writeFits(filename)
                cameraOut = Camera.readFits(filename)
            self.assertCamerasEqual(cameraIn, cameraOut)


class TestMemory(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
