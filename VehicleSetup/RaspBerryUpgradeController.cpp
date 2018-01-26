
/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Console Upgrade UI
///     @author Xiang hong <Xiang hong@Xiang hong.com>

#include "RaspBerryUpgradeController.h"
#include "QGCApplication.h"
#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "QGCApplication.h"
#include "FirmwareUpgradeController.h"

#include <QFile>
#include <QDir>
#include <string>

RaspBerryUpgradeController::RaspBerryUpgradeController(QObject* parent)
  :QObject(parent)
  , _currentOperation(kCOIdle)
  ,_ackTimes(0)
  ,_link(NULL)
  , _lastOutgoingSeqNumber(0)
  , _activeSession(0)
  ,_systemIdQGC(255)// _systemIdQGC(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId())
  , _resetStatus(false)
  , _rebootStatus(false)
  ,_selectedComponent(-2)
  ,_selectedComponentReboot(-2)
  ,_rebootGpsTimes(0)
{
  connect(&_ackTimer, &QTimer::timeout, this, &RaspBerryUpgradeController::_ackTimeout);
  connect(&_rebootGpsTimer, &QTimer::timeout, this, &RaspBerryUpgradeController::_sendBootReadyCommand);
  _systemIdServer = 0; //consoleLink->id();

  // Make sure we don't have bad structure packing
  Q_ASSERT(sizeof(RequestHeader) == 12);
}

/// Respond to the Ack associated with the Open command with the next read command.
void RaspBerryUpgradeController::_openAckResponse(Request* openAck)
{
    Q_ASSERT(_currentOperation == kCOOpenRead || _currentOperation == kCOOpenBurst);
    _currentOperation = _currentOperation == kCOOpenRead ? kCORead : kCOBurst;
    _activeSession = openAck->hdr.session;

    // File length comes back in data
    if(openAck->hdr.size != sizeof(uint32_t)){
        _emitErrorMessage(tr("Receive size not equal loacal size"));
        return ;
    }
    _downloadFileSize = openAck->openFileLength;

    // Start the sequence of read commands

    _downloadOffset = 0;            // Start reading at beginning of file
    _readFileAccumulator.clear();   // Start with an empty file

    Request request;
    request.hdr.session = _activeSession;
    Q_ASSERT(_currentOperation == kCORead || _currentOperation == kCOBurst);
    request.hdr.opcode = _currentOperation == kCORead ? kCmdReadFile : kCmdBurstReadFile;
    request.hdr.offset = _downloadOffset;
    request.hdr.size = sizeof(request.data);

    _sendRequest(&request);
}

/// Closes out a download session by writing the file and doing cleanup.
///     @param success true: successful download completion, false: error during download
void RaspBerryUpgradeController::_closeDownloadSession(bool success)
{

    _currentOperation = kCOIdle;

    if (success) {
        QString downloadFilePath = _readFileDownloadDir.absoluteFilePath(_readFileDownloadFilename);

        QFile file(downloadFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            _emitErrorMessage(tr("Unable to open local file for writing (%1)").arg(downloadFilePath));
            return;
        }

        qint64 bytesWritten = file.write((const char *)_readFileAccumulator, _readFileAccumulator.length());
        if (bytesWritten != _readFileAccumulator.length()) {
            file.close();
            _emitErrorMessage(tr("Unable to write data to local file (%1)").arg(downloadFilePath));
            return;
        }
        file.close();

        emit commandComplete();
    }

    _readFileAccumulator.clear();

    // Close the open session
    _sendResetCommand();
}

/// Closes out an upload session doing cleanup.
///     @param success true: successful upload completion, false: error during download
void RaspBerryUpgradeController::_closeUploadSession(bool success)
{
    if (!success){
        _rebootStatus = false;
        emit appendStatusLog("Upload Firmware failed");
    }

    _currentOperation = kCOIdle;
    _writeFileAccumulator.clear();
    _writeFileSize = 0;

    if (success) {
        _rebootStatus = true;
        emit appendStatusLog("Upload Firmware complete");
        emit commandComplete();
    }

    // Close the open session
    _sendResetCommand();
}

