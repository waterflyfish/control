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

#ifndef TX1_MANAGER_H
#define TX1_MANAGER_H

// Below define for tx1 and qgc test
#ifndef QGC_TX1_TEST_UDP
#define QGC_TX1_TEST_UDP
#endif


#include <QObject>
#include <QTimer>

#include "QGCMAVLink.h"
#include "MAVLinkProtocol.h"

class Vehicle;

class Tx1Manager : public QObject
{
    Q_OBJECT

public:
    Tx1Manager(Vehicle* parent = NULL);
    ~Tx1Manager();

    /// Message to control the streaming flow of a pipeline
    ///     @param target_id : the id of the target pipe channel
    void tx1ControlPipeFlow(uint8_t target_id);

    /// Message to control the recoding state of a pipeline
    ///     @param target_id : the id of the target pipe channel
    ///     @param send    : true  : request send Feature point,
    ///                      false : request stop sending on the pipeline
    void tx1ControlPipeFp(uint8_t target_id, uint8_t send);

    /// Message to control the screenshot function of a pipeline
    ///     @param target_id : the id of the target pipe channel
    ///     @param screenee  : take screene or stop screene series (if that is implemented)
    ///     @param fileName  : optional filename for the video recording
    void tx1ControlPipeScreenee(uint8_t target_id, uint8_t screenee, char *fileName);

    /// Message to control the tracking system of a pieline
    ///     @param target_id : tThe id of the target pipeline
    ///     @param track     : true  : start tracking,
    ///                        false : stop tracking
    ///     @param topLeftX  : top left corner x coordinate of target to track
    ///     @param topLeftY  : top left corner y coordinate of target to track
    ///     @param bottomRightX  : bottom right corner x coordinate of target to track
    ///     @param bottomRightY  : bottom right corner y coordinate of target to track
    void tx1ControlPipeTrack(uint8_t target_id, uint8_t track,int topLeftX, int topLeftY, int bottomRightX, int bottomRightY);

    /// To control the activation of a com channel
    ///     @param target_id : the id of the target com channel
    void tx1ControlComFlow(uint8_t target_id);


    /// Message to setup or change a com channel
    ///     @param target_id : the id of the target com channele
    void tx1ControlSetupCom(uint8_t target_id);

    /// Message to setup or change a pipeline
    ///     @param target_id : the id of the target pipeline
    void tx1ControlSetupPipe(uint8_t target_id);

    /// Request the status of a target (com channel or pipeline)
    ///     @param target_type : type of the target for that the status message is requested
    ///     @param target_id   : the id of the target pipeline or com channel
    void tx1RequestStatus(uint8_t target_type, uint8_t target_id);

    /// A message acknowledging a received message
    ///     @param status : enum TX1_STATUS_ACK : ACK status of the command this message is acknowledgeing
    void tx1StatusAck(uint8_t status);

    ///Message to stop tracking
    ///     @param  target_id : the id of the target pipeline
    void tx1StopTracking(uint8_t target_id);

    //// !!! dummy messages - not used right now. just fror blocking the ids !!!
    void tx1Dummy1();
    void tx1Dummy2();
    void tx1Dummy3();
    void tx1Dummy4();
    void tx1Dummy5();
    //// !!! dummy messages - not used right now. just fror blocking the ids !!!

signals:
    /// A message acknowledging fllow target
    ///     @param newTarget : enum TX1_STATUS_ACK : ACK status of the command this message is acknowledgeing
    void tx1StatusTrackedTargetChanged( float tl_x, float tl_y, float rb_x, float rb_y,float c_x,float c_y);
    void tx1StatusFeaturePointChanged(float x,float y,int z);
    void tx1StatusTrackedPossibleTargetChanged(float tl_x, float tl_y, float rb_x, float rb_y);
    /// Type of the pipeline or com changed
    void tx1PipeComTypeChanged(uint8_t type);

    /// Running status of the pipeline or com changed
    void tx1PipeComRuningChanged(uint8_t running);

    /// Recording status of the pipeline or com changed
    void tx1PipeComRecordingChanged(uint8_t recording);

    /// Tracking status of the pinpeline or com changed
    void tx1PipeComTrackingChanged(uint8_t tracking);

private slots:
    void _handleTx1Message( const mavlink_message_t& message);
#ifdef QGC_TX1_TEST_UDP
    void _handleTx1Message(LinkInterface *link, const mavlink_message_t message);
    void _testTargetTracked();
#endif

private:

    void _handleTx1StatusTrackedTarget(const mavlink_message_t& message);
    void _handleTx1StatusPossibleTrackedTarget(const mavlink_message_t& message);
    void _handleTx1StatusSystem(const mavlink_message_t& message);
    void _handleTx1StatusFpTracked(const mavlink_message_t& message);

#ifdef QGC_TX1_TEST_UDP
    void _sendMessageOnLink(const mavlink_message_t& message);
#endif

private:
    Vehicle*         _vehicle;
    LinkInterface*   _dedicatedLink;
    MAVLinkProtocol* _mavlink;
    QTimer           _timer;
    uint8_t          _type;      // Type of the pipeline or com
    uint8_t          _runing;    // Running status of the pipeline or com
    uint8_t          _recording; // Recording status of the pipeline or com
    uint8_t          _tracking;  // Tracking status of the pinpeline or com

#ifdef QGC_TX1_TEST_UDP
    int              _id;       // Test target id
    LinkInterface   *_link;
#endif
};
#endif    //TX1_MANAGER_H
