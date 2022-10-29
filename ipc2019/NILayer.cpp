#include "pch.h"
#include "NILayer.h"

#define BPF_MAJOR_VERSION

#include <pcap.h>
#include <Packet32.h>
#pragma comment(lib, "packet.lib")

// Abstraction layer for transmissions over the data link layer
CNILayer::CNILayer(char* pName)
    : CBaseLayer(pName), devicesList{}
{
    LoadNpcapDlls();
    PopulateDeviceList();
}

CNILayer::~CNILayer()
{
    pcap_freealldevs(pointerToDeviceList);
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

// Method called on class initialization to populate the devices list.
void CNILayer::PopulateDeviceList()
{
    char error[PCAP_ERRBUF_SIZE];
    pcap_if_t* allDevices;
    if (!pcap_findalldevs(&allDevices, error))
    {
        this->pointerToDeviceList = allDevices;
        for (pcap_if_t* currentDevice = allDevices; currentDevice != nullptr; currentDevice = currentDevice->next)
        {
            NetworkDevice device{};
            device.name = currentDevice->name;
            device.description = currentDevice->description;
            devicesList.push_back(device);
        }
    }
    // We don't free the device list here because device_list has references to resources owned by allDevices.
    // We'll free them in the destructor.
}

std::vector<CNILayer::NetworkDevice>* CNILayer::GetDevicesList()
{
    return &this->devicesList;
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
    if (deviceName == nullptr)
    {
        return false;
    }
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

// Sends the specified packet over the wire.
BOOL CNILayer::Send(unsigned char* payload, int length)
{
    int code = pcap_inject(_adapter, payload, length);
    return code == 0;
}

void CNILayer::StartReceive(char* adapterName)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    // pcap_open_live(device, snaplen, promisc, to_ms, errbuf) - Opens and prepares a capture handle for the specified device.
    // Parameters:
    // device - the device identifier string
    // snaplen - the maximum snapshot length
    // promisc - flag to set promiscuous mode
    // to_ms - timeout in milliseconds.
    // 
    // From https://npcap.com/guide/wpcap/pcap.html, "A snapshot length of 65535 should be sufficient, on most if not all networks..."
    // As per the buffer size (for storing packets as it arrives) it's apparently 1MB when opened using pcap_open_live
    // If needed, pcap_set_buffer_size() with pcap_create can be used instead to create & configure the capture handle
    // However 1MB should be fine for us.
    pcap_t* handle = pcap_create(adapterName, errbuf);
    if (!handle)
    {
        AfxMessageBox("Failed to open device handle");
#ifdef DEBUG
        DebugBreak();
#endif
        return;
    }
    pcap_set_timeout(handle, 0); // Packet buffer timeout; time to wait for the buffer to fill up with traffics, for efficient processing. We don't need it
    pcap_set_promisc(handle, TRUE); // Enable promiscuous mode; no hardware filtering, though we probably don't need it?
    pcap_set_immediate_mode(handle, TRUE); // Don't wait for the buffer to fill up. Receive packets as soon as they arrive
    pcap_set_buffer_size(handle, 1024 * 1024); // 1MB is apparently the default for handles created with pcap_open_live.
    pcap_set_snaplen(handle, 65535); // From https://npcap.com/guide/wpcap/pcap.html
    // "A snapshot length of 65535 should be sufficient, on most if not all networks..."

    pcap_activate(handle);

    _adapter = handle;

    // Fire up a thread to eagerly wait
    AfxBeginThread(ReceiveMessageLoop, this);
}

BOOL CNILayer::Receive()
{
    // use pcap_next_ex() to fetch a packet
    pcap_pkthdr* packetHeader;
    u_char* packet;
    // pcap_next_ex will continue to block until the timeout is reached or a packet is received
    int code = pcap_next_ex(_adapter, &packetHeader, (const u_char**)&packet);
    switch (code)
    {
        case 1: // The packet is read normally
            break;
        case PCAP_ERROR: // An error occured while reading
            AfxMessageBox(pcap_geterr(_adapter));
#ifdef DEBUG
            DebugBreak();
#endif
            // Fallthrough
        case 0:	// Timeout
        default:
            return false;
    }

    return this->GetUpperLayer(0)->Receive(packet);
}

UINT CNILayer::ReceiveMessageLoop(LPVOID pParam)
{
    CNILayer* thisPtr = (CNILayer*)pParam;
    assert(thisPtr->_adapter);

    while (true)
    {
        thisPtr->Receive();
    }

    return 0;
}
