/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief This class mimics QTemporaryFile. We have our own implementation due to the fact that
///				QTemporaryFile implemenation differs cross platform making it unusable for our use-case.
///				Look for bug reports on QTemporaryFile keeping the file locked for details.
///
///     @author Don Gagne <don@thegagnes.com>

#include "QGCTemporaryFile.h"

#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#ifdef __android__
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QAndroidJniObject>
#endif

QGCTemporaryFile::QGCTemporaryFile(const QString& fileTemplate, QObject* parent) :
    QFile(parent),
    _template(fileTemplate)
{

}

bool QGCTemporaryFile::open(QFile::OpenMode openMode)
{
    QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    
    // Generate unique, non-existing filename
    
    static const char rgDigits[] = "0123456789";
    
    QString tempFilename;
    
    do {
        QString uniqueStr;
        for (int i=0; i<6; i++) {
            uniqueStr += rgDigits[qrand() % 10];
        }
        
        if (_template.contains("XXXXXX")) {
            tempFilename = _template.replace("XXXXXX", uniqueStr, Qt::CaseSensitive);
        } else {
            tempFilename = _template + uniqueStr;
        }
    } while (tempDir.exists(tempFilename));

    setFileName(tempDir.filePath(tempFilename));
    
    return QFile::open(openMode);
}


bool QGCTemporaryFile::tempOpen(QFile::OpenMode openMode, QString dir)
{
#ifdef __android__
    QAndroidJniEnvironment jniEnv;
    if(jniEnv->ExceptionCheck()){
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }
    QAndroidJniObject jstrObj = QAndroidJniObject::callStaticObjectMethod(
                "android/os/Environment",
                "getExternalStorageDirectory",
                "()Ljava/io/File;");
    if(!jstrObj.isValid()){
        qDebug()<<"ExternalStorageDirectory is no exist";
    }
    QString jstrDir = jstrObj.toString();
    if(jstrDir.endsWith("/")){
        jstrDir = jstrDir + "log/";
    }else{
        jstrDir = jstrDir + "/log/";
    }
    QDir tempDir (jstrDir);
    if(!tempDir.exists()){
        tempDir.mkdir(jstrDir);
    }
#else
    QDir tempDir(dir);
    if(!tempDir.exists ()){
        tempDir.mkdir(dir);
    }
#endif
    //qDebug()<<__FILE__<<" : "<<__LINE__<<" : "<<tempDir.absolutePath ()<< " : "<< (QDateTime::currentDateTime()).toString("yyyyMMddhhmmss");
    // Generate unique, non-existing filename

    static const char rgDigits[] = "0123456789";

    QString tempFilename;
    QString tempPlate;
    do {
        tempPlate = _template;
        QString uniqueStr;
        for (int i=0; i<6; i++) {
            uniqueStr += rgDigits[qrand() % 10];
        }
        QString dateTime= (QDateTime::currentDateTime()).toString("yyyyMMddhhmmss");

        if (tempPlate.contains("XXXXXX")) {
            tempFilename = tempPlate.replace("XXXXXX", uniqueStr + dateTime, Qt::CaseSensitive);
        } else {
            tempFilename = tempPlate.replace("XXXXXX", uniqueStr + dateTime, Qt::CaseSensitive);
        }
    } while (tempDir.exists(tempFilename));

    setFileName(tempDir.filePath(tempFilename));

    return QFile::open(openMode);
}
