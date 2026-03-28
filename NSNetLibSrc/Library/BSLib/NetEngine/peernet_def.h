#pragma once

#define PEER_NET_SERVER_SESSION_ID	(BYTE)0

#define PEER_NET_PORT_SERVER		(WORD)5001
#define PEER_NET_PORT_PEER			(WORD)5005
#define PEER_NET_PORT_DYNACMIC		(WORD)0

enum PeerNetRole
{
	ROLE_None,
	ROLE_DumbProxy,
	ROLE_SimulatedProxy,
	ROLE_AutonomousProxy,
	ROLE_Authority
};

struct PeerSessionContext
{
	WORD			m_wAssocServerID;
	DWORD			m_dwUserJID;

	SOCKADDR_IN		m_addr;	
	SOCKADDR_IN		m_addrPrivate;
	
	DWORD			m_dwLatestActivityTime;
	
	PeerSessionContext()
	{
		Reset();
	}

	void Reset()
	{		
		m_wAssocServerID = 0;
		m_dwUserJID = 0;

		::ZeroMemory( &m_addr, sizeof(m_addr) );
		::ZeroMemory( &m_addrPrivate, sizeof(m_addrPrivate) );

		m_dwLatestActivityTime = 0;
	}

	/*PeerSessionContext& operator = (PeerSessionContext& src )
	{
		m_wAssocServerID = src.m_wAssocServerID;
		m_dwUserJID = src.m_dwUserJID;

		::memcpy( &m_addr, &src.m_addr, sizeof(m_addr) );
		::memcpy( &m_addrPrivate, &src.m_addrPrivate, sizeof(m_addr) );
		
		m_dwLatestActivityTime = src.m_dwLatestActivityTime;

		return (*this);
	}*/

	bool IsNAT(){ return (bool)(m_addr.sin_addr.s_addr != m_addrPrivate.sin_addr.s_addr); }

	template <class T>
	void PushData( T& msg )
	{
		msg << m_wAssocServerID << m_dwUserJID;
		
		msg.WriteBytes( &m_addr, sizeof(m_addr) );
		msg.WriteBytes( &m_addrPrivate, sizeof(m_addrPrivate) );
	}

	template <class T>
	void PopData( T& msg )
	{
		msg >> m_wAssocServerID >> m_dwUserJID;
		
		msg.ReadBytes( &m_addr, sizeof(m_addr) );
		msg.ReadBytes( &m_addrPrivate, sizeof(m_addrPrivate) );
	}
};