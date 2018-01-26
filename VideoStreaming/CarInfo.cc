#include "CarInfo.h"
#include <synchapi.h>
void CarInfo::run(){
	if(_rece){
        if(_flag){
                _rece->startroirecording();
                Sleep(1000);
                _rece->stopimagerecording();
        }else{
                _rece->saveImg();
        }

	}
}
void CarInfo::setVideoRece(VideoReceiver* rece){
    this->_rece = rece ;
}
void CarInfo::setSaveImg(bool flag){
    this->_flag=flag;
}
