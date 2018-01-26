/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MissionController.h"
#include "MultiVehicleManager.h"
#include "MissionManager.h"
#include "CoordinateVector.h"
#include "FirmwarePlugin.h"
#include "QGCApplication.h"
#include "SimpleMissionItem.h"
#include "SurveyMissionItem.h"
#include "JsonHelper.h"
#include "ParameterManager.h"
#include "QGroundControlQmlGlobal.h"

#ifndef __mobile__
#include "QGCFileDialog.h"
#endif

/* Changed by chu.fumin 2016122711 start : dms degree,minute,second convert */
#include "xlsxdocument.h"
#include <iostream>
#include <exception>
#include <string>
#include <GeographicLib/UTMUPS.hpp>
#include <GeographicLib/MGRS.hpp>
#include <GeographicLib/AlbersEqualArea.hpp>
#include <GeographicLib/DMS.hpp>

using namespace std;
using namespace GeographicLib;
/* Changed by chu.fumin 2016122711 end : dms degree,minute,second convert */

QGC_LOGGING_CATEGORY(MissionControllerLog, "MissionControllerLog")

const char* MissionController::jsonSimpleItemsKey = "items";

const char* MissionController::_settingsGroup =                 "MissionController";
const char* MissionController::_jsonMavAutopilotKey =           "MAV_AUTOPILOT";
const char* MissionController::_jsonComplexItemsKey =           "complexItems";
const char* MissionController::_jsonPlannedHomePositionKey =    "plannedHomePosition";

MissionController::MissionController(QObject *parent)
    : PlanElementController(parent)
    , _visualItems(NULL)
    , _hideVisualItems(false)
    , _complexItems(NULL)
    , _polygonn(new QmlObjectListModel(this))
    , _lineString(new QmlObjectListModel(this))
    , _cameraFeedItems(new QmlObjectListModel(this))
    , _cameraFeedItemsLocal(new QmlObjectListModel(this))
    , _cameraModelChanged(false)
    , _hideCameraItems(false)
    , _firstItemsFromVehicle(false)
    , _missionItemsRequested(false)
    , _queuedSend(false)
    , _missionDistance(0.0)
    , _missionMaxTelemetry(0.0)
    , _cruiseDistance(0.0)
    , _hoverDistance(0.0)
    ,_ackTimeoutTimer(NULL)
{
    _ackTimeoutTimer = new QTimer(this);
    _ackTimeoutTimer->setInterval(1000);
    _ackTimeoutTimer->setSingleShot(true);

    connect(_ackTimeoutTimer,&QTimer::timeout,this,&MissionController::_ackTimeout);
}

MissionController::~MissionController()
{

}

void MissionController::start(bool editMode)
{
    qCDebug(MissionControllerLog) << "start editMode" << editMode;

    PlanElementController::start(editMode);

    // We start with an empty mission
    _visualItems = new QmlObjectListModel(this);
    _visualItemsflag = new QmlObjectListModel(this);
    _addPlannedHomePosition(_visualItems, false /* addToCenter */);
    _initAllVisualItems();
}

// Called when new mission items have completed downloading from Vehicle
void MissionController::_newMissionItemsAvailableFromVehicle(void)
{
    //qCDebug(MissionControllerLog) << "_newMissionItemsAvailableFromVehicle";

    if (!_editMode || _missionItemsRequested || _visualItems->count() == 1) {
        // Fly Mode:
        //      - Always accepts new items from the vehicle so Fly view is kept up to date
        // Edit Mode:
        //      - Either a load from vehicle was manually requested or
        //      - The initial automatic load from a vehicle completed and the current editor is empty

        QmlObjectListModel* newControllerMissionItems = new QmlObjectListModel(this);
        const QList<MissionItem*>& newMissionItems = _activeVehicle->missionManager()->missionItems();

        //qCDebug(MissionControllerLog) << "loading from vehicle: count"<< _visualItems->count();
        foreach(const MissionItem* missionItem, newMissionItems) {
            newControllerMissionItems->append(new SimpleMissionItem(_activeVehicle, *missionItem, this));
        }

        _deinitAllVisualItems();

        _visualItems->deleteListAndContents();
        _visualItems = newControllerMissionItems;

        if (!_activeVehicle->firmwarePlugin()->sendHomePositionToVehicle() || _visualItems->count() == 0) {
            _addPlannedHomePosition(_visualItems,true /* addToCenter */);
        }

        _missionItemsRequested = false;

        _initAllVisualItems();
        emit newItemsFromVehicle();
    }
}

void MissionController::loadFromVehicle(void)
{
    Vehicle* activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (activeVehicle) {
        _missionItemsRequested = true;
        activeVehicle->missionManager()->requestMissionItems();
    }
}

void MissionController::sendToVehicle(QGeoCoordinate itemCooedinate)
{
    if (_activeVehicle) {
        _cameraFeedItemsLocal->clearAndDeleteContents ();
        emit cameraFeedItemsLocalChanged();

        // Convert to MissionItems so we can send to vehicle
        QList<MissionItem*> missionItems;

        for (int i=0; i<_visualItems->count(); i++) {
            VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
            if (visualItem->isSimpleItem()) {
                missionItems.append(new MissionItem(qobject_cast<SimpleMissionItem*>(visualItem)->missionItem()));
            } else {
                ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(visualItem);
                complexItem->updateMissionItems(true);
                QmlObjectListModel* complexMissionItems = NULL;
                if(_activeVehicle->vtol())
                    complexMissionItems = complexItem->getVtolMissionItems();
                else
                     complexMissionItems = complexItem->getMissionItems(itemCooedinate);
                for (int j=0; j<complexMissionItems->count(); j++) {
                    missionItems.append(new MissionItem(*qobject_cast<MissionItem*>(complexMissionItems->get(j))));
                }
                delete complexMissionItems;

                //This is for editor mission
                QVariantList& cameraPoints = complexItem->getCameraTriggerPointList ();
                for(int k = 0; k < cameraPoints.count (); k++){
                    QGeoCoordinate coord = cameraPoints[k].value<QGeoCoordinate>();
                    SimpleMissionItem * newItem = new SimpleMissionItem(_activeVehicle, this);
                    newItem->setCoordinate(coord);
                    newItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT);
                    newItem->setSequenceNumber(k);
                    newItem->setDefaultsForCommand();
                    _cameraFeedItemsLocal->append(newItem);
                }
            }
        }
        emit cameraFeedItemsLocalChanged();
        _activeVehicle->missionManager()->writeMissionItems(missionItems);
        _visualItems->setDirty(false);

        for (int i=0; i<missionItems.count(); i++) {
            delete missionItems[i];
        }
    }
}

int MissionController::_nextSequenceNumber(void)
{
    if (_visualItems->count() == 0) {
        qWarning() << "Internal error: Empty visual item list";
        return 0;
    } else {
        VisualMissionItem* lastItem = qobject_cast<VisualMissionItem*>(_visualItems->get(_visualItems->count() - 1));

        if (lastItem->isSimpleItem()) {
            return lastItem->sequenceNumber() + 1;
        } else {
            return qobject_cast<ComplexMissionItem*>(lastItem)->lastSequenceNumber() + 1;
        }
    }
}

int MissionController::insertSimpleMissionItem(QGeoCoordinate coordinate, int i)
{
    int sequenceNumber = _nextSequenceNumber();
    SimpleMissionItem * newItem = new SimpleMissionItem(_activeVehicle, this);
    newItem->setSequenceNumber(sequenceNumber);
    newItem->setCoordinate(coordinate);
    newItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT);
    _initVisualItem(newItem);
    if (_visualItems->count() == 1) {
        newItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF);
    }
    newItem->setDefaultsForCommand();
    if ((MAV_CMD)newItem->command() == MAV_CMD_NAV_WAYPOINT) {
        double lastValue;
        MAV_FRAME lastFrame;

        if (_findLastAcceptanceRadius(&lastValue)) {
            newItem->missionItem().setParam2(lastValue);
        }
        if (_findLastAltitude(&lastValue, &lastFrame)) {
            newItem->missionItem().setFrame(lastFrame);
            newItem->missionItem().setParam7(lastValue);
        }
    }
    _visualItems->insert(i, newItem);

    _recalcAll();

    return newItem->sequenceNumber();
}

int MissionController::insertComplexMissionItem(QGeoCoordinate coordinate, int i)
{
    int sequenceNumber = _nextSequenceNumber();
    SurveyMissionItem* newItem = new SurveyMissionItem(_activeVehicle, this);
    newItem->setSequenceNumber(sequenceNumber);
    newItem->setCoordinate(coordinate);
    _initVisualItem(newItem);

    _visualItems->insert(i, newItem);
    _complexItems->append(newItem);

    _recalcAll();

    return newItem->sequenceNumber();
}

void MissionController::_insertCameraFeedBackItem(int currentItem)
{
    if (!_activeVehicle) {
        return ;
    }

    if(currentItem < 0){
        //_cameraFeedItems->clear();
        //emit cameraFeedItemsChanged();
        return ;
    }

    MissionManager* missionManager = _activeVehicle->missionManager();
    const QList<MissionItem> &missionItem = missionManager->cameraFeedbackItems ();
    if(missionItem.size () < currentItem){
        return ;
    }

    SimpleMissionItem * newItem = new SimpleMissionItem(_activeVehicle, missionItem[currentItem], this);
    if(!_cameraFeedItems->contains (newItem)){
        _cameraFeedItems->append(newItem);
        emit cameraFeedItemsChanged();
    }else {
        delete newItem;
        newItem = NULL;
    }
}
QmlObjectListModel* MissionController::cameraFeedItems (void){
        if(!_hideCameraItems){
            return _cameraFeedItems;
        }else{
            return NULL;
        }
}

QmlObjectListModel* MissionController::cameraFeedItemsLocal (void){
    qDebug()<<__FILE__<<" "<<__LINE__<<" "<<_cameraFeedItemsLocal->count ();
    //return NULL;
    if(!_hideCameraItems){
        return _cameraFeedItemsLocal;
    }else{
        return NULL;
    }
}

