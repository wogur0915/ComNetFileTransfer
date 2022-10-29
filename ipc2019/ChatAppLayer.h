#pragma once
// ChatAppLayer.h: interface for the CChatAppLayer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHATAPPLAYER_H__E78615DE_0F23_41A9_B814_34E2B3697EF2__INCLUDED_)
#define AFX_CHATAPPLAYER_H__E78615DE_0F23_41A9_B814_34E2B3697EF2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BaseLayer.h"
#include "pch.h"
class CChatAppLayer
	: public CBaseLayer
{
private:
	inline void		ResetHeader();
	CObject* mp_Dlg;

public:
	BOOL			Receive(unsigned char* ppayload);
	BOOL			Send(unsigned char* ppayload, int nlength);

	CChatAppLayer(char* pName);
	virtual ~CChatAppLayer();

	typedef struct _CHAT_APP_HEADER {
		unsigned short	dataLength; // total length of the data
		unsigned char	messageType; // type of application data. For chat messages, 0x1 indicates broadcast; otherwise it's from the recipient.
		unsigned char   unused; // Unused
		unsigned char	data[APP_DATA_SIZE]; // application data

	} CHAT_APP_HEADER, * PCHAT_APP_HEADER;

	// Used for indicating message type.
	enum : unsigned short {
		CHAT_MESSAGE_NORMAL,
		CHAT_MESSAGE_BROADCAST
	};

protected:
	CHAT_APP_HEADER		m_sHeader;
};

#endif // !defined(AFX_CHATAPPLAYER_H__E78615DE_0F23_41A9_B814_34E2B3697EF2__INCLUDED_)










