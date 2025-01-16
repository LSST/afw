/*
 * LSST Data Management System
 * Copyright 2017 LSST Corporation.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

#include <cmath>
#include <iostream>
#include <iomanip>

#include "lsst/geom/Point.h"
#include "lsst/afw/image/PhotoCalib.h"
#include "lsst/afw/math/BoundedField.h"
#include "lsst/afw/table/io/CatalogVector.h"
#include "lsst/afw/table/io/OutputArchive.h"
#include "lsst/daf/base/PropertySet.h"
#include "lsst/pex/exceptions.h"
#include "ndarray.h"
#include "lsst/afw/table/io/Persistable.cc"
#include "lsst/cpputils/Magnitude.h"

namespace lsst {
namespace afw {

template std::shared_ptr<image::PhotoCalib> table::io::PersistableFacade<image::PhotoCalib>::dynamicCast(
        std::shared_ptr<table::io::Persistable> const &);

namespace image {

// ------------------- helpers -------------------

std::ostream &operator<<(std::ostream &os, Measurement const &measurement) {
    std::stringstream s;
    s << std::setprecision(std::numeric_limits<long double>::digits10 + 1);
    s << "value=" << measurement.value << ", error=" << measurement.error;
    return os << s.str();
}

namespace {

int const SERIALIZATION_VERSION = 1;

double toNanojansky(double instFlux, double scale) { return instFlux * scale; }

double toMagnitude(double instFlux, double scale) {
    return cpputils::nanojanskyToABMagnitude(instFlux * scale);
}

double toInstFluxFromMagnitude(double magnitude, double scale) {
    // Note: flux[nJy] / scale = instFlux[counts]
    return cpputils::ABMagnitudeToNanojansky(magnitude) / scale;
}

double toInstFluxFromNanojansky(double nanojansky, double scale) { return nanojansky / scale; }

double toNanojanskyErr(double instFlux, double instFluxErr, double scale, double scaleErr,
                       double nanojansky) {
    return std::abs(nanojansky) * hypot(instFluxErr / instFlux, scaleErr / scale);
}

/**
 * Compute the variance of an array of fluxes, for calculations on MaskedImages.
 *
 * MaskedImage stores the variance instead of the standard deviation, so we can skip a sqrt().
 * Usage in calibrateImage() is to compute flux (nJy) directly, so that the calibration scale is
 * implictly `flux/instFlux` (and thus not passed as an argument).
 *
 * @param instFlux[in] The instrumental flux.
 * @param instFluxVar[in] The variance of the instrumental fluxes.
 * @param scaleErr[in] The error on the calibration scale.
 * @param flux[in] The physical fluxes calculated from the instrumental fluxes.
 * @param out[out] The output array to fill with the variance values.
 */
void toNanojanskyVariance(ndarray::Array<float const, 2, 1> const &instFlux,
                          ndarray::Array<float const, 2, 1> const &instFluxVar, float scaleErr,
                          ndarray::Array<float const, 2, 1> const &flux, ndarray::Array<float, 2, 1> out) {
    auto eigenFlux = ndarray::asEigen<Eigen::ArrayXpr>(flux);
    auto eigenInstFluxVar = ndarray::asEigen<Eigen::ArrayXpr>(instFluxVar);
    auto eigenInstFlux = ndarray::asEigen<Eigen::ArrayXpr>(instFlux);
    auto eigenOut = ndarray::asEigen<Eigen::ArrayXpr>(out);
    eigenOut = eigenFlux.square() * (eigenInstFluxVar / eigenInstFlux.square() +
                                     (scaleErr / (eigenFlux / eigenInstFlux)).square());
}

/**
 * Compute the variance of an array of fluxes, for calculations on nJy calibrated MaskedImages.
 *
 * MaskedImage stores the variance instead of the standard deviation, so we can skip a sqrt().
 * Usage in calibrateImage() is to compute instFlux (ADU) directly, so that the calibration scale is
 * implictly `flux/instFlux` (and thus not passed as an argument).
 *
 * @param flux[in] The flux in nJy.
 * @param fluxVar[in] The variance of the fluxes.
 * @param scaleErr[in] The error on the calibration scale.
 * @param instFlux[in] The instrumental fluxes calculated from the fluxes.
 * @param out[out] The output array to fill with the instrumental variance values.
 */
