/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef TX1_CONTROLLER_H
#define TX1_CONTROLLER_H

#include <QObject>

#include "MultiVehicleManager.h"
#include "MAVLinkProtocol.h"



class Tx1Controller : public QObject
{
    Q_OBJECT

public:
    /// <<  operation :
    typedef enum {
        Tx1None,       /// 0 : none;
        Tx1ComFlow,    /// 1 : control com flow;
        Tx1PipeFlow,   /// 2 : control pipe flow;
        Tx1PipeFp,      /// 3 : control pipe Sending Fp;
        Tx1PipeScreen, /// 4 : control pipe screenee;
        Tx1PipeTrack,  /// 5 : control pipe tracked;
        Tx1SetUpCom,   /// 6 : control setup com;
        Tx1SetUpPipe,   /// 7 : control setup pipe;
        Tx1TrackStop    ///8:  stop Tracking
    } Tx1Operation_t;

    Q_ENUMS(Tx1Operation_t)

    Q_PROPERTY(bool    tx1TrackRunning    READ tx1TrackRunning    WRITE setTx1TrackRunning   NOTIFY tx1TrackRunningChanged)
    Q_PROPERTY(bool    tx1FpSending       READ tx1FpSending       WRITE setTx1FpSending      NOTIFY tx1FpSendingChanged)

    Tx1Controller(QObject* parent = NULL);
    ~Tx1Controller();

    /// Control Tx1 Message
    Q_INVOKABLE void controlTx1Manager(Tx1Controller::Tx1Operation_t operation, int done, int topLeftX = 0, int topLeftY = 0, int bottomRightX = 0, int bottomRightY = 0);

    bool tx1TrackRunning() { return _tx1TrackRunning;}
    bool tx1FpSending() { return _tx1FpSending;}

    void setTx1TrackRunning(bool tx1TrackRunning) {
        if(tx1TrackRunning != _tx1TrackRunning) {
            _tx1TrackRunning = tx1TrackRunning;
            emit tx1TrackRunningChanged();
        }
    }

    void setTx1FpSending(bool tx1FpSending) {
        if(tx1FpSending != _tx1FpSending) {
            _tx1FpSending = tx1FpSending;
            emit tx1FpSendingChanged();
        }
    }

signals:
    void tx1StatusTrackedTargetChanged(float tl_x, float tl_y, float rb_x, float rb_y,float c_x,float c_y);
    void tx1StatusTrackedPossibleTargetChanged(float tl_x, float tl_y, float rb_x, float rb_y);
    void tx1StatusFeaturePointChanged(float x,float y,int z);
    void tx1FpSendingChanged();
    void tx1TrackRunningChanged();

private slots:
    void tx1PipeComTracking(uint8_t tracking);
    void tx1PipeComRecording(uint8_t recording);

private:
    void _start();
    void _activeVehicleChanged(Vehicle* activeVehicle);
    void _activeVehicleBeingRemoved(void);
    void _activeVehicleSet(void);
    void _removeAll();

    void _errorMessage(QString errMsg);

private:
    MultiVehicleManager* _multiVehicleMgr;
    Vehicle*             _activeVehicle;
    bool                 _tx1TrackRunning;
    bool                 _tx1FpSending;
};
#endif    //TX1_CONTROLLER_H
