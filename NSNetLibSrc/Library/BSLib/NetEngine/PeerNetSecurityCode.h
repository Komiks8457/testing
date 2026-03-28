#pragma once

typedef std::vector<WORD> VECTOR_SECURITY_CODES;
class CPeerNetSecurity
{
public:
	CPeerNetSecurity()
	{
		Reset();
	}

	~CPeerNetSecurity(){}

protected:	
	VECTOR_SECURITY_CODES	m_vectorSecurityCodes;
	BYTE					m_btMaxPeerCount;

public:
	void	Create( BYTE max_peer_count )
	{
		m_btMaxPeerCount = max_peer_count;
		m_vectorSecurityCodes.resize( (max_peer_count+1) * (max_peer_count+1) );

		//reset and create security codes
		std::fill( m_vectorSecurityCodes.begin(), m_vectorSecurityCodes.end(), (WORD)0xffff );
		
		BYTE i, j;
		for( j = 0 ; j < m_btMaxPeerCount+1 ; ++j )
		{
			for( i = 0 ; i < m_btMaxPeerCount+1 ; ++i )
			{
				if( i == j )	
					continue;

				m_vectorSecurityCodes[ j*(m_btMaxPeerCount+1) + i ] = ::rand() % 0xffff;
			}
		}
	}

	void	Create( BYTE max_peer_count, CMsg& msg )
	{
		m_btMaxPeerCount = max_peer_count;
		m_vectorSecurityCodes.resize( (max_peer_count+1) * (max_peer_count+1) );

		size_t total_count = m_vectorSecurityCodes.size();
		for( size_t i = 0 ; i < total_count ; ++i )
		{
			msg >> m_vectorSecurityCodes[ i ];
		}
	}

	void	Reset()
	{
		m_btMaxPeerCount = 0;
		m_vectorSecurityCodes.clear();
	}

	inline BOOL	GetSecurityCode( BYTE src, BYTE target, WORD& result )
	{
		if( m_btMaxPeerCount < src || m_btMaxPeerCount < target )
			return FALSE;

		result = m_vectorSecurityCodes[ target*(m_btMaxPeerCount+1) + src ];

		return TRUE;
	}

	inline void SetSecurityCode( BYTE src, BYTE target, WORD value )
	{
		_ASSERT( m_btMaxPeerCount >= src || m_btMaxPeerCount >= target );
		m_vectorSecurityCodes[ target*(m_btMaxPeerCount+1) + src ] = value;
	}

	void PushData( CMsg& msg )
	{
		size_t total_count = m_vectorSecurityCodes.size();		
		for( size_t i = 0 ; i < total_count ; ++i )
		{
			msg << m_vectorSecurityCodes[ i ];
		}
	}

	void PopData( CMsg& msg )
	{
		WORD total_count = msg;
		m_vectorSecurityCodes.resize( total_count );

		for( int i = 0 ; i < total_count ; ++i )
		{
			msg >> m_vectorSecurityCodes[ i ];
		}
	}

	void PushData( BYTE session, CMsg& msg )
	{
		_ASSERT( session > 0 && session <= m_btMaxPeerCount );

		WORD code1 = 0, code2 = 0; 
		GetSecurityCode(0,session, code1);
		GetSecurityCode(session,0, code2);
		msg << code1 << code2;

		BYTE src, target;
		//active first
		for( src = session, target = 1; target <= m_btMaxPeerCount ; ++target )
		{
			if( src == target )
				continue;

			WORD code = 0; 
			GetSecurityCode( src, target, code);
			msg << code;
		}

		//passive later
		for( target = session, src = 1 ; src <= m_btMaxPeerCount ; ++src )
		{
			if( src == target )
				continue;

			WORD code = 0; 
			GetSecurityCode( src, target, code);
			msg << code;
		}
	}

	void PopData( BYTE session, CMsg& msg )
	{
		_ASSERT( session > 0 && session <= m_btMaxPeerCount );

		WORD value;

		msg >> value;
		SetSecurityCode( 0, session, value );
		msg >> value;
		SetSecurityCode( session, 0, value );

		BYTE src, target;
		//active first
		for( src = session, target = 1; target <= m_btMaxPeerCount ; ++target )
		{
			if( src == target )
				continue;

			msg >> value;
			SetSecurityCode( src, target, value );
		}

		//passive later
		for( target = session, src = 1 ; src <= m_btMaxPeerCount ; ++src )
		{
			if( src == target )
				continue;

			msg >> value;
			SetSecurityCode( src, target, value );
		}
	}
};

typedef HASH_MAP<DWORD,CPeerNetSecurity*> HASH_GROUP_SECURITY_CODES;

