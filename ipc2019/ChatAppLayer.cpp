// ChatAppLayer.cpp: implementation of the CChatAppLayer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "pch.h"
#include "ChatAppLayer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CChatAppLayer::CChatAppLayer(char* pName)
    : CBaseLayer(pName),
    mp_Dlg(NULL)
{
    ResetHeader();
}

CChatAppLayer::~CChatAppLayer()
{

}

void CChatAppLayer::ResetHeader()
{
    m_sHeader.dataLength = 0x0000;
    m_sHeader.messageType = 0x00;
    memset(m_sHeader.data, 0, APP_DATA_SIZE);
}

BOOL CChatAppLayer::Send(unsigned char* ppayload, int nlength)
{
    // Chat layer does not support payloads with size greater than APP_DATA_SIZE.
    // In such cases, simply truncate the rest of the data.
    unsigned short length = min(APP_DATA_SIZE, nlength);
    m_sHeader.dataLength = length;

    BOOL bSuccess = FALSE;

    memcpy(m_sHeader.data, ppayload, length);
    bSuccess = ((CEthernetLayer*)(mp_UnderLayer))->Send((unsigned char*)&m_sHeader, length + APP_HEADER_SIZE, CHAT_TYPE);
    return bSuccess;
}

BOOL CChatAppLayer::Receive(unsigned char* ppayload)
{
    PCHAT_APP_HEADER app_hdr = (PCHAT_APP_HEADER)ppayload;

    // The payload may not be properly null terminated if it was truncated.
    // We handle such situations by copying the value into local buffer then adding the null terminator at the end manually.
    unsigned char buffer[APP_DATA_SIZE + 1];

    memcpy(buffer, app_hdr->data, app_hdr->dataLength);
    buffer[app_hdr->dataLength] = '\0';

    CString Msg;

    // The type field. This is actually set from the layer below...
    if (app_hdr->messageType == CHAT_MESSAGE_BROADCAST)
        Msg.Format(_T("[BROADCAST] %s"), (char*)buffer);
    else
        Msg.Format(_T("[Recipient] %s"), (char*)buffer);

    // Passing the newly formatted string to the dialog for display.
    mp_aUpperLayer[0]->Receive((unsigned char*)Msg.GetBuffer(0));
    return TRUE;
}