QmlObjectListModel *MissionController::cameraModelItems(void){
    if(_cameraModelChanged){
        return &_cameraModelItems;
    }
    _cameraModelItems.clearAndDeleteContents ();
    QDir tempDir(QDir::currentPath() + "/log");
    if(!tempDir.exists () || !QFile(QDir::currentPath() + "/log/CameraModelParaData.xml").exists ()){
        return &_cameraModelItems;
    }

    QFile xmlFile(QDir::currentPath() + "/log/CameraModelParaData.xml");
    if(!xmlFile.exists () || xmlFile.size () == 0 || !xmlFile.open(QIODevice::ReadOnly)){
        return &_cameraModelItems;
    }

    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    if (xml.hasError()) {
        qWarning() << __LINE__<<"Badly formed XML" << xml.errorString();
        return &_cameraModelItems;
    }

    QString cameraName;
    double sensorWidth;
    double sensorHeight;
    double imageWidth;
    double imageHeight;
    double focalLength;
    double frontal;
    double side;
    double architecture;
    double turnaroundDist;
    double speed;
    bool fixedvalueisaltitude;
    double gridaltitude;
    double groundresolution;

    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "cameras") {
                // Just skip over for now
            } else if (elementName == "version") {
                // Just skip over for now
            } else if (elementName == "cameraframe_version_major") {
                // Just skip over for now
            } else if (elementName == "cameraframe_version_minor") {
                // Just skip over for now
            } else if (elementName == "cameraframe_group") {
                if (!xml.attributes().hasAttribute("name")
                        || !xml.attributes().hasAttribute("sensorwidth")
                        || !xml.attributes().hasAttribute("sensorheight")
                        || !xml.attributes().hasAttribute("imagewidth")
                        || !xml.attributes().hasAttribute("imageheight")
                        || !xml.attributes().hasAttribute("focallength")
                        || !xml.attributes().hasAttribute("frontal")
                        || !xml.attributes().hasAttribute("side")) {
                    qWarning() <<  __LINE__<<"Badly formed XML";
                    break;
                }

                bool convertOk = false;
                cameraName = xml.attributes().value("name").toString();
                if(cameraName.size () == 0){
                    qWarning() <<  __LINE__<<"Badly formed XML";
                    break;
                }
                sensorWidth = xml.attributes().value("sensorwidth").toDouble (&convertOk);
                if (!convertOk) {
                    qWarning() <<  __LINE__<<"Badly formed XML";
                    break;
                }
                sensorHeight = xml.attributes().value("sensorheight").toDouble (&convertOk);
                if (!convertOk) {
                    qWarning() <<  __LINE__<<"Badly formed XML";
                    break;
                }
                imageWidth = xml.attributes().value("imagewidth").toDouble (&convertOk);
                if (!convertOk) {
                    qWarning() <<  __LINE__<<"Badly formed XML";
                    break;
                }
                imageHeight = xml.attributes().value("imageheight").toDouble (&convertOk);
                if (!convertOk) {
                    qWarning() << __LINE__<< "Badly formed XML";
                    break;
                }
                focalLength = xml.attributes().value("focallength").toDouble (&convertOk);
                if (!convertOk) {
                    qWarning() << __LINE__<< "Badly formed XML";
                    break;
                }
                frontal = xml.attributes().value("frontal").toDouble(&convertOk);
                if(!convertOk){
                    qWarning()<< __LINE__<<"Badly formed XML";
                    break;
                }
                side = xml.attributes().value("side").toDouble(&convertOk);
                if(!convertOk){
                    qWarning()<< __LINE__<<"Badly formed XML";
                    break;
                }
                architecture = xml.attributes().value("architecture").toDouble(&convertOk);
                if(!convertOk){
                    qWarning()<<__LINE__<<"Badly formed XML";
                    break;
                }
                turnaroundDist = xml.attributes().value("turnarounddist").toDouble(&convertOk);
                if(!convertOk){
                    qWarning()<<__LINE__<<"Badly formed XML";
                    break;
                }
                speed = xml.attributes().value("speed").toDouble(&convertOk);
                if(!convertOk){
                    qWarning()<<__LINE__<<"Badly formed XML";
                    break;
                } 
                fixedvalueisaltitude = static_cast<bool> (xml.attributes().value("fixedValueIsAltitude").toInt(&convertOk,10));
                if(!convertOk){
                    qWarning()<<__LINE__<<"Badly formed XML";
                    break;
                }
                qDebug()<<"fixedvalueisaltitude    "<<fixedvalueisaltitude;
                gridaltitude = xml.attributes().value("gridAltitude").toDouble(&convertOk);
                if(!convertOk){
                    qWarning()<<__LINE__<<"Badly formed XML";
                    break;
                }
                groundresolution = xml.attributes().value("groundResolution").toDouble(&convertOk);
                if(!convertOk){
                    qWarning()<<__LINE__<<"Badly formed XML";
                    break;
                }
                CameraModelItem *cam = new CameraModelItem();
                if(cam == NULL){
                    continue;
                }
                cam->setNameText (cameraName);
                cam->setSensorWidth (sensorWidth);
                cam->setSensorHeight (sensorHeight);
                cam->setImageHeight (imageHeight);
                cam->setImageWidth  (imageWidth);
                cam->setFocalLength (focalLength);
                cam->setFrontal(frontal);
                cam->setSide(side);
                cam->setGridspacingadd1(architecture);
                cam->setTurnarounddist(turnaroundDist);
                cam->setGridchangespeed(speed);
                cam->setFixedValueIsAltitude(fixedvalueisaltitude);
                cam->setGridAltitude(gridaltitude);
                cam->setGroundResolution(groundresolution);
                _cameraModelItems.append (cam);
            }
        } else if (xml.isEndElement()) {
            QString elementName = xml.name().toString();
            if (elementName == "cameras") {
                 break;
            } else if (elementName == "cameraframe_group") {
            }
        }
        xml.readNext();
    }

    _cameraModelChanged = true;
    return &_cameraModelItems;
}

CameraModelItem MissionController::cameraModelItemList(void){
    CameraModelItem cameraModelList;
    return cameraModelList;
}


bool MissionController::operateCameraModel(QString currentCameraName, QList<double> cameraPara, int operate)
{
    if(currentCameraName.isEmpty () || cameraPara.size () < 5){
        return false;
    }

    QDir tempDir(QDir::currentPath() + "/log");
    if(!tempDir.exists ()){
        if(tempDir.mkdir(QDir::currentPath() + "/log")){
            return false;
        }
    }

    QFile file(QDir::currentPath() + "/log/CameraModelParaData.xml");
    if(!file.exists ()){
        file.open(QFile::ReadWrite);
        if(!file.isOpen ()){
            return false;
        }
        file.close ();
    }

    if(file.size () == 0){
        file.open(QFile::ReadWrite | QFile::Text);
        if(!file.isOpen ()){
            return false;
        }
        QDomDocument docDom;
        QDomProcessingInstruction instructions;
        instructions = docDom.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\"");
        docDom.appendChild(instructions);
        QDomElement roots = docDom.createElement("cameras");

        docDom.appendChild(roots);
        QDomText texts = docDom.createTextNode("");
        roots.appendChild(texts);
        QTextStream out(&file);
        docDom.save(out,4);
        file.flush ();
        file.close ();
    }

    QFile docFile(QDir::currentPath() + "/log/CameraModelParaData.xml");
    if(!docFile.exists () || !docFile.open(QIODevice::ReadWrite | QFile::Text)){
        return false;
    }

    QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;

    if (!doc.setContent(&docFile, false, &errorStr, &errorLine, &errorColumn)) {
        qDebug()<<"add setcontent error..."<<errorStr<<" : "<<errorLine<<" : "<<errorColumn;
        docFile.resize (0);
        return false;
    }

    QDomElement root = doc.documentElement();
    if(root.isNull())
    {
        if(operate == 1){
            docFile.close();
            return false;
        }
        root = doc.createElement("cameras");
    }
    docFile.close ();

    if(operate == 1){
        QDomNodeList lists = root.childNodes ();
        for(int i = 0; i < lists.size (); i++){
            if(lists.at (i).toElement ().attribute ("name") == currentCameraName){
                root.removeChild(lists.at(i));
                if(!docFile.open(QIODevice::WriteOnly | QIODevice::Truncate)){
                    return false;
                }

                if(!docFile.isOpen ()){
                    return false;
                }

                if(docFile.openMode () != (QIODevice::WriteOnly | QIODevice::Truncate)){
                    docFile.close();
                    return false;
                }

                QTextStream out(&docFile);
                out.setCodec("utf-8");
                doc.save(out,4);
                docFile.close();
                _cameraModelChanged = false;
                return true;
            }
        }
        return false;
    }

    QDomElement element_root = doc.createElement("cameraframe_group");
    QDomAttr name_id = doc.createAttribute("name");
    QDomAttr sensorwidth_id = doc.createAttribute("sensorwidth");
    QDomAttr sensorheight_id = doc.createAttribute("sensorheight");
    QDomAttr imagewidth_id = doc.createAttribute("imagewidth");
    QDomAttr imageheight_id = doc.createAttribute("imageheight");
    QDomAttr focallength_id = doc.createAttribute("focallength");
    QDomAttr frontal_id = doc.createAttribute("frontal");
    QDomAttr side_id  = doc.createAttribute("side");
    QDomAttr gridSpacingAdd1_id = doc.createAttribute("architecture");
    QDomAttr turnaroundDist_id = doc.createAttribute("turnarounddist");
    QDomAttr gridChangeSpeed_id = doc.createAttribute("speed");
    QDomAttr fixedValueIsAltitude_id = doc.createAttribute("fixedValueIsAltitude");
    QDomAttr gridAltitude_id = doc.createAttribute("gridAltitude");
    QDomAttr groundResolution_id = doc.createAttribute("groundResolution");

    name_id.setValue(currentCameraName);
    sensorwidth_id.setValue(QString::number(cameraPara.at (0)));
    sensorheight_id.setValue(QString::number(cameraPara.at (1)));
    imagewidth_id.setValue(QString::number(cameraPara.at (2)));
    imageheight_id.setValue(QString::number(cameraPara.at (3)));
    focallength_id.setValue(QString::number(cameraPara.at (4)));
    frontal_id.setValue(QString::number(cameraPara.at(5)));
    side_id.setValue(QString::number(cameraPara.at(6)));
    gridSpacingAdd1_id.setValue(QString::number(cameraPara.at(7)));
    turnaroundDist_id.setValue(QString::number(cameraPara.at(8)));
    gridChangeSpeed_id.setValue(QString::number(cameraPara.at(9)));
    fixedValueIsAltitude_id.setValue(QString::number(cameraPara.at(10)));
    gridAltitude_id.setValue(QString::number(cameraPara.at(11)));
    groundResolution_id.setValue(QString::number(cameraPara.at(12)));

    element_root.setAttributeNode(name_id);
    element_root.setAttributeNode(sensorwidth_id);
    element_root.setAttributeNode(sensorheight_id);
    element_root.setAttributeNode(imagewidth_id);
    element_root.setAttributeNode(imageheight_id);
    element_root.setAttributeNode(focallength_id);
    element_root.setAttributeNode(frontal_id);
    element_root.setAttributeNode(side_id);
    element_root.setAttributeNode(gridSpacingAdd1_id);
    element_root.setAttributeNode(turnaroundDist_id);
    element_root.setAttributeNode(gridChangeSpeed_id);
    element_root.setAttributeNode(fixedValueIsAltitude_id);
    element_root.setAttributeNode(gridAltitude_id);
    element_root.setAttributeNode(groundResolution_id);

    root.appendChild(element_root);

    if(!docFile.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        return false;
    }

    if(!docFile.isOpen ()){
        return false;
    }

    if(docFile.openMode () != (QIODevice::WriteOnly | QIODevice::Truncate)){
        docFile.close();
        return false;
    }

    QTextStream out(&docFile);
    out.setCodec("utf-8");
    doc.save(out, 4);
    docFile.close();
    CameraModelItem *cam = new CameraModelItem();
    if(cam == NULL){
        return false;
    }
    cam->setNameText (currentCameraName);
    cam->setSensorWidth (cameraPara.at (0));
    cam->setSensorHeight (cameraPara.at (1));
    cam->setImageWidth  (cameraPara.at (2));
    cam->setImageHeight (cameraPara.at (3));
    cam->setFocalLength (cameraPara.at (4));
    cam->setFrontal(cameraPara.at(5));
    cam->setSide(cameraPara.at(6));
    cam->setGridspacingadd1(cameraPara.at(7));
    cam->setTurnarounddist(cameraPara.at(8));
    cam->setGridchangespeed(cameraPara.at(9));
    cam->setFixedValueIsAltitude(cameraPara.at(10));
    cam->setGridAltitude(cameraPara.at(11));
    cam->setGroundResolution(cameraPara.at(12));

    _cameraModelItems.append (cam);
    _cameraModelChanged = true;
    return true;
}

void MissionController::setHideCameraItems(bool hide)
{
    if(_hideCameraItems != hide){
        _hideCameraItems = hide;
        emit cameraFeedItemsChanged();
    }
}
bool MissionController::hideCameraItems(void)
{
    return _hideCameraItems;
}

void MissionController::setHideVisualItems(bool hide)
{
    if(_hideVisualItems != hide){
        _hideVisualItems = hide;
        emit visualItemsChanged();
    }
}
bool MissionController::hideVisualItems (void)
{
    return _hideVisualItems;
}
void MissionController::removeMissionItem(int index)
{
    VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->removeAt(index));

    _deinitVisualItem(item);
    if (!item->isSimpleItem()) {
        ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(_complexItems->removeOne(item));
        if (!complexItem) {
            qWarning() << "Complex item missing";
        }
    }
    item->deleteLater();

    _recalcAll();

    // Set the new current item

    if (index >= _visualItems->count()) {
        index--;
    }
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item =  qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        item->setIsCurrentItem(i == index);
    }
    _visualItems->setDirty(true);
}

void MissionController::removeAll(void)
{
    if (_visualItems) {
        _deinitAllVisualItems();
        _visualItems->deleteListAndContents();
        _visualItems = new QmlObjectListModel(this);
        _addPlannedHomePosition(_visualItems, false /* addToCenter */);
        _initAllVisualItems();
        _visualItems->setDirty(true);

    }
    if(_cameraFeedItemsLocal){
        _cameraFeedItemsLocal->deleteListAndContents();
        _cameraFeedItemsLocal = new QmlObjectListModel(this);
        emit cameraFeedItemsLocalChanged ();
    }
    if(_cameraFeedItems){
        _cameraFeedItems->deleteListAndContents();
        _cameraFeedItems = new QmlObjectListModel(this);
        emit cameraFeedItemsChanged ();
    }
}

