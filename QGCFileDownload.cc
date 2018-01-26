/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCFileDownload.h"
#include "QGCApplication.h"

#include <QFileInfo>
#include <QStandardPaths>
#include <QNetworkProxy>
#ifdef __android__
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QAndroidJniObject>
#endif

QGCFileDownload::QGCFileDownload(QObject* parent)
    : QNetworkAccessManager(parent)
{

}

bool QGCFileDownload::download(const QString& remoteFile, bool redirect)
{
    if (!redirect) {
        _originalRemoteFile = remoteFile;
    }

    if (remoteFile.isEmpty()) {
        qWarning() << "downloadFile empty";
        return false;
    }
    
    // Split out filename from path
    QString remoteFileName = QFileInfo(remoteFile).fileName();
    if (remoteFileName.isEmpty()) {
        qWarning() << "Unabled to parse filename from downloadFile" << remoteFile;
        return false;
    }

    // Strip out parameters from remote filename
    int parameterIndex = remoteFileName.indexOf("?");
    if (parameterIndex != -1) {
        remoteFileName  = remoteFileName.left(parameterIndex);
    }

    // Determine location to download file to
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
        qDebug()<<"ExternalStorageDirectory is no exist!";
    }
    QString jstrDir = jstrObj.toString();
//    qDebug()<<jstrDir<<"wai bu cun chu";
    if(jstrDir.endsWith("/")){
        jstrDir = jstrDir + "NewQGC/";
    }else{
        jstrDir = jstrDir + "/NewQGC/";
    }
    QDir tempDir (jstrDir) ;
    if(!tempDir.exists()){
//        qDebug()<<jstrDir<<"is not exist!";
        tempDir.mkdir(jstrDir);
    }
     _localfile = jstrDir + remoteFileName;
    qDebug()<<_localfile;
#else
    QString localFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    if (localFile.isEmpty()) {
//        qDebug()<<localFile<<"is exist!";
        localFile = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (localFile.isEmpty()) {
            qDebug() << "Unabled to find writable download location. Tried downloads and temp directory.";
            return false;
        }
    }
    localFile += "/"  + remoteFileName;
#endif

    QUrl remoteUrl;
    if (remoteFile.startsWith("http:") || remoteFile.startsWith("https:")) {
        remoteUrl.setUrl(remoteFile);
    } else {
        remoteUrl = QUrl::fromLocalFile(remoteFile);
    }
    if (!remoteUrl.isValid()) {
        qWarning() << "Remote URL is invalid" << remoteFile;
        return false;
    }
    
    QNetworkRequest networkRequest(remoteUrl);

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    setProxy(tProxy);
    QSslConfiguration conf = QSslConfiguration::defaultConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    conf.setProtocol(QSsl::TlsV1_0);
    networkRequest.setSslConfiguration(conf);
    
    // Store local file location in user attribute so we can retrieve when the download finishes
#ifdef __android__
    networkRequest.setAttribute(QNetworkRequest::User, _localfile);
#else
    networkRequest.setAttribute(QNetworkRequest::User, localFile);
#endif
    QNetworkReply* networkReply = get(networkRequest);
    if (!networkReply) {
        qWarning() << "QNetworkAccessManager::get failed";
        return false;
    }

    connect(networkReply, &QNetworkReply::downloadProgress, this, &QGCFileDownload::downloadProgress);
    connect(networkReply, &QNetworkReply::finished, this, &QGCFileDownload::_downloadFinished);
    connect(networkReply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &QGCFileDownload::_downloadError);

    return true;
}

void QGCFileDownload::_downloadFinished(void)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    
    // When an error occurs or the user cancels the download, we still end up here. So bail out in
    // those cases.
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    // Check for redirection
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectionTarget.isNull()) {
        QUrl redirectUrl = reply->url().resolved(redirectionTarget.toUrl());
        download(redirectUrl.toString(), true /* redirect */);
        reply->deleteLater();
        return;
    }

    // Download file location is in user attribute
    QString downloadFilename = reply->request().attribute(QNetworkRequest::User).toString();
    Q_ASSERT(!downloadFilename.isEmpty());
    
    // Store downloaded file in download location
    QFile file(downloadFilename);
//    qDebug()<<downloadFilename;
    if (!file.open(QIODevice::WriteOnly)) {
        emit error(QString("Could not save downloaded file to %1. Error: %2").arg(downloadFilename).arg(file.errorString()));
        return;
    }
//    file.flush ();
    file.write(reply->readAll());
    file.close();
    emit downloadFinished(_originalRemoteFile, downloadFilename);
}

/// @brief Called when an error occurs during download
void QGCFileDownload::_downloadError(QNetworkReply::NetworkError code)
{
    QString errorMsg;
    
    if (code == QNetworkReply::OperationCanceledError) {
        errorMsg = "Download cancelled";

    } else if (code == QNetworkReply::ContentNotFoundError) {
        errorMsg = "Error: File Not Found";

    } else {
        errorMsg = QString("Error during download. Error: %1").arg(code);
    }

    qDebug()<<__FILE__<<" "<<__LINE__<<" "<<errorMsg;
    qgcApp()->showMessage(errorMsg);
    emit error(errorMsg);
}