void fromNanojanskyVariance(ndarray::Array<float const, 2, 1> const &flux,
                            ndarray::Array<float const, 2, 1> const &fluxVar, float scaleErr,
                            ndarray::Array<float const, 2, 1> const &instFlux,
                            ndarray::Array<float, 2, 1> out) {
    auto eigenFlux = ndarray::asEigen<Eigen::ArrayXpr>(flux);
    auto eigenFluxVar = ndarray::asEigen<Eigen::ArrayXpr>(fluxVar);
    auto eigenInstFlux = ndarray::asEigen<Eigen::ArrayXpr>(instFlux);
    auto eigenOut = ndarray::asEigen<Eigen::ArrayXpr>(out);
    eigenOut = eigenInstFlux.square() *
               (eigenFluxVar / eigenFlux.square() - (scaleErr / (eigenFlux / eigenInstFlux)).square());
}

double toMagnitudeErr(double instFlux, double instFluxErr, double scale, double scaleErr) {
    return 2.5 / std::log(10.0) * hypot(instFluxErr / instFlux, scaleErr / scale);
}

}  // anonymous namespace

// ------------------- Conversions to nanojansky -------------------

double PhotoCalib::instFluxToNanojansky(double instFlux, lsst::geom::Point<double, 2> const &point) const {
    return toNanojansky(instFlux, evaluate(point));
}

double PhotoCalib::instFluxToNanojansky(double instFlux) const {
    return toNanojansky(instFlux, _calibrationMean);
}

Measurement PhotoCalib::instFluxToNanojansky(double instFlux, double instFluxErr,
                                             lsst::geom::Point<double, 2> const &point) const {
    double calibration, error, nanojansky;
    calibration = evaluate(point);
    nanojansky = toNanojansky(instFlux, calibration);
    error = toNanojanskyErr(instFlux, instFluxErr, calibration, _calibrationErr, nanojansky);
    return Measurement(nanojansky, error);
}

Measurement PhotoCalib::instFluxToNanojansky(double instFlux, double instFluxErr) const {
    double nanojansky = toNanojansky(instFlux, _calibrationMean);
    double error = toNanojanskyErr(instFlux, instFluxErr, _calibrationMean, _calibrationErr, nanojansky);
    return Measurement(nanojansky, error);
}

Measurement PhotoCalib::instFluxToNanojansky(afw::table::SourceRecord const &sourceRecord,
                                             std::string const &instFluxField) const {
    auto position = sourceRecord.getCentroid();
    auto instFluxKey = sourceRecord.getSchema().find<double>(instFluxField + "_instFlux").key;
    auto instFluxErrKey = sourceRecord.getSchema().find<double>(instFluxField + "_instFluxErr").key;
    return instFluxToNanojansky(sourceRecord.get(instFluxKey), sourceRecord.get(instFluxErrKey), position);
}
ndarray::Array<double, 2, 2> PhotoCalib::instFluxToNanojansky(afw::table::SourceCatalog const &sourceCatalog,
                                                              std::string const &instFluxField) const {
    ndarray::Array<double, 2, 2> result =
            ndarray::allocate(ndarray::makeVector(int(sourceCatalog.size()), 2));
    instFluxToNanojanskyArray(sourceCatalog, instFluxField, result);
    return result;
}

void PhotoCalib::instFluxToNanojansky(afw::table::SourceCatalog &sourceCatalog,
                                      std::string const &instFluxField, std::string const &outField) const {
    auto instFluxKey = sourceCatalog.getSchema().find<double>(instFluxField + "_instFlux").key;
    auto instFluxErrKey = sourceCatalog.getSchema().find<double>(instFluxField + "_instFluxErr").key;
    auto nanojanskyKey = sourceCatalog.getSchema().find<double>(outField + "_flux").key;
    auto nanojanskyErrKey = sourceCatalog.getSchema().find<double>(outField + "_fluxErr").key;
    for (auto &record : sourceCatalog) {
        auto result = instFluxToNanojansky(record.get(instFluxKey), record.get(instFluxErrKey),
                                           record.getCentroid());
        record.set(nanojanskyKey, result.value);
        record.set(nanojanskyErrKey, result.error);
    }
}

// ------------------- Conversions to Magnitudes -------------------

