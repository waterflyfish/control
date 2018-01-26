/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "SurveyMissionItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "SimpleMissionItem.h"
#include <QPolygonF>

QGC_LOGGING_CATEGORY(SurveyMissionItemLog, "SurveyMissionItemLog")

const char* SurveyMissionItem::_jsonTypeKey =                       "type";
const char* SurveyMissionItem::_jsonPolygonObjectKey =              "polygon";
const char* SurveyMissionItem::_jsonIdKey =                         "id";
const char* SurveyMissionItem::_jsonGridObjectKey =                 "grid";
const char* SurveyMissionItem::_jsonGridAltitudeKey =               "altitude";
const char* SurveyMissionItem::_jsonGridAltitudeRelativeKey =       "relativeAltitude";
const char* SurveyMissionItem::_jsonGridAngleKey =                  "angle";
const char* SurveyMissionItem::_jsonGridSpacingKey =                "spacing";
const char* SurveyMissionItem::_jsonGridSpacingAdd1Key =       "Architecture";
const char* SurveyMissionItem::_jsonTurnaroundDistKey =             "turnAroundDistance";
const char* SurveyMissionItem::_jsonCameraTriggerKey =              "cameraTrigger";
const char* SurveyMissionItem::_jsonCameraTriggerDistanceKey =      "cameraTriggerDistance";
const char* SurveyMissionItem::_jsonGroundResolutionKey =           "groundResolution";
const char* SurveyMissionItem::_jsonFrontalOverlapKey =             "imageFrontalOverlap";
const char* SurveyMissionItem::_jsonSideOverlapKey =                "imageSizeOverlap";
const char* SurveyMissionItem::_jsonCameraSensorWidthKey =          "sensorWidth";
const char* SurveyMissionItem::_jsonCameraSensorHeightKey =         "sensorHeight";
const char* SurveyMissionItem::_jsonCameraResolutionWidthKey =      "resolutionWidth";
const char* SurveyMissionItem::_jsonCameraResolutionHeightKey =     "resolutionHeight";
const char* SurveyMissionItem::_jsonCameraFocalLengthKey =          "focalLength";
const char* SurveyMissionItem::_jsonCameraObjectKey =               "camera";
const char* SurveyMissionItem::_jsonCameraNameKey =                 "name";
const char* SurveyMissionItem::_jsonManualGridKey =                 "manualGrid";
const char* SurveyMissionItem::_jsonCameraOrientationLandscapeKey = "orientationLandscape";
const char* SurveyMissionItem::_jsonFixedValueIsAltitudeKey =       "fixedValueIsAltitude";

const char* SurveyMissionItem::_gridAltitudeFactName =              "Altitude";
const char* SurveyMissionItem::_gridAngleFactName =                 "Grid angle";
const char* SurveyMissionItem::_gridSpacingFactName =               "Grid spacing";
const char* SurveyMissionItem::_turnaroundDistFactName =            "Turnaround dist";
const char* SurveyMissionItem::_cameraTriggerDistanceFactName =     "Camera trigger distance";
const char* SurveyMissionItem::_groundResolutionFactName =          "Ground resolution";
const char* SurveyMissionItem::_frontalOverlapFactName =            "Frontal overlap";
const char* SurveyMissionItem::_sideOverlapFactName =               "Side overlap";
const char* SurveyMissionItem::_cameraSensorWidthFactName =         "Camera sensor width";
const char* SurveyMissionItem::_cameraSensorHeightFactName =        "Camera sensor height";
const char* SurveyMissionItem::_cameraResolutionWidthFactName =     "Camera resolution width";
const char* SurveyMissionItem::_cameraResolutionHeightFactName =    "Camera resolution height";
const char* SurveyMissionItem::_cameraFocalLengthFactName =         "Focal length";

const char* SurveyMissionItem::_complexType = "survey";

QMap<QString, FactMetaData*> SurveyMissionItem::_metaDataMap;

/// < Synchronization algorithm with vehicle
///

#ifdef CONSTANTS_RADIUS_OF_EARTH_VEHICLE
#error defined CONSTANTS_RADIUS_OF_EARTH_VEHICLE conflict
#else
#define CONSTANTS_RADIUS_OF_EARTH_VEHICLE    6371000    /* meters (m) */
#endif

#ifdef M_DEG_TO_RAD_VEHICLE
#error defined M_DEG_TO_RAD_VEHICLE conflict
#else
#define M_DEG_TO_RAD_VEHICLE    0.01745329251994
#endif

#ifdef M_RAD_TO_DEG_VEHICLE
#error defined M_RAD_TO_DEG_VEHICLE conflict
#else
#define M_RAD_TO_DEG_VEHICLE    57.2957795130823
#endif

#ifdef M_DEG_TO_RAD_F_VEHICLE
#error defined M_DEG_TO_RAD_F_VEHICLE conflict
#else
#define M_DEG_TO_RAD_F_VEHICLE    0.01745329251994f
#endif

#ifdef M_RAD_TO_DEG_F_VEHICLE
#error defined M_RAD_TO_DEG_F_VEHICLE conflict
#else
#define M_RAD_TO_DEG_F_VEHICLE    57.2957795130823f
#endif

#ifdef M_PI_F_VEHICLE
#error defined M_PI_F_VEHICLE conflict
#else
#define M_PI_F_VEHICLE    3.14159265358979323846f
#endif

#ifdef M_TWOPI_F_VEHICLE
#error defined M_TWOPI_F_VEHICLE conflict
#else
#define M_TWOPI_F_VEHICLE    (M_PI_F_VEHICLE * 2.0f)
#endif

#ifdef PX4_ISFINITE_VEHICLE
#error defined PX4_ISFINITE_VEHICLE(x) conflict
#else
#define PX4_ISFINITE_VEHICLE(x)    std::isfinite(x)
#endif




/* The difference between 1 and the least value greater than 1 that is
 * representable in the given floating-point type, b1-p.
 */

#define FLT_EPSILON 1.1920929e-07F  /* 1E-5 */

float _wrap_pi(float bearing)
{
    /* value is inf or NaN */
    if (!std::isfinite(bearing)) {
        return bearing;
    }

    int c = 0;

    while (bearing >= M_PI_F_VEHICLE) {
        bearing -= M_TWOPI_F_VEHICLE;

        if (c++ > 3) {
            return NAN;
        }
    }

    c = 0;

    while (bearing < -M_PI_F_VEHICLE) {
        bearing += M_TWOPI_F_VEHICLE;

        if (c++ > 3) {
            return NAN;
        }
    }

    return bearing;
}

float _wrap_2pi(float bearing)
{
    /* value is inf or NaN */
    if (!PX4_ISFINITE_VEHICLE(bearing)) {
        return bearing;
    }

    int c = 0;

    while (bearing >= M_TWOPI_F_VEHICLE) {
        bearing -= M_TWOPI_F_VEHICLE;

        if (c++ > 3) {
            return NAN;
        }
    }

    c = 0;

    while (bearing < 0.0f) {
        bearing += M_TWOPI_F_VEHICLE;

        if (c++ > 3) {
            return NAN;
        }
    }

    return bearing;
}

/**
 * Returns the bearing to the next waypoint in radians.
 *
 * @param lat_now current position in degrees (47.1234567, not 471234567)
 * @param lon_now current position in degrees (8.1234567, not 81234567)
 * @param lat_next next waypoint position in degrees (47.1234567, not 471234567)
 * @param lon_next next waypoint position in degrees (8.1234567, not 81234567)
 */

float get_bearing_to_next_waypoint(double lat_now, double lon_now, double lat_next, double lon_next)
{
    double lat_now_rad = lat_now * M_DEG_TO_RAD_VEHICLE;
    double lon_now_rad = lon_now * M_DEG_TO_RAD_VEHICLE;
    double lat_next_rad = lat_next * M_DEG_TO_RAD_VEHICLE;
    double lon_next_rad = lon_next * M_DEG_TO_RAD_VEHICLE;

    double d_lon = lon_next_rad - lon_now_rad;

    /* conscious mix of double and float trig function to maximize speed and efficiency */
    float theta = atan2f(sin(d_lon) * cos(lat_next_rad) ,
                 cos(lat_now_rad) * sin(lat_next_rad) - sin(lat_now_rad) * cos(lat_next_rad) * cos(d_lon));

    theta = _wrap_pi(theta);

    return theta;
}

/**
 * Returns the distance to the next waypoint in meters.
 *
 * @param lat_now current position in degrees (47.1234567, not 471234567)
 * @param lon_now current position in degrees (8.1234567, not 81234567)
 * @param lat_next next waypoint position in degrees (47.1234567, not 471234567)
 * @param lon_next next waypoint position in degrees (8.1234567, not 81234567)
 */
double get_distance_to_next_waypoint(double lat_now, double lon_now, double lat_next, double lon_next)
{
    double lat_now_rad = lat_now / (double)180.0 * M_PI;
    double lon_now_rad = lon_now / (double)180.0 * M_PI;
    double lat_next_rad = lat_next / (double)180.0 * M_PI;
    double lon_next_rad = lon_next / (double)180.0 * M_PI;


    double d_lat = lat_next_rad - lat_now_rad;
    double d_lon = lon_next_rad - lon_now_rad;

    double a = sin(d_lat / (double)2.0) * sin(d_lat / (double)2.0) + sin(d_lon / (double)2.0) * sin(d_lon /
            (double)2.0) * cos(lat_now_rad) * cos(lat_next_rad);
    double c = (double)2.0 * atan2(sqrt(a), sqrt((double)1.0 - a));

    return CONSTANTS_RADIUS_OF_EARTH_VEHICLE * c;
}

/**
 * Creates a waypoint from given waypoint, distance and bearing
 * see http://www.movable-type.co.uk/scripts/latlong.html
 *
 * @param lat_start latitude of starting waypoint in degrees (47.1234567, not 471234567)
 * @param lon_start longitude of starting waypoint in degrees (8.1234567, not 81234567)
 * @param bearing in rad
 * @param distance in meters
 * @param lat_target latitude of target waypoint in degrees (47.1234567, not 471234567)
 * @param lon_target longitude of target waypoint in degrees (47.1234567, not 471234567)
 */
void waypoint_from_heading_and_distance(double lat_start, double lon_start, float bearing, float dist,
        double *lat_target, double *lon_target)
{
    bearing = _wrap_2pi(bearing);
    double radius_ratio = (double)(fabs(dist) / CONSTANTS_RADIUS_OF_EARTH_VEHICLE);

    double lat_start_rad = lat_start * M_DEG_TO_RAD_VEHICLE;
    double lon_start_rad = lon_start * M_DEG_TO_RAD_VEHICLE;

    *lat_target = asin(sin(lat_start_rad) * cos(radius_ratio) + cos(lat_start_rad) * sin(radius_ratio) * cos((
                   double)bearing));
    *lon_target = lon_start_rad + atan2(sin((double)bearing) * sin(radius_ratio) * cos(lat_start_rad),
                        cos(radius_ratio) - sin(lat_start_rad) * sin(*lat_target));

    *lat_target *= M_RAD_TO_DEG_VEHICLE;
    *lon_target *= M_RAD_TO_DEG_VEHICLE;
}

/**
 * Creates a new waypoint C on the line of two given waypoints (A, B) at certain distance
 * from waypoint A
 *
 * @param lat_A waypoint A latitude in degrees (47.1234567, not 471234567)
 * @param lon_A waypoint A longitude in degrees (8.1234567, not 81234567)
 * @param lat_B waypoint B latitude in degrees (47.1234567, not 471234567)
 * @param lon_B waypoint B longitude in degrees (8.1234567, not 81234567)
 * @param dist distance of target waypoint from waypoint A in meters (can be negative)
 * @param lat_target latitude of target waypoint C in degrees (47.1234567, not 471234567)
 * @param lon_target longitude of target waypoint C in degrees (47.1234567, not 471234567)
 */
void create_waypoint_from_line_and_dist(double lat_A, double lon_A, double lat_B, double lon_B, float dist,
                                        double *lat_target, double *lon_target)

{
    if (fabsf(dist) < FLT_EPSILON) {
        *lat_target = lat_A;
        *lon_target = lon_A;

    } else if (dist >= FLT_EPSILON) {
        float heading = get_bearing_to_next_waypoint(lat_A, lon_A, lat_B, lon_B);
        waypoint_from_heading_and_distance(lat_A, lon_A, heading, dist, lat_target, lon_target);

    } else {
        float heading = get_bearing_to_next_waypoint(lat_A, lon_A, lat_B, lon_B);
        heading = _wrap_2pi(heading + M_PI_F_VEHICLE);
        waypoint_from_heading_and_distance(lat_A, lon_A, heading, dist, lat_target, lon_target);
    }
}
/// < end


