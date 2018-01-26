/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "Tx1Controller.h"
#include "QGCApplication.h"
#include "Tx1Manager.h"

Tx1Controller::Tx1Controller(QObject* parent)
    :_multiVehicleMgr(qgcApp()->toolbox()->multiVehicleManager())
    , _activeVehicle(_multiVehicleMgr->offlineEditingVehicle())
    , _tx1TrackRunning(false)
    , _tx1FpSending(false)
{
    Q_UNUSED(parent)
    _start();
}

Tx1Controller::~Tx1Controller(){
}

/// << done
/// 0 : false , stop
/// 1 : true , start
void Tx1Controller::controlTx1Manager(Tx1Controller::Tx1Operation_t operation, int done,
                                      int topLeftX, int topLeftY, int bottomRightX,  int bottomRightY){
   // qDebug()<<"JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ=="<<topLeftX;
    if(!_activeVehicle){
        return ;
    }

    Tx1Manager* tx1Manager = _activeVehicle->tx1Manager ();
    if(!tx1Manager){
        return ;
    }

//    qDebug()<<" Tx1 opertion : "<<operation <<"  done : "<<done
//           <<" topLeftX : "<<topLeftX <<" topLeftY : "<<topLeftY
//          <<" bottomRightX : "<<bottomRightX <<" bottomRightY : "<<bottomRightY;

    switch(operation){
    case Tx1None:    /// Tx1None : none;
        qWarning()<<"Warning : Tx1 Manager can not find this operation : 0";
        break;

    case Tx1ComFlow:    /// Tx1ComFlow : control com flow;
        tx1Manager->tx1ControlComFlow (1);
        break;

    case Tx1PipeFlow:    /// Tx1PipeFlow : control pipe flow;
        tx1Manager->tx1ControlPipeFlow (1);
        break;

    case Tx1PipeFp:    /// Tx1PipeRecord : control pipe record;
        tx1Manager->tx1ControlPipeFp (1, done);
        break;

    case Tx1PipeScreen:    /// Tx1PipeScreen : control pipe screenee;
        tx1Manager->tx1ControlPipeScreenee (1, done, NULL);
        break;

    case Tx1PipeTrack:     /// Tx1PipeTrack : control pipe tracked;
        tx1Manager->tx1ControlPipeTrack (1,done,topLeftX, topLeftY, bottomRightX, bottomRightY);
        break;

    case Tx1SetUpCom:    /// Tx1SetUpCom : control setup com;
        tx1Manager->tx1ControlSetupCom (1);
        break;

    case Tx1SetUpPipe:    /// Tx1SetUpPipe : control setup pipe;
        tx1Manager->tx1ControlSetupPipe (1);
        break;
    case Tx1TrackStop:
        tx1Manager->tx1StopTracking(1);

    default:
        qWarning()<<"Warning : Tx1 Manager can not find this operation : "<<operation;
    }
}

void Tx1Controller::tx1PipeComRecording(uint8_t recording){
//    if(recording != 0){
//        setTx1RecordRunning(true);
//    }else{
//        setTx1RecordRunning(false);
//    }
}

void Tx1Controller::tx1PipeComTracking(uint8_t tracking){
    if(tracking != 0){
        setTx1TrackRunning(true);
    }else{
        setTx1TrackRunning(false);
    }
}


void Tx1Controller::_start()
{
    connect(_multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &Tx1Controller::_activeVehicleChanged);
    _activeVehicleChanged(_multiVehicleMgr->activeVehicle());
}

void Tx1Controller::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (_activeVehicle) {
        _activeVehicleBeingRemoved();
        _activeVehicle = NULL;
    }

    if (activeVehicle) {
        _activeVehicle = activeVehicle;
    } else {
        _activeVehicle = _multiVehicleMgr->offlineEditingVehicle();
    }
    _activeVehicleSet();
}

void Tx1Controller::_activeVehicleBeingRemoved(void)
{

    if(!_activeVehicle){
        return ;
    }

    Tx1Manager* tx1Manager = _activeVehicle->tx1Manager();
    if(!tx1Manager){
        return ;
    }

    disconnect(tx1Manager, &Tx1Manager::tx1StatusTrackedTargetChanged,   this, &Tx1Controller::tx1StatusTrackedTargetChanged);
    disconnect(tx1Manager, &Tx1Manager::tx1StatusFeaturePointChanged,   this, &Tx1Controller::tx1StatusFeaturePointChanged);
    disconnect(tx1Manager, &Tx1Manager::tx1PipeComTrackingChanged,       this, &Tx1Controller::tx1PipeComTracking);
    disconnect(tx1Manager, &Tx1Manager::tx1PipeComRecordingChanged,      this, &Tx1Controller::tx1PipeComRecording);
    disconnect(tx1Manager, &Tx1Manager::tx1StatusTrackedPossibleTargetChanged,   this, &Tx1Controller::tx1StatusTrackedPossibleTargetChanged);
    // We always remove all items on vehicle change. This leaves a user model hole:
    //      If the user has unsaved changes in the Plan view they will lose them
    _removeAll();
}

void Tx1Controller::_activeVehicleSet(void)
{
    _removeAll();
    if(!_activeVehicle){
        return ;
    }

    Tx1Manager* tx1Manager = _activeVehicle->tx1Manager();
    if(!tx1Manager){
        return ;
    }
    connect(tx1Manager, &Tx1Manager::tx1StatusTrackedTargetChanged,   this, &Tx1Controller::tx1StatusTrackedTargetChanged);
    connect(tx1Manager, &Tx1Manager::tx1PipeComTrackingChanged,       this, &Tx1Controller::tx1PipeComTracking);
    connect(tx1Manager, &Tx1Manager::tx1PipeComRecordingChanged,      this, &Tx1Controller::tx1PipeComRecording);
    connect(tx1Manager, &Tx1Manager::tx1StatusFeaturePointChanged,   this, &Tx1Controller::tx1StatusFeaturePointChanged);
    connect(tx1Manager, &Tx1Manager::tx1StatusTrackedPossibleTargetChanged,   this, &Tx1Controller::tx1StatusTrackedPossibleTargetChanged);
}

void Tx1Controller::_removeAll(){

}