/// Respond to the Ack associated with the Read or Stream commands.
///		@param readFile: true: read file, false: stream file
void RaspBerryUpgradeController::_downloadAckResponse(Request* readAck, bool readFile)
{
    if (readAck->hdr.session != _activeSession) {
        _closeDownloadSession(false /* failure */);
        _emitErrorMessage(tr("Download: Incorrect session returned"));
        return;
    }

    if (readAck->hdr.offset != _downloadOffset) {
        _closeDownloadSession(false /* failure */);
        _emitErrorMessage(tr("Download: Offset returned (%1) differs from offset requested/expected (%2)").arg(readAck->hdr.offset).arg(_downloadOffset));
        return;
    }

    _downloadOffset += readAck->hdr.size;
    _readFileAccumulator.append((const char*)readAck->data, readAck->hdr.size);

    if (_downloadFileSize != 0) {
        emit commandProgress(100 * ((float)_readFileAccumulator.length() / (float)_downloadFileSize));
    }

    if (readFile || readAck->hdr.burstComplete) {
        // Possibly still more data to read, send next read request

        Request request;
        request.hdr.session = _activeSession;
        request.hdr.opcode = readFile ? kCmdReadFile : kCmdBurstReadFile;
        request.hdr.offset = _downloadOffset;
        request.hdr.size = 0;

        _sendRequest(&request);
    } else if (!readFile) {
        // Streaming, so next ack should come automatically
        _setupAckTimeout();
    }
}

/// @brief Respond to the Ack associated with the List command.
void RaspBerryUpgradeController::_listAckResponse(Request* listAck)
{
    if (listAck->hdr.offset != _listOffset) {
        _currentOperation = kCOIdle;
        _emitErrorMessage(tr("List: Offset returned (%1) differs from offset requested (%2)").arg(listAck->hdr.offset).arg(_listOffset));
        return;
    }

    uint8_t offset = 0;
    uint8_t cListEntries = 0;
    uint8_t cBytes = listAck->hdr.size;

    // parse filenames out of the buffer
    while (offset < cBytes) {
        const char * ptr = ((const char *)listAck->data) + offset;

        // get the length of the name
        uint8_t cBytesLeft = cBytes - offset;
        uint8_t nlen = static_cast<uint8_t>(strnlen(ptr, cBytesLeft));
        if ((*ptr == 'S' && nlen > 1) || (*ptr != 'S' && nlen < 2)) {
            _currentOperation = kCOIdle;
            _emitErrorMessage(tr("Incorrectly formed list entry: '%1'").arg(ptr));
            return;
        } else if (nlen == cBytesLeft) {
            _currentOperation = kCOIdle;
            _emitErrorMessage(tr("Missing NULL termination in list entry"));
            return;
        }

        // Returned names are prepended with D for directory, F for file, S for skip
        if (*ptr == 'F' || *ptr == 'D') {
            // put it in the view
            _emitListEntry(ptr);
        } else if (*ptr == 'S') {
            // do nothing
        } else {
            qDebug() << "unknown entry" << ptr;
        }

        // account for the name + NUL
        offset += nlen + 1;

        cListEntries++;
    }

    if (listAck->hdr.size == 0 || cListEntries == 0) {
        // Directory is empty, we're done
        Q_ASSERT(listAck->hdr.opcode == kRspAck);
        _currentOperation = kCOIdle;
        emit commandComplete();
    } else {
        // Possibly more entries to come, need to keep trying till we get EOF
        _currentOperation = kCOList;
        _listOffset += cListEntries;
        _sendListCommand();
    }
}

/// @brief Respond to the Ack associated with the create command.
void RaspBerryUpgradeController::_createAckResponse(Request* createAck)
{
    _currentOperation = kCOWrite;
    _activeSession = createAck->hdr.session;

    // Start the sequence of write commands from the beginning of the file

    _writeOffset = 0;
    _writeSize = 0;

    _writeFileDatablock();
}