SurveyMissionItem::SurveyMissionItem(Vehicle* vehicle, QObject* parent)
    : ComplexMissionItem(vehicle, parent)
    , _sequenceNumber(0)
    ,_pathList(new QmlObjectListModel(this))
    , _dirty(false)
    , _cameraTrigger(true)
    , _hideCameraItems(false)
    , _hideturnaround(false)
    , _returntolaunch(true)
    , _gridAltitudeRelative(true)
    , _manualGrid(true)
    , _cameraOrientationLandscape(true)
    , _fixedValueIsAltitude(false)
    , _surveyDistance(0.0)
    , _cameraShots(0)
    , _coveredArea(0.0)
    , _gridAltitudeFact             (0, tr("altitude"),                     FactMetaData::valueTypeDouble)
    , _gridChangeSpeedFact          (0, tr("speed"),                        FactMetaData::valueTypeDouble)
    , _gridAngleFact                (0, tr("angle"),                        FactMetaData::valueTypeDouble)
    , _gridSpacingFact              (0, tr("spacing"),                      FactMetaData::valueTypeDouble)
    , _turnaroundDistFact           (0, tr("turnAroundDistance"),           FactMetaData::valueTypeDouble)
    , _cameraTriggerDistanceFact    (0, _cameraTriggerDistanceFactName,     FactMetaData::valueTypeDouble)
    , _groundResolutionFact         (0, _groundResolutionFactName,          FactMetaData::valueTypeDouble)
    , _frontalOverlapFact           (0, _frontalOverlapFactName,            FactMetaData::valueTypeDouble)
    , _sideOverlapFact              (0, _sideOverlapFactName,               FactMetaData::valueTypeDouble)
    , _cameraSensorWidthFact        (0, _cameraSensorWidthFactName,         FactMetaData::valueTypeDouble)
    , _cameraSensorHeightFact       (0, _cameraSensorHeightFactName,        FactMetaData::valueTypeDouble)
    , _cameraResolutionWidthFact    (0, _cameraResolutionWidthFactName,     FactMetaData::valueTypeUint32)
    , _cameraResolutionHeightFact   (0, _cameraResolutionHeightFactName,    FactMetaData::valueTypeUint32)
    , _cameraFocalLengthFact        (0, tr("Focal length"),                 FactMetaData::valueTypeDouble)
    , _gridSpacingFactAdd1          (0, tr("Architecture(%)"),              FactMetaData::valueTypeDouble)
    , _changeSpeedMetaData          (FactMetaData::valueTypeDouble)
{
    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Survey.FactMetaData.json"), NULL /* metaDataParent */);
    }

    _gridAltitudeFact.setRawValue(50);
    _gridChangeSpeedFact.setRawValue((_vehicle && _vehicle->multiRotor()) ? 10 : 16);
    _gridSpacingFactAdd1.setRawValue (0);
    _gridAngleFact.setRawValue(90);
    _gridSpacingFact.setRawValue(30);
    _turnaroundDistFact.setRawValue((_vehicle && _vehicle->multiRotor()) ? 0 : 30);
    _cameraTriggerDistanceFact.setRawValue(25);
    _groundResolutionFact.setRawValue(3);
    _frontalOverlapFact.setRawValue(10);
    _sideOverlapFact.setRawValue(10);

    _changeSpeedMetaData.setRawUnits("m/s");
	
    _cameraSensorWidthFact.setRawValue(6.17);
    _cameraSensorHeightFact.setRawValue(4.55);
    _cameraResolutionWidthFact.setRawValue(4000);
    _cameraResolutionHeightFact.setRawValue(3000);
    _cameraFocalLengthFact.setRawValue(4.5);

    _gridAltitudeFact.setMetaData(_metaDataMap[_gridAltitudeFactName]);
    _gridAngleFact.setMetaData(_metaDataMap[_gridAngleFactName]);
    _gridSpacingFact.setMetaData(_metaDataMap[_gridSpacingFactName]);
    _turnaroundDistFact.setMetaData(_metaDataMap[_turnaroundDistFactName]);
    _cameraTriggerDistanceFact.setMetaData(_metaDataMap[_cameraTriggerDistanceFactName]);
    _groundResolutionFact.setMetaData(_metaDataMap[_groundResolutionFactName]);
    _frontalOverlapFact.setMetaData(_metaDataMap[_frontalOverlapFactName]);
    _sideOverlapFact.setMetaData(_metaDataMap[_sideOverlapFactName]);
    _cameraSensorWidthFact.setMetaData(_metaDataMap[_cameraSensorWidthFactName]);
    _cameraSensorHeightFact.setMetaData(_metaDataMap[_cameraSensorHeightFactName]);
    _cameraResolutionWidthFact.setMetaData(_metaDataMap[_cameraResolutionWidthFactName]);
    _cameraResolutionHeightFact.setMetaData(_metaDataMap[_cameraResolutionHeightFactName]);
    _cameraFocalLengthFact.setMetaData(_metaDataMap[_cameraFocalLengthFactName]);

    connect(&_gridSpacingFactAdd1,          SIGNAL(valueChanged(QVariant)), this, SLOT(_generateGrid(void)));
    //===============================================
    connect(&_gridSpacingFact,              SIGNAL(valueChanged(QVariant)), this, SLOT(_generateGrid(void)));
    connect(&_gridAngleFact,                SIGNAL(valueChanged(QVariant)), this, SLOT(_generateGrid(void)));
    connect(&_turnaroundDistFact,           SIGNAL(valueChanged(QVariant)), this, SLOT(_generateGrid(void)));
    connect(&_cameraTriggerDistanceFact,    SIGNAL(valueChanged(QVariant)), this, SLOT(_generateGrid(void)));
    connect(&_gridAltitudeFact,             SIGNAL(valueChanged(QVariant)), this, SLOT(_updateCoordinateAltitude(void)));

    // Signal to Qml when camera value changes to it can recalc
    connect(&_groundResolutionFact,         &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_frontalOverlapFact,           &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_sideOverlapFact,              &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_cameraSensorWidthFact,        &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_cameraSensorHeightFact,       &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_cameraResolutionWidthFact,    &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_cameraResolutionHeightFact,   &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_cameraFocalLengthFact,        &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);

    connect(this, &SurveyMissionItem::cameraTriggerChanged, this, &SurveyMissionItem::_cameraTriggerChanged);
    connect(this, &SurveyMissionItem::hideturnaroundChanged, this, &SurveyMissionItem::_generateGrid);
}
const SurveyMissionItem& SurveyMissionItem::operator=(const SurveyMissionItem& other)
{
    ComplexMissionItem::operator=(other);

    _setSurveyDistance(other._surveyDistance);
    _setCameraShots(other._cameraShots);
    _setCoveredArea(other._coveredArea);

    return *this;
}
void SurveyMissionItem::_setSurveyDistance(double surveyDistance)
{
    if (!qFuzzyCompare(_surveyDistance, surveyDistance)) {
        _surveyDistance = surveyDistance;
        emit complexDistanceChanged(_surveyDistance);
    }
}

void SurveyMissionItem::_setCameraShots(int cameraShots)
{
    if (_cameraShots != cameraShots) {
        _cameraShots = cameraShots;
        emit cameraShotsChanged(this->cameraShots());
    }
}

void SurveyMissionItem::_setCoveredArea(double coveredArea)
{
    if (!qFuzzyCompare(_coveredArea, coveredArea)) {
        _coveredArea = coveredArea;
        emit coveredAreaChanged(_coveredArea);
    }
}


void SurveyMissionItem::clearPolygon(void)
{
    // Bug workaround, see below
    while (_polygonPath.count() > 1) {
        _polygonPath.takeLast();
    }
    emit polygonPathChanged();

    // Although this code should remove the polygon from the map it doesn't. There appears
    // to be a bug in MapPolygon which causes it to not be redrawn if the list is empty. So
    // we work around it by using the code above to remove all but the last point which in turn
    // will cause the polygon to go away.
    _polygonPath.clear();
    emit polygonPathChanged();

    _clearGrid();
    setDirty(true);

    emit specifiesCoordinateChanged();
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

void SurveyMissionItem::addPolygonCoordinate(const QGeoCoordinate coordinate)
{
    _polygonPath << QVariant::fromValue(coordinate);
    emit polygonPathChanged();

    int pointCount = _polygonPath.count();
    if (pointCount >= 3) {
        if (pointCount == 3) {
            emit specifiesCoordinateChanged();
        }
        _generateGrid();
    }
    setDirty(true);
}

void SurveyMissionItem::adjustPolygonCoordinate(int vertexIndex, const QGeoCoordinate coordinate)
{
    _polygonPath[vertexIndex] = QVariant::fromValue(coordinate);
    emit polygonPathChanged();
    _generateGrid();
    setDirty(true);
}
QmlObjectListModel* SurveyMissionItem::returnList(){
_pathList->clear();
Vehicle* vehicle=NULL;
    for(int i =0;i<_polygonPath.count();i++){
        const QGeoCoordinate coordinate = _polygonPath[i].value<QGeoCoordinate>();
        SimpleMissionItem * newItem = new SimpleMissionItem(vehicle, this);
        newItem->setCoordinate(coordinate);
        newItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT);
        newItem->setSequenceNumber(i);
        newItem->setDefaultsForCommand();
        _pathList->append(newItem);
    }
    return _pathList;
}

int SurveyMissionItem::lastSequenceNumber(void) const
{
    int lastSeq = _sequenceNumber;

    if (_gridPoints.count()) {
        lastSeq += _gridPoints.count() - 1;
        if (_cameraTrigger) {
            // Account for two trigger messages
            lastSeq += 2;
        }
    }

    return lastSeq;
}

void SurveyMissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (_coordinate != coordinate) {
        _coordinate = coordinate;
        emit coordinateChanged(_coordinate);
    }
}

void SurveyMissionItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void SurveyMissionItem::save(QJsonObject& saveObject) const
{
    saveObject[JsonHelper::jsonVersionKey] =    2;
    saveObject[_jsonTypeKey] =                  _complexType;
    saveObject[_jsonIdKey] =                    sequenceNumber();
    saveObject[_jsonCameraTriggerKey] =         _cameraTrigger;
    saveObject[_jsonManualGridKey] =            _manualGrid;
    saveObject[_jsonFixedValueIsAltitudeKey] =  _fixedValueIsAltitude;

    if (_cameraTrigger) {
        saveObject[_jsonCameraTriggerDistanceKey] = _cameraTriggerDistanceFact.rawValue().toDouble();
    }

    QJsonObject gridObject;
    gridObject[_jsonGridAltitudeKey] =          _gridAltitudeFact.rawValue().toDouble();
    gridObject[_jsonGridAltitudeRelativeKey] =  _gridAltitudeRelative;
    gridObject[_jsonGridAngleKey] =             _gridAngleFact.rawValue().toDouble();
    gridObject[_jsonGridSpacingKey] =           _gridSpacingFact.rawValue().toDouble();
    gridObject[_jsonTurnaroundDistKey] =        _turnaroundDistFact.rawValue().toDouble();
    gridObject[_jsonGridSpacingAdd1Key] =    _gridSpacingFactAdd1.rawValue().toDouble();

    saveObject[_jsonGridObjectKey] = gridObject;

    if (!_manualGrid) {
        QJsonObject cameraObject;
        cameraObject[_jsonCameraNameKey] =                  _camera;
        cameraObject[_jsonCameraOrientationLandscapeKey] =  _cameraOrientationLandscape;
        cameraObject[_jsonCameraSensorWidthKey] =           _cameraSensorWidthFact.rawValue().toDouble();
        cameraObject[_jsonCameraSensorHeightKey] =          _cameraSensorHeightFact.rawValue().toDouble();
        cameraObject[_jsonCameraResolutionWidthKey] =       _cameraResolutionWidthFact.rawValue().toDouble();
        cameraObject[_jsonCameraResolutionHeightKey] =      _cameraResolutionHeightFact.rawValue().toDouble();
        cameraObject[_jsonCameraFocalLengthKey] =           _cameraFocalLengthFact.rawValue().toDouble();
        cameraObject[_jsonGroundResolutionKey] =            _groundResolutionFact.rawValue().toDouble();
        cameraObject[_jsonFrontalOverlapKey] =              _frontalOverlapFact.rawValue().toInt();
        cameraObject[_jsonSideOverlapKey] =                 _sideOverlapFact.rawValue().toInt();

        saveObject[_jsonCameraObjectKey] = cameraObject;
    }

    // Polygon shape

    QJsonArray polygonArray;

    for (int i=0; i<_polygonPath.count(); i++) {
        const QVariant& polyVar = _polygonPath[i];

        QJsonValue jsonValue;
        JsonHelper::saveGeoCoordinate(polyVar.value<QGeoCoordinate>(), false /* writeAltitude */, jsonValue);
        polygonArray += jsonValue;
    }

    saveObject[_jsonPolygonObjectKey] = polygonArray;
}

void SurveyMissionItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}
//===============================================================
void SurveyMissionItem::setFlage(bool flage)
{
    if(_flage!=flage){
        _flage=flage;
        emit flageChanged(_flage);
    }
}
//====================================================================
void SurveyMissionItem::_clear(void)
{
    clearPolygon();
    _clearGrid();
}

/* Changed by chu.fumin 2016122711 start : dms degree,minute,second convert */
bool SurveyMissionItem::loadDMS(const QVariantList &polygonPath, unsigned int seq){
    if(!polygonPath.count ()){
        return false;
    }

    _clear();
    setSequenceNumber(seq);

    _manualGrid = true;

    _polygonPath.clear ();
    // Polygon shape
    for (int i = 0; i < polygonPath.count (); i++) {
        _polygonPath << polygonPath.at (i);
    }

    _generateGrid();
    return true;
}
//===============================================================
//edit by wang.lichen
bool SurveyMissionItem::loadKMLPolygon(const QVariantList &polygonPath, unsigned int seq){
    if(!polygonPath.count ()){
        return false;
    }
    _clear();
    setSequenceNumber(seq);
    _gisFlag.clear ();
    // Polygon shape
    for (int i = 0; i < polygonPath.count (); i++) {
        _gisFlag << polygonPath.at (i);
    }

    if (polygonPath.count()) {
        QGeoCoordinate coordinate = polygonPath.at(0).value<QGeoCoordinate>();
        setCoordinate(coordinate);
    }
    return true;
}

