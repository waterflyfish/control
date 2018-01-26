/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef SurveyMissionItem_H
#define SurveyMissionItem_H

#include "ComplexMissionItem.h"
#include "MissionItem.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(SurveyMissionItemLog)

class SurveyMissionItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    SurveyMissionItem(Vehicle* vehicle, QObject* parent = NULL);

    const SurveyMissionItem& operator=(const SurveyMissionItem& other);
//============================================================================
    //edit by wang.lichen
    Q_PROPERTY(Fact*                gridSpacingAdd1             READ gridSpacingAdd1                CONSTANT)
//=============================================================================
    Q_PROPERTY(Fact*                gridAltitude                READ gridAltitude                   CONSTANT)
    Q_PROPERTY(Fact*                gridChangeSpeedFact      READ gridChangeSpeedFact         CONSTANT)
    Q_PROPERTY(bool                 gridAltitudeRelative        MEMBER _gridAltitudeRelative        NOTIFY gridAltitudeRelativeChanged)
    Q_PROPERTY(Fact*                gridAngle                   READ gridAngle                      CONSTANT)
    Q_PROPERTY(Fact*                gridSpacing                 READ gridSpacing                    CONSTANT)
    Q_PROPERTY(Fact*                turnaroundDist              READ turnaroundDist                 CONSTANT)
    Q_PROPERTY(bool                 cameraTrigger               MEMBER _cameraTrigger               NOTIFY cameraTriggerChanged)
    Q_PROPERTY(bool                 hideCameraItems             READ hideCameraItems                WRITE setHideCameraItems    NOTIFY hideCameraItemsChanged)
    Q_PROPERTY(bool                 hideturnaround              READ hideturnaround                 WRITE setHideturnaround     NOTIFY hideturnaroundChanged)
    Q_PROPERTY(bool                 landatLaunch                READ landatLaunch                   WRITE setLandatLaunch       NOTIFY landatLaunchChanged)
    Q_PROPERTY(Fact*                cameraTriggerDistance       READ cameraTriggerDistance          CONSTANT)
    Q_PROPERTY(Fact*                groundResolution            READ groundResolution               CONSTANT)
    Q_PROPERTY(Fact*                frontalOverlap              READ frontalOverlap                 CONSTANT)
    Q_PROPERTY(Fact*                sideOverlap                 READ sideOverlap                    CONSTANT)
    Q_PROPERTY(Fact*                cameraSensorWidth           READ cameraSensorWidth              CONSTANT)
    Q_PROPERTY(Fact*                cameraSensorHeight          READ cameraSensorHeight             CONSTANT)
    Q_PROPERTY(Fact*                cameraResolutionWidth       READ cameraResolutionWidth          CONSTANT)
    Q_PROPERTY(Fact*                cameraResolutionHeight      READ cameraResolutionHeight         CONSTANT)
    Q_PROPERTY(Fact*                cameraFocalLength           READ cameraFocalLength              CONSTANT)
    Q_PROPERTY(QVariantList         polygonPath                 READ polygonPath                    NOTIFY polygonPathChanged)
    Q_PROPERTY(QVariantList         gridPoints                  READ gridPoints                     NOTIFY gridPointsChanged)
    Q_PROPERTY(QVariantList         gisFlag                     READ gisFlag                        NOTIFY gisFlagChanged)
    Q_PROPERTY(QVariantList         linePoints                  READ linePoints                     NOTIFY linePointsChanged)
    Q_PROPERTY(QmlObjectListModel*         pathList                 READ pathList                   NOTIFY pathListChanged)
    Q_PROPERTY(int                  cameraShots                 READ cameraShots                    NOTIFY cameraShotsChanged)
    Q_PROPERTY(double               coveredArea                 READ coveredArea                    NOTIFY coveredAreaChanged)
    Q_PROPERTY(bool                 fixedValueIsAltitude        MEMBER _fixedValueIsAltitude        NOTIFY fixedValueIsAltitudeChanged)
    Q_PROPERTY(bool                 cameraOrientationLandscape  MEMBER _cameraOrientationLandscape  NOTIFY cameraOrientationLandscapeChanged)
    Q_PROPERTY(bool                 manualGrid                  MEMBER _manualGrid                  NOTIFY manualGridChanged)
    Q_PROPERTY(QString              camera                      MEMBER _camera                      NOTIFY cameraChanged)
    Q_INVOKABLE void clearPolygon(void);
    Q_INVOKABLE void addPolygonCoordinate(const QGeoCoordinate coordinate);
    Q_INVOKABLE void adjustPolygonCoordinate(int vertexIndex, const QGeoCoordinate coordinate);
    Q_INVOKABLE QmlObjectListModel* returnList(void);

    QVariantList polygonPath(void) { return _polygonPath; }
    QVariantList gridPoints (void) { return _gridPoints; }
    QVariantList gisFlag (void) { return _gisFlag; }
    QVariantList linePoints (void) {return _linePoints;}
    QmlObjectListModel* pathList(void){return _pathList;}
    Fact* gridAltitude              (void) { return &_gridAltitudeFact; }
    Fact* gridChangeSpeedFact(void) { return &_gridChangeSpeedFact; }
    Fact* gridAngle                 (void) { return &_gridAngleFact; }
    Fact* gridSpacing               (void) { return &_gridSpacingFact; }
    //============================================================
    //edit by wang.lichen
    Fact* gridSpacingAdd1(void)     { return &_gridSpacingFactAdd1; }
    Fact* turnaroundDist            (void) { return &_turnaroundDistFact; }
    Fact* cameraTriggerDistance     (void) { return &_cameraTriggerDistanceFact; }
    Fact* groundResolution          (void) { return &_groundResolutionFact; }
    Fact* frontalOverlap            (void) { return &_frontalOverlapFact; }
    Fact* sideOverlap               (void) { return &_sideOverlapFact; }
    Fact* cameraSensorWidth         (void) { return &_cameraSensorWidthFact; }
    Fact* cameraSensorHeight        (void) { return &_cameraSensorHeightFact; }
    Fact* cameraResolutionWidth     (void) { return &_cameraResolutionWidthFact; }
    Fact* cameraResolutionHeight    (void) { return &_cameraResolutionHeightFact; }
    Fact* cameraFocalLength         (void) { return &_cameraFocalLengthFact; }

    int     cameraShots(void) const;
    double  coveredArea(void) const { return _coveredArea; }

    // Overrides from ComplexMissionItem

    double              complexDistance     (void) const final { return _surveyDistance; }
    int                 lastSequenceNumber  (void) const final;
    /// Update the mission items associated with the complex item.
    /// must be called before call function getMissionItems
    void updateMissionItems(bool update = false) final;
    QmlObjectListModel* getMissionItems     (QGeoCoordinate itemCoordinate) const final;
    QmlObjectListModel* getVtolMissionItems (void)const final;

    /* Changed by chu.fumin 2016122711 start : dms degree,minute,second convert */
    /// Load the complex mission item from Coordinate of VariantList,
    ///     @param polygonPath Coordinate item
    ///     @param seq Coordinate seq
    /// @return true: load success, false: load failed
    /// Item is dms, example :
    /// - <i>LEGAL</i> (these are all equivalent)
    /// - 070:00:45, 70:01:15W+0:0.5, 70:01:15W-0:0:30W, W70:01:15+0:0:30E
    /// - <i>ILLEGAL</i> (the exception thrown explains the problem)
    /// - 70:01:15W+0:0:15N, W70:01:15+W0:0:15
    bool loadDMS(const QVariantList &polygonPath, unsigned int seq) final;
    /* Changed by chu.fumin 2016122711 end : dms degree,minute,second convert */