/// @brief Respond to the Ack associated with the write command.
void RaspBerryUpgradeController::_writeAckResponse(Request* writeAck)
{
    qDebug() << "_writeAckResponse : " << float(_writeOffset+_writeSize)/float(_writeFileSize);
    if (_writeFileSize != 0) {
        /// this is error , next will changed this for progress
        emit uploadProgram((qint64)(_writeOffset+_writeSize), (qint64)_writeFileSize);
    }

    if (writeAck->hdr.session != _activeSession) {
        _closeUploadSession(false /* failure */);
        _emitErrorMessage(tr("Write: Incorrect session returned"));
        return;
    }

    if (writeAck->hdr.offset != _writeOffset) {
        _closeUploadSession(false /* failure */);
        _emitErrorMessage(tr("Write: Offset returned (%1) differs from offset requested (%2)").arg(writeAck->hdr.offset).arg(_writeOffset));
        return;
    }

    if (writeAck->hdr.size != sizeof(uint32_t)) {
        _closeUploadSession(false /* failure */);
        _emitErrorMessage(tr("Write: Returned invalid size of write size data"));
        return;
    }


    if( writeAck->writeFileLength !=_writeSize) {
        _closeUploadSession(false /* failure */);
        _emitErrorMessage(tr("Write: Size returned (%1) differs from size requested (%2)").arg(writeAck->writeFileLength).arg(_writeSize));
        return;
    }

    _writeFileDatablock();
}

/// @brief Send next write file data block.
void RaspBerryUpgradeController::_writeFileDatablock(void)
{
    if (_writeOffset + _writeSize >= _writeFileSize){
        _closeUploadSession(true /* success */);
        return;
    }

    _writeOffset += _writeSize;

    Request request;
    request.hdr.session = _activeSession;
    request.hdr.opcode = kCmdWriteFile;
    request.hdr.offset = _writeOffset;

    if(_writeFileSize -_writeOffset > sizeof(request.data) )
        _writeSize = sizeof(request.data);
    else
        _writeSize = _writeFileSize - _writeOffset;

    request.hdr.size = _writeSize;

    memcpy(request.data, &_writeFileAccumulator.data()[_writeOffset], _writeSize);

    _sendRequest(&request);
}

void RaspBerryUpgradeController::_receiveBytes(LinkInterface* link, mavlink_message_t message){
    if (message.sysid != 250 && message.sysid != 0) {
        return;
    }

    if (message.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) {
        return ;
    }

    if(!_ackTimer.isActive ()){
        return ;
    }
    mavlink_file_transfer_protocol_t data;
    mavlink_msg_file_transfer_protocol_decode(&message, &data);
    // Make sure we are the target system
    if (data.target_system != 255) {
        qWarning() << "Received MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL with incorrect target_system:" <<  data.target_system << "expected:" << _systemIdQGC;
        return;
    }
    _link = link;
    _receiveMessage(message);
}

