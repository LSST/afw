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

#include <map>
#include <optional>
#include <regex>
#include <set>

#include "lsst/log/Log.h"

#include "lsst/afw/image/PhotoCalib.h"
#include "lsst/afw/geom/polygon/Polygon.h"
#include "lsst/afw/geom/SkyWcs.h"
#include "lsst/afw/cameraGeom/Detector.h"
#include "lsst/afw/image/ApCorrMap.h"
#include "lsst/afw/detection/Psf.h"
#include "lsst/afw/image/TransmissionCurve.h"
#include "lsst/afw/image/ExposureFitsReader.h"

namespace lsst {
namespace afw {
namespace image {

namespace {

LOG_LOGGER _log = LOG_GET("lsst.afw.image.fits.ExposureFitsReader");

template <typename T, std::size_t N>
bool _contains(std::array<T, N> const& array, T const& value) {
    for (T const& element : array) {
        if (element == value) {
            return true;
        }
    }
    return false;
}

// Map from compatibility "afw name" to correct filter label
std::map<std::string, FilterLabel> const _AFW_NAMES = {
        std::make_pair("r2", FilterLabel::fromBandPhysical("r", "HSC-R2")),
        std::make_pair("i2", FilterLabel::fromBandPhysical("i", "HSC-I2")),
        std::make_pair("SOLID", FilterLabel::fromPhysical("solid plate 0.0 0.0")),
};

/**
 * Determine heuristically whether a filter name represents a band or a physical filter.
 *
 * @param name The name to test.
 */
bool _isBand(std::string const& name) {
    static std::set<std::string> const BANDS = {"u", "g", "r", "i", "z", "y", "SH", "PH", "VR", "white"};
    // Standard band
    if (BANDS.count(name) > 0) {
        return true;
    }
    // Looks like a narrow-band band
    if (std::regex_match(name, std::regex("N\\d+"))) {
        return true;
    }
    // Looks like an intermediate-band band; exclude "I2"
    if (std::regex_match(name, std::regex("I\\d{2,}"))) {
        return true;
    }
    return false;
}

}  // namespace

/**
 * Convert an old-style filter name to a FilterLabel without external information.
 *
 * Guaranteed to not call any code related to Filter or FilterDefinition.
 *
 * @param name A name for the filter.
 * @returns A FilterLabel containing that name.
 */
std::shared_ptr<FilterLabel> makeFilterLabelDirect(std::string const& name) {
    static std::string const DEFAULT = "_unknown_";
    // Avoid turning dummy filters into real FilterLabels.
    if (name == DEFAULT) {
        return nullptr;
    }

    // FilterLabel::from* returns a statically allocated object, so only way
    // to get it into shared_ptr is to copy it.
    if (_isBand(name)) {
        return std::make_shared<FilterLabel>(FilterLabel::fromBand(name));
    } else {
        return std::make_shared<FilterLabel>(FilterLabel::fromPhysical(name));
    }
}

/**
 * Convert an old-style single Filter name to a FilterLabel, using available information.
 *
 * @param name The name persisted in a FITS file. May be any of a Filter's many names.
 * @returns The closest equivalent FilterLabel, given available information.
 */
std::shared_ptr<FilterLabel> makeFilterLabel(std::string const& name) {
    if (_AFW_NAMES.count(name) > 0) {
        return std::make_shared<FilterLabel>(_AFW_NAMES.at(name));
    }
    // else name is either a band, a physical filter, or a deprecated alias

    // Unknown filter, no extra info to be gained
    return makeFilterLabelDirect(name);
}

class ExposureFitsReader::MetadataReader {
public:
    MetadataReader(std::shared_ptr<daf::base::PropertyList> primaryMetadata,
                   std::shared_ptr<daf::base::PropertyList> imageMetadata, lsst::geom::Point2I const& xy0) {
        auto versionName = ExposureInfo::getFitsSerializationVersionName();
        if (primaryMetadata->exists(versionName)) {
            version = primaryMetadata->getAsInt(versionName);
            primaryMetadata->remove(versionName);
        } else {
            version = 0;  // unversioned files are implicitly version 0
        }
        if (version > ExposureInfo::getFitsSerializationVersion()) {
            throw LSST_EXCEPT(pex::exceptions::TypeError,
                              str(boost::format("Cannot read Exposure FITS version > %i") %
                                  ExposureInfo::getFitsSerializationVersion()));
        }

        // Try to read WCS from image metadata, and if found, strip the keywords used
        try {
            wcs = afw::geom::makeSkyWcs(*imageMetadata, true);
        } catch (lsst::pex::exceptions::TypeError const&) {
            LOGLS_DEBUG(_log, "No WCS found in FITS metadata");
        }
        if (wcs && any(xy0.ne(lsst::geom::Point2I(0, 0)))) {
            wcs = wcs->copyAtShiftedPixelOrigin(lsst::geom::Extent2D(xy0));
        }

        // Strip LTV1, LTV2 from imageMetadata, because we don't use it internally
        imageMetadata->remove("LTV1");
        imageMetadata->remove("LTV2");

        if (!imageMetadata->exists("INHERIT")) {
            // New-style exposures put everything but the Wcs in the primary HDU, use
            // INHERIT keyword in the others.  For backwards compatibility, if we don't
            // find the INHERIT keyword, we ignore the primary HDU metadata and expect
            // everything to be in the image HDU metadata.  Note that we can't merge them,
            // because they're probably duplicates.
            metadata = imageMetadata;
        } else {
            metadata = primaryMetadata;
        }

        // Earlier versions persisted Filter as header keyword, version 2 persists FilterLabel as a Storable
        if (version < 2) {
            std::string key = "FILTER";
            if (metadata->exists(key)) {
                // Original Filter code depended on Boost for string trimming.
                // DIY to avoid making this module depend on Boost.
                std::string name = metadata->getAsString(key);
                size_t end = name.find_last_not_of(' ');
                filterLabel = makeFilterLabel(name.substr(0, end + 1));
            }
        }

        // EXPID keyword used in all versions, but was VisitInfo's responsibility before VisitInfo v3.
        if (metadata->exists("EXPID")) {
            exposureId = metadata->getAsInt64("EXPID");
        }

        visitInfo = std::make_shared<VisitInfo>(*metadata);
        detail::stripVisitInfoKeywords(*metadata);

        // This keyword is no longer handled by VisitInfo version >= 3.
        metadata->remove("EXPID");

        // Version 0 persisted Calib FLUXMAG0 in the metadata, >=1 persisted PhotoCalib as a binary table.
        if (version == 0) {
            photoCalib = makePhotoCalibFromMetadata(*metadata, true);
        }

        // Strip MJD-OBS and DATE-OBS from metadata; those may be read by
        // either SkyWcs or VisitInfo or both, so neither can strip them.
        metadata->remove("MJD-OBS");
        metadata->remove("DATE-OBS");

        // Strip DETSER, DETNAME; these are added when writing an Exposure
        // with a Detector
        metadata->remove("DETNAME");
        metadata->remove("DETSER");
    }