//=========================================================
    bool loadKMLPolygon(const QVariantList &polygonPath, unsigned int seq) ;
    bool loadKMLLineString(const QVariantList &lineString, unsigned int seq);
    //======================================================
    bool                load                (const QJsonObject& complexObject, QString& errorString) final;
    double              greatestDistanceTo  (const QGeoCoordinate &other) const final;

    QVariantList&    getCameraTriggerPointList(){ if(!_hideCameraItems) return  _cameraTriggerPointList; else return _cameraTriggerPointListNull;}
    bool             hideCameraItems(){ return _hideCameraItems; }
    bool             hideturnaround() {return _hideturnaround;}     //vtol turnaround
    bool             landatLaunch() {return _returntolaunch;}//Return to launch
    void             setHideCameraItems(bool hideCameraItems){ _hideCameraItems = hideCameraItems;}
    void             setHideturnaround(bool hideturnaround){
        _hideturnaround = hideturnaround;
        emit hideturnaroundChanged (hideturnaround);
    }
    void             setLandatLaunch(bool returntolaunch)
    {
        _returntolaunch = returntolaunch;
        emit landatLaunchChanged(returntolaunch);
    }

    // Overrides from VisualMissionItem

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
    bool            specifiesCoordinate     (void) const final;
    QString         commandDescription      (void) const final { return "Survey"; }
    QString         commandName             (void) const final { return "Survey"; }
    QString         abbreviation            (void) const final { return "S"; }
    QGeoCoordinate  coordinate              (void) const final { return _coordinate; }
    QGeoCoordinate  exitCoordinate          (void) const final { return _exitCoordinate; }
    int             sequenceNumber          (void) const final { return _sequenceNumber; }
    //===========================================================================
    bool            flage                   (void) const final {return _flage;}
    bool coordinateHasRelativeAltitude      (void) const final { return _gridAltitudeRelative; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return _gridAltitudeRelative; }
    bool exitCoordinateSameAsEntry          (void) const final { return false; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final;
    void setSequenceNumber  (int sequenceNumber) final;
    //============================================================
   void setFlage             (bool flage) final;
    //============================================================
    void setTurnaroundDist  (double dist) { _turnaroundDistFact.setRawValue(dist); }
    void save               (QJsonObject& saveObject) const final;

signals:
    void polygonPathChanged                 (void);
    void pathListChanged                    (void);
    void gisFlagChanged                     (void);
    void altitudeChanged                    (double altitude);
    void linePointsChanged                  (void);
    void gridAngleChanged                   (double gridAngle);
    void gridPointsChanged                  (void);
    void cameraTriggerChanged               (bool cameraTrigger);
    void hideCameraItemsChanged             (bool hideCameraItems);
    void hideturnaroundChanged              (bool hideturnaround);
    void landatLaunchChanged                (bool returntolaunch);
    void gridAltitudeRelativeChanged        (bool gridAltitudeRelative);
    void cameraShotsChanged                 (int cameraShots);
    void coveredAreaChanged                 (double coveredArea);
    void cameraValueChanged                 (void);
    void fixedValueIsAltitudeChanged        (bool fixedValueIsAltitude);
    void gridTypeChanged                    (QString gridType);
    void cameraOrientationLandscapeChanged  (bool cameraOrientationLandscape);
    void cameraChanged                      (QString camera);
    void manualGridChanged                  (bool manualGrid);

private slots:
    void _cameraTriggerChanged(void);
    void _generateGrid(void);//QVariant value
    void _updateCoordinateAltitude(void);
//private:
//    QList<QPointF> _convexPolygon(const QList<QPointF>& polygon);
//    void _swapPoints(QList<QPointF>& points, int index1, int index2);
//    qreal _ccw(QPointF pt1, QPointF pt2, QPointF pt3);
//    qreal _dp(QPointF pt1, QPointF pt2);
//    double _clampGridAngle90(double gridAngle);
private:
   QmlObjectListModel* _pathList;
    enum CameraTrigger{
        CAMPOINT,
        CAMTURNON,
        CAMTURNOFF,
        CAMSELSTARTFPOINT,
        CAMSELENDFPOINT,
        CAMUNKNOW
    };

    class CameraTriggerPoint {
    public:
        CameraTriggerPoint(CameraTrigger cameraTrigger,
                           QPointF startPoint, ///< QPointF() : xp(0), yp(0)
                           QPointF endPoint ,///< QPointF() : xp(0), yp(0)
                           unsigned int numberCamera = 0
                ):
            _cameraTrigger(cameraTrigger)
          , _startPoint(startPoint)
          , _endPoint(endPoint)
          ,_numberCamera(numberCamera)
        {
        }

        CameraTriggerPoint(CameraTrigger cameraTrigger,
                           QPointF startPoint///< QPointF() : xp(0), yp(0)
                ):
            _cameraTrigger(cameraTrigger)
          , _startPoint(startPoint)
          , _endPoint(0,0)
          ,_numberCamera(0)
        {
        }

        CameraTrigger getCameraTrigger(){
            return _cameraTrigger;
        }

        QPointF getStartPoint(){
            return _startPoint;
        }

        QPointF getEndPoint(){
            return _endPoint;
        }

        unsigned int getNumberCamera(){
            return _numberCamera;
        }

        void setCameraTrigger(CameraTrigger cameraTrigger){
            _cameraTrigger = cameraTrigger;
        }

        void setStartPoint(QPointF startPoint){
            _startPoint = startPoint;
        }

        void setEndPoint(QPointF endPoint){
            _endPoint = endPoint;
        }

        void setNumberCamera(unsigned int numberCamera){
            _numberCamera = numberCamera;
        }

    private:
        CameraTrigger _cameraTrigger;
        QPointF        _startPoint;    ///< CAMPOINT : point , CAMTURNON : camera turn on start point, CAMTURNOFF : null
        QPointF        _endPoint;      ///< CAMTURNOFF : camera turn off end point, else : null
        unsigned int  _numberCamera;    ///< camera shots of number
    };
    void _clear(void);
    void _setExitCoordinate(const QGeoCoordinate& coordinate);
    void _clearGrid(void);
    void _updateGenerateGrid(bool update = false);
    void _gridGenerator(const QList<QPointF>& polygonPoints, QList<QPointF>& gridPoints, QGeoCoordinate &tangentOrigin, bool update = false);
    QPointF _rotatePoint(const QPointF& point, const QPointF& origin, double angle);
    void _intersectLinesWithRect(const QList<QLineF>& lineList, const QRectF& boundRect, QList<QLineF>& resultLines);
    void _intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon, QList<QLineF>& resultLines);
    void _adjustLineDirection(const QList<QLineF>& lineList, QList<QLineF>& resultLines);
    void _setSurveyDistance(double surveyDistance);
    void _setCameraShots(int cameraShots);
    void _setCoveredArea(double coveredArea);
    void _cameraValueChanged(void);

    int             _sequenceNumber;
    //=================================
    bool                _flage;
    bool                flaag;
    //============================
    bool            _dirty;
    QVariantList    _polygonPath;
    QVariantList    _gisFlag;
    QVariantList    _linePoints;
    QVariantList    _gridPoints;
    QGeoCoordinate  _coordinate;
    QGeoCoordinate  _exitCoordinate;
    double          _altitude;
    bool            _cameraTrigger;
    bool            _hideCameraItems;
    bool            _hideturnaround;
    bool            _returntolaunch;
    bool            _gridAltitudeRelative;
    bool            _manualGrid;
    QString         _camera;
    bool            _cameraOrientationLandscape;
    bool            _fixedValueIsAltitude;

    double          _surveyDistance;
    int             _cameraShots;
    double          _coveredArea;

    Fact            _gridAltitudeFact;
    Fact            _gridChangeSpeedFact;
    Fact            _gridAngleFact;
    Fact            _gridSpacingFact;
    Fact            _turnaroundDistFact;
    Fact            _cameraTriggerDistanceFact;
    Fact            _groundResolutionFact;
    Fact            _frontalOverlapFact;
    Fact            _sideOverlapFact;
    Fact            _cameraSensorWidthFact;
    Fact            _cameraSensorHeightFact;
    Fact            _cameraResolutionWidthFact;
    Fact            _cameraResolutionHeightFact;
    Fact            _cameraFocalLengthFact;

    static QMap<QString, FactMetaData*> _metaDataMap;

    //==========================================
    //edit by wang.lichen
    Fact            _gridSpacingFactAdd1;
    FactMetaData    _gridAltitudeMetaData;
    FactMetaData    _changeSpeedMetaData;
    QList<CameraTrigger> _cameraTriggerPoint;
    QList<double>        _cameraTriggerNumber;
    QList<QPointF>      _cameraTriggerPointLocal;
    QVariantList        _cameraTriggerPointList;
    QVariantList        _cameraTriggerPointListNull;
    static const char* _jsonTypeKey;
    static const char* _jsonPolygonObjectKey;
    static const char* _jsonIdKey;
    static const char* _jsonGridObjectKey;
    static const char* _jsonGridAltitudeKey;
    static const char* _jsonGridAltitudeRelativeKey;
    static const char* _jsonGridAngleKey;
    static const char* _jsonGridSpacingKey;
    static const char* _jsonGridSpacingAdd1Key;
    static const char* _jsonTurnaroundDistKey;
    static const char* _jsonCameraTriggerKey;
    static const char* _jsonCameraTriggerDistanceKey;
    static const char* _jsonGroundResolutionKey;
    static const char* _jsonFrontalOverlapKey;
    static const char* _jsonSideOverlapKey;
    static const char* _jsonCameraSensorWidthKey;
    static const char* _jsonCameraSensorHeightKey;
    static const char* _jsonCameraResolutionWidthKey;
    static const char* _jsonCameraResolutionHeightKey;
    static const char* _jsonCameraFocalLengthKey;
    static const char* _jsonManualGridKey;
    static const char* _jsonCameraObjectKey;
    static const char* _jsonCameraNameKey;
    static const char* _jsonCameraOrientationLandscapeKey;
    static const char* _jsonFixedValueIsAltitudeKey;


    static const char* _gridAltitudeFactName;
    static const char* _gridAngleFactName;
    static const char* _gridSpacingFactName;
    static const char* _turnaroundDistFactName;
    static const char* _cameraTriggerDistanceFactName;
    static const char* _groundResolutionFactName;
    static const char* _frontalOverlapFactName;
    static const char* _sideOverlapFactName;
    static const char* _cameraSensorWidthFactName;
    static const char* _cameraSensorHeightFactName;
    static const char* _cameraResolutionWidthFactName;
    static const char* _cameraResolutionHeightFactName;
    static const char* _cameraFocalLengthFactName;

    static const char* _complexType;
};

#endif
