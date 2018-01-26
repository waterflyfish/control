/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "FileManager.h"
#include "QGC.h"
#include "MAVLinkProtocol.h"
#ifndef __android__
#include "MainWindow.h"
#endif
#include "Vehicle.h"
#include "QGCApplication.h"
#include "FirmwareUpgradeController.h"

#include <QFile>
#include <QDir>
#include <string>

QGC_LOGGING_CATEGORY(FileManagerLog, "FileManagerLog")

FileManager::FileManager(QObject* parent, Vehicle* vehicle)
    : QObject(parent)
    , _currentOperation(kCOIdle)
    , _ackTimes(0)
    , _vehicle(vehicle)
    , _dedicatedLink(NULL)
    , _lastOutgoingSeqNumber(0)
    , _activeSession(0)
    //, _systemIdQGC(0)
    , _systemIdQGC(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId())
    , _resetStatus(false)
    , _rebootStatus(false)
    , _selectedComponent(-2)
    , _selectedComponentReboot(-2)
    , _rebootGpsTimes(0)
{
    connect(&_ackTimer, &QTimer::timeout, this, &FileManager::_ackTimeout);
    connect(&_rebootGpsTimer, &QTimer::timeout, this, &FileManager::_sendBootReadyCommand);
    
    _systemIdServer = _vehicle->id();
    
    // Make sure we don't have bad structure packing
    Q_ASSERT(sizeof(RequestHeader) == 12);
}