    int version;
    std::optional<table::RecordId> exposureId;
    std::shared_ptr<daf::base::PropertyList> metadata;
    std::shared_ptr<FilterLabel> filterLabel;
    std::shared_ptr<afw::geom::SkyWcs> wcs;
    std::shared_ptr<PhotoCalib> photoCalib;
    std::shared_ptr<VisitInfo> visitInfo;
};

class ExposureFitsReader::ArchiveReader {
public:
    enum Component {
        PSF = 0,
        WCS,
        COADD_INPUTS,
        AP_CORR_MAP,
        VALID_POLYGON,
        TRANSMISSION_CURVE,
        DETECTOR,
        PHOTOCALIB,
        N_ARCHIVE_COMPONENTS
    };

    explicit ArchiveReader(daf::base::PropertyList& metadata) {
        auto popInt = [&metadata](std::string const& name) {
            // The default of zero will cause archive.get to return a
            // null/empty pointer, just as if a null/empty pointer was
            // originally written to the archive.
            int r = 0;
            if (metadata.exists(name)) {
                r = metadata.get<int>(name);
                // We remove metadata entries to maintaing our practice
                // of stripped metadata entries that have been used to
                // construct more structured components.
                metadata.remove(name);
            }
            return r;
        };
        _hdu = popInt("AR_HDU");
        if (_hdu == 0) {
            _state = ArchiveState::MISSING;
        } else {
            --_hdu;  // Switch from FITS 1-indexed convention to LSST 0-indexed convention.
            _state = ArchiveState::PRESENT;
        }
        // Read in traditional components using old-style IDs, for backwards compatibility
        _ids[PSF] = popInt("PSF_ID");
        _ids[WCS] = popInt("SKYWCS_ID");
        _ids[COADD_INPUTS] = popInt("COADD_INPUTS_ID");
        _ids[AP_CORR_MAP] = popInt("AP_CORR_MAP_ID");
        _ids[VALID_POLYGON] = popInt("VALID_POLYGON_ID");
        _ids[TRANSMISSION_CURVE] = popInt("TRANSMISSION_CURVE_ID");
        _ids[DETECTOR] = popInt("DETECTOR_ID");
        _ids[PHOTOCALIB] = popInt("PHOTOCALIB_ID");

        // "Extra" components use a different keyword convention to avoid collisions with non-persistence IDs
        std::vector<std::string> toStrip;
        for (std::string const& headerKey : metadata) {
            static std::string const PREFIX = "ARCHIVE_ID_";
            if (headerKey.substr(0, PREFIX.size()) == PREFIX) {
                std::string componentName = headerKey.substr(PREFIX.size());
                int archiveId = metadata.get<int>(headerKey);
                _genericIds.emplace(componentName, archiveId);
                if (!_contains(_ids, archiveId)) {
                    _extraIds.emplace(componentName);
                }
                toStrip.push_back(headerKey);
                toStrip.push_back(componentName + "_ID");  // strip corresponding old-style ID, if it exists
            }
        }
        for (std::string const& key : toStrip) {
            metadata.remove(key);
        }
    }

