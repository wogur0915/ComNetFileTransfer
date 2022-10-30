#pragma once
#include "BaseLayer.h"

constexpr auto FILE_APP_HEADER_SIZE = sizeof(short) * 2 + sizeof(int);
constexpr auto FILE_APP_MAX_MESSAGE_SIZE = ETHER_MAX_DATA_SIZE;
constexpr auto FILE_APP_MAX_DATA_SIZE = FILE_APP_MAX_MESSAGE_SIZE - FILE_APP_HEADER_SIZE;

class CFileAppLayer : public CBaseLayer
{
public:
    typedef void (*ProgressUpdateHandler)(void* param, unsigned int processed, unsigned int total);

    BOOL SendFile(CString filePath, ProgressUpdateHandler progressHandler, void* param);
    BOOL Receive(unsigned char* payload);

    void SetFileReceiveHandler(ProgressUpdateHandler handler, void* param);
    CFileAppLayer(char* pName);
    virtual ~CFileAppLayer();
private:
    BOOL Send(unsigned char* data, size_t len);
    static UINT SendFileInternal(LPVOID pParam);
    // Returns bytes consumed
    int SendFirstPart(CString fileName, unsigned int fragmentCount, CFile& file);
    // Returns bytes consumed
    int SendPart(unsigned int sequenceNumber, CFile& file);

    // Requirements
    // Does support out-of-order files
    // however 
    typedef struct {
        unsigned short dataLength; // Length of the data contained in this fragment. For the first message, this includes the metadata
        unsigned short unused;
        unsigned int sequenceNumber; // Sequence number of this fragment. 0 based
        char data[FILE_APP_MAX_DATA_SIZE];
    } FILE_MESSAGE;
    
    // Some fixed buffer space for storing file message when receiving
    FILE_MESSAGE _message;

    // Handler invoked when a new file is received.
    ProgressUpdateHandler _receivedHandler;
    void* _handlerParam;

    // In addition to the actual file, the first packet that's transmitted also contain the following:
    // a 4-byte unsigned number, indicating how many fragments are expected - for progress report on the receiving end
    // and a file name stored as a null terminated system-locale string.
    
    // The data is always split up into chunks of FILE_APP_MAX_DATA_SIZE size.
    // To determine when the transmission gets completed, read the fragment counts from the first message.

    struct
    {
        // Handle to the file currently being written to. Null file (see CFile::hFileNull) if no files are currently being received.
        CFile receivedFile;
        CString name;
        // No. of fragments to receive
        unsigned int fragmentsReceived;
        // No. of fragments to receive. is set to 0 until the first packet arrives
        unsigned int totalFragmentsToReceive;
    } _fileReceiving;

    struct {
        CString path;
    } _fileSending;
};