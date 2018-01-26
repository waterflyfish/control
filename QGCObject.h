#ifndef QGCOBJECT_H
#define QGCOBJECT_H

#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include "Vehicle.h"
#include "QGCApplication.h"
#include "QGroundControlQmlGlobal.h"
class QGCObject : public QObject
{
    Q_OBJECT

public:
    QGCObject(QObject *parent = NULL)
        :_vehicle(NULL)
    {

    }
    ~QGCObject(){}
     bool eventFilter (QObject *target, QEvent *event)
     {
         if(event->type () == QEvent::KeyPress){
             QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
             double roll = 0.0,pitch = 0.0,yaw = 0.0,thrust = 0.5;
             bool posevent;
             _vehicle = qgcApp ()->toolbox ()->multiVehicleManager ()->activeVehicle ();
             if((keyEvent->modifiers () ==(Qt::ShiftModifier |  Qt::ControlModifier))){
                 if(keyEvent->key () == Qt::Key_W){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     pitch = -1.0;
                 }else if(keyEvent->key () == Qt::Key_S){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     pitch = 1.0;
                 }else if(keyEvent->key () == Qt::Key_A){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     yaw = -1.0;
                 }else if(keyEvent->key () == Qt::Key_D){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     yaw = 1.0;
                 }else if(keyEvent->key () == Qt::Key_Up){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     thrust = 1.0;
                 }else if(keyEvent->key () == Qt::Key_Down ){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     thrust = -1.0;
                 }else if(keyEvent->key () == Qt::Key_Left){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     roll = -1.0;
                 }else if(keyEvent->key () == Qt::Key_Right ){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     roll = 1.0;
                 }
                 else{
                     posevent = false;
                 }
             }else if(keyEvent->modifiers () == Qt::ControlModifier){
                 if(keyEvent->key () == Qt::Key_W){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     pitch = -0.5;
                 }else if(keyEvent->key () == Qt::Key_S){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     pitch = 0.5;
                 }else if(keyEvent->key () == Qt::Key_A){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     yaw = -0.5;
                 }else if(keyEvent->key () == Qt::Key_D){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     yaw = 0.5;
                 }else if(keyEvent->key () == Qt::Key_Up){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     thrust = 0.75;
                 }else if(keyEvent->key () == Qt::Key_Down ){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     thrust = 0.25;
                 }else if(keyEvent->key () == Qt::Key_Left){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     roll = -0.5;
                 }else if(keyEvent->key () == Qt::Key_Right ){
                     posevent = true;
                     if(_vehicle){
                         _vehicle->setKeyCtlAtitude (true);
                     }else{
                         posevent = false;
                     }
                     roll = 0.5;
                 }
                 else{
                     posevent = false;
                 }
             }else{
                 posevent = false;
             }

             if(_vehicle && posevent && _vehicle->active () && !(_vehicle->joystickEnabled ())&& _vehicle->isPosCtlMode ()){
                 _vehicle->virtualTabletJoystickValue (roll, pitch, yaw, thrust);
             }
             return posevent;
         }else if(event->type () == QEvent::KeyRelease){
             QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
             if((keyEvent->key () == Qt::Key_W || keyEvent->key () == Qt::Key_S ||
                 keyEvent->key () == Qt::Key_A || keyEvent->key () == Qt::Key_D ||
                 keyEvent->key () == Qt::Key_Up || keyEvent->key () == Qt::Key_Down ||
                 keyEvent->key () == Qt::Key_Left || keyEvent->key () == Qt::Key_Right)&&!keyEvent->isAutoRepeat ()){
                 if(_vehicle){
                     if(_vehicle && _vehicle->active () && !(_vehicle->joystickEnabled ())&& _vehicle->isPosCtlMode ()){
                         _vehicle->virtualTabletJoystickValue (0, 0, 0, 0.5);
                     }
                     _vehicle->setKeyCtlAtitude (false);
                 }
                 return true;
             }
         }
         return QObject::eventFilter (target,event);
     }
private:
     Vehicle * _vehicle;

};
#endif // QGCOBJECT