    /**
     * Read a known component, if available.
     *
     * @param fitsFile The file from which to read the component. Must match
     *                 the metadata used to construct this object.
     * @param c The component to read. Must be convertible to ``T``.
     *
     * @return The desired component, or ``nullptr`` if the file could not be read.
     */
    template <typename T>
    std::shared_ptr<T> readComponent(afw::fits::Fits* fitsFile, Component c) {
        if (!_ensureLoaded(fitsFile)) {
            return nullptr;
        }
        return _archive.get<T>(_ids[c]);
    }

    /**
     * Read an arbitrary component, if available.
     *
     * @param fitsFile The file from which to read the component. Must match
     *                 the metadata used to construct this object.
     * @param c The archive ID of the component to read.
     *
     * @return The desired component, or ``nullptr`` if the file could not be read.
     *
     * @throws pex::exceptions::NotFoundError Thrown if the component is
     *     registered in the file metadata but could not be found.
     *
     * @note When accessing from python, components with derived subclasses, such
     *       as ``TransmissionCurve``, are not properly type converted and thus
     *       the specialized reader must be used instead of ``readComponent``.
     */
    // This method takes a string instead of a strongly typed Key because
    // readExtraComponents() gets its keys from the FITS metadata.
    // Using a Key would make the calling code more complicated.
    template <typename T>
    std::shared_ptr<T> readComponent(afw::fits::Fits* fitsFile, std::string c) {
        if (!_ensureLoaded(fitsFile)) {
            return nullptr;
        }

        if (_genericIds.count(c) > 0) {
            int archiveId = _genericIds.at(c);
            return _archive.get<T>(archiveId);
        } else {
            return nullptr;
        }
    }

