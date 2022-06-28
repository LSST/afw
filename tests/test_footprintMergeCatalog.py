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
import lsst.pex.exceptions
import lsst.geom
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDetect
import lsst.afw.table as afwTable


def insertPsf(pos, im, psf, kernelSize, flux):
    for x, y in pos:
        x0 = x-kernelSize//2
        y0 = y-kernelSize//2
        tmpbox = lsst.geom.Box2I(lsst.geom.Point2I(x0, y0),
                                 lsst.geom.Extent2I(kernelSize, kernelSize))
        tmp = psf.computeImage(lsst.geom.Point2D(x0, y0))
        tmp *= flux
        im.image[tmpbox, afwImage.LOCAL] += tmp


def mergeCatalogs(catList, names, peakDist, idFactory, indivNames=[], samePeakDist=-1.):
    schema = afwTable.SourceTable.makeMinimalSchema()
    merged = afwDetect.FootprintMergeList(schema, names)

    if not indivNames:
        indivNames = names

    # Count the number of objects and peaks in this list
    mergedList = merged.getMergedSourceCatalog(catList, indivNames, peakDist,
                                               schema, idFactory, samePeakDist)
    nob = len(mergedList)
    npeaks = sum([len(ob.getFootprint().getPeaks()) for ob in mergedList])

    return mergedList, nob, npeaks


def isPeakInCatalog(peak, catalog):
    for record in catalog:
        for p in record.getFootprint().getPeaks():
            if p.getI() == peak.getI():
                return True
    return False


