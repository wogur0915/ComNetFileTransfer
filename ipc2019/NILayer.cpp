#include "pch.h"
#include "NILayer.h"

#define BPF_MAJOR_VERSION

#include <pcap.h>
#include <Packet32.h>
#pragma comment(lib, "packet.lib")

// Copied from ntddndls.h - "The address of the NIC encoded in the hardware."
#define OID_802_3_PERMANENT_ADDRESS 0x01010101

// Abstraction layer for transmissions over the data link layer
CNILayer::CNILayer(char* pName)
	: CBaseLayer(pName)
{
	LoadNpcapDlls();
}

CNILayer::~CNILayer()
{
	
}

// Reference:
// https://github.com/nmap/npcap/blob/777dc56c76614f69c309ad0efdb325db86dc4cba/Examples/PacketDriver/GetMacAddress/GetMacAddress.c

BOOL CNILayer::LoadNpcapDlls()
{
	TCHAR npcap_dir[512];
	UINT len;
	len = GetSystemDirectory(npcap_dir, 480);
	if (!len) {
		fprintf(stderr, "Error in GetSystemDirectory: %x", GetLastError());
		return FALSE;
	}
	strcat_s(npcap_dir, 512, TEXT("\\Npcap"));
	if (SetDllDirectory(npcap_dir) == 0) {
		fprintf(stderr, "Error in SetDllDirectory: %x", GetLastError());
		return FALSE;
	}
	return TRUE;
}


// Sends the specified packet over the wire
BOOL CNILayer::Send(unsigned char* payload, int length)
{
	return true;
}

BOOL CNILayer::Receive()
{
	return true;
}