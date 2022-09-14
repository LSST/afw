#
# LSST Data Management System
# Copyright 2017 LSST Corporation.
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

import os
import unittest

from lsst.daf.base import PropertyList

import lsst.afw.fits
import lsst.utils.tests

testPath = os.path.abspath(os.path.dirname(__file__))


class FitsTestCase(lsst.utils.tests.TestCase):

    def setUp(self):
        # May appear only once in the FITS file (because cfitsio will insist on putting them there)
        self.single = ["SIMPLE", "BITPIX", "EXTEND", "NAXIS"]

    def writeAndRead(self, header):
        """Write the supplied header and read it back again.
        """
        fitsFile = lsst.afw.fits.MemFileManager()
        with lsst.afw.fits.Fits(fitsFile, "w") as fits:
            fits.createEmpty()
            fits.writeMetadata(header)
        with lsst.afw.fits.Fits(fitsFile, "r") as fits:
            metadata = fits.readMetadata()
        return metadata

    def testSimpleIO(self):
        """Check that a simple header can be written and read back."""

        expected = {
            "ASTRING": "Test String",
            "ANUNDEF": None,
            "AFLOAT": 3.1415,
            "ANINT": 42,
        }

        header = PropertyList()
        for k, v in expected.items():
            header[k] = v

        output = self.writeAndRead(header)

        # Remove keys added by cfitsio
        for k in self.single:
            if k in output:
                del output[k]
        if "COMMENT" in output:
            del output["COMMENT"]

        self.assertEqual(output.toDict(), header.toDict())

    def testReadUndefined(self):
        """Read a header with some undefined values that might override."""
        testFile = os.path.join(testPath, "data", "ticket18864.fits")
        metadata = lsst.afw.fits.readMetadata(testFile)

        # Neither of these should be arrays despite having doubled keywords
        # The first value for ADC-STR should override the second undef value
        self.assertAlmostEqual(metadata.getScalar("ADC-STR"), 22.01)

        # The value for DOM-WND should be the second value since the first
        # was undefined
        self.assertAlmostEqual(metadata.getScalar("DOM-WND"), 4.8)

    def testReadBlankKeywordComment(self):
        """Read a header that uses blank keyword comments."""
        testFile = os.path.join(testPath, "data", "ticket20143.fits")
        metadata = lsst.afw.fits.readMetadata(testFile)

        self.assertEqual("---- Checksums ----", metadata["COMMENT"])
        self.assertNotIn("", metadata, "Check empty strings as keys")

    def testIgnoreKeywords(self):
        """Check that certain keywords are ignored in read/write of headers"""
        # May not appear at all in the FITS file (cfitsio doesn't write these by default)
        notAtAll = [
            # FITS core keywords
            "GCOUNT", "PCOUNT", "XTENSION", "BSCALE", "BZERO", "TZERO", "TSCAL",
            # FITS compression keywords
            "ZBITPIX", "ZIMAGE", "ZCMPTYPE", "ZSIMPLE", "ZEXTEND", "ZBLANK", "ZDATASUM", "ZHECKSUM",
            "ZNAXIS", "ZTILE", "ZNAME", "ZVAL", "ZQUANTIZ",
            # Not essential these be excluded, but will prevent fitsverify warnings
            "DATASUM", "CHECKSUM",
        ]
        # Additional keywords to check; these should go straight through
        # Some of these are longer/shorter versions of strings above,
        # to test that the checks for just the start of strings is working.
        others = ["FOOBAR", "SIMPLETN", "DATASUMX", "NAX", "SIM"]

        header = PropertyList()
        for ii, key in enumerate(self.single + notAtAll + others):
            header.add(key, ii)
        metadata = self.writeAndRead(header)
        for key in self.single:
            self.assertEqual(metadata.valueCount(key), 1, key)
        for key in notAtAll:
            self.assertEqual(metadata.valueCount(key), 0, key)
        for key in others:
            self.assertEqual(metadata.valueCount(key), 1, key)

    def testUndefinedVector(self):
        header = PropertyList()
        header.set("FOO", [None, None])
        metadata = self.writeAndRead(header)
        self.assertEqual(metadata.getArray("FOO"), [None, None])

    def test_ticket_dm_36207(self):
        # test whether moving to invalid HDU and then moving back throws or not
        testfile = os.path.join(testPath, "data", "parent.fits")
        fts = lsst.afw.fits.Fits(testfile, "r")

        # parent.fits has 2 HDUs.
        with self.assertRaises(lsst.afw.fits.FitsError):
            fts.setHdu(5)

        # check that we can move back to primary HDU and pull out a header
        fts.setHdu(0)
        mdattest = fts.readMetadata()["COMMENT"]
        self.assertIn("and Astrophysics', volume 376, page 359;", mdattest)

    def testNamedHeaderNavigate(self):
        testfile = os.path.join(testPath, "data", "multi_extension_metadata.fits")

        # load metadata from the extra table header
        md_extra_tab = lsst.afw.fits.readMetadata(testfile, hduName="EXTRA_TAB")
        # assert the value we put in is in the read metadata
        self.assertTrue("FVALUE" in md_extra_tab)

        # load metadata from the extra image header, do same test
        md_extra_im = lsst.afw.fits.readMetadata(testfile, hduName="EXTRA_IM")
        self.assertTrue("BLORP" in md_extra_im)

        # now try to load a non-existent named HDU and check that we throw
        with self.assertRaises(lsst.afw.fits.FitsError):
            lsst.afw.fits.readMetadata(testfile, hduName="CORDON_BLEAU")


class TestMemory(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    import sys
    setup_module(sys.modules[__name__])
    unittest.main()
