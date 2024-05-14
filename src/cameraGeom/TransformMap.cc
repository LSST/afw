// -*- lsst-c++ -*-
/*
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
#include <sstream>
#include <unordered_set>

#include "lsst/log/Log.h"
#include "lsst/pex/exceptions.h"
#include "lsst/afw/table/io/OutputArchive.h"
#include "lsst/afw/table/io/CatalogVector.h"
#include "lsst/afw/table/io/Persistable.cc"
#include "lsst/afw/cameraGeom/TransformMap.h"

namespace lsst {
namespace afw {
namespace cameraGeom {

namespace {

// Allows conversions between LSST and AST data formats
static lsst::afw::geom::Point2Endpoint const POINT2_ENDPOINT;

static auto LOGGER = LOG_GET("lsst.afw.cameraGeom.TransformMap");

// Make an AST Frame name for a CameraSys.
std::string makeFrameName(CameraSys const & sys) {
    std::string r = "Ident=" + sys.getSysName();
    if (sys.hasDetectorName()) {
        r += "_";
        r += sys.getDetectorName();
    }
    return r;
}

/*
 * Make a vector of `Connection` instances that can be safely passed to
 * TransformMap's private constructor.
 *
 * This guarantees that:
 *  - Connections are sorted according to their distance (in number of
 *    intermediate connections) from the given reference `CameraSys`;
 *  - The `fromSys` of each `Connection` is closer to the reference than the
 *    `toSys`.
 *
 * @param[in] reference   Reference coordinate system.  All systems must be
 *                        (indirectly) connected to this system, and will be
 *                        sorted according to the number of connections to it.
 * @param[in] connections Vector of `Connection` instances.  Passed by value so
 *                        we can either move into it (avoiding a copy) or copy
 *                        into it (when we have a const reference and a copy is
 *                        unavoidable), depending on the context.
 *
 * @returns connections An updated version of the connections vector.
 *
 * @throws pex::exceptions::InvalidParameterError  Thrown if the vector of
 *      connections graph is empty, contains cycles, is not fully connected, or
 *      includes any connections in which `fromSys == toSys`.
 */
std::vector<TransformMap::Connection> standardizeConnections(
    CameraSys const & reference,
    std::vector<TransformMap::Connection> connections
) {
    if (connections.empty()) {
        throw LSST_EXCEPT(
            pex::exceptions::InvalidParameterError,
            "Cannot create a TransformMap with no connections."
        );
    }
    // Iterator to the first unprocessed connection in result; will be
    // incremented as we proceed.
    auto firstUnprocessed = connections.begin();
    // All CameraSys whose associated Connections are already in the processed
    // part of `connections`.
    std::unordered_set<CameraSys> knownSystems = {reference};
    // The set of CameraSys whose associated Connections are being processed
    // in this iteration of the outer (while) loop.  These are all some common
    // distance N from the reference system (in number of connections), where
    // N increases for each iteration (but is not tracked).
    std::unordered_set<CameraSys> currentSystems = {reference};
    // The set of CameraSys that will become currentSys at the next
    // iteration.
    std::unordered_set<CameraSys> nextSystems;
    LOGLS_DEBUG(LOGGER, "Standardizing: starting with reference " << reference);
    while (!currentSystems.empty()) {
        LOGLS_DEBUG(LOGGER, "Standardizing: beginning iteration with currentSystems={ ");
        for (auto const & sys : currentSystems) {
            LOGLS_DEBUG(LOGGER, "Standardizing:   " << sys << ", ");
        }
        LOGLS_DEBUG(LOGGER, "Standardizing: }");
        // Iterate over all unsorted connections, looking for those associated
        // with a CameraSys in currentSystems.
        for (auto connection = firstUnprocessed; connection != connections.end(); ++connection) {
            bool related = currentSystems.count(connection->fromSys) > 0;
            if (!related && currentSystems.count(connection->toSys)) {
                LOGLS_DEBUG(LOGGER, "Standardizing: reversing " << (*connection));
                // Safe because `connections` is passed by value.
                connection->reverse();
                related = true;
            }
            if (related) {
                if (connection->toSys == connection->fromSys) {
                    std::ostringstream ss;
                    ss << "Identity connection found: " << (*connection) << ".";
                    throw LSST_EXCEPT(pex::exceptions::InvalidParameterError, ss.str());
                }
                if (knownSystems.count(connection->toSys)) {
                    std::ostringstream ss;
                    ss << "Multiple paths between reference " << reference
                       << " and " << connection->toSys << ".";
                    throw LSST_EXCEPT(pex::exceptions::InvalidParameterError, ss.str());
                }
                LOGLS_DEBUG(LOGGER, "Standardizing: adding " << (*connection));
                nextSystems.insert(connection->toSys);
                knownSystems.insert(connection->toSys);
                std::swap(*firstUnprocessed, *connection);
                ++firstUnprocessed;
            }
        }
        currentSystems.swap(nextSystems);
        nextSystems.clear();
    }
    // Any connections we haven't processed yet must include only CameraSys
    // we've never seen before.
    if (firstUnprocessed != connections.end()) {
        std::ostringstream ss;
        ss << "Disconnected connection(s) found: " << (*firstUnprocessed);
        ++firstUnprocessed;
        for (auto connection = firstUnprocessed; connection != connections.end(); ++connection) {
            ss << ", " << (*connection);
        }
        ss << ".";
        throw LSST_EXCEPT(pex::exceptions::InvalidParameterError, ss.str());
    }
    // No RVO, because this is a function argument, but it's still a move so we
    // don't care.
    return connections;
}