double PhotoCalib::instFluxToMagnitude(double instFlux, lsst::geom::Point<double, 2> const &point) const {
    return toMagnitude(instFlux, evaluate(point));
}

double PhotoCalib::instFluxToMagnitude(double instFlux) const {
    return toMagnitude(instFlux, _calibrationMean);
}

Measurement PhotoCalib::instFluxToMagnitude(double instFlux, double instFluxErr,
                                            lsst::geom::Point<double, 2> const &point) const {
    double calibration, error, magnitude;
    calibration = evaluate(point);
    magnitude = toMagnitude(instFlux, calibration);
    error = toMagnitudeErr(instFlux, instFluxErr, calibration, _calibrationErr);
    return Measurement(magnitude, error);
}

Measurement PhotoCalib::instFluxToMagnitude(double instFlux, double instFluxErr) const {
    double magnitude = toMagnitude(instFlux, _calibrationMean);
    double error = toMagnitudeErr(instFlux, instFluxErr, _calibrationMean, _calibrationErr);
    return Measurement(magnitude, error);
}

Measurement PhotoCalib::instFluxToMagnitude(afw::table::SourceRecord const &sourceRecord,
                                            std::string const &instFluxField) const {
    auto position = sourceRecord.getCentroid();
    auto instFluxKey = sourceRecord.getSchema().find<double>(instFluxField + "_instFlux").key;
    auto instFluxErrKey = sourceRecord.getSchema().find<double>(instFluxField + "_instFluxErr").key;
    return instFluxToMagnitude(sourceRecord.get(instFluxKey), sourceRecord.get(instFluxErrKey), position);
}

ndarray::Array<double, 2, 2> PhotoCalib::instFluxToMagnitude(afw::table::SourceCatalog const &sourceCatalog,
                                                             std::string const &instFluxField) const {
    ndarray::Array<double, 2, 2> result =
            ndarray::allocate(ndarray::makeVector(int(sourceCatalog.size()), 2));
    instFluxToMagnitudeArray(sourceCatalog, instFluxField, result);
    return result;
}

void PhotoCalib::instFluxToMagnitude(afw::table::SourceCatalog &sourceCatalog,
                                     std::string const &instFluxField, std::string const &outField) const {
    auto instFluxKey = sourceCatalog.getSchema().find<double>(instFluxField + "_instFlux").key;
    auto instFluxErrKey = sourceCatalog.getSchema().find<double>(instFluxField + "_instFluxErr").key;
    auto magKey = sourceCatalog.getSchema().find<double>(outField + "_mag").key;
    auto magErrKey = sourceCatalog.getSchema().find<double>(outField + "_magErr").key;
    for (auto &record : sourceCatalog) {
        auto result = instFluxToMagnitude(record.get(instFluxKey), record.get(instFluxErrKey),
                                          record.getCentroid());
        record.set(magKey, result.value);
        record.set(magErrKey, result.error);
    }
}

// ------------------- other utility methods -------------------

double PhotoCalib::magnitudeToInstFlux(double magnitude) const {
    return toInstFluxFromMagnitude(magnitude, _calibrationMean);
}

double PhotoCalib::magnitudeToInstFlux(double magnitude, lsst::geom::Point<double, 2> const &point) const {
    return toInstFluxFromMagnitude(magnitude, evaluate(point));
}

double PhotoCalib::nanojanskyToInstFlux(double nanojansky) const {
    return toInstFluxFromNanojansky(nanojansky, _calibrationMean);
}

double PhotoCalib::nanojanskyToInstFlux(double nanojansky, lsst::geom::Point<double, 2> const &point) const {
    return toInstFluxFromNanojansky(nanojansky, evaluate(point));
}

std::shared_ptr<math::BoundedField> PhotoCalib::computeScaledCalibration() const {
    return *(_calibration) / _calibrationMean;
}

std::shared_ptr<math::BoundedField> PhotoCalib::computeScalingTo(std::shared_ptr<PhotoCalib> other) const {
    throw LSST_EXCEPT(pex::exceptions::LogicError, "Not Implemented: See DM-10154.");
}

bool PhotoCalib::operator==(PhotoCalib const &rhs) const {
    return (_calibrationMean == rhs._calibrationMean && _calibrationErr == rhs._calibrationErr &&
            (*_calibration) == *(rhs._calibration));
}