bool SurveyMissionItem::loadKMLLineString(const QVariantList &lineString, unsigned int seq)
{
    if(!lineString.count ()){
        return false;
    }
    _clear();
    setSequenceNumber(seq);
    _linePoints.clear ();
    // Polygon shape
    for (int i = 0; i < lineString.count (); i++) {
        _linePoints << lineString.at (i);
    }

    if (lineString.count()) {
        QGeoCoordinate coordinate = lineString.at(0).value<QGeoCoordinate>();
        setCoordinate(coordinate);
    }
    return true;
}
//==========================================================
bool SurveyMissionItem::load(const QJsonObject& complexObject, QString& errorString)
{
    struct jsonKeyInfo_s {
        const char*         key;
        QJsonValue::Type    type;
        bool                required;
    };

    QList<JsonHelper::KeyValidateInfo> mainKeyInfoList = {
        { JsonHelper::jsonVersionKey,           QJsonValue::Double, true },
        { _jsonTypeKey,                         QJsonValue::String, true },
        { _jsonPolygonObjectKey,                QJsonValue::Array,  true },
        { _jsonIdKey,                           QJsonValue::Double, true },
        { _jsonGridObjectKey,                   QJsonValue::Object, true },
        { _jsonCameraObjectKey,                 QJsonValue::Object, false },
        { _jsonCameraTriggerKey,                QJsonValue::Bool,   true },
        { _jsonCameraTriggerDistanceKey,        QJsonValue::Double, false },
        { _jsonManualGridKey,                   QJsonValue::Bool,   true },
        { _jsonFixedValueIsAltitudeKey,         QJsonValue::Bool,   true },
    };

    QList<JsonHelper::KeyValidateInfo> gridKeyInfoList = {
        { _jsonGridAltitudeKey,                 QJsonValue::Double, true },
        { _jsonGridAltitudeRelativeKey,         QJsonValue::Bool,   true },
        { _jsonGridAngleKey,                    QJsonValue::Double, true },
        { _jsonGridSpacingKey,                  QJsonValue::Double, true },
        { _jsonTurnaroundDistKey,               QJsonValue::Double, true },
        {_jsonGridSpacingAdd1Key,               QJsonValue::Double,true},
    };

    QList<JsonHelper::KeyValidateInfo> cameraKeyInfoList = {
        { _jsonGroundResolutionKey,             QJsonValue::Double, true },
        { _jsonFrontalOverlapKey,               QJsonValue::Double, true },
        { _jsonSideOverlapKey,                  QJsonValue::Double, true },
        { _jsonCameraSensorWidthKey,            QJsonValue::Double, true },
        { _jsonCameraSensorHeightKey,           QJsonValue::Double, true },
        { _jsonCameraResolutionWidthKey,        QJsonValue::Double, true },
        { _jsonCameraResolutionHeightKey,       QJsonValue::Double, true },
        { _jsonCameraFocalLengthKey,            QJsonValue::Double, true },
        { _jsonCameraNameKey,                   QJsonValue::String, true },
        { _jsonCameraOrientationLandscapeKey,   QJsonValue::Bool,   true },
    };

    if (!JsonHelper::validateKeys(complexObject, mainKeyInfoList, errorString)) {
        return false;
    }
    if (!JsonHelper::validateKeys(complexObject[_jsonGridObjectKey].toObject(), gridKeyInfoList, errorString)) {
        return false;
    }

    // Version check
    if (complexObject[JsonHelper::jsonVersionKey].toInt() != 2) {
        errorString = tr("QGroundControl does not support this version of survey items");
        _clear();
        return false;
    }
    QString complexType = complexObject[_jsonTypeKey].toString();
    if (complexType != _complexType) {
        errorString = tr("QGroundControl does not support loading this complex mission item type: %1").arg(complexType);
        _clear();
        return false;
    }

    _clear();

    setSequenceNumber(complexObject[_jsonIdKey].toInt());

    _manualGrid =           complexObject[_jsonManualGridKey].toBool(true);
    _cameraTrigger =        complexObject[_jsonCameraTriggerKey].toBool(false);
    _fixedValueIsAltitude = complexObject[_jsonFixedValueIsAltitudeKey].toBool(true);
    _gridAltitudeRelative = complexObject[_jsonGridAltitudeRelativeKey].toBool(true);

    QJsonObject gridObject = complexObject[_jsonGridObjectKey].toObject();

    _gridAltitudeFact.setRawValue   (gridObject[_jsonGridAltitudeKey].toDouble());
    _gridAngleFact.setRawValue      (gridObject[_jsonGridAngleKey].toDouble());
    _gridSpacingFact.setRawValue    (gridObject[_jsonGridSpacingKey].toDouble());
    _gridSpacingFactAdd1.setRawValue(gridObject[_jsonGridSpacingAdd1Key].toDouble());
    _turnaroundDistFact.setRawValue (gridObject[_jsonTurnaroundDistKey].toDouble());

    if (_cameraTrigger) {
        if (!complexObject.contains(_jsonCameraTriggerDistanceKey)) {
            errorString = tr("%1 but %2 is missing").arg("cameraTrigger = true").arg("cameraTriggerDistance");
            return false;
        }
        _cameraTriggerDistanceFact.setRawValue(complexObject[_jsonCameraTriggerDistanceKey].toDouble());
    }

    if (!_manualGrid) {
        if (!complexObject.contains(_jsonCameraObjectKey)) {
            errorString = tr("%1 but %2 object is missing").arg("manualGrid = false").arg("camera");
            return false;
        }

        QJsonObject cameraObject = complexObject[_jsonCameraObjectKey].toObject();

        if (!JsonHelper::validateKeys(cameraObject, cameraKeyInfoList, errorString)) {
            return false;
        }

        _camera =                       cameraObject[_jsonCameraNameKey].toString();
        _cameraOrientationLandscape =   cameraObject[_jsonCameraOrientationLandscapeKey].toBool(true);

        _groundResolutionFact.setRawValue       (cameraObject[_jsonGroundResolutionKey].toDouble());
        _frontalOverlapFact.setRawValue         (cameraObject[_jsonFrontalOverlapKey].toInt());
        _sideOverlapFact.setRawValue            (cameraObject[_jsonSideOverlapKey].toInt());
        _cameraSensorWidthFact.setRawValue      (cameraObject[_jsonCameraSensorWidthKey].toDouble());
        _cameraSensorHeightFact.setRawValue     (cameraObject[_jsonCameraSensorHeightKey].toDouble());
        _cameraResolutionWidthFact.setRawValue  (cameraObject[_jsonCameraResolutionWidthKey].toDouble());
        _cameraResolutionHeightFact.setRawValue (cameraObject[_jsonCameraResolutionHeightKey].toDouble());
        _cameraFocalLengthFact.setRawValue      (cameraObject[_jsonCameraFocalLengthKey].toDouble());
    }

    // Polygon shape
    QJsonArray polygonArray(complexObject[_jsonPolygonObjectKey].toArray());
    for (int i=0; i<polygonArray.count(); i++) {
        const QJsonValue& pointValue = polygonArray[i];

        QGeoCoordinate pointCoord;
        if (!JsonHelper::loadGeoCoordinate(pointValue, false /* altitudeRequired */, pointCoord, errorString)) {
            _clear();
            return false;
        }
        _polygonPath << QVariant::fromValue(pointCoord);
    }

    _generateGrid();

    return true;
}

double SurveyMissionItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    double greatestDistance = 0.0;
    for (int i=0; i<_gridPoints.count(); i++) {
        QGeoCoordinate currentCoord = _gridPoints[i].value<QGeoCoordinate>();
        double distance = currentCoord.distanceTo(other);
        if (distance > greatestDistance) {
            greatestDistance = distance;
        }
    }
    return greatestDistance;
}

void SurveyMissionItem::_setExitCoordinate(const QGeoCoordinate& coordinate)
{
    if (_exitCoordinate != coordinate) {
        _exitCoordinate = coordinate;
        emit exitCoordinateChanged(coordinate);
    }
}

bool SurveyMissionItem::specifiesCoordinate(void) const
{
    return _polygonPath.count() > 2;
}

void SurveyMissionItem::_clearGrid(void)
{
    // Bug workaround
    while (_gridPoints.count() > 1) {
        _gridPoints.takeLast();
    }
    emit gridPointsChanged();

    _gridPoints.clear();
    emit gridPointsChanged();
}
void SurveyMissionItem::_generateGrid(void){
    _updateGenerateGrid(false);
}
void SurveyMissionItem::_updateGenerateGrid(bool update)
{
    if (_polygonPath.count() < 3 || _gridSpacingFact.rawValue().toDouble() <= 0) {
        _clearGrid();
        return;
    }

    _gridPoints.clear();
    emit gridPointsChanged();

    QList<QPointF> polygonPoints;
    QList<QPointF> gridPoints;

    // Convert polygon to Qt coordinate system (y positive is down)
    qCDebug(SurveyMissionItemLog) << "Convert polygon";
    QGeoCoordinate tangentOrigin = _polygonPath[0].value<QGeoCoordinate>();
    for (int i=0; i<_polygonPath.count(); i++) {
        double y, x, down;
        convertGeoToNed(_polygonPath[i].value<QGeoCoordinate>(), tangentOrigin, &y, &x, &down);
        polygonPoints += QPointF(x, -y);
        //qCDebug(ComplexMissionItemLog) << _polygonPath[i].value<QGeoCoordinate>() << polygonPoints.last().x() << polygonPoints.last().y();
    }

    double coveredArea = 0.0;
    for (int i=0; i<polygonPoints.count(); i++) {
        if (i != 0) {
            coveredArea += polygonPoints[i - 1].x() * polygonPoints[i].y() - polygonPoints[i].x() * polygonPoints[i -1].y();
        } else {
            coveredArea += polygonPoints.last().x() * polygonPoints[i].y() - polygonPoints[i].x() * polygonPoints.last().y();
        }
    }
    _setCoveredArea(0.5 * fabs(coveredArea));


    _cameraTriggerPointLocal.clear ();
    // Generate grid
    _gridGenerator(polygonPoints, gridPoints, tangentOrigin, update);

    emit gridPointsChanged();
    emit lastSequenceNumberChanged(lastSequenceNumber());

    if (_gridPoints.count()) {
        QGeoCoordinate coordinate = _gridPoints.first().value<QGeoCoordinate>();
        coordinate.setAltitude(_gridAltitudeFact.rawValue().toDouble());
        setCoordinate(coordinate);
        QGeoCoordinate exitCoordinate = _gridPoints.last().value<QGeoCoordinate>();
        exitCoordinate.setAltitude(_gridAltitudeFact.rawValue().toDouble());
        _setExitCoordinate(exitCoordinate);
		//edit by wang.lichen
        if(_turnaroundDistFact.rawValue().toDouble() > 0.0){
            if(_gridSpacingFactAdd1.rawValue().toDouble()!=0){
                if(_gridPoints.count() > 10){
                    _setExitCoordinate(_gridPoints[_gridPoints.count()-10].value<QGeoCoordinate>());
                }
            }else{
                if(_gridPoints.count() > 2){
                    _setExitCoordinate(_gridPoints[_gridPoints.count()-2].value<QGeoCoordinate>());
                }
            }
        }else{
            if(_gridSpacingFactAdd1.rawValue().toDouble()!=0){
                if(_gridPoints.count() > 5){
                    _setExitCoordinate(_gridPoints[_gridPoints.count()-5].value<QGeoCoordinate>());
                }
            }
            else{
                if(_gridPoints.count() > 1){
                    _setExitCoordinate(_gridPoints[_gridPoints.count()-1].value<QGeoCoordinate>());
                }
            }
        }
    }
}

void SurveyMissionItem::_updateCoordinateAltitude(void)
{
    _coordinate.setAltitude(_gridAltitudeFact.rawValue().toDouble());
    _exitCoordinate.setAltitude(_gridAltitudeFact.rawValue().toDouble());
    emit coordinateChanged(_coordinate);
    emit exitCoordinateChanged(_exitCoordinate);
}

QPointF SurveyMissionItem::_rotatePoint(const QPointF& point, const QPointF& origin, double angle)
{
    QPointF rotated;
    double radians = (M_PI / 180.0) * angle;

    rotated.setX(((point.x() - origin.x()) * cos(radians)) - ((point.y() - origin.y()) * sin(radians)) + origin.x());
    rotated.setY(((point.x() - origin.x()) * sin(radians)) + ((point.y() - origin.y()) * cos(radians)) + origin.y());

    return rotated;
}

void SurveyMissionItem::_intersectLinesWithRect(const QList<QLineF>& lineList, const QRectF& boundRect, QList<QLineF>& resultLines)
{
    QLineF topLine      (boundRect.topLeft(),       boundRect.topRight());
    QLineF bottomLine   (boundRect.bottomLeft(),    boundRect.bottomRight());
    QLineF leftLine     (boundRect.topLeft(),       boundRect.bottomLeft());
    QLineF rightLine    (boundRect.topRight(),      boundRect.bottomRight());

    for (int i=0; i<lineList.count(); i++) {
        QPointF intersectPoint;
        QLineF intersectLine;
        const QLineF& line = lineList[i];

        int foundCount = 0;
        if (line.intersect(topLine, &intersectPoint) == QLineF::BoundedIntersection) {
            intersectLine.setP1(intersectPoint);
            foundCount++;
        }
        if (line.intersect(rightLine, &intersectPoint) == QLineF::BoundedIntersection) {
            if (foundCount == 0) {
                intersectLine.setP1(intersectPoint);
            } else {
                if (foundCount != 1) {
                    qWarning() << "Found more than two intersecting points";
                }
                intersectLine.setP2(intersectPoint);
            }
            foundCount++;
        }
        if (line.intersect(bottomLine, &intersectPoint) == QLineF::BoundedIntersection) {
            if (foundCount == 0) {
                intersectLine.setP1(intersectPoint);
            } else {
                if (foundCount != 1) {
                    qWarning() << "Found more than two intersecting points";
                }
                intersectLine.setP2(intersectPoint);
            }
            foundCount++;
        }
        if (line.intersect(leftLine, &intersectPoint) == QLineF::BoundedIntersection) {
            if (foundCount == 0) {
                intersectLine.setP1(intersectPoint);
            } else {
                if (foundCount != 1) {
                    qWarning() << "Found more than two intersecting points";
                }
                intersectLine.setP2(intersectPoint);
            }
            foundCount++;
        }

        if (foundCount == 2) {
            resultLines += intersectLine;
        }
    }
}

