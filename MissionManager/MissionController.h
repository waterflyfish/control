/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MissionController_H
#define MissionController_H

#include "PlanElementController.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"
#include "MavlinkQmlSingleton.h"
#include "VisualMissionItem.h"
#include "SurveyMissionItem.h"

#include <QHash>
#include <QDomDocument>

class CameraModelItem;
class CoordinateVector;

Q_DECLARE_LOGGING_CATEGORY(MissionControllerLog)

typedef QPair<VisualMissionItem*,VisualMissionItem*> VisualItemPair;
typedef QHash<VisualItemPair, CoordinateVector*> CoordVectHashTable;
class MissionController : public PlanElementController
{
    Q_OBJECT

public:
    MissionController(QObject* parent = NULL);
    ~MissionController();

    enum  VersionMission_Xh{
        SystemMission_Xh,
        LngLatMission_Xh,
        MGRSMission_Xh,
        UTMMission_Xh,
        DMSMission_Xh,
        DDMission_Xh,
        UNKnowMission_Xh
    };
    Q_PROPERTY(QGeoCoordinate       plannedHomePosition  READ plannedHomePosition                        NOTIFY plannedHomePositionChanged)
    Q_PROPERTY(QmlObjectListModel*  visualItems          READ visualItems                                NOTIFY visualItemsChanged)
    Q_PROPERTY(bool                 hideVisualItems      READ hideVisualItems   WRITE setHideVisualItems NOTIFY hideVisualItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  complexVisualItems   READ complexVisualItems                         NOTIFY complexVisualItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  cameraFeedItems      READ cameraFeedItems                            NOTIFY cameraFeedItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  cameraFeedItemsLocal READ cameraFeedItemsLocal                       NOTIFY cameraFeedItemsLocalChanged)
    Q_PROPERTY(bool                 hideCameraItems      READ hideCameraItems   WRITE setHideCameraItems NOTIFY hideCameraItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  cameraModelItems     READ cameraModelItems                           NOTIFY cameraModelItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  waypointLines        READ waypointLines                              NOTIFY waypointLinesChanged)
    Q_PROPERTY(QmlObjectListModel*  polygonn             READ polygonn                                   NOTIFY polygonnChanged)
    Q_PROPERTY(QmlObjectListModel*  lineString           READ lineString                                 NOTIFY lineStringChanged)
    //===============================================================================
    Q_PROPERTY(QmlObjectListModel*  waypointLinesAdd       READ waypointLinesAdd                              NOTIFY waypointLinesAddChanged)
    Q_PROPERTY(QmlObjectListModel*  visualItemsflag        READ visualItemsflag  WRITE setVisualItemsflag     NOTIFY visualItemsflagChanged)
    Q_PROPERTY(double                 lati                 READ lati                WRITE setLati   NOTIFY latiChanged)
    Q_PROPERTY(double                 lagi                 READ lagi                WRITE setLagi   NOTIFY lagiChanged)
    Q_PROPERTY(double                 multi                READ multi               WRITE setMulti   NOTIFY multiChanged)
    //===============================================================================
    Q_PROPERTY(double               missionDistance     READ missionDistance                            NOTIFY missionDistanceChanged)
    Q_PROPERTY(double               missionMaxTelemetry READ missionMaxTelemetry                        NOTIFY missionMaxTelemetryChanged)
    Q_PROPERTY(double               cruiseDistance      READ cruiseDistance                             NOTIFY cruiseDistanceChanged)
    Q_PROPERTY(double               hoverDistance       READ hoverDistance                              NOTIFY hoverDistanceChanged)
    Q_PROPERTY(QVariantList         polygonPathh                READ polygonPathh                    NOTIFY polygonPathhChanged)
    Q_INVOKABLE void removeMissionItem(int index);

    Q_INVOKABLE void sendAltitudee(double alti);
    Q_INVOKABLE void sendMoveOffset(double x1,double y1);
    Q_INVOKABLE void sendMovePoint(int x1,QGeoCoordinate coordinate);
    Q_INVOKABLE void sendLarge(double x1,QGeoCoordinate coordinate);
    Q_INVOKABLE void sendSmall(double x1,QGeoCoordinate coordinate);
     Q_INVOKABLE void sendShun(double x1,QGeoCoordinate coordinate);
     Q_INVOKABLE void sendNi(double x1,QGeoCoordinate coordinate);
     Q_INVOKABLE void sendFlagg(void);
    Q_INVOKABLE void sendMissionItem(int  index );
   //===========================================================
    Q_INVOKABLE void loadMoveLine(void);
     Q_INVOKABLE void loadRecover(void);
    Q_INVOKABLE void loadMoveLineAdd(void);
     Q_INVOKABLE void loadRecoverAdd(void);
    //========================================================
    /// Add a new simple mission item to the list
    ///     @param i: index to insert at
    /// @return Sequence number for new item
    Q_INVOKABLE int insertSimpleMissionItem(QGeoCoordinate coordinate, int i);

