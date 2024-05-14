/*
 * This file is part of afw.
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string>

// Despite this file being called "python.cc", it must not actually #include
// Python's main header (Python.h), which means it can't #include any nanobind
// headers either.  The symbols referenced by that header are provided by the
// python executable itself, so including them in a non-Python library like
// libafw.so will make it impossible to link it to direct C++ binaries (like
// many of our tests).

namespace lsst {
namespace afw {
namespace typehandling {

std::string declareGenericMapRestrictions(std::string const& className, std::string const& keyName) {
    // Give the class a custom docstring to avoid confusing Python users
    std::string docstring = R"docstring(
For compatibility with C++, ``)docstring" +
                            className + R"docstring(`` has the following restrictions:
    - all keys must be )docstring" + keyName +
                            R"docstring(
    - values must be built-in types or subclasses of `lsst.afw.typehandling.Storable`.
      Almost any user-defined class in C++ or Python can have
      `~lsst.afw.typehandling.Storable` as a mixin.

As a safety precaution, `~lsst.afw.typehandling.Storable` objects that are
added from C++ may be copied when you retrieve them from Python, making it
impossible to modify them in-place. This issue does not affect objects that
are added from Python, or objects that are always passed by
:cpp:class:`shared_ptr` in C++.
)docstring";
    return docstring;
}

}  // namespace typehandling
}  // namespace afw
}  // namespace lsst