class FootprintMergeCatalogTestCase(lsst.utils.tests.TestCase):

    def setUp(self):
        """Build up three different sets of objects that are to be merged"""
        pos1 = [(40, 40), (220, 35), (40, 48), (220, 50),
                (67, 67), (150, 50), (40, 90), (70, 160),
                (35, 255), (70, 180), (250, 200), (120, 120),
                (170, 180), (100, 210), (20, 210),
                ]
        pos2 = [(43, 45), (215, 31), (171, 258), (211, 117),
                (48, 99), (70, 160), (125, 45), (251, 33),
                (37, 170), (134, 191), (79, 223), (258, 182)
                ]
        pos3 = [(70, 170), (219, 41), (253, 173), (253, 192)]

        box = lsst.geom.Box2I(lsst.geom.Point2I(0, 0), lsst.geom.Point2I(300, 300))
        psfsig = 1.
        kernelSize = 41
        flux = 1000

        # Create a different sized psf for each image and insert them at the
        # desired positions
        im1 = afwImage.MaskedImageD(box)
        psf1 = afwDetect.GaussianPsf(kernelSize, kernelSize, psfsig)

        im2 = afwImage.MaskedImageD(box)
        psf2 = afwDetect.GaussianPsf(kernelSize, kernelSize, 2*psfsig)

        im3 = afwImage.MaskedImageD(box)
        psf3 = afwDetect.GaussianPsf(kernelSize, kernelSize, 1.3*psfsig)

        insertPsf(pos1, im1, psf1, kernelSize, flux)
        insertPsf(pos2, im2, psf2, kernelSize, flux)
        insertPsf(pos3, im3, psf3, kernelSize, flux)

        schema = afwTable.SourceTable.makeMinimalSchema()

        # Create SourceCatalogs from these objects
        fp1 = afwDetect.FootprintSet(
            im1, afwDetect.Threshold(0.001), "DETECTED")
        # Use a different ID factory for each footprint so that the peak ID's
        # overlap. This allows us to test ID collisions.
        table = afwTable.SourceTable.make(schema, afwTable.IdFactory.makeSimple())
        self.catalog1 = afwTable.SourceCatalog(table)
        fp1.makeSources(self.catalog1)

        table = afwTable.SourceTable.make(schema, afwTable.IdFactory.makeSimple())
        fp2 = afwDetect.FootprintSet(
            im2, afwDetect.Threshold(0.001), "DETECTED")
        self.catalog2 = afwTable.SourceCatalog(table)
        fp2.makeSources(self.catalog2)

        table = afwTable.SourceTable.make(schema, afwTable.IdFactory.makeSimple())
        fp3 = afwDetect.FootprintSet(
            im3, afwDetect.Threshold(0.001), "DETECTED")
        self.catalog3 = afwTable.SourceCatalog(table)
        fp3.makeSources(self.catalog3)

    def tearDown(self):
        del self.catalog1
        del self.catalog2
        del self.catalog3

    def assertUniqueIds(self, catalog):
        """Ensure that all of the peak IDs in a single parent are unique.
        """
        for src in catalog:
            # The peak catalog may be non-contiguous,
            # so add each peak individually
            peaks = [peak.getId() for peak in src.getFootprint().peaks]
            self.assertEqual(sorted(peaks), sorted(set(peaks)))

    def testMerge1(self):
        idFactory = afwTable.IdFactory.makeSimple()
        # Add the first catalog only
        merge, nob, npeak = mergeCatalogs([self.catalog1], ["1"], [-1], idFactory)
        self.assertEqual(nob, 14)
        self.assertEqual(npeak, 15)

        for record in merge:
            self.assertTrue(record.get("merge_footprint_1"))
            for peak in record.getFootprint().getPeaks():
                self.assertTrue(peak.get("merge_peak_1"))

        # area for each object
        pixArea = np.empty(14)
        pixArea.fill(69)
        pixArea[1] = 135
        measArea = [i.getFootprint().getArea() for i in merge]
        np.testing.assert_array_equal(pixArea, measArea)

        # Add the first catalog and second catalog with the wrong name, which should result
        # an exception being raised
        with self.assertRaises(lsst.pex.exceptions.LogicError):
            mergeCatalogs([self.catalog1, self.catalog2], ["1", "2"], [0, 0], idFactory, ["1", "3"])

        # Add the first catalog and second catalog with the wrong number of peakDist elements,
        # which should raise an exception
        with self.assertRaises(ValueError):
            mergeCatalogs([self.catalog1, self.catalog2], ["1", "2"], [0], idFactory, ["1", "3"])

        # Add the first catalog and second catalog with the wrong number of filters,
        # which should raise an exception
        with self.assertRaises(ValueError):
            mergeCatalogs([self.catalog1, self.catalog2], ["1"], [0], idFactory, ["1", "3"])

        # Add the first catalog and second catalog with minPeak < 1 so it will
        # not add new peaks
        merge, nob, npeak = mergeCatalogs([self.catalog1, self.catalog2], ["1", "2"], [0, -1], idFactory)
        self.assertEqual(nob, 22)
        self.assertEqual(npeak, 23)
        self.assertUniqueIds(merge)
        # area for each object
        pixArea = np.ones(22)
        pixArea[0] = 275
        pixArea[1] = 270
        pixArea[2:5].fill(69)
        pixArea[5] = 323
        pixArea[6] = 69
        pixArea[7] = 261
        pixArea[8:14].fill(69)
        pixArea[14:22].fill(261)
        measArea = [i.getFootprint().getArea() for i in merge]
        np.testing.assert_array_equal(pixArea, measArea)

        for record in merge:
            for peak in record.getFootprint().getPeaks():
                # Should only get peaks from catalog2 if catalog1 didn't
                # contribute to the footprint
                if record.get("merge_footprint_1"):
                    self.assertTrue(peak.get("merge_peak_1"))
                    self.assertFalse(peak.get("merge_peak_2"))
                else:
                    self.assertFalse(peak.get("merge_peak_1"))
                    self.assertTrue(peak.get("merge_peak_2"))

        # Same as previous with another catalog
        merge, nob, npeak = mergeCatalogs([self.catalog1, self.catalog2, self.catalog3],
                                          ["1", "2", "3"], [0, -1, -1],
                                          idFactory)
        self.assertEqual(nob, 19)
        self.assertEqual(npeak, 20)
        self.assertUniqueIds(merge)
        pixArea = np.ones(19)
        pixArea[0] = 416
        pixArea[1] = 270
        pixArea[2:4].fill(69)
        pixArea[4] = 323
        pixArea[5] = 69
        pixArea[6] = 406
        pixArea[7] = 69
        pixArea[8] = 493
        pixArea[9:13].fill(69)
        pixArea[12:19].fill(261)
        measArea = [i.getFootprint().getArea() for i in merge]
        np.testing.assert_array_equal(pixArea, measArea)

        for record in merge:
            for peak in record.getFootprint().getPeaks():
                # Should only get peaks from catalog2 if catalog1 didn't
                # contribute to the footprint
                if record.get("merge_footprint_1"):
                    self.assertTrue(peak.get("merge_peak_1"))
                    self.assertFalse(peak.get("merge_peak_2"))
                else:
                    self.assertFalse(peak.get("merge_peak_1"))
                    self.assertTrue(peak.get("merge_peak_2"))

        # Add all the catalogs with minPeak = 0 so all peaks will be added
        merge, nob, npeak = mergeCatalogs([self.catalog1, self.catalog2, self.catalog3],
                                          ["1", "2", "3"], [0, 0, 0],
                                          idFactory)
        self.assertEqual(nob, 19)
        self.assertEqual(npeak, 30)
        self.assertUniqueIds(merge)
        measArea = [i.getFootprint().getArea() for i in merge]
        np.testing.assert_array_equal(pixArea, measArea)

        # Add all the catalogs with minPeak = 10 so some peaks will be added to
        # the footprint
        merge, nob, npeak = mergeCatalogs([self.catalog1, self.catalog2, self.catalog3],
                                          ["1", "2", "3"], 10, idFactory)
        self.assertEqual(nob, 19)
        self.assertEqual(npeak, 25)
        self.assertUniqueIds(merge)
        measArea = [i.getFootprint().getArea() for i in merge]
        np.testing.assert_array_equal(pixArea, measArea)

        for record in merge:
            for peak in record.getFootprint().getPeaks():
                if peak.get("merge_peak_1"):
                    self.assertTrue(record.get("merge_footprint_1"))
                    self.assertTrue(isPeakInCatalog(peak, self.catalog1))
                elif peak.get("merge_peak_2"):
                    self.assertTrue(record.get("merge_footprint_2"))
                    self.assertTrue(isPeakInCatalog(peak, self.catalog2))
                elif peak.get("merge_peak_3"):
                    self.assertTrue(record.get("merge_footprint_3"))
                    self.assertTrue(isPeakInCatalog(peak, self.catalog3))
                else:
                    self.fail("At least one merge.peak flag must be set")

        # Add all the catalogs with minPeak = 100 so no new peaks will be added
        merge, nob, npeak = mergeCatalogs([self.catalog1, self.catalog2, self.catalog3],
                                          ["1", "2", "3"], 100, idFactory)
        self.assertEqual(nob, 19)
        self.assertEqual(npeak, 20)
        self.assertUniqueIds(merge)
        measArea = [i.getFootprint().getArea() for i in merge]
        np.testing.assert_array_equal(pixArea, measArea)

        for record in merge:
            for peak in record.getFootprint().getPeaks():
                if peak.get("merge_peak_1"):
                    self.assertTrue(record.get("merge_footprint_1"))
                    self.assertTrue(isPeakInCatalog(peak, self.catalog1))
                elif peak.get("merge_peak_2"):
                    self.assertTrue(record.get("merge_footprint_2"))
                    self.assertTrue(isPeakInCatalog(peak, self.catalog2))
                elif peak.get("merge_peak_3"):
                    self.assertTrue(record.get("merge_footprint_3"))
                    self.assertTrue(isPeakInCatalog(peak, self.catalog3))
                else:
                    self.fail("At least one merge_peak flag must be set")

        # Add footprints with large samePeakDist so that any footprint that merges will also
        # have the peak flagged
        merge, nob, npeak = mergeCatalogs([self.catalog1, self.catalog2, self.catalog3],
                                          ["1", "2", "3"], 100, idFactory, samePeakDist=40)

        # peaks detected in more than one catalog
        self.assertUniqueIds(merge)
        multiPeakIndex = [0, 2, 5, 7, 9]
        peakIndex = 0
        for record in merge:
            for peak in record.getFootprint().getPeaks():
                numPeak = np.sum([peak.get("merge_peak_1"), peak.get("merge_peak_2"),
                                  peak.get("merge_peak_3")])
                if peakIndex in multiPeakIndex:
                    self.assertGreater(numPeak, 1)
                else:
                    self.assertEqual(numPeak, 1)
                peakIndex += 1

    def testDM17431(self):
        """Test that priority order is respected

        specifically when lower priority footprints overlap two previously
        disconnected higher priority footprints.
        """
        box = lsst.geom.Box2I(lsst.geom.Point2I(0, 0), lsst.geom.Point2I(100, 100))
        psfsig = 1.
        kernelSize = 41
        flux = 1000
        cat1 = {}
        cat2 = {}
        peakDist = 10
        samePeakDist = 3

        # cat2 connects cat1's 2 separated footprints.
        # 65 and 70 are too close to be new or same peaks.
        # 70 is higher priority and should be in the catalog instead of 65.
        cat1['pos'] = [(50, 50), (70, 50)]
        cat2['pos'] = [(55, 50), (65, 50)]
        schema = afwTable.SourceTable.makeMinimalSchema()
        idFactory = afwTable.IdFactory.makeSimple()
        table = afwTable.SourceTable.make(schema, idFactory)

        for (cat, psfFactor) in zip([cat1, cat2], [1, 2]):
            cat['mi'] = afwImage.MaskedImageD(box)
            cat['psf'] = afwDetect.GaussianPsf(kernelSize, kernelSize, psfFactor*psfsig)
            insertPsf(cat['pos'], cat['mi'], cat['psf'], kernelSize, flux)
            fp = afwDetect.FootprintSet(cat['mi'], afwDetect.Threshold(0.001), "DETECTED")
            cat['catalog'] = afwTable.SourceCatalog(table)
            fp.makeSources(cat['catalog'])

        merge, nob, npeak = mergeCatalogs([cat1['catalog'], cat2['catalog']], ["1", "2"],
                                          peakDist, idFactory, samePeakDist=samePeakDist)

        # Check that both higher-priority cat1 records survive peak merging
        for record in cat1['catalog']:
            for peak in record.getFootprint().getPeaks():
                self.assertTrue(isPeakInCatalog(peak, merge))


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
