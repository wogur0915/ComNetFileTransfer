#pragma once
#include "BaseLayer.h"

class CFileAppLayer : public CBaseLayer
{
    BOOL Send(unsigned char* payload, int length);
    BOOL Receive();

    CFileAppLayer(char* pName);
    virtual ~CFileAppLayer();
};