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
	//////////////////////// fill the blank ///////////////////////////////
	// Ethernet 목적지 주소 return
	return m_sHeader.enet_dstaddr;
	///////////////////////////////////////////////////////////////////////
}

void CEthernetLayer::SetSourceAddress(unsigned char* pAddress)
{
	//////////////////////// fill the blank ///////////////////////////////
		// 넘겨받은 source 주소를 Ethernet source주소로 지정
	memcpy(m_sHeader.enet_srcaddr, pAddress, 6);
	///////////////////////////////////////////////////////////////////////
}

void CEthernetLayer::SetDestinAddress(unsigned char* pAddress)
{
	memcpy(m_sHeader.enet_dstaddr, pAddress, 6);
}

BOOL CEthernetLayer::Send(unsigned char* ppayload, int nlength)
{
	// ChatApp 계층에서 받은 App 계층의 Frame 길이만큼을 Ethernet계층의 data로 넣는다.
	memcpy(m_sHeader.enet_data, ppayload, nlength);

	BOOL bSuccess = FALSE;
	//////////////////////// fill the blank ///////////////////////////////

		// Ethernet Data + Ethernet Header의 사이즈를 합한 크기만큼의 Ethernet Frame을
		// File 계층으로 보낸다.
	bSuccess = mp_UnderLayer->Send((unsigned char*)&m_sHeader, nlength + ETHER_HEADER_SIZE);
	///////////////////////////////////////////////////////////////////////
	return bSuccess;
}

BOOL CEthernetLayer::Receive(unsigned char* ppayload)
{
	PETHERNET_HEADER pFrame = (PETHERNET_HEADER)ppayload;

	BOOL bSuccess = FALSE;
	//////////////////////// fill the blank ///////////////////////////////
		// ChatApp 계층으로 Ethernet Frame의 data를 넘겨준다.
	bSuccess = GetUpperLayer(0)->Receive(pFrame->enet_data);
	///////////////////////////////////////////////////////////////////////

	return bSuccess;
}
