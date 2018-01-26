/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include <QQmlContext>
#include <QQmlEngine>
#include <QDateTime>
#include <QDir>
#include <QSettings>

#ifndef QGC_DISABLE_UVC
#include <QCameraInfo>
#endif

#include <VideoItem.h>

#if defined __android__ || defined __mobile__
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QAndroidJniObject>
static const char kJniClassName[] {"android/os/Environment"};
static const char kJniClassFunction[] {"getExternalStorageDirectory"};
#endif

#include "ScreenToolsController.h"
#include "VideoManager.h"

static const char* kVideoSourceKey  = "VideoSource";
static const char* kVideoUDPPortKey = "VideoUDPPort";
static const char* kVideoRTSPUrlKey = "VideoRTSPUrl";
static const char* kVideoLATENCYKey = "VideoLATENCY";
static const char* kUDPStream       = "UDP Video Stream";
#if defined(QGC_GST_STREAMING)
static const char* kRTSPStream      = "RTSP Video Stream";
#endif
static const char* kNoVideo         = "No Video Available";
//static const char* kRecordVideo     = "XHQGCRecordVideo";

QGC_LOGGING_CATEGORY(VideoManagerLog, "VideoManagerLog")

//-----------------------------------------------------------------------------
VideoManager::VideoManager(QGCApplication* app)
    : QGCTool(app)
    , _videoSurface(NULL)
    , _videoReceiver(NULL)
    , _videoRunning(false)
    , _udpPort(5600) //-- Defalut Port 5600 == Solo UDP Port
    , _init(false)
    , _recordVideo(true)
    , _recordImage(true)
    ,_videoChoice(false)
    , _recordVideoDir()
    , _recordPictureDir()
    ,_counter(0)
{
    //-- Get saved settings
    QSettings settings;
    //_videoSurface  = new VideoSurface;
    car = new CarInfo();
    thread = new QThread();
    ftp = new Ftp();

   //ftp->moveToThread(thread);
    //ftp->set(thread);
    _videoReceiver = new VideoReceiver(this);
    car->setVideoRece(_videoReceiver);
    connect(_videoReceiver,&VideoReceiver::carinfChanged,this,&VideoManager::setInf);
    //car->setVideoMana(this);
    connect(this,&VideoManager::change,_videoReceiver,&VideoReceiver::setRtsp);
    setVideoSource(settings.value(kVideoSourceKey,
#if defined(QGC_GST_STREAMING)
                                  kRTSPStream
#else
                                  kUDPStream
#endif
                                  ).toString());

    setUdpPort(settings.value(kVideoUDPPortKey, 5600).toUInt());
    setRtspURL(settings.value(kVideoRTSPUrlKey, "rtsp://192.168.42.123:8557/Onvif/Streaming/2?videoCodecType=H.264").toString()); //-- Example RTSP URL
    setVideoLatency(settings.value(kVideoLATENCYKey,"0000").toString());
#if !defined __android__ && !defined __mobile__
    QDir tempDir(QDir::currentPath() + "/recordVideo/");
    if(!tempDir.exists ()){
        tempDir.mkdir(QDir::currentPath() + "/recordVideo/");
    }
    QDir tempDir1(QDir::currentPath() + "/recordJpeg/");
    if(!tempDir1.exists ()){
        tempDir1.mkdir(QDir::currentPath() + "/recordJpeg/");
    }
#endif

    _init = true;

#if defined(QGC_GST_STREAMING)

  // connect(&_frameTimer, &QTimer::timeout, this, &VideoManager::_updateTimer);
   // if(_frameTimer.isActive())
    //    _frameTimer.stop();
     _updateVideo();
#endif
}

//-----------------------------------------------------------------------------
VideoManager::~VideoManager()
{

}

//-----------------------------------------------------------------------------
void
VideoManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);
   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<VideoManager>("QGroundControl.VideoManager", 1, 0, "VideoManager", "Reference only");
}

//-----------------------------------------------------------------------------
bool
VideoManager::hasVideo()
{
#if defined(QGC_GST_STREAMING)
    return true;
#endif
    return !_videoSource.isEmpty();
}

