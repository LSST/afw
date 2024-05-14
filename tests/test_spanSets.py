#
# LSST Data Management System
#
# Copyright 2008-2016  AURA/LSST.
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
# see <https://www.lsstcorp.org/LegalNotices/>.
#

import unittest
import numpy as np

import lsst.utils.tests
import lsst.geom
import lsst.afw.geom as afwGeom
import lsst.afw.geom.ellipses as afwGeomEllipses
import lsst.afw.image as afwImage


class SpanSetTestCase(lsst.utils.tests.TestCase):
    '''
    This is a python level unit test of the SpanSets class. It is mean to work in conjuction
    with the c++ unit test. The C++ test has much more coverage, and tests some features which
    are not exposed to python. This test serves mainly as a way to test that the python bindings
    correctly work. Many of these tests are smaller versions of the C++ tests.
    '''

    def testNullSpanSet(self):
        nullSS = afwGeom.SpanSet()
        self.assertEqual(nullSS.getArea(), 0)
        self.assertEqual(len(nullSS), 0)
        self.assertEqual(nullSS.getBBox().getDimensions().getX(), 0)
        self.assertEqual(nullSS.getBBox().getDimensions().getY(), 0)

    def testBBoxSpanSet(self):
        boxSS = afwGeom.SpanSet(lsst.geom.Box2I(lsst.geom.Point2I(2, 2),
                                                lsst.geom.Point2I(6, 6)))
        self.assertEqual(boxSS.getArea(), 25)
        bBox = boxSS.getBBox()
        self.assertEqual(bBox.getMinX(), 2)
        self.assertEqual(bBox.getMinY(), 2)

    def testIteratorConstructor(self):
        spans = [afwGeom.Span(0, 2, 4), afwGeom.Span(1, 2, 4),
                 afwGeom.Span(2, 2, 4)]
        spanSetFromList = afwGeom.SpanSet(spans)
        spanSetFromArray = afwGeom.SpanSet(np.array(spans))

        self.assertEqual(spanSetFromList.getBBox().getMinX(), 2)
        self.assertEqual(spanSetFromList.getBBox().getMaxX(), 4)
        self.assertEqual(spanSetFromList.getBBox().getMinY(), 0)

        self.assertEqual(spanSetFromArray.getBBox().getMinX(), 2)
        self.assertEqual(spanSetFromArray.getBBox().getMaxX(), 4)
        self.assertEqual(spanSetFromArray.getBBox().getMinY(), 0)

    def testIsContiguous(self):
        spanSetConList = [afwGeom.Span(0, 2, 5), afwGeom.Span(1, 5, 8)]
        spanSetCon = afwGeom.SpanSet(spanSetConList)
        self.assertTrue(spanSetCon.isContiguous())

        spanSetNotConList = [afwGeom.Span(0, 2, 5), afwGeom.Span(1, 20, 25)]
        spanSetNotCon = afwGeom.SpanSet(spanSetNotConList)
        self.assertFalse(spanSetNotCon.isContiguous())

    def testSplit(self):
        spanSetOne = afwGeom.SpanSet.fromShape(2, afwGeom.Stencil.BOX).shiftedBy(2, 2)
        spanSetTwo = afwGeom.SpanSet.fromShape(2, afwGeom.Stencil.BOX).shiftedBy(8, 8)

        spanSetList = []
        for spn in spanSetOne:
            spanSetList.append(spn)
        for spn in spanSetTwo:
            spanSetList.append(spn)
        spanSetTogether = afwGeom.SpanSet(spanSetList)

        spanSetSplit = spanSetTogether.split()
        self.assertEqual(len(spanSetSplit), 2)

        for a, b in zip(spanSetOne, spanSetSplit[0]):
            self.assertEqual(a, b)

        for a, b in zip(spanSetTwo, spanSetSplit[1]):
            self.assertEqual(a, b)

    def testTransform(self):
        transform = lsst.geom.LinearTransform(np.array([[2.0, 0.0], [0.0, 2.0]]))
        spanSetPreScale = afwGeom.SpanSet.fromShape(2, afwGeom.Stencil.CIRCLE)
        spanSetPostScale = spanSetPreScale.transformedBy(transform)

        self.assertEqual(spanSetPostScale.getBBox().getMinX(), -4)
        self.assertEqual(spanSetPostScale.getBBox().getMinY(), -4)

    def testOverlaps(self):
        spanSetNoShift = afwGeom.SpanSet.fromShape(4, afwGeom.Stencil.CIRCLE)
        spanSetShift = spanSetNoShift.shiftedBy(2, 2)

        self.assertTrue(spanSetNoShift.overlaps(spanSetShift))

    def testContains(self):
        spanSetLarge = afwGeom.SpanSet.fromShape(4, afwGeom.Stencil.CIRCLE)
        spanSetSmall = afwGeom.SpanSet.fromShape(1, afwGeom.Stencil.CIRCLE)

        self.assertTrue(spanSetLarge.contains(spanSetSmall))
        self.assertFalse(spanSetSmall.contains(lsst.geom.Point2I(100, 100)))

    def testComputeCentroid(self):
        spanSetShape = afwGeom.SpanSet.fromShape(4, afwGeom.Stencil.CIRCLE).shiftedBy(2, 2)
        center = spanSetShape.computeCentroid()

        self.assertEqual(center.getX(), 2)
        self.assertEqual(center.getY(), 2)

    def testComputeShape(self):
        spanSetShape = afwGeom.SpanSet.fromShape(1, afwGeom.Stencil.CIRCLE)
        quad = spanSetShape.computeShape()

        self.assertEqual(quad.getIxx(), 0.4)
        self.assertEqual(quad.getIyy(), 0.4)
        self.assertEqual(quad.getIxy(), 0)

    def testdilated(self):
        spanSetPredilated = afwGeom.SpanSet.fromShape(1, afwGeom.Stencil.CIRCLE)
        spanSetPostdilated = spanSetPredilated.dilated(1)

        bBox = spanSetPostdilated.getBBox()
        self.assertEqual(bBox.getMinX(), -2)
        self.assertEqual(bBox.getMinY(), -2)

    def testErode(self):
        spanSetPreErode = afwGeom.SpanSet.fromShape(2, afwGeom.Stencil.CIRCLE)
        spanSetPostErode = spanSetPreErode.eroded(1)

        bBox = spanSetPostErode.getBBox()
        self.assertEqual(bBox.getMinX(), -1)
        self.assertEqual(bBox.getMinY(), -1)

    def testFlatten(self):
        # Give an initial value to an input array
        inputArray = np.ones((6, 6)) * 9
        inputArray[1, 1] = 1
        inputArray[1, 2] = 2
        inputArray[2, 1] = 3
        inputArray[2, 2] = 4

        inputSpanSet = afwGeom.SpanSet([afwGeom.Span(0, 0, 1),
                                        afwGeom.Span(1, 0, 1)])
        flatArr = inputSpanSet.flatten(inputArray, lsst.geom.Point2I(-1, -1))

        self.assertEqual(flatArr.size, inputSpanSet.getArea())

        # Test flatttening a 3D array
        spanSetArea = afwGeom.SpanSet.fromShape(2, afwGeom.Stencil.BOX)
        spanSetArea = spanSetArea.shiftedBy(2, 2)

        testArray = np.arange(5*5*3).reshape(5, 5, 3)
        #print('------------- flatten ------------------')
        #print(testArray)
        flattened2DArray = spanSetArea.flatten(testArray)

        #print(flattened2DArray)
        #print('------------- flatten ------------------')
        truthArray = np.arange(5*5*3).reshape(5*5, 3)
        self.assertFloatsAlmostEqual(flattened2DArray, truthArray)

    def testUnflatten(self):
        inputArray = np.ones(6) * 4
        inputSpanSet = afwGeom.SpanSet([afwGeom.Span(9, 2, 3),
                                        afwGeom.Span(10, 3, 4),
                                        afwGeom.Span(11, 2, 3)])
        outputArray = inputSpanSet.unflatten(inputArray)

        arrayShape = outputArray.shape
        bBox = inputSpanSet.getBBox()
        self.assertEqual(arrayShape[0], bBox.getHeight())
        self.assertEqual(arrayShape[1], bBox.getWidth())

        # Test unflattening a 2D array
        spanSetArea = afwGeom.SpanSet.fromShape(2, afwGeom.Stencil.BOX)
        spanSetArea = spanSetArea.shiftedBy(2, 2)

        testArray = np.arange(5*5*3).reshape(5*5, 3)
        unflattened3DArray = spanSetArea.unflatten(testArray)

        truthArray = np.arange(5*5*3).reshape(5, 5, 3)
        self.assertFloatsAlmostEqual(unflattened3DArray, truthArray)

    def populateMask(self):
        msk = afwImage.Mask(10, 10, 1)
        spanSetMask = afwGeom.SpanSet.fromShape(3, afwGeom.Stencil.CIRCLE).shiftedBy(5, 5)
        spanSetMask.setMask(msk, 2)
        return msk, spanSetMask

    def testSetMask(self):
        mask, spanSetMask = self.populateMask()
        mskArray = mask.getArray()
        for i in range(mskArray.shape[0]):
            for j in range(mskArray.shape[1]):
                if lsst.geom.Point2I(i, j) in spanSetMask:
                    self.assertEqual(mskArray[i, j], 3)
                else:
                    self.assertEqual(mskArray[i, j], 1)

    def testClearMask(self):
        mask, spanSetMask = self.populateMask()
        spanSetMask.clearMask(mask, 2)
        mskArray = mask.getArray()
        for i in range(mskArray.shape[0]):
            for j in range(mskArray.shape[1]):
                self.assertEqual(mskArray[i, j], 1)

    def makeOverlapSpanSets(self):
        firstSpanSet = afwGeom.SpanSet.fromShape(2, afwGeom.Stencil.BOX).shiftedBy(2, 4)
        secondSpanSet = afwGeom.SpanSet.fromShape(2, afwGeom.Stencil.BOX).shiftedBy(2, 2)
        return firstSpanSet, secondSpanSet

    def makeMaskAndSpanSetForOperationTest(self):
        firstMaskPart = afwGeom.SpanSet.fromShape(2, afwGeom.Stencil.BOX).shiftedBy(3, 2)
        secondMaskPart = afwGeom.SpanSet.fromShape(2, afwGeom.Stencil.BOX).shiftedBy(3, 8)
        spanSetMaskOperation = afwGeom.SpanSet.fromShape(2, afwGeom.Stencil.BOX).shiftedBy(3, 5)

        mask = afwImage.Mask(20, 20)
        firstMaskPart.setMask(mask, 3)
        secondMaskPart.setMask(mask, 3)
        spanSetMaskOperation.setMask(mask, 4)

        return mask, spanSetMaskOperation

    def testIntersection(self):
        firstSpanSet, secondSpanSet = self.makeOverlapSpanSets()

        overlap = firstSpanSet.intersect(secondSpanSet)
        for i, span in enumerate(overlap):
            self.assertEqual(span.getY(), i+2)
            self.assertEqual(span.getMinX(), 0)
            self.assertEqual(span.getMaxX(), 4)

        mask, spanSetMaskOperation = self.makeMaskAndSpanSetForOperationTest()
        spanSetIntersectMask = spanSetMaskOperation.intersect(mask, 2)

        expectedYRange = [3, 4, 6, 7]
        for expected, val in zip(expectedYRange, spanSetIntersectMask):
            self.assertEqual(expected, val.getY())

    def testIntersectNot(self):
        firstSpanSet, secondSpanSet = self.makeOverlapSpanSets()

        overlap = firstSpanSet.intersectNot(secondSpanSet)
        for yVal, span in enumerate(overlap):
            self.assertEqual(span.getY(), yVal+5)
            self.assertEqual(span.getMinX(), 0)
            self.assertEqual(span.getMaxX(), 4)

        mask, spanSetMaskOperation = self.makeMaskAndSpanSetForOperationTest()

        spanSetIntersectNotMask = spanSetMaskOperation.intersectNot(mask, 2)

        self.assertEqual(len(spanSetIntersectNotMask), 1)
        self.assertEqual(next(iter(spanSetIntersectNotMask)).getY(), 5)

        # More complicated intersection with disconnected SpanSets
        spanList1 = [afwGeom.Span(0, 0, 10),
                     afwGeom.Span(1, 0, 10),
                     afwGeom.Span(2, 0, 10)]

        spanList2 = [afwGeom.Span(1, 2, 4), afwGeom.Span(1, 7, 8)]

        resultList = [afwGeom.Span(0, 0, 10),
                      afwGeom.Span(1, 0, 1),
                      afwGeom.Span(1, 5, 6),
                      afwGeom.Span(1, 9, 10),
                      afwGeom.Span(2, 0, 10)]

        spanSet1 = afwGeom.SpanSet(spanList1)
        spanSet2 = afwGeom.SpanSet(spanList2)
        expectedSpanSet = afwGeom.SpanSet(resultList)

        outputSpanSet = spanSet1.intersectNot(spanSet2)

        self.assertEqual(outputSpanSet, expectedSpanSet)

        numIntersectNotTrials = 100
        spanRow = 5
        # Set a seed for random functions
        np.random.seed(400)
        for N in range(numIntersectNotTrials):
            # Create two random SpanSets, both with holes in them
            listOfRandomSpanSets = []
            for i in range(2):
                # Make two rectangles to be turned into a SpanSet
                rand1 = np.random.randint(0, 26, 2)
                rand2 = np.random.randint(rand1.max(), 51, 2)
                tempList = [afwGeom.Span(spanRow, rand1.min(), rand1.max()),
                            afwGeom.Span(spanRow, rand2.min(), rand2.max())]
                listOfRandomSpanSets.append(afwGeom.SpanSet(tempList))

            # IntersectNot the SpanSets, randomly choosing which one is the one
            # to be the negated SpanSet
            randChoice = np.random.randint(0, 2)
            negatedRandChoice = int(not randChoice)
            sourceSpanSet = listOfRandomSpanSets[randChoice]
            targetSpanSet = listOfRandomSpanSets[negatedRandChoice]
            resultSpanSet = sourceSpanSet.intersectNot(targetSpanSet)
            for span in resultSpanSet:
                for point in span:
                    self.assertTrue(sourceSpanSet.contains(point))
                    self.assertFalse(targetSpanSet.contains(point))

            for x in range(51):
                point = lsst.geom.Point2I(x, spanRow)
                if sourceSpanSet.contains(point) and not\
                        targetSpanSet.contains(point):
                    self.assertTrue(resultSpanSet.contains(point))

    def testUnion(self):
        firstSpanSet, secondSpanSet = self.makeOverlapSpanSets()

        overlap = firstSpanSet.union(secondSpanSet)

        for yVal, span in enumerate(overlap):
            self.assertEqual(span.getY(), yVal)
            self.assertEqual(span.getMinX(), 0)
            self.assertEqual(span.getMaxX(), 4)

        mask, spanSetMaskOperation = self.makeMaskAndSpanSetForOperationTest()

        spanSetUnion = spanSetMaskOperation.union(mask, 2)

        for yVal, span in enumerate(spanSetUnion):
            self.assertEqual(span.getY(), yVal)

    def testMaskToSpanSet(self):
        mask, _ = self.makeMaskAndSpanSetForOperationTest()
        spanSetFromMask = afwGeom.SpanSet.fromMask(mask)

        for yCoord, span in enumerate(spanSetFromMask):
            self.assertEqual(span, afwGeom.Span(yCoord, 1, 5))

    def testFromMask(self):
        xy0 = lsst.geom.Point2I(12345, 67890)  # xy0 for image
        dims = lsst.geom.Extent2I(123, 45)  # Dimensions of image
        box = lsst.geom.Box2I(xy0, dims)  # Bounding box of image
        value = 32
        other = 16
        assert value & other == 0  # Setting 'other' unsets 'value'
        point = lsst.geom.Point2I(3 + xy0.getX(), 3 + xy0.getY())  # Point in the image

        mask = afwImage.Mask(box)
        mask.set(value)

        # We can create a SpanSet from bit planes
        spans = afwGeom.SpanSet.fromMask(mask, value)
        self.assertEqual(spans.getArea(), box.getArea())
        self.assertEqual(spans.getBBox(), box)
        self.assertTrue(point in spans)

        # Pixels not matching the desired bit plane are ignored as they should be
        mask[point] = other  # unset one pixel
        spans = afwGeom.SpanSet.fromMask(mask, value)
        self.assertEqual(spans.getArea(), box.getArea() - 1)
        self.assertEqual(spans.getBBox(), box)
        self.assertFalse(point in spans)

    def testEquality(self):
        firstSpanSet, secondSpanSet = self.makeOverlapSpanSets()
        secondSpanSetShift = secondSpanSet.shiftedBy(0, 2)

        self.assertFalse(firstSpanSet == secondSpanSet)
        self.assertTrue(firstSpanSet != secondSpanSet)
        self.assertTrue(firstSpanSet == secondSpanSetShift)
        self.assertFalse(firstSpanSet != secondSpanSetShift)

    def testSpanSetFromEllipse(self):
        axes = afwGeomEllipses.Axes(6, 6, 0)
        ellipse = afwGeom.Ellipse(axes, lsst.geom.Point2D(5, 6))
        spanSet = afwGeom.SpanSet.fromShape(ellipse)
        for ss, es in zip(spanSet, afwGeomEllipses.PixelRegion(ellipse)):
            self.assertEqual(ss, es)

    def testfromShapeOffset(self):
        shift = lsst.geom.Point2I(2, 2)
        spanSetShifted = afwGeom.SpanSet.fromShape(2, offset=shift)
        bbox = spanSetShifted.getBBox()
        self.assertEqual(bbox.getMinX(), 0)
        self.assertEqual(bbox.getMinY(), 0)

    def testFindEdgePixels(self):
        spanSet = afwGeom.SpanSet.fromShape(6, afwGeom.Stencil.CIRCLE)
        spanSetEdge = spanSet.findEdgePixels()

        truthSpans = [afwGeom.Span(-6, 0, 0),
                      afwGeom.Span(-5, -3, -1),
                      afwGeom.Span(-5, 1, 3),
                      afwGeom.Span(-4, -4, -4),
                      afwGeom.Span(-4, 4, 4),
                      afwGeom.Span(-3, -5, -5),
                      afwGeom.Span(-3, 5, 5),
                      afwGeom.Span(-2, -5, -5),
                      afwGeom.Span(-2, 5, 5),
                      afwGeom.Span(-1, -5, -5),
                      afwGeom.Span(-1, 5, 5),
                      afwGeom.Span(0, -6, -6),
                      afwGeom.Span(0, 6, 6),
                      afwGeom.Span(1, -5, -5),
                      afwGeom.Span(1, 5, 5),
                      afwGeom.Span(2, -5, -5),
                      afwGeom.Span(2, 5, 5),
                      afwGeom.Span(3, -5, -5),
                      afwGeom.Span(3, 5, 5),
                      afwGeom.Span(4, -4, -4),
                      afwGeom.Span(4, 4, 4),
                      afwGeom.Span(5, -3, -1),
                      afwGeom.Span(5, 1, 3),
                      afwGeom.Span(6, 0, 0)]
        truthSpanSet = afwGeom.SpanSet(truthSpans)
        self.assertEqual(spanSetEdge, truthSpanSet)

    def testIndices(self):
        dataArray = np.zeros((5, 5))
        spanSet = afwGeom.SpanSet.fromShape(2,
                                            afwGeom.Stencil.BOX,
                                            offset=(2, 2))
        yind, xind = spanSet.indices()
        dataArray[yind, xind] = 9
        self.assertTrue((dataArray == 9).all())

    def testSpanIteration(self):
        span = afwGeom.Span(4, 3, 8)
        points = list(span)
        self.assertEqual(len(span), len(points))
        self.assertEqual(points, [lsst.geom.Point2I(x, 4) for x in range(3, 9)])

    def testAsArray(self):
        spans = afwGeom.SpanSet(lsst.geom.Box2I(lsst.geom.Point2I(2, 3), lsst.geom.Point2I(6, 7)))
        truth = np.ones((5, 5), dtype=bool)
        arr = spans.asArray()
        np.testing.assert_array_equal(arr, truth)

        shape = (10, 10)
        truth = np.zeros(shape, dtype=bool)
        truth[:5, :5] = 1
        arr = spans.asArray(shape)
        np.testing.assert_array_equal(arr, truth)

        truth = np.zeros(shape, dtype=bool)
        truth[3:8, 2:7] = 1
        arr = spans.asArray(shape, (0, 0))
        np.testing.assert_array_equal(arr, truth)


class TestMemory(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