void RaspBerryUpgradeController::_receiveMessage(mavlink_message_t message)
{
    // receiveMessage is signalled will all mavlink messages so we need to filter everything else out but ours.
    if (message.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) {
       return;
    }

    mavlink_file_transfer_protocol_t data;
    mavlink_msg_file_transfer_protocol_decode(&message, &data);
    // Make sure we are the target system
    if (data.target_system != 255) {
        qDebug() << "Received MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL with incorrect target_system:" <<  data.target_system << "expected:" << _systemIdQGC;
        return;
    }

    Request* request = (Request*)&data.payload[0];

    _clearAckTimeout();
    qDebug()<< " opcode : "<< request->hdr.opcode << " , req_opcode : "<< request->hdr.req_opcode
            << " , seqNumber : "<< request->hdr.seqNumber << " , session : "<< request->hdr.session
            <<" ,data : "<<(uint8_t)request->data[0];
    uint16_t incomingSeqNumber = request->hdr.seqNumber;
    // Make sure we have a good sequence number
    uint16_t expectedSeqNumber = _lastOutgoingSeqNumber + 1;
    if (incomingSeqNumber != expectedSeqNumber) {
        switch (_currentOperation) {
        case kCOBurst:
        case kCORead:
            _closeDownloadSession(false /* failure */);
            break;

        case kCOWrite:
            _closeUploadSession(false /* failure */);
            break;

        case kCOOpenRead:
        case kCOOpenBurst:
        case kCOCreate:
            // We could have an open session hanging around
            _currentOperation = kCOIdle;
            _sendResetCommand();
            break;

        default:
            // Don't need to do anything special
            _currentOperation = kCOIdle;
            break;
        }

        _emitErrorMessage(tr("Bad sequence number on received message: expected(%1) received(%2)").arg(expectedSeqNumber).arg(incomingSeqNumber));
        return;
    }

    // Move past the incoming sequence number for next request
    _lastOutgoingSeqNumber = incomingSeqNumber;

    if (request->hdr.opcode == kRspAck) {
        switch (request->hdr.req_opcode) {
        case kCmdListDirectory:
            _listAckResponse(request);
            break;

        case kCmdOpenFileRO:
        case kCmdOpenFileWO:
            _openAckResponse(request);
            break;

        case kCmdReadFile:
            _downloadAckResponse(request, true /* read file */);
            break;

        case kCmdBurstReadFile:
            _downloadAckResponse(request, false /* stream file */);
            break;

        case kCmdCreateFile:
            _createAckResponse(request);
            break;

        case kCmdWriteFile:
            _writeAckResponse(request);
            break;

        case kCmdSearchVersion:
            _currentOperation = kCOIdle;
            if(request->hdr.size == 0){
                request->data[0] = '\0';
            }
            emit getVersion(QString(QLatin1String((char*)request->data)), true);
            break;

        case kCmdReboot:
            _currentOperation = kCOIdle;
            if( _selectedComponentReboot == FirmwareUpgradeController::GPS && _rebootGpsTimes - 1 == 0){
                // GPS need two reboot
                qDebug() << __FILE__  << " : " << __LINE__ << " reboot again!";
                _rebootGpsTimes += 1;
                _rebootGpsTimer.setSingleShot(true);
                _rebootGpsTimer.start(5000);
                //emit appendStatusLog("GPS Ready to boot first, wait 5s for GPS Upgrade", true);
            }else{
                QString bootString = "Please restart the SmartControl";
                _rebootGpsTimes = 0;
                emit appendStatusLog(bootString, true);
                emit nextFirmwareUpgrade(true);
            }
            break;

        case kCmdResetSessions:
            _resetStatus = false;
            if(_currentOperation == kCOCreate) {
                qDebug() <<"Current time : " << QTime::currentTime().toString()<< "Reset command excuted, now send Create command again";
                _rebootGpsTimes = 0;
                Request requestCreate;
                requestCreate.hdr.session = 0;
                requestCreate.hdr.opcode = kCmdCreateFile;
                requestCreate.hdr.offset = 0;
                requestCreate.hdr.size = 0;
                _fillRequestWithString(&requestCreate, _toPath + "/" + _uploadFile.fileName());
                _sendRequest(&requestCreate);
            } else if (_rebootStatus ) {
                qDebug()<< __FILE__ << " : " <<__LINE__<<" : Current time : " << QTime::currentTime().toString() <<" : start restart the machine";
                _rebootGpsTimes += 1;
                _sendBootReadyCommand();
                _rebootStatus = false;
            } else {
                _currentOperation = kCOIdle;
            }
            break;

        default:
            // Ack back from operation which does not require additional work
            _currentOperation = kCOIdle;
            break;
        }
    } else if (request->hdr.opcode == kRspNak) {
        qDebug()<< " opcode : "<< request->hdr.opcode << " , req_opcode : "<< request->hdr.req_opcode
                << " , seqNumber : "<< request->hdr.seqNumber << " , session : "<< request->hdr.session;

        uint8_t errorCode = request->data[0];

        // Nak's normally have 1 byte of data for error code, except for kErrFailErrno which has additional byte for errno
        Q_ASSERT((errorCode == kErrFailErrno && request->hdr.size == 2) || request->hdr.size == 1);

        _currentOperation = kCOIdle;

        if (request->hdr.req_opcode == kCmdListDirectory && errorCode == kErrEOF) {
            // This is not an error, just the end of the list loop
            emit commandComplete();
            return;
        } else if ((request->hdr.req_opcode == kCmdReadFile || request->hdr.req_opcode == kCmdBurstReadFile) && errorCode == kErrEOF) {
            // This is not an error, just the end of the download loop
            _closeDownloadSession(true /* success */);
            return;
        } else if (request->hdr.req_opcode == kCmdCreateFile) {
            _currentOperation = kCOCreate;
            _sendResetCommand();
            _emitErrorMessage(tr("Nak received creating file, error: %1").arg(errorString(request->data[0])));
            return;
        } else {
            // Generic Nak handling
            if (request->hdr.req_opcode == kCmdReadFile || request->hdr.req_opcode == kCmdBurstReadFile) {
                // Nak error during download loop, download failed
                _closeDownloadSession(false /* failure */);
            } else if (request->hdr.req_opcode == kCmdWriteFile) {
                // Nak error during upload loop, upload failed
                _closeUploadSession(false /* failure */);
            }
            if(kErrUnknownCommand == request->data[0]){
                _sendResetCommand();
            }
            _emitErrorMessage(tr("Nak received, error: %1").arg(errorString(request->data[0])));
        }
    } else {
        // Note that we don't change our operation state. If something goes wrong beyond this, the operation
        // will time out.
        _emitErrorMessage(tr("Unknown opcode returned from server: %1").arg(request->hdr.opcode));
    }
}