    /// Add a new complex mission item to the list
    ///     @param i: index to insert at
    /// @return Sequence number for new item
    Q_INVOKABLE int insertComplexMissionItem(QGeoCoordinate coordinate, int i);

    /// Save Camera Model to File
    Q_INVOKABLE bool operateCameraModel(QString currentCameraName, QList<double> cameraPara, int operate = 0);



    /// Add a new camera feedback item to the list
    ///     @param currentItem : index to insert at
    /// @return Sequence number for new item
    Q_INVOKABLE void _insertCameraFeedBackItem(int currentItem);

    QmlObjectListModel* cameraFeedItems (void);
    QmlObjectListModel* cameraFeedItemsLocal (void);
    void setHideCameraItems(bool hide = false);
    bool hideCameraItems   (void);
    QmlObjectListModel* cameraModelItems(void);
    Q_INVOKABLE CameraModelItem cameraModelItemList(void);
    // Overrides from PlanElementController
    void start              (bool editMode) final;
    void loadFromVehicle    (void) final;
    void sendToVehicle      (QGeoCoordinate itemCoodinate) final;
    void loadFromFilePicker (void) final;
    void loadFromFile       (const QString& filename) final;
    void saveToFilePicker   (void) final;
    void saveToFile         (const QString& filename) final;
    void removeAll          (void) final;
    bool syncInProgress     (void) const final;
    bool dirty              (void) const final;
    void setDirty           (bool dirty) final;
    Vehicle* getVehicle(void);
    QString fileExtension(void) const final;

    // Property accessors
QVariantList polygonPathh(void) { return _polygonPathh; }
    QGeoCoordinate      plannedHomePosition (void);
    QmlObjectListModel* visualItems         (void) { if(!_hideVisualItems) return _visualItems; else return NULL; }
    void setHideVisualItems(bool hide = false);
    bool hideVisualItems   (void);
    QmlObjectListModel* visualItemsflag        (void) { return _visualItemsflag; }
    QmlObjectListModel* waypointLinesAdd       (void) { return &_waypointLinesAdd; }
    QmlObjectListModel* complexVisualItems  (void) { return _complexItems; }
    QmlObjectListModel* polygonn  (void) { return _polygonn; }
    QmlObjectListModel* lineString (void) { return _lineString;}
    QmlObjectListModel* waypointLines       (void) { return &_waypointLines; }

    //===============================
    double lati(void) { return _lati; }
    void setLati(double lati);
    void setVisualItemsflag(QmlObjectListModel* visualItemsflag);
    double lagi(void) { return _lagi; }
    void setLagi(double lagi);
    double multi(void) { return _multi; }
    void setMulti(double multi);
    //=====================================
    double  missionDistance         (void) const { return _missionDistance; }
    double  missionMaxTelemetry     (void) const { return _missionMaxTelemetry; }
    double  cruiseDistance          (void) const { return _cruiseDistance; }
    double  hoverDistance           (void) const { return _hoverDistance; }

    void setMissionDistance         (double missionDistance );
    void setMissionMaxTelemetry     (double missionMaxTelemetry);
    void setCruiseDistance          (double cruiseDistance );
    void setHoverDistance           (double hoverDistance );