    /**
     * Read the components that are stored using arbitrary-component support.
     *
     * @param fitsFile The file from which to read the components. Must match
     *                 the metadata used to construct this object.
     *
     * @return a map from string IDs to components, or an empty map if the
     *         file could not be read.
     */
    std::map<std::string, std::shared_ptr<table::io::Persistable>> readExtraComponents(
            afw::fits::Fits* fitsFile) {
        std::map<std::string, std::shared_ptr<table::io::Persistable>> result;

        if (!_ensureLoaded(fitsFile)) {
            return result;
        }

        // Not safe to call getAll if a component cannot be unpersisted
        // Instead, look for the archives registered in the metadata
        for (std::string const& componentName : _extraIds) {
            try {
                result.emplace(componentName, readComponent<table::io::Persistable>(fitsFile, componentName));
            } catch (pex::exceptions::NotFoundError const& err) {
                LOGLS_WARN(_log,
                           "Could not read component " << componentName << "; skipping: " << err.what());
            }
        }
        return result;
    }

private:
    bool _ensureLoaded(afw::fits::Fits* fitsFile) {
        if (_state == ArchiveState::MISSING) {
            return false;
        }
        if (_state == ArchiveState::PRESENT) {
            afw::fits::HduMoveGuard guard(*fitsFile, _hdu);
            _archive = table::io::InputArchive::readFits(*fitsFile);
            _state = ArchiveState::LOADED;
        }
        assert(_state == ArchiveState::LOADED);  // constructor body should guarantee it's not UNKNOWN
        return true;
    }

    enum class ArchiveState { UNKNOWN, MISSING, PRESENT, LOADED };

