#include "stdafx.h"
#include "UDPPing.h"
#include <process.h>


typedef unsigned (__stdcall *PTHREAD_START)(void*);
#define BEGINTHREADEX(psa, cbStack, pfnStartAddr, pvParam, fdwCreate, pdwThreadId) \
	((HANDLE)_beginthreadex((void*)(psa), \
	(unsigned)(cbStack), (PTHREAD_START)(pfnStartAddr),\
	(void*)(pvParam), (unsigned)(fdwCreate), (unsigned*)(pdwThreadId)))


//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
static DWORD WINAPI fnUdpPingServerWorker(LPVOID p)
{
	ASSERT(NULL != p);
	CUdpPingServer* pInstance = (CUdpPingServer*) p;
	
	return pInstance->fnUdpPing();
}

CUdpPingServer::CUdpPingServer()
{
	SetOpMode(eOpAsServer);

	WSADATA WsaData;
	WSAStartup(0x202, &WsaData);

	m_Sock			= INVALID_SOCKET;
	m_hThread		= NULL;
	m_bSuspended	= TRUE;
}

CUdpPingServer::~CUdpPingServer()
{
	Destroy();

	WSACleanup();
}

void CUdpPingServer::Create(std::tstring& szIP, unsigned short usPort)
{
	m_Sock = CreateUdpSocket();
	ASSERT(INVALID_SOCKET != m_Sock);

	int nRet = BindSocket(m_Sock, MakeIP(szIP.c_str()), usPort);
	if( nRet == SOCKET_ERROR )
	{
		ASSERT( false );
	}

	DWORD dwThreadId;
	m_hThread = BEGINTHREADEX(NULL, 0, fnUdpPingServerWorker, this, CREATE_SUSPENDED, &dwThreadId);
	ASSERT(m_hThread != NULL);
}

