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

from collections.abc import MutableMapping
import unittest

import lsst.utils.tests
import lsst.pex.exceptions as pexExcept

from lsst.afw.typehandling import SimpleGenericMap, Storable
from lsst.afw.typehandling.testUtils import MutableGenericMapTestBaseClass
import testGenericMapLib as cppLib


class SimpleGenericMapTestSuite(MutableGenericMapTestBaseClass):

    @staticmethod
    def makeMap(mapType, values):
        """Initialize a map type using __setattr__ instead of a bulk constructor.
        """
        result = mapType()
        for key, value in values.items():
            result[key] = value
        return result

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.genericConstructor = SimpleGenericMap  # Must be mapping-constructible
        cls.targets = {SimpleGenericMap[str]}  # Must be default-constructible
        cls.examples = {
            "SimpleGenericMap(testData(str))": (SimpleGenericMap[str], cls.getTestData(str)),
            "SimpleGenericMap(testKeys(str) : 0)":
                (SimpleGenericMap[str], {key: 0 for key in cls.getTestData(str).keys()}),
            "SimpleGenericMap(dtype=str)": (SimpleGenericMap[str], {}),
        }

    def tearDown(self):
        pass

    def testClass(self):
        for target in self.targets:
            self.assertTrue(issubclass(target, MutableMapping))
            self.assertIsInstance(target(), MutableMapping)

    def testInitKeywords(self):
        for target in self.targets:
            for keyType in self.getValidKeys(target):
                self.checkInitKwargs(target, self.getTestData(keyType), msg=str(target))

    def testInitPairs(self):
        for target in self.targets | {self.genericConstructor}:
            for keyType in self.getValidKeys(target):
                self.checkInitPairs(target, self.getTestData(keyType), msg=str(target))

    def testInitMapping(self):
        for target in self.targets | {self.genericConstructor}:
            for keyType in self.getValidKeys(target):
                # Init from dict
                self.checkInitMapping(target, self.getTestData(keyType), msg=str(target))
                # Init from GenericMap
                self.checkInitMapping(target, target(self.getTestData(keyType)),
                                      msg=str(target))

    def testUnknownKeys(self):
        with self.assertRaises(TypeError):
            self.genericConstructor()
        # Should not raise
        self.genericConstructor(dtype=str)

    def testMixedKeys(self):
        badData = {"What do you get if you multiply six by nine?": "Ultimate Question",
                   42: "Ultimate Answer",
                   }
        for target in self.targets | {self.genericConstructor}:
            with self.assertRaises(TypeError):
                target(badData)
            with self.assertRaises(TypeError):
                target(badData.items())
        for target in self.targets:
            with self.assertRaises(TypeError):
                target(**badData)

    def testFromKeys(self):
        for target in self.targets:
            for keyType in self.getValidKeys(target):
                keys = self.getTestData(keyType).keys()
                for value in self.getTestData(keyType).values():
                    self.checkFromKeys(target, keys, value,
                                       msg=f" class={target}, value={value}")
                self.checkFromKeysDefault(target, keys, msg=f" class={target}, no value")

    def testCopy(self):
        for label, (mappingType, contents) in self.examples.items():
            for keyType in self.getValidKeys(mappingType):
                mapping1 = self.makeMap(mappingType, contents)
                mapping2 = mapping1.copy()
                self.assertEqual(mapping1, mapping2, msg=f"{label}")
                mapping1[keyType(42)] = "A random value!"
                self.assertNotEqual(mapping1, mapping2, msg=f"{label}")

    def testEquality(self):
        for label1, (mappingType1, contents1) in self.examples.items():
            mapping1 = self.makeMap(mappingType1, contents1)
            for label2, (mappingType2, contents2) in self.examples.items():
                mapping2 = self.makeMap(mappingType2, contents2)
                if contents1 == contents2:
                    self.assertIsNot(mapping1, mapping2, msg=f"{label1} vs {label2}")
                    self.assertEqual(mapping1, mapping2, msg=f"{label1} vs {label2}")
                    self.assertEqual(mapping1, contents2, msg=f"{label1} vs dict({label2})")
                    self.assertEqual(contents1, mapping2, msg=f"dict({label1}) vs {label2}")
                else:
                    self.assertNotEqual(mapping1, mapping2, msg=f"{label1} vs {label2}")
                    self.assertNotEqual(mapping1, contents2, msg=f"{label1} vs dict({label2})")
                    self.assertNotEqual(contents1, mapping2, msg=f"dict({label1}) vs {label2}")

    def testBool(self):
        for label, (mappingType, contents) in self.examples.items():
            mapping = self.makeMap(mappingType, contents)
            if contents:
                self.assertTrue(mapping, msg=label)
            else:
                self.assertFalse(mapping, msg=label)

    def testContains(self):
        for label, (mappingType, contents) in self.examples.items():
            mapping = self.makeMap(mappingType, contents)
            self.checkContains(mapping, contents, msg=label)

    def testContents(self):
        for label, (mappingType, contents) in self.examples.items():
            mapping = self.makeMap(mappingType, contents)
            self.checkContents(mapping, contents, msg=label)

    def testGet(self):
        for label, (mappingType, contents) in self.examples.items():
            mapping = self.makeMap(mappingType, contents)
            self.checkGet(mapping, contents, msg=label)

    def testIteration(self):
        for label, (mappingType, contents) in self.examples.items():
            mapping = self.makeMap(mappingType, contents)
            self.checkIteration(mapping, contents, msg=label)

    def testViews(self):
        for label, (mappingType, contents) in self.examples.items():
            self.checkMutableViews(mappingType, contents, msg=label)

    def testInsertItem(self):
        for target in self.targets:
            for keyType in self.getValidKeys(target):
                self.checkInsertItem(target, self.getTestData(keyType), msg=str(target))

    def testSetdefault(self):
        for target in self.targets:
            for keyType in self.getValidKeys(target):
                self.checkSetdefault(target, self.getTestData(keyType), msg=str(target))

    def testUpdateMapping(self):
        for target in self.targets:
            for keyType in self.getValidKeys(target):
                # Update from dict
                self.checkUpdateMapping(target, self.getTestData(keyType), msg=str(target))
                # Update from GenericMap
                self.checkUpdateMapping(target, self.makeMap(target, self.getTestData(keyType)),
                                        msg=str(target))

    def testUpdatePairs(self):
        for target in self.targets:
            for keyType in self.getValidKeys(target):
                self.checkUpdatePairs(target, self.getTestData(keyType), msg=str(target))

    def testUpdateKwargs(self):
        for target in self.targets:
            for keyType in self.getValidKeys(target):
                self.checkUpdateKwargs(target, self.getTestData(keyType), msg=str(target))

    def testReplaceItem(self):
        for target in self.targets:
            self.checkReplaceItem(target(), msg=str(target))

    def testRemoveItem(self):
        for target in self.targets:
            for keyType in self.getValidKeys(target):
                self.checkRemoveItem(target, self.getTestData(keyType), msg=str(target))

    def testPop(self):
        for target in self.targets:
            for keyType in self.getValidKeys(target):
                self.checkPop(target, self.getTestData(keyType), msg=str(target))

    def testPopitem(self):
        for target in self.targets:
            for keyType in self.getValidKeys(target):
                self.checkPopitem(target, self.getTestData(keyType), msg=str(target))

    def testClear(self):
        for target in self.targets:
            for keyType in self.getValidKeys(target):
                self.checkClear(target, self.getTestData(keyType), msg=str(target))


