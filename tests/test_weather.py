#
# LSST Data Management System
# Copyright 2016 LSST Corporation.
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
Tests for lsst.afw.image.Weather
"""
import unittest

import numpy as np

import lsst.utils.tests
import lsst.pex.exceptions
from lsst.afw.coord import Weather


class WeatherTestCase(unittest.TestCase):
    """Test lsst.afw.coord.Weather, a simple struct-like class"""

    def testBasics(self):
        prevWeather = None
        for temp, pressure in ((1.1, 2.2), (100.1, 200.2)):  # arbitrary values
            # 0 and greater, including supersaturation
            for humidity in (0.0, 10.1, 100.0, 120.5):
                weather = Weather(temp, pressure, humidity)
                self.assertEqual(weather.getAirTemperature(), temp)
                self.assertEqual(weather.getAirPressure(), pressure)
                self.assertEqual(weather.getHumidity(), humidity)

                # test copy constructor
                weatherCopy = Weather(weather)
                self.assertEqual(weatherCopy.getAirTemperature(), temp)
                self.assertEqual(weatherCopy.getAirPressure(), pressure)
                self.assertEqual(weatherCopy.getHumidity(), humidity)

                # test == (using a copy, to make sure the test is not based on
                # identity) and !=
                self.assertEqual(weather, weatherCopy)
                if prevWeather is not None:
                    self.assertNotEqual(weather, prevWeather)
                prevWeather = weather

    def testBadHumidity(self):
        """Check bad humidity handling (humidity is the only value whose range is checked)"""
        for humidity in (-1, -0.0001):
            with self.assertRaises(lsst.pex.exceptions.InvalidParameterError):
                Weather(1.1, 2.2, humidity)

    def testEquals(self):
        weather1 = Weather(1.1, 100.1, 10.1)
        weather2 = Weather(2.2, 200.2, 0.0)
        weather3 = Weather(np.nan, np.nan, np.nan)

        # objects with "same" values should be equal
        self.assertEqual(Weather(1.1, 100.1, 10.1), Weather(1.1, 100.1, 10.1))
        self.assertEqual(Weather(np.nan, np.nan, np.nan), Weather(np.nan, np.nan, np.nan))
        self.assertNotEqual(weather1, weather2)
        self.assertNotEqual(weather3, weather2)

        # equality must be reflexive
        self.assertEqual(weather1, weather1)
        self.assertEqual(weather2, weather2)
        self.assertEqual(weather3, weather3)


def setup_module(module):
    lsst.utils.tests.init()


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
