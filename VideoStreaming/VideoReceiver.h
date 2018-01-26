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
 *   @brief QGC Video Receiver
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include "QGCLoggingCategory.h"
#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/video/background_segm.hpp"
#include "VideoSurface.h"
#include "QFile"
#if defined(QGC_GST_STREAMING)
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#endif

Q_DECLARE_LOGGING_CATEGORY(VideoReceiverLog)
using namespace cv;
class VideoReceiver : public QObject
{
    Q_OBJECT
public:
#if defined(QGC_GST_STREAMING)
    Q_PROPERTY(bool             recording           READ    recording           NOTIFY recordingChanged)
    Q_PROPERTY(bool             running             READ    running             NOTIFY runningChanged)
#endif
    Q_PROPERTY(VideoSurface*    videoSurface        READ    videoSurface        CONSTANT)
    Q_PROPERTY(bool             videoRunning        READ    videoRunning        NOTIFY videoRunningChanged)
    Q_PROPERTY(QString          imageFile           READ    imageFile           NOTIFY imageFileChanged)
    Q_PROPERTY(bool             showFullScreen      READ    showFullScreen      WRITE setShowFullScreen     NOTIFY showFullScreenChanged)

    explicit VideoReceiver(QObject* parent = 0);
    ~VideoReceiver();

#if defined(QGC_GST_STREAMING)
    bool            running         () { return _running;   }
    bool            recording       () { return _recording; }
    bool            streaming       () { return _streaming; }
    bool            starting        () { return _starting;  }
    bool            stopping        () { return _stopping;  }
#endif

    VideoSurface*   videoSurface    () { return _videoSurface; }
    bool            videoRunning    () { return _videoRunning; }
    QString         imageFile       () { return _imageFile; }
    bool            showFullScreen  () { return _showFullScreen; }
    void            grabImage       (QString imageFile);
    void                        _setVideoSink           (GstElement* sink);

    void        setShowFullScreen   (bool show) { _showFullScreen = show; emit showFullScreenChanged(); }
    //==========================================

    VideoSurface* getvideoRec(){return  _videoSurface;}
    void stoprecording();
    void startrecording();
    void stopimagerecording();

    void startimagerecording();
    void startroirecording();
    void screeneCallback();
    void setRoi(QVariantList roi);
    //===================================================
signals:
    void videoRunningChanged        ();
    void imageFileChanged           ();
    void showFullScreenChanged      ();
#if defined(QGC_GST_STREAMING)
    void recordingChanged           ();
    void runningChanged             ();
    void msgErrorReceived           ();
    void msgEOSReceived             ();
    void msgStateChangedReceived    ();
    void carinfChanged              (QString inf);
#endif

public slots:
    void start                      ();
    void stop                       ();
    void setUri                     (const QString &videoLantency,const QString & uri, bool recordVideo = false, const QString &recordVideoDir = "", const QString &recordPictureDir = "");
    void stopRecording              ();
    void startRecording             ();
 //==================================================
    void setRecordVideo (bool recordVideo);
    void setRecordVideoDir(QString recordVideoDir);
    void setRtsp(bool change);
    void saveImg();
//=====================================================
private slots:
    void _updateTimer               ();
#if defined(QGC_GST_STREAMING)
    void _timeout                   ();
    void _connected                 ();
    void _socketError               (QAbstractSocket::SocketError socketError);
    void _handleError               ();
    void _handleEOS                 ();
    void _handleStateChanged        ();
#endif

private:
#if defined(QGC_GST_STREAMING)
cv::Rect getInterestRect(cv::Mat* frame);
    typedef struct
    {
        GstPad*         teepad;
        GstElement*     queue;
        GstElement*     mux;
        GstElement*     filesink;
        GstElement*     parse;
        gboolean        removing;
    } Sink;
    typedef struct
    {
        GstPad*         teepad;
        GstElement*     queue;
        GstElement*     mux;
        GstElement*     filesink;
        GstElement*     convert;
        GstElement*     videorate;
        gboolean        removing;
    } Sink1;
    bool                _running;
    bool                _recording;
    bool                _streaming;
    bool                _starting;
    bool                _stopping;
    bool                imageRecord=false;
    Sink*               _sink;
    Sink1*               _sink1;
    GstElement*         _tee;
    GstCaps* cap;
    static gboolean             _onBusMessage           (GstBus* bus, GstMessage* message, gpointer user_data);
    static GstPadProbeReturn    _unlinkCallBack         (GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    void                        _detachRecordingBranch  (GstPadProbeInfo* info);
    void                        _shutdownRecordingBranch();
    void                        _shutdownPipeline       ();
    void                        _cleanupOldVideos       ();

    GstElement*     _pipeline;
    GstElement*     _pipelineStopRec;
    GstElement*     _videoSink;
    GstElement*     tee1;

    //-- Wait for Video Server to show up before starting
    QTimer          _frameTimer;
    QTimer          _timer;
    QTcpSocket*     _socket;
    bool            _serverPresent;

#endif

    QString         _uri;
    QString         _imageFile;
    VideoSurface*   _videoSurface;
    bool            _videoRunning;
    bool            _showFullScreen;
    QString         _videoLantency;
    //===================================
    bool        _recordVideo;
    QString     _recordVideoDir;
    QString     _recordPictureDir;
    long imageBufferProbeId=-1;
     QFile *file;
     QVariantList _roi;
    //====================================
    cv::Mat fgMaskMOG2;
     cv::Ptr<cv::BackgroundSubtractor> pMOG2 = createBackgroundSubtractorMOG2();
     long read_file(const char* file_path, unsigned char** buffer);
      Rect expandRect(Rect original, int expandXPixels, int expandYPixels, int maxX, int maxY);
};

#endif // VIDEORECEIVER_H