/* Changed by chu.fumin 2016122711 start : dms degree,minute,second convert */
bool MissionController::_loadExcelMissionFile(const QString& fileName, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems, QString& errorString){
#ifndef __mobile__
    if(fileName.isEmpty () || !visualItems || !complexItems){
        errorString = tr("_loadExcelMissionFile error : arg is error.");
        return false;
    }
    QXlsx::Document xlsx(fileName);
    int complexCount = 1;
    foreach(QString sheetName, xlsx.sheetNames())
    {
        QXlsx::Worksheet *sheet = static_cast<QXlsx::Worksheet *>(xlsx.sheet(sheetName));
        if(sheet){
            QXlsx::CellRange srange = sheet->dimension();

            if(srange.lastColumn () == 1 && srange.lastRow () == 1){
                qDebug() <<  "_loadExcelMissionFile no data found : " + sheet->sheetName();
                continue;
            }

            QXlsx::Cell *cellLat = sheet->cellAt (1, 1);
            if(!cellLat){
                qDebug() <<  "_loadExcelMissionFile no data found, cellLat is null : " + sheet->sheetName();
                continue;
            }
            if(cellLat->value ().toString ().compare ("Lat", Qt::CaseInsensitive)){
                qDebug() <<  "_loadExcelMissionFile no data found, not Lat format : " + sheet->sheetName();
                continue;
            }
            qDebug()<<cellLat->value ().toString ();

            QXlsx::Cell *cellLng = sheet->cellAt (1, 2);
            if(!cellLng || cellLng->value ().toString ().compare ("Lng", Qt::CaseInsensitive)){
                errorString = "_loadExcelMissionFile error : Lng is null.";
                continue;
            }
            qDebug()<<cellLng->value ().toString ();

            QXlsx::Cell *cellAlt = sheet->cellAt (1, 3);
            if(!cellAlt || cellAlt->value ().toString ().compare ("Alt", Qt::CaseInsensitive)){
                errorString = tr("_loadExcelMissionFile error : Alt is null.");
                continue;
            }
            qDebug()<<cellAlt->value ().toString ();

            QVariantList polygonPath;
            qDebug()<<sheet->dimension ().rowCount ()<<" "<<sheet->dimension ().lastRow ();
            for(int i = 2; i <= sheet->dimension ().lastRow ();i++){
                QXlsx::Cell *latCell = sheet->cellAt (i, 1);
                if(!latCell){
                    errorString =tr( "_loadExcelMissionFile error : sheet Lat is null.");
                    return false;
                }

                QXlsx::Cell *lngCell = sheet->cellAt (i, 2);
                if(!lngCell){
                    errorString = "_loadExcelMissionFile error : sheet Lng is null.";
                    return false;
                }

                QXlsx::Cell *altCell = sheet->cellAt (i, 3);
                if(!altCell){
                    errorString = tr("_loadExcelMissionFile error : sheet Alt is null.");
                    return false;
                }
                if(latCell && lngCell && altCell){

                }else{
                    qDebug()<<"_loadExcelMissionFile no lat lng and alt data found.";
                    break;
                }

                QString latStr = latCell->value ().toString ();
                QString lngStr = lngCell->value ().toString ();
                QString altStr = altCell->value ().toString ();

                bool ok = false;
                double lat = 0.0, lng = 0.0, alt = 1000.0;
                if(latStr.contains ("d", Qt::CaseInsensitive) || latStr.contains ("°", Qt::CaseInsensitive)
                        || latStr.contains ("\'", Qt::CaseInsensitive)
                        || latStr.contains ("\"", Qt::CaseInsensitive)){
                    try {
                        DMS::flag type;
                        lat = DMS::Decode(latStr.toStdString (), type);
                    }
                    catch (const exception& e) {
                        errorString = tr("_loadExcelMissionFile error : ") + e.what ();
                        qDebug()<<" errorString "<<errorString;
                        return false;
                    }
                }else if(latStr.contains (".",  Qt::CaseInsensitive)){
                    lat = latStr.toDouble (&ok);
                    if(!ok){
                        errorString = tr("_loadExcelMissionFile error : latStr toDouble is null.");
                        return false;
                    }
                }else{
                    errorString = tr("_loadExcelMissionFile error : latStr is null.");
                    return false;
                }

                if(lngStr.contains ("d", Qt::CaseInsensitive) || lngStr.contains ("°", Qt::CaseInsensitive)
                        || lngStr.contains ("\'", Qt::CaseInsensitive) 
					    || lngStr.contains ("\"", Qt::CaseInsensitive)){
                    try {
                        DMS::flag type;
                        lng = DMS::Decode(lngStr.toStdString (), type);
                    }
                    catch (const exception& e) {
                        errorString = tr("_loadExcelMissionFile error : ") + e.what ();
                        qDebug()<<" errorString "<<errorString;
                        return false;
                    }
                }else if(lngStr.contains (".",  Qt::CaseInsensitive)){
                    lng = lngStr.toDouble (&ok);
                    if(!ok){
                        errorString = tr("_loadExcelMissionFile error : latStr toDouble is null.");
                        return false;
                    }
                }else{
                    errorString = tr("_loadExcelMissionFile error : latStr is null.");
                    return false;
                }

                alt = altStr.toDouble (&ok);
                if(!ok){
                    errorString = tr("_loadExcelMissionFile error : alt toDouble is null.");
                    //return false;
                    alt = 1000.0;
                }

                QGeoCoordinate coordinate;
                coordinate.setAltitude (alt);
                coordinate.setLatitude (lat);
                coordinate.setLongitude (lng);
                polygonPath << QVariant::fromValue(coordinate);
                qDebug()<<coordinate;
            }
            if(polygonPath.count () > 0){
                SurveyMissionItem* item = new SurveyMissionItem(_activeVehicle, this);
                if (item->loadDMS (polygonPath, complexCount)) {
                    complexCount++;
                    complexItems->append(item);
                    visualItems->append(item);
                } else {
                    delete item;
                    item = NULL;
                    return false;
                }
            }
        }
    }
    _addPlannedHomePosition(visualItems, true /* addToCenter */);
    errorString.clear ();
    return true;
#else
    Q_UNUSED(fileName);
    Q_UNUSED(visualItems);
    Q_UNUSED(complexItems);
    Q_UNUSED(errorString);
    return false;
#endif
}
/* Changed by chu.fumin 2016122711 end : dms degree,minute,second convert */

bool MissionController::_loadJsonMissionFile(const QByteArray& bytes, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems, QString& errorString)
{
    QJsonParseError jsonParseError;
    QJsonDocument   jsonDoc(QJsonDocument::fromJson(bytes, &jsonParseError));

    if (jsonParseError.error != QJsonParseError::NoError) {
        errorString = jsonParseError.errorString();
        return false;
    }

    QJsonObject json = jsonDoc.object();

    // Check for required keys
    QStringList requiredKeys;
    requiredKeys << JsonHelper::jsonVersionKey << _jsonPlannedHomePositionKey;
    if (!JsonHelper::validateRequiredKeys(json, requiredKeys, errorString)) {
        return false;
    }

    // Validate base key types
    QStringList             keyList;
    QList<QJsonValue::Type> typeList;
    keyList << jsonSimpleItemsKey << JsonHelper::jsonVersionKey << JsonHelper::jsonGroundStationKey << _jsonMavAutopilotKey << _jsonComplexItemsKey << _jsonPlannedHomePositionKey;
    typeList << QJsonValue::Array << QJsonValue::String << QJsonValue::String << QJsonValue::Double << QJsonValue::Array << QJsonValue::Object;
    if (!JsonHelper::validateKeyTypes(json, keyList, typeList, errorString)) {
        return false;
    }

    // Version check
    if (json[JsonHelper::jsonVersionKey].toString() != "1.0") {
        errorString = QStringLiteral("QGroundControl does not support this file version");
        return false;
    }

    // Read complex items
    QJsonArray complexArray(json[_jsonComplexItemsKey].toArray());
    qCDebug(MissionControllerLog) << "Json load: complex item count" << complexArray.count();
    for (int i=0; i<complexArray.count(); i++) {
        const QJsonValue& itemValue = complexArray[i];

        if (!itemValue.isObject()) {
            errorString = QStringLiteral("Mission item is not an object");
            return false;
        }

        SurveyMissionItem* item = new SurveyMissionItem(_activeVehicle, this);
        if (item->load(itemValue.toObject(), errorString)) {
            qCDebug(MissionControllerLog) << "Json load: complex item start:stop" << item->sequenceNumber() << item->lastSequenceNumber();
            complexItems->append(item);
        } else {
            return false;
        }
    }

    // Read simple items, interspersing complex items into the full list

    int nextSimpleItemIndex= 0;
    int nextComplexItemIndex= 0;
    int nextSequenceNumber = 1; // Start with 1 since home is in 0
    QJsonArray itemArray(json[jsonSimpleItemsKey].toArray());

    qCDebug(MissionControllerLog) << "Json load: simple item loop start simpleItemCount:ComplexItemCount" << itemArray.count() << complexItems->count();
    do {
        qCDebug(MissionControllerLog) << "Json load: simple item loop nextSimpleItemIndex:nextComplexItemIndex:nextSequenceNumber" << nextSimpleItemIndex << nextComplexItemIndex << nextSequenceNumber;

        // If there is a complex item that should be next in sequence add it in
        if (nextComplexItemIndex < complexItems->count()) {
            SurveyMissionItem* complexItem = qobject_cast<SurveyMissionItem*>(complexItems->get(nextComplexItemIndex));

            if (complexItem->sequenceNumber() == nextSequenceNumber) {
                qCDebug(MissionControllerLog) << "Json load: injecting complex item expectedSequence:actualSequence:" << nextSequenceNumber << complexItem->sequenceNumber();
                visualItems->append(complexItem);
                nextSequenceNumber = complexItem->lastSequenceNumber() + 1;
                nextComplexItemIndex++;
                continue;
            }
        }

        // Add the next available simple item
        if (nextSimpleItemIndex < itemArray.count()) {
            const QJsonValue& itemValue = itemArray[nextSimpleItemIndex++];

            if (!itemValue.isObject()) {
                errorString = QStringLiteral("Mission item is not an object");
                return false;
            }

            SimpleMissionItem* item = new SimpleMissionItem(_activeVehicle, this);
            if (item->load(itemValue.toObject(), errorString)) {
                qCDebug(MissionControllerLog) << "Json load: adding simple item expectedSequence:actualSequence" << nextSequenceNumber << item->sequenceNumber();
                visualItems->append(item);
            } else {
                return false;
            }

            nextSequenceNumber++;
        }
    } while (nextSimpleItemIndex < itemArray.count() || nextComplexItemIndex < complexItems->count());

    if (json.contains(_jsonPlannedHomePositionKey)) {
        SimpleMissionItem* item = new SimpleMissionItem(_activeVehicle, this);

        if (item->load(json[_jsonPlannedHomePositionKey].toObject(), errorString)) {
            visualItems->insert(0, item);
        } else {
            return false;
        }
    } else {
        _addPlannedHomePosition(visualItems, true /* addToCenter */);
    }

    return true;
}