void RaspBerryUpgradeController::listDirectory(const QString& dirPath)
{
    if (_currentOperation != kCOIdle) {
        _emitErrorMessage(tr("Command not sent. Waiting for previous command to complete."));
        return;
    }

    // initialise the lister
    _listPath = dirPath;
    _listOffset = 0;
    _currentOperation = kCOList;

    // and send the initial request
    _sendListCommand();
}

void RaspBerryUpgradeController::_fillRequestWithString(Request* request, const QString& str)
{
    strncpy((char *)&request->data[0], str.toStdString().c_str(), sizeof(request->data));
    request->hdr.size = static_cast<uint8_t>(strnlen((const char *)&request->data[0], sizeof(request->data)));
}

void RaspBerryUpgradeController::_sendListCommand(void)
{
    Request request;

    request.hdr.session = 0;
    request.hdr.opcode = kCmdListDirectory;
    request.hdr.offset = _listOffset;
    request.hdr.size = 0;

    _fillRequestWithString(&request, _listPath);
    _sendRequest(&request);
}

void RaspBerryUpgradeController::downloadPath(const QString& from, const QDir& downloadDir)
{
    if (_currentOperation != kCOIdle) {
        _emitErrorMessage(tr("Command not sent. Waiting for previous command to complete."));
        return;
    }

    _downloadWorker(from, downloadDir, true /* read file */);
}

void RaspBerryUpgradeController::streamPath(const QString& from, const QDir& downloadDir)
{
    if (_currentOperation != kCOIdle) {
        _emitErrorMessage(tr("Command not sent. Waiting for previous command to complete."));
        return;
    }

    _downloadWorker(from, downloadDir, false /* stream file */);
}

void RaspBerryUpgradeController::_downloadWorker(const QString& from, const QDir& downloadDir, bool readFile)
{
    if (from.isEmpty()) {
        return;
    }

    _readFileDownloadDir.setPath(downloadDir.absolutePath());

    // We need to strip off the file name from the fully qualified path. We can't use the usual QDir
    // routines because this path does not exist locally.
    int i;
    for (i=from.size()-1; i>=0; i--) {
        if (from[i] == '/') {
            break;
        }
    }
    i++; // move past slash
    _readFileDownloadFilename = from.right(from.size() - i);

    _currentOperation = readFile ? kCOOpenRead : kCOOpenBurst;

    Request request;
    request.hdr.session = 0;
    request.hdr.opcode = kCmdOpenFileRO;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    _fillRequestWithString(&request, from);
    _sendRequest(&request);
}

