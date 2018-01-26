/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Main Tool Bar
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include <QQmlContext>
#include <QQmlEngine>

#include "MainToolBarController.h"
#include "ScreenToolsController.h"
#include "UASMessageView.h"
#include "UASMessageHandler.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "UAS.h"
#include "ParameterManager.h"
#include "QGCFileDownload.h"

MainToolBarController::MainToolBarController(QObject* parent)
    : QObject(parent)
    , _vehicle(NULL)
    , _mav(NULL)
    , _progressBarValue(0.0f)
    , _telemetryRRSSI(0)
    , _telemetryLRSSI(0)
    , _firmwareNew(false)
{
    _activeVehicleChanged(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());
    connect(qgcApp()->toolbox()->mavlinkProtocol(),     &MAVLinkProtocol::radioStatusChanged, this, &MainToolBarController::_telemetryChanged);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &MainToolBarController::_activeVehicleChanged);
}

MainToolBarController::~MainToolBarController()
{

}

void MainToolBarController::loadFirmwareVersionXml()
{
    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadFinished, this, &MainToolBarController::firmwareVersionXmlDownloadFinished);
    downloader->download("https://download.brainybeeuav.com/Firmware/.version.xml");
}

bool MainToolBarController::XmlReaderStart()
{
    while(!_xmlReader.atEnd ()){
        QXmlStreamReader::TokenType type = _xmlReader.readNext ();
        if(type == QXmlStreamReader::DTD || type == QXmlStreamReader::StartDocument){
            continue;
        }
        if(type == QXmlStreamReader::StartElement){
//            qDebug() <<__LINE__<<_xmlReader.name ().toString ();
            if(!_xmlReader.name ().toString ().compare ("vehicleType",Qt::CaseInsensitive)){
                ReaderPilotStackType();
                break;
            }
        }
    }
    return true;
}

bool MainToolBarController::ReaderPilotStackType()
{
    while(!_xmlReader.atEnd ()){
        QXmlStreamReader::TokenType type = _xmlReader.readNext ();
        if(type == QXmlStreamReader::DTD || type == QXmlStreamReader::StartDocument){
            continue;
        }
        if(type == QXmlStreamReader::StartElement){
//            qDebug() <<__LINE__<<_xmlReader.name ().toString ();
            if(!_xmlReader.name ().toString ().compare ("AutoPilotStackPX4",Qt::CaseInsensitive)){
                ReaderFirmwareType();
                break;
            }
        }
    }
    return true;
}

bool MainToolBarController::ReaderFirmwareType()
{
    while(!_xmlReader.atEnd ()){
        QXmlStreamReader::TokenType type = _xmlReader.readNext ();
        if(type == QXmlStreamReader::DTD || type == QXmlStreamReader::StartDocument){
            continue;
        }
        if(type == QXmlStreamReader::StartElement){
//            qDebug() <<__LINE__<<_xmlReader.name ().toString ();
            if(!_xmlReader.name ().toString ().compare ("StableFirmware",Qt::CaseInsensitive)){
                ReaderVersion();
                break;
            }
        }
    }
    return true;
}

bool MainToolBarController::ReaderVersion()
{
    while(!_xmlReader.atEnd ()){
        QXmlStreamReader::TokenType type = _xmlReader.readNext ();
        if(type == QXmlStreamReader::DTD || type == QXmlStreamReader::StartDocument){
            continue;
        }
        if(type == QXmlStreamReader::StartElement){
            if(!_xmlReader.name ().toString ().compare ("version",Qt::CaseInsensitive)){
                qDebug()<<__LINE__<<_xmlReader.name ().toString ();
                _xmlReader.readNextStartElement ();
//                qDebug()<<__LINE__<<_xmlReader.name ().toString ();
                if(!_xmlReader.name ().toString ().compare ("versionNum",Qt::CaseInsensitive)){
                   QString version =  _xmlReader.readElementText();
                    _xmlReader.readNextStartElement ();
                    if(!_xmlReader.name ().toString ().compare ("detailstext",Qt::CaseInsensitive)){
                        QString detailstext = _xmlReader.readElementText();
                        emit newfirmwarwVersion (version,detailstext);
                        break;
                    }
                }
            }
        }
    }
    return true;
}

