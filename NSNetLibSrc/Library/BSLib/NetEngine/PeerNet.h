#pragma once

#include "socketudp.h"
#include "FileSystem.h"

#include "peernet_def.h"

class CPeerNet
{
public:
	CPeerNet(void);
	~CPeerNet(void);

public:
	CSockDatagram*		m_pSocket;

	DWORD				m_dwMainTask;

	CNetEngine*			engine_;

public:
	BOOL				StartPeerTask( CNetEngine* engine, DWORD dwTaskID );
	BOOL				StartPeerNetService( WORD port );
	void				Finalize();

	//long				SendPeerMsg( SOCKADDR_IN& addr, CMsg* pMsg );
	//BOOL				GetPeerNetSockAddr( SOCKADDR_IN& addr );

};