// Return the reference coordinate system from an already-standardized vector of connections.
CameraSys getReferenceSys(std::vector<TransformMap::Connection> const & connections) {
    return connections.front().fromSys;
}

} // anonymous

void TransformMap::Connection::reverse() {
    transform = transform->inverted();
    toSys.swap(fromSys);
}

std::ostream & operator<<(std::ostream & os, TransformMap::Connection const & connection) {
    return os << connection.fromSys << "->" << connection.toSys;
}

std::shared_ptr<TransformMap const> TransformMap::make(
    CameraSys const & reference,
    Transforms const & transforms
) {
    std::vector<Connection> connections;
    connections.reserve(transforms.size());
    for (auto const & pair : transforms) {
        connections.push_back(Connection{pair.second, reference, pair.first});
    }
    // We can't use make_shared because TransformMap ctor is private.
    return std::shared_ptr<TransformMap>(
        new TransformMap(standardizeConnections(reference, std::move(connections)))
    );
}

void TransformMap::make(
        TransformMap *transformMap,
        CameraSys const & reference,
        Transforms const & transforms
) {
    std::vector<Connection> connections;
    connections.reserve(transforms.size());
    for (auto const & pair : transforms) {
        connections.push_back(Connection{pair.second, reference, pair.first});
    }
    // We can't use make_shared because TransformMap ctor is private.
    new (transformMap) TransformMap(standardizeConnections(reference, std::move(connections)));
}

std::shared_ptr<TransformMap const> TransformMap::make(
    CameraSys const &reference,
    std::vector<Connection> const & connections
) {
    // We can't use make_shared because TransformMap ctor is private.
    return std::shared_ptr<TransformMap>(
        new TransformMap(standardizeConnections(reference, connections))
    );
}

void TransformMap::make(
        TransformMap *transformMap,
        CameraSys const &reference,
        std::vector<Connection> const & connections
) {
    // We can't use make_shared because TransformMap ctor is private.
    new (transformMap) TransformMap(standardizeConnections(reference, connections));
}

// All resources owned by value or by smart pointer
TransformMap::~TransformMap() noexcept = default;