void SurveyMissionItem::_intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon, QList<QLineF>& resultLines)
{
    resultLines.clear ();
    for (int i=0; i<lineList.count(); i++) {
        int foundCount = 0;
        QLineF intersectLine;
        const QLineF& line = lineList[i];

        for (int j=0; j<polygon.count()-1; j++) {
            QPointF intersectPoint;
            QLineF polygonLine = QLineF(polygon[j], polygon[j+1]);
            if (line.intersect(polygonLine, &intersectPoint) == QLineF::BoundedIntersection) {
                if (foundCount == 0) {
                    foundCount++;
                    intersectLine.setP1(intersectPoint);
                } else {
                    foundCount++;
                    intersectLine.setP2(intersectPoint);
                    break;
                }
            }
        }

        if (foundCount == 2) {
            resultLines += intersectLine;
        }
    }
}

/// Adjust the line segments such that they are all going the same direction with respect to going from P1->P2
void SurveyMissionItem::_adjustLineDirection(const QList<QLineF>& lineList, QList<QLineF>& resultLines)
{
    qreal firstAngle = 0;
    for(int i = 0; i < lineList.count (); i++){
        const QLineF& line = lineList[i];
        QLineF adjustedLine;
        if(i == 0){
            firstAngle = line.angle ();
        }

        if (qAbs(line.angle() - firstAngle) > 1.0) {
            adjustedLine.setP1(line.p2());
            adjustedLine.setP2(line.p1());
        } else {
            adjustedLine = line;
        }

        resultLines += adjustedLine;
    }
}