    int _hdu = 0;
    ArchiveState _state = ArchiveState::UNKNOWN;
    table::io::InputArchive _archive;
    std::array<int, N_ARCHIVE_COMPONENTS> _ids = {0};
    std::map<std::string, int> _genericIds;
    std::set<std::string> _extraIds;  // _genericIds not included in _ids
};

ExposureFitsReader::ExposureFitsReader(std::string const& fileName) : _maskedImageReader(fileName) {}

ExposureFitsReader::ExposureFitsReader(fits::MemFileManager& manager) : _maskedImageReader(manager) {}

ExposureFitsReader::ExposureFitsReader(fits::Fits* fitsFile) : _maskedImageReader(fitsFile) {}

ExposureFitsReader::~ExposureFitsReader() noexcept = default;

lsst::geom::Box2I ExposureFitsReader::readBBox(ImageOrigin origin) {
    return _maskedImageReader.readBBox(origin);
}

lsst::geom::Point2I ExposureFitsReader::readXY0(lsst::geom::Box2I const& bbox, ImageOrigin origin) {
    return _maskedImageReader.readXY0(bbox, origin);
}

int ExposureFitsReader::readSerializationVersion() {
    _ensureReaders();
    return _metadataReader->version;
}

std::string ExposureFitsReader::readImageDType() const { return _maskedImageReader.readImageDType(); }

std::string ExposureFitsReader::readMaskDType() const { return _maskedImageReader.readMaskDType(); }

std::string ExposureFitsReader::readVarianceDType() const { return _maskedImageReader.readVarianceDType(); }

std::shared_ptr<daf::base::PropertyList> ExposureFitsReader::readMetadata() {
    _ensureReaders();
    return _metadataReader->metadata;
}

std::optional<table::RecordId> ExposureFitsReader::readExposureId() {
    _ensureReaders();
    return _metadataReader->exposureId;
}

std::shared_ptr<afw::geom::SkyWcs> ExposureFitsReader::readWcs() {
    _ensureReaders();
    auto r = _archiveReader->readComponent<afw::geom::SkyWcs>(_getFitsFile(), ArchiveReader::WCS);
    if (!r) {
        r = _metadataReader->wcs;
    }
    return r;
}

std::shared_ptr<FilterLabel> ExposureFitsReader::readFilter() {
    _ensureReaders();
    if (_metadataReader->version < 2) {
        return _metadataReader->filterLabel;
    } else {
        return _archiveReader->readComponent<FilterLabel>(_getFitsFile(), ExposureInfo::KEY_FILTER.getId());
    }
}

std::shared_ptr<PhotoCalib> ExposureFitsReader::readPhotoCalib() {
    _ensureReaders();
    if (_metadataReader->version == 0) {
        return _metadataReader->photoCalib;
    } else {
        return _archiveReader->readComponent<image::PhotoCalib>(_getFitsFile(), ArchiveReader::PHOTOCALIB);
    }
}

std::shared_ptr<detection::Psf> ExposureFitsReader::readPsf() {
    _ensureReaders();
    return _archiveReader->readComponent<detection::Psf>(_getFitsFile(), ArchiveReader::PSF);
}

std::shared_ptr<afw::geom::polygon::Polygon> ExposureFitsReader::readValidPolygon() {
    _ensureReaders();
    return _archiveReader->readComponent<afw::geom::polygon::Polygon>(_getFitsFile(),
                                                                      ArchiveReader::VALID_POLYGON);
}

std::shared_ptr<ApCorrMap> ExposureFitsReader::readApCorrMap() {
    _ensureReaders();
    return _archiveReader->readComponent<ApCorrMap>(_getFitsFile(), ArchiveReader::AP_CORR_MAP);
}

std::shared_ptr<CoaddInputs> ExposureFitsReader::readCoaddInputs() {
    _ensureReaders();
    return _archiveReader->readComponent<CoaddInputs>(_getFitsFile(), ArchiveReader::COADD_INPUTS);
}

std::shared_ptr<VisitInfo> ExposureFitsReader::readVisitInfo() {
    _ensureReaders();
    return _metadataReader->visitInfo;
}

std::shared_ptr<TransmissionCurve> ExposureFitsReader::readTransmissionCurve() {
    _ensureReaders();
    return _archiveReader->readComponent<TransmissionCurve>(_getFitsFile(),
                                                            ArchiveReader::TRANSMISSION_CURVE);
}

std::shared_ptr<cameraGeom::Detector> ExposureFitsReader::readDetector() {
    _ensureReaders();
    return _archiveReader->readComponent<cameraGeom::Detector>(_getFitsFile(), ArchiveReader::DETECTOR);
}

std::shared_ptr<typehandling::Storable> ExposureFitsReader::readComponent(std::string const& componentName) {
    _ensureReaders();
    return _archiveReader->readComponent<typehandling::Storable>(_getFitsFile(), componentName);
}

std::map<std::string, std::shared_ptr<table::io::Persistable>> ExposureFitsReader::readExtraComponents() {
    _ensureReaders();
    return _archiveReader->readExtraComponents(_getFitsFile());
}

std::shared_ptr<ExposureInfo> ExposureFitsReader::readExposureInfo() {
    auto result = std::make_shared<ExposureInfo>();
    result->setMetadata(readMetadata());
    result->setPhotoCalib(readPhotoCalib());
    result->setVisitInfo(readVisitInfo());
    // Override ID set in visitInfo, if necessary
    std::optional<table::RecordId> exposureId = readExposureId();
    if (exposureId) {
        result->setId(*exposureId);
    }
    // When reading an ExposureInfo (as opposed to reading individual
    // components), we warn and try to proceed when a component is present
    // but can't be read due its serialization factory not being set up
    // (that's what throws the NotFoundErrors caught below).
    try {
        result->setPsf(readPsf());
    } catch (pex::exceptions::NotFoundError& err) {
        LOGLS_WARN(_log, "Could not read PSF; setting to null: " << err.what());
    }
    try {
        result->setCoaddInputs(readCoaddInputs());
    } catch (pex::exceptions::NotFoundError& err) {
        LOGLS_WARN(_log, "Could not read CoaddInputs; setting to null: " << err.what());
    }
    try {
        result->setApCorrMap(readApCorrMap());
    } catch (pex::exceptions::NotFoundError& err) {
        LOGLS_WARN(_log, "Could not read ApCorrMap; setting to null: " << err.what());
    }
    try {
        result->setValidPolygon(readValidPolygon());
    } catch (pex::exceptions::NotFoundError& err) {
        LOGLS_WARN(_log, "Could not read ValidPolygon; setting to null: " << err.what());
    }
    try {
        result->setTransmissionCurve(readTransmissionCurve());
    } catch (pex::exceptions::NotFoundError& err) {
        LOGLS_WARN(_log, "Could not read TransmissionCurve; setting to null: " << err.what());
    }
    try {
        result->setDetector(readDetector());
    } catch (pex::exceptions::NotFoundError& err) {
        LOGLS_WARN(_log, "Could not read Detector; setting to null: " << err.what());
    }
    // In the case of WCS, we fall back to the metadata WCS if the one from
    // the archive can't be read.
    _ensureReaders();
    result->setWcs(_metadataReader->wcs);
    try {
        auto wcs = _archiveReader->readComponent<afw::geom::SkyWcs>(_getFitsFile(), ArchiveReader::WCS);
        if (!wcs) {
            LOGLS_DEBUG(_log, "No WCS found in binary table");
        } else {
            result->setWcs(wcs);
        }
    } catch (pex::exceptions::NotFoundError& err) {
        auto msg = str(boost::format("Could not read WCS extension; setting to null: %s") % err.what());
        if (result->hasWcs()) {
            msg += " ; using WCS from FITS header";
        }
        LOGLS_WARN(_log, msg);
    }
    for (const auto& keyValue : readExtraComponents()) {
        using StorablePtr = std::shared_ptr<typehandling::Storable const>;
        std::string key = keyValue.first;
        StorablePtr object = std::dynamic_pointer_cast<StorablePtr::element_type>(keyValue.second);

        if (object.use_count() > 0) {  // Failed cast guarantees empty pointer, but not a null one
            result->setComponent(typehandling::makeKey<StorablePtr>(key), object);
        } else {
            LOGLS_WARN(_log, "Data corruption: generic component " << key << " is not a Storable; skipping.");
        }
    }
    // Convert old-style Filter to new-style FilterLabel
    // In newer versions this is handled by readExtraComponents()
    if (_metadataReader->version < 2 && !result->hasFilter()) {
        result->setFilter(readFilter());
    }
    return result;
}  // namespace image

template <typename ImagePixelT>
Image<ImagePixelT> ExposureFitsReader::readImage(lsst::geom::Box2I const& bbox, ImageOrigin origin,
                                                 bool allowUnsafe) {
    return _maskedImageReader.readImage<ImagePixelT>(bbox, origin, allowUnsafe);
}

template <typename ImagePixelT>
ndarray::Array<ImagePixelT, 2, 2> ExposureFitsReader::readImageArray(lsst::geom::Box2I const& bbox,
                                                                     ImageOrigin origin, bool allowUnsafe) {
    return _maskedImageReader.readImageArray<ImagePixelT>(bbox, origin, allowUnsafe);
}

template <typename MaskPixelT>
Mask<MaskPixelT> ExposureFitsReader::readMask(lsst::geom::Box2I const& bbox, ImageOrigin origin,
                                              bool conformMasks, bool allowUnsafe) {
    return _maskedImageReader.readMask<MaskPixelT>(bbox, origin, conformMasks, allowUnsafe);
}

template <typename MaskPixelT>
ndarray::Array<MaskPixelT, 2, 2> ExposureFitsReader::readMaskArray(lsst::geom::Box2I const& bbox,
                                                                   ImageOrigin origin, bool allowUnsafe) {
    return _maskedImageReader.readMaskArray<MaskPixelT>(bbox, origin, allowUnsafe);
}

template <typename VariancePixelT>
Image<VariancePixelT> ExposureFitsReader::readVariance(lsst::geom::Box2I const& bbox, ImageOrigin origin,
                                                       bool allowUnsafe) {
    return _maskedImageReader.readVariance<VariancePixelT>(bbox, origin, allowUnsafe);
}

template <typename VariancePixelT>
ndarray::Array<VariancePixelT, 2, 2> ExposureFitsReader::readVarianceArray(lsst::geom::Box2I const& bbox,
                                                                           ImageOrigin origin,
                                                                           bool allowUnsafe) {
    return _maskedImageReader.readVarianceArray<VariancePixelT>(bbox, origin, allowUnsafe);
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> ExposureFitsReader::readMaskedImage(
        lsst::geom::Box2I const& bbox, ImageOrigin origin, bool conformMasks, bool allowUnsafe) {
    return _maskedImageReader.read<ImagePixelT, MaskPixelT, VariancePixelT>(bbox, origin, conformMasks,
                                                                            /* needAllHdus= */ false,
                                                                            allowUnsafe);
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
Exposure<ImagePixelT, MaskPixelT, VariancePixelT> ExposureFitsReader::read(lsst::geom::Box2I const& bbox,
                                                                           ImageOrigin origin,
                                                                           bool conformMasks,
                                                                           bool allowUnsafe) {
    auto mi =
            readMaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>(bbox, origin, conformMasks, allowUnsafe);
    return Exposure<ImagePixelT, MaskPixelT, VariancePixelT>(mi, readExposureInfo());
}

void ExposureFitsReader::_ensureReaders() {
    if (!_metadataReader) {
        auto metadataReader = std::make_unique<MetadataReader>(_maskedImageReader.readPrimaryMetadata(),
                                                               _maskedImageReader.readImageMetadata(),
                                                               _maskedImageReader.readXY0());
        _archiveReader = std::make_unique<ArchiveReader>(*metadataReader->metadata);
        _metadataReader = std::move(metadataReader);  // deferred for exception safety
    }
    assert(_archiveReader);  // should always be initialized with _metadataReader.
}

#define INSTANTIATE(ImagePixelT)                                                                            \
    template Exposure<ImagePixelT, MaskPixel, VariancePixel> ExposureFitsReader::read(                      \
            lsst::geom::Box2I const&, ImageOrigin, bool, bool);                                             \
    template Image<ImagePixelT> ExposureFitsReader::readImage(lsst::geom::Box2I const&, ImageOrigin, bool); \
    template ndarray::Array<ImagePixelT, 2, 2> ExposureFitsReader::readImageArray(lsst::geom::Box2I const&, \
                                                                                  ImageOrigin, bool);       \
    template MaskedImage<ImagePixelT, MaskPixel, VariancePixel> ExposureFitsReader::readMaskedImage(        \
            lsst::geom::Box2I const&, ImageOrigin, bool, bool)

INSTANTIATE(std::uint16_t);
INSTANTIATE(int);
INSTANTIATE(float);
INSTANTIATE(double);
INSTANTIATE(std::uint64_t);

template Mask<MaskPixel> ExposureFitsReader::readMask(lsst::geom::Box2I const&, ImageOrigin, bool, bool);
template ndarray::Array<MaskPixel, 2, 2> ExposureFitsReader::readMaskArray(lsst::geom::Box2I const&,
                                                                           ImageOrigin, bool);

template Image<VariancePixel> ExposureFitsReader::readVariance(lsst::geom::Box2I const&, ImageOrigin, bool);
template ndarray::Array<VariancePixel, 2, 2> ExposureFitsReader::readVarianceArray(lsst::geom::Box2I const&,
                                                                                   ImageOrigin, bool);

}  // namespace image
}  // namespace afw
}  // namespace lsst
