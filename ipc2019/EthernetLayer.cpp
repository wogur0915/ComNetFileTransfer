// EthernetLayer.cpp: implementation of the CEthernetLayer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "pch.h"
#include "EthernetLayer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEthernetLayer::CEthernetLayer(char* pName)
	: CBaseLayer(pName)
{
	ResetHeader();
}

CEthernetLayer::~CEthernetLayer()
{
}

void CEthernetLayer::ResetHeader()
{
	memset(m_sHeader.enet_dstaddr, 0, 6);
	memset(m_sHeader.enet_srcaddr, 0, 6);
	memset(m_sHeader.enet_data, ETHER_MAX_DATA_SIZE, 6);
	m_sHeader.enet_type = 0;
}

unsigned char* CEthernetLayer::GetSourceAddress()
{
	return m_sHeader.enet_srcaddr;
}

unsigned char* CEthernetLayer::GetDestinAddress()
{
	return m_sHeader.enet_dstaddr;
}

void CEthernetLayer::SetSourceAddress(unsigned char* pAddress)
{
	memcpy(m_sHeader.enet_srcaddr, pAddress, 6);
}

void CEthernetLayer::SetDestinAddress(unsigned char* pAddress)
{
	memcpy(m_sHeader.enet_dstaddr, pAddress, 6);
}

BOOL CEthernetLayer::Send(unsigned char* ppayload, int nlength, unsigned short type)
{
	memcpy(m_sHeader.enet_data, ppayload, nlength);
	m_sHeader.enet_type = type;
		
	return mp_UnderLayer->Send((unsigned char*)&m_sHeader, nlength + ETHER_HEADER_SIZE);
}

BOOL CEthernetLayer::Receive(unsigned char* ppayload)
{
	PETHERNET_HEADER pFrame = (PETHERNET_HEADER)ppayload;

	BOOL bSuccess = FALSE;


	// Only take in ethernet frames that are sent directly to us or is broadcast.
	if (!AddressEquals(pFrame->enet_dstaddr, m_sHeader.enet_srcaddr) || !IsBroadcast(pFrame->enet_dstaddr))
	{
		return FALSE;
	}

	// Demultiplexing
	if (pFrame->enet_type == CHAT_TYPE)
	{
		// Kind of a wonky hack. We're indicating that the message is broadcast by setting a field here when we receive the message
		if (IsBroadcast(pFrame->enet_dstaddr))
		{
			((CChatAppLayer::PCHAT_APP_HEADER)pFrame->enet_data)->messageType = CChatAppLayer::CHAT_MESSAGE_BROADCAST;
		}

		bSuccess = GetUpperLayer(0)->Receive(pFrame->enet_data);
	}

	return bSuccess;
}

bool CEthernetLayer::AddressEquals(unsigned char* addr1, unsigned char* addr2)
{
	return memcmp(addr1, addr2, 6) == 0;
}

bool CEthernetLayer::IsBroadcast(unsigned char* address)
{
	static unsigned char broadcastAddress[6] { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	return AddressEquals(address, broadcastAddress);
}