#ifndef _CARINFO_H_
#define _CARINFO_H_
#include "VideoReceiver.h"
#include <QThread>
class CarInfo : public QThread
 {
     Q_OBJECT
public:
    void setVideoRece(VideoReceiver* rece);
    void setSaveImg(bool flag);
 private:
     void run();
     VideoReceiver* _rece;
     bool _flag;
 };
 #endif
