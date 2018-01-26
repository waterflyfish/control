#include "Ftp.h"
#include "QEventLoop"
Ftp::Ftp(){
    ftp = new FtpManager();
    //ftp->moveToThread(this);
    //ftp->m_manager.moveToThread(_thread);
    ftp->setHostPort("192.168.1.82",21);
    ftp->setUserInfo("nvidia","nvidia");
    connect(ftp, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
    connect(ftp, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(uploadProgress(qint64, qint64)));
}
Ftp::~Ftp(){
	
}
void Ftp::run(){

    //file->close();
    //delete file;
    //file=NULL;

    //ftp->put("E:\\Qt.zip", "/home/nvidia/Qt.zip");
    QString mm;
    if(_title=="update.conf"){
        mm="/home/nvidia/update/etc/"+_title;
    }else{
        file=new QFile("update.conf");
        file->open(QIODevice::WriteOnly);
        QTextStream out(file);
        out<<"FTP_STAT=1"<<endl<<"FILE_NAME="<<_title<<endl<<"FILE_PATH=/home/nvidia/cmtTldApp/"<<endl;
        out<<"FILE_TYPE=1"<<endl<<"FILE_MD5="<<getMD5()<<endl;
        mm = "/home/nvidia/update/data/"+_title;
    }


    qDebug()<<"mm=="<<_path.mid(8);
    ftp->put(_path.mid(8), mm);
 QEventLoop eventLoop;

   // _path.

   // exec();
}
void Ftp::setFtpPath(QString path,QString title){
    this->_path= path;
    this->_title = title;
}
QString Ftp::getMD5(){
    QFile theFile(_path.mid(8));
        theFile.open(QIODevice::ReadOnly);
        QByteArray ba = QCryptographicHash::hash(theFile.readAll(),QCryptographicHash::Md5);
        QString mm = ba.toHex();
        theFile.close();
        return mm;
}
void Ftp::uploadProgress(qint64 bytesSent, qint64 bytesTotal){
    qDebug()<<"bytesSent==="<<bytesSent;
    qDebug()<<"bytesTotal==="<<bytesTotal;
    float a = (float)bytesSent;
    float b = (float)bytesTotal;
    if(bytesTotal>0)qDebug()<<"ratio==="<<a/b;
     _bytesSent = bytesSent;
     _bytesTotal = bytesTotal;
     if(bytesSent==bytesTotal){
//         disconnect(ftp, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
//         disconnect(ftp, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(uploadProgress(qint64, qint64)));
//         ftp->put("update.conf", "/home/nvidia/update/etc/update.conf");
         //this->quit();
     }
}
float Ftp::getPorcess(){
    float a = (float)_bytesSent;
    float b = (float)_bytesTotal;

    if(_bytesTotal>0)return a/b;
    else return 0;
}
void Ftp::set(QThread* thread){
    _thread = thread;
}
void Ftp::error(QNetworkReply::NetworkError error)
{
    switch (error) {
    case QNetworkReply::HostNotFoundError :
        qDebug() << "can not find the host";
        break;
        // 其他错误处理
    default:
        qDebug()<<"another error";
        break;
    }
}