bool MissionController::_loadTextMissionFile(QTextStream& stream, QmlObjectListModel* visualItems, QString& errorString)
{
    bool addPlannedHomePosition = false;

    QString firstLine = stream.readLine();
    const QStringList& version = firstLine.split(" ");

    bool versionOk = false;
    VersionMission_Xh versionMission_Xh = SystemMission_Xh;
    if (version.size() == 3 && version[0] == "QGC" && version[1] == "WPL") {
        if (version[2] == "110") {
            // ArduPilot file, planned home position is already in position 0
            versionOk = true;
            versionMission_Xh = SystemMission_Xh;
        } else if (version[2] == "120") {
            // Old QGC file, no planned home position
            versionOk = true;
            addPlannedHomePosition = true;
            versionMission_Xh = SystemMission_Xh;
        } else if (version[2] == "130"){
            versionOk = true;
            addPlannedHomePosition = true;
            versionMission_Xh = LngLatMission_Xh;
        } else if (version[2] == "140"){
            versionOk = true;
            addPlannedHomePosition = true;
            versionMission_Xh = UNKnowMission_Xh;
        } else if (version[2] == "150"){
            versionOk = true;
            addPlannedHomePosition = true;
            versionMission_Xh = UNKnowMission_Xh;
        }
    }else if(version.size() == 3 && version[0] == "QGC" && version[1] == "COMPLEXWPL"){
        if (version[2] == "130"){
            versionOk = true;
            addPlannedHomePosition = true;
            versionMission_Xh = LngLatMission_Xh;
        }
    }

    if(versionOk && !stream.atEnd ()){
        stream.readLine();
    }
    if (versionOk && versionMission_Xh == SystemMission_Xh) {
        while (!stream.atEnd()) {
            SimpleMissionItem* item = new SimpleMissionItem(_activeVehicle, this);

            if (item->load(stream)) {
                visualItems->append(item);
            } else {
                errorString = QStringLiteral("The mission file is corrupted.");
                return false;
            }
        }
    } else if(versionOk && versionMission_Xh == LngLatMission_Xh){
        bool success = false;
        bool OK = false;
        while (!stream.atEnd()) {
            SimpleMissionItem* item = new SimpleMissionItem(_activeVehicle, this);

            if(item == NULL){
                success = false;
                break;
            }

            const QStringList &wpParams = stream.readLine().split("\t");
            if (wpParams.size() == 4) {
                item->missionItem().setSequenceNumber(wpParams[0].toInt(&OK));
                if(!OK){
                    success = false;
                    break;
                }
                item->missionItem().setIsCurrentItem(false);
                item->missionItem().setFrame(MAV_FRAME_GLOBAL);
                item->missionItem().setCommand(MAV_CMD_NAV_WAYPOINT);
                item->missionItem().setParam1(0.0);
                item->missionItem().setParam2(0.0);
                item->missionItem().setParam3(0.0);
                item->missionItem().setParam4(0.0);
                item->missionItem().setParam5(wpParams[1].toDouble(&OK));
                if(!OK){
                    success = false;
                    break;
                }
                item->missionItem().setParam6(wpParams[2].toDouble(&OK));
                if(!OK){
                    success = false;
                    break;
                }
                item->missionItem().setParam7(wpParams[3].toDouble(&OK));
                if(!OK){
                    success = false;
                    break;
                }
                item->missionItem().setAutoContinue(true);
                visualItems->append(item);
                success = true;
            } else {
                errorString = QStringLiteral("The mission file is corrupted.");
                return false;
            }
        }

        if(!success) {
            errorString = QStringLiteral("The mission file is corrupted.");
            return false;
        }
    } else {
        errorString = QStringLiteral("The mission file is not compatible with this version of QGroundControl.");
        return false;
    }

    if (addPlannedHomePosition || visualItems->count() == 0) {
        _addPlannedHomePosition(visualItems, true /* addToCenter */);

        // Update sequence numbers in DO_JUMP commands to take into account added home position in index 0
        for (int i=1; i<visualItems->count(); i++) {
            SimpleMissionItem* item = qobject_cast<SimpleMissionItem*>(visualItems->get(i));
            if (item && item->command() == MavlinkQmlSingleton::MAV_CMD_DO_JUMP) {
                item->missionItem().setParam1((int)item->missionItem().param1() + 1);
            }
        }
    }

    return true;
}
//===============================================================
//edit by wang.lichen
bool MissionController::_loadKmlMissionFile(const QString& fileName, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems, QString& errorString){
#ifndef __mobile__
    if(_polygonn->count ()){
        qDebug()<<"_polygon : "<<_polygonn;
        _polygonn->deleteListAndContents ();
        _polygonn = new QmlObjectListModel(this);
        emit polygonnChanged ();
    }
    if(_lineString){
        _lineString->deleteListAndContents ();
        _lineString = new QmlObjectListModel(this);
        emit lineStringChanged ();

    }
    if(fileName.isEmpty () || !visualItems || !complexItems){
        errorString = tr("_loadKmlMissionFile error : arg is error.");
        return false;
    }
    QFile xmlFile(fileName);
    if (!xmlFile.exists()) {
        qDebug() << "Internal error: metaDataFile mission" ;
        return false;
    }
    if (!xmlFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Internal error: Unable to open parameter file:" ;
        return false;
    }
    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    if (xml.hasError()) {
        qDebug() << "Badly formed XML" ;
        return false;
    }
    qDebug()<<"read kml success";
    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();
            if(elementName == "Polygon"){
                parsePolygon(xml, visualItems, complexItems);
            }

            if(elementName == "LineString"){
                parseLineString(xml,visualItems, complexItems);
            }
        }
        xml.readNext();
    }
    for(int i = 0; i < complexItems->count (); i++){
        if(_lineString)
            _lineString->append (complexItems->get (i));
    }
    emit lineStringChanged ();
    for(int i = 0; i < complexItems->count (); i++){
        if(_polygonn)
            _polygonn->append (complexItems->get (i));
    }
    emit polygonnChanged ();
    SimpleMissionItem* homeItem = new SimpleMissionItem(_activeVehicle, this);
    visualItems->insert(0, homeItem);
    VisualMissionItem* item = qobject_cast<VisualMissionItem*>(visualItems->get(1));
    double aa = _normalizeLat(item->coordinate().latitude());
    double bb = _normalizeLon(item->coordinate().longitude());
    homeItem->setCoordinate(QGeoCoordinate(aa-90.0,bb-180.0,0));
    errorString.clear ();
    return true;
#else
    Q_UNUSED(fileName);
    Q_UNUSED(visualItems);
    Q_UNUSED(complexItems);
    Q_UNUSED(errorString);
    return false;
#endif
}

void MissionController::parsePolygon(QXmlStreamReader &reader, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems)
{
    while(!reader.atEnd()){
        reader.readNext();
        if(reader.isStartElement ()){
            QString elementName = reader.name ().toString ();
            if(elementName == "coordinates"){
                QVariantList polygonPath;
                int complexCount = 0;
                QString strVersion = reader.readElementText();
                QString coorString = strVersion.simplified ();
                QStringList coorArray = coorString.split (" ");
                int length = coorArray.length();
                QGeoCoordinate coordinate;
                if(length > 2 ){
                    for (int i=0;i<length;i++){
                        QString coord = coorArray.at(i);
                        int len = coord.split(",").length();
                        for(int j=0;j<len;j++){
                            QStringList coord1= coord.split(",");
                            QString cor = coord1.at(j);
                            if(j==0)coordinate.setLongitude(cor.toDouble());
                            else if(j==1)coordinate.setLatitude(cor.toDouble());
                            else coordinate.setAltitude(cor.toDouble());
                        }
                        polygonPath << QVariant::fromValue(coordinate);
                    }
                }
                if(polygonPath.count () > 0){
                    SurveyMissionItem* item = new SurveyMissionItem(_activeVehicle, this);
                    if (item->loadKMLPolygon(polygonPath, complexCount)) {
                        complexCount++;
                        complexItems->append(item);
                        visualItems->append(item);
                        return;
                    } else {
                        delete item;
                        item = NULL;
                        return;
                    }
                }
                return;
            }
        }
    }
}

void MissionController::parseLineString(QXmlStreamReader &reader, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems)
{
    while(!reader.atEnd ()){
        reader.readNext ();
        QString elementName = reader.name ().toString ();
        if(elementName == "coordinates"){
            QVariantList lineString;
            int complexCount = 0;
            QString strVersion = reader.readElementText();
            QString coorString = strVersion.simplified ();
            QStringList coorArray = coorString.split (" ");
            int length = coorArray.length();
            QGeoCoordinate coordinate;
            if(length > 1 ){
                for (int i=0;i<length;i++){
                    QString coord = coorArray.at(i);
                    int len = coord.split(",").length();
                    for(int j=0;j<len;j++){
                        QStringList coord1= coord.split(",");
                        QString cor = coord1.at(j);
                        if(j==0)coordinate.setLongitude(cor.toDouble());
                        else if(j==1)coordinate.setLatitude(cor.toDouble());
                        else coordinate.setAltitude(cor.toDouble());
                    }
                    lineString << QVariant::fromValue(coordinate);
                }
            }
            if(lineString.count ()){
                SurveyMissionItem* item = new SurveyMissionItem(_activeVehicle, this);
                if (item->loadKMLLineString (lineString, complexCount)) {
                    complexCount++;
                    complexItems->append (item);
                    visualItems->append(item);
                    return;
                } else {
                    delete item;
                    item = NULL;
                    return;
                }
            }
            return;
        }
    }

}
//=========================================================
void MissionController::loadFromFile(const QString& filename)
{
    QString errorString;

    if (filename.isEmpty()) {
        return;
    }

    QmlObjectListModel* newVisualItems = new QmlObjectListModel(this);
    QmlObjectListModel* newComplexItems = new QmlObjectListModel(this);

    QFile file(filename);
    bool fileFlag = false;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString();
        fileFlag = true;
    } else {
        /* Changed by chu.fumin 2016122711 start : dms degree,minute,second convert */
        QFileInfo fileInfo(filename);
        if(fileInfo.suffix () == "xlsx"){
            _loadExcelMissionFile(filename, newVisualItems, newComplexItems, errorString);
            qDebug()<<errorString;
            fileFlag = true;
        }else if(fileInfo.suffix()=="kml" || fileInfo.suffix ()== "KML"){
            fileFlag = false;
            _loadKmlMissionFile(filename, newVisualItems, newComplexItems, errorString);
        }else if(fileInfo.suffix () == "xls"){
            fileFlag = true;
            errorString = tr("Ground station does not support this type of file end with .xls : ") + filename;
        }else{
        /* Changed by chu.fumin 2016122711 end : dms degree,minute,second convert */
            QByteArray  bytes = file.readAll();
            QTextStream stream(&bytes);
            fileFlag = true;
            QString firstLine = stream.readLine();
            if (firstLine.contains(QRegExp("QGC.*WPL"))) {
                stream.seek(0);
                _loadTextMissionFile(stream, newVisualItems, errorString);
            } else {
                _loadJsonMissionFile(bytes, newVisualItems, newComplexItems, errorString);
            }
        /* Changed by chu.fumin 2016122711 start : dms degree,minute,second convert */
        }
        /* Changed by chu.fumin 2016122711 end : dms degree,minute,second convert */
    }
    if (!errorString.isEmpty()) {
        for (int i=0; i<newVisualItems->count(); i++) {
            newVisualItems->get(i)->deleteLater();
        }
        for (int i=0; i<newComplexItems->count(); i++) {
            newComplexItems->get(i)->deleteLater();
        }
        delete newVisualItems;
        delete newComplexItems;

        qgcApp()->showMessage(errorString);
        return;
    }

    if (_visualItems) {
        _deinitAllVisualItems();
        _visualItems->deleteListAndContents();
    }
    if (_complexItems) {
        _complexItems->deleteLater();
    }

    _visualItems = newVisualItems;
    _complexItems = newComplexItems;

    if (_visualItems->count() == 0) {
        _addPlannedHomePosition(_visualItems, true /* addToCenter */);
    }
    if(fileFlag){
            _initAllVisualItems();
    }else{
        SimpleMissionItem* homeItem = NULL;
        // Setup home position at index 0

        homeItem = qobject_cast<SimpleMissionItem*>(_visualItems->get(0));
        if (!homeItem) {
            qWarning() << "homeItem not SimpleMissionItem";
            return;
        }

        homeItem->setHomePositionSpecialCase(true);
        homeItem->setShowHomePosition(_editMode);
        homeItem->missionItem().setCommand(MAV_CMD_NAV_WAYPOINT);
        homeItem->missionItem().setFrame(MAV_FRAME_GLOBAL);
        homeItem->setIsCurrentItem(true);

        if (!_editMode && _activeVehicle && _activeVehicle->homePositionAvailable()) {
            homeItem->setCoordinate(_activeVehicle->homePosition());
            homeItem->setShowHomePosition(true);
        }

        emit plannedHomePositionChanged(plannedHomePosition());

        connect(homeItem, &VisualMissionItem::coordinateChanged, this, &MissionController::_homeCoordinateChanged);

        QmlObjectListModel* newComplexItems = new QmlObjectListModel(this);
        for (int i=0; i<_visualItems->count(); i++) {
            VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
            _initVisualItem(item);

            // Set up complex item list
            if (!item->isSimpleItem()) {
                ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(item);

                if (complexItem) {
                    newComplexItems->append(item);
                } else {
                    qWarning() << "isSimpleItem == false, but not ComplexMissionItem";
                }
            }
        }

        if (_complexItems) {
            _complexItems->deleteLater();
        }
        _complexItems = newComplexItems;
//        _complexItems->clear();
        QmlObjectListModel* aa = new QmlObjectListModel();
        aa->append(_visualItems->get(0));
        _visualItems->clear();
        _visualItems=aa;
        emit visualItemsChanged();
        emit complexVisualItemsChanged();
        connect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::dirtyChanged);
        _visualItems->setDirty(false);
    }
}

