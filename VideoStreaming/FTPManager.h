#ifndef FTP_MANAGER
#define FTP_MANAGER

#include <QUrl>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkAccessManager>
class FtpManager : public QObject
{
    Q_OBJECT

public:
    explicit FtpManager(QObject *parent = 0);

    void setHostPort(const QString &host, int port = 21);
  
    void setUserInfo(const QString &userName, const QString &password);
 
    void put(const QString &fileName, const QString &path);

    void get(const QString &path, const QString &fileName);

signals:
    void error(QNetworkReply::NetworkError);
  
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
   
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    
    void finished();

private:
    QUrl m_pUrl;
    QFile m_file;
    QNetworkAccessManager m_manager;
};

#endif // FTP_MANAGER
