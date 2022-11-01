#include "pch.h"
#include "FileAppLayer.h"

CFileAppLayer::CFileAppLayer(char* pName) : CBaseLayer(pName), _message(), _receivedHandler(),
_receiveHandlerParam(), _fileReceiving(), _fileSending()
{
}

CFileAppLayer::~CFileAppLayer()
{

}

void CFileAppLayer::SetFileReceiveHandler(CFileAppLayer::ProgressUpdateHandler handler, void* param)
{
	_receivedHandler = handler;
	_receiveHandlerParam = param;
}

// Sending methods
// Takes in a file path and a progress update handler
// Fires up a thread that wraps the segment up appropriately & send it down the layer

// Internal method that just passes down the raw data to the next layer.
BOOL CFileAppLayer::Send(unsigned char* data, size_t len)
{
	auto ethernet = (CEthernetLayer*)GetUnderLayer();
	ethernet->Send(data, len, FILE_TYPE);
	_fileSending.lastAck = 0xFFFFFFFF;
	return TRUE;
}

BOOL CFileAppLayer::SendFile(CString filePath, CFileAppLayer::ProgressUpdateHandler progressUpdated, void* handlerParam)
{
	_sendingHandler = progressUpdated;
	_sendingHandlerParam = handlerParam;
	_fileSending.path = filePath;

	AfxBeginThread(SendFileInternal, this);
	return TRUE;
}

UINT CFileAppLayer::SendFileInternal(LPVOID pParam)
{
	CFileAppLayer* thisPtr = (CFileAppLayer*)pParam;
	CFile file;
	file.Open(thisPtr->_fileSending.path, CFile::modeRead | CFile::typeBinary);
	
	CString fileName = file.GetFileName();
	int fileNameLength = fileName.GetLength();
	ULONGLONG fileLength = file.GetLength();
	// The calculation is required, because we are going to send the total fragment count and the file name
	unsigned long long messageLength = static_cast<unsigned long long>(sizeof(int)) + fileNameLength + 1 + fileLength;
	unsigned int fragmentCount = messageLength / FILE_APP_MAX_DATA_SIZE + 1;
	if (fileLength % FILE_APP_MAX_DATA_SIZE > 0)
	{
		fragmentCount++;
	}

	do
	{
		TRACE("Sending file header (fragment #0)");
		thisPtr->SendMetadata(fileName, fragmentCount);
	} while (!thisPtr->WaitForAck(0));

	thisPtr->_sendingHandler(thisPtr->_receiveHandlerParam, 1, fragmentCount);

	for (unsigned int i = 1; i < fragmentCount - 1; i++)
	{
		do
		{
			TRACE("Sending file part %d out of %d", i, fragmentCount);
			thisPtr->SendPart(i, file, false);
		} while (!thisPtr->WaitForAck(i));
		thisPtr->_sendingHandler(thisPtr->_receiveHandlerParam, i, fragmentCount);
	}

	do
	{
		TRACE("Sending final fragment");
		thisPtr->SendPart(fragmentCount - 1, file, true);
	} while (!thisPtr->WaitForAck(fragmentCount - 1));
	thisPtr->_sendingHandler(thisPtr->_receiveHandlerParam, fragmentCount, fragmentCount);

	file.Close();
	return 0;
}

// Sends the metadata
int CFileAppLayer::SendMetadata(CString fileName, unsigned int fragmentCount)
{
	FILE_MESSAGE message;
	message.messageType = MessageType::METADATA;
	message.unused = 0;
	message.sequenceNumber = 0;

	int stringLength = fileName.GetLength() + 1; // Incl. null terminator
	int metadataLength = sizeof(int) + stringLength;

	message.dataLength = metadataLength;
	char* buffer = message.data;
	*(int*)buffer = fragmentCount;
	buffer += sizeof(int);
	strcpy_s(buffer, APP_DATA_SIZE - sizeof(int), fileName.GetBuffer());
	buffer += stringLength;
	*(buffer - 1) = '\0';

	Send((unsigned char*)&message, FILE_APP_HEADER_SIZE + metadataLength);
	return 0;
}

int CFileAppLayer::SendPart(unsigned int sequenceNumber, CFile& file, bool isFinal)
{
	FILE_MESSAGE message;
	message.messageType = isFinal ? MessageType::FINALFRAGMENT : MessageType::FRAGMENT;
	message.unused = 0;
	message.sequenceNumber = sequenceNumber;

	int bytesRead = file.Read(message.data, FILE_APP_MAX_DATA_SIZE);
	message.dataLength = bytesRead;

	Send((unsigned char*)&message, FILE_APP_HEADER_SIZE + bytesRead);
	return bytesRead;
}

bool CFileAppLayer::WaitForAck(unsigned int sequenceNumber)
{
	for (int i = 0; i < 10; i++)
	{
		if (_fileSending.lastAck == sequenceNumber)
		{
			TRACE("ACK for %d processed", sequenceNumber);
			return true;
		}
		Sleep(1);
	}

	return false;
}

// Receiving method
// When the first file is encountered, create a file, then write it to a file next to the current location.

BOOL CFileAppLayer::Receive(unsigned char* payload)
{
	FILE_MESSAGE* message = (FILE_MESSAGE*)payload;

	char* data = message->data;
	if (message->messageType == MessageType::ACK)
	{
		TRACE("Received ack for %d", message->sequenceNumber);
		_fileSending.lastAck = message->sequenceNumber;
		return true;
	}
	
	TRACE("Sending ACK for %d", message->sequenceNumber);
	SendAck(message->sequenceNumber);

	if (message->messageType == MessageType::METADATA)
	{
		TRACE("Received metadata");
		// Prepare to receive file
		_fileReceiving.fragmentsReceived = 1;
		unsigned int fragmentCount = *(unsigned int*)data;
		data += sizeof(int);
		int len = strlen(data) + 1;
		_fileReceiving.totalFragmentsToReceive = fragmentCount;
		_fileReceiving.name.Format("%s", data);
		data += len;
		CFileException ex;
		if (!_fileReceiving.receivedFile.Open("tempfile_downloading", CFile::modeCreate | CFile::modeWrite | CFile::typeBinary, &ex))
		{
			TRACE("Failed to create file");
		}

		_receivedHandler(_receiveHandlerParam, 1, fragmentCount);
		return true;
	}

	if (_fileReceiving.receivedFile.m_hFile == CFile::hFileNull)
	{
		// Don't have any ongoing transmission to care about. Ignore
		return false;
	}

	TRACE("writing segment %d to file", message->sequenceNumber);
	// here, the file must be some sort of fragment
	LONGLONG position = static_cast<LONGLONG>(message->sequenceNumber - 1) * FILE_APP_MAX_DATA_SIZE;
	_fileReceiving.receivedFile.Seek(position, CFile::begin);
	_fileReceiving.receivedFile.Write(data, message->dataLength);
	_receivedHandler(_receiveHandlerParam, message->sequenceNumber + 1, _fileReceiving.totalFragmentsToReceive);

	if (message->messageType == MessageType::FINALFRAGMENT)
	{
		TRACE("Last segment received, closing file");
		// Close file
		_fileReceiving.receivedFile.Close();
		CFile::Rename("tempfile_downloading", _fileReceiving.name);
	}
	return true;
}

void CFileAppLayer::SendAck(unsigned int sequenceNumber)
{
	FILE_MESSAGE message;
	message.messageType = MessageType::ACK;
	message.dataLength = 0;
	message.unused = 0;
	message.sequenceNumber = sequenceNumber;

	Send((unsigned char*)&message, FILE_APP_HEADER_SIZE);
}