double PhotoCalib::computeCalibrationMean(std::shared_ptr<afw::math::BoundedField> calibration) const {
    return calibration->mean();
}

std::shared_ptr<typehandling::Storable> PhotoCalib::cloneStorable() const {
    return std::make_unique<PhotoCalib>(*this);
}

std::string PhotoCalib::toString() const {
    std::stringstream buffer;
    if (_isConstant)
        buffer << "spatially constant with ";
    else
        buffer << *_calibration << " with ";
    buffer << "mean: " << _calibrationMean << " error: " << _calibrationErr;
    return buffer.str();
}

bool PhotoCalib::equals(typehandling::Storable const &other) const noexcept {
    return singleClassEquals(*this, other);
}

std::ostream &operator<<(std::ostream &os, PhotoCalib const &photoCalib) {
    return os << photoCalib.toString();
}

MaskedImage<float> PhotoCalib::calibrateImage(MaskedImage<float> const &maskedImage,
                                              bool includeScaleUncertainty) const {
    // Deep copy construct, as we're mutiplying in-place.
    auto result = MaskedImage<float>(maskedImage, true);

    if (_isConstant) {
        *(result.getImage()) *= _calibrationMean;
    } else {
        _calibration->multiplyImage(*(result.getImage()), true);  // only in the overlap region
    }
    if (includeScaleUncertainty) {
        toNanojanskyVariance(maskedImage.getImage()->getArray(), maskedImage.getVariance()->getArray(),
                             _calibrationErr, result.getImage()->getArray(),
                             result.getVariance()->getArray());
    } else {
        toNanojanskyVariance(maskedImage.getImage()->getArray(), maskedImage.getVariance()->getArray(), 0,
                             result.getImage()->getArray(), result.getVariance()->getArray());
    }

    return result;
}

MaskedImage<float> PhotoCalib::uncalibrateImage(MaskedImage<float> const &maskedImage,
                                                bool includeScaleUncertainty) const {
    // Deep copy construct, as we're mutiplying in-place.
    auto result = MaskedImage<float>(maskedImage, true);

    if (_isConstant) {
        *(result.getImage()) /= _calibrationMean;
    } else {
        _calibration->divideImage(*(result.getImage()), true);  // only in the overlap region
    }
    if (includeScaleUncertainty) {
        fromNanojanskyVariance(maskedImage.getImage()->getArray(), maskedImage.getVariance()->getArray(),
                               _calibrationErr, result.getImage()->getArray(),
                               result.getVariance()->getArray());
    } else {
        fromNanojanskyVariance(maskedImage.getImage()->getArray(), maskedImage.getVariance()->getArray(), 0,
                               result.getImage()->getArray(), result.getVariance()->getArray());
    }

    return result;
}

afw::table::SourceCatalog PhotoCalib::calibrateCatalog(afw::table::SourceCatalog const &catalog,
                                                       std::vector<std::string> const &instFluxFields) const {
    auto const &inSchema = catalog.getSchema();
    afw::table::SchemaMapper mapper(inSchema, true);  // true: share the alias map
    mapper.addMinimalSchema(inSchema);

    using FieldD = afw::table::Field<double>;

    struct Keys {
        table::Key<double> instFlux;
        table::Key<double> instFluxErr;
        table::Key<double> flux;
        table::Key<double> fluxErr;
        table::Key<double> mag;
        table::Key<double> magErr;
    };

    std::vector<Keys> keys;
    keys.reserve(instFluxFields.size());
    for (auto const &field : instFluxFields) {
        Keys newKey;
        newKey.instFlux = inSchema[inSchema.join(field, "instFlux")];
        newKey.flux =
                mapper.addOutputField(FieldD(inSchema.join(field, "flux"), "calibrated flux", "nJy"), true);
        newKey.mag = mapper.addOutputField(
                FieldD(inSchema.join(field, "mag"), "calibrated magnitude", "mag(AB)"), true);
        try {
            newKey.instFluxErr = inSchema.find<double>(inSchema.join(field, "instFluxErr")).key;
            newKey.fluxErr = mapper.addOutputField(
                    FieldD(inSchema.join(field, "fluxErr"), "calibrated flux uncertainty", "nJy"), true);
            newKey.magErr = mapper.addOutputField(
                    FieldD(inSchema.join(field, "magErr"), "calibrated magnitude uncertainty", "mag(AB)"),
                    true);
        } catch (pex::exceptions::NotFoundError &) {
            ;  // Keys struct defaults to invalid keys; that marks the error as missing.
        }
        keys.emplace_back(newKey);
    }

    // Create the new catalog
    afw::table::SourceCatalog output(mapper.getOutputSchema());
    output.insert(mapper, output.begin(), catalog.begin(), catalog.end());

    auto calibration = evaluateCatalog(output);

    // fill in the catalog values
    int iRec = 0;
    for (auto &rec : output) {
        for (auto &key : keys) {
            double instFlux = rec.get(key.instFlux);
            double nanojansky = toNanojansky(instFlux, calibration[iRec]);
            rec.set(key.flux, nanojansky);
            rec.set(key.mag, toMagnitude(instFlux, calibration[iRec]));
            if (key.instFluxErr.isValid()) {
                double instFluxErr = rec.get(key.instFluxErr);
                rec.set(key.fluxErr, toNanojanskyErr(instFlux, instFluxErr, calibration[iRec],
                                                     _calibrationErr, nanojansky));
                rec.set(key.magErr,
                        toMagnitudeErr(instFlux, instFluxErr, calibration[iRec], _calibrationErr));
            }
        }
        ++iRec;
    }

    return output;
}