class PyStorable(Storable):
    """A Storable with simple, mutable state.

    Parameters
    ----------
    value
        A value to be stored inside the object. Affects the object's string
        representation. Two PyStorables are equal if and only if their
        internal values are the same.
    """

    def __init__(self, value):
        Storable.__init__(self)  # pybind11 discourages using super()
        self.value = value

    def __repr__(self):
        return repr(self.value)

    def __eq__(self, other):
        return self.value == other.value


class SimpleGenericMapCppTestSuite(lsst.utils.tests.TestCase):
    def setUp(self):
        self.data = {'one': 1,
                     'pi': 3.1415927,
                     'string': 'neither a number nor NaN',
                     }
        self.pymap = SimpleGenericMap(self.data)
        self.cppmap = cppLib.makeInitialMap()

    def testPythonValues(self):
        """Check that built-in types added in Python are visible in C++.
        """
        for key, value in self.data.items():
            cppLib.assertKeyValue(self.pymap, key, value)
        # Ensure the test isn't giving false negatives
        with self.assertRaises(pexExcept.NotFoundError):
            cppLib.assertKeyValue(self.pymap, "NotAKey", 42)

    def testCppValues(self):
        """Check that built-in types added in C++ are visible in Python.
        """
        for key, value in self.data.items():
            self.assertIn(key, self.cppmap)
            self.assertEqual(value, self.cppmap[key], msg="key=" + key)
        # Ensure the test isn't giving false negatives
        self.assertNotIn("NotAKey", self.cppmap)

    def _checkPythonUpdates(self, testmap, msg=''):
        for key, value in self.data.items():
            self.assertIn(key, testmap, msg=msg)
            self.assertEqual(value, testmap[key], msg='key=' + key + ', ' + msg)
            cppLib.assertKeyValue(testmap, key, value)
        testmap['answer'] = 42  # New key-value pair
        testmap['pi'] = 3.0  # Replace `float` with `float`
        testmap['string'] = False  # Replace `str` with `bool`

        for key, value in {'answer': 42, 'pi': 3.0, 'string': False}.items():
            # Test both Python and C++ state
            self.assertIn(key, testmap, msg=msg)
            self.assertEqual(value, testmap[key], msg='key=' + key + ', ' + msg)
            cppLib.assertKeyValue(testmap, key, value)

    def testPythonUpdates(self):
        """Check that changes to built-in types made in Python are visible in
        both languages.
        """
        self._checkPythonUpdates(self.pymap, msg='map=pymap')
        self._checkPythonUpdates(self.cppmap, msg='map=cppmap')

    def _checkCppUpdates(self, testmap, msg=''):
        for key, value in self.data.items():
            self.assertIn(key, testmap, msg=msg)
            self.assertEqual(value, testmap[key], msg='key=' + key + ', ' + msg)
            cppLib.assertKeyValue(testmap, key, value)
        cppLib.makeCppUpdates(testmap)

        for key, value in {'answer': 42, 'pi': 3.0, 'string': False}.items():
            # Test both Python and C++ state
            self.assertIn(key, testmap, msg=msg)
            self.assertEqual(value, testmap[key], msg='key=' + key + ', ' + msg)
            cppLib.assertKeyValue(testmap, key, value)

    def testCppUpdates(self):
        """Check that changes to built-in types made in C++ are visible in
        both languages.
        """
        self._checkCppUpdates(self.pymap, msg='map=pymap')
        self._checkCppUpdates(self.cppmap, msg='map=cppmap')

    def _checkPythonStorableUpdates(self, testmap, msg=''):
        cppLib.addCppStorable(testmap)
        self.assertIn('cppValue', testmap, msg=msg)
        self.assertEqual(testmap['cppValue'], cppLib.CppStorable('value'), msg=msg)
        self.assertIn('cppPointer', testmap, msg=msg)
        self.assertEqual(testmap['cppPointer'], cppLib.CppStorable('pointer'), msg=msg)

        # should have no effect because pybind11 copies Storable values for safety
        testmap['cppValue'].value = 'new_value'
        testmap['cppPointer'].value = 'extra_pointy'

        for key, value in {'cppValue': cppLib.CppStorable('value'),
                           'cppPointer': cppLib.CppStorable('extra_pointy'),
                           }.items():
            # Test both Python and C++ state
            self.assertIn(key, testmap, msg=msg)
            self.assertEqual(value, testmap[key], msg='key=' + key + ', ' + msg)
            cppLib.assertKeyValue(testmap, key, value)

    def testPythonStorableUpdates(self):
        """Check that changes to Storables made in Python are visible in
        both languages.
        """
        self._checkPythonStorableUpdates(self.pymap, msg='map=pymap')
        self._checkPythonStorableUpdates(self.cppmap, msg='map=cppmap')

    def _checkCppStorableRead(self, testmap, msg=''):
        # WARNING: the Python variables holding PyStorable must survive to the end of the test
        # This is a known bug in pybind11; see DM-21314
        storableData = {'answer': PyStorable(42),
                        'question': PyStorable('Unknown'),
                        }
        testmap.update(storableData)

        for key, value in storableData.items():
            self.assertIn(key, testmap, msg=msg)
            self.assertEqual(value, testmap[key], msg='key=' + key + ', ' + msg)
            # Exercise C++ equality operator
            cppLib.assertKeyValue(testmap, key, PyStorable(value.value))
            # Exercise C++ string representation
            cppLib.assertPythonStorable(testmap, key, repr(value))

    def testCppStorableRead(self):
        """Check that Storables made in Python are visible in both languages.
        """
        self._checkCppStorableRead(self.pymap, msg='map=pymap')
        self._checkCppStorableRead(self.cppmap, msg='map=cppmap')


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