    static const char* jsonSimpleItemsKey;  ///< Key for simple items in a json file

signals:
    //===========================
    void visualItemsflagChanged(void);
    void waypointLinesAddChanged(void);
    void latiChanged(double lati);
    void lagiChanged(double lagi);
    void multiChanged(double multi);
    void polygonPathhChanged();
    //===============================
    void plannedHomePositionChanged(QGeoCoordinate plannedHomePosition);
    void activeVehicleMissionItemsTempChanged(double latitude, double longtitude, double altitude);
    void visualItemsChanged(void);
    void complexVisualItemsChanged(void);
    void lineStringChanged(void);
    void polygonnChanged(void);
    void cameraFeedItemsChanged(void);
    void cameraFeedItemsLocalChanged(void);
    void cameraModelItemsChanged(void);
    void hideCameraItemsChanged(void);
    void hideVisualItemsChanged(void);
    void waypointLinesChanged(void);
    void newItemsFromVehicle(void);
    void missionDistanceChanged(double missionDistance);
    void missionMaxTelemetryChanged(double missionMaxTelemetry);
    void cruiseDistanceChanged(double cruiseDistance);
    void hoverDistanceChanged(double hoverDistance);

private slots:
    void _newMissionItemsAvailableFromVehicle();
    void _itemCommandChanged(void);
    void _activeVehicleHomePositionAvailableChanged(bool homePositionAvailable);
    void _activeVehicleMissionItemsTempChanged(double latitude, double longtitude, double altitude);
    void _activeVehicleHomePositionChanged(const QGeoCoordinate& homePosition);
    void _inProgressChanged(bool inProgress);
    void _currentMissionItemChanged(int sequenceNumber);
    void _recalcWaypointLines(void);
    void _recalcAltitudeRangeBearing(void);
    void _homeCoordinateChanged(void);
    //====================================================
    void _ffCoordinatee(bool flage);
    void _itemCoordinateChanged(const QGeoCoordinate& coordinate);
    void _itemCoordinateAddChanged(const QGeoCoordinate& coordinate);
    void _recalcWaypointLinesAdd(void);

    void _ackTimeout();
    void _ackTimeStop(bool receive);

private:
    void _recalcSequence(void);
    void _recalcChildItems(void);
    void _recalcAll(void);
    void _initAllVisualItems(void);
    void _deinitAllVisualItems(void);
    void _initVisualItem(VisualMissionItem* item);
    void _deinitVisualItem(VisualMissionItem* item);
    void _setupActiveVehicle(Vehicle* activeVehicle, bool forceLoadFromVehicle);
    static void _calcPrevWaypointValues(double homeAlt, VisualMissionItem* currentItem, VisualMissionItem* prevItem, double* azimuth, double* distance, double* altDifference);
    static void _calcHomeDist(VisualMissionItem* currentItem, VisualMissionItem* homeItem, double* distance);
    bool _findLastAltitude(double* lastAltitude, MAV_FRAME* frame);
    bool _findLastAcceptanceRadius(double* lastAcceptanceRadius);
    void _addPlannedHomePosition(QmlObjectListModel* visualItems, bool addToCenter);
    double _normalizeLat(double lat);
    double _normalizeLon(double lon);
    bool _loadJsonMissionFile(const QByteArray& bytes, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems, QString& errorString);
    /* Changed by chu.fumin 2016122414 start : load items from excel */
    bool _loadExcelMissionFile(const QString& fileName, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems, QString& errorString);
    /* Changed by chu.fumin 2016122414 end : load items from excel */
    bool _loadTextMissionFile(QTextStream& stream, QmlObjectListModel* visualItems, QString& errorString);
    int _nextSequenceNumber(void);
    //edit by wang.lichen load the kml file
    bool _loadKmlMissionFile(const QString& fileName, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems, QString& errorString);
    void parsePolygon(QXmlStreamReader &reader, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems);
    void parseLineString(QXmlStreamReader &reader, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems);
    //==========================================================
    // Overrides from PlanElementController
    void _activeVehicleBeingRemoved(void) final;
    void _activeVehicleSet(void) final;

private:
    QmlObjectListModel* _visualItems;
    bool                _hideVisualItems;
    //==================================
    QmlObjectListModel* _visualItemsflag;
    QmlObjectListModel  _waypointLinesAdd;
    CoordVectHashTable  _linesTableAdd;
    double                _lati;
    double                _lagi;
    double                _multi;
    QmlObjectListModel* _complexItems;
    QmlObjectListModel* _polygonn;
    QmlObjectListModel* _lineString;
    QmlObjectListModel* _cameraFeedItems;
    QmlObjectListModel* _cameraFeedItemsLocal;
    QmlObjectListModel  _cameraModelItems;
    bool                _cameraModelChanged;
    bool                _hideCameraItems;
    QmlObjectListModel  _waypointLines;
    CoordVectHashTable  _linesTable;
    LinkInterface*      _dedicatedLink;
    bool                _firstItemsFromVehicle;
    bool                _missionItemsRequested;
    bool                _queuedSend;
    double              _missionDistance;
    double              _missionMaxTelemetry;
    double              _cruiseDistance;
    double              _hoverDistance;
 	QVariantList    _polygonPathh;
    static const char*  _settingsGroup;
    static const char*  _jsonMavAutopilotKey;
    static const char*  _jsonComplexItemsKey;
    static const char*  _jsonPlannedHomePositionKey;

