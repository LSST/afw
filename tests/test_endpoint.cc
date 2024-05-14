// -*- LSST-C++ -*-

/*
 * LSST Data Management System
 * See COPYRIGHT file at the top of the source tree.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program. If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE EndpointCpp

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

#include "lsst/afw/geom/Endpoint.h"

using namespace std;

/* C++ unit tests for Endpoint.
 *
 * In addition to tests of C++-only functionality, this test suite includes
 * tests that need custom subclasses of standard endpoints. While these could
 * be written in Python, allowing Python classes to inherit from nanobind
 * wrappers requires a lot of extra support code.
 *
 * See test_endpoint.py for remaining unit tests.
 */
namespace lsst {
namespace afw {
namespace geom {

namespace {
/**
 * A SpherePointEndpoint whose sole purpose is to be a nontrivial subclass
 * of SpherePointEndpoint.
 */
class GratuitousSpherePointEndpoint : public SpherePointEndpoint {
    // Default constructor is adequate
};

/**
 * An SpherePointEndpoint that adds equality-relevant fields to SpherePointEndpoint.
 */
class CoolSpherePointEndpoint : public SpherePointEndpoint {
public:
    CoolSpherePointEndpoint(string const &coolness) : SpherePointEndpoint(), _coolness(coolness) {}
    string getCoolness() const noexcept { return _coolness; }

    bool operator==(BaseEndpoint const &other) const noexcept override {
        if (!SpherePointEndpoint::operator==(other)) {
            return false;
        } else {
            // Guaranteed not to throw by BaseEndpoint::operator==
            auto coolOther = dynamic_cast<CoolSpherePointEndpoint const &>(other);
            return _coolness == coolOther._coolness;
        }
    }

private:
    string const _coolness;
};
}  // namespace

/// Test whether Endpoint equality treats subclasses as non-substitutable.
BOOST_AUTO_TEST_CASE(EndpointEqualsNotPolymorphic) {
    SpherePointEndpoint superclass;
    GratuitousSpherePointEndpoint subclass;

    // Test both SpherePointEndpoint::operator== and GratuitousSpherePointEndpoint::operator==
    BOOST_TEST(superclass != subclass);
    BOOST_TEST(subclass != superclass);
    BOOST_TEST(!(superclass == subclass));
    BOOST_TEST(!(subclass == superclass));
}

string printEquals(bool equal, string const &name1, string const &name2) {
    return name1 + (equal ? " == " : " != ") + name2;
}

/**
 * Test whether Endpoint equality is symmetric and transitive even in the
 * presence of subclasses that add value fields.
 */
BOOST_AUTO_TEST_CASE(EndpointEqualsAlgebraic) {
    using namespace std::string_literals;

    vector<std::shared_ptr<SpherePointEndpoint const>> endpoints = {
            make_shared<CoolSpherePointEndpoint>("Very cool"s),
            make_shared<CoolSpherePointEndpoint>("Slightly nifty"s),
            make_shared<CoolSpherePointEndpoint>("Very cool"s), make_shared<SpherePointEndpoint>()};

    BOOST_TEST(*(endpoints[0]) != *(endpoints[1]));
    BOOST_TEST(*(endpoints[1]) != *(endpoints[0]));
    BOOST_TEST(*(endpoints[0]) == *(endpoints[2]));
    BOOST_TEST(*(endpoints[2]) == *(endpoints[0]));

    for (auto const& endpoint1 : endpoints) {
        for (auto const& endpoint2 : endpoints) {
            // == and != must always disagree
            if (*endpoint1 == *endpoint2) {
                BOOST_TEST(!(*endpoint1 != *endpoint2));
            } else {
                BOOST_TEST(*endpoint1 != *endpoint2);
            }

            for (auto const& endpoint3 : endpoints) {
                bool const equals12 = *endpoint1 == *endpoint2;
                bool const equals13 = *endpoint1 == *endpoint3;
                bool const equals23 = *endpoint2 == *endpoint3;

                // Test point1 == point2 && point1 == point3 => point2 == point3
                // Iteration covers all permutations, so no need to test, for example,
                //     point1 == point3 && point2 == point3 => point1 == point2
                bool const transitive = !equals12 || !equals13 || equals23;
                BOOST_TEST(transitive, "Not transitive: " << printEquals(equals12, "1", "2") << " "
                                                          << printEquals(equals13, "1", "3") << " "
                                                          << printEquals(equals23, "2", "3"));
            }
        }
    }
}

}  // namespace geom
}  // namespace afw
}  // namespace lsst
