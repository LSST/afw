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

#ifndef LSST_AFW_IMAGE_DETAIL_MASKDICT_H
#define LSST_AFW_IMAGE_DETAIL_MASKDICT_H

#include <memory>
#include <map>
#include <iostream>

namespace lsst {
namespace afw {
namespace image {
namespace detail {

struct MaskDictImpl;

using MaskPlaneDict = std::map<std::string, int>;
using MaskPlaneDocDict = std::map<std::string, std::string>;

/*
 * MaskDict is the internal copy-on-write object that relates Mask's string plane names
 * to bit IDs.
 *
 * TODO: the below text should be rewritten for the new interface.
 *
 * The Mask public API only uses MaskPlaneDict, which is just a typedef
 * to std::map.  A MaskDict holds this MaskPlaneDict, limiting non-const
 * access to just the add() and erase() methods in order to maintain
 * a hash of the full dictionary for fast comparison between MaskDicts.
 *
 * In order to maximize consistency of mask plane definitions across different
 * Mask instances, MaskDict also maintains a global list of all active
 * MaskDict instances.  When a plane is added to Mask, it is typically
 * automatically added to *all* active Mask instances that do not already use
 * that bit (via MaskDict::addMaskPlane) as well as a default MaskDict that is
 * used when constructing future Mask instances.  In contrast, when a plane is
 * removed, it typically only affects that Mask, and if that Mask is currently
 * using the default MaskDict, the default is redefined to a copy prior to
 * the plane's removal (via detachDefault).
 *
 * To maintain that global state, all MaskDicts must be held by shared_ptr,
 * and hence MaskDict's constructors are private, and static or instance
 * methods that return shared_ptr are provided to replace them.
 *
 * Mask equality (via the hash) is determined by the bit fields, field names,
 * and field docs; mask docstrings define the meaning of the mask, so two
 * masks with the same named fields but different docs should not be assumed
 * to be the same!
 *
 * MaskDict is an implementation detail (albeit and important one) and hence
 * its "documentation" is intentionally in the form of regular comments
 * rather than Doxygen-parsed blocks.  It is also not available from Python.
 *
 * With the exception of addMaskPlane, all MaskDict methods provide strong
 * exception safety.
 */
class MaskDict final {
public:
    using value_type = MaskPlaneDict::value_type;
    using const_iterator = MaskPlaneDict::const_iterator;

    explicit MaskDict(int maxPlanes, bool _default = true);
    explicit MaskDict(int maxPlanes, MaskPlaneDict const &dict, MaskPlaneDocDict const &docs);

    int add(std::string name, std::string doc);
    void remove(std::string name);
    void conformTo(MaskDict const &other);

    /**
     * Add a mask plane with the given name and doc.
     *
     * * If the name doesn't exit, add it to this, and reserve the bit for this name in the global state,
     * and return the new bit id and this.
     * * If the name already exists and has the same doc, do nothing, return the existing bit id and this.
     * * If the name already exists and `doc` is empty, do nothing, return the existing bit id and this.
     * * If the name already exists and has no doc, replace the doc with the new doc, and return the existing
     * bit id and this.
     * * If the name already exists and has a different doc, modify a copy of this and return it along with
     * the new bit.
     *
     * @param name Mask plane name to add.
     * @param doc Docstring for new mask plane.
     * @param maxPlanes Maximum number of allowed planes (defined by the pixel type of the Mask).
     *
     * @return Tuple containing the new bit id added and a pointer to the ????
     */
    // std::tuple<int, std::shared_ptr<MaskDict>> withNewMaskPlane(std::string name, std::string doc,
    //                                                             int maxPlanes);

    /// Return a mask dict without the given mask plane (may or may not be a copy).
    // std::shared_ptr<MaskDict> withRemovedMaskPlane(std::string name);

    /// Return the default MaskDict if `dict` is an empty map.
    // static MaskDict getDefaultIfEmpty(MaskDict const dict);

    // Return the default MaskDict to be used for new Mask instances.
    // static std::shared_ptr<MaskDict> getDefault();

    // Set the default MaskDict.
    // static void setDefault(std::shared_ptr<MaskDict> dict);