void SurveyMissionItem::_gridGenerator(const QList<QPointF>& polygonPoints,  QList<QPointF>& gridPoints, QGeoCoordinate &tangentOrigin, bool update)
{
    double gridAngle = _gridAngleFact.rawValue().toDouble();
    double gridSpacing = _gridSpacingFact.rawValue().toDouble();

    //qCDebug(SurveyMissionItemLog) << "SurveyMissionItem::_gridGenerator gridSpacing:gridAngle" << gridSpacing << gridAngle;
    gridPoints.clear();

    // Convert polygon to bounding rect

    QPolygonF polygon;
    for (int i=0; i<polygonPoints.count(); i++) {
        polygon << polygonPoints[i];
    }
    polygon << polygonPoints[0];
    QRectF smallBoundRect = polygon.boundingRect();
    QPointF center = smallBoundRect.center();

    // Rotate the bounding rect around it's center to generate the larger bounding rect
    QPolygonF boundPolygon;
    boundPolygon << _rotatePoint(smallBoundRect.topLeft(),       center, gridAngle);
    boundPolygon << _rotatePoint(smallBoundRect.topRight(),      center, gridAngle);
    boundPolygon << _rotatePoint(smallBoundRect.bottomRight(),   center, gridAngle);
    boundPolygon << _rotatePoint(smallBoundRect.bottomLeft(),    center, gridAngle);
    boundPolygon << boundPolygon[0];
    QRectF largeBoundRect = boundPolygon.boundingRect();

    // Create set of rotated parallel lines within the expanded bounding rect. Make the lines larger than the
    // bounding box to guarantee intersection.
    QList<QLineF> lineList;
    //====================
    //edit by wang.lichen
    QList<QLineF> lineListAdd;
    //===========================
    float x = largeBoundRect.topLeft().x() - (gridSpacing / 2);
    //edit by wang.lichen
    float gridSpacingAdd1 = _gridSpacingFactAdd1.rawValue().toDouble();
    float offset1 = (largeBoundRect.bottomRight().x()-largeBoundRect.topLeft().x())*0.5+(largeBoundRect.bottomRight().x()-largeBoundRect.topLeft().x())*gridSpacingAdd1/100;
    float offset2 = (largeBoundRect.bottomRight().x()-largeBoundRect.topLeft().x())*0.5-(largeBoundRect.bottomRight().x()-largeBoundRect.topLeft().x())*gridSpacingAdd1/100;
    if(gridSpacingAdd1==0){
        offset1=100000000;
        offset2=100000000;
    }
    //==================================================================
    while (x < largeBoundRect.bottomRight().x()) {
            float yTop =    largeBoundRect.topLeft().y() - 100.0;
            float yBottom = largeBoundRect.bottomRight().y() + 100.0;

            lineList += QLineF(_rotatePoint(QPointF(x, yTop), center, gridAngle), _rotatePoint(QPointF(x, yBottom), center, gridAngle));
            // qCDebug(SurveyMissionItemLog) << "line(" << lineList.last().x1() << ", " << lineList.last().y1() << ")-(" << lineList.last().x2() <<", " << lineList.last().y2() << ")";

            x += gridSpacing;
    }
    //=======================================================
    //edit by wang.lichen
    float yTop =    largeBoundRect.topLeft().y() - 100000;
    float yBottom = largeBoundRect.bottomRight().y() + 10000.0;
    lineListAdd += QLineF(_rotatePoint(QPointF(largeBoundRect.topLeft().x()+offset1, yTop), center, gridAngle+90), _rotatePoint(QPointF(largeBoundRect.topLeft().x()+offset1, yBottom), center, gridAngle+90));
    lineListAdd += QLineF(_rotatePoint(QPointF(largeBoundRect.topLeft().x()+offset2, yTop), center, gridAngle+90), _rotatePoint(QPointF(largeBoundRect.topLeft().x()+offset2, yBottom), center, gridAngle+90));
    //==========================================================
    // Now intersect the lines with the polygon
    QList<QLineF> intersectLines;
    //======================================================
    //edit by wang.lichen
    QList<QLineF> intersectLinesAdd;
    //======================================================
#if 1
    _intersectLinesWithPolygon(lineList, polygon, intersectLines);
    //edit by wang.lichen
    _intersectLinesWithPolygon(lineListAdd, polygon, intersectLinesAdd);
    //=========================================================
#else
    // This is handy for debugging grid problems, not for release
    intersectLines = lineList;
#endif

    // Make sure all lines are going to same direction. Polygon intersection leads to line which
    // can be in varied directions depending on the order of the intesecting sides.
    QList<QLineF> resultLines;
    _adjustLineDirection(intersectLines, resultLines);
    //===========================================================
    //edit by wang.lichen
    QList<QLineF> resultLinesAdd;
    _adjustLineDirection(intersectLinesAdd, resultLinesAdd); 
    //Camera tirgger path
    double cameraTriggerDist = _cameraTriggerDistanceFact.rawValue().toDouble();
    int sumLines = resultLines.count();
    //==========================================
    // Turn into a path
    double turnaroundDist = _turnaroundDistFact.rawValue().toDouble();

    _cameraTriggerPoint.clear ();
    _cameraTriggerNumber.clear ();
    _cameraTriggerPointLocal.clear ();
    _cameraTriggerPointList.clear ();

    if(!update){
        gridPoints.clear ();
        for (int i=0; i<resultLines.count(); i++) {
            const QLineF& line = resultLines[i];

            QPointF cameraTriggerOffset = line.p2 () - line.p1 ();
            cameraTriggerOffset = cameraTriggerOffset * cameraTriggerDist /sqrt(pow(cameraTriggerOffset.x (),2.0) + pow(cameraTriggerOffset.y (),2));
            QPointF turnaroundOffset = line.p2() - line.p1();
            turnaroundOffset = turnaroundOffset * turnaroundDist / sqrt(pow(turnaroundOffset.x(),2.0) + pow(turnaroundOffset.y(),2.0));

            if (i & 1) {
                if (turnaroundDist > 0.0) {
                    if(_vehicle->vtol()  && _hideturnaround){
                        gridPoints << line.p2() + turnaroundOffset  + 3*cameraTriggerOffset<< line.p2() + 3*cameraTriggerOffset << line.p1() - 3*cameraTriggerOffset;
                    }else{
                         gridPoints << line.p2() + turnaroundOffset  + 3*cameraTriggerOffset<< line.p2() + 3*cameraTriggerOffset<< line.p1() - 3*cameraTriggerOffset<< line.p1() - turnaroundOffset - 3*cameraTriggerOffset;
                    }
                } else {
                    gridPoints << line.p2() + 3 * cameraTriggerOffset <<line.p2() <<line.p1()<<line.p1 () - 3 * cameraTriggerOffset;
                }
            } else {
                if (turnaroundDist > 0.0) {
                    if(_vehicle->vtol()  && _hideturnaround){
                          gridPoints << line.p1() - turnaroundOffset - 3*cameraTriggerOffset<< line.p1() - 3*cameraTriggerOffset<< line.p2() + 3*cameraTriggerOffset ;
                    }else{
                        gridPoints << line.p1() - turnaroundOffset - 3*cameraTriggerOffset<< line.p1() - 3*cameraTriggerOffset<< line.p2() + 3*cameraTriggerOffset<< line.p2() + turnaroundOffset + 3*cameraTriggerOffset;
                    }
                } else {
                    gridPoints << line.p1() - 3*cameraTriggerOffset <<line.p1 ()<<line.p2()<< line.p2() + 3 * cameraTriggerOffset;
                }
            }
        }

        //============================================
        //edit by wang.lichen
        for (int i = 0; i < resultLinesAdd.count(); i++) {
            QPointF addPoint(0,0);
            if (turnaroundDist > 0.0 && sumLines >= 2) {
                if((sumLines - 1) & 1){
                    addPoint = resultLines[ sumLines - 1].p1 ();
                }else{
                    addPoint = resultLines[ sumLines - 1].p2 ();
                }
            }else if(sumLines >= 2){
                if((sumLines - 1) & 1){
                    addPoint = resultLines[ sumLines - 1].p2 ();
                }else{
                    addPoint = resultLines[ sumLines - 1].p1 ();
                }
            }
            QLineF& line = resultLinesAdd[i];
            QPointF pp = line.p1();
            QPointF qq = line.p1();
            qreal x1 = line.p1().x();
            qreal y1 = line.p1().y();
            qreal x2 = line.p2().x();
            qreal y2 = line.p2().y();
            qreal len = sqrt(pow(x2-x1,2)+pow(y2-y1,2)) + 100;
            pp.setX((60*x1-len*x2)/(60-len));
            pp.setY((60*y1-len*y2)/(60-len));
            line.setP1(pp);
            qq.setX((60*x2-len*x1)/(60-len));
            qq.setY((60*y2-len*y1)/(60-len));
            line.setP2(qq);
            qreal xxx;
            qreal yyy;
            qreal xxx1;
            qreal yyy1;
            qreal xxx2;
            qreal yyy2;
            xxx = addPoint.x ();
            yyy = addPoint.y ();
            xxx1 = line.p1().x();
            yyy1 = line.p1().y();
            xxx2 = line.p2().x();
            yyy2 = line.p2().y();
            QPointF turnaroundOffset = line.p2() - line.p1();
            turnaroundOffset = turnaroundOffset * turnaroundDist / sqrt(pow(turnaroundOffset.x(),2.0) + pow(turnaroundOffset.y(),2.0));

            if (i & 1) {
                if (turnaroundDist > 0.0) {
                    if(pow(xxx-xxx1,2)+pow(yyy-yyy1,2)-pow(xxx-xxx2,2)-pow(yyy-yyy2,2) < 0){
                        gridPoints   << line.p2() + turnaroundOffset<< line.p1() << line.p2()<< line.p1() - turnaroundOffset ;
                    }
                    else{
                        gridPoints << line.p1() - turnaroundOffset << line.p2() << line.p1() << line.p2() + turnaroundOffset ;
                    }
                } else {
                    if(pow(xxx-xxx1,2)+pow(yyy-yyy1,2)-pow(xxx-xxx2,2)-pow(yyy-yyy2,2)<0){
                        gridPoints << line.p2() << line.p2() << line.p1() << line.p1();
                    }
                    else{
                        gridPoints << line.p1() << line.p1() << line.p2() << line.p2();
                    }
                }
            } else {
                if (turnaroundDist > 0.0) {
                    if(pow(xxx-xxx1,2)+pow(yyy-yyy1,2)-pow(xxx-xxx2,2)-pow(yyy-yyy2,2)<0){
                        gridPoints << line.p1() - turnaroundOffset << line.p1() << line.p2() << line.p2() + turnaroundOffset;
                    }
                    else{
                        gridPoints << line.p2() + turnaroundOffset << line.p2() << line.p1() << line.p1() - turnaroundOffset;
                    }
                } else {
                    if(pow(xxx-xxx1,2)+pow(yyy-yyy1,2)-pow(xxx-xxx2,2)-pow(yyy-yyy2,2)<0){
                        gridPoints << line.p1() << line.p1() << line.p2() << line.p2();
                    }
                    else{
                        gridPoints << line.p2() << line.p2() << line.p1() << line.p1();
                    }
                }
            }
        }

        double surveyDistance = 0.0;
        // Convert to Geo and set altitude
        for (int i=0; i<gridPoints.count(); i++) {
            if (i != 0) {
                surveyDistance += sqrt(pow((gridPoints[i] - gridPoints[i - 1]).x(),2.0) + pow((gridPoints[i] - gridPoints[i - 1]).y(),2.0));
            }
            QPointF& point = gridPoints[i];
            QGeoCoordinate geoCoord;
            convertNedToGeo(-point.y(), point.x(), 0, tangentOrigin, &geoCoord);
            _gridPoints += QVariant::fromValue(geoCoord);
        }
        _setSurveyDistance(surveyDistance);
        return ;
    }
    bool firstIntersectionPoint = false;
    QLineF linenormalVector;
    if(sumLines){
        linenormalVector = resultLines[0];
    }
    for (int i = 0; i < sumLines; i++) {
        const QLineF& line = resultLines[i];

        QPointF turnaroundOffset = line.p2() - line.p1();
        turnaroundOffset = turnaroundOffset * turnaroundDist / sqrt(pow(turnaroundOffset.x(), 2.0) + pow(turnaroundOffset.y(), 2.0));

        QPointF cameraTriggerOffset = line.p2() - line.p1();
        cameraTriggerOffset = cameraTriggerOffset * _cameraTriggerDistanceFact.rawValue().toDouble() / sqrt(pow(cameraTriggerOffset.x(), 2.0) + pow(cameraTriggerOffset.y(), 2.0));

        QVariantList cameraTriggerGeoLocal;
        cameraTriggerGeoLocal.clear ();
        if(_cameraTrigger && _cameraTriggerDistanceFact.rawValue().toDouble() > 0.0 && update){
            //next line
            QLineF line0 = resultLines[0];
            QLineF  startLine = line0.normalVector ();
            if(firstIntersectionPoint){
                startLine = linenormalVector.normalVector ();
            }
            QPointF intersectionPoint;
            QLineF::IntersectType result = resultLines[i].intersect (startLine, &intersectionPoint);

            QGeoCoordinate geoCoord1, geoCoord2;
            double bearing = 0;
            convertNedToGeo(-(line.p1() - 3*cameraTriggerOffset).y(), (line.p1() - 3*cameraTriggerOffset).x(), 0, tangentOrigin, &geoCoord1);
            convertNedToGeo(-(line.p2() + 3*cameraTriggerOffset).y(), (line.p2() + 3*cameraTriggerOffset).x(), 0, tangentOrigin, &geoCoord2);
            //double distP1P2 = get_distance_to_next_waypoint(geoCoord1.latitude (), geoCoord1.longitude (), geoCoord2.latitude (), geoCoord2.longitude ());
            double distP1P2 = sqrt(pow((line.p2()+6*cameraTriggerOffset - line.p1()).x(), 2.0) + pow((line.p2()  + 6*cameraTriggerOffset- line.p1()).y(), 2.0));
            int numCam = distP1P2 / sqrt(pow(cameraTriggerOffset.x(), 2.0) + pow(cameraTriggerOffset.y(), 2.0));

            if(!firstIntersectionPoint){
                intersectionPoint = line.p1() - 3*cameraTriggerOffset;
                bearing = get_bearing_to_next_waypoint(geoCoord1.latitude (), geoCoord1.longitude (), geoCoord2.latitude (), geoCoord2.longitude ());
                if(numCam){
                    numCam += 1;
                }
                for( int num = 0; num < numCam; num++){
                    QGeoCoordinate geoCoord, geoCoordNext;
                    double lat, lng;
                    convertNedToGeo(-intersectionPoint.y(), intersectionPoint.x(), 0, tangentOrigin, &geoCoord);
                    waypoint_from_heading_and_distance(geoCoord.latitude (), geoCoord.longitude (), bearing, num * cameraTriggerDist, &lat, &lng);
                    geoCoordNext.setLatitude (lat);
                    geoCoordNext.setLongitude (lng);
                    cameraTriggerGeoLocal.append (QVariant::fromValue(geoCoordNext));
                }

                if(cameraTriggerGeoLocal.size ()){
                    firstIntersectionPoint = true;
                    linenormalVector = resultLines[i];
                }
            }else if(result == QLineF::BoundedIntersection || result == QLineF::UnboundedIntersection){
                QGeoCoordinate geoCoord3;
                convertNedToGeo(-(intersectionPoint).y(), (intersectionPoint).x(), 0, tangentOrigin, &geoCoord3);
                //double distIntP1 = get_distance_to_next_waypoint(geoCoord3.latitude (), geoCoord3.longitude (), geoCoord1.latitude (), geoCoord1.longitude ());
                //double distIntP2 = get_distance_to_next_waypoint(geoCoord3.latitude (), geoCoord3.longitude (), geoCoord2.latitude (), geoCoord2.longitude ());
                double distIntP1 = sqrt(pow((intersectionPoint - line.p1() + 3*cameraTriggerOffset).x(), 2.0) + pow((intersectionPoint - line.p1() + 3*cameraTriggerOffset).y(), 2.0));
                double distIntP2 = sqrt(pow((intersectionPoint - line.p2() - 3*cameraTriggerOffset).x(), 2.0) + pow((intersectionPoint - line.p2() - 3*cameraTriggerOffset).y(), 2.0));
                if(fabs ((distP1P2 - (distIntP1 + distIntP2))) < 0.01){
                    //qDebug()<<__FILE__<<" "<<__LINE__;
                    bearing = get_bearing_to_next_waypoint(geoCoord3.latitude (), geoCoord3.longitude (), geoCoord1.latitude (), geoCoord1.longitude ());
                    for( int numStart = 0; numStart <=  floor(sqrt(pow((intersectionPoint - line.p1() + 3*cameraTriggerOffset).x(), 2.0) + pow((intersectionPoint - line.p1() + 3*cameraTriggerOffset).y(), 2.0)) / sqrt(pow(cameraTriggerOffset.x(), 2.0) + pow(cameraTriggerOffset.y(), 2.0))); numStart++){
                        QGeoCoordinate geoCoordNext;
                        double lat, lng;
                        waypoint_from_heading_and_distance(geoCoord3.latitude (), geoCoord3.longitude (), bearing, (numStart * cameraTriggerDist), &lat, &lng);
                        geoCoordNext.setLatitude (lat);
                        geoCoordNext.setLongitude (lng);
                        cameraTriggerGeoLocal.insert (0, QVariant::fromValue(geoCoordNext));
                        //cameraTriggerPointLocal.insert (0, intersectionPoint - (cameraTriggerOffset * numStart));
                    }
                    bearing = get_bearing_to_next_waypoint(geoCoord3.latitude (), geoCoord3.longitude (), geoCoord2.latitude (), geoCoord2.longitude ());
                    for( int num = 1; num <=   floor(sqrt(pow((line.p2()+ 3*cameraTriggerOffset - intersectionPoint).x(), 2.0) + pow((line.p2()+ 3*cameraTriggerOffset - intersectionPoint).y(), 2.0)) / sqrt(pow(cameraTriggerOffset.x(), 2.0) + pow(cameraTriggerOffset.y(), 2.0))); num++){
                        QGeoCoordinate geoCoordNext;
                        double lat, lng;
                        waypoint_from_heading_and_distance(geoCoord3.latitude (), geoCoord3.longitude (), bearing, num * cameraTriggerDist, &lat, &lng);
                        geoCoordNext.setLatitude (lat);
                        geoCoordNext.setLongitude (lng);
                        cameraTriggerGeoLocal.append (QVariant::fromValue(geoCoordNext));
                        //cameraTriggerPointLocal << intersectionPoint + (cameraTriggerOffset * num);
                    }
                }else if( fabs ((distIntP1 - (distP1P2 + distIntP2))) < 0.01){
                    //qDebug()<<__FILE__<<" "<<__LINE__<<" "<<numDisparity;
                    QGeoCoordinate geoCoord4;
                    int numDisparity = floor(sqrt(pow((intersectionPoint - line.p2() - 3*cameraTriggerOffset).x(), 2.0) + pow((intersectionPoint - line.p2() - 3*cameraTriggerOffset).y(), 2.0)) / sqrt(pow(cameraTriggerOffset.x(), 2.0) + pow(cameraTriggerOffset.y(), 2.0)));
                    intersectionPoint = intersectionPoint - (cameraTriggerOffset * (numDisparity + 1));
                    convertNedToGeo(-(intersectionPoint).y(), (intersectionPoint).x(), 0, tangentOrigin, &geoCoord4);
                    bearing = get_bearing_to_next_waypoint(geoCoord3.latitude (), geoCoord3.longitude (), geoCoord4.latitude (), geoCoord4.longitude ());
                    for( int num = 0; num <= floor(sqrt(pow((line.p1()-3*cameraTriggerOffset - intersectionPoint).x(), 2.0) + pow((line.p1() - 3*cameraTriggerOffset - intersectionPoint).y(), 2.0)) / sqrt(pow(cameraTriggerOffset.x(), 2.0) + pow(cameraTriggerOffset.y(), 2.0)));
                         num++){
                        QGeoCoordinate geoCoordNext;
                        double lat, lng;
                        waypoint_from_heading_and_distance(geoCoord4.latitude (), geoCoord4.longitude (), bearing, num * cameraTriggerDist, &lat, &lng);
                        geoCoordNext.setLatitude (lat);
                        geoCoordNext.setLongitude (lng);
                        cameraTriggerGeoLocal.append (QVariant::fromValue(geoCoordNext));
                        //cameraTriggerPointLocal << intersectionPoint - (cameraTriggerOffset * num);
                    }
                }else if( fabs ((distIntP2 - (distIntP1 + distP1P2))) < 0.01){
                    //qDebug()<<__FILE__<<" "<<__LINE__<<" "<<numDisparity;
                    QGeoCoordinate geoCoord5;
                    int numDisparity = floor(sqrt(pow((intersectionPoint - line.p1() + 3*cameraTriggerOffset).x(), 2.0) + pow((intersectionPoint - line.p1() + 3*cameraTriggerOffset).y(), 2.0)) / sqrt(pow(cameraTriggerOffset.x(), 2.0) + pow(cameraTriggerOffset.y(), 2.0)));
                    intersectionPoint = intersectionPoint + (cameraTriggerOffset * (numDisparity + 1));
                    convertNedToGeo(-(intersectionPoint).y(), (intersectionPoint).x(), 0, tangentOrigin, &geoCoord5);
                    bearing = get_bearing_to_next_waypoint(geoCoord3.latitude (), geoCoord3.longitude (), geoCoord5.latitude (), geoCoord5.longitude ());
                    for( int num = 0; num <= floor(sqrt(pow((line.p2()+ 3*cameraTriggerOffset - intersectionPoint).x(), 2.0) + pow((line.p2()+3*cameraTriggerOffset - intersectionPoint).y(), 2.0)) / sqrt(pow(cameraTriggerOffset.x(), 2.0) + pow(cameraTriggerOffset.y(), 2.0)));
                         num++){
                        QGeoCoordinate geoCoordNext;
                        double lat, lng;
                        waypoint_from_heading_and_distance(geoCoord5.latitude (), geoCoord5.longitude (), bearing, num * cameraTriggerDist, &lat, &lng);
                        geoCoordNext.setLatitude (lat);
                        geoCoordNext.setLongitude (lng);
                        cameraTriggerGeoLocal.append ( QVariant::fromValue(geoCoordNext));
                        //cameraTriggerPointLocal << intersectionPoint + (cameraTriggerOffset * num);
                    }
                }else{
                    qDebug()<<__FILE__<<" "<<__LINE__
                           <<" distP1P2 : "<<distP1P2
                          <<" distIntP1 : "<<distIntP1
                         <<" distIntP2 : "<<distIntP2;
                }
            }
        }
        if(cameraTriggerGeoLocal.size () > 0){
            _cameraTriggerPointList.append (cameraTriggerGeoLocal);
        }

        if (i & 1) {
            if (turnaroundDist > 0.0) {
                QGeoCoordinate firstCoord = convertNedToGeo(-(line.p2() + turnaroundOffset + 3*cameraTriggerOffset).y(), (line.p2() + turnaroundOffset + 3*cameraTriggerOffset).x(), 0, tangentOrigin);
                _gridPoints.append (QVariant::fromValue(firstCoord));
//                gridPoints << line.p2() + turnaroundOffset << line.p2() << line.p1() << line.p1() - turnaroundOffset;
                if(cameraTriggerGeoLocal.size () == 0){
                    _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p2() + 3*cameraTriggerOffset).y(), (line.p2()+3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                    _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p1() - 3*cameraTriggerOffset).y(), (line.p1() - 3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                    _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT<< CAMPOINT << CAMTURNOFF;
                    _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                }else{
                    if(get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                            >= (cameraTriggerDist * 1.5 + turnaroundDist)){
                        _gridPoints.append (cameraTriggerGeoLocal.at (0));
                        _gridPoints.append (cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1));
                        _cameraTriggerPoint << CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT<< CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber << CAMPOINT << CAMSELSTARTFPOINT
                                             <<get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (0).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (0).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                                            << cameraTriggerGeoLocal.size ()
                                            << CAMPOINT << CAMTURNOFF;
                    }else{
                        _gridPoints.append (cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1));
                        _gridPoints.append (cameraTriggerGeoLocal.at (0));
                        _cameraTriggerPoint << CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT<< CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber << CAMPOINT << CAMSELSTARTFPOINT
                                             <<get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                                            << cameraTriggerGeoLocal.size ()
                                            << CAMPOINT << CAMTURNOFF;
                    }
                }
                if(_vehicle->vtol() && _hideturnaround){
                     _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p1()- 3*cameraTriggerOffset).y(), (line.p1()+3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                }else{
                      _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p1() - turnaroundOffset -3*cameraTriggerOffset).y(), (line.p1() - turnaroundOffset - 3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                }
            } else {
                QGeoCoordinate firstCoord = convertNedToGeo(-(line.p2() + 3*cameraTriggerOffset).y(), (line.p2()+ 3*cameraTriggerOffset).x(), 0, tangentOrigin);
                _gridPoints.append (QVariant::fromValue(firstCoord));
//                gridPoints << line.p2() << line.p2() << line.p1() << line.p1();
                if(cameraTriggerGeoLocal.size () == 0){
                    _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p2()+ 3*cameraTriggerOffset).y(), (line.p2() + 3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                    _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p1() - 3*cameraTriggerOffset).y(), (line.p1() - 3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                    _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT<< CAMPOINT << CAMTURNOFF;
                    _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                }else{
                    if(get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                            >= cameraTriggerDist * 1.5){
                        _gridPoints.append (cameraTriggerGeoLocal.at (0));
                        _gridPoints.append (cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1));
                        _cameraTriggerPoint << CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT<< CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber << CAMPOINT << CAMSELSTARTFPOINT
                                             <<get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (0).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (0).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                                            << cameraTriggerGeoLocal.size ()
                                            << CAMPOINT << CAMTURNOFF;
                    }else{
                        _gridPoints.append (cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1));
                        _gridPoints.append (cameraTriggerGeoLocal.at (0));
                        _cameraTriggerPoint << CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT<< CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber << CAMPOINT << CAMSELSTARTFPOINT
                                             <<get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                                            << cameraTriggerGeoLocal.size ()
                                            << CAMPOINT << CAMTURNOFF;
                    }
                }
                _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p1() - 3*cameraTriggerOffset).y(), (line.p1() - 3*cameraTriggerOffset).x(), 0, tangentOrigin)));
            }
        } else {
            if (turnaroundDist > 0.0) {
                QGeoCoordinate firstCoord = convertNedToGeo(-(line.p1() -3*cameraTriggerOffset - turnaroundOffset).y(), (line.p1()-3*cameraTriggerOffset - turnaroundOffset).x(), 0, tangentOrigin);
                _gridPoints.append (QVariant::fromValue(firstCoord));
//                gridPoints << line.p1() - turnaroundOffset << line.p1() << line.p2() << line.p2() + turnaroundOffset;
                if(cameraTriggerGeoLocal.size () == 0){
                    _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p1()-3*cameraTriggerOffset).y(), (line.p1()-3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                    _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p2()+3*cameraTriggerOffset).y(), (line.p2()+3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                    _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                    _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                }else{
                    if(get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                            >= cameraTriggerDist * 1.5 + turnaroundDist){
                        _gridPoints.append (cameraTriggerGeoLocal.at (0));
                        _gridPoints.append (cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1));
                        _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT <<
                                               get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (0).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (0).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                                            << cameraTriggerGeoLocal.size ()
                                            << CAMPOINT << CAMTURNOFF;
                    }else{
                        _gridPoints.append (cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1));
                        _gridPoints.append (cameraTriggerGeoLocal.at (0));
                        _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT <<
                                               get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                                            << cameraTriggerGeoLocal.size ()
                                            << CAMPOINT << CAMTURNOFF;
                    }
                }
                if(_vehicle->vtol()  && _hideturnaround){
                     _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p2() + 3*cameraTriggerOffset).y(), (line.p2()+3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                }else{
                     _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p2() + turnaroundOffset + 3*cameraTriggerOffset).y(), (line.p2() + turnaroundOffset + 3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                }
            } else {
                QGeoCoordinate firstCoord = convertNedToGeo(-(line.p1()- 3*cameraTriggerOffset).y(), (line.p1()- 3*cameraTriggerOffset).x(), 0, tangentOrigin);
                _gridPoints.append (QVariant::fromValue(firstCoord));
//                gridPoints << line.p1() << line.p1() << line.p2() << line.p2();
                if(cameraTriggerGeoLocal.size () == 0){
                    _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p1() - 3*cameraTriggerOffset).y(), (line.p1() - 3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                    _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p2() + 3*cameraTriggerOffset).y(), (line.p2() + 3*cameraTriggerOffset).x(), 0, tangentOrigin)));
                    _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                    _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                }else{
                    if(get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                            >= cameraTriggerDist * 1.5){
                        _gridPoints.append (cameraTriggerGeoLocal.at (0));
                        _gridPoints.append (cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1));
                        _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT <<
                                               get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (0).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (0).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                                            << cameraTriggerGeoLocal.size ()
                                            << CAMPOINT << CAMTURNOFF;
                    }else{
                        _gridPoints.append (cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1));
                        _gridPoints.append (cameraTriggerGeoLocal.at (0));
                        _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT <<
                                               get_distance_to_next_waypoint(cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().latitude (), cameraTriggerGeoLocal.at (cameraTriggerGeoLocal.size () - 1).value<QGeoCoordinate>().longitude (), firstCoord.latitude (), firstCoord.longitude ())
                                            << cameraTriggerGeoLocal.size ()
                                            << CAMPOINT << CAMTURNOFF;
                    }
                }
                _gridPoints.append (QVariant::fromValue(convertNedToGeo(-(line.p2() + 3*cameraTriggerOffset).y(), (line.p2() + 3*cameraTriggerOffset).x(), 0, tangentOrigin)));
            }
        }
    }
    //============================================
    //edit by wang.lichen
    QList<QPointF> gridPointsAdd;
    for (int i = 0; i < resultLinesAdd.count(); i++) {
        QPointF addPoint(0,0);
        if (turnaroundDist > 0.0 && sumLines >= 2) {
            if((sumLines - 1) & 1){
                addPoint = resultLines[ sumLines - 1].p1 ();
            }else{
                addPoint = resultLines[ sumLines - 1].p2 ();
            }
        }else if(sumLines >= 2){
            if((sumLines - 1) & 1){
                addPoint = resultLines[ sumLines - 1].p2 ();
            }else{
                addPoint = resultLines[ sumLines - 1].p1 ();
            }
        }
        QLineF& line = resultLinesAdd[i];
        QPointF pp = line.p1();
        QPointF qq = line.p1();
        qreal x1 = line.p1().x();
        qreal y1 = line.p1().y();
        qreal x2 = line.p2().x();
        qreal y2 = line.p2().y();
        qreal len = sqrt(pow(x2-x1,2)+pow(y2-y1,2)) + 100;
        pp.setX((60*x1-len*x2)/(60-len));
        pp.setY((60*y1-len*y2)/(60-len));
        line.setP1(pp);
        qq.setX((60*x2-len*x1)/(60-len));
        qq.setY((60*y2-len*y1)/(60-len));
        line.setP2(qq);
        qreal xxx;
        qreal yyy;
        qreal xxx1;
        qreal yyy1;
        qreal xxx2;
        qreal yyy2;
        xxx = addPoint.x ();
        yyy = addPoint.y ();
        xxx1 = line.p1().x();
        yyy1 = line.p1().y();
        xxx2 = line.p2().x();
        yyy2 = line.p2().y();
        QPointF turnaroundOffset = line.p2() - line.p1();
        turnaroundOffset = turnaroundOffset * turnaroundDist / sqrt(pow(turnaroundOffset.x(),2.0) + pow(turnaroundOffset.y(),2.0));

        QPointF cameraTriggerOffset = line.p2() - line.p1();
        cameraTriggerOffset = cameraTriggerOffset * _cameraTriggerDistanceFact.rawValue().toDouble() / sqrt(pow(cameraTriggerOffset.x(),2.0) + pow(cameraTriggerOffset.y(),2.0));

        QList<QPointF> cameraTriggerPointLocal;
        cameraTriggerPointLocal.clear ();
        if(_cameraTrigger && _cameraTriggerDistanceFact.rawValue().toDouble() > 0.0){
            //next line
            for( int num = 0; num < sqrt(pow((line.p2() - line.p1()).x(),2.0) + pow((line.p2() - line.p1()).y(),2.0)) / sqrt(pow(cameraTriggerOffset.x(),2.0) + pow(cameraTriggerOffset.y(),2.0));
                 num++){
                cameraTriggerPointLocal << line.p1() + (cameraTriggerOffset * num);
            }
        }

        if(cameraTriggerPointLocal.size () > 0){
            for (int j=0; j<cameraTriggerPointLocal.count(); j++) {
                QPointF& point = cameraTriggerPointLocal[j];
                QGeoCoordinate geoCoord;
                convertNedToGeo(-point.y(), point.x(), 0, tangentOrigin, &geoCoord);
                _cameraTriggerPointList += QVariant::fromValue(geoCoord);
            }
        }
        gridPointsAdd.clear ();
        if (i & 1) {
            if (turnaroundDist > 0.0) {
                if(pow(xxx-xxx1,2)+pow(yyy-yyy1,2)-pow(xxx-xxx2,2)-pow(yyy-yyy2,2) < 0){
                    if(cameraTriggerPointLocal.size () == 0){
                        gridPointsAdd   << line.p2() + turnaroundOffset<< line.p1() << line.p2()<< line.p1() - turnaroundOffset ;
                        _cameraTriggerPoint << CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber << CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                    }else{
                        if(sqrt(pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2 ())).x(), 2.0)
                                + pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2 ())).y(), 2.0)) >=
                                cameraTriggerDist * 1.5){
                            gridPointsAdd << line.p2() + turnaroundOffset
                                       << cameraTriggerPointLocal.at (0)
                                       << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1)
                                       << line.p1() - turnaroundOffset;
                            _cameraTriggerPoint << CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber << CAMPOINT << CAMSELSTARTFPOINT
                                                 << sqrt(pow((cameraTriggerPointLocal.at (0) - (line.p2() + turnaroundOffset)).x(),2.0)
                                                         + pow((cameraTriggerPointLocal.at (0) - (line.p2() + turnaroundOffset)).y(),2.0))
                                                 << cameraTriggerPointLocal.size ()
                                                 << CAMPOINT << CAMTURNOFF;
                        }else{
                            gridPointsAdd << line.p2() + turnaroundOffset
                                       << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1)
                                       << cameraTriggerPointLocal.at (0)
                                       << line.p1() - turnaroundOffset;
                            _cameraTriggerPoint << CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber << CAMPOINT << CAMSELSTARTFPOINT
                                                 << sqrt(pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2() + turnaroundOffset)).x(),2.0)
                                                         + pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2() + turnaroundOffset)).y(),2.0))
                                                 << cameraTriggerPointLocal.size ()
                                                 << CAMPOINT << CAMTURNOFF;
                        }
                    }
                }
                else{
                    if(cameraTriggerPointLocal.size () == 0){
                        gridPointsAdd << line.p1() - turnaroundOffset << line.p2() << line.p1() << line.p2() + turnaroundOffset ;
                        _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                    }else{
                        if(sqrt(pow((cameraTriggerPointLocal.at (0) - (line.p1 ())).x(), 2.0)
                                + pow((cameraTriggerPointLocal.at (0) - (line.p1 ())).y(), 2.0)) >=
                                cameraTriggerDist * 1.5){
                            gridPointsAdd  << line.p1() - turnaroundOffset
                                        << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1)
                                        << cameraTriggerPointLocal.at (0)
                                        << line.p2() + turnaroundOffset;
                            _cameraTriggerPoint << CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber << CAMPOINT << CAMSELSTARTFPOINT
                                                 << sqrt(pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p1() - turnaroundOffset)).x(),2.0)
                                                         + pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p1() - turnaroundOffset)).y(),2.0))
                                                 << cameraTriggerPointLocal.size ()
                                                 << CAMPOINT << CAMTURNOFF;
                        }else{
                            gridPointsAdd  << line.p1() - turnaroundOffset
                                        << cameraTriggerPointLocal.at (0)
                                        << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1)
                                        << line.p2() + turnaroundOffset;
                            _cameraTriggerPoint << CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber << CAMPOINT << CAMSELSTARTFPOINT
                                                 << sqrt(pow((cameraTriggerPointLocal.at (0) - (line.p1() - turnaroundOffset)).x(),2.0)
                                                         + pow((cameraTriggerPointLocal.at (0) - (line.p1() - turnaroundOffset)).y(),2.0))
                                                 << cameraTriggerPointLocal.size ()
                                                 << CAMPOINT << CAMTURNOFF;
                        }
                    }
                }
            } else {
                if(pow(xxx-xxx1,2)+pow(yyy-yyy1,2)-pow(xxx-xxx2,2)-pow(yyy-yyy2,2)<0){
                    gridPointsAdd << line.p2() << line.p2() << line.p1() << line.p1();
                    _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                    _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                }
                else{
                    gridPointsAdd << line.p1() << line.p1() << line.p2() << line.p2();
                    _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                    _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                }
            }
        } else {
            if (turnaroundDist > 0.0) {
                if(pow(xxx-xxx1,2)+pow(yyy-yyy1,2)-pow(xxx-xxx2,2)-pow(yyy-yyy2,2)<0){
                    if(cameraTriggerPointLocal.size () == 0){
                        gridPointsAdd << line.p1() - turnaroundOffset << line.p1() << line.p2() << line.p2() + turnaroundOffset;
                        _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                    }else{
                        if(sqrt(pow((cameraTriggerPointLocal.at (0) - (line.p1 ())).x(), 2.0)
                                + pow((cameraTriggerPointLocal.at (0) - (line.p1 ())).y(), 2.0)) >=
                                cameraTriggerDist * 1.5){
                            gridPointsAdd << line.p1() - turnaroundOffset
                                       << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size() - 1)
                                       << cameraTriggerPointLocal.at (0)
                                       << line.p2() + turnaroundOffset;
                            _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT
                                                << sqrt(pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size() - 1) - (line.p1() - turnaroundOffset)).x(),2.0) +
                                                        pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size() - 1) - (line.p1() - turnaroundOffset)).y(),2.0))
                                                << cameraTriggerPointLocal.size ()
                                                << CAMPOINT << CAMTURNOFF;
                        } else{
                            gridPointsAdd << line.p1() - turnaroundOffset
                                       << cameraTriggerPointLocal.at (0)
                                       << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size() - 1)
                                       << line.p2() + turnaroundOffset;
                            _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT
                                                << sqrt(pow((cameraTriggerPointLocal.at (0) - (line.p1() - turnaroundOffset)).x(),2.0) +
                                                        pow((cameraTriggerPointLocal.at (0) - (line.p1() - turnaroundOffset)).y(),2.0))
                                                << cameraTriggerPointLocal.size ()
                                                << CAMPOINT << CAMTURNOFF;
                        }
                    }
                }
                else{
                    if(cameraTriggerPointLocal.size () == 0){
                        gridPointsAdd << line.p2() + turnaroundOffset << line.p2() << line.p1() << line.p1() - turnaroundOffset;
                        _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                    }else{
                        if(sqrt(pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2 ())).x(), 2.0)
                                + pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2 ())).y(), 2.0)) >=
                                cameraTriggerDist * 1.5){
                            gridPointsAdd << line.p2() + turnaroundOffset
                                       << cameraTriggerPointLocal.at (0)
                                       << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1)
                                       << line.p1() - turnaroundOffset;
                            _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT
                                                << sqrt(pow((cameraTriggerPointLocal.at (0) - (line.p2() + turnaroundOffset)).x(),2.0) +
                                                        pow((cameraTriggerPointLocal.at (0) - (line.p2() + turnaroundOffset)).y(),2.0))
                                                << cameraTriggerPointLocal.size ()
                                                << CAMPOINT << CAMTURNOFF;
                        }else{
                            gridPointsAdd << line.p2() + turnaroundOffset
                                       << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1)
                                       << cameraTriggerPointLocal.at (0)
                                       << line.p1() - turnaroundOffset;
                            _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT
                                                << sqrt(pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2() + turnaroundOffset)).x(),2.0) +
                                                        pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2() + turnaroundOffset)).y(),2.0))
                                                << cameraTriggerPointLocal.size ()
                                                << CAMPOINT << CAMTURNOFF;
                        }
                    }
                }
            } else {
                if(pow(xxx-xxx1,2)+pow(yyy-yyy1,2)-pow(xxx-xxx2,2)-pow(yyy-yyy2,2)<0){
//                    gridPointsAdd << line.p1() << line.p1() << line.p2() << line.p2();
//                    _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
//                    _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;

                    if(cameraTriggerPointLocal.size () == 0){
                        gridPointsAdd << line.p1() << line.p1() << line.p2() << line.p2();
                        _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                    }else{
                        if(sqrt(pow((cameraTriggerPointLocal.at (0) - (line.p1 ())).x(), 2.0)
                                + pow((cameraTriggerPointLocal.at (0) - (line.p1 ())).y(), 2.0)) >=
                                cameraTriggerDist * 1.5){
                            gridPointsAdd << line.p1()
                                       << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size() - 1)
                                       << cameraTriggerPointLocal.at (0)
                                       << line.p2();
                            _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT
                                                << sqrt(pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size() - 1) - (line.p1())).x(),2.0) +
                                                        pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size() - 1) - (line.p1())).y(),2.0))
                                                << cameraTriggerPointLocal.size ()
                                                << CAMPOINT << CAMTURNOFF;
                        } else{
                            gridPointsAdd << line.p1()
                                       << cameraTriggerPointLocal.at (0)
                                       << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size() - 1)
                                       << line.p2();
                            _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT
                                                << sqrt(pow((cameraTriggerPointLocal.at (0) - (line.p1())).x(),2.0) +
                                                        pow((cameraTriggerPointLocal.at (0) - (line.p1())).y(),2.0))
                                                << cameraTriggerPointLocal.size ()
                                                << CAMPOINT << CAMTURNOFF;
                        }
                    }
                }
                else{
//                    gridPointsAdd << line.p2() << line.p2() << line.p1() << line.p1();
//                    _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
//                    _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;


                    if(cameraTriggerPointLocal.size () == 0){
                        gridPointsAdd << line.p2() << line.p2() << line.p1() << line.p1();
                        _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                        _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT << 0 << 0 << CAMPOINT << CAMTURNOFF;
                    }else{
                        if(sqrt(pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2 ())).x(), 2.0)
                                + pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2 ())).y(), 2.0)) >=
                                cameraTriggerDist * 1.5){
                            gridPointsAdd << line.p2()
                                       << cameraTriggerPointLocal.at (0)
                                       << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1)
                                       << line.p1();
                            _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT
                                                << sqrt(pow((cameraTriggerPointLocal.at (0) - (line.p2())).x(),2.0) +
                                                        pow((cameraTriggerPointLocal.at (0) - (line.p2())).y(),2.0))
                                                << cameraTriggerPointLocal.size ()
                                                << CAMPOINT << CAMTURNOFF;
                        }else{
                            gridPointsAdd << line.p2()
                                       << cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1)
                                       << cameraTriggerPointLocal.at (0)
                                       << line.p1();
                            _cameraTriggerPoint<< CAMPOINT << CAMSELSTARTFPOINT << CAMTURNON << CAMSELENDFPOINT << CAMPOINT << CAMTURNOFF;
                            _cameraTriggerNumber<< CAMPOINT << CAMSELSTARTFPOINT
                                                << sqrt(pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2())).x(),2.0) +
                                                        pow((cameraTriggerPointLocal.at (cameraTriggerPointLocal.size () - 1) - (line.p2())).y(),2.0))
                                                << cameraTriggerPointLocal.size ()
                                                << CAMPOINT << CAMTURNOFF;
                        }
                    }

                }
            }
        }
        for (int j=0; j<gridPointsAdd.count(); j++) {
            QPointF& point = gridPointsAdd[j];
            QGeoCoordinate geoCoord;
            convertNedToGeo(-point.y(), point.x(), 0, tangentOrigin, &geoCoord);
            if(gridSpacingAdd1!=0){
               _gridPoints += QVariant::fromValue(geoCoord);
            }
        }
    }
    gridPointsAdd.clear ();
    //================================================

    double surveyDistance =_surveyDistance;