//-----------------------------------------------------------------------------
bool
VideoManager::isGStreamer()
{
#if defined(QGC_GST_STREAMING)
    return _videoSource == kUDPStream || _videoSource == kRTSPStream;
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------
#ifndef QGC_DISABLE_UVC
bool
VideoManager::uvcEnabled()
{
    return QCameraInfo::availableCameras().count() > 0;
}
#endif

//-----------------------------------------------------------------------------
void
VideoManager::setVideoSource(QString vSource)
{
    if(vSource == kNoVideo)
        return;
    _videoSource = vSource;
    QSettings settings;
    settings.setValue(kVideoSourceKey, vSource);
    emit videoSourceChanged();
#ifndef QGC_DISABLE_UVC
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, cameras) {
        if(cameraInfo.description() == vSource) {
            _videoSourceID = cameraInfo.deviceName();
            emit videoSourceIDChanged();
            qCDebug(VideoManagerLog) << "Found USB source:" << _videoSourceID << " Name:" << _videoSource;
            break;
        }
    }
#endif
    emit isGStreamerChanged();
    qCDebug(VideoManagerLog) << "New Video Source:" << vSource;
    /*
     * Not working. Requires restart for now. (Undef KRTSP/kUDP above when enabling this)
    if(isGStreamer())
        _updateVideo();
    */
    if(_videoReceiver) {
        if(isGStreamer()) {
            _videoReceiver->start();
        } else {
            _videoReceiver->stop();
        }
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::setUdpPort(quint16 port)
{
    _udpPort = port;
    QSettings settings;
    settings.setValue(kVideoUDPPortKey, port);
    emit udpPortChanged();
    /*
     * Not working. Requires restart for now. (Undef KRTSP/kUDP above when enabling this)
    if(_videoSource == kUDPStream)
        _updateVideo();
    */
}

//-----------------------------------------------------------------------------
void
VideoManager::setRtspURL(QString url)
{
    _rtspURL = url;
    QSettings settings;
    settings.setValue(kVideoRTSPUrlKey, url);
    emit rtspURLChanged();
    /*
     * Not working. Requires restart for now. (Undef KRTSP/kUDP above when enabling this)
    if(_videoSource == kRTSPStream)
        _updateVideo();
    */
}

void VideoManager::getRecordVideoName(){
#if defined __android__ || defined __mobile__
    //  REGISTER THE C++ FUNCTION WITH JNI
    if(!_recordVideoDir.isEmpty ()){
        QDir tempDir(_recordVideoDir);
        if(tempDir.exists ()){
            qDebug() << __FILE__<<" "<<__LINE__<<" "<<tempDir.absolutePath ();
        }
    }

    QString jstrDir;
    QString jstrDirVideo;
    QString jstrDirPicture;
    cleanJavaException();
    QAndroidJniEnvironment jniEnv;
    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }

    jclass objectClass = jniEnv->FindClass(kJniClassName);
    if(!objectClass) {
        return;
    }

    QAndroidJniObject  jstrObj = QAndroidJniObject::callStaticObjectMethod(
                kJniClassName,
                kJniClassFunction,
                "()Ljava/io/File;");
    if(!jstrObj.isValid()){
        qDebug() << "getExternalStorageDirectory result is error!";
    }

    jstrDir = jstrObj.toString ();

    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }

    if(jstrDir.endsWith ("/")){
        jstrDirVideo = (jstrDir + "recordVideo/");
        jstrDirPicture = (jstrDir + "recordJpeg/");
    }else{
        jstrDirVideo = (jstrDir + "/recordVideo/");
        jstrDirPicture = (jstrDir + "/recordJpeg/");
    }
    QDir tempVideo(jstrDirVideo);
    if(!tempVideo.exists ()){
        qDebug() << jstrDirVideo<<" not exits! now mkdir";
        tempVideo.mkdir (jstrDirVideo);
    }

    QDir tempPicture(jstrDirPicture);
    if(!tempPicture.exists ()){
        qDebug() << jstrDirPicture<<" not exits! now mkdir";
        tempPicture.mkdir (jstrDirPicture);
    }
#else
   QDir tempVideo(QDir::currentPath() + "/recordVideo/");
   QDir tempPicture(QDir::currentPath() + "/recordJpeg/");
#endif
    if(tempVideo.exists () && tempPicture.exists ()){
#if defined __android__ || defined __mobile__
        _recordVideoDir = jstrDirVideo;
        _recordPictureDir = jstrDirPicture;
#else
        _recordVideoDir = tempVideo.absolutePath ();
        _recordPictureDir = tempPicture.absolutePath ();
#endif
   // qDebug()<<"Video dir : "<< _recordVideoDir <<" , Picture dir : "<< _recordPictureDir;
    }
}

#if defined __android__ || defined __mobile__
void VideoManager::cleanJavaException()
{
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}
#endif

//-----------------------------------------------------------------------------
QStringList
VideoManager::videoSourceList()
{
    _videoSourceList.clear();
#if defined(QGC_GST_STREAMING)
    _videoSourceList.append(kUDPStream);
    _videoSourceList.append(kRTSPStream);
#endif
#ifndef QGC_DISABLE_UVC
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, cameras) {
        qCDebug(VideoManagerLog) << "UVC Video source ID:" << cameraInfo.deviceName() << " Name:" << cameraInfo.description();
        _videoSourceList.append(cameraInfo.description());
    }
#endif
    if(_videoSourceList.count() == 0)
        _videoSourceList.append(kNoVideo);
    return _videoSourceList;
}