void MissionController::loadFromFilePicker(void)
{
#ifndef __mobile__
    /* Changed by chu.fumin 2016122711 end : dms degree,minute,second convert */
    QString filename = QGCFileDialog::getOpenFileName(NULL, "Select Mission File to load", QString(), "Mission file (*.mission *.xlsx *.kml);;All Files (*.*)");
    /* Changed by chu.fumin 2016122711 end : dms degree,minute,second convert */
    if (filename.isEmpty()) {
        return;
    }
    loadFromFile(filename);
#endif
}

void MissionController::saveToFile(const QString& filename)
{
    qDebug() << filename;

    if (filename.isEmpty()) {
        return;
    }

    QString missionFilename = filename;
    if (!QFileInfo(filename).fileName().contains(".")) {
        missionFilename += QString(".%1").arg(QGCApplication::missionFileExtension);
    }

    QFile file(missionFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showMessage(file.errorString());
    } else {
        QJsonObject missionFileObject;      // top level json object
        QJsonArray  simpleItemsObject;
        QJsonArray  complexItemsObject;

        missionFileObject[JsonHelper::jsonVersionKey] =         "1.0";
        missionFileObject[JsonHelper::jsonGroundStationKey] =   JsonHelper::jsonGroundStationValue;

        MAV_AUTOPILOT firmwareType = MAV_AUTOPILOT_GENERIC;
        if (_activeVehicle) {
            firmwareType = _activeVehicle->firmwareType();
        } else {
            // FIXME: Hack duplicated code from QGroundControlQmlGlobal. Had to do this for now since
            // QGroundControlQmlGlobal is not available from C++ side.

            QSettings settings;
            firmwareType = (MAV_AUTOPILOT)settings.value("OfflineEditingFirmwareType", MAV_AUTOPILOT_ARDUPILOTMEGA).toInt();
        }
        missionFileObject[_jsonMavAutopilotKey] = firmwareType;

        // Save planned home position
        QJsonObject homePositionObject;
        SimpleMissionItem* homeItem = qobject_cast<SimpleMissionItem*>(_visualItems->get(0));
        if (homeItem) {
            homeItem->missionItem().save(homePositionObject);
        } else {
            qgcApp()->showMessage(QStringLiteral("Internal error: VisualMissionItem at index 0 not SimpleMissionItem"));
            return;
        }
        missionFileObject[_jsonPlannedHomePositionKey] = homePositionObject;

        // Save the visual items
        for (int i=1; i<_visualItems->count(); i++) {
            QJsonObject itemObject;

            VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
            visualItem->save(itemObject);

            if (visualItem->isSimpleItem()) {
                simpleItemsObject.append(itemObject);
            } else {
                complexItemsObject.append(itemObject);
            }
        }

        missionFileObject[jsonSimpleItemsKey] = simpleItemsObject;
        missionFileObject[_jsonComplexItemsKey] = complexItemsObject;

        QJsonDocument saveDoc(missionFileObject);
        file.write(saveDoc.toJson());
    }

    _visualItems->setDirty(false);
}

void MissionController::saveToFilePicker(void)
{
#ifndef __mobile__
    QString filename = QGCFileDialog::getSaveFileName(NULL, "Select file to save mission to", QString(), "Mission file (*.mission);;All Files (*.*)");

    if (filename.isEmpty()) {
        return;
    }
    saveToFile(filename);
#endif
}

void MissionController::_calcPrevWaypointValues(double homeAlt, VisualMissionItem* currentItem, VisualMissionItem* prevItem, double* azimuth, double* distance, double* altDifference)
{
    QGeoCoordinate  currentCoord =  currentItem->coordinate();
    QGeoCoordinate  prevCoord =     prevItem->exitCoordinate();
    bool            distanceOk =    false;

    // Convert to fixed altitudes

    qCDebug(MissionControllerLog) << homeAlt
                                  << currentItem->coordinateHasRelativeAltitude() << currentItem->coordinate().altitude()
                                  << prevItem->exitCoordinateHasRelativeAltitude() << prevItem->exitCoordinate().altitude();

    distanceOk = true;
    if (currentItem->coordinateHasRelativeAltitude()) {
        currentCoord.setAltitude(homeAlt + currentCoord.altitude());
    }
    if (prevItem->exitCoordinateHasRelativeAltitude()) {
        prevCoord.setAltitude(homeAlt + prevCoord.altitude());
    }

    qCDebug(MissionControllerLog) << "distanceOk" << distanceOk;

    if (distanceOk) {
        *altDifference = currentCoord.altitude() - prevCoord.altitude();
        *distance = prevCoord.distanceTo(currentCoord);
        *azimuth = prevCoord.azimuthTo(currentCoord);
    } else {
        *altDifference = 0.0;
        *azimuth = 0.0;
        *distance = 0.0;
    }
}

void MissionController::_calcHomeDist(VisualMissionItem* currentItem, VisualMissionItem* homeItem, double* distance)
{
    QGeoCoordinate  currentCoord =  currentItem->coordinate();
    QGeoCoordinate  homeCoord =     homeItem->exitCoordinate();
    bool            distanceOk =    false;

    distanceOk = true;

    qCDebug(MissionControllerLog) << "distanceOk" << distanceOk;

    if (distanceOk) {
        *distance = homeCoord.distanceTo(currentCoord);
    } else {
        *distance = 0.0;
    }
}

void MissionController::_recalcWaypointLines(void)
{
    /// << notify : We must use the following function instead of the official function.
    bool                firstCoordinateItem =   true;
    VisualMissionItem*  lastCoordinateItem =    qobject_cast<VisualMissionItem*>(_visualItems->get(0));

    SimpleMissionItem*  homeItem = qobject_cast<SimpleMissionItem*>(lastCoordinateItem);
    if (!homeItem) {
        qWarning() << "Home item is not SimpleMissionItem";
    }

    bool    showHomePosition =  homeItem->showHomePosition();
    double  homeAlt =           homeItem->coordinate().altitude();

    qCDebug(MissionControllerLog) << "_recalcWaypointLines";

    // If home position is valid we can calculate distances between all waypoints.
    // If home position is not valid we can only calculate distances between waypoints which are
    // both relative altitude.

    // No values for first item
    lastCoordinateItem->setAltDifference(0.0);
    lastCoordinateItem->setAzimuth(0.0);
    lastCoordinateItem->setDistance(-1.0);

    double minAltSeen = 0.0;
    double maxAltSeen = 0.0;
    double homePositionAltitude = homeItem->coordinate().altitude();
    minAltSeen = maxAltSeen = homeItem->coordinate().altitude();

    _waypointLines.clearAndDeleteContents();

    bool linkBackToHome = false;
    for (int i=1; i<_visualItems->count(); i++) {

        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        // Assume the worst
        item->setAzimuth(0.0);
        item->setDistance(-1.0);

        // If we still haven't found the first coordinate item and we hit a a takeoff command link back to home
        if (firstCoordinateItem &&
                item->isSimpleItem() &&
                qobject_cast<SimpleMissionItem*>(item)->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF) {
            linkBackToHome = true;
        }
        if (item->specifiesCoordinate()) {
            // Keep track of the min/max altitude for all waypoints so we can show altitudes as a percentage

            double absoluteAltitude = item->coordinate().altitude();
            if (item->coordinateHasRelativeAltitude()) {
                absoluteAltitude += homePositionAltitude;
            }
            minAltSeen = std::min(minAltSeen, absoluteAltitude);
            maxAltSeen = std::max(maxAltSeen, absoluteAltitude);
            if (!item->exitCoordinateSameAsEntry()) {
                absoluteAltitude = item->exitCoordinate().altitude();
                if (item->exitCoordinateHasRelativeAltitude()) {
                    absoluteAltitude += homePositionAltitude;
                }
                minAltSeen = std::min(minAltSeen, absoluteAltitude);
                maxAltSeen = std::max(maxAltSeen, absoluteAltitude);
            }
            if (!item->isStandaloneCoordinate()) {
                firstCoordinateItem = false;
                if (lastCoordinateItem != homeItem || (showHomePosition && linkBackToHome)) {
                    double azimuth, distance, altDifference;

                    // Subsequent coordinate items link to last coordinate item. If the last coordinate item
                    // is an invalid home position we skip the line
                    _calcPrevWaypointValues(homeAlt, item, lastCoordinateItem, &azimuth, &distance, &altDifference);
                    item->setAltDifference(altDifference);
                    item->setAzimuth(azimuth);
                    item->setDistance(distance);
                    _waypointLines.append(new CoordinateVector(lastCoordinateItem->isSimpleItem() ? lastCoordinateItem->coordinate() : lastCoordinateItem->exitCoordinate(), item->coordinate()));
                }
                lastCoordinateItem = item;
            }
        }
    }

    // Walk the list again calculating altitude percentages
    double altRange = maxAltSeen - minAltSeen;
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        if (item->specifiesCoordinate()) {
            double absoluteAltitude = item->coordinate().altitude();
            if (item->coordinateHasRelativeAltitude()) {
                absoluteAltitude += homePositionAltitude;
            }
            if (altRange == 0.0) {
                item->setAltPercent(0.0);
            } else {
                item->setAltPercent((absoluteAltitude - minAltSeen) / altRange);
            }
        }
    }

    emit waypointLinesChanged();
}