//    // Convert to Geo and set altitude
//    for (int i=0; i< gridPoints.count(); i++) {
//        qDebug()<<"gridPoints.count() is 0";
//        if (i != 0) {
//            surveyDistance += sqrt(pow((gridPoints[i] - gridPoints[i - 1]).x(),2.0) + pow((gridPoints[i] - gridPoints[i - 1]).y(),2.0));
//            qDebug()<<"second surveyDistance:"<<surveyDistance;
//        }
//    }
    _setSurveyDistance(surveyDistance);

    if (_cameraTriggerDistanceFact.rawValue().toDouble() > 0) {
        _setCameraShots(_cameraTriggerPointList.size());
    } else {
        _setCameraShots(0);
    }
}

void SurveyMissionItem::updateMissionItems(bool update){
    if(update){
        _updateGenerateGrid(true);
    }else{
        qWarning()<<"Do you do not want update your mission items, Are you sure?";
    }
}

QmlObjectListModel* SurveyMissionItem::getMissionItems(QGeoCoordinate itemCoordinate) const
{
    QmlObjectListModel* pMissionItems = new QmlObjectListModel;

    if(_gridPoints.count() <= 0){
        return pMissionItems;
    }
    float gridChangeSpeed = _gridChangeSpeedFact.rawValue().toDouble();
    int seqNum = _sequenceNumber;
    MissionItem *item = new MissionItem(seqNum++,
                                        MAV_CMD_NAV_TAKEOFF,
                                        _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                                        15.00, //pitch
                                        0,0,
                                        0,    //yaw angle
                                        itemCoordinate.latitude(),
                                        itemCoordinate.longitude(),
                                        50,
                                        true,
                                        false,
                                        pMissionItems
                                        );
    pMissionItems->append(item);

    if (gridChangeSpeed > 0.0)
    {
        MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                            MAV_CMD_DO_CHANGE_SPEED,        // MAV_CMD
                                            MAV_FRAME_MISSION,              // MAV_FRAME
                                            0.0,
                                            gridChangeSpeed,
                                            -1,
                                            0.0, 0.0, 0.0, 0.0,             // param 1-7
                                            true,                           // autoContinue
                                            false,                          // isCurrentItem
                                            pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
        pMissionItems->append(item);
    }

    if (_cameraTrigger && _cameraTriggerDistanceFact.rawValue().toDouble() > 0)
    {
        int j = 0;
        QGeoCoordinate coordCamera   = _gridPoints[0].value<QGeoCoordinate>();
        QGeoCoordinate camStartCoord = _gridPoints[0].value<QGeoCoordinate>();
        QGeoCoordinate camEndCoord = _gridPoints[0].value<QGeoCoordinate>();
        QGeoCoordinate camEndCoord2   = _gridPoints[0].value<QGeoCoordinate>();
        // Turn into a path
        int turnNumber = 5;
        if(_turnaroundDistFact.rawValue().toDouble() > 0.0){
            turnNumber = 9;
        }
        for (int i = 0; i < _cameraTriggerPoint.count(); i++) {

            if(_cameraTriggerPoint.at (i)== CAMPOINT){
                if(j > _gridPoints.count () - 1){
                    break;
                }
                QGeoCoordinate coord = _gridPoints[j].value<QGeoCoordinate>();
                coordCamera = coord;
                double altitude = _gridAltitudeFact.rawValue().toDouble();

                if((j > _gridPoints.count() - turnNumber)&&(_gridSpacingFactAdd1.rawValue().toDouble()!=0)){
                    altitude = altitude + altitude * 0.2;
                }
                MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                    MAV_CMD_NAV_WAYPOINT,           // MAV_CMD
                                                    _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,  // MAV_FRAME
                                                    0.0, 0.0, 0.0, 0.0,             // param 1-4
                                                    coord.latitude(),
                                                    coord.longitude(),
                                                    altitude,
                                                    true,                           // autoContinue
                                                    false,                          // isCurrentItem
                                                    pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
                pMissionItems->append(item);
                j++;
            }else if(_cameraTriggerPoint.at (i) == CAMTURNON){
                int num = 0;
                if((i + 1) < _cameraTriggerNumber.count ()){
                    num = _cameraTriggerNumber.at (i+1);
                }
                double dist = _cameraTriggerDistanceFact.rawValue().toDouble() ;
                double param1 = dist + num / 1000.0;
                //qDebug()<<num<<" "<<param1;

                MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                    MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // MAV_CMD
                                                    MAV_FRAME_MISSION,              // MAV_FRAME
                                                    param1,   // trigger distance
                                                    _cameraTriggerNumber.at (i),
                                                    coordCamera.latitude (),
                                                    coordCamera.longitude (),
                                                    camEndCoord.latitude (),
                                                    camEndCoord.longitude (),
                                                    coordCamera.azimuthTo (camEndCoord),   // param 2-7
                                                    true,                           // autoContinue
                                                    false,                          // isCurrentItem
                                                    pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
                pMissionItems->append(item);
            }else if(_cameraTriggerPoint.at (i) == CAMTURNOFF){
                MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                    MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // MAV_CMD
                                                    MAV_FRAME_MISSION,              // MAV_FRAME
                                                    0.0,   // trigger distance
                                                    0.0,
                                                    0.0,
                                                    0.0,
                                                    camEndCoord2.latitude (),
                                                    camEndCoord2.longitude (),
                                                    0.0,   // param 2-7
                                                    true,                           // autoContinue
                                                    false,                          // isCurrentItem
                                                    pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
                pMissionItems->append(item);
            }else if(_cameraTriggerPoint.at (i) == CAMSELSTARTFPOINT){
                camStartCoord = _gridPoints[j].value<QGeoCoordinate>();
                if(j + 1 < _gridPoints.size ()){
                    camEndCoord = _gridPoints[j + 1].value<QGeoCoordinate>();
                }
                j++;
            }else if(_cameraTriggerPoint.at (i) == CAMSELENDFPOINT){
                camEndCoord2 = _gridPoints[j].value<QGeoCoordinate>();
                j++;
            }
        }
    }else{
        // Turn into a path
        int j = 0;
        int turnNumber = 5;
        if(_turnaroundDistFact.rawValue().toDouble() > 0.0){
            turnNumber = 9;
        }
//        for (int i = 0; i < _gridPoints.count(); i++) {
//            QGeoCoordinate coord = _gridPoints[i].value<QGeoCoordinate>();
//            double altitude = _gridAltitudeFact.rawValue().toDouble();

//            if((i > _gridPoints.count() - turnNumber)&&(_gridSpacingFactAdd1.rawValue().toDouble()!=0)){
//                altitude = altitude + altitude * 0.2;
//            }
//            MissionItem* item = new MissionItem(seqNum++,                       // sequence number
//                                                MAV_CMD_NAV_WAYPOINT,           // MAV_CMD
//                                                _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,  // MAV_FRAME
//                                                0.0, 0.0, 0.0, 0.0,             // param 1-4
//                                                coord.latitude(),
//                                                coord.longitude(),
//                                                altitude,
//                                                true,                           // autoContinue
//                                                false,                          // isCurrentItem
//                                                pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
//            pMissionItems->append(item);
//        }

        for (int i = 0; i < _cameraTriggerPoint.count(); i++) {

            if(_cameraTriggerPoint.at (i)== CAMPOINT){
                if(j > _gridPoints.count () - 1){
                    break;
                }
                QGeoCoordinate coord = _gridPoints[j].value<QGeoCoordinate>();
                double altitude = _gridAltitudeFact.rawValue().toDouble();

                if((j > _gridPoints.count() - turnNumber)&&(_gridSpacingFactAdd1.rawValue().toDouble()!=0)){
                    altitude = altitude + altitude * 0.2;
                }
                MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                    MAV_CMD_NAV_WAYPOINT,           // MAV_CMD
                                                    _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,  // MAV_FRAME
                                                    0.0, 0.0, 0.0, 0.0,             // param 1-4
                                                    coord.latitude(),
                                                    coord.longitude(),
                                                    altitude,
                                                    true,                           // autoContinue
                                                    false,                          // isCurrentItem
                                                    pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
                pMissionItems->append(item);
                j++;
            }else if(_cameraTriggerPoint.at (i) == CAMTURNON){
            }else if(_cameraTriggerPoint.at (i) == CAMTURNOFF){
            }else if(_cameraTriggerPoint.at (i) == CAMSELSTARTFPOINT){
                j++;
            }else if(_cameraTriggerPoint.at (i) == CAMSELENDFPOINT){
                j++;
            }
        }
    }

//    if (_cameraTrigger) {
//        MissionItem* item = new MissionItem(seqNum++,                       // sequence number
//                                            MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // MAV_CMD
//                                            MAV_FRAME_MISSION,              // MAV_FRAME
//                                            0.0,                            // trigger distance
//                                            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // param 2-7
//                                            true,                           // autoContinue
//                                            false,                          // isCurrentItem
//                                            pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
//        pMissionItems->append(item);
//    }

    if(_vehicle && _vehicle->homePositionAvailable ()){
        QGeoCoordinate homePosition = _vehicle->homePosition();
        double altitude = _gridAltitudeFact.rawValue().toDouble();
        MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                            MAV_CMD_NAV_LOITER_TIME,           // MAV_CMD
                                            MAV_FRAME_GLOBAL_RELATIVE_ALT,  // MAV_FRAME
                                            100000.0,
                                            0.0,
                                            100.0,
                                            1.0,             // param 1-4
                                            homePosition.latitude(),
                                            homePosition.longitude(),
                                            altitude,
                                            true,                           // autoContinue
                                            false,                          // isCurrentItem
                                            pMissionItems);
        pMissionItems->append(item);
    }
    return pMissionItems;
}

