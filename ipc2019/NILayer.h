#pragma once
#include "BaseLayer.h"
#include <pcap.h>
#include <vector>
#include <tuple>
class CNILayer :
    public CBaseLayer
{
public:

    BOOL Send(unsigned char* payload, int length);
    BOOL Receive();

    CNILayer(char* pName);
    virtual ~CNILayer();

    typedef struct {
        char a, b, c, d, e, f;
    } PhysicalAddress;

    BOOL CNILayer::GetMacAddress(char* deviceName, PhysicalAddress* outAddress);

private:
    BOOL LoadNpcapDlls();


};

