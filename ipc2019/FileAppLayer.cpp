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
}

BOOL CFileAppLayer::SendFile(CString filePath, CFileAppLayer::ProgressUpdateHandler progressUpdated, void* handlerParam)
{
	_fileSending.path = filePath;

	AfxBeginThread(SendFileInternal, this);
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

	thisPtr->SendFirstPart(fileName, fragmentCount, file);
	thisPtr->_receivedHandler(thisPtr->_handlerParam, 1, fragmentCount);

	for (unsigned int i = 1; i < fragmentCount; i++)
	{
		thisPtr->SendPart(i, file);
		thisPtr->_receivedHandler(thisPtr->_handlerParam, i, fragmentCount);
	}
	file.Close();
}

int CFileAppLayer::SendFirstPart(CString fileName, unsigned int fragmentCount, CFile& file)
{
	FILE_MESSAGE message;
	message.unused = 0;
	message.sequenceNumber = 0;

	int stringLength = fileName.GetLength() + 1; // Incl. null terminator
	int metadataLength = sizeof(int) + stringLength;
	int dataLen = file.GetLength();
	int bytesToSend = min(FILE_APP_MAX_DATA_SIZE - metadataLength, dataLen);

	message.dataLength = metadataLength + bytesToSend;
	char* buffer = message.data;
	*(int*)buffer = fragmentCount;
	buffer += sizeof(int);
	strcpy(buffer, fileName.GetBuffer());
	buffer += stringLength;
	*(buffer - 1) = '\0';
	file.Read(buffer, bytesToSend);

	Send((unsigned char*)&message, FILE_APP_HEADER_SIZE + metadataLength + bytesToSend);
	return bytesToSend;
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

// Receiving method
// When the first file is encountered, create a file, then write it to a file next to the current location.

BOOL CFileAppLayer::Receive(unsigned char* payload)
{
	FILE_MESSAGE* message = (FILE_MESSAGE*)payload;
	_fileReceiving.fragmentsReceived += 1;
	if (_fileReceiving.receivedFile.m_hFile == CFile::hFileNull)
	{
		// File not initialised. Create a temporary file for writing
		_fileReceiving.receivedFile.Open("tempfile_downloading", CFile::modeCreate | CFile::typeBinary);
	}
	char* data = message->data;
	if (message->sequenceNumber == 0)
	{
		unsigned int fragmentCount = *(unsigned int*)data;
		data += sizeof(int);
		int len = strlen(data) + 1;
		_fileReceiving.totalFragmentsToReceive = fragmentCount;
		_fileReceiving.name.Format("%s", data);
		data += len;
	}
}