QmlObjectListModel *SurveyMissionItem::getVtolMissionItems(void) const
{
    QmlObjectListModel* pMissionItems = new QmlObjectListModel;
    int sum = _gridPoints.count();
    if(_gridPoints.count() <= 0){
        return pMissionItems;
    }

    int seqNum = _sequenceNumber;
    if(_vehicle && _vehicle->homePositionAvailable ()){
        QGeoCoordinate homePosition = _vehicle->homePosition();
        MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                            MAV_CMD_NAV_VTOL_TAKEOFF,        // MAV_CMD
                                             _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,         // MAV_FRAME
                                            0.0,0.0, 0.0,
                                            0.0,  //Yaw angle in degrees
                                            homePosition.latitude(), //latitude
                                           homePosition.longitude(),
                                            30.0,             //
                                            true,                           // autoContinue
                                            false,                          // isCurrentItem
                                            pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
        pMissionItems->append(item);
    }


    if (_cameraTrigger && _cameraTriggerDistanceFact.rawValue().toDouble() > 0)
    {
        int j = 0;
        QGeoCoordinate coordCamera   = _gridPoints[0].value<QGeoCoordinate>();
        QGeoCoordinate camStartCoord = _gridPoints[0].value<QGeoCoordinate>();
        QGeoCoordinate camEndCoord = _gridPoints[0].value<QGeoCoordinate>();
        QGeoCoordinate camEndCoord2   = _gridPoints[0].value<QGeoCoordinate>();
        // Turn into a path
        int turnNumber = 5;
        if(_turnaroundDistFact.rawValue().toDouble() > 0.0){
            turnNumber = 9;
        }
        for (int i = 0; i < _cameraTriggerPoint.count(); i++) {

            if(_cameraTriggerPoint.at (i)== CAMPOINT){
                if(j > _gridPoints.count () - 1){
                    break;
                }
                QGeoCoordinate coord = _gridPoints[j].value<QGeoCoordinate>();
                coordCamera = coord;
                double altitude = _gridAltitudeFact.rawValue().toDouble();

                if((j > _gridPoints.count() - turnNumber)&&(_gridSpacingFactAdd1.rawValue().toDouble()!=0)){
                    altitude = altitude + altitude * 0.2;
                }

                if (j % 4 == 0 && _hideturnaround){
                    MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                        MAV_CMD_NAV_WAYPOINT,           // MAV_CMD
                                                        _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,  // MAV_FRAME
                                                       1.0, 0.0, 0.0, 0.0,             // param 1-4
                                                        coord.latitude(),
                                                        coord.longitude(),
                                                        altitude,
                                                        true,                           // autoContinue
                                                        false,                          // isCurrentItem
                                                        pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
                    pMissionItems->append(item);
                    j++;
                }else{
                    MissionItem* item1 = new MissionItem(seqNum++,                       // sequence number
                                                        MAV_CMD_NAV_WAYPOINT,           // MAV_CMD
                                                        _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,  // MAV_FRAME
                                                       0.0, 0.0, 0.0, 0.0,             // param 1-4
                                                        coord.latitude(),
                                                        coord.longitude(),
                                                        altitude,
                                                        true,                           // autoContinue
                                                        false,                          // isCurrentItem
                                                        pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
                    pMissionItems->append(item1);
                    j++;
                }
            }else if(_cameraTriggerPoint.at (i) == CAMTURNON){
                int num = 0;
                if((i + 1) < _cameraTriggerNumber.count ()){
                    num = _cameraTriggerNumber.at (i+1);
                }
                double dist = _cameraTriggerDistanceFact.rawValue().toDouble() ;
                double param1 = dist + num / 1000.0;

                MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                    MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // MAV_CMD
                                                    MAV_FRAME_MISSION,              // MAV_FRAME
                                                    param1,   // trigger distance
                                                    _cameraTriggerNumber.at (i),
                                                    coordCamera.latitude (),
                                                    coordCamera.longitude (),
                                                    camEndCoord.latitude (),
                                                    camEndCoord.longitude (),
                                                    coordCamera.azimuthTo (camEndCoord),   // param 2-7
                                                    true,                           // autoContinue
                                                    false,                          // isCurrentItem
                                                    pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
                pMissionItems->append(item);
                if(_hideturnaround){
                    MissionItem* item1 = new MissionItem(seqNum++,
                                                        MAV_CMD_DO_VTOL_TRANSITION,
                                                        MAV_FRAME_MISSION,
                                                        4.0,  //plane
                                                        0.0,0.0,0.0,0.0,0.0,0.0,
                                                        true,
                                                        false,
                                                        pMissionItems
                                );
                    pMissionItems->append(item1);
                }

            }else if(_cameraTriggerPoint.at (i) == CAMTURNOFF){
                MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                    MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // MAV_CMD
                                                    MAV_FRAME_MISSION,              // MAV_FRAME
                                                    0.0,   // trigger distance
                                                    0.0,
                                                    0.0,
                                                    0.0,
                                                    camEndCoord2.latitude (),
                                                    camEndCoord2.longitude (),
                                                    0.0,   // param 2-7
                                                    true,                           // autoContinue
                                                    false,                          // isCurrentItem
                                                    pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
                pMissionItems->append(item);
//                    if(j!= _gridPoints.count()){
//                        MissionItem* item2 = new MissionItem(seqNum++,
//                                                            MAV_CMD_DO_VTOL_TRANSITION,
//                                                            MAV_FRAME_MISSION,
//                                                            3.0,  //hover
//                                                            0.0,0.0,0.0,0.0,0.0,0.0,
//                                                            true,
//                                                            false,
//                                                            pMissionItems
//                                    );
//                        pMissionItems->append(item2);
//                    }

            }else if(_cameraTriggerPoint.at (i) == CAMSELSTARTFPOINT){
                camStartCoord = _gridPoints[j].value<QGeoCoordinate>();
                if(j + 1 < _gridPoints.size ()){
                    camEndCoord = _gridPoints[j + 1].value<QGeoCoordinate>();
                }
                j++;
            }else if(_cameraTriggerPoint.at (i) == CAMSELENDFPOINT){
                camEndCoord2 = _gridPoints[j].value<QGeoCoordinate>();
                j++;
            }
        }
    }else{
        // Turn into a path
        int j = 0;
        int turnNumber = 5;
        if(_turnaroundDistFact.rawValue().toDouble() > 0.0){
            turnNumber = 9;
        }

        for (int i = 0; i < _cameraTriggerPoint.count(); i++) {

            if(_cameraTriggerPoint.at (i)== CAMPOINT){
                if(j > _gridPoints.count () - 1){
                    break;
                }
                QGeoCoordinate coord = _gridPoints[j].value<QGeoCoordinate>();
                double altitude = _gridAltitudeFact.rawValue().toDouble();

                if((j > _gridPoints.count() - turnNumber)&&(_gridSpacingFactAdd1.rawValue().toDouble()!=0)){
                    altitude = altitude + altitude * 0.2;
                }

                if(j % 4 == 0 && _hideturnaround){
                    MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                        MAV_CMD_NAV_WAYPOINT,           // MAV_CMD
                                                        _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,  // MAV_FRAME
                                                        1.0, 0.0, 0.0, 0.0,             // param 1-4
                                                        coord.latitude(),
                                                        coord.longitude(),
                                                        altitude,
                                                        true,                           // autoContinue
                                                        false,                          // isCurrentItem
                                                        pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
                    pMissionItems->append(item);
                    j++;
                }else{
                    MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                        MAV_CMD_NAV_WAYPOINT,           // MAV_CMD
                                                        _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,  // MAV_FRAME
                                                        0.0, 0.0, 0.0, 0.0,             // param 1-4
                                                        coord.latitude(),
                                                        coord.longitude(),
                                                        altitude,
                                                        true,                           // autoContinue
                                                        false,                          // isCurrentItem
                                                        pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
                    pMissionItems->append(item);
                    j++;
                }

            }else if(_cameraTriggerPoint.at (i) == CAMTURNON &&_hideturnaround){
                    MissionItem* item1 = new MissionItem(seqNum++,
                                                        MAV_CMD_DO_VTOL_TRANSITION,
                                                        MAV_FRAME_MISSION,
                                                        4.0,  //plane
                                                        0.0,0.0,0.0,0.0,0.0,0.0,
                                                        true,
                                                        false,
                                                        pMissionItems
                                );
                    pMissionItems->append(item1);
            }else if(_cameraTriggerPoint.at (i) == CAMTURNOFF){
//                if(j!= _gridPoints.count()){
//                    MissionItem* item2 = new MissionItem(seqNum++,
//                                                    MAV_CMD_DO_VTOL_TRANSITION,
//                                                    MAV_FRAME_MISSION,
//                                                    3.0,  //hover
//                                                    0.0,0.0,0.0,0.0,0.0,0.0,
//                                                    true,
//                                                    false,
//                                                    pMissionItems
//                            );
//                    pMissionItems->append(item2);
//                }
            }else if(_cameraTriggerPoint.at (i) == CAMSELSTARTFPOINT){
                j++;
            }else if(_cameraTriggerPoint.at (i) == CAMSELENDFPOINT){
                j++;
            }
        }
    }

        if(_vehicle && _vehicle->homePositionAvailable ()){
            QGeoCoordinate homePosition = _vehicle->homePosition();
            MissionItem* item1 = new MissionItem(seqNum++,                       // sequence number
                                                MAV_CMD_NAV_LOITER_TO_ALT,        // MAV_CMD
                                                 _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,         // MAV_FRAME
                                                0.0,
                                                80.0,
                                                0.0,
                                                1.0,  //Yaw angle in degrees
                                                homePosition.latitude(), //latitude
                                               homePosition.longitude(),
                                                50,             //
                                                true,                           // autoContinue
                                                false,                          // isCurrentItem
                                                pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
            pMissionItems->append(item1);
            MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                MAV_CMD_NAV_VTOL_LAND,        // MAV_CMD
                                                 _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,         // MAV_FRAME
                                                0.0,0.0,
                                                _returntolaunch ? 1.0 : 0.0,
                                                0.0,  //Yaw angle in degrees
                                                homePosition.latitude(), //latitude
                                               homePosition.longitude(),
                                                30,             //
                                                true,                           // autoContinue
                                                false,                          // isCurrentItem
                                                pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
            pMissionItems->append(item);
        }
        return pMissionItems;
}

void SurveyMissionItem::_cameraTriggerChanged(void)
{
    setDirty(true);
    if (_gridPoints.count()) {
        // If we have grid turn on/off camera trigger will add/remove two camera trigger mission items
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
    emit cameraShotsChanged(cameraShots());
}

int SurveyMissionItem::cameraShots(void) const
{
    return _cameraTrigger ? _cameraShots : 0;
}

void SurveyMissionItem::_cameraValueChanged(void)
{
    emit cameraValueChanged();
}