/// Respond to the Ack associated with the Open command with the next read command.
void FileManager::_openAckResponse(Request* openAck)
{
    qCDebug(FileManagerLog) << QString("_openAckResponse: _currentOperation(%1) _readFileLength(%2)").arg(_currentOperation).arg(openAck->openFileLength);
    
	Q_ASSERT(_currentOperation == kCOOpenRead || _currentOperation == kCOOpenBurst);
	_currentOperation = _currentOperation == kCOOpenRead ? kCORead : kCOBurst;
    _activeSession = openAck->hdr.session;
    
    // File length comes back in data
    Q_ASSERT(openAck->hdr.size == sizeof(uint32_t));
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
void FileManager::_closeDownloadSession(bool success)
{
    qCDebug(FileManagerLog) << QString("_closeDownloadSession: success(%1)").arg(success);
    
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
void FileManager::_closeUploadSession(bool success)
{
    qCDebug(FileManagerLog) << QString("_closeUploadSession: success(%1)").arg(success);
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
void FileManager::_downloadAckResponse(Request* readAck, bool readFile)
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
    
    qCDebug(FileManagerLog) << QString("_downloadAckResponse: offset(%1) size(%2) burstComplete(%3)").arg(readAck->hdr.offset).arg(readAck->hdr.size).arg(readAck->hdr.burstComplete);

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
void FileManager::_listAckResponse(Request* listAck)
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
void FileManager::_createAckResponse(Request* createAck)
{
    qCDebug(FileManagerLog) << "_createAckResponse";
    
    _currentOperation = kCOWrite;
    _activeSession = createAck->hdr.session;

    // Start the sequence of write commands from the beginning of the file

    _writeOffset = 0;
    _writeSize = 0;
    
    _writeFileDatablock();
}

/// @brief Respond to the Ack associated with the write command.
void FileManager::_writeAckResponse(Request* writeAck)
{
    if (_writeFileSize != 0) {
        emit commandProgress(100 * ((float)(_writeOffset+_writeSize) / (float)_writeFileSize));
        emit uploadProgram((qint64)(_writeOffset+_writeSize), (qint64)_writeFileSize);
    }
    if(_writeOffset + _writeSize >= _writeFileSize){
        _closeUploadSession(true /* success */);
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
void FileManager::_writeFileDatablock(void)
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

void FileManager::receiveMessage(mavlink_message_t message)
{
    // receiveMessage is signalled will all mavlink messages so we need to filter everything else out but ours.
    if (message.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) {
        return;
    }
    if(!_ackTimer.isActive()){
        return ;
    }
    mavlink_file_transfer_protocol_t data;
    mavlink_msg_file_transfer_protocol_decode(&message, &data);
	
    // Make sure we are the target system
    if (data.target_system != qgcApp()->toolbox()->mavlinkProtocol()->getSystemId()) {
        qDebug() << "Received MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL with incorrect target_system:" <<  data.target_system << "expected:" << _systemIdQGC;
        return;
    }
    
    Request* request = (Request*)&data.payload[0];
    
    _clearAckTimeout();
    
	qCDebug(FileManagerLog) << "receiveMessage" << request->hdr.opcode;
	
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
            if(request->hdr.size - 1 <= 0){
                request->data[0] = '\0';
            }
            emit getVersion(QString(QLatin1String((char*)request->data)), true);
            break;
        case kCmdReboot:
            _currentOperation = kCOIdle;
            if( _selectedComponentReboot == FirmwareUpgradeController::GPS && _rebootGpsTimes - 1 == 0){
                qDebug() << __FILE__  << " : " << __LINE__ << " reboot again!";
                _rebootGpsTimes += 1;
                _rebootGpsTimer.setSingleShot(true);
                _rebootGpsTimer.start(5000);
            }else{
                QString bootString = QString(QLatin1String((char*)request->data))+ " Ready to boot";
                _rebootGpsTimes = 0;
                emit appendStatusLog(bootString, true);
                emit nextFirmwareUpgrade();
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

void FileManager::listDirectory(const QString& dirPath)
{
    if (_currentOperation != kCOIdle) {
        _emitErrorMessage(tr("Command not sent. Waiting for previous command to complete."));
        return;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _emitErrorMessage(tr("Command not sent. No Vehicle links."));
        return;
    }

    // initialise the lister
    _listPath = dirPath;
    _listOffset = 0;
    _currentOperation = kCOList;

    // and send the initial request
    _sendListCommand();
}

void FileManager::_fillRequestWithString(Request* request, const QString& str)
{
    strncpy((char *)&request->data[0], str.toStdString().c_str(), sizeof(request->data));
    request->hdr.size = static_cast<uint8_t>(strnlen((const char *)&request->data[0], sizeof(request->data)));
}

void FileManager::_sendListCommand(void)
{
    Request request;

    request.hdr.session = 0;
    request.hdr.opcode = kCmdListDirectory;
    request.hdr.offset = _listOffset;
    request.hdr.size = 0;

    _fillRequestWithString(&request, _listPath);

    qCDebug(FileManagerLog) << "listDirectory: path:" << _listPath << "offset:" <<  _listOffset;
    
    _sendRequest(&request);
}

void FileManager::downloadPath(const QString& from, const QDir& downloadDir)
{
    if (_currentOperation != kCOIdle) {
        _emitErrorMessage(tr("Command not sent. Waiting for previous command to complete."));
        return;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _emitErrorMessage(tr("Command not sent. No Vehicle links."));
        return;
    }
    
	qCDebug(FileManagerLog) << "downloadPath from:" << from << "to:" << downloadDir;
	_downloadWorker(from, downloadDir, true /* read file */);
}

void FileManager::streamPath(const QString& from, const QDir& downloadDir)
{
    if (_currentOperation != kCOIdle) {
        _emitErrorMessage(tr("Command not sent. Waiting for previous command to complete."));
        return;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _emitErrorMessage(tr("Command not sent. No Vehicle links."));
        return;
    }
    
	qCDebug(FileManagerLog) << "streamPath from:" << from << "to:" << downloadDir;
	_downloadWorker(from, downloadDir, false /* stream file */);
}

void FileManager::_downloadWorker(const QString& from, const QDir& downloadDir, bool readFile)
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
///     @param selectedFirmware  Reboot Firmware
void FileManager::uploadPath(const QString& toPath, const QFileInfo& uploadFile, int selectedFirmware)
{
    _selectedComponent = -1;
    _selectedComponentReboot = selectedFirmware;
    uploadPath(toPath,uploadFile);
}
void FileManager::uploadPath(const QString& toPath, const QFileInfo& uploadFile)
{
    _toPath = toPath;
    _uploadFile = uploadFile;
    if(_currentOperation != kCOIdle){
        _emitErrorMessage(tr("UAS File manager busy.  Try again later"));
        return;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _emitErrorMessage(tr("Command not sent. No Vehicle links."));
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
    request.hdr.offset = 0;
    request.hdr.size = 0;
    _fillRequestWithString(&request, toPath + "/" + uploadFile.fileName());
    _sendRequest(&request);
}

QString FileManager::errorString(uint8_t errorCode)
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
bool FileManager::_sendOpcodeOnlyCmd(uint8_t opcode, OperationState newOpState)
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
/// @return TRUE: _ackTimer start, FALSE: _ackTimer not start,_ackTimer is starting or Nak response
bool FileManager::_setupAckTimeout(void)
{
    qCDebug(FileManagerLog) << "_setupAckTimeout";
    if(_ackTimer.isActive () || _ackTimes >= 30){
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
void FileManager::_clearAckTimeout(void)
{
	qCDebug(FileManagerLog) << "_clearAckTimeout";
    _ackTimes = 0;
    _ackTimer.stop();
}

/// @brief Stop the ack timer, restart init the parm
void FileManager::stopAckTimeout()
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
void FileManager::_ackTimeout(void)
{
    qCDebug(FileManagerLog) << "_ackTimeout";
    
    // Make sure to set _currentOperation state before emitting error message. Code may respond
    // to error message signal by sending another command, which will fail if state is not back
    // to idle. FileView UI works this way with the List command.
    _ackTimes++;
    Request requestRead, requestWrite;
    qDebug() << "_currentOperation: " << _currentOperation << " : "<< _resetStatus;
    if(_resetStatus) {
        qDebug() << "_sendResetCommand timeout, now send it again";
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

void FileManager::_sendResetCommand(void)
{
    Request request;
    request.hdr.opcode = kCmdResetSessions;
    request.hdr.size = 0;
    _resetStatus = true;
    _sendRequest(&request);
}

void FileManager::_emitErrorMessage(const QString& msg)
{
    qCDebug(FileManagerLog) << "Error:" << msg;
    //deal error
    qWarning()<<__FILE__<<" : "<<__LINE__<<" : "<<msg;
    if(_selectedComponent == -1)
    {
        emit appendStatusLog("Upload Firmware failed: " + msg);
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
        emit commandError(msg);
    }
}

void FileManager::_emitListEntry(const QString& entry)
{
    qCDebug(FileManagerLog) << "_emitListEntry" << entry;
    emit listEntry(entry);
}

/// @brief Used when update firmware by wireless connection
void FileManager::_sendSearchVersionCommand(int selectedComponent)
{
    qDebug() << "FileManager _sendSearchVersionCommand_ send";
    if(_selectedComponent == -1)
    {
        _emitErrorMessage("Can not send search version cmd : select component failed!");
        return ;
    }
    _selectedComponentReboot = -1;
    _selectedComponent = selectedComponent;
    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _emitErrorMessage(tr("Command not sent. No Vehicle links."));
        return;
    }

    _currentOperation = kCOSearchVersion;
    Request request;
    request.hdr.session = 0;
    request.hdr.opcode = kCmdSearchVersion;
    request.hdr.offset = 0;
    request.hdr.size = sizeof(uint8_t);
    request.data[0] = (uint8_t)_selectedComponent;
    _sendRequest(&request);
}

/// @brief Upload firmware file complete, now ready to boot
void FileManager::_sendBootReadyCommand()
{
    qDebug() << "_sendBootReadyCommand";
    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _emitErrorMessage(tr("Command not sent. No Vehicle links."));
        return;
    }

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
void FileManager::_sendRequest(Request* request)
{
   if(!_setupAckTimeout()){
       return ;
   }

    mavlink_message_t message;

    
    _lastOutgoingSeqNumber++;

    request->hdr.seqNumber = _lastOutgoingSeqNumber;
    
    qCDebug(FileManagerLog) << "_sendRequest opcode:" << request->hdr.opcode << "seqNumber:" << request->hdr.seqNumber;
    
    if (_systemIdQGC == 0) {
        _systemIdQGC = qgcApp()->toolbox()->mavlinkProtocol()->getSystemId();
    }

    // Unit testing code can end up here without _dedicateLink set since it tests inidividual commands.
    LinkInterface* link;
    if (_dedicatedLink) {
        link = _dedicatedLink;
    } else {
        link = _vehicle->priorityLink();
    }
    
    Q_ASSERT(_vehicle);
    mavlink_msg_file_transfer_protocol_pack_chan(_systemIdQGC,       // QGC System ID
                                                 0,                  // QGC Component ID
                                                 link->mavlinkChannel(),
                                                 &message,           // Mavlink Message to pack into
                                                 0,                  // Target network
                                                 _systemIdServer,    // Target system
                                                 0,                  // Target component
                                                 (uint8_t*)request); // Payload
    
    _vehicle->sendMessageOnLink(link, message);
}
