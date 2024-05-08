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

#ifndef LSST_AFW_CAMERAGEOM_DETECTORCOLLECTION_H
#define LSST_AFW_CAMERAGEOM_DETECTORCOLLECTION_H

#include <map>
#include <string>
#include <memory>

#include "lsst/geom/Box.h"
#include "lsst/afw/table/io/Persistable.h"
#include "lsst/afw/cameraGeom/Detector.h"

namespace lsst {
namespace afw {
namespace cameraGeom {

/**
 * An abstract base class for collections of Detectors and specific subclasses
 * thereof.
 *
 * @tparam T   Element type; either `Detector` or a subclass thereof.
 *
 * This class provides the common interface and implementation for
 * `DetectorCollection` (which holds true `Detector` instances) and
 * `Camera::Builder` (which holds `Detector::InCameraBuilder` instances).  It
 * is not intended to define an interface independent of those classes.
 */
template <typename T>
class DetectorCollectionBase {
public:

    using NameMap = std::unordered_map<std::string, std::shared_ptr<T>>;
    using IdMap = std::map<int, std::shared_ptr<T>>;
    using List = std::vector<std::shared_ptr<T>>;

    virtual ~DetectorCollectionBase() noexcept = 0;

    /// Get a map keyed and ordered by name.
    NameMap const & getNameMap() const noexcept { return _nameDict; }

    /// Get a map keyed and ordered by ID.
    IdMap const & getIdMap() const noexcept { return _idDict; }

    /**
     * Get the number of detectors.  Renamed to `__len__` in Python.
     */
    std::size_t size() const noexcept { return _idDict.size(); }

    /**
     * Determine if the collection contains any detectors.
     */
    bool empty() const noexcept { return _idDict.empty(); }

    /**
     * Implement the [name] operator
     *
     * @param[in] name  detector name
     * @returns pointer to detector entry
     */
    std::shared_ptr<T> operator[](std::string const & name) const;

    /**
     * Implement the [id] operator
     *
     * @param[in] id  detector name
     * @returns pointer to detector entry
     */
    std::shared_ptr<T> operator[](int id) const;

    /**
     * Retrieve a detector by name, or fall back to a default.
     *
     * @param[in] name  detector name
     * @param[in] def  default detector to return.  This defaults to `nullptr`.
     *
     * @returns pointer to detector entry if the entry exists, else return
     *          the default value
     */
    std::shared_ptr<T> get(std::string const & name, std::shared_ptr<T> def=nullptr) const;

    /**
     * Retrieve a detector by ID, or fall back to a default.
     *
     * @param[in] id  detector id
     * @param[in] def  default detector to return.  This defaults to `nullptr`.
     *
     * @returns pointer to detector entry if the entry exists, else return
     *          the default value
     */
    std::shared_ptr<T> get(int id, std::shared_ptr<T> def=nullptr) const;

protected:

    explicit DetectorCollectionBase(List const & detectorList);

    DetectorCollectionBase() noexcept = default;

    DetectorCollectionBase(DetectorCollectionBase const &) = default;
    DetectorCollectionBase(DetectorCollectionBase &&) = default;

    DetectorCollectionBase & operator=(DetectorCollectionBase const &) = default;
    DetectorCollectionBase & operator=(DetectorCollectionBase &&) = default;

    /**
     * Add a detector to the collection.
     *
     * @param[in] detector  New detector to add to the collection.
     *
     * @throw pex::exceptions::RuntimeError  Thrown if the ID and/or name
     *     conflict with those of detectors already in the collection.
     *
     * @exceptsafe  Strong for pex::exceptions::RuntimeError, weak (collection
     *              is made empty) otherwise.
     */
    void add(std::shared_ptr<T> detector);

    void remove(std::string const & name);
    void remove(int id);

private:
    NameMap _nameDict;                //< map keyed on name
    IdMap _idDict;                    //< map keyed on id
};


/**
 * An immutable collection of Detectors that can be accessed by name or ID
 */
class DetectorCollection : public DetectorCollectionBase<Detector const>,
                           public table::io::Persistable {
DECLARE_PERSISTABLE_FACADE(DetectorCollection);
public:
    DetectorCollection(List const & list);

    virtual ~DetectorCollection() noexcept;

    /// Return a focal plane bounding box that encompasses all detectors
    lsst::geom::Box2D const & getFpBBox() const noexcept { return _fpBBox; }

    // DetectorCollection is immutable, so it cannot be moveable.  It is also
    // always held by shared_ptr, so there is no good reason to copy it.
    DetectorCollection(DetectorCollection const &) = delete;
    DetectorCollection(DetectorCollection &&) = delete;

    // DetectorCollection is immutable, so it cannot be assignable.
    DetectorCollection & operator=(DetectorCollection const &) = delete;
    DetectorCollection & operator=(DetectorCollection &&) = delete;

    /// DetectorCollections are always persistable.
    bool isPersistable() const noexcept override {
        return true;
    }

protected:

    DetectorCollection(table::io::InputArchive const & archive, table::io::CatalogVector const & catalogs);

    std::string getPersistenceName() const override;

    std::string getPythonModule() const override;

    void write(OutputArchiveHandle& handle) const override;

private:

    class Factory;

    lsst::geom::Box2D _fpBBox;        //< bounding box of collection
};

} // namespace cameraGeom
} // namespace afw
} // namespace lsst


#endif // LSST_AFW_CAMERAGEOM_DETECTORCOLLECTION_H