void CUdpPingServer::Destroy()
{
	if(m_hThread != NULL)
	{
		if(m_bSuspended)
			StartPingSerivce();

		CloseSocket(m_Sock);
		m_Sock = INVALID_SOCKET;

		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
}

BOOL CUdpPingServer::StartPingSerivce()
{
	if(m_hThread == NULL)
		return FALSE;

	if(m_bSuspended)
	{
		DWORD dwResumeCount = ResumeThread(m_hThread);
		if(0xffffffff == dwResumeCount)
			return FALSE;

		if(dwResumeCount > 1)
		{
			for (DWORD i = 0; i < dwResumeCount - 1; i++)
				ResumeThread(m_hThread);
		}

		InterlockedExchange((LONG volatile*)&m_bSuspended, FALSE);
	}	

	return TRUE;
}

BOOL CUdpPingServer::StopPingService()
{
	if(m_hThread == NULL)
		return FALSE;

	if(m_bSuspended == FALSE)
	{
		DWORD dwPrevSuspendCount = SuspendThread(m_hThread);
		if(dwPrevSuspendCount == 0xFFFFFFFF)
		{
			return (dwPrevSuspendCount == MAXIMUM_SUSPEND_COUNT);
		}

		if(dwPrevSuspendCount > 0)
		{
			for(DWORD i = 0; i < dwPrevSuspendCount; i++)
				ResumeThread(m_hThread);
		}

		InterlockedExchange((LONG volatile*)&m_bSuspended, TRUE);		
	}

	return TRUE;
}

DWORD CUdpPingServer::fnUdpPing()
{
	char szBuf[PING_BUFFER_LEN];
	SOCKADDR Addr;
	int nAddrLen;
	int nRet = 0;

	for(;;)
	{
		nAddrLen = sizeof(Addr);
		ZeroMemory(szBuf, sizeof(szBuf));

		nRet = recvfrom(m_Sock, szBuf, PING_BUFFER_LEN, 0, &Addr, &nAddrLen);
		if(nRet == SOCKET_ERROR)
		{
			DWORD dwErr = WSAGetLastError();
			if(WSAECONNRESET == dwErr)
			{
				OutputDebugString(_T("unreachable host!\n"));
				continue;
			}

			TCHAR tszErr[0xff];
			if (SF_sprintf(tszErr, 0xff, _T("recvfrom failed! [err: %d]\n"), dwErr) != S_OK)
			{
				_ASSERT(FALSE);
				OutputDebugString(tszErr);
			}
			
			return 0;
		}

		if(szBuf[0] == 'R' && szBuf[1] == 'E' && szBuf[2] == 'Q')
		{
			OutputDebugString(_T("REQ received!\n"));

			_PingPacket* pPacket = (_PingPacket*)szBuf;

			// 수신받은 UDP Req에 대해 무조건 ACK을 던진다.
			if(SOCKET_ERROR == SendAcknowledge(m_Sock, (SOCKADDR_IN*)&Addr, pPacket->lPerfCount))
			{
				OutputDebugString(_T("failed to send ack!\n"));
			}
		}
	}
}

//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------

DWORD WINAPI fnUdpPingClientWorker(LPVOID p)
{
	ASSERT(NULL != p);
	CUdpPingClient* pInstance = (CUdpPingClient*) p;
	
	return pInstance->fnUdpPing();
}

CUdpPingClient::CUdpPingClient()
{
	SetOpMode(eOpAsClient);

	m_hNotifyWnd	= NULL;
	m_nMsgID		= UINT(-1);

	m_Sock			= INVALID_SOCKET;
	m_hThread		= NULL;

	WSADATA WsaData;
	WSAStartup(0x202, &WsaData);

	GetFrequency();
}

CUdpPingClient::~CUdpPingClient()
{
	Destroy();

	WSACleanup();
}

void CUdpPingClient::Create(HWND hNotifyWnd, UINT nMsgID)
{
	m_hNotifyWnd = hNotifyWnd;
	m_nMsgID	 = nMsgID;

	m_Sock = CreateUdpSocket();
	ASSERT(INVALID_SOCKET != m_Sock);
}

void CUdpPingClient::Destroy()
{
	if(m_Sock != INVALID_SOCKET)
	{
		CloseSocket(m_Sock);
		m_Sock = INVALID_SOCKET;
	}

	if(m_hThread)
	{
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	m_PingDestMap.clear();
}

WORD CUdpPingClient::AddPingDestination(WORD wIndex, std::tstring& szDestIP, unsigned short usDestPort)
{
	std::map<WORD, UdpPingDestination>::iterator it = m_PingDestMap.find(wIndex);
	if(it != m_PingDestMap.end())
		return (WORD)-1;

	UdpPingDestination Dest; 
	Dest.Init(wIndex, szDestIP, usDestPort);

	m_PingDestMap.insert(std::map<WORD, UdpPingDestination>::value_type(wIndex, Dest));

	return wIndex;
}

void CUdpPingClient::RemovePingDestination(WORD wIndex)
{
	std::map<WORD, UdpPingDestination>::iterator it = m_PingDestMap.find(wIndex);
	if(it != m_PingDestMap.end())
		return;
	
	m_PingDestMap.erase(it);
}

inline void CUdpPingClient::SendPingRequest(UdpPingDestination& Dest)
{
	SendRequest(m_Sock, Dest.GetDestination(), GetPerformanceCounter());

	// 첫 송신 후 recv 대기하도록 하기 위해.... ㅡ,.ㅡ
	if(m_hThread == NULL)
	{
		DWORD dwThreadId;
		m_hThread = BEGINTHREADEX(NULL, 0, fnUdpPingClientWorker, this, 0, &dwThreadId);
		ASSERT(m_hThread != NULL);
	}
}

void CUdpPingClient::SendPingRequest(WORD wIndex)
{
	std::map<WORD, UdpPingDestination>::iterator it;
	
	if(ALL_SERVERS == wIndex)
	{		
		it = m_PingDestMap.begin();
		std::map<WORD, UdpPingDestination>::iterator it_end = m_PingDestMap.end();

		for(; it != it_end; ++it)
		{
			SendPingRequest((*it).second);
		}
	}
	else
	{
		it = m_PingDestMap.find(wIndex);
		if(it != m_PingDestMap.end())
		{
			SendPingRequest((*it).second);
		}
	}
}

WORD CUdpPingClient::GetDestIndex(const SOCKADDR_IN* pAddr)
{
	WORD wIndex = (WORD)-1;

	std::map<WORD, UdpPingDestination>::iterator it = m_PingDestMap.begin();
	std::map<WORD, UdpPingDestination>::iterator it_end = m_PingDestMap.end();
	for(; it != it_end; ++it)
	{
		SOCKADDR_IN* pDest = (*it).second.GetDestination();
		if(0 == memcmp(pAddr, pDest, sizeof(SOCKADDR_IN)))
		{
			wIndex = (*it).second.GetIndex();
			break;
		}
	}

	return wIndex;
}

DWORD CUdpPingClient::fnUdpPing()
{
	char szBuf[PING_BUFFER_LEN];
	SOCKADDR Addr;
	int nAddrLen;
	int nRet = 0;

	for(;;)
	{
		nAddrLen = sizeof(SOCKADDR);
		ZeroMemory(szBuf, sizeof(szBuf));

		nRet = recvfrom(m_Sock, szBuf, PING_BUFFER_LEN, 0, &Addr, &nAddrLen);
		if(nRet == SOCKET_ERROR)
		{
			DWORD dwErr = WSAGetLastError();
			if(WSAECONNRESET == dwErr)
			{				
				PostMessage(m_hNotifyWnd, m_nMsgID, MAKE_WPARAM(GetDestIndex((SOCKADDR_IN*)&Addr), WSAECONNRESET), 0);
				OutputDebugString(_T("unreachable host!\n"));
				continue;
			}
			else
			{
				TCHAR tszErr[0xff];
				if (SF_sprintf(tszErr, 0xff, _T("recvfrom failed! [err: %d]\n"), dwErr) != S_OK)
				{
					_ASSERT(FALSE);
					OutputDebugString(tszErr);
				}
				return 0;
			}
		}

		if(szBuf[0] == 'A' && szBuf[1] == 'C' && szBuf[2] == 'K')
		{
			_PingPacket* pPacket = (_PingPacket*)szBuf;

			LONGLONG lRTT = (GetPerformanceCounter() - pPacket->lPerfCount);
			if(m_bUsePerfCounter)
			{
				lRTT = lRTT * 1000 / m_lFrequency;
			}

			// ACK 수신 결과를 통보한다.
			PostMessage(m_hNotifyWnd, m_nMsgID, MAKE_WPARAM(GetDestIndex((SOCKADDR_IN*)&Addr), 0), (DWORD)lRTT);
			OutputDebugString(_T("ACK received!\n"));
		}
		else
		{
			OutputDebugString(_T("unknown datagram received!\n"));
		}
	}
}