afw::table::SourceCatalog PhotoCalib::calibrateCatalog(afw::table::SourceCatalog const &catalog) const {
    std::vector<std::string> instFluxFields;
    static std::string const SUFFIX = "_instFlux";
    for (auto const &name : catalog.getSchema().getNames()) {
        // Pick every field ending in "_instFlux", grabbing everything before that prefix.
        if (name.size() > SUFFIX.size() + 1 &&
            name.compare(name.size() - SUFFIX.size(), SUFFIX.size(), SUFFIX) == 0) {
            instFluxFields.emplace_back(name.substr(0, name.size() - 9));
        }
    }
    return calibrateCatalog(catalog, instFluxFields);
}

// ------------------- persistence -------------------

namespace {

class PhotoCalibSchema {
public:
    table::Schema schema;
    table::Key<double> calibrationMean;
    table::Key<double> calibrationErr;
    table::Key<table::Flag> isConstant;
    table::Key<int> field;
    table::Key<int> version;

    // No copying
    PhotoCalibSchema(PhotoCalibSchema const &) = delete;
    PhotoCalibSchema &operator=(PhotoCalibSchema const &) = delete;
    // No moving
    PhotoCalibSchema(PhotoCalibSchema &&) = delete;
    PhotoCalibSchema &operator=(PhotoCalibSchema &&) = delete;

    static PhotoCalibSchema const &get() {
        static PhotoCalibSchema const instance;
        return instance;
    }

private:
    PhotoCalibSchema()
            : schema(),
              calibrationMean(schema.addField<double>(
                      "calibrationMean", "mean calibration on this PhotoCalib's domain", "count")),
              calibrationErr(
                      schema.addField<double>("calibrationErr", "1-sigma error on calibrationMean", "count")),
              isConstant(schema.addField<table::Flag>("isConstant", "Is this spatially-constant?")),
              field(schema.addField<int>("field", "archive ID of the BoundedField object")),
              version(schema.addField<int>("version", "version of this PhotoCalib")) {}
};

class PhotoCalibFactory : public table::io::PersistableFactory {
public:
    std::shared_ptr<table::io::Persistable> read(InputArchive const &archive,
                                                 CatalogVector const &catalogs) const override {
        table::BaseRecord const &record = catalogs.front().front();
        PhotoCalibSchema const &keys = PhotoCalibSchema::get();
        int version = getVersion(record);
        if (version < 1) {
            throw(pex::exceptions::RuntimeError("Unsupported version (version 0 was defined in maggies): " +
                                                std::to_string(version)));
        }
        return std::make_shared<PhotoCalib>(record.get(keys.calibrationMean), record.get(keys.calibrationErr),
                                            archive.get<afw::math::BoundedField>(record.get(keys.field)),
                                            record.get(keys.isConstant));
    }