void MissionController::_recalcAltitudeRangeBearing()
{
    if (!_visualItems->count())
        return;

    bool                firstCoordinateItem =   true;
    VisualMissionItem*  lastCoordinateItem =    qobject_cast<VisualMissionItem*>(_visualItems->get(0));
    SimpleMissionItem*  homeItem = qobject_cast<SimpleMissionItem*>(lastCoordinateItem);

    if (!homeItem) {
        qWarning() << "Home item is not SimpleMissionItem";
    }

    bool    showHomePosition =  homeItem->showHomePosition();

    qCDebug(MissionControllerLog) << "_recalcAltitudeRangeBearing";

    // If home position is valid we can calculate distances between all waypoints.
    // If home position is not valid we can only calculate distances between waypoints which are
    // both relative altitude.

    // No values for first item
    lastCoordinateItem->setAltDifference(0.0);
    lastCoordinateItem->setAzimuth(0.0);
    lastCoordinateItem->setDistance(0.0);

    double minAltSeen = 0.0;
    double maxAltSeen = 0.0;
    const double homePositionAltitude = homeItem->coordinate().altitude();
    minAltSeen = maxAltSeen = homeItem->coordinate().altitude();

    double missionDistance = 0.0;
    double missionMaxTelemetry = 0.0;

    bool vtolCalc = (QGroundControlQmlGlobal::offlineEditingVehicleType()->enumStringValue() == "VTOL" || (_activeVehicle && _activeVehicle->vtol())) ? true : false ;
    double cruiseDistance = 0.0;
    double hoverDistance = 0.0;
    bool hoverDistanceCalc = false;
    bool hoverTransition = false;
    bool cruiseTransition = false;
    bool hoverDistanceReset = false;

    bool linkBackToHome = false;


    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        // Assume the worst
        item->setAzimuth(0.0);
        item->setDistance(0.0);

        // If we still haven't found the first coordinate item and we hit a takeoff command link back to home
        if (firstCoordinateItem &&item->isSimpleItem() &&
                qobject_cast<SimpleMissionItem*>(item)->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF) {
            linkBackToHome = true;
            hoverDistanceCalc = true;
        }

        if (item->isSimpleItem() && vtolCalc) {
            SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(item);
            if (simpleItem->command() == MavlinkQmlSingleton::MAV_CMD_DO_VTOL_TRANSITION){  //transition waypoint value
                if (simpleItem->missionItem().param1() == 3){ //hover mode value
                    hoverDistanceCalc = true;
                    hoverTransition = true;
                }
                else if (simpleItem->missionItem().param1() == 4){
                    hoverDistanceCalc = false;
                    cruiseTransition = true;
                }
            }
            if(!hoverTransition && cruiseTransition && !hoverDistanceReset && !linkBackToHome){
                hoverDistance = missionDistance;
                hoverDistanceReset = true;
            }
        }

        if (item->specifiesCoordinate()) {
            // Keep track of the min/max altitude for all waypoints so we can show altitudes as a percentage

            double absoluteAltitude = item->coordinate().altitude();
            if (item->coordinateHasRelativeAltitude()) {
                absoluteAltitude += homePositionAltitude;
            }
            minAltSeen = std::min(minAltSeen, absoluteAltitude);
            maxAltSeen = std::max(maxAltSeen, absoluteAltitude);

            if (!item->exitCoordinateSameAsEntry()) {
                absoluteAltitude = item->exitCoordinate().altitude();
                if (item->exitCoordinateHasRelativeAltitude()) {
                    absoluteAltitude += homePositionAltitude;
                }
                minAltSeen = std::min(minAltSeen, absoluteAltitude);
                maxAltSeen = std::max(maxAltSeen, absoluteAltitude);
            }

            if (!item->isStandaloneCoordinate()) {
                firstCoordinateItem = false;
                if (lastCoordinateItem != homeItem || (showHomePosition && linkBackToHome)) {
                    double azimuth, distance, altDifference, telemetryDistance;

                    // Subsequent coordinate items link to last coordinate item. If the last coordinate item
                    // is an invalid home position we skip the line
                    _calcPrevWaypointValues(homePositionAltitude, item, lastCoordinateItem, &azimuth, &distance, &altDifference);
                    item->setAltDifference(altDifference);
                    item->setAzimuth(azimuth);
                    item->setDistance(distance);

                    missionDistance += distance;

                    if (item->isSimpleItem()) {
                        _calcHomeDist(item, homeItem, &telemetryDistance);

                        if (vtolCalc) {
                            SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(item);
                            if (simpleItem->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF || hoverDistanceCalc){
                                hoverDistance += distance;
                            }
                            cruiseDistance = missionDistance - hoverDistance;
                            if(simpleItem->command() == MavlinkQmlSingleton::MAV_CMD_NAV_LAND && !linkBackToHome && !cruiseTransition && !hoverTransition){
                                hoverDistance = cruiseDistance;
                                cruiseDistance = missionDistance - hoverDistance;
                            }
                        }
                    } else {
                        missionDistance += qobject_cast<ComplexMissionItem*>(item)->complexDistance();
                        telemetryDistance = qobject_cast<ComplexMissionItem*>(item)->greatestDistanceTo(homeItem->exitCoordinate());

                        if (vtolCalc){
                            cruiseDistance += qobject_cast<ComplexMissionItem*>(item)->complexDistance(); //assume all survey missions undertaken in cruise
                        }
                    }

                    if (telemetryDistance > missionMaxTelemetry) {
                        missionMaxTelemetry = telemetryDistance;
                    }
                }
                else if (lastCoordinateItem == homeItem && !item->isSimpleItem()){
                    missionDistance += qobject_cast<ComplexMissionItem*>(item)->complexDistance();
                    missionMaxTelemetry = qobject_cast<ComplexMissionItem*>(item)->greatestDistanceTo(homeItem->exitCoordinate());

                    if (vtolCalc){
                        cruiseDistance += qobject_cast<ComplexMissionItem*>(item)->complexDistance(); //assume all survey missions undertaken in cruise
                    }
                }
                lastCoordinateItem = item;
            }
        }
    }

    setMissionDistance(missionDistance);
    setMissionMaxTelemetry(missionMaxTelemetry);
    setCruiseDistance(cruiseDistance);
    setHoverDistance(hoverDistance);

    // Walk the list again calculating altitude percentages
    double altRange = maxAltSeen - minAltSeen;
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        if (item->specifiesCoordinate()) {
            double absoluteAltitude = item->coordinate().altitude();
            if (item->coordinateHasRelativeAltitude()) {
                absoluteAltitude += homePositionAltitude;
            }
            if (altRange == 0.0) {
                item->setAltPercent(0.0);
            } else {
                item->setAltPercent((absoluteAltitude - minAltSeen) / altRange);
            }
        }
    }
}

// This will update the sequence numbers to be sequential starting from 0
void MissionController::_recalcSequence(void)
{
    // Setup ascending sequence numbers for all visual items

    int sequenceNumber = 0;
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        item->setSequenceNumber(sequenceNumber++);
        if (!item->isSimpleItem()) {
            ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(item);

            if (complexItem) {
                 sequenceNumber = complexItem->lastSequenceNumber() + 1;
            } else {
                qWarning() << "isSimpleItem == false, yet not ComplexMissionItem";
            }
        }
    }
}

// This will update the child item hierarchy
void MissionController::_recalcChildItems(void)
{
    VisualMissionItem* currentParentItem = qobject_cast<VisualMissionItem*>(_visualItems->get(0));

    currentParentItem->childItems()->clear();

    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        // Set up non-coordinate item child hierarchy
        if (item->specifiesCoordinate()) {
            item->childItems()->clear();
            currentParentItem = item;
        } else if (item->isSimpleItem()) {
            currentParentItem->childItems()->append(item);
        }
    }
}

void MissionController::_recalcAll(void)
{
    _recalcSequence();
    _recalcChildItems();
    _recalcWaypointLines();
}

/// Initializes a new set of mission items
void MissionController::_initAllVisualItems(void)
{
    SimpleMissionItem* homeItem = NULL;

    // Setup home position at index 0

    homeItem = qobject_cast<SimpleMissionItem*>(_visualItems->get(0));
    if (!homeItem) {
        qWarning() << "homeItem not SimpleMissionItem";
        return;
    }

    homeItem->setHomePositionSpecialCase(true);
    homeItem->setShowHomePosition(_editMode);
    homeItem->missionItem().setCommand(MAV_CMD_NAV_WAYPOINT);
    homeItem->missionItem().setFrame(MAV_FRAME_GLOBAL);
    homeItem->setIsCurrentItem(true);

    if (!_editMode && _activeVehicle && _activeVehicle->homePositionAvailable()) {
        homeItem->setCoordinate(_activeVehicle->homePosition());
        homeItem->setShowHomePosition(true);
    }

    emit plannedHomePositionChanged(plannedHomePosition());

    connect(homeItem, &VisualMissionItem::coordinateChanged, this, &MissionController::_homeCoordinateChanged);

    QmlObjectListModel* newComplexItems = new QmlObjectListModel(this);
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        _initVisualItem(item);

        // Set up complex item list
        if (!item->isSimpleItem()) {
            ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(item);

            if (complexItem) {
                newComplexItems->append(item);
            } else {
                qWarning() << "isSimpleItem == false, but not ComplexMissionItem";
            }
        }
    }

    if (_complexItems) {
        _complexItems->deleteLater();
    }
    _complexItems = newComplexItems;

    _recalcAll();

    emit visualItemsChanged();
    emit complexVisualItemsChanged();

    connect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::dirtyChanged);

    _visualItems->setDirty(false);
}

void MissionController::_deinitAllVisualItems(void)
{
    for (int i=0; i<_visualItems->count(); i++) {
        _deinitVisualItem(qobject_cast<VisualMissionItem*>(_visualItems->get(i)));
    }

    disconnect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::dirtyChanged);
}

void MissionController::_initVisualItem(VisualMissionItem* visualItem)
{
    _visualItems->setDirty(false);
    connect(visualItem, &VisualMissionItem::coordinateChanged,                          this, &MissionController::_itemCoordinateChanged);

    connect(visualItem, &VisualMissionItem::specifiesCoordinateChanged,                 this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::coordinateHasRelativeAltitudeChanged,       this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::exitCoordinateHasRelativeAltitudeChanged,   this, &MissionController::_recalcWaypointLines);
    //=======================================================================================================
    connect(visualItem, &VisualMissionItem::coordinateChanged,                          this, &MissionController::_itemCoordinateAddChanged);

    connect(visualItem, &VisualMissionItem::specifiesCoordinateChanged,                 this, &MissionController::_recalcWaypointLinesAdd);
    connect(visualItem, &VisualMissionItem::coordinateHasRelativeAltitudeChanged,       this, &MissionController::_recalcWaypointLinesAdd);
    connect(visualItem, &VisualMissionItem::exitCoordinateHasRelativeAltitudeChanged,   this, &MissionController::_recalcWaypointLinesAdd);
    //=====================================================================================================
    if (visualItem->isSimpleItem()) {
        // We need to track commandChanged on simple item since recalc has special handling for takeoff command
        SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);
        if (simpleItem) {
            connect(&simpleItem->missionItem(), &MissionItem::flageChanged,       this, &MissionController::_ffCoordinatee);
            connect(&simpleItem->missionItem()._commandFact, &Fact::valueChanged, this, &MissionController::_itemCommandChanged);
        } else {
            qWarning() << "isSimpleItem == true, yet not SimpleMissionItem";
        }
    } else {
        // We need to track changes of lastSequenceNumber so we can recalc sequence numbers for subsequence items
        ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(visualItem);
        connect(complexItem, &ComplexMissionItem::lastSequenceNumberChanged, this, &MissionController::_recalcSequence);
        connect(complexItem, &ComplexMissionItem::complexDistanceChanged, this, &MissionController::_recalcAltitudeRangeBearing);
    }
}

void MissionController::_deinitVisualItem(VisualMissionItem* visualItem)
{
    // Disconnect all signals
    disconnect(visualItem, 0, 0, 0);
}

void MissionController::_itemCoordinateChanged(const QGeoCoordinate& coordinate)
{
    Q_UNUSED(coordinate);
    _recalcWaypointLines();
}
//=====================================================================
void MissionController::_itemCoordinateAddChanged(const QGeoCoordinate& coordinate)
{
    Q_UNUSED(coordinate);
    _recalcWaypointLinesAdd();
}
//=======================================================================
//=================================================================
void MissionController::_ffCoordinatee(bool flage)
{
   if(flage){
       for(int i=0;i<_visualItems->count();i++){
           VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
           //VisualMissionItem* visualItem=;
           disconnect(visualItem, &VisualMissionItem::coordinateChanged,                          this, &MissionController::_itemCoordinateChanged);
           disconnect(visualItem, &VisualMissionItem::coordinateChanged,                          this, &MissionController::_itemCoordinateAddChanged);
       }
   }else{
       for(int i=0;i<_visualItems->count();i++){
           VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
           //VisualMissionItem* visualItem=;
           connect(visualItem, &VisualMissionItem::coordinateChanged,                          this, &MissionController::_itemCoordinateChanged);
           connect(visualItem, &VisualMissionItem::coordinateChanged,                          this, &MissionController::_itemCoordinateAddChanged);
        }
    }
}
void MissionController::loadMoveLine(void)
{
    _recalcWaypointLines();
}
void MissionController::loadMoveLineAdd(void)
{
    _recalcWaypointLinesAdd();
}
void MissionController::loadRecover(void)
{
    //qDebug()<<"loadRecover dgsdgdsg";
    for(int i=0;i<_visualItems->count();i++){
        VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
         connect(visualItem, &VisualMissionItem::coordinateChanged, this, &MissionController::_itemCoordinateChanged);
    }
}
void MissionController::loadRecoverAdd(void)
{
    qDebug()<<"loadRecoverAdd dgsdgdsg";
    for(int i=0;i<_visualItems->count();i++){
        VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
         connect(visualItem, &VisualMissionItem::coordinateChanged, this, &MissionController::_itemCoordinateAddChanged);
    }
}
void MissionController::_itemCommandChanged(void)
{
    _recalcChildItems();
    _recalcWaypointLines();
}

void MissionController::_activeVehicleBeingRemoved(void)
{
    qCDebug(MissionControllerLog) << "MissionController::_activeVehicleBeingRemoved";

    MissionManager* missionManager = _activeVehicle->missionManager();

    disconnect(missionManager, &MissionManager::newMissionItemsAvailable,   this, &MissionController::_newMissionItemsAvailableFromVehicle);
    disconnect(missionManager, &MissionManager::missionItemsTempChanged,   this, &MissionController::_activeVehicleMissionItemsTempChanged);
    disconnect(missionManager, &MissionManager::inProgressChanged,          this, &MissionController::_inProgressChanged);
    disconnect(missionManager, &MissionManager::currentItemChanged,         this, &MissionController::_currentMissionItemChanged);
    disconnect(missionManager, &MissionManager::cameraFeedItemsChanged,     this, &MissionController::_insertCameraFeedBackItem);
    disconnect(_activeVehicle, &Vehicle::homePositionAvailableChanged,             this, &MissionController::_activeVehicleHomePositionAvailableChanged);
    disconnect(_activeVehicle, &Vehicle::homePositionChanged,                      this, &MissionController::_activeVehicleHomePositionChanged);

    // We always remove all items on vehicle change. This leaves a user model hole:
    //      If the user has unsaved changes in the Plan view they will lose them
    removeAll();
    if(_cameraFeedItems){
        _cameraFeedItems->clearAndDeleteContents ();
    }
}