lsst::geom::Point2D TransformMap::transform(lsst::geom::Point2D const &point, CameraSys const &fromSys,
                                            CameraSys const &toSys) const {
    auto mapping = _getMapping(fromSys, toSys);
    return POINT2_ENDPOINT.pointFromData(mapping->applyForward(POINT2_ENDPOINT.dataFromPoint(point)));
}

std::vector<lsst::geom::Point2D> TransformMap::transform(std::vector<lsst::geom::Point2D> const &pointList,
                                                         CameraSys const &fromSys,
                                                         CameraSys const &toSys) const {
    auto mapping = _getMapping(fromSys, toSys);
    return POINT2_ENDPOINT.arrayFromData(mapping->applyForward(POINT2_ENDPOINT.dataFromArray(pointList)));
}

bool TransformMap::contains(CameraSys const &system) const noexcept { return _frameIds.count(system) > 0; }

std::shared_ptr<geom::TransformPoint2ToPoint2> TransformMap::getTransform(CameraSys const &fromSys,
                                                                          CameraSys const &toSys) const {
    return std::make_shared<geom::TransformPoint2ToPoint2>(*_getMapping(fromSys, toSys));
}

int TransformMap::_getFrame(CameraSys const &system) const {
    try {
        return _frameIds.at(system);
    } catch (std::out_of_range const &e) {
        std::ostringstream buffer;
        buffer << "Unsupported coordinate system: " << system;
        std::throw_with_nested(LSST_EXCEPT(pex::exceptions::InvalidParameterError, buffer.str()));
    }
}

std::shared_ptr<ast::Mapping const> TransformMap::_getMapping(CameraSys const &fromSys,
                                                              CameraSys const &toSys) const {
    return _frameSet->getMapping(_getFrame(fromSys), _getFrame(toSys));
}

size_t TransformMap::size() const noexcept { return _frameIds.size(); }


TransformMap::TransformMap(std::vector<Connection> && connections) :
    _connections(std::move(connections))
{
    // standardizeConnections must be run by anything that calls the
    // constructor, and that should throw on all of the conditions we assert
    // on below (which is why those are asserts).
    assert(!_connections.empty());

    int nFrames = 0;  // tracks frameSet->getNFrame() to avoid those (expensive) calls

    // Local helper function that creates a Frame, updates the nFrames counter,
    // and adds an entry to the frameIds map.  Returns the new Frame.
    // Should always be called in concert with an update to frameSet.
    auto addFrameForSys = [this, &nFrames](CameraSys const & sys) mutable -> ast::Frame {
        #ifndef NDEBUG
        auto r = // We only care about this return value for the assert below;
        #endif
        _frameIds.emplace(sys, ++nFrames);
        assert(r.second);  // this must actually insert something, not find an already-inserted CameraSys.
        return ast::Frame(2, makeFrameName(sys));
    };

    // FrameSet that manages all transforms; should always be updated in
    // concert with a call to addFrameForSys.
    _frameSet = std::make_unique<ast::FrameSet>(addFrameForSys(getReferenceSys(_connections)));

    for (auto const & connection : _connections) {
        auto fromSysIdIter = _frameIds.find(connection.fromSys);
        assert(fromSysIdIter != _frameIds.end());
        _frameSet->addFrame(fromSysIdIter->second, *connection.transform->getMapping(),
                            addFrameForSys(connection.toSys));
    }

    // We've maintained our own counter for frame IDs for performance and
    // convenience reasons, but it had better match AST's internal counter.
    assert(_frameSet->getNFrame() == nFrames);
}

std::vector<TransformMap::Connection> TransformMap::getConnections() const { return _connections; }


namespace {

struct PersistenceHelper {

    static PersistenceHelper const & get() {
        static PersistenceHelper const instance;
        return instance;
    }

