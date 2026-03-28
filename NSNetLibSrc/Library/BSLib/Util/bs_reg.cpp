#include "stdafx.h"
#include "bs_reg.h"

BOOL ClsRegKey::IsNT()
{	
	OSVERSIONINFO vi = { sizeof(vi)};	
	::GetVersionEx(&vi);
	return vi.dwPlatformId == VER_PLATFORM_WIN32_NT;
}



ClsRegKey::ClsRegKey()
{
	m_hKey = NULL;
}



ClsRegKey::~ClsRegKey()
{
	CloseKey();
}



LONG ClsRegKey::OpenKey( HKEY hKey, LPCTSTR pszSubKey, REGSAM samDesired /* = KEY_ALL_ACCESS */ )
{
	_ASSERT( m_hKey == NULL );	
	return ::RegOpenKeyEx( hKey, pszSubKey, 0, samDesired, &m_hKey );
}



LONG ClsRegKey::CreateKey( HKEY hKey, LPCTSTR pszSubKey, LPTSTR lpClass /* = REG_NONE */, DWORD dwOptions /* = REG_OPTION_NON_VOLATILE */, REGSAM samDesired /* = KEY_ALL_ACCESS */, LPSECURITY_ATTRIBUTES lpSecurityAttributes /* = NULL */, LPDWORD lpdwDisposition /* = NULL */ )
{
	_ASSERT( m_hKey == NULL );	
	return ::RegCreateKeyEx( hKey, pszSubKey, 0, lpClass, dwOptions, samDesired, lpSecurityAttributes, &m_hKey, lpdwDisposition );
}


/////////////////////////////////////////////////////////////////////////
// Attach a key to the object.
/////////////////////////////////////////////////////////////////////////
BOOL ClsRegKey::Attach( HKEY hKey )
{
	_ASSERT( m_hKey == NULL );
	_ASSERT( hKey != NULL ); 
	
	if ( m_hKey == NULL )
	{
		m_hKey = hKey;
		return TRUE;
	}
	return FALSE;
}


HKEY ClsRegKey::Detach()
{
	_ASSERT( m_hKey != NULL );
	
	HKEY hResult = m_hKey;
	m_hKey = NULL;
	
	return hResult;
}



LONG ClsRegKey::CloseKey()
{
	LONG rc = ERROR_SUCCESS;

	if ( m_hKey )
	{
		rc = ::RegCloseKey( m_hKey );
		if ( rc == ERROR_SUCCESS )	
			m_hKey = NULL;
	}
	return rc;
}


/////////////////////////////////////////////////////////////////////////
// Delete a key and all of it's sub-keys.
/////////////////////////////////////////////////////////////////////////
LONG ClsRegKey::DeleteKey( LPCTSTR pszSubKey )
{
	_ASSERT( m_hKey != NULL); 
	_ASSERT( pszSubKey != NULL); 
	
	LONG lResult;
	
	if ( IsNT() )		
		lResult = ::RegDeleteKey( m_hKey, pszSubKey );
	else
	{		
		ClsRegKey key;
		if (( lResult = key.OpenKey( m_hKey, pszSubKey )) != ERROR_SUCCESS )
			return lResult;
		
		FILETIME	ftime;
		TCHAR		szKey[ MAX_PATH ];
		DWORD		dwSize = MAX_PATH;
		
		while ( ::RegEnumKeyEx( key.m_hKey, 0, szKey, &dwSize, NULL, NULL, NULL, &ftime ) == ERROR_SUCCESS )
		{			
			if (( lResult = key.DeleteKey( szKey )) != ERROR_SUCCESS )
				return lResult;
			
			dwSize = MAX_PATH;
		}
		
		key.CloseKey();		
		lResult = ::RegDeleteKey( m_hKey, pszSubKey );
	}
	return lResult;
}


///////////////////////////////////////////////////////////////////////
// Delete a value.
///////////////////////////////////////////////////////////////////////
LONG ClsRegKey::DeleteValue( LPCTSTR pszValueName )
{
	_ASSERT( m_hKey != NULL ); 
	_ASSERT( pszValueName != NULL ); 

	
	return ::RegDeleteValue( m_hKey, pszValueName );
}


///////////////////////////////////////////////////////////////////////
// Query a 32 bit value.
//////////////////////////////////////////////////////////////////////
LONG ClsRegKey::QueryValue( LPCTSTR pszValueName, DWORD& dwValue ) const
{
	_ASSERT( m_hKey != NULL ); 
	_ASSERT( pszValueName != NULL ); 
	
	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof( DWORD );

	return ::RegQueryValueEx( m_hKey, pszValueName, NULL, &dwType, ( LPBYTE )&dwValue, &dwSize );
}

//////////////////////////////////////////////////////////////////////
// Query a 0-terminated string.
//////////////////////////////////////////////////////////////////////
LONG ClsRegKey::QueryValue( LPCTSTR pszValueName, LPTSTR pszValue, LPDWORD lpdwCount ) const
{
	_ASSERT( m_hKey != NULL ); 
	_ASSERT( pszValueName != NULL ); 
	
	DWORD dwType = REG_SZ;

	return ::RegQueryValueEx( m_hKey, pszValueName, NULL, &dwType, ( LPBYTE )pszValue, lpdwCount );
}



//////////////////////////////////////////////////////////////////////////
// Query a byte array.
//////////////////////////////////////////////////////////////////////////
LONG ClsRegKey::QueryValue( LPCTSTR pszValueName, LPBYTE pValue, LPDWORD lpdwSize ) const
{
	_ASSERT( m_hKey != NULL ); 
	_ASSERT( pszValueName != NULL ); 
	
	DWORD dwType = REG_NONE;	
	return ::RegQueryValueEx( m_hKey, pszValueName, NULL, &dwType, pValue, lpdwSize );
}

////////////////////////////////////////////////////////////////////////
// Set a 32 bit value.
////////////////////////////////////////////////////////////////////////
LONG ClsRegKey::SetValue( DWORD dwValue, LPCTSTR pszValueName /* = NULL */ )
{
	_ASSERT( m_hKey != NULL ); 

	return ::RegSetValueEx( m_hKey, pszValueName, 0, REG_DWORD, ( LPBYTE )&dwValue, sizeof( DWORD ));
}

////////////////////////////////////////////////////////////////////////
// Set a 0-terminated string.
////////////////////////////////////////////////////////////////////////
LONG ClsRegKey::SetValue( LPCTSTR pszValue, LPCTSTR pszValueName /* = NULL */ )
{
	_ASSERT( m_hKey != NULL ); 
	_ASSERT( pszValue != NULL ); 
	
	return ::RegSetValueEx( m_hKey, pszValueName, 0, REG_SZ, ( LPBYTE )pszValue, ( DWORD )_tcslen( pszValue ) + 1 );
}

//////////////////////////////////////////////////////////////////////////
// Set a byte array.
//////////////////////////////////////////////////////////////////////////
LONG ClsRegKey::SetValue( LPBYTE pValue, DWORD dwSize, LPCTSTR pszValueName /* = NULL */ )
{
	_ASSERT( m_hKey != NULL ); 
	_ASSERT( pValue != NULL ); 

	return ::RegSetValueEx( m_hKey, pszValueName, 0, REG_NONE, pValue, dwSize );
}