     QTimer*             _ackTimeoutTimer;
     int                        _retrycount;
     int                        _currentMissionItem;
};

class CameraModelItem : public QObject
{
    Q_OBJECT

public:
    struct CameraModel{
        char name[50];
        double sensorWidth;
        double sensorHeight;
        double imageWidth;
        double imageHeight;
        double focalLength;
        double  frontal;
        double side;
        double gridSpacingAdd1;
        double turnaroundDist;
        double gridChangeSpeed;
    };

    Q_PROPERTY(QString    nameText     READ     nameText        WRITE    setNameText       NOTIFY    nameTextChanged)
    Q_PROPERTY(double     sensorWidth  READ     sensorWidth     WRITE    setSensorWidth    NOTIFY    sensorWidthChanged)
    Q_PROPERTY(double     sensorHeight READ     sensorHeight    WRITE    setSensorHeight   NOTIFY    sensorHeightChanged)
    Q_PROPERTY(double     imageWidth   READ     imageWidth      WRITE    setImageWidth     NOTIFY    imageWidthChanged)
    Q_PROPERTY(double     imageHeight  READ     imageHeight     WRITE    setImageHeight    NOTIFY    imageHeightChanged)
    Q_PROPERTY(double     focalLength    READ      focalLength       WRITE    setFocalLength      NOTIFY    focalLengthChanged)
    Q_PROPERTY(double     frontal              READ      frontal                 WRITE    setFrontal                NOTIFY    frontalChanged)
    Q_PROPERTY(double     side                   READ      side                      WRITE    setSide                      NOTIFY    sideChanged)
    Q_PROPERTY(double gridspacingadd1 READ gridspacingadd1 WRITE setGridspacingadd1 NOTIFY gridspacingadd1Changed)
    Q_PROPERTY(double turnarounddist READ turnarounddist WRITE setTurnarounddist NOTIFY turnarounddistChanged)
    Q_PROPERTY(double gridchangespeed READ gridchangespeed WRITE setGridchangespeed NOTIFY gridchangespeedChanged)
    Q_PROPERTY(bool fixedValueIsAltitude READ fixedValueIsAltitude WRITE setFixedValueIsAltitude NOTIFY fixedValueIsAltitudeChanged)
    Q_PROPERTY(double gridAltitude READ gridAltitude WRITE setGridAltitude NOTIFY gridAltitudeChanged)
    Q_PROPERTY(double groundResolution READ groundResolution WRITE setGroundResolution NOTIFY groundResolutionChanged)
    QString nameText(void){ return _nameText;}
    double sensorWidth(void){ return _sensorWidth;}
    double sensorHeight(void){ return _sensorHeight;}
    double imageWidth(void){ return _imageWidth;}
    double imageHeight(void){ return _imageHeight;}
    double focalLength(void){ return _focalLength;}
    double frontal(void){return _frontal;}
    double side(void){return _side;}
    double gridspacingadd1(void){return _gridSpacingAdd1;}
    double turnarounddist(void){return _turnaroundDist;}
    double gridchangespeed(void){return _gridChangeSpeed;}
    bool   fixedValueIsAltitude(void){return _fixedValueIsAltitude;}
    double gridAltitude(void){return _gridAltitude;}
    double groundResolution(void){return _groundResolution;}

    void setNameText(QString nameText){_nameText = nameText;}
    void setSensorWidth(double sensorWidth){ _sensorWidth = sensorWidth;}
    void setSensorHeight(double sensorHeight){ _sensorHeight = sensorHeight;}
    void setImageWidth(double imageWidth){ _imageWidth = imageWidth;}
    void setImageHeight(double imageHeight){ _imageHeight = imageHeight;}
    void setFocalLength(double focalLength){ _focalLength = focalLength;}
    void setFrontal(double frontal){_frontal = frontal;}
    void setSide(double side){_side = side;}
    void setGridspacingadd1(double gridSpacingAdd1){_gridSpacingAdd1 = gridSpacingAdd1;}
    void setTurnarounddist(double turnaroundDist){_turnaroundDist = turnaroundDist;}
    void setGridchangespeed(double gridchangespeed){_gridChangeSpeed = gridchangespeed;}
    void setFixedValueIsAltitude(bool fixedValueIsAltitude){_fixedValueIsAltitude = fixedValueIsAltitude;}
    void setGridAltitude(double gridAltitude){_gridAltitude = gridAltitude;}
    void setGroundResolution(double groundResolution){_groundResolution = groundResolution;}