    // Schema and keys for the catalog that stores Connection objects.
    // Considered as a graph, 'from' and 'to' identify vertices, and
    // 'transform' identifies an edge.
    table::Schema schema;
    table::Key<std::string> fromSysName;
    table::Key<std::string> fromSysDetectorName;
    table::Key<std::string> toSysName;
    table::Key<std::string> toSysDetectorName;
    table::Key<int> transform;

private:

    PersistenceHelper() :
        schema(),
        fromSysName(schema.addField<std::string>("fromSysName",
                                                 "Camera coordinate system name.", "", 0)),
        fromSysDetectorName(schema.addField<std::string>("fromSysDetectorName",
                                                         "Camera coordinate system detector name.", "", 0)),
        toSysName(schema.addField<std::string>("toSysName",
                                               "Camera coordinate system name.", "", 0)),
        toSysDetectorName(schema.addField<std::string>("toSysDetectorName",
                                                       "Camera coordinate system detector name.", "", 0)),
        transform(schema.addField<int>("transform", "Archive ID of the transform.", ""))
    {}

    PersistenceHelper(PersistenceHelper const &) = delete;
    PersistenceHelper(PersistenceHelper &&) = delete;

    PersistenceHelper & operator=(PersistenceHelper const &) = delete;
    PersistenceHelper & operator=(PersistenceHelper &&) = delete;

};


// PersistenceHelper for a previous format version; now only supported in
// reading.
struct OldPersistenceHelper {

    static OldPersistenceHelper const & get() {
        static OldPersistenceHelper const instance;
        return instance;
    }

    // Schema and keys for the catalog that stores TransformMap._frameIds.
    // Considered as a graph, this is a list of all of the vertices with the
    // integers that identify them in the list of edges below.
    table::Schema sysSchema;
    table::Key<std::string> sysName;
    table::Key<std::string> detectorName;
    table::Key<int> id;

    // Schema and keys for the catalog that stores
    // TransformMap._canonicalConnections entries and the associated Transform
    // extracted from TransformMap._transforms.
    // Considered as a graph, 'from' and 'to' identify vertices, and
    // 'transform' identifies an edge.
    table::Schema connectionSchema;
    table::Key<int> from;
    table::Key<int> to;
    table::Key<int> transform;

    CameraSys makeCameraSys(table::BaseRecord const & record) const {
        return CameraSys(record.get(sysName), record.get(detectorName));
    }

private:

    OldPersistenceHelper() :
        sysSchema(),
        sysName(sysSchema.addField<std::string>("sysName", "Camera coordinate system name", "", 0)),
        detectorName(sysSchema.addField<std::string>("detectorName",
                                                     "Camera coordinate system detector name", "", 0)),
        id(sysSchema.addField<int>("id", "AST ID of the Frame for the CameraSys", "")),
        connectionSchema(),
        from(connectionSchema.addField<int>("from", "AST ID of the Frame this transform maps from.", "")),
        to(connectionSchema.addField<int>("to", "AST ID of the Frame this transform maps to.", "")),
        transform(connectionSchema.addField<int>("transform", "Archive ID of the transform.", ""))
    {}

    OldPersistenceHelper(OldPersistenceHelper const &) = delete;
    OldPersistenceHelper(OldPersistenceHelper &&) = delete;

    OldPersistenceHelper & operator=(OldPersistenceHelper const &) = delete;
    OldPersistenceHelper & operator=(OldPersistenceHelper &&) = delete;

};


}  // namespace


std::string TransformMap::getPersistenceName() const {
    return "TransformMap";
}

std::string TransformMap::getPythonModule() const {
    return "lsst.afw.cameraGeom";
}

void TransformMap::write(OutputArchiveHandle& handle) const {
    auto const & keys = PersistenceHelper::get();

    auto cat = handle.makeCatalog(keys.schema);
    for (auto const & connection : _connections) {
        auto record = cat.addNew();
        record->set(keys.fromSysName, connection.fromSys.getSysName());
        record->set(keys.fromSysDetectorName, connection.fromSys.getDetectorName());
        record->set(keys.toSysName, connection.toSys.getSysName());
        record->set(keys.toSysDetectorName, connection.toSys.getDetectorName());
        record->set(keys.transform, handle.put(connection.transform));
    }
    handle.saveCatalog(cat);
}

