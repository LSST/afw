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
Tests for SourceTable slots with version > 0
"""

import unittest

import numpy as np

import lsst.utils.tests
import lsst.pex.exceptions
import lsst.geom
import lsst.afw.table
import lsst.afw.geom
import lsst.afw.image
import lsst.afw.detection


def makeArray(size, dtype):
    return np.array(np.random.randn(*size), dtype=dtype)


def makeCov(size, dtype):
    m = np.array(np.random.randn(size, size), dtype=dtype)
    return np.dot(m, m.transpose())


def makeWcs():
    crval = lsst.geom.SpherePoint(1.606631, 5.090329, lsst.geom.degrees)
    crpix = lsst.geom.Point2D(2036., 2000.)
    cdMatrix = np.array([[5.399452e-5, -1.30770e-5], [1.30770e-5, 5.399452e-5]])
    return lsst.afw.geom.makeSkyWcs(crval=crval, crpix=crpix, cdMatrix=cdMatrix)


class SourceTableTestCase(lsst.utils.tests.TestCase):

    def fillRecord(self, record):
        record.set(self.instFluxKey, np.random.randn())
        record.set(self.instFluxErrKey, np.random.randn())
        record.set(self.centroidKey,
                   lsst.geom.Point2D(*np.random.randn(2)))
        record.set(self.centroidErrKey, makeCov(2, np.float32))
        record.set(self.shapeKey,
                   lsst.afw.geom.Quadrupole(*np.random.randn(3)))
        record.set(self.shapeErrKey, makeCov(3, np.float32))
        record.set(self.fluxFlagKey, np.random.randn() > 0)
        record.set(self.centroidFlagKey, np.random.randn() > 0)
        record.set(self.shapeFlagKey, np.random.randn() > 0)

    def makeInstFlux(self, schema, prefix, uncertainty):
        self.instFluxKey = self.schema.addField(prefix+"_instFlux", type="D")
        if uncertainty:
            self.instFluxErrKey = self.schema.addField(
                prefix+"_instFluxErr", type="D")
        self.fluxFlagKey = self.schema.addField(prefix+"_flag", type="Flag")

    def makeCentroid(self, schema, prefix, uncertainty):
        self.centroidXKey = self.schema.addField(prefix+"_x", type="D")
        self.centroidYKey = self.schema.addField(prefix+"_y", type="D")
        sigmaArray = []
        covArray = []
        if uncertainty > 0:
            self.centroidXErrKey = self.schema.addField(
                prefix+"_xErr", type="F")
            self.centroidYErrKey = self.schema.addField(
                prefix+"_yErr", type="F")
            sigmaArray.append(self.centroidXErrKey)
            sigmaArray.append(self.centroidYErrKey)
        if uncertainty > 1:
            self.centroidXYCovKey = self.schema.addField(
                prefix+"_x_y_Cov", type="F")
            covArray.append(self.centroidXYCovKey)
        self.centroidKey = lsst.afw.table.Point2DKey(
            self.centroidXKey, self.centroidYKey)
        self.centroidErrKey = lsst.afw.table.CovarianceMatrix2fKey(
            sigmaArray, covArray)
        self.centroidFlagKey = self.schema.addField(
            prefix+"_flag", type="Flag")

    def makeShape(self, schema, prefix, uncertainty):
        self.shapeXXKey = self.schema.addField(prefix+"_xx", type="D")
        self.shapeYYKey = self.schema.addField(prefix+"_yy", type="D")
        self.shapeXYKey = self.schema.addField(prefix+"_xy", type="D")
        self.shapeKey = lsst.afw.table.QuadrupoleKey(
            self.shapeXXKey, self.shapeYYKey, self.shapeXYKey)
        sigmaArray = []
        covArray = []
        if uncertainty > 0:
            self.shapeXXErrKey = self.schema.addField(
                prefix+"_xxErr", type="F")
            self.shapeYYErrKey = self.schema.addField(
                prefix+"_yyErr", type="F")
            self.shapeXYErrKey = self.schema.addField(
                prefix+"_xyErr", type="F")
            sigmaArray.append(self.shapeXXErrKey)
            sigmaArray.append(self.shapeYYErrKey)
            sigmaArray.append(self.shapeXYErrKey)
        if uncertainty > 1:
            self.shapeXXYYCovKey = self.schema.addField(
                prefix+"_xx_yy_Cov", type="F")
            self.shapeXXXYCovKey = self.schema.addField(
                prefix+"_xx_xy_Cov", type="F")
            self.shapeYYXYCovKey = self.schema.addField(
                prefix+"_yy_xy_Cov", type="F")
            covArray.append(self.shapeXXYYCovKey)
            covArray.append(self.shapeXXXYCovKey)
            covArray.append(self.shapeYYXYCovKey)
        self.shapeErrKey = lsst.afw.table.CovarianceMatrix3fKey(
            sigmaArray, covArray)
        self.shapeFlagKey = self.schema.addField(prefix+"_flag", type="Flag")

    def setUp(self):
        np.random.seed(1)
        self.schema = lsst.afw.table.SourceTable.makeMinimalSchema()
        self.makeInstFlux(self.schema, "a", 1)
        self.makeCentroid(self.schema, "b", 2)
        self.makeShape(self.schema, "c", 2)
        self.table = lsst.afw.table.SourceTable.make(self.schema)
        self.catalog = lsst.afw.table.SourceCatalog(self.table)
        self.record = self.catalog.addNew()
        self.fillRecord(self.record)
        self.record.setId(50)
        self.fillRecord(self.catalog.addNew())
        self.fillRecord(self.catalog.addNew())
        self.table.definePsfFlux("a")
        self.table.defineCentroid("b")
        self.table.defineShape("c")

    def tearDown(self):
        del self.schema
        del self.record
        del self.table
        del self.catalog

    def testPersisted(self):
        self.table.definePsfFlux("a")
        self.table.defineCentroid("b")
        self.table.defineShape("c")
        with lsst.utils.tests.getTempFilePath(".fits") as filename:
            self.catalog.writeFits(filename)
            catalog = lsst.afw.table.SourceCatalog.readFits(filename)
            table = catalog.getTable()
            record = catalog[0]
            # I'm using the keys from the non-persisted table.  They should work at least in the
            # current implementation
            self.assertEqual(record.get(self.instFluxKey), record.getPsfInstFlux())
            self.assertEqual(record.get(self.fluxFlagKey),
                             record.getPsfFluxFlag())
            self.assertEqual(table.getSchema().getAliasMap().get("slot_Centroid"), "b")
            self.assertEqual(record.get(self.centroidKey),
                             record.getCentroid())
            self.assertFloatsAlmostEqual(
                record.get(self.centroidErrKey),
                record.getCentroidErr())
            self.assertEqual(table.getSchema().getAliasMap().get("slot_Shape"), "c")
            self.assertEqual(record.get(self.shapeKey), record.getShape())
            self.assertFloatsAlmostEqual(
                record.get(self.shapeErrKey),
                record.getShapeErr())

    def testDefiner1(self):
        self.table.definePsfFlux("a")
        self.table.defineCentroid("b")
        self.table.defineShape("c")
        self.assertEqual(self.record.get(self.instFluxKey),
                         self.record.getPsfInstFlux())
        self.assertEqual(self.record.get(self.fluxFlagKey),
                         self.record.getPsfFluxFlag())
        self.assertEqual(self.table.getSchema().getAliasMap().get("slot_Centroid"), "b")
        self.assertEqual(self.record.get(self.centroidKey),
                         self.record.getCentroid())
        self.assertFloatsAlmostEqual(
            self.record.get(self.centroidErrKey), self.record.getCentroidErr())
        self.assertEqual(self.table.getSchema().getAliasMap().get("slot_Shape"), "c")
        self.assertEqual(self.record.get(self.shapeKey),
                         self.record.getShape())
        self.assertFloatsAlmostEqual(self.record.get(self.shapeErrKey),
                                     self.record.getShapeErr())

    def testCoordUpdate(self):
        self.table.defineCentroid("b")
        wcs = makeWcs()
        self.record.updateCoord(wcs)
        coord1 = self.record.getCoord()
        coord2 = wcs.pixelToSky(self.record.get(self.centroidKey))
        self.assertEqual(coord1, coord2)

    def testColumnView(self):
        cols1 = self.catalog.getColumnView()
        cols2 = self.catalog.columns
        self.assertIs(cols1, cols2)
        self.assertIsInstance(cols1, lsst.afw.table.SourceColumnView)
        self.table.definePsfFlux("a")
        self.table.defineCentroid("b")
        self.table.defineShape("c")
        self.assertTrue((cols2["a_instFlux"] == cols2.getPsfInstFlux()).all())
        self.assertTrue((cols2["a_instFluxErr"] == cols2.getPsfInstFluxErr()).all())
        self.assertTrue((cols2["b_x"] == cols2.getX()).all())
        self.assertTrue((cols2["b_y"] == cols2.getY()).all())
        self.assertTrue((cols2["c_xx"] == cols2.getIxx()).all())
        self.assertTrue((cols2["c_yy"] == cols2.getIyy()).all())
        self.assertTrue((cols2["c_xy"] == cols2.getIxy()).all())


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
