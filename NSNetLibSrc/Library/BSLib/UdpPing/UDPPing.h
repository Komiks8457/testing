#pragma once

//#include <string>
//#include <map>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

/*////////////////////////////////////////////////////////////////////////////////////////////

  UDP Ping Class
		2005. 08. 29. (by deepdark)

	- CUdpPingServer :	핑 서버(Create -> Start -> Stop -> Destroy 의 순으로 사용) 
	- CUdpPingClient :	핑 클라이언트. 다수의 Ping Destination을 등록하고, Index로 접근
						Create -> AddDestinations -> AddDestination/RemoveDestination/SendPingRequest
						-> Destroy 순으로 사용

						핑 수신 및 타임아웃 시, 등록된 윈도우 메시지를 포스팅.
						이 때, WPARAM 은 Index와 에러코드를 담는다.
						LPARAM은 PingRequest를 전송한 tick.


*/////////////////////////////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------------------------------------
#define ALL_SERVERS			0xffff
#define PING_BUFFER_LEN		4096

enum { eOpAsServer, eOpAsClient };

#define MAKE_WPARAM(index, errorcode)	MAKELONG(index, errorcode)
#define GET_PING_INDEX(val)				LOWORD(val)
#define GET_PING_ERROR(val)				HIWORD(val)

struct _PingPacket  
{
	char			szType[4];
	LONGLONG		lPerfCount; // in ms.
};

//--------------------------------------------------------------------------------------
inline SOCKET CreateUdpSocket()
{
	return socket(AF_INET, SOCK_DGRAM, IPPROTO_IP); 
}

inline void CloseSocket(SOCKET Sock)
{
	closesocket(Sock);
}

inline DWORD MakeIP(LPCTSTR szIP)
{
	if(szIP == NULL || ::_tcslen(szIP) == 0)
		return INADDR_ANY;

#ifdef _UNICODE
	char szName[256];
	::ZeroMemory( szName, sizeof(szName) );

	::WideCharToMultiByte( CP_ACP, 0, szIP, -1, szName, 256, NULL, NULL );
	DWORD dwIP = inet_addr( szName );
#else
	DWORD dwIP = ::inet_addr(szIP);
#endif	

	if(dwIP == INADDR_NONE)
	{
#ifdef _UNICODE
		PHOSTENT lphost = ::gethostbyname(szName);
#else
		PHOSTENT lphost = ::gethostbyname(szIP);
#endif
		ASSERT(lphost != NULL);

		dwIP = ((LPIN_ADDR)lphost->h_addr)->s_addr;
	}	

	return dwIP;		
}

inline int BindSocket(SOCKET Sock, DWORD dwIP, unsigned short usPort)
{
	SOCKADDR_IN	si_addr;
	
	ZeroMemory(&si_addr, sizeof(SOCKADDR_IN));
	si_addr.sin_family		= AF_INET;
	si_addr.sin_port		= htons(usPort);
	si_addr.sin_addr.s_addr = dwIP;

	return bind(Sock, (struct sockaddr *) &si_addr, sizeof(si_addr));
}

inline int SendRequest(SOCKET Sock, SOCKADDR_IN* pAddr, LONGLONG lPerfCount)
{
	_PingPacket Packet;
	Packet.szType[0]	= 'R';
	Packet.szType[1]	= 'E';
	Packet.szType[2]	= 'Q';
	Packet.szType[3]	= 0;
	Packet.lPerfCount   = lPerfCount;

	return sendto(Sock, (const char*)&Packet, sizeof(Packet), 0, (SOCKADDR*)pAddr, sizeof(SOCKADDR_IN));
}

inline int SendAcknowledge(SOCKET Sock, SOCKADDR_IN* pAddr, LONGLONG lPerfCount)
{
	_PingPacket Packet;
	Packet.szType[0]	= 'A';
	Packet.szType[1]	= 'C';
	Packet.szType[2]	= 'K';
	Packet.szType[3]	= 0;
	Packet.lPerfCount	= lPerfCount;

	return sendto(Sock, (const char*)&Packet, sizeof(Packet), 0, (SOCKADDR*)pAddr, sizeof(SOCKADDR_IN));
}

//----------------------------------------------------------------------
class CUdpPingBase
{
public:
	CUdpPingBase() : m_nOpMode(0) {};
	virtual ~CUdpPingBase(){};
	
	void	SetOpMode(int nOpCode)	{ m_nOpMode = nOpCode;	} 
	int		GetOpMode()				{ return m_nOpMode;		}

protected:
	int		m_nOpMode;
};

//--------------------------------------------------------------------------------------------------
class CUdpPingServer : public CUdpPingBase
{
public:
	CUdpPingServer();
	~CUdpPingServer();

	void	Create(std::tstring& szIP, unsigned short usPort);
	void	Destroy();

	BOOL	StartPingSerivce();
	BOOL	StopPingService();

	// Create -> Start -> Stop -> Destroy... 의 순으로 진행되겠지?

	DWORD	fnUdpPing(); // Worker Thread Function.

protected:
	SOCKET	m_Sock;
	HANDLE	m_hThread;
	BOOL	m_bSuspended;
};


//--------------------------------------------------------------------------------------------------
class CUdpPingClient : public CUdpPingBase
{
public:
	struct UdpPingDestination
	{
		UdpPingDestination() 
		{ 
			m_wIndex = (WORD)-1; 
			ZeroMemory(&m_Addr, sizeof(m_Addr)); 
		}

		void			Init(WORD wIndex, std::tstring& szDestIP, unsigned short usDestPort)
		{
			m_wIndex				= wIndex;
			m_Addr.sin_family		= AF_INET;
			m_Addr.sin_addr.s_addr	= MakeIP(szDestIP.c_str());
			m_Addr.sin_port			= htons(usDestPort);
		}

		SOCKADDR_IN* const	GetDestination()	{ return &m_Addr;	}
		WORD const			GetIndex()			{ return m_wIndex; }

		WORD			m_wIndex;
		SOCKADDR_IN		m_Addr;
	};

	CUdpPingClient();
	~CUdpPingClient();

	void	Create(HWND hNotifyWnd, UINT nMsgID);
	void	Destroy();

	WORD	AddPingDestination(WORD wIndex, std::tstring& szDestIP, unsigned short usDestPort);
	void	RemovePingDestination(WORD wIndex);

	void	SendPingRequest(WORD wIndex = ALL_SERVERS);	

	DWORD	fnUdpPing();	// Worker Thread Function.

private:
	WORD	GetDestIndex(const SOCKADDR_IN* pAddr);
	void	SendPingRequest(UdpPingDestination& Dest);

	void	GetFrequency() { m_bUsePerfCounter = QueryPerformanceFrequency((_LARGE_INTEGER*)&m_lFrequency); }
	LONGLONG	GetPerformanceCounter() // ms 단위로 제공한다.
	{
		if(m_bUsePerfCounter)
		{
			LONGLONG lCounterNow;
			QueryPerformanceCounter((_LARGE_INTEGER*)&lCounterNow); 
			return lCounterNow;
		//	return (lCounterNow / m_lFrequency * 1000);
		}
		else
		{
			return GetTickCount64();
		}
	}

	HWND	m_hNotifyWnd;
	UINT	m_nMsgID;
	
	SOCKET	m_Sock;
	HANDLE	m_hThread;
	BOOL	m_bUsePerfCounter;
	LONGLONG m_lFrequency;

	std::map<WORD, UdpPingDestination> m_PingDestMap; 
};
