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

#include "VideoReceiver.h"
//#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"
#include <QNetworkProxy>
#include <QNetworkInterface>
#include <QDebug>
#include <QUrl>
#include <QDir>
#include <QDateTime>
#include <QSysInfo>

#include <alpr.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <iterator>
#include<QLibrary>
using namespace alpr;
QGC_LOGGING_CATEGORY(VideoReceiverLog, "VideoReceiverLog")

#if defined(QGC_GST_STREAMING)
static GstPadProbeReturn event_probe_cb(GstPad * pad, GstPadProbeInfo * info, gpointer user_data) {
    Q_UNUSED(pad);
    Q_UNUSED(info);
    VideoReceiver* pipeline = (VideoReceiver*)user_data;
    pipeline->screeneCallback();
    return GST_PAD_PROBE_OK;
}

static const char* kVideoExtensions[] =
{
    "mkv",
    "mov",
    "mp4"
};

static const char* kVideoMuxes[] =
{
    "matroskamux",
    "qtmux",
    "mp4mux"
};

#define NUM_MUXES (sizeof(kVideoMuxes) / sizeof(char*))

#endif

VideoReceiver::VideoReceiver(QObject* parent)
    : QObject(parent)
#if defined(QGC_GST_STREAMING)
    , _running(false)
    , _recording(false)
    , _streaming(false)
    , _starting(false)
    , _stopping(false)
    , _sink(NULL)
    , _tee(NULL)
    , _pipeline(NULL)
    , _pipelineStopRec(NULL)
    , _videoSink(NULL)
    , _socket(NULL)
    , _serverPresent(false)
#endif
    , _videoSurface(NULL)
    , _videoRunning(false)
    , _showFullScreen(false)
{
    _videoSurface  = new VideoSurface;
#if defined(QGC_GST_STREAMING)
    //_setVideoSink(_videoSurface->videoSink());
    _timer.setSingleShot(true);
    file=new QFile("info.txt");
    //if(file.exists())file.flush();


   // connect(&_timer, &QTimer::timeout, this, &VideoReceiver::_timeout);
    connect(this, &VideoReceiver::msgErrorReceived, this, &VideoReceiver::_handleError);
    connect(this, &VideoReceiver::msgEOSReceived, this, &VideoReceiver::_handleEOS);
    connect(this, &VideoReceiver::msgStateChangedReceived, this, &VideoReceiver::_handleStateChanged);
    connect(&_frameTimer, &QTimer::timeout, this, &VideoReceiver::_updateTimer);
    _frameTimer.start(1000);
#endif
}

VideoReceiver::~VideoReceiver()
{
#if defined(QGC_GST_STREAMING)
    //stop();
    if(_socket) {
        delete _socket;
    }
    if (_videoSink) {
        gst_object_unref(_videoSink);
    }
#endif
    if(_videoSurface)
        delete _videoSurface;
}

#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_setVideoSink(GstElement* sink)
{
    if (_videoSink) {
        gst_object_unref(_videoSink);
        _videoSink = NULL;
    }
    if (sink) {
        _videoSink = sink;
        gst_object_ref_sink(_videoSink);
    }
}
#endif