    CameraModelItem()
        : QObject(NULL)
        ,_nameText()
        , _sensorWidth(0.0)
        , _sensorHeight(0.0)
        , _imageWidth(0.0)
        ,_imageHeight(0.0)
        ,_focalLength(0.0)
        ,_frontal(0.0)
        ,_side(0.0)
        ,_gridSpacingAdd1(0.0)
        ,_turnaroundDist(0.0)
        ,_gridChangeSpeed(0.0)
        ,_fixedValueIsAltitude(false)
        ,_gridAltitude(0.0)
        ,_groundResolution(0.0)
    {}

    CameraModelItem(const CameraModelItem& cameraModelItem)
        : QObject(NULL)
        ,_nameText(cameraModelItem._nameText)
        , _sensorWidth(cameraModelItem._sensorWidth)
        , _sensorHeight(cameraModelItem._sensorHeight)
        , _imageWidth(cameraModelItem._imageWidth)
        ,_imageHeight(cameraModelItem._imageHeight)
        ,_focalLength(cameraModelItem._focalLength)
        ,_frontal(cameraModelItem._frontal)
        ,_side(cameraModelItem._side)
        ,_gridSpacingAdd1(cameraModelItem._gridChangeSpeed)
        ,_turnaroundDist(cameraModelItem._turnaroundDist)
        ,_gridChangeSpeed(cameraModelItem._gridChangeSpeed)
        ,_fixedValueIsAltitude(cameraModelItem._fixedValueIsAltitude)
        ,_gridAltitude(cameraModelItem._gridAltitude)
        ,_groundResolution(cameraModelItem._groundResolution)
    {}

    CameraModelItem& operator=(const CameraModelItem& cameraModelItem)
    {
        _nameText = cameraModelItem._nameText;
        _sensorWidth = cameraModelItem._sensorWidth;
        _sensorHeight = cameraModelItem._sensorHeight;
        _imageWidth = cameraModelItem._imageWidth;
        _imageHeight = cameraModelItem._imageHeight;
        _focalLength = cameraModelItem._focalLength;
        _frontal = cameraModelItem._frontal;
        _side = cameraModelItem._side;
        _gridSpacingAdd1 = cameraModelItem._gridSpacingAdd1;
        _turnaroundDist = cameraModelItem._turnaroundDist;
        _gridChangeSpeed = cameraModelItem._gridChangeSpeed;
        _fixedValueIsAltitude = cameraModelItem._fixedValueIsAltitude;
        _gridAltitude = cameraModelItem._gridAltitude;
        _groundResolution = cameraModelItem._groundResolution;
        return *this;
    }

    bool operator==(const CameraModelItem& cameraModelItem) const
    {
        return (cameraModelItem._nameText == _nameText
                &&cameraModelItem._sensorWidth == _sensorWidth
                &&cameraModelItem._sensorHeight == _sensorHeight
                &&cameraModelItem._imageWidth == _imageWidth
                &&cameraModelItem._imageHeight == _imageHeight
                &&cameraModelItem._focalLength == _focalLength
                &&cameraModelItem._frontal == _frontal
                &&cameraModelItem._side == _side
                &&cameraModelItem._gridSpacingAdd1 == _gridSpacingAdd1
                &&cameraModelItem._turnaroundDist == _turnaroundDist
                &&cameraModelItem._gridChangeSpeed == _gridChangeSpeed
                &&cameraModelItem._fixedValueIsAltitude == _fixedValueIsAltitude
                &&cameraModelItem._gridAltitude == _gridAltitude
                &&cameraModelItem._groundResolution == _groundResolution);
    }
signals:
    void nameTextChanged(void);
    void sensorWidthChanged(void);
    void sensorHeightChanged(void);
    void imageWidthChanged(void);
    void imageHeightChanged(void);
    void focalLengthChanged(void);
    void frontalChanged(void);
    void sideChanged(void);
    void gridspacingadd1Changed(void);
    void turnarounddistChanged(void);
    void gridchangespeedChanged(void);
    void fixedValueIsAltitudeChanged(void);
    void gridAltitudeChanged(void);
    void groundResolutionChanged(void);

private:
    QString _nameText;
    double _sensorWidth;
    double _sensorHeight;
    double _imageWidth;
    double _imageHeight;
    double _focalLength;
    double _frontal;
    double _side;
    double _gridSpacingAdd1;
    double _turnaroundDist;
    double _gridChangeSpeed;
    bool   _fixedValueIsAltitude;
    double _gridAltitude;
    double _groundResolution;
};

#endif