/// @brief Uploads the specified file.
///     @param toPath File in UAS to upload to, fully qualified path
///     @param uploadFile Local file to upload from
void RaspBerryUpgradeController::uploadPath(LinkInterface *link, const QString& toPath, const QFileInfo& uploadFile, int selectedFirmware){
    Q_UNUSED(link);
    _selectedComponent = -1;
    _selectedComponentReboot = selectedFirmware;
    uploadPath(toPath, uploadFile);
}

/// @brief Uploads the specified file.
///     @param toPath File in UAS to upload to, fully qualified path
///     @param uploadFile Local file to upload from
void RaspBerryUpgradeController::uploadPath(const QString& toPath, const QFileInfo& uploadFile)
{
    qDebug()<<"Current time : "<< QTime::currentTime().toString()
           << " : from : " << uploadFile.filePath()
           <<" : "<< uploadFile.fileName() << " to : " << toPath;
    _toPath = toPath;
    _uploadFile = uploadFile;
    if(_currentOperation != kCOIdle){
        _emitErrorMessage(tr("UAS File manager busy.  Try again later"));
        return;
    }

    if (toPath.isEmpty()) {
        return;
    }

    if (!uploadFile.isReadable()){
        _emitErrorMessage(tr("File (%1) is not readable for upload").arg(uploadFile.path()));
        return;
    }

    QFile file(uploadFile.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        _emitErrorMessage(tr("Unable to open local file for upload (%1)").arg(uploadFile.absoluteFilePath()));
        return;
    }

    _writeFileAccumulator = file.readAll();
    _writeFileSize = _writeFileAccumulator.size();

    file.close();

    if (_writeFileAccumulator.size() == 0) {
        _emitErrorMessage(tr("Unable to read data from local file (%1)").arg(uploadFile.absoluteFilePath()));
        return;
    }

    _currentOperation = kCOCreate;

    Request request;
    request.hdr.session = 0;
    request.hdr.opcode = kCmdCreateFile;
    //request.hdr.padding = _writeFileAccumulator.size()/1024;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    _fillRequestWithString(&request, toPath + "/" + uploadFile.fileName());
    _sendRequest(&request);
}

QString RaspBerryUpgradeController::errorString(uint8_t errorCode)
{
    switch(errorCode) {
    case kErrNone:
        return QString("no error");
    case kErrFail:
        return QString("unknown error");
    case kErrEOF:
        return QString("read beyond end of file");
    case kErrUnknownCommand:
        return QString("unknown command");
    case kErrFailErrno:
        return QString("command failed");
    case kErrInvalidDataSize:
        return QString("invalid data size");
    case kErrInvalidSession:
        return QString("invalid session");
    case kErrNoSessionsAvailable:
        return QString("no sessions available");
    case kErrFailFileExists:
        return QString("File already exists on target");
    case kErrFailFileProtected:
        return QString("File is write protected");
    default:
        return QString("unknown error code");
    }
}

/// @brief Sends a command which only requires an opcode and no additional data
///     @param opcode Opcode to send
///     @param newOpState State to put state machine into
/// @return TRUE: command sent, FALSE: command not sent, waiting for previous command to finish
bool RaspBerryUpgradeController::_sendOpcodeOnlyCmd(uint8_t opcode, OperationState newOpState)
{
    if (_currentOperation != kCOIdle) {
        // Can't have multiple commands in play at the same time
        return false;
    }

    Request request;
    request.hdr.session = 0;
    request.hdr.opcode = opcode;
    request.hdr.offset = 0;
    request.hdr.size = 0;

    _currentOperation = newOpState;

    _sendRequest(&request);

    return true;
}

/// @brief Starts the ack timeout timer
bool RaspBerryUpgradeController::_setupAckTimeout(void)
{
    if(_ackTimer.isActive () || _ackTimes >= 8){
        _emitErrorMessage(QString("Setup AckTimeout busy : Command failed (%1)").arg(_currentOperation));
        _ackTimes = 0;
        return false;
    }else{
        _ackTimer.setSingleShot(true);
        _ackTimer.start(ackTimerTimeoutMsecs);
        return true;
    }
}

/// @brief Clears the ack timeout timer
void RaspBerryUpgradeController::_clearAckTimeout(void)
{
    _ackTimes = 0;
    _ackTimer.stop();
}