QString MainToolBarController::vehicleFirmwareVersion(Vehicle *vehicle)
{
    QString firmwareMajorVersion = QString::number (vehicle->firmwareMajorVersion ());
    QString firmwareMinorVersion = QString::number (vehicle->firmwareMinorVersion ());
    QString firmwarePatchVersion = QString::number (vehicle->firmwarePatchVersion ());
    QString vehicleFirmwareVersion = "V" + firmwareMajorVersion + "." + firmwareMinorVersion + "." + firmwarePatchVersion;
    return vehicleFirmwareVersion;
}

void MainToolBarController::_activeVehicleChanged(Vehicle* vehicle)
{
    // Disconnect the previous one (if any)
    if (_vehicle) {
        disconnect(_vehicle->parameterManager(), &ParameterManager::parameterListProgress, this, &MainToolBarController::_setProgressBarValue);
        _mav = NULL;
        _vehicle = NULL;
    }

    // Connect new system
    if (vehicle)
    {
        _vehicle = vehicle;
        _mav = vehicle->uas();
        connect(_vehicle->parameterManager(), &ParameterManager::parameterListProgress, this, &MainToolBarController::_setProgressBarValue);
    }
}

void MainToolBarController::_telemetryChanged(LinkInterface*, unsigned rxerrors, unsigned fixed, int rssi, int remrssi, unsigned txbuf, unsigned noise, unsigned remnoise)
{
    if(_telemetryLRSSI != rssi) {
        _telemetryLRSSI = rssi;
        emit telemetryLRSSIChanged(_telemetryLRSSI);
    }
    if(_telemetryRRSSI != remrssi) {
        _telemetryRRSSI = remrssi;
        emit telemetryRRSSIChanged(_telemetryRRSSI);
    }
    if(_telemetryRXErrors != rxerrors) {
        _telemetryRXErrors = rxerrors;
        emit telemetryRXErrorsChanged(_telemetryRXErrors);
    }
    if(_telemetryFixed != fixed) {
        _telemetryFixed = fixed;
        emit telemetryFixedChanged(_telemetryFixed);
    }
    if(_telemetryTXBuffer != txbuf) {
        _telemetryTXBuffer = txbuf;
        emit telemetryTXBufferChanged(_telemetryTXBuffer);
    }
    if(_telemetryLNoise != noise) {
        _telemetryLNoise = noise;
        emit telemetryLNoiseChanged(_telemetryLNoise);
    }
    if(_telemetryRNoise != remnoise) {
        _telemetryRNoise = remnoise;
        emit telemetryRNoiseChanged(_telemetryRNoise);
    }
}

void MainToolBarController::setFirmwareNew(bool value)
{
    _firmwareNew = value;
    emit firmwareNewChanged (value);
}

void MainToolBarController::setDetailsText(QString detailsText)
{
    _detailsText = detailsText;
    emit detailsTextChanged (detailsText);
}

void MainToolBarController::firmwareVersionXmlDownloadFinished(QString remoteFile, QString localFile)
{
    Q_UNUSED(remoteFile);
    QFile versionXmlFile(localFile);
    if(!versionXmlFile.open (QFile::ReadOnly | QFile::Text)){
        qDebug()<<localFile<<"can not open";
        return;
    }
    _xmlReader.setDevice (&versionXmlFile);
    if(_xmlReader.hasError ()){
        qDebug()<<localFile<<"read error";
        return;
    }
    XmlReaderStart();
    versionXmlFile.close ();
}

void MainToolBarController::versionCompare(const QString &version, const QString &detailstext)
{
    _vehicle = qgcApp ()->toolbox ()->multiVehicleManager ()->activeVehicle ();
    QString vehiclefirmwareversion = vehicleFirmwareVersion(_vehicle);
    int num = vehiclefirmwareversion.compare (version,Qt::CaseInsensitive);
    if(num < 0){
        QString text = version + ":" + "\n" + detailstext;
//        qDebug()<<text;
        setDetailsText (text);
        setFirmwareNew (true);
    }
}

void MainToolBarController::_setProgressBarValue(float value)
{
    _progressBarValue = value;
    if(!value){
        loadFirmwareVersionXml ();
        connect(this,&MainToolBarController::newfirmwarwVersion,this, &MainToolBarController::versionCompare);
    }
    emit progressBarValueChanged(value);
}
