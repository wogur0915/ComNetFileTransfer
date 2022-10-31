#include "pch.h"
#include "FileAppLayer.h"

CFileAppLayer::CFileAppLayer(char* pName) : CBaseLayer(pName), _message(), _receivedHandler(),
_handlerParam(), _fileReceiving(), _fileSending()
{
}

CFileAppLayer::~CFileAppLayer()
{

}

void CFileAppLayer::SetFileReceiveHandler(CFileAppLayer::ProgressUpdateHandler handler, void* param)
{
	_receivedHandler = handler;
	_handlerParam = param;
}

// Sending methods
// Takes in a file path and a progress update handler
// Fires up a thread that wraps the segment up appropriately & send it down the layer

// Internal method that just passes down the raw data to the next layer.
BOOL CFileAppLayer::Send(unsigned char* data, size_t len)
{
	GetUnderLayer()->Send(data, len);
	_fileSending.lastAck = 0xFFFFFFFF;
	return TRUE;
}

BOOL CFileAppLayer::SendFile(CString filePath, CFileAppLayer::ProgressUpdateHandler progressUpdated, void* handlerParam)
{
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
	unsigned int fragmentCount = messageLength / FILE_APP_MAX_DATA_SIZE;
	if (fileLength % FILE_APP_MAX_DATA_SIZE > 0)
	{
		fragmentCount++;
	}

	do
	{
		thisPtr->SendMetadata(fileName, fragmentCount);
	} while (thisPtr->WaitForAck(0));

	thisPtr->_receivedHandler(thisPtr->_handlerParam, 1, fragmentCount);

	for (unsigned int i = 1; i < fragmentCount; i++)
	{
		do
		{
			thisPtr->SendPart(i, file);
		} while (thisPtr->WaitForAck(i));
		thisPtr->_receivedHandler(thisPtr->_handlerParam, i, fragmentCount);
	}
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

int CFileAppLayer::SendPart(unsigned int sequenceNumber, CFile& file)
{
	FILE_MESSAGE message;
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
		_fileSending.lastAck = *(unsigned int*)data;
		return true;
	}
	
	SendAck(message->sequenceNumber);

	if (message->messageType == MessageType::METADATA)
	{
		// Prepare to receive file
		_fileReceiving.fragmentsReceived = 1;
		unsigned int fragmentCount = *(unsigned int*)data;
		data += sizeof(int);
		int len = strlen(data) + 1;
		_fileReceiving.totalFragmentsToReceive = fragmentCount;
		_fileReceiving.name.Format("%s", data);
		data += len;
		_fileReceiving.receivedFile.Open("tempfile_downloading", CFile::modeCreate | CFile::typeBinary);
		return true;
	}

	if (_fileReceiving.receivedFile.m_hFile == CFile::hFileNull)
	{
		// Don't have any ongoing transmission to care about. Ignore
		return false;
	}

	// here, the file must be some sort of fragment
	LONGLONG position = static_cast<LONGLONG>(message->sequenceNumber) * FILE_APP_MAX_DATA_SIZE;
	_fileReceiving.receivedFile.Seek(position, CFile::begin);
	_fileReceiving.receivedFile.Write(data, message->dataLength);

	if (message->messageType == MessageType::FINALFRAGMENT)
	{
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