/// @brief Stop the time when delete the fileManger, must be init the parm for upgrade
void RaspBerryUpgradeController::stopAckTimeout()
{
    if(_ackTimer.isActive())
    {
        _ackTimer.stop ();
    }
    _lastOutgoingSeqNumber = 0;
    _currentOperation = kCOIdle;
    _activeSession = 0;
    _ackTimes = 0;
    _resetStatus = false;
    _rebootStatus = false;
    _selectedComponent = -2;
    _selectedComponentReboot = -2;
    _rebootGpsTimes = 0;
}

/// @brief Called when ack timeout timer fires
void RaspBerryUpgradeController::_ackTimeout(void)
{
    // Make sure to set _currentOperation state before emitting error message. Code may respond
    // to error message signal by sending another command, which will fail if state is not back
    // to idle. FileView UI works this way with the List command.

    Request requestRead, requestWrite;
    _ackTimes++;
    if(_selectedComponentReboot == -1){
         _ackTimes +=2;
    }
    qDebug() << "_currentOperation: " << _currentOperation << " : "<< _resetStatus;
    if(_resetStatus) {
        qDebug() << "RaspBerryUpgradeController _sendResetCommand timeout, now send it again";
        _sendResetCommand();
        return;
    }
    switch (_currentOperation) {
    case kCORead:
    case kCOBurst:
        // _closeDownloadSession(false /* failure */);
        // _emitErrorMessage(tr("Timeout waiting for ack: Download failed"));
        qDebug() << "FileManager::_ackTimeout sendRequest excuted";
        requestRead.hdr.session = _activeSession;
        requestRead.hdr.opcode = kCmdReadFile;
        requestRead.hdr.offset = _downloadOffset;
        requestRead.hdr.size = 0;
        _sendRequest(&requestRead);
        break;
    case kCOList:
        qDebug() << "_acktimeout,now send listcommand again";
        _sendListCommand();
        break;
    case kCOSearchVersion:
        qDebug() << "search version timeout when _sendSearchVersionCommand, now send it again";
        _currentOperation = kCOIdle;
        _sendSearchVersionCommand(_selectedComponent);
        break;
    case kCOReboot:
        _sendBootReadyCommand();
        break;

    case kCOOpenRead:
    case kCOOpenBurst:
        _currentOperation = kCOIdle;
        _emitErrorMessage(tr("Timeout waiting for ack: Download failed"));
        _sendResetCommand();
        break;

    case kCOCreate:
        //_currentOperation = kCOIdle;
        //_emitErrorMessage(tr("Timeout waiting for ack: Upload failed"));
        _sendResetCommand();
        break;

    case kCOWrite:
        //_closeUploadSession(false /* failure */);
        //_emitErrorMessage(tr("Timeout waiting for ack: Upload failed"));
        qDebug() << "FileManager::_writeFileDatablock excuted _writeoffset: " << _writeOffset;
        requestWrite.hdr.session = _activeSession;
        requestWrite.hdr.opcode = kCmdWriteFile;
        requestWrite.hdr.offset = _writeOffset;
        requestWrite.hdr.size = _writeSize;
        memcpy(requestWrite.data, &_writeFileAccumulator.data()[_writeOffset], _writeSize);
        _sendRequest(&requestWrite);
        break;

    default:
        _currentOperation = kCOIdle;
        _emitErrorMessage(QString("Timeout waiting for ack: Command failed (%1)").arg(_currentOperation));
        break;
    }
}

void RaspBerryUpgradeController::_sendResetCommand(void)
{
    qDebug() << "RaspBerryUpgradeController::_sendResetCommand excuted";
    Request request;
    request.hdr.opcode = kCmdResetSessions;
    request.hdr.size = 0;
    _resetStatus = true;
    _sendRequest(&request);
}

void RaspBerryUpgradeController::_emitErrorMessage(const QString& msg)
{
    /// print error message
    qDebug()<<__FILE__<<" : "<<__LINE__<<" : "<<msg;
    if(_selectedComponent == -1)
    {
        emit appendStatusLog("Upload Firmware failed : no sdcard or disconnect SmartConsole");
        emit nextFirmwareUpgrade ();
        _selectedComponent = -2;
    }
    else if(_selectedComponentReboot == -1)
    {
        emit getVersion ("", false);
        _selectedComponentReboot = -2;
    }
    else
    {

    }
    /// emit commandError(msg);
}