    /// Remove all defined ids and docs from the default map and canonical list.
    // static void clearDefaultPlanes(bool clearCanonical = false);

    /// Reset the default MaskDict to the normal initial list, and set the canonical planes to match.
    // static void restoreDefaultMaskDict();

    // OLD STUFF

    /*
     * Add the given mask plane to all active MaskDicts for which there is
     * no conflict, including the default MaskDict.
     *
     * MaskDicts that already have a plane with the same name or bit ID
     * are not affected.
     *
     * Provides basic exception safety: mask planes may be added to some
     * MaskDict instances even if an exception is raised while adding
     * the mask plane to later ones (though the only exception that could
     * be thrown in practice is std::bad_alloc).
     */
    // static void addAllMasksPlane(std::string const &name, int bitId, std::string const &doc);

    // ??? - I think these are all good to have?
    MaskDict &operator=(MaskDict const &) = default;
    MaskDict &operator=(MaskDict &&) = default;
    MaskDict(MaskDict const &) = default;
    MaskDict(MaskDict &&) = default;

    ~MaskDict() noexcept = default;

    /// Return a deep copy of the MaskDict.
    MaskDict clone() const;

    /*
     * Return an integer bit ID that is not currently used in this MaskDict.
     *
     * Always succeeds in returning a new plane (except in the extraordinarily
     * unlikely case that int overflows), but is is the responsibility of the
     * caller to check that the Mask pixel size has enough bits for it.
     */
    // int getUnusedPlane() const;

    /*
     * Return the bit ID associated with the given mask plane name.
     *
     * Returns -1 if no such plane is found.
     */
    int getPlaneId(std::string const &name) const;
    std::string getPlaneDoc(std::string const &name) const;

    /// Return a formatted string showing the mask plane bits, names, and docs.
    std::string print() const;

    // Fast comparison of MaskDicts, using the hash (and assuming there are
    // no unlucky collisions).
    bool operator==(MaskDict const &rhs) const;
    bool operator!=(MaskDict const &rhs) const { return !(*this == rhs); }

    // Iterators over MaskDict items (yields std::pair<std::string, int>).
    // const_iterator begin() const noexcept { return _dict.begin(); }
    // const_iterator end() const noexcept { return _dict.end(); }

    // // Return an iterator to the item with the given name, or end().
    // const_iterator find(std::string const &name) const { return _dict.find(name); }

    // Return the number of planes in this MaskDict.
    // std::size_t size() const noexcept { return _dict.size(); }

    // Return true if the MaskDict contains no mask planes.
    // bool empty() const noexcept { return _dict.empty(); }

    // Return the internal MaskPlaneDict.
    MaskPlaneDict const &getMaskPlaneDict() const noexcept { return _dict->_dict; }
    // Return the internal MaskPlaneDocDict.
    MaskPlaneDocDict const &getMaskPlaneDocDict() const noexcept { return _dict->_docs; }

    // Add a mask plane to just this MaskDict.
    // If a plane with the given name already exists, it is overridden.
    // Caller is responsible for ensuring that the bit is not in use; if it is,
    // the MaskDict will be in a corrupted state.
    // void add(std::string const &name, int bitId, std::string const &doc);

    // Remove the plane with the given name from just this MaskDict.
    // Does nothing if no such plane exists.
    // void erase(std::string const &name);

    // Remove all planes from this MaskDict.
    // void clear();

private:
    // Internal shared_ptr storage for the maps.
    struct MaskDictImpl {
        MaskPlaneDict _dict;
        MaskPlaneDocDict _docs;
        explicit MaskDictImpl(bool _default);
        explicit MaskDictImpl(MaskPlaneDict const &dict, MaskPlaneDocDict const &docs);

        // Add mask planes that should be present in a default Mask.
        void _addInitialMaskPlanes();
    };

    int _maxPlanes;
    std::shared_ptr<MaskDictImpl> _dict;
};

}  // namespace detail
}  // namespace image
}  // namespace afw
}  // namespace lsst

#endif  // !LSST_AFW_IMAGE_DETAIL_MASKDICT_H