class CPeerNetSecurityCodeMgrForServer
{
public:
	CPeerNetSecurityCodeMgrForServer(){}
	~CPeerNetSecurityCodeMgrForServer(){}

protected:
	HASH_GROUP_SECURITY_CODES			m_hashGroupSecurityCodes;
	ChunkAllocatorST<CPeerNetSecurity> m_poolSecurityCodes;

public:
	CPeerNetSecurity* CreatePeerGroupCodes( DWORD dwGroupID, BYTE max_peer_count )
	{
		if( GetGroupSecurity(dwGroupID) != NULL )
		{
			_ASSERT( FALSE );
			return NULL;
		}

		CPeerNetSecurity* pCodes = m_poolSecurityCodes.NewItem();
		pCodes->Create( max_peer_count );

		m_hashGroupSecurityCodes.insert( HASH_GROUP_SECURITY_CODES::value_type(dwGroupID,pCodes) );
		return pCodes;
	}

	void	CreatePeerGroupCodes( DWORD dwGroupID, BYTE max_peer_count, CMsg& msg )
	{
		if( GetGroupSecurity(dwGroupID) != NULL )
			return;

		CPeerNetSecurity* pCodes = m_poolSecurityCodes.NewItem();
		pCodes->Create( max_peer_count, msg );
		m_hashGroupSecurityCodes.insert( HASH_GROUP_SECURITY_CODES::value_type(dwGroupID,pCodes) );
	}

	bool GetSecurityCode( DWORD dwGroupID, BYTE src, BYTE target, WORD& result )
	{
		CPeerNetSecurity* pSecurity = GetGroupSecurity( dwGroupID );
		if( pSecurity == NULL )
			return false;

		pSecurity->GetSecurityCode( src, target, result );

		return true;
	}

	bool	GetSecurityCodeCouple( DWORD peer_group_id, BYTE session_1, BYTE session_2, WORD& one_to_two, WORD& two_to_one )
	{
		CPeerNetSecurity* pSecurity = GetGroupSecurity( peer_group_id );
		if( pSecurity == NULL )
			return false;
		
		pSecurity->GetSecurityCode( session_1, session_2, one_to_two );
		pSecurity->GetSecurityCode( session_2, session_1, two_to_one );
		return true;
	}

	void PushData( DWORD dwGroupID, CMsg& msg )
	{
		CPeerNetSecurity* pSecurity = GetGroupSecurity( dwGroupID );
		if( pSecurity == NULL )
		{
			_ASSERT( FALSE );
			return;
		}

		pSecurity->PushData( msg );
	}

	void PopData( DWORD dwGroupID, CMsg& msg )
	{
		CPeerNetSecurity* pSecurity = GetGroupSecurity( dwGroupID );
		if( pSecurity == NULL )
		{
			_ASSERT( FALSE );
			return;
		}

		pSecurity->PopData( msg );
	}

	void PushData( DWORD dwGroupID, BYTE session, CMsg& msg )
	{
		CPeerNetSecurity* pSecurity = GetGroupSecurity( dwGroupID );
		if( pSecurity == NULL )
		{
			_ASSERT( FALSE );
			return;
		}

		pSecurity->PushData( session, msg );
	}

	void	CloseGroupCode( DWORD dwGroupID )
	{
		CPeerNetSecurity* pSecurity = GetGroupSecurity( dwGroupID, TRUE );
		if( pSecurity == NULL )
			return;

		pSecurity->Reset();
		m_poolSecurityCodes.FreeItem( pSecurity );
	}

	CPeerNetSecurity* GetGroupSecurity( DWORD dwGroupID, BOOL bPop = FALSE )
	{
		HASH_GROUP_SECURITY_CODES::iterator it = m_hashGroupSecurityCodes.find( dwGroupID );
		if( it == m_hashGroupSecurityCodes.end() )
			return NULL;
		
		CPeerNetSecurity* pSecurity = (*it).second;

		if( bPop == TRUE )
			m_hashGroupSecurityCodes.erase( it );

		return pSecurity;
	}
};


class CPeerNetSecurityCodeMgrForClient
{
public:
	CPeerNetSecurityCodeMgrForClient()
	{
	}
	~CPeerNetSecurityCodeMgrForClient()
	{
	}

protected:
	CPeerNetSecurity m_SecurityCodes;

public:
	void Create( BYTE max_peer_count )
	{
		m_SecurityCodes.Create( max_peer_count );
	}

	void PopData( BYTE session, CMsg& msg )
	{
		m_SecurityCodes.PopData( session, msg );
	}

	inline BOOL GetSecurityCode( BYTE src, BYTE target, WORD& result )
	{
		return m_SecurityCodes.GetSecurityCode( src, target, result );
	}
};