void RaspBerryUpgradeController::_emitListEntry(const QString& entry)
{
    emit listEntry(entry);
}

/// @brief Used when update firmware by wireless connection
void RaspBerryUpgradeController::_sendSearchVersionCommand(LinkInterface * link, int selectedComponent){
    Q_UNUSED(link);
    _sendSearchVersionCommand(selectedComponent);
}

/// @brief Used when update firmware by wireless connection
void RaspBerryUpgradeController::_sendSearchVersionCommand(int selectedComponent) {
    qDebug() << "RaspBerryUpgradeController _sendSearchVersionCommand_ send : " << _selectedComponent;
    if(_selectedComponent == -1)
    {
        _emitErrorMessage("Can not send search version cmd : select component failed!");
        return ;
    }
    _selectedComponentReboot = -1;
    _selectedComponent = selectedComponent;

    _currentOperation = kCOSearchVersion;
    Request request;
    request.hdr.session = 0;
    request.hdr.opcode = kCmdSearchVersion;
    request.hdr.offset = 0;
    request.hdr.size = sizeof(uint8_t);
    request.data[0] = (uint8_t)_selectedComponent;
    _sendRequest(&request);
}

void RaspBerryUpgradeController::_sendBootReadyCommand() {
    qDebug() << "_sendBootReadyCommand";

    _currentOperation = kCOReboot;
    _rebootStatus = false;
    Request request;
    request.hdr.session = 0;
    request.hdr.opcode = kCmdReboot;
    request.hdr.offset = 0;
    request.hdr.size = sizeof(uint8_t);
    request.data[0] = (uint8_t)_selectedComponentReboot;

    _sendRequest(&request);
}

/// @brief Sends the specified Request out to the UAS.
void RaspBerryUpgradeController::_sendRequest(Request* request)
{
   if(!_setupAckTimeout()){
       return ;
   }

   mavlink_message_t message;
    _lastOutgoingSeqNumber++;

    request->hdr.seqNumber = _lastOutgoingSeqNumber;
    if (_systemIdQGC == 0) {
        _systemIdQGC = 255;//qgcApp()->toolbox()->mavlinkProtocol()->getSystemId();
    }
    mavlink_msg_file_transfer_protocol_pack(_systemIdQGC,       // QGC System ID
                                            0,                  // QGC Component ID
                                            &message,           // Mavlink Message to pack into
                                            0,                  // Target network
                                            250,    // Target system
                                            0,                  // Target component
                                            (uint8_t*)request); // Payload

    _sendMessageOnLink(_link,message);
}

/// @brief Sends the specified Request out to the serial.
void RaspBerryUpgradeController::_sendMessageOnLink(LinkInterface* link, mavlink_message_t message)
{
    if(link){
        LinkManager *linkManager = qgcApp()->toolbox()->linkManager ();
        LinkInterface *tempLink = NULL;
        for (int i=0; i<linkManager->links()->count(); i++) {
            LinkInterface* links = linkManager->links()->value<LinkInterface*>(i);
            if(link == links ){
                tempLink = links;
                break;
            }
        }
        link = tempLink;
    }
    if(link && link->isConnected()){
                                                // _mavlink->getSystemId()
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        int len = mavlink_msg_to_send_buffer(buffer, &message);
        link->writeBytesSafe((const char*)buffer, len);
    }else{
        LinkManager *linkManager = qgcApp()->toolbox()->linkManager ();
        for (int i=0; i<linkManager->links()->count(); i++) {
            LinkInterface* links = linkManager->links()->value<LinkInterface*>(i);
            if(links && links->isConnected()){
                uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
                int len = mavlink_msg_to_send_buffer(buffer, &message);
                qDebug()<<__FILE__<<" : "<<__LINE__;
                links->writeBytesSafe((const char*)buffer, len);
            }
        }
    }
}