void MissionController::_activeVehicleSet(void)
{
    // We always remove all items on vehicle change. This leaves a user model hole:
    //      If the user has unsaved changes in the Plan view they will lose them
    removeAll();
    if(_cameraFeedItems){
        _cameraFeedItems->clearAndDeleteContents ();
    }

    MissionManager* missionManager = _activeVehicle->missionManager();

    connect(missionManager, &MissionManager::newMissionItemsAvailable,  this, &MissionController::_newMissionItemsAvailableFromVehicle);
    connect(missionManager, &MissionManager::missionItemsTempChanged,  this, &MissionController::_activeVehicleMissionItemsTempChanged);
    connect(missionManager,  &MissionManager::missionItemReceived,       this, &MissionController::_ackTimeStop);
    connect(missionManager, &MissionManager::inProgressChanged,         this, &MissionController::_inProgressChanged);
    connect(missionManager, &MissionManager::currentItemChanged,        this, &MissionController::_currentMissionItemChanged);
    connect(missionManager, &MissionManager::cameraFeedItemsChanged,    this, &MissionController::_insertCameraFeedBackItem);
    connect(_activeVehicle, &Vehicle::homePositionAvailableChanged,     this, &MissionController::_activeVehicleHomePositionAvailableChanged);
    connect(_activeVehicle, &Vehicle::homePositionChanged,              this, &MissionController::_activeVehicleHomePositionChanged);

    if (_activeVehicle->parameterManager()->parametersReady() && !syncInProgress()) {
        // We are switching between two previously existing vehicles. We have to manually ask for the items from the Vehicle.
        // We don't request mission items for new vehicles since that will happen autamatically.
        loadFromVehicle();
    }
    const QList<MissionItem> &missionItem = missionManager->cameraFeedbackItems ();
    for(int i = 0; i < missionItem.size(); i++){
        SimpleMissionItem * newItem = new SimpleMissionItem(_activeVehicle, missionItem[i], this);
        if(!_cameraFeedItems->contains (newItem)){
            _cameraFeedItems->insert(i, newItem);
        }else{
            delete newItem;
             newItem = NULL;
        }
    }
    emit cameraFeedItemsChanged();

    _activeVehicleHomePositionChanged(_activeVehicle->homePosition());
    _activeVehicleHomePositionAvailableChanged(_activeVehicle->homePositionAvailable());
}

void MissionController::_activeVehicleHomePositionAvailableChanged(bool homePositionAvailable)
{
    if (!_editMode && _visualItems) {
        SimpleMissionItem* homeItem = qobject_cast<SimpleMissionItem*>(_visualItems->get(0));

        if (homeItem) {
            homeItem->setShowHomePosition(homePositionAvailable);
            emit plannedHomePositionChanged(plannedHomePosition());
            _recalcWaypointLines();
        } else {
            qWarning() << "Unabled to cast home item to SimpleMissionItem";
        }
    }
}

void MissionController::_activeVehicleMissionItemsTempChanged(double latitude, double longtitude, double altitude){
    emit activeVehicleMissionItemsTempChanged(latitude, longtitude, altitude);
}

void MissionController::_activeVehicleHomePositionChanged(const QGeoCoordinate& homePosition)
{
    if (!_editMode && _visualItems) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(0));
        int index = _visualItems->count () - 1;
        SimpleMissionItem *landitem = qobject_cast<SimpleMissionItem *>(_visualItems->get(index));
        if (item) {
            if (item->coordinate() != homePosition) {
                item->setCoordinate(homePosition);
                qCDebug(MissionControllerLog) << "Home position update" << homePosition;
                emit plannedHomePositionChanged(plannedHomePosition());
                _recalcWaypointLines();
            }
        } else {
            qWarning() << "Unabled to cast home item to VisualMissionItem";
        }
        bool returntolaunch = landitem->param3 ();
        if(landitem){
            if((landitem->command () == 85 && returntolaunch)|| (landitem->command () == 21 && returntolaunch)){
                if(landitem->coordinate () != homePosition){
                    landitem->setCoordinate (homePosition);
                    qCDebug(MissionControllerLog)<<"Land position update"<<homePosition;
                    _visualItems->removeAt (index);
                    _visualItems->append (landitem);
                    _recalcWaypointLines();
                }
            }
        }
    }
}

void MissionController::setMissionDistance(double missionDistance)
{
    if (!qFuzzyCompare(_missionDistance, missionDistance)) {
        _missionDistance = missionDistance;
        emit missionDistanceChanged(_missionDistance);
    }
}

void MissionController::setMissionMaxTelemetry(double missionMaxTelemetry)
{
    if (!qFuzzyCompare(_missionMaxTelemetry, missionMaxTelemetry)) {
        _missionMaxTelemetry = missionMaxTelemetry;
        emit missionMaxTelemetryChanged(_missionMaxTelemetry);
    }
}

void MissionController::setCruiseDistance(double cruiseDistance)
{
    if (!qFuzzyCompare(_cruiseDistance, cruiseDistance)) {
        _cruiseDistance = cruiseDistance;
        emit cruiseDistanceChanged(_cruiseDistance);
    }
}

void MissionController::setHoverDistance(double hoverDistance)
{
    if (!qFuzzyCompare(_hoverDistance, hoverDistance)) {
        _hoverDistance = hoverDistance;
        emit hoverDistanceChanged(_hoverDistance);
    }
}

void MissionController::_inProgressChanged(bool inProgress)
{
    emit syncInProgressChanged(inProgress);
}

bool MissionController::_findLastAltitude(double* lastAltitude, MAV_FRAME* frame)
{
    bool found = false;
    double foundAltitude;
    MAV_FRAME foundFrame;

    // Don't use home position
    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        if (visualItem->specifiesCoordinate() && !visualItem->isStandaloneCoordinate()) {

            if (visualItem->isSimpleItem()) {
                SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);
                if ((MAV_CMD)simpleItem->command() != MAV_CMD_NAV_TAKEOFF) {
                    foundAltitude = simpleItem->exitCoordinate().altitude();
                    foundFrame = simpleItem->missionItem().frame();
                    found = true;
                }
            }
        }
    }

    if (found) {
        *lastAltitude = foundAltitude;
        *frame = foundFrame;
    }

    return found;
}

bool MissionController::_findLastAcceptanceRadius(double* lastAcceptanceRadius)
{
    bool found = false;
    double foundAcceptanceRadius;

    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        if (visualItem->isSimpleItem()) {
            SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);

            if (simpleItem) {
                if ((MAV_CMD)simpleItem->command() == MAV_CMD_NAV_WAYPOINT) {
                    foundAcceptanceRadius = simpleItem->missionItem().param2();
                    found = true;
                }
            } else {
                qWarning() << "isSimpleItem == true, yet not SimpleMissionItem";
            }
        }
    }

    if (found) {
        *lastAcceptanceRadius = foundAcceptanceRadius;
    }

    return found;
}

double MissionController::_normalizeLat(double lat)
{
    // Normalize latitude to range: 0 to 180, S to N
    return lat + 90.0;
}

double MissionController::_normalizeLon(double lon)
{
    // Normalize longitude to range: 0 to 360, W to E
    return lon  + 180.0;
}

/// Add the home position item to the front of the list
void MissionController::_addPlannedHomePosition(QmlObjectListModel* visualItems, bool addToCenter)
{
    SimpleMissionItem* homeItem = new SimpleMissionItem(_activeVehicle, this);
    visualItems->insert(0, homeItem);

    if (visualItems->count() > 1  && addToCenter) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(visualItems->get(1));

        double north = _normalizeLat(item->coordinate().latitude());
        double south = north;
        double east = _normalizeLon(item->coordinate().longitude());
        double west = east;

        for (int i=2; i<visualItems->count(); i++) {
            item = qobject_cast<VisualMissionItem*>(visualItems->get(i));

            double lat = _normalizeLat(item->coordinate().latitude());
            double lon = _normalizeLon(item->coordinate().longitude());

            north = fmax(north, lat);
            south = fmin(south, lat);
            east = fmax(east, lon);
            west = fmin(west, lon);
        }
        if(_activeVehicle->coordinateValid ()){
            homeItem->setCoordinate (_activeVehicle->homePosition ());
        }else{
            homeItem->setCoordinate(QGeoCoordinate((south + ((north - south) / 2)) - 90.0, (west + ((east - west) / 2)) - 180.0, 0.0));
        }
    } else {
        homeItem->setCoordinate(qgcApp()->lastKnownHomePosition());
    }
}

void MissionController::_currentMissionItemChanged(int sequenceNumber)
{
    if (!_editMode) {
        if (!_activeVehicle->firmwarePlugin()->sendHomePositionToVehicle()) {
            sequenceNumber++;
        }

        for (int i=0; i<_visualItems->count(); i++) {
            VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
            item->setIsCurrentItem(item->sequenceNumber() == sequenceNumber);
        }
    }
}

bool MissionController::syncInProgress(void) const
{
    return _activeVehicle ? _activeVehicle->missionManager()->inProgress() : false;
}

bool MissionController::dirty(void) const
{
    return _visualItems ? _visualItems->dirty() : false;
}


void MissionController::setDirty(bool dirty)
{
    if (_visualItems) {
        _visualItems->setDirty(dirty);
    }
}

QGeoCoordinate MissionController::plannedHomePosition(void)
{
    if (_visualItems && _visualItems->count() > 0) {
        SimpleMissionItem* item = qobject_cast<SimpleMissionItem*>(_visualItems->get(0));
        if (item && item->showHomePosition()) {
            return item->coordinate();
        }
    }

    return QGeoCoordinate();
}

void MissionController::_homeCoordinateChanged(void)
{
    emit plannedHomePositionChanged(plannedHomePosition());
    _recalcAltitudeRangeBearing();
}

QString MissionController::fileExtension(void) const
{
    return QGCApplication::missionFileExtension;
}
void MissionController::sendAltitudee(double alti)
{
    //qDebug()<<"MissionController sendAltitudee";
    if (_activeVehicle) {
        mavlink_message_t sendMessage;
        mavlink_mission_item_t item;
        item.target_system = _activeVehicle->id();
        item.target_component = MAV_COMP_ID_MISSIONPLANNER;
        item.param1=100002;
        item.param2=3;
        item.param3 = 1;
        item.param4=_visualItems->count();
        item.x= 0;
        item.y = 0;
        item.z = alti;
        item.command = 500;
        item.seq=10;
        item.frame = 160;
        _dedicatedLink = _activeVehicle->priorityLink();
        mavlink_msg_mission_item_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &sendMessage, &item);
        _activeVehicle->sendMessageOnLink(_dedicatedLink, sendMessage);
    }
}
void MissionController::sendMoveOffset(double x1,double y1)
{
    //qDebug()<<"MissionController : sendMoveOffset";
    if (_activeVehicle) {
        mavlink_message_t sendMessage;
        mavlink_mission_item_t item;
        int tmp;
        tmp = x1*10000000;
        memcpy(&item.x,&tmp,sizeof(tmp));
        tmp = y1*10000000;
        memcpy(&item.y,&tmp,sizeof(tmp));
        item.target_system = _activeVehicle->id();
        item.target_component = MAV_COMP_ID_MISSIONPLANNER;
        item.param1=100002;
        item.param2=3;
        item.param3 = 1;
        item.param4=_visualItems->count();

        //item.x= x1;
        //item.y = y1;
        item.z = 0;
        item.seq=10;
        item.frame = 160;
        item.command = 500;
        _dedicatedLink = _activeVehicle->priorityLink();
        mavlink_msg_mission_item_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &sendMessage, &item);
        _activeVehicle->sendMessageOnLink(_dedicatedLink, sendMessage);
    }
}