class TransformMap::Factory : public table::io::PersistableFactory {
public:

    Factory() : PersistableFactory("TransformMap") {}

    std::shared_ptr<Persistable> readOld(InputArchive const& archive,
                                         CatalogVector const& catalogs) const {
        auto const & keys = OldPersistenceHelper::get();

        LSST_ARCHIVE_ASSERT(catalogs.size() == 2u);
        auto const & sysCat = catalogs[0];
        auto const & connectionCat = catalogs[1];
        LSST_ARCHIVE_ASSERT(sysCat.getSchema() == keys.sysSchema);
        LSST_ARCHIVE_ASSERT(connectionCat.getSchema() == keys.connectionSchema);
        LSST_ARCHIVE_ASSERT(sysCat.size() == connectionCat.size() + 1);
        LSST_ARCHIVE_ASSERT(sysCat.isSorted(keys.id));

        std::unordered_map<int, CameraSys> sysById;
        for (auto const & sysRecord : sysCat) {
            auto sys = keys.makeCameraSys(sysRecord);
            sysById.emplace(sysRecord.get(keys.id), sys);
        }

        auto const referenceSysIter = sysById.find(1);
        LSST_ARCHIVE_ASSERT(referenceSysIter != sysById.end());
        std::vector<Connection> connections;
        for (auto const & connectionRecord : connectionCat) {
            auto const fromSysIter = sysById.find(connectionRecord.get(keys.from));
            LSST_ARCHIVE_ASSERT(fromSysIter != sysById.end());
            auto const toSysIter = sysById.find(connectionRecord.get(keys.to));
            LSST_ARCHIVE_ASSERT(toSysIter != sysById.end());
            auto const transform = archive.get<geom::TransformPoint2ToPoint2>(
                connectionRecord.get(keys.transform)
            );

            connections.push_back(Connection{transform, fromSysIter->second, toSysIter->second});
        }

        connections = standardizeConnections(referenceSysIter->second, std::move(connections));
        return std::shared_ptr<TransformMap>(new TransformMap(std::move(connections)));
    }

    std::shared_ptr<Persistable> read(InputArchive const& archive,
                                      CatalogVector const& catalogs) const override {
        if (catalogs.size() == 2u) {
            return readOld(archive, catalogs);
        }

        auto const & keys = PersistenceHelper::get();

        LSST_ARCHIVE_ASSERT(catalogs.size() == 1u);
        auto const & cat = catalogs[0];
        LSST_ARCHIVE_ASSERT(cat.getSchema() == keys.schema);

        std::vector<Connection> connections;
        for (auto const & record : cat) {
            CameraSys const fromSys(record.get(keys.fromSysName), record.get(keys.fromSysDetectorName));
            CameraSys const toSys(record.get(keys.toSysName), record.get(keys.toSysDetectorName));
            auto const transform = archive.get<geom::TransformPoint2ToPoint2>(record.get(keys.transform));
            connections.push_back(Connection{transform, fromSys, toSys});
        }

        // Deserialized connections should already be standardized, but be
        // defensive anyway.
        auto const referenceSys = getReferenceSys(connections);
        connections = standardizeConnections(referenceSys, std::move(connections));
        return std::shared_ptr<TransformMap>(new TransformMap(std::move(connections)));
    }

    static Factory const registration;

};

TransformMap::Factory const TransformMap::Factory::registration;

}  // namespace cameraGeom

namespace table {
namespace io {

template class PersistableFacade<cameraGeom::TransformMap>;

}  // namespace io
}  // namespace table

}  // namespace afw
}  // namespace lsst