    PhotoCalibFactory(std::string const &name) : afw::table::io::PersistableFactory(name) {}

protected:
    int getVersion(table::BaseRecord const &record) const {
        int version = -1;
        try {
            std::string versionName = "version";
            auto versionKey = record.getSchema().find<int>(versionName);
            version = record.get(versionKey.key);
        } catch (const pex::exceptions::NotFoundError &) {
            // un-versioned files are version 0
            version = 0;
        }
        return version;
    }
};

std::string getPhotoCalibPersistenceName() { return "PhotoCalib"; }

PhotoCalibFactory registration(getPhotoCalibPersistenceName());

}  // namespace

/*
 * Backwards-compatibility support for depersisting the old Calib (FluxMag0/FluxMag0Err) objects.
 */

namespace {
int const CALIB_TABLE_CURRENT_VERSION = 2;         // final version of Calib in ExposureTable
std::string const EXPTIME_FIELD_NAME = "exptime";  // name of exposure time field (no longer used)

class CalibKeys {
public:
    table::Schema schema;
    table::Key<std::int64_t> midTime;
    table::Key<double> expTime;
    table::Key<double> fluxMag0;
    table::Key<double> fluxMag0Err;

    // No copying
    CalibKeys(const CalibKeys &) = delete;
    CalibKeys &operator=(const CalibKeys &) = delete;

    // No moving
    CalibKeys(CalibKeys &&) = delete;
    CalibKeys &operator=(CalibKeys &&) = delete;

    CalibKeys(int tableVersion = CALIB_TABLE_CURRENT_VERSION)
            : schema(), midTime(), expTime(), fluxMag0(), fluxMag0Err() {
        if (tableVersion == 1) {
            // obsolete fields
            midTime = schema.addField<std::int64_t>(
                    "midtime", "middle of the time of the exposure relative to Unix epoch", "ns");
            expTime = schema.addField<double>(EXPTIME_FIELD_NAME, "exposure time", "s");
        }
        fluxMag0 = schema.addField<double>("fluxmag0", "flux of a zero-magnitude object", "count");
        fluxMag0Err = schema.addField<double>("fluxmag0.err", "1-sigma error on fluxmag0", "count");
    }
};

class CalibFactory : public table::io::PersistableFactory {
public:
    std::shared_ptr<table::io::Persistable> read(InputArchive const &archive,
                                                 CatalogVector const &catalogs) const override {
        // table version is not persisted, so we don't have a clean way to determine the version;
        // the hack is version = 1 if exptime found, else current
        int tableVersion = 1;
        try {
            catalogs.front().getSchema().find<double>(EXPTIME_FIELD_NAME);
        } catch (pex::exceptions::NotFoundError const &) {
            tableVersion = CALIB_TABLE_CURRENT_VERSION;
        }

        CalibKeys const keys{tableVersion};
        LSST_ARCHIVE_ASSERT(catalogs.size() == 1u);
        LSST_ARCHIVE_ASSERT(catalogs.front().size() == 1u);
        LSST_ARCHIVE_ASSERT(catalogs.front().getSchema() == keys.schema);
        table::BaseRecord const &record = catalogs.front().front();

        double calibration = cpputils::referenceFlux / record.get(keys.fluxMag0);
        double calibrationErr = cpputils::referenceFlux * record.get(keys.fluxMag0Err) /
                                std::pow(record.get(keys.fluxMag0), 2);
        return std::make_shared<PhotoCalib>(calibration, calibrationErr);
    }

    explicit CalibFactory(std::string const &name) : table::io::PersistableFactory(name) {}
};

std::string getCalibPersistenceName() { return "Calib"; }

CalibFactory calibRegistration(getCalibPersistenceName());

}  // namespace

std::string PhotoCalib::getPersistenceName() const { return getPhotoCalibPersistenceName(); }

void PhotoCalib::write(OutputArchiveHandle &handle) const {
    PhotoCalibSchema const &keys = PhotoCalibSchema::get();
    table::BaseCatalog catalog = handle.makeCatalog(keys.schema);
    auto record = catalog.addNew();
    record->set(keys.calibrationMean, _calibrationMean);
    record->set(keys.calibrationErr, _calibrationErr);
    record->set(keys.isConstant, _isConstant);
    record->set(keys.field, handle.put(_calibration));
    record->set(keys.version, SERIALIZATION_VERSION);
    handle.saveCatalog(catalog);
}

// ------------------- private/protected helpers -------------------

double PhotoCalib::evaluate(lsst::geom::Point<double, 2> const &point) const {
    if (_isConstant)
        return _calibrationMean;
    else
        return _calibration->evaluate(point);
}