void MissionController::sendMovePoint(int x1,QGeoCoordinate coordinate)
{
    //qDebug()<<"MissionController : sendMovePoint";
    if (_activeVehicle) {
        mavlink_message_t sendMessage;
        mavlink_mission_item_t item;
        int tmp;
        tmp = (double)(coordinate.latitude())*10000000;
        memcpy(&item.x,&tmp,sizeof(tmp));
        tmp = (double)(coordinate.longitude())*10000000;
        memcpy(&item.y,&tmp,sizeof(tmp));
        item.target_system = _activeVehicle->id();
        item.target_component = MAV_COMP_ID_MISSIONPLANNER;
        item.param1=100002;
        item.param2=3;

        item.param3 = x1;
        item.param4=1;
        item.seq=10;
        item.command = 500;
        item.frame = 160;
        item.z = coordinate.altitude();
        _dedicatedLink = _activeVehicle->priorityLink();
        mavlink_msg_mission_item_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &sendMessage, &item);
        _activeVehicle->sendMessageOnLink(_dedicatedLink, sendMessage);
    }
}
void MissionController::sendLarge(double x1,QGeoCoordinate coordinate)
{
      //qDebug()<<"MissionController sendLarge";
      if (_activeVehicle) {
          mavlink_message_t sendMessage;
          mavlink_mission_item_t item;
          int tmp;
          tmp = (double)(coordinate.latitude())*10000000;
          memcpy(&item.x,&tmp,sizeof(tmp));
          tmp = (double)(coordinate.longitude())*10000000;
          memcpy(&item.y,&tmp,sizeof(tmp));

          item.target_system = _activeVehicle->id();
          item.target_component = MAV_COMP_ID_MISSIONPLANNER;
          item.param1=100001;
          item.param2=x1;
          item.command = 500;

          item.param3=1;
          item.param4=_visualItems->count();
          item.seq=10;
          item.z = 0;
          item.frame = 160;
          _dedicatedLink = _activeVehicle->priorityLink();
          mavlink_msg_mission_item_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &sendMessage, &item);
          _activeVehicle->sendMessageOnLink(_dedicatedLink, sendMessage);
      }
}
void MissionController::sendSmall(double x1,QGeoCoordinate coordinate)
{
     //qDebug()<<"MissionController sendSmall";
      if (_activeVehicle) {
          mavlink_message_t sendMessage;
          mavlink_mission_item_t item;
          int tmp;
          tmp = (double)(coordinate.latitude())*10000000;
          memcpy(&item.x,&tmp,sizeof(tmp));
          tmp = (double)(coordinate.longitude())*10000000;
          memcpy(&item.y,&tmp,sizeof(tmp));

          item.target_system = _activeVehicle->id();
          item.target_component = MAV_COMP_ID_MISSIONPLANNER;
          item.param1=100001;
          item.param2=x1;
          item.command = 500;
          item.param3=1;
          item.param4=_visualItems->count();
          item.seq=10;
          item.z = 0;
          item.frame = 160;
          _dedicatedLink = _activeVehicle->priorityLink();
          mavlink_msg_mission_item_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &sendMessage, &item);
          _activeVehicle->sendMessageOnLink(_dedicatedLink, sendMessage);
      }
}
void MissionController::sendShun(double x1,QGeoCoordinate coordinate)
{
    //qDebug()<<"MissionController : sendShun";
    if (_activeVehicle) {
        mavlink_message_t sendMessage;
        mavlink_mission_item_t item;
        int tmp;
        tmp = (double)(coordinate.latitude())*10000000;
        memcpy(&item.x,&tmp,sizeof(tmp));
        tmp = (double)(coordinate.longitude())*10000000;
        memcpy(&item.y,&tmp,sizeof(tmp));
        item.target_system = _activeVehicle->id();
        item.target_component = MAV_COMP_ID_MISSIONPLANNER;
        item.param1=100003;
        int x2 = x1*10000;
        item.param2=x2;
        item.command = 500;
        item.param3=1;
        item.param4=_visualItems->count();
        item.seq=10;
        item.z=0;
        //item.x=coordinate.latitude();
        //item.y=coordinate.longitude();
        item.frame = 160;
        _dedicatedLink = _activeVehicle->priorityLink();
        mavlink_msg_mission_item_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &sendMessage, &item);
        _activeVehicle->sendMessageOnLink(_dedicatedLink, sendMessage);
    }
}
void MissionController::sendNi(double x1,QGeoCoordinate coordinate)
{
    if (_activeVehicle) {
        mavlink_message_t sendMessage;
        mavlink_mission_item_t item;
        int tmp;
        tmp = coordinate.latitude()*10000000;
        memcpy(&item.x, &tmp, sizeof(tmp));
        tmp = coordinate.longitude()*10000000;
        memcpy(&item.y, &tmp, sizeof(tmp));
        item.target_system = _activeVehicle->id();
        item.target_component = MAV_COMP_ID_MISSIONPLANNER;
        item.param1=100003;

        int x2 = x1*10000;
        item.param2=x2;
        item.param3=1;
        item.param4=_visualItems->count();
        item.seq=10;
        item.z=0;
        item.command = 500;
        item.frame = 160;
        _dedicatedLink = _activeVehicle->priorityLink();
        mavlink_msg_mission_item_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &sendMessage, &item);
        _activeVehicle->sendMessageOnLink(_dedicatedLink, sendMessage);
    }
}
void MissionController::sendFlagg(void){
    if(!_activeVehicle || !_activeVehicle->missionManager ()){
        return ;
    }

    if( _activeVehicle->missionManager()->missionItemsflag().count()!=0){
        const QList<MissionItem*>& newMissionItemsflag = _activeVehicle->missionManager()->missionItemsflag();

        foreach(const MissionItem* missionItemflag, newMissionItemsflag) {
            _visualItemsflag->append(new SimpleMissionItem(_activeVehicle, *missionItemflag, this));
        }
        _lati = (qobject_cast<SimpleMissionItem*>(_visualItemsflag->get(0)))->missionItem().param5();
        _lagi = (qobject_cast<SimpleMissionItem*>(_visualItemsflag->get(0)))->missionItem().param6();
        _multi = (qobject_cast<SimpleMissionItem*>(_visualItemsflag->get(0)))->missionItem().param2();
    }else {
        _visualItemsflag->clear();
    }
}

void MissionController::sendMissionItem(int index)
{
    _currentMissionItem = index;
    VisualMissionItem*visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(index));
    if(visualItem->isSimpleItem()){
         MissionItem *missionItem = new MissionItem(qobject_cast<SimpleMissionItem*>(visualItem)->missionItem());

         mavlink_message_t  message;
         mavlink_mission_item_t item;

         item.target_system =  _activeVehicle->id();
         item.target_component = MAV_COMP_ID_MISSIONPLANNER;
         item.seq = index;
         item.command = missionItem->command() + 200;
         item.param1 = missionItem->param1();
         item.param2 = missionItem->param2();
         item.param3 = missionItem->param3();
         item.param4 = missionItem->param4();
         int temp;
         temp = missionItem->param5()*10000000;
         memcpy(&item.x,&temp,sizeof(temp));
         temp = missionItem->param6()*10000000;
         memcpy(&item.y, &temp,sizeof(temp));
         item.z = missionItem->param7();
         item.frame = missionItem->frame()+200;
         item.current = missionItem->sequenceNumber() == 0;
         item.autocontinue = missionItem->autoContinue();

         _dedicatedLink = _activeVehicle->priorityLink();
         mavlink_msg_mission_item_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId()
                                              ,qgcApp()->toolbox()->mavlinkProtocol()->getComponentId()
                                              ,&message
                                              ,&item);
         _activeVehicle->sendMessageOnLink(_dedicatedLink,message);
         _retrycount =  0;
         _ackTimeoutTimer->start();
     }else{
        qDebug()<<__FILE__<<" "<<__LINE__<<" "<<"the item is not a SimpleItem(FlightMap)";
    }
}

void MissionController::_recalcWaypointLinesAdd(void)
{
    bool                firstCoordinateItem =   true;
    VisualMissionItem*  lastCoordinateItem =    qobject_cast<VisualMissionItem*>(_visualItems->get(0));

    SimpleMissionItem*  homeItem = qobject_cast<SimpleMissionItem*>(lastCoordinateItem);
    if (!homeItem) {
        qWarning() << "Home item is not SimpleMissionItem";
    }

    bool    showHomePosition =  homeItem->showHomePosition();
    double  homeAlt =           homeItem->coordinate().altitude();

    qCDebug(MissionControllerLog) << "_recalcWaypointLines";

    // If home position is valid we can calculate distances between all waypoints.
    // If home position is not valid we can only calculate distances between waypoints which are
    // both relative altitude.

    // No values for first item
    lastCoordinateItem->setAltDifference(0.0);
    lastCoordinateItem->setAzimuth(0.0);
    lastCoordinateItem->setDistance(-1.0);

    double minAltSeen = 0.0;
    double maxAltSeen = 0.0;
    double homePositionAltitude = homeItem->coordinate().altitude();
    minAltSeen = maxAltSeen = homeItem->coordinate().altitude();

    _waypointLinesAdd.clearAndDeleteContents ();

    bool linkBackToHome = false;
    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        // Assume the worst
        item->setAzimuth(0.0);
        item->setDistance(-1.0);

        // If we still haven't found the first coordinate item and we hit a a takeoff command link back to home
        if (firstCoordinateItem &&
                item->isSimpleItem() &&
                qobject_cast<SimpleMissionItem*>(item)->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF) {
            linkBackToHome = true;
        }

        if (item->specifiesCoordinate()) {
            // Keep track of the min/max altitude for all waypoints so we can show altitudes as a percentage

            double absoluteAltitude = item->coordinate().altitude();
            if (item->coordinateHasRelativeAltitude()) {
                absoluteAltitude += homePositionAltitude;
            }
            minAltSeen = std::min(minAltSeen, absoluteAltitude);
            maxAltSeen = std::max(maxAltSeen, absoluteAltitude);
            if (!item->exitCoordinateSameAsEntry()) {
                absoluteAltitude = item->exitCoordinate().altitude();
                if (item->exitCoordinateHasRelativeAltitude()) {
                    absoluteAltitude += homePositionAltitude;
                }
                minAltSeen = std::min(minAltSeen, absoluteAltitude);
                maxAltSeen = std::max(maxAltSeen, absoluteAltitude);
            }
            if (!item->isStandaloneCoordinate()) {
                firstCoordinateItem = false;
                if (lastCoordinateItem != homeItem || (showHomePosition && linkBackToHome)) {
                    double azimuth, distance, altDifference;

                    // Subsequent coordinate items link to last coordinate item. If the last coordinate item
                    // is an invalid home position we skip the line
                    _calcPrevWaypointValues(homeAlt, item, lastCoordinateItem, &azimuth, &distance, &altDifference);
                    item->setAltDifference(altDifference);
                    item->setAzimuth(azimuth);
                    item->setDistance(distance);
                    _waypointLinesAdd.append(new CoordinateVector(lastCoordinateItem->isSimpleItem() ? lastCoordinateItem->coordinate() : lastCoordinateItem->exitCoordinate(), item->coordinate()));
                }
                lastCoordinateItem = item;
            }
        }
    }

    // Walk the list again calculating altitude percentages
    double altRange = maxAltSeen - minAltSeen;
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        if (item->specifiesCoordinate()) {
            double absoluteAltitude = item->coordinate().altitude();
            if (item->coordinateHasRelativeAltitude()) {
                absoluteAltitude += homePositionAltitude;
            }
            if (altRange == 0.0) {
                item->setAltPercent(0.0);
            } else {
                item->setAltPercent((absoluteAltitude - minAltSeen) / altRange);
            }
        }
    }

    emit waypointLinesAddChanged();

}

void MissionController::_ackTimeout()
{
    if(_retrycount > 3){
        qDebug()<<"Mission write mission  failed, maximum retries exceeded.";
        qgcApp()->showMessage("Mission write mission  failed, maximum retries exceeded.");
    }else{
        _retrycount++;
        sendMissionItem(_currentMissionItem);
    }
}

void MissionController::_ackTimeStop(bool receive)
{
    if(receive)
        _ackTimeoutTimer->stop();
}
void MissionController::setLati(double lati)
{
    // FIXME: AutoSync temporarily turned off

    _lati = lati;

}
void MissionController::setVisualItemsflag(QmlObjectListModel* visualItemsflag)
{
    // FIXME: AutoSync temporarily turned off

    _visualItemsflag = visualItemsflag;

}
void MissionController::setLagi(double lagi)
{
    // FIXME: AutoSync temporarily turned off

    _lagi = lagi;

}
void MissionController::setMulti(double multi)
{
    // FIXME: AutoSync temporarily turned off

    _multi = multi;

}
