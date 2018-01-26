#ifndef QGCUPDATE_H
#define QGCUPDATE_H

#include <QXmlStreamReader>
#include <QDebug>
#include <QFile>

class QGCUpdate : public QObject
{
    Q_OBJECT


public:
    Q_PROPERTY(bool newVersion          READ newVersion         WRITE setNewVersion             NOTIFY newVersionChanged)
    Q_PROPERTY(bool qgcDownload         READ qgcDownload        WRITE setQgcDownlod             NOTIFY qgcDownloadChanged)
    Q_PROPERTY(QString progressBarValue   READ progressBarValue   WRITE setProgressBarValue       NOTIFY progressBarValueChanged)
    Q_PROPERTY(QString detailsText          READ detailsText        WRITE setDetailsText        NOTIFY detailsTextChanged)
    QGCUpdate(QObject* parent = NULL);
    ~QGCUpdate();

    Q_INVOKABLE void updateQGC();
#ifndef __mobile__
    Q_INVOKABLE void startInstall();
#endif

    bool newVersion(void) {return _newVersion;}
    bool qgcDownload(void) {return _qgcDownload;}
    QString progressBarValue(void) {return _progressBarValue;}
    QString detailsText(void){return _detailsText;}

    void setNewVersion(bool newVersion) ;
    void setQgcDownlod(bool qgcDownload) ;
    void setProgressBarValue (QString value);
    void setDetailsText(QString text);

    bool XmlReaderFile(const QString &xmlFile);
    bool XmlReaderSysVersion();
    bool XmlReaderVersion();
    bool XmlReaderUrl();

    void QGCDownload(const QString &remoteFile);
    void downloadProgress(qint64 curr, qint64 total);

signals:
    void newVersionChanged(bool Version);
    void qgcDownloadChanged(bool Download);
    void progressBarValueChanged(QString value);
    void detailsTextChanged(QString text);

public slots:
    bool down(bool Download);
private slots:
    void qgcDownloadFinished(const QString &remoteFile,QString localFile);
    void qgcVersionDownloadFinished(const QString &remoteFile,QString localFile );
private:
    QFile               _xmlFile;
    QXmlStreamReader    _xmlReader;

    bool _newVersion;
    bool _qgcDownload;
//    bool _qgcDownloadFinishedflag;
    QString _progressBarValue;
    QString _detailsText;
};

#endif // QGCUPDATE