//-----------------------------------------------------------------------------
void VideoManager::_updateTimer()
{
#if defined(QGC_GST_STREAMING)
//    if(_videoRunning)
//    {
//        time_t elapsed = 0;
//        if(_videoSurface)
//        {
//            elapsed = time(0) - _videoSurface->lastFrame();
//        }
//        if(elapsed > 2 && _videoSurface)
//        {
//            _videoRunning = false;
//            _videoSurface->setLastFrame(0);
//            emit videoRunningChanged();
//        }
//    }
//    else
//    {
//          _counter++;
//        if(_counter > 3){
//            _videoReceiver->stop();
//            _counter =  0;
//            emit videoRunningChanged();
//            _updateVideo();
//        }
//        if(_videoSurface && _videoSurface->lastFrame()) {
//            if(!_videoRunning)
//            {
//                _videoRunning = true;
//                emit videoRunningChanged();
//            }
//        }
//    }
#endif
}

//-----------------------------------------------------------------------------
void VideoManager::_updateVideo()
{
    qDebug()<<"CAOCAO!!!!!";
    if(_init){
#if defined(QGC_GST_STREAMING)
        _counter  = 0;
        _frameTimer.stop();
        _videoSurface=_videoReceiver->getvideoRec();
        _videoReceiver->_setVideoSink(_videoSurface->videoSink());
        getRecordVideoName();
        if(_videoSource == kUDPStream)
            _videoReceiver->setUri(_videoLatency,QStringLiteral("udp://0.0.0.0:%1").arg(_udpPort), _recordVideo, _recordVideoDir, _recordPictureDir);
        else

            _videoReceiver->setUri(_videoLatency,_rtspURL, _recordVideo, _recordVideoDir, _recordPictureDir);
        #endif
        _videoReceiver->start();
#if  defined(QGC_GST_STREAMING)
       // _frameTimer.start(1000);
#endif
    }
}
//================================================================
bool VideoManager::saveVideo(){

    return _saveVideo;
}
bool VideoManager::saveImage(){

    return _saveImage;
}
bool VideoManager::carInfo(){
    return _carInfo;
}
bool VideoManager::updateInfo(){
    return _updateInfo;
}

bool VideoManager::videoChoice(){
    return _videoChoice;
}
bool VideoManager::trackFlag(){
    return _trackFlag;
}
//QVector<int> VideoManager::location()(){
//    return _location ;
//}

void VideoManager::setInf(QString inf){
    qDebug()<<"shou dao xiao xi";
    _inf = inf;
    emit infChanged();
}

void VideoManager::setSaveVideo(bool saveVideo){

    _recordVideo = saveVideo;
    if(!_recordVideo){
        if(_videoReceiver!=NULL)_videoReceiver->stoprecording();
    }
    else{
        if(_videoReceiver!=NULL)_videoReceiver->startrecording();
    }
    emit saveVideoChanged();
}
void VideoManager::setSaveImage(bool saveImage){

    _recordImage = saveImage;
    if(!_recordImage){
        if(_videoReceiver!=NULL)_videoReceiver->stopimagerecording();
    }
    else{
        if(_videoReceiver!=NULL)_videoReceiver->startimagerecording();

    }
    emit saveImageChanged();
}
void VideoManager::setCarInfo(bool carInfo){
    _carInfo = carInfo;
   _videoReceiver->setRoi(_location);
    car->setSaveImg(false);
    if(car&&_trackFlag)car->start();

    emit carInfoChanged();
}

void VideoManager::setVideoChoice(bool videoChoice){
    _videoChoice=videoChoice;
    emit videoChoiceChanged();
    emit change(_videoChoice);
}
void
VideoManager::setVideoLatency(QString videoLatency)
{
    _videoLatency = videoLatency;
    QSettings settings;
    settings.setValue(kVideoLATENCYKey, videoLatency);
    emit videoLatencyChanged();

}
void VideoManager::setUpdateInfo(bool updateInfo){
    _updateInfo=updateInfo;
    emit updateInfoChanged();
}
void VideoManager::setTrackFlag(bool trackFlag){
    qDebug()<<"track flag";
    _trackFlag = trackFlag;
    emit trackFlagChanged();
}
void VideoManager::setLocation(QVariantList list){
//    foreach (int i, list) {
//           qDebug() << list.at(i);
//       }
//    for(int i=0;i<list.count();i++){
//        qDebug()<<"arawr=="<< list[i].toInt();
//    }
   // qDebug()<<"count=="<list.count();
    _location.clear();
    _location  = list ;
    for(int i=0;i<list.count();i++){
        qDebug()<<"arawr=="<< _location[i].toInt();
    }
    //car->setSaveImg(true);
   // car->start();
}

void VideoManager::setUpdateInf(QString updateInf){
    _updateInf = updateInf;
    int first = updateInf.lastIndexOf("/");
    QString title = updateInf.right(updateInf.length ()-first-1);
    qDebug()<<"path=="<<title;

    if(ftp){
        //ftp->quit();

       ftp->setFtpPath(_updateInf,title);
      // QString md5 = ftp->getMD5();
      // qDebug()<<"pathmd5=="<<md5;
        ftp->run();
    }
    emit updateInfChanged();
}
float VideoManager::getProcess(){
    qDebug()<<"jinlaile";
    if(ftp){
        qDebug()<<"jindu"<<ftp->getPorcess();
        return ftp->getPorcess();
    }else{
        return 0;
    }
}
