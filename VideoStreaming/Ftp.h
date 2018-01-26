#ifndef _FTP_H_
#define _FTP_H_
#include "FTPManager.h"
#include <QThread>
class Ftp :  public QObject
 {
     Q_OBJECT
public:
    Ftp();
	~Ftp();
    void setFtpPath(QString path,QString title);

    QString getMD5();
    float getPorcess();
    void set(QThread* thread);
    void run();
public slots:
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void error(QNetworkReply::NetworkError error);

 private:
    void finish();

     QString _path;
     QString _title;
	 FtpManager* ftp;
     qint64 _bytesSent;
     qint64 _bytesTotal;
     QThread* _thread;
     QFile* file;
 };
 #endif
