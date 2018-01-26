#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>

#include "QGCUpdate.h"
#include "QGCFileDownload.h"
#include "QGCApplication.h"
#ifndef __mobile__
#include "MainWindow.h"
#endif

const static QString map_xml_url("https://download.brainybeeuav.com/QGC_BrainyBee/qgcversion.xml");

QGCUpdate::QGCUpdate(QObject *parent)
    :QObject(parent)
{
   _newVersion =  false;
   _qgcDownload = false;
}

QGCUpdate::~QGCUpdate()
{

}

void QGCUpdate::setNewVersion(bool newVersion)
{
    _newVersion = newVersion;
    emit newVersionChanged(newVersion);
}

void QGCUpdate::setQgcDownlod(bool qgcDownload)
{
    _qgcDownload = qgcDownload;
    emit qgcDownloadChanged(qgcDownload);
}


void QGCUpdate::setProgressBarValue(QString value)
{
    _progressBarValue = value;
    emit progressBarValueChanged (value);
}

void QGCUpdate::setDetailsText(QString text)
{
    _detailsText = text;
    emit detailsTextChanged (text);
}

void QGCUpdate:: updateQGC()
{
   QGCFileDownload* downloader = new QGCFileDownload(this);
   connect(downloader,&QGCFileDownload::downloadFinished,this,&QGCUpdate::qgcVersionDownloadFinished);
   downloader->download(map_xml_url);
}
#ifndef __mobile__
void QGCUpdate::startInstall()
{
    QProcess newprocess(0);
    QString localfile = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if(localfile.isEmpty()){
        localfile = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (localfile.isEmpty()) {
            qDebug() << "Unabled to find writable download location. Tried downloads and temp directory.";
            return;
        }
    }
    localfile += "/brainybee_installer.exe";
    if(localfile.isEmpty ()){
        qDebug()<<"file is exist";
        return;
    }
    QString command = localfile;
    qgcApp ()->exit ();
    newprocess.startDetached (command);
}
#endif
void QGCUpdate::qgcVersionDownloadFinished(const QString &remoteFile,QString localFile )
{
    Q_UNUSED(remoteFile);
    QFile versionFile(localFile);
    if(!versionFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"qgcversiom.xml file open failed";
        return;
    }
    versionFile.close();
    XmlReaderFile(localFile);
}

bool QGCUpdate::XmlReaderFile(const QString &xmlFile)
{
    _xmlFile.setFileName(xmlFile);
    if(!_xmlFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        return false;
    }
    _xmlReader.setDevice(&_xmlFile);
    if(_xmlReader.hasError()){
        qDebug()<<"_xmlFile parse failed!";
        return false;
    }
    while(!_xmlReader.atEnd()){
        QXmlStreamReader::TokenType type = _xmlReader.readNext();
        if(type == QXmlStreamReader::DTD || type == QXmlStreamReader::StartDocument)
            continue;
        if(_xmlReader.isStartElement()){
//            qDebug()<<_xmlReader.name().toString();
            if(!_xmlReader.name().toString().compare("QGroudControl",Qt::CaseInsensitive)){
                XmlReaderSysVersion();
                break;
            }
        }
    }
    _xmlFile.close();
    return true;
}
bool QGCUpdate::XmlReaderSysVersion()
{
    while(!_xmlReader.atEnd()){
        QXmlStreamReader::TokenType type = _xmlReader.readNext();
        if(type == QXmlStreamReader::DTD || type == QXmlStreamReader::StartDocument)
            continue;
        if(_xmlReader.isStartElement()){
#ifdef __mobile__
            if(!_xmlReader.name().toString().compare("android",Qt::CaseInsensitive)){
                XmlReaderVersion();
                break;
            }
#else
            if(!_xmlReader.name().toString().compare("windows",Qt::CaseInsensitive)){
                XmlReaderVersion();
                break;
            }
#endif
        }
    }
    return true;
}

bool QGCUpdate::XmlReaderVersion()
{

    while(!_xmlReader.atEnd()){
        QXmlStreamReader::TokenType type = _xmlReader.readNext();
        if(type == QXmlStreamReader::DTD || type == QXmlStreamReader::StartDocument){
            continue;
        }
        if(_xmlReader.isStartElement()){
            if(!_xmlReader.name().toString().compare("version",Qt::CaseInsensitive)){
                XmlReaderUrl();
                break;
            }
        }
    }
    return true;
}
bool QGCUpdate::XmlReaderUrl()
{

    while(!_xmlReader.atEnd()){
        QXmlStreamReader::TokenType type = _xmlReader.readNext();
        if(type == QXmlStreamReader::DTD || type == QXmlStreamReader::StartDocument){
            continue;
        }
        if(_xmlReader.isStartElement()){
//            qDebug()<<_xmlReader.name().toString();
            if(!_xmlReader.name().toString().compare("versionNum",Qt::CaseInsensitive)){
                QString currentVersion = qgcApp()->applicationVersion();
//                qDebug()<<currentVersion;
//                qDebug()<<_xmlReader.readElementText();
                if(currentVersion.compare(_xmlReader.readElementText(),Qt::CaseInsensitive)){
//                    qDebug()<<"there is a different version";
                    bool Version = true;
                    _xmlReader.readNextStartElement ();
                    if(!_xmlReader.name ().toString ().compare ("details", Qt::CaseInsensitive)){
                        setDetailsText (_xmlReader.readElementText ());
                     }
                    setNewVersion(Version);
                    connect(this,&QGCUpdate::qgcDownloadChanged,this,&QGCUpdate::down);
                    break;
                 }else{
                    return false;
                }
            }
        }
    }
    return true;
}
bool QGCUpdate::down(bool Download){
//    qDebug()<<_xmlReader.readElementText();
    if(Download){
        _xmlReader.readNextStartElement();
//        qDebug()<<_xmlReader.name().toString()<<"*************************";
        if(!_xmlReader.name ().toString ().compare ("url", Qt::CaseInsensitive)){
             QString qgcremoteFile = _xmlReader.readElementText();
//             qDebug()<<qgcremoteFile;
             QGCDownload(qgcremoteFile);
         }
       return false;
    }
    return true;
}

void QGCUpdate::qgcDownloadFinished(const QString &remoteFile, QString localFile)
{
    Q_UNUSED(remoteFile);
    if(localFile.isEmpty ()){
        return;
    }
    QFileInfo *installer = new QFileInfo(localFile);
    if(installer->size () == 0){
        return;
    }
#ifndef __mobile__
    startInstall();
#endif

}

void QGCUpdate::QGCDownload(const QString &remoteFile)
{
    QGCFileDownload* downloaderqgc = new QGCFileDownload(this);
    connect(downloaderqgc,&QGCFileDownload::downloadProgress,this,&QGCUpdate::downloadProgress);
    connect(downloaderqgc,&QGCFileDownload::downloadFinished,this,&QGCUpdate::qgcDownloadFinished);
    downloaderqgc->download(remoteFile);
}

void QGCUpdate::downloadProgress(qint64 curr, qint64 total){
    setProgressBarValue (QString::number (curr * 1.0/total * 100 , 10,1));
//    if(curr*1.0/total == 1){
//#ifndef __mobile__
//
//#endif
//    }
}
