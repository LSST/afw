import os.path
import lsst.geom
from lsst.afw.table import AmpInfoCatalog
from .cameraGeomLib import FOCAL_PLANE, FIELD_ANGLE, PIXELS, TAN_PIXELS, ACTUAL_PIXELS, CameraSys, \
    Detector, DetectorType, Orientation, TransformMap
from .camera import Camera
from .makePixelToTanPixel import makePixelToTanPixel
from .pupil import PupilFactory

__all__ = ["makeCameraFromPath", "makeCameraFromCatalogs",
           "makeDetector", "copyDetector"]

cameraSysList = [FIELD_ANGLE, FOCAL_PLANE, PIXELS, TAN_PIXELS, ACTUAL_PIXELS]
cameraSysMap = dict((sys.getSysName(), sys) for sys in cameraSysList)


def makeDetector(detectorConfig, ampInfoCatalog, focalPlaneToField):
    """Make a Detector instance from a detector config and amp info catalog

    Parameters
    ----------
    detectorConfig : `lsst.pex.config.Config`
        Configuration for this detector.
    ampInfoCatalog : `lsst.afw.table.AmpInfoCatalog`
        amplifier information for this detector
    focalPlaneToField : `lsst.afw.geom.TransformPoint2ToPoint2`
        FOCAL_PLANE to FIELD_ANGLE Transform

    Returns
    -------
    detector : `lsst.afw.cameraGeom.Detector`
        New Detector instance.
    """
    orientation = makeOrientation(detectorConfig)
    pixelSizeMm = lsst.geom.Extent2D(
        detectorConfig.pixelSize_x, detectorConfig.pixelSize_y)
    transforms = makeTransformDict(detectorConfig.transformDict.transforms)
    transforms[FOCAL_PLANE] = orientation.makePixelFpTransform(pixelSizeMm)

    llPoint = lsst.geom.Point2I(detectorConfig.bbox_x0, detectorConfig.bbox_y0)
    urPoint = lsst.geom.Point2I(detectorConfig.bbox_x1, detectorConfig.bbox_y1)
    bbox = lsst.geom.Box2I(llPoint, urPoint)

    tanPixSys = CameraSys(TAN_PIXELS, detectorConfig.name)
    transforms[tanPixSys] = makePixelToTanPixel(
        bbox=bbox,
        orientation=orientation,
        focalPlaneToField=focalPlaneToField,
        pixelSizeMm=pixelSizeMm,
    )

    data = dict(
        name=detectorConfig.name,
        id=detectorConfig.id,
        type=DetectorType(detectorConfig.detectorType),
        serial=detectorConfig.serial,
        bbox=bbox,
        ampInfoCatalog=ampInfoCatalog,
        orientation=orientation,
        pixelSize=pixelSizeMm,
        transforms=transforms,
    )
    crosstalk = detectorConfig.getCrosstalk(len(ampInfoCatalog))
    if crosstalk is not None:
        data["crosstalk"] = crosstalk
    return Detector(**data)


def copyDetector(detector, ampInfoCatalog=None):
    """Return a copy of a Detector with possibly-updated amplifier information.

    No deep copies are made; the input transformDict is used unmodified

    Parameters
    ----------
    detector : `lsst.afw.cameraGeom.Detector`
        The Detector to clone
    ampInfoCatalog  The ampInfoCatalog to use; default use original

    Returns
    -------
    detector : `lsst.afw.cameraGeom.Detector`
        New Detector instance.
    """
    if ampInfoCatalog is None:
        ampInfoCatalog = detector.getAmpInfoCatalog()

    return Detector(detector.getName(), detector.getId(), detector.getType(),
                    detector.getSerial(), detector.getBBox(),
                    ampInfoCatalog, detector.getOrientation(), detector.getPixelSize(),
                    detector.getTransformMap(), detector.getCrosstalk())


