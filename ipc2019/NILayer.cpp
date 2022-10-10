#include "pch.h"
#include "NILayer.h"

#define BPF_MAJOR_VERSION

#include <pcap.h>
#include <Packet32.h>
#pragma comment(lib, "packet.lib")

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

BOOL CNILayer::GetMacAddress(char* deviceName, CNILayer::PhysicalAddress* outAddress)
{
	// Allocate enough space for PACKET_OID_DATA and the physical address on the stack
	// (PACKET_OID_DATA is a variable length structure)
	PPACKET_OID_DATA oidData = (PPACKET_OID_DATA)_malloca(sizeof(PACKET_OID_DATA) + 6);
	if (oidData == nullptr)
	{
		// Out of stack space. Don't think we really can proceed much further at this point... but whatever.
		return false;
	}

	oidData->Oid = OID_802_3_PERMANENT_ADDRESS;
	oidData->Length = 6;
	ZeroMemory(oidData->Data, 6);

	LPADAPTER adapter = PacketOpenAdapter(deviceName);
	if (adapter == nullptr)
	{
		// Either: The device doesn't have an associated physical address.
		// (Possible if we're querying for virtual devices for loopback capture etc.)
		// or: The device name is invalid.
		return false;
	}

	if (!PacketRequest(adapter, false, oidData))
	{
		return false;
	}

	for (int i = 0; i < 6; i++)
	{
		((char*)outAddress)[i] = oidData->Data[i];
	}

	PacketCloseAdapter(adapter);
	return true;
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