//-----------------------------------------------------------------------------
void
VideoReceiver::grabImage(QString imageFile)
{
    _imageFile = imageFile;
    emit imageFileChanged();
}

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
static void
newPadCB(GstElement* element, GstPad* pad, gpointer data)
{
    gchar* name;
    name = gst_pad_get_name(pad);
    //g_print("A new pad %s was created\n", name);
    GstCaps* p_caps = gst_pad_get_pad_template_caps (pad);
    gchar* description = gst_caps_to_string(p_caps);
    qCDebug(VideoReceiverLog) << p_caps << ", " << description;
    g_free(description);
    GstElement* sink = GST_ELEMENT(data);
    if(gst_element_link_pads(element, name, sink, "sink") == false)
        qCritical() << "newPadCB : failed to link elements\n";
    g_free(name);
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_connected()
{
    //-- Server showed up. Now we start the stream.
    qDebug()<<"conn";
    _timer.stop();
    _socket->deleteLater();
    _socket = NULL;
    _serverPresent = true;
    start();
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_socketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    _socket->deleteLater();
    _socket = NULL;
    //-- Try again in 5 seconds
    _timer.start(5000);
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_timeout()
{
    //-- If socket is live, we got no connection nor a socket error
    if(_socket) {
        delete _socket;
        _socket = NULL;
    }
//    //-- RTSP will try to connect to the server. If it cannot connect,
//    //   it will simply give up and never try again. Instead, we keep
//    //   attempting a connection on this timer. Once a connection is
//    //   found to be working, only then we actually start the stream.
    QUrl url(_uri);
    _socket = new QTcpSocket;
    _socket->setProxy(QNetworkProxy::NoProxy);
    connect(_socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &VideoReceiver::_socketError);
    connect(_socket, &QTcpSocket::connected, this, &VideoReceiver::_connected);
    //qCDebug(VideoReceiverLog) << "Trying to connect to:" << url.host() << url.port();
    _socket->connectToHost(url.host(), url.port());
    _timer.start(5000);
}
#endif

//-----------------------------------------------------------------------------
// When we finish our pipeline will look like this:
//
//                                   +-->queue-->decoder-->_videosink
//                                   |
//    datasource-->demux-->parser-->tee
//
//                                   ^
//                                   |
//                                   +-Here we will later link elements for recording
void
VideoReceiver::start()
{
#if defined(QGC_GST_STREAMING)
    //qCDebug(VideoReceiverLog) << "start()";
    if (_uri.isEmpty()) {
        qCritical() << "VideoReceiver::start() failed because URI is not specified";
        return;
    }
    if (_videoSink == NULL) {
        qCritical() << "VideoReceiver::start() failed because video sink is not set";
        return;
    }
    if(_running) {
        qCDebug(VideoReceiverLog) << "Already running!";
        return;
    }

    _starting = true;
    bool isUdp  = _uri.contains("udp://");
    bool isRtsp = _uri.contains("rtsp://");
    bool isTCP  = _uri.contains("tcp://");

    //-- For RTSP and TCP, check to see if server is there first

    bool running = false;
    bool pipelineUp = false;

    GstCaps*        caps        = NULL;
    GstElement*     dataSource  = NULL;

    GstElement*     demux       = NULL;
    GstElement*     parser      = NULL;
    GstElement*     queue       = NULL;
    GstElement*     decoder     = NULL;
    GstElement*     queue1      = NULL;

    do {
        if ((_pipeline = gst_pipeline_new("receiver")) == NULL) {
            qCritical() << "VideoReceiver::start() failed. Error with gst_pipeline_new()";
            break;
        }
        if(isUdp) {
            dataSource = gst_element_factory_make("udpsrc", "udp-source");
        } else if(isTCP) {
            dataSource = gst_element_factory_make("tcpclientsrc", "tcpclient-source");
        } else {
            dataSource = gst_element_factory_make("rtspsrc", "rtsp-source");
        }
        if (!dataSource) {
            qCritical() << "VideoReceiver::start() failed. Error with data source for gst_element_factory_make()";
            break;
        }

        if(isUdp) {
            if ((caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264")) == NULL) {
                qCritical() << "VideoReceiver::start() failed. Error with gst_caps_from_string()";
                break;
            }
            g_object_set(G_OBJECT(dataSource), "uri", qPrintable(_uri), "caps", caps, NULL);
        } else if(isTCP) {
            QUrl url(_uri);
            g_object_set(G_OBJECT(dataSource), "host", qPrintable(url.host()), "port", url.port(), NULL );
        } else {

             //int m = _videoLantency.toInt();
            g_object_set(G_OBJECT(dataSource), "location", qPrintable(_uri), "latency", 17, "udp-reconnect", 1, "timeout", static_cast<guint64>(5000000), NULL);
        }

        // Currently, we expect H264 when using anything except for TCP.  Long term we may want this to be settable
        if (isTCP) {
            if ((demux = gst_element_factory_make("tsdemux", "mpeg2-ts-demuxer")) == NULL) {
                qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('tsdemux')";
                break;
            }
        } else {
            if ((demux = gst_element_factory_make("rtph264depay", "rtp-h264-depacketizer")) == NULL) {
                qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('rtph264depay')";
                break;
            }
        }

        if ((parser = gst_element_factory_make("h264parse", "h264-parser")) == NULL) {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('h264parse')";
            break;
        }

        if((_tee = gst_element_factory_make("tee", NULL)) == NULL)  {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('tee')";
            break;
        }
        if((tee1 = gst_element_factory_make("tee", NULL)) == NULL)  {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('tee')";
            break;
        }
        if((queue = gst_element_factory_make("queue", NULL)) == NULL)  {
            // TODO: We may want to add queue2 max-size-buffers=1 to get lower latency
            //       We should compare gstreamer scripts to QGroundControl to determine the need
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('queue')";
            break;
        }

        if ((decoder = gst_element_factory_make("avdec_h264", "h264-decoder")) == NULL) {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('avdec_h264')";
            break;
        }

        if ((queue1 = gst_element_factory_make("queue", NULL)) == NULL) {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('queue') [1]";
            break;
        }

        gst_bin_add_many(GST_BIN(_pipeline), dataSource, demux, parser, _tee, queue, decoder,tee1, queue1, _videoSink, NULL);
        pipelineUp = true;

        if(isUdp) {
            // Link the pipeline in front of the tee
            if(!gst_element_link_many(dataSource, demux, parser, _tee, queue, decoder, tee1,queue1, _videoSink, NULL)) {
                qCritical() << "Unable to link UDP elements.";
                break;
            }
        } else if (isTCP) {
            if(!gst_element_link(dataSource, demux)) {
                qCritical() << "Unable to link TCP dataSource to Demux.";
                break;
            }
            if(!gst_element_link_many(parser, _tee, queue, decoder, tee1,queue1, _videoSink, NULL)) {
                qCritical() << "Unable to link TCP pipline to parser.";
                break;
            }
            g_signal_connect(demux, "pad-added", G_CALLBACK(newPadCB), parser);
        } else {
            g_signal_connect(dataSource, "pad-added", G_CALLBACK(newPadCB), demux);
            if(!gst_element_link_many(demux, parser, _tee, queue, decoder,tee1,queue1, _videoSink, NULL)) {
                qCritical() << "Unable to link RTSP elements.";
                break;
            }
        }

        dataSource = demux = parser = queue = decoder = queue1 = NULL;

        GstBus* bus = NULL;

        if ((bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline))) != NULL) {
            gst_bus_enable_sync_message_emission(bus);
            g_signal_connect(bus, "sync-message", G_CALLBACK(_onBusMessage), this);
            gst_object_unref(bus);
            bus = NULL;
        }

        //running = gst_element_set_state(_pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;

    } while(0);

    if (caps != NULL) {
        gst_caps_unref(caps);
        caps = NULL;
    }

    if (!running) {
        qCritical() << "VideoReceiver::start() failed";

        // In newer versions, the pipeline will clean up all references that are added to it
        if (_pipeline != NULL) {
            gst_object_unref(_pipeline);
            _pipeline = NULL;
        }

        // If we failed before adding items to the pipeline, then clean up
        if (!pipelineUp) {
            if (decoder != NULL) {
                gst_object_unref(decoder);
                decoder = NULL;
            }

            if (parser != NULL) {
                gst_object_unref(parser);
                parser = NULL;
            }

            if (demux != NULL) {
                gst_object_unref(demux);
                demux = NULL;
            }

            if (dataSource != NULL) {
                gst_object_unref(dataSource);
                dataSource = NULL;
            }

            if (_tee != NULL) {
                gst_object_unref(_tee);
                dataSource = NULL;
            }

            if (queue != NULL) {
                gst_object_unref(queue);
                dataSource = NULL;
            }
            if (tee1 != NULL) {
                gst_object_unref(tee1);
                dataSource = NULL;
            }

            if (queue1 != NULL) {
                gst_object_unref(queue1);
                dataSource = NULL;
            }
        }

        _running = false;
    } else {
        _running = true;
        qCDebug(VideoReceiverLog) << "Running";
    }
    _starting = false;
#endif
}

//-----------------------------------------------------------------------------
void
VideoReceiver::stop()
{
#if defined(QGC_GST_STREAMING)
    qCDebug(VideoReceiverLog) << "stop()";
    if(!_streaming) {
        _shutdownPipeline();
    } else if (_pipeline != NULL && !_stopping) {
        qCDebug(VideoReceiverLog) << "Stopping _pipeline";
        gst_element_send_event(_pipeline, gst_event_new_eos());
        _stopping = true;
        GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
        GstMessage* message = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_EOS|GST_MESSAGE_ERROR));
        gst_object_unref(bus);
        if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
            _shutdownPipeline();
            qCritical() << "Error stopping pipeline!";
        } else if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
            _handleEOS();
        }
        gst_message_unref(message);
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoReceiver::setUri(const QString & videoLantency,const QString & uri, bool recordVideo, const QString &recordVideoDir, const QString &recordPictureDir)
{
    _uri = uri;
    _videoLantency=videoLantency;
    _recordVideo = recordVideo;
    _recordVideoDir = recordVideoDir;
    _recordPictureDir = recordPictureDir;
}

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_shutdownPipeline() {
    if(!_pipeline) {
        qCDebug(VideoReceiverLog) << "No pipeline";
        return;
    }
    GstBus* bus = NULL;
    if ((bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline))) != NULL) {
        gst_bus_disable_sync_message_emission(bus);
        gst_object_unref(bus);
        bus = NULL;
    }
    gst_element_set_state(_pipeline, GST_STATE_NULL);
    gst_bin_remove(GST_BIN(_pipeline), _videoSink);
    gst_object_unref(_pipeline);
    _pipeline = NULL;
    delete _sink;
    _sink = NULL;
    _serverPresent = false;
    _streaming = false;
    _recording = false;
    _stopping = false;
    _running = false;
    emit recordingChanged();
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_handleError() {
    qCDebug(VideoReceiverLog) << "Gstreamer error!";
    _shutdownPipeline();
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_handleEOS() {
    if(_stopping) {
        _shutdownPipeline();
        qCDebug(VideoReceiverLog) << "Stopped";
    } else if(_recording && _sink->removing) {
        _shutdownRecordingBranch();
    } else {
        qWarning() << "VideoReceiver: Unexpected EOS!";
        _shutdownPipeline();
    }
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_handleStateChanged() {
    if(_pipeline) {
        _streaming = GST_STATE(_pipeline) == GST_STATE_PLAYING;
        qCDebug(VideoReceiverLog) << "State changed, _streaming:" << _streaming;
    }
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
gboolean
VideoReceiver::_onBusMessage(GstBus* bus, GstMessage* msg, gpointer data)
{
    Q_UNUSED(bus)
    Q_ASSERT(msg != NULL && data != NULL);
    VideoReceiver* pThis = (VideoReceiver*)data;

    switch(GST_MESSAGE_TYPE(msg)) {
    case(GST_MESSAGE_ERROR): {
        gchar* debug;
        GError* error;
        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);
        qCritical() << error->message;
        g_error_free(error);
        pThis->msgErrorReceived();
    }
        break;
    case(GST_MESSAGE_EOS):
        pThis->msgEOSReceived();
        break;
    case(GST_MESSAGE_STATE_CHANGED):
        pThis->msgStateChangedReceived();
        break;
    default:
        break;
    }

    return TRUE;
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_cleanupOldVideos()
{
    //-- Only perform cleanup if storage limit is enabled
}
#endif

//-----------------------------------------------------------------------------
// When we finish our pipeline will look like this:
//
//                                   +-->queue-->decoder-->_videosink
//                                   |
//    datasource-->demux-->parser-->tee
//                                   |
//                                   |    +--------------_sink-------------------+
//                                   |    |                                      |
//   we are adding these elements->  +->teepad-->queue-->matroskamux-->_filesink |
//                                        |                                      |
//                                        +--------------------------------------+
void
VideoReceiver::startRecording(void)
{
}

//-----------------------------------------------------------------------------
void
VideoReceiver::stopRecording(void)
{

}

//-----------------------------------------------------------------------------
// This is only installed on the transient _pipelineStopRec in order
// to finalize a video file. It is not used for the main _pipeline.
// -EOS has appeared on the bus of the temporary pipeline
// -At this point all of the recoring elements have been flushed, and the video file has been finalized
// -Now we can remove the temporary pipeline and its elements
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_shutdownRecordingBranch()
{
    gst_bin_remove(GST_BIN(_pipelineStopRec), _sink->queue);
    gst_bin_remove(GST_BIN(_pipelineStopRec), _sink->parse);
    gst_bin_remove(GST_BIN(_pipelineStopRec), _sink->mux);
    gst_bin_remove(GST_BIN(_pipelineStopRec), _sink->filesink);

    gst_element_set_state(_pipelineStopRec, GST_STATE_NULL);
    gst_object_unref(_pipelineStopRec);
    _pipelineStopRec = NULL;

    gst_element_set_state(_sink->filesink,  GST_STATE_NULL);
    gst_element_set_state(_sink->parse,     GST_STATE_NULL);
    gst_element_set_state(_sink->mux,       GST_STATE_NULL);
    gst_element_set_state(_sink->queue,     GST_STATE_NULL);

    gst_object_unref(_sink->queue);
    gst_object_unref(_sink->parse);
    gst_object_unref(_sink->mux);
    gst_object_unref(_sink->filesink);

    delete _sink;
    _sink = NULL;
    _recording = false;

    emit recordingChanged();
    qCDebug(VideoReceiverLog) << "Recording Stopped";
}
#endif

//-----------------------------------------------------------------------------
// -Unlink the recording branch from the tee in the main _pipeline
// -Create a second temporary pipeline, and place the recording branch elements into that pipeline
// -Setup watch and handler for EOS event on the temporary pipeline's bus
// -Send an EOS event at the beginning of that pipeline
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_detachRecordingBranch(GstPadProbeInfo* info)
{
    Q_UNUSED(info)

    // Also unlinks and unrefs
    gst_bin_remove_many(GST_BIN(_pipeline), _sink->queue, _sink->parse, _sink->mux, _sink->filesink, NULL);

    // Give tee its pad back
    gst_element_release_request_pad(_tee, _sink->teepad);
    gst_object_unref(_sink->teepad);

    // Create temporary pipeline
    _pipelineStopRec = gst_pipeline_new("pipeStopRec");

    // Put our elements from the recording branch into the temporary pipeline
    gst_bin_add_many(GST_BIN(_pipelineStopRec), _sink->queue, _sink->parse, _sink->mux, _sink->filesink, NULL);
    gst_element_link_many(_sink->queue, _sink->parse, _sink->mux, _sink->filesink, NULL);

    // Add handler for EOS event
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_pipelineStopRec));
    gst_bus_enable_sync_message_emission(bus);
    g_signal_connect(bus, "sync-message", G_CALLBACK(_onBusMessage), this);
    gst_object_unref(bus);

    if(gst_element_set_state(_pipelineStopRec, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        qCDebug(VideoReceiverLog) << "problem starting _pipelineStopRec";
    }

    // Send EOS at the beginning of the pipeline
    GstPad* sinkpad = gst_element_get_static_pad(_sink->queue, "sink");
    gst_pad_send_event(sinkpad, gst_event_new_eos());
    gst_object_unref(sinkpad);
    qCDebug(VideoReceiverLog) << "Recording branch unlinked";
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
GstPadProbeReturn
VideoReceiver::_unlinkCallBack(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    Q_UNUSED(pad);
    if(info != NULL && user_data != NULL) {
        VideoReceiver* pThis = (VideoReceiver*)user_data;
        // We will only act once
        if(g_atomic_int_compare_and_exchange(&pThis->_sink->removing, FALSE, TRUE)) {
            pThis->_detachRecordingBranch(info);
        }
    }
    return GST_PAD_PROBE_REMOVE;
}
#endif

//-----------------------------------------------------------------------------
void
VideoReceiver::_updateTimer()
{
#if defined(QGC_GST_STREAMING)
    if(_videoSurface) {
        if(stopping() || starting()) {

            return;
        }
        if(streaming()) {

            if(!_videoRunning) {

                _videoSurface->setLastFrame(0);
                _videoRunning = true;
                emit videoRunningChanged();
            }
        } else {

            if(_videoRunning) {
                _videoRunning = false;
                emit videoRunningChanged();
            }
        }
        if(_videoRunning) {

            uint32_t timeout = 2;
            time_t elapsed = 0;
            time_t lastFrame = _videoSurface->lastFrame();
            if(lastFrame != 0) {
                elapsed = time(0) - _videoSurface->lastFrame();
            }
            if(elapsed > (time_t)timeout && _videoSurface) {
                _videoRunning = false;
               _videoSurface->setLastFrame(0);
               emit videoRunningChanged();
            }
        } else {
            if(!running() && !_uri.isEmpty()) {
                start();
            }
        }
    }
#endif
}




//===========================================================================
void VideoReceiver::stoprecording (){
#if defined(QGC_GST_STREAMING)
    qCDebug(VideoReceiverLog) << "stopRecording()";
    // exit immediately if we are not recording
    if(_pipeline == NULL || !_recording) {
        qCDebug(VideoReceiverLog) << "Not recording!";
        return;
    }
    // Wait for data block before unlinking
    gst_pad_add_probe(_sink->teepad, GST_PAD_PROBE_TYPE_IDLE, _unlinkCallBack, this, NULL);
#endif
}
void VideoReceiver::startrecording (){
#if defined(QGC_GST_STREAMING)
        QString recordVideoName = (_recordVideoDir + "/XH" + (QDateTime::currentDateTime()).toString("yyyyMMddhhmmsszzz") + ".mkv");
        QDir tempDir(_recordVideoDir);
        if(!tempDir.exists ()){
            tempDir.mkdir(_recordVideoDir);
        }
        int times = 0;
        do {
            static const char rgDigits[] = "0123456789";
            QString uniqueStr;
            for (int i = 0; i < 6; i++) {
                uniqueStr += rgDigits[qrand() % 10];
            }
            recordVideoName = (_recordVideoDir + "/XH" + (QDateTime::currentDateTime()).toString("yyyyMMddhhmmsszzz") + uniqueStr + ".mkv");
            times++;
        } while (tempDir.exists(recordVideoName) && times < 10);
        _sink           = new Sink();
        _sink->teepad   = gst_element_get_request_pad(_tee, "src_%u");
        _sink->queue    = gst_element_factory_make("queue", NULL);
        _sink->parse    = gst_element_factory_make("h264parse", NULL);
        _sink->mux      = gst_element_factory_make("matroskamux", NULL);
        _sink->filesink = gst_element_factory_make("filesink", NULL);
        _sink->removing = false;

        if(!_sink->teepad || !_sink->queue || !_sink->mux || !_sink->filesink || !_sink->parse) {
            qCritical() << "VideoReceiver::startRecording() failed to make _sink elements";
            return;
        }
        g_object_set(G_OBJECT(_sink->filesink), "location", recordVideoName.toStdString ().c_str (), NULL);
        gst_object_ref(_sink->queue);
        gst_object_ref(_sink->parse);
        gst_object_ref(_sink->mux);
        gst_object_ref(_sink->filesink);

        gst_bin_add_many(GST_BIN(_pipeline), _sink->queue, _sink->parse, _sink->mux, _sink->filesink, NULL);
        gst_element_link_many(_sink->queue, _sink->parse, _sink->mux, _sink->filesink, NULL);

        gst_element_sync_state_with_parent(_sink->queue);
        gst_element_sync_state_with_parent(_sink->parse);
        gst_element_sync_state_with_parent(_sink->mux);
        gst_element_sync_state_with_parent(_sink->filesink);

        GstPad* sinkpad = gst_element_get_static_pad(_sink->queue, "sink");
        gst_pad_link(_sink->teepad, sinkpad);
        gst_object_unref(sinkpad);

        _recording = true;

   // }
#endif
}
void VideoReceiver::startimagerecording (){
    _sink1           = new Sink1();
    _sink1->teepad   = gst_element_get_request_pad(tee1, "src_%u");
    _sink1->queue    = gst_element_factory_make("queue", NULL);
    _sink1->mux      = gst_element_factory_make("jpegenc", NULL);
    _sink1->videorate = gst_element_factory_make("valve", NULL);
    _sink1->filesink = gst_element_factory_make("filesink", NULL);
    _sink1->removing = false;
    if( !_sink1->queue || !_sink1->mux || !_sink1->filesink || !_sink1->videorate) {
        qCritical() << "VideoReceiver::startRecording() failed to make _sink elements";
        return;
    }
    g_object_set(G_OBJECT(_sink1->filesink), "location",(_recordPictureDir+"/XH" + (QDateTime::currentDateTime()).toString("yyyyMMddhhmmsszzz") + ".jpeg").toStdString ().c_str (), NULL);
    gst_bin_add_many(GST_BIN(_pipeline), _sink1->queue, _sink1->mux, _sink1->filesink,_sink1->videorate, NULL);
    gst_element_link_many(_sink1->queue ,_sink1->mux, _sink1->videorate,_sink1->filesink, NULL);
    gst_element_set_state(_sink1->filesink, GST_STATE_PLAYING);
    gst_element_set_state(_sink1->queue, GST_STATE_PLAYING);
    gst_element_set_state(_sink1->videorate, GST_STATE_PLAYING);
    gst_element_set_state(_sink1->mux, GST_STATE_PLAYING);
    imageRecord=false;
    GstPad* sinkpad = gst_element_get_static_pad(_sink1->queue, "sink");
    gst_pad_link(_sink1->teepad, sinkpad);

    GstPad * pad = gst_element_get_static_pad(_sink1->videorate, "src");
    imageBufferProbeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, event_probe_cb, this, NULL);

    g_object_set(G_OBJECT(_sink1->videorate), "drop", (gboolean) FALSE, NULL);
    gst_object_unref(sinkpad);
    _recording = true;

}
void VideoReceiver::startroirecording (){
    _sink1           = new Sink1();
    _sink1->teepad   = gst_element_get_request_pad(tee1, "src_%u");
    _sink1->queue    = gst_element_factory_make("queue", NULL);
    _sink1->mux      = gst_element_factory_make("jpegenc", NULL);
    _sink1->videorate = gst_element_factory_make("valve", NULL);
    _sink1->filesink = gst_element_factory_make("filesink", NULL);
    _sink1->removing = false;
    if( !_sink1->queue || !_sink1->mux || !_sink1->filesink || !_sink1->videorate) {
        qCritical() << "VideoReceiver::startRecording() failed to make _sink elements";
        return;
    }
    qDebug()<<"wwwwwwwwwwwwwwwww!!!!!!1";
    g_object_set(G_OBJECT(_sink1->filesink), "location", "roi.jpeg", NULL);
    gst_bin_add_many(GST_BIN(_pipeline), _sink1->queue, _sink1->mux, _sink1->filesink,_sink1->videorate, NULL);
    gst_element_link_many(_sink1->queue ,_sink1->mux, _sink1->videorate,_sink1->filesink, NULL);
    gst_element_set_state(_sink1->filesink, GST_STATE_PLAYING);
    gst_element_set_state(_sink1->queue, GST_STATE_PLAYING);
    gst_element_set_state(_sink1->videorate, GST_STATE_PLAYING);
    gst_element_set_state(_sink1->mux, GST_STATE_PLAYING);
    imageRecord=false;
    GstPad* sinkpad = gst_element_get_static_pad(_sink1->queue, "sink");
    gst_pad_link(_sink1->teepad, sinkpad);

    GstPad * pad = gst_element_get_static_pad(_sink1->videorate, "src");
    imageBufferProbeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, event_probe_cb, this, NULL);

    g_object_set(G_OBJECT(_sink1->videorate), "drop", (gboolean) FALSE, NULL);
    gst_object_unref(sinkpad);
    _recording = true;

}
void VideoReceiver::stopimagerecording (){
#if defined(QGC_GST_STREAMING)
    if(imageRecord){
        GstPad * blockpadd = gst_element_get_static_pad(_sink1->queue, "src");
        gst_pad_push_event(blockpadd, gst_event_new_eos());
    }else{
        GstPad * pad = gst_element_get_static_pad(_sink1->videorate, "src");
        gst_pad_remove_probe(pad, imageBufferProbeId);
    }
    gst_element_set_state(_sink1->filesink, GST_STATE_NULL);
    gst_element_set_state(_sink1->mux, GST_STATE_NULL);
    gst_element_set_state(_sink1->queue, GST_STATE_NULL);
    gst_element_set_state(_sink1->videorate, GST_STATE_NULL);
    gst_bin_remove(GST_BIN(_pipeline), _sink1->mux);
    gst_bin_remove(GST_BIN(_pipeline), _sink1->filesink);
    gst_bin_remove(GST_BIN(_pipeline), _sink1->queue);
    gst_bin_remove(GST_BIN(_pipeline), _sink1->videorate);
    gst_element_release_request_pad(tee1, _sink1->teepad);
    gst_object_unref(_sink1->teepad);
    _sink1->queue = NULL;
    if (_sink1->queue!= NULL) {
        gst_object_unref(_sink1->queue);
        _sink1->queue = NULL;
    }
    _sink1->filesink;
    if (_sink1->filesink != NULL) {
        gst_object_unref(_sink1->filesink);
        _sink1->filesink = NULL;
    }
    _sink1->mux = NULL;
    if (_sink1->mux != NULL) {
        gst_object_unref(_sink1->mux);
        _sink1->mux = NULL;
    }
    _sink1->convert = NULL;
    if (_sink1->convert != NULL) {
        gst_object_unref(_sink1->convert);
        _sink1->convert= NULL;
    }
    _sink1->videorate = NULL;
    if (_sink1->videorate != NULL) {
        gst_object_unref(_sink1->videorate);
        _sink1->videorate= NULL;
    }
    delete _sink1;
    _sink1 = NULL;
#endif

}

void VideoReceiver::setRecordVideo (bool recordVideo){
    _recordVideo = recordVideo;
}

void VideoReceiver::setRecordVideoDir(QString recordVideoDir){
    _recordVideoDir = recordVideoDir;
}

void VideoReceiver::screeneCallback(){
#if defined(QGC_GST_STREAMING)
    g_object_set(G_OBJECT(_sink1->videorate), "drop", (gboolean) TRUE, NULL);
    GstPad * pad = gst_element_get_static_pad(_sink1->videorate, "src");
    //stopimagerecording();
    gst_pad_remove_probe(pad, imageBufferProbeId);
    imageRecord=true;
   qDebug()<<"int==="<<imageBufferProbeId;
#endif
}
void VideoReceiver::setRtsp(bool change){
    if(change){
        _shutdownPipeline();
        _uri = "rtsp://192.168.1.82:8554/test1";
        start();
    }else{
        _shutdownPipeline();
         _uri = "rtsp://192.168.1.82:8554/test";
        start();
    }
}
void VideoReceiver::saveImg(){
//    GstMapInfo map;
//    GstSample *sample = gst_app_sink_pull_sample((GstAppSink*)_sink1->filesink);
//    GstBuffer *buffer = gst_sample_get_buffer(sample);
//    int size = gst_buffer_get_size(buffer);
//    qDebug()<<"size=="<<size;
//    gst_buffer_map(buffer,&map,GST_MAP_READ);
//    QString str = (_recordPictureDir+"/XH" + (QDateTime::currentDateTime()).toString("yyyyMMddhhmmsszzz") + ".jpg");

//    QByteArray ba = str.toLatin1();
//     char *mm = ba.data();
//     QImage iim((uchar*)map.data,240,160,QImage::Format_RGB32);
//     iim.save(str);
//    FILE *fp = fopen(mm, "wb");
//   // fwrite(&map.data, 1, size, fp);
//      //    fclose(fp);
////          gst_buffer_unref (buffer);
       //================================================
    //======================================================

//    OPENALPR* alpr_obj;
//QLibrary lib("../libopenalpr.dll");
//if(lib.load()){
// qDebug()<<"cccccccccccccccccc";
// alpr_obj = openalpr_init("us","", "","");

// if (openalpr_is_loaded(alpr_obj))
// {
//     // We don't want to restrict the size of the recognition area, so we set this to an extremely large pixel size
//     // rather than decode and find the actual image width/height.
//     struct AlprCRegionOfInterest roi;
//     roi.x = 0;
//     roi.y = 0;
//     roi.width = 10000;
//     roi.height = 10000;

//     // Read the image file
//     unsigned char* buffer;
//     long long length = read_file("eu-1.jpg", &buffer);

//     printf("file size (bytes): %d\n", length);

//     if (length > 0)
//     {
//         char* plate_response = openalpr_recognize_encodedimage(alpr_obj, buffer, length, roi);
//         printf("Alpr response:\n%s\n", plate_response);
//         openalpr_free_response_string(plate_response);
//     }

//     free(buffer);


// }

// openalpr_cleanup(alpr_obj);

//}

        // Leave the config and runtime directory blank to look for these in the current directory.

    //=======================================================
    qDebug()<<"this is alpr";
    Mat img = imread("roi.jpeg",1);
    Mat imageROI;
    int x=_roi[0].toInt();
    int y=_roi[1].toInt();
    int width = _roi[2].toInt()-x;
    int height = _roi[3].toInt()-y;
    imageROI = img(Rect(x, y, width, height));
    imwrite("carimg.jpeg",imageROI);
    Alpr alpr("eu", "openalpr.conf");
    alpr.setTopN(10);
    alpr.setDefaultRegion("");
    QString mm;
    if (alpr.isLoaded() == false)
    {
        //std::cerr << "Error loading OpenALPR" << std::endl;
        //return 1;
    }
   // ================================================================
    alpr::AlprResults results = alpr.recognize("carimg.jpeg");
 //file->open(QIODevice::WriteOnly);
    for (int i = 0; i < results.plates.size(); i++)
    {
      alpr::AlprPlateResult plate = results.plates[i];
      mm.append("plate").append(QString::number(i, 10)).append(": ").append(QString::number(plate.topNPlates.size(), 10)).append(" results").append("\n");
      std::cout << "plate" << i << ": " << plate.topNPlates.size() << " results" << std::endl;
     // QTextStream out(file);
            // out <<endl << "plate" << i << ": " << plate.topNPlates.size() << " results" <<endl ;
        for (int k = 0; k < plate.topNPlates.size(); k++)
        {
          alpr::AlprPlate candidate = plate.topNPlates[k];

          std::cout << "    - " << candidate.characters << "\t confidence: " << candidate.overall_confidence;
          mm.append( "    - ").append(QString::fromStdString(candidate.characters)).append(" confidence: ").append(QString::number( candidate.overall_confidence, 'f', 4 )).append("\n");
          // QTextStream out1(file);
          // out1 <<endl<< "    - " << QString::fromStdString(candidate.characters) << "\t confidence: " << QString::number( candidate.overall_confidence, 'f', 4 )<<endl;
          std::cout << "\t pattern_match: " << candidate.matches_template << std::endl;
        }
    }
    qDebug()<<"mm=="<<mm;
    // file->close();
    //======================================================================
//    cv::Mat frame = cv::imread("eu-2.jpg");
//    std::vector<AlprRegionOfInterest> regionsOfInterest;
//    qDebug()<<"frame=="<<frame.rows;
//    cv::Rect rectan = getInterestRect(&frame);
//    qDebug()<<"width=="<<rectan.width;
//    if (rectan.width>0) regionsOfInterest.push_back(AlprRegionOfInterest(rectan.x, rectan.y, rectan.width, rectan.height));


//     AlprResults results;
//     if (regionsOfInterest.size()>0) results = alpr.recognize(frame.data, frame.elemSize(), frame.cols, frame.rows, regionsOfInterest);
//         for (int i = 0; i < results.plates.size(); i++)
//         {
//           alpr::AlprPlateResult plate = results.plates[i];
//           std::cout << "plate" << i << ": " << plate.topNPlates.size() << " results" << std::endl;

//             for (int k = 0; k < plate.topNPlates.size(); k++)
//             {
//               alpr::AlprPlate candidate = plate.topNPlates[k];
//               std::cout << "    - " << candidate.characters << "\t confidence: " << candidate.overall_confidence;
//               std::cout << "\t pattern_match: " << candidate.matches_template << std::endl;
//             }
//         }
    emit carinfChanged(mm);
}
cv::Rect VideoReceiver::getInterestRect(cv::Mat* frame){
    std::vector<std::vector<cv::Point> > contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::Rect bounding_rect;
        std::vector<cv::Rect> rects;
        cv::Rect largest_rect, rect_temp;
        qDebug()<<"frame=="<<frame->rows;
        qDebug()<<"111111111111111111";
        pMOG2->apply(*frame, fgMaskMOG2);
        qDebug()<<"222222222222222222";

        //Remove noise
        cv::erode(fgMaskMOG2, fgMaskMOG2, getStructuringElement(cv::MORPH_RECT, cv::Size(6, 6)));
        // Find the contours of motion areas in the image
        findContours(fgMaskMOG2, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
        // Find the bounding rectangles of the areas of motion
        if (contours.size() > 0)
        {
            for (int i = 0; i < contours.size(); i++)
            {
                bounding_rect = boundingRect(contours[i]);
                rects.push_back(bounding_rect);
            }
            // Determine the overall area with motion.
            largest_rect = rects[0];
            for (int i = 1; i < rects.size(); i++)
            {
                rect_temp.x = min(largest_rect.x,rects[i].x);
                rect_temp.y = min(largest_rect.y,rects[i].y);
                rect_temp.width = max(largest_rect.x + largest_rect.width, rects[i].x + rects[i].width)-rect_temp.x;
                rect_temp.height = max(largest_rect.y + largest_rect.height, rects[i].y + rects[i].height) - rect_temp.y;
                largest_rect = rect_temp;
            }
            rectangle(*frame, rect_temp, cv::Scalar(0, 255, 0), 1, 8, 0);
        }
        else
            {
              largest_rect.x = 0;
              largest_rect.y = 0;
              largest_rect.width = 0;
              largest_rect.height = 0;
            }

    //	imshow("Motion detect", fgMaskMOG2);
        return expandRect(largest_rect, 0, 0, frame->cols, frame->rows);
}
Rect VideoReceiver::expandRect(Rect original, int expandXPixels, int expandYPixels, int maxX, int maxY)
  {
    Rect expandedRegion = Rect(original);

    float halfX = round((float) expandXPixels / 2.0);
    float halfY = round((float) expandYPixels / 2.0);
    expandedRegion.x = expandedRegion.x - halfX;
    expandedRegion.width =  expandedRegion.width + expandXPixels;
    expandedRegion.y = expandedRegion.y - halfY;
    expandedRegion.height =  expandedRegion.height + expandYPixels;

    expandedRegion.x = std::min(std::max(expandedRegion.x, 0), maxX);
    expandedRegion.y = std::min(std::max(expandedRegion.y, 0), maxY);
    if (expandedRegion.x + expandedRegion.width > maxX)
      expandedRegion.width = maxX - expandedRegion.x;
    if (expandedRegion.y + expandedRegion.height > maxY)
      expandedRegion.height = maxY - expandedRegion.y;

    return expandedRegion;
  }
long VideoReceiver::read_file(const char *file_path, unsigned char **buffer){
    FILE *fileptr;
        long filelen;

        fileptr = fopen(file_path, "rb");     // Open the file in binary mode
        if (!fileptr)
            return 0;

        fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
        filelen = ftell(fileptr);             // Get the current byte offset in the file
        rewind(fileptr);                      // Jump back to the beginning of the file

        *buffer = (unsigned char *)malloc((filelen+1)*sizeof(char)); // Enough memory for file + \0
        fread(*buffer, filelen, 1, fileptr); // Read in the entire file
        fclose(fileptr); // Close the file

        return filelen;
}
void VideoReceiver::setRoi(QVariantList roi){
    _roi = roi;
}