def makeOrientation(detectorConfig):
    """Make an Orientation instance from a detector config

    Parameters
    ----------
    detectorConfig : `lsst.pex.config.Config`
        Configuration for this detector.

    Returns
    -------
    orientation : `lsst.afw.cameraGeom.Orientation`
        Location and rotation of the Detector.
    """
    offset = lsst.geom.Point2D(detectorConfig.offset_x, detectorConfig.offset_y)
    refPos = lsst.geom.Point2D(detectorConfig.refpos_x, detectorConfig.refpos_y)
    yaw = lsst.geom.Angle(detectorConfig.yawDeg, lsst.geom.degrees)
    pitch = lsst.geom.Angle(detectorConfig.pitchDeg, lsst.geom.degrees)
    roll = lsst.geom.Angle(detectorConfig.rollDeg, lsst.geom.degrees)
    return Orientation(offset, refPos, yaw, pitch, roll)


def makeTransformDict(transformConfigDict):
    """Make a dictionary of CameraSys: lsst.afw.geom.Transform from a config dict.

    Parameters
    ----------
    transformConfigDict : value obtained from a `lsst.pex.config.ConfigDictField`
        registry; keys are camera system names.

    Returns
    -------
    transforms : `dict`
        A dict of CameraSys or CameraSysPrefix: lsst.afw.geom.Transform
    """
    resMap = dict()
    if transformConfigDict is not None:
        for key in transformConfigDict:
            transform = transformConfigDict[key].transform.apply()
            resMap[CameraSys(key)] = transform
    return resMap


def makeCameraFromPath(cameraConfig, ampInfoPath, shortNameFunc,
                       pupilFactoryClass=PupilFactory):
    """Make a Camera instance from a directory of ampInfo files

    The directory must contain one ampInfo fits file for each detector in cameraConfig.detectorList.
    The name of each ampInfo file must be shortNameFunc(fullDetectorName) + ".fits".

    Parameters
    ----------
    cameraConfig : `CameraConfig`
        Config describing camera and its detectors.
    ampInfoPath : `str`
        Path to ampInfo data files.
    shortNameFunc : callable
        A function that converts a long detector name to a short one.
    pupilFactoryClass : `type`, optional
        Class to attach to camera; default is `lsst.afw.cameraGeom.PupilFactory`.

    Returns
    -------
    camera : `lsst.afw.cameraGeom.Camera`
        New Camera instance.
    """
    ampInfoCatDict = dict()
    for detectorConfig in cameraConfig.detectorList.values():
        shortName = shortNameFunc(detectorConfig.name)
        ampCatPath = os.path.join(ampInfoPath, shortName + ".fits")
        ampInfoCatalog = AmpInfoCatalog.readFits(ampCatPath)
        ampInfoCatDict[detectorConfig.name] = ampInfoCatalog

    return makeCameraFromCatalogs(cameraConfig, ampInfoCatDict, pupilFactoryClass)


def makeCameraFromCatalogs(cameraConfig, ampInfoCatDict,
                           pupilFactoryClass=PupilFactory):
    """Construct a Camera instance from a dictionary of detector name: AmpInfoCatalog

    Parameters
    ----------
    cameraConfig : `CameraConfig`
        Config describing camera and its detectors.
    ampInfoCatDict : `dict`
        A dictionary of detector name: AmpInfoCatalog
    pupilFactoryClass : `type`, optional
        Class to attach to camera; `lsst.default afw.cameraGeom.PupilFactory`.

    Returns
    -------
    camera : `lsst.afw.cameraGeom.Camera`
        New Camera instance.
    """
    nativeSys = cameraSysMap[cameraConfig.transformDict.nativeSys]
    transformDict = makeTransformDict(cameraConfig.transformDict.transforms)
    focalPlaneToField = transformDict[FIELD_ANGLE]
    transformMap = TransformMap(nativeSys, transformDict)

    detectorList = []
    for detectorConfig in cameraConfig.detectorList.values():
        ampInfoCatalog = ampInfoCatDict[detectorConfig.name]

        detectorList.append(makeDetector(
            detectorConfig=detectorConfig,
            ampInfoCatalog=ampInfoCatalog,
            focalPlaneToField=focalPlaneToField,
        ))

    return Camera(cameraConfig.name, detectorList, transformMap, pupilFactoryClass)