ndarray::Array<double, 1> PhotoCalib::evaluateArray(ndarray::Array<double, 1> const &xx,
                                                    ndarray::Array<double, 1> const &yy) const {
    if (_isConstant) {
        ndarray::Array<double, 1> result = ndarray::allocate(ndarray::makeVector(xx.size()));
        result.deep() = _calibrationMean;
        return result;
    } else {
        return _calibration->evaluate(xx, yy);
    }
}

ndarray::Array<double, 1> PhotoCalib::evaluateCatalog(afw::table::SourceCatalog const &sourceCatalog) const {
    ndarray::Array<double, 1> xx = ndarray::allocate(ndarray::makeVector(sourceCatalog.size()));
    ndarray::Array<double, 1> yy = ndarray::allocate(ndarray::makeVector(sourceCatalog.size()));
    size_t i = 0;
    for (auto const &rec : sourceCatalog) {
        auto point = rec.getCentroid();
        xx[i] = point.getX();
        yy[i] = point.getY();
        ++i;
    }
    return evaluateArray(xx, yy);
}

void PhotoCalib::instFluxToNanojanskyArray(afw::table::SourceCatalog const &sourceCatalog,
                                           std::string const &instFluxField,
                                           ndarray::Array<double, 2, 2> result) const {
    auto instFluxKey = sourceCatalog.getSchema().find<double>(instFluxField + "_instFlux").key;
    auto instFluxErrKey = sourceCatalog.getSchema().find<double>(instFluxField + "_instFluxErr").key;

    auto calibration = evaluateCatalog(sourceCatalog);
    int i = 0;
    auto iter = result.begin();
    for (auto const &rec : sourceCatalog) {
        double instFlux = rec.get(instFluxKey);
        double instFluxErr = rec.get(instFluxErrKey);
        double nanojansky = toNanojansky(instFlux, calibration[i]);
        (*iter)[0] = nanojansky;
        (*iter)[1] = toNanojanskyErr(instFlux, instFluxErr, calibration[i], _calibrationErr, nanojansky);
        ++iter;
        ++i;
    }
}

void PhotoCalib::instFluxToMagnitudeArray(afw::table::SourceCatalog const &sourceCatalog,
                                          std::string const &instFluxField,
                                          ndarray::Array<double, 2, 2> result) const {
    auto instFluxKey = sourceCatalog.getSchema().find<double>(instFluxField + "_instFlux").key;
    auto instFluxErrKey = sourceCatalog.getSchema().find<double>(instFluxField + "_instFluxErr").key;

    auto calibration = evaluateCatalog(sourceCatalog);
    auto iter = result.begin();
    int i = 0;
    for (auto const &rec : sourceCatalog) {
        double instFlux = rec.get(instFluxKey);
        double instFluxErr = rec.get(instFluxErrKey);
        (*iter)[0] = toMagnitude(instFlux, calibration[i]);
        (*iter)[1] = toMagnitudeErr(instFlux, instFluxErr, calibration[i], _calibrationErr);
        ++iter;
        ++i;
    }
}

std::shared_ptr<PhotoCalib> makePhotoCalibFromMetadata(daf::base::PropertySet &metadata, bool strip) {
    auto key = "FLUXMAG0";
    if (metadata.exists(key)) {
        double instFluxMag0 = metadata.getAsDouble(key);
        if (strip) metadata.remove(key);

        double instFluxMag0Err = 0.0;
        key = "FLUXMAG0ERR";
        if (metadata.exists(key)) {
            instFluxMag0Err = metadata.getAsDouble(key);
            if (strip) metadata.remove(key);
        }
        return makePhotoCalibFromCalibZeroPoint(instFluxMag0, instFluxMag0Err);
    } else {
        return nullptr;
    }
}

std::shared_ptr<PhotoCalib> makePhotoCalibFromCalibZeroPoint(double instFluxMag0, double instFluxMag0Err) {
    double calibration = cpputils::referenceFlux / instFluxMag0;
    double calibrationErr = cpputils::referenceFlux * instFluxMag0Err / std::pow(instFluxMag0, 2);
    return std::make_shared<PhotoCalib>(calibration, calibrationErr);
}

}  // namespace image
}  // namespace afw
}  // namespace lsst
