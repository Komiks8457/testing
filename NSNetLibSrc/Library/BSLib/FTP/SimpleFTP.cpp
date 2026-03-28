#include "stdafx.h"
#include "SimpleFTP.h"
#pragma comment (lib, "WinInet.lib")



BOOL CSimpleFTP::CreateFtpSession(LPCTSTR lpszFullUrl, BOOL bPassive)
{
	m_hInet = InternetOpen( _T("SimpleFTPUploader"), 
							PRE_CONFIG_INTERNET_ACCESS, 
							NULL, 
							INTERNET_INVALID_PORT_NUMBER, 
							0);
	if(m_hInet == NULL)
	{
		PutLog(LOG_FATAL, _T("InternetOpen Failed (%d)\n"), GetLastError());
		return FALSE;
	}

	URL_COMPONENTS UrlComponent;
	ZeroMemory(&UrlComponent, sizeof(UrlComponent));

	TCHAR tszScheme[256];
	TCHAR tszHostName[256];
	TCHAR tszUserName[256];
	TCHAR tszPassword[256];
	TCHAR tszUrlPath[256];
	TCHAR tszExtraInfo[256];

	UrlComponent.dwStructSize		= sizeof(URL_COMPONENTS);
	UrlComponent.lpszScheme			= tszScheme;
	UrlComponent.dwSchemeLength		= 256;
	UrlComponent.nScheme			= INTERNET_SCHEME_FTP;
	UrlComponent.lpszHostName		= tszHostName;
	UrlComponent.dwHostNameLength	= 256;
	UrlComponent.nPort				= 0;
	UrlComponent.lpszUserName		= tszUserName;
	UrlComponent.dwUserNameLength	= 256;
	UrlComponent.lpszPassword		= tszPassword;
	UrlComponent.dwPasswordLength	= 256;
	UrlComponent.lpszUrlPath		= tszUrlPath;
	UrlComponent.dwUrlPathLength	= 256;
	UrlComponent.lpszExtraInfo		= tszExtraInfo;
	UrlComponent.dwExtraInfoLength	= 256;

	if(!InternetCrackUrl(lpszFullUrl, lstrlen(lpszFullUrl), 0, &UrlComponent))
	{
		PutLog(LOG_FATAL, _T("InternetCrackUrl Failed (%d)\n"), GetLastError());
		return FALSE;
	}

	m_hSession = InternetConnect(m_hInet, 
								UrlComponent.lpszHostName,
								UrlComponent.nPort,
								UrlComponent.lpszUserName,
								UrlComponent.lpszPassword,
								INTERNET_SERVICE_FTP,
								(bPassive ? INTERNET_FLAG_PASSIVE : 0),
								0);
	if(m_hSession == NULL)
	{
		PutLog(LOG_FATAL, _T("InternetConnect[%s] Failed (%d)\n"), lpszFullUrl, GetLastError());
		return FALSE;
	}

	return TRUE;
}

BOOL CSimpleFTP::CreateFtpSession(LPCTSTR lpszHostName, int nPort, LPCTSTR lpszUserName, LPCTSTR lpszPassword, BOOL bPassive)
{
	m_hInet = InternetOpen( _T("SimpleFTPUploader"), 
							PRE_CONFIG_INTERNET_ACCESS, 
							NULL, 
							INTERNET_INVALID_PORT_NUMBER, 
							0);
	if(m_hInet == NULL)
	{
		PutLog(LOG_FATAL, _T("InternetOpen Failed (%d)\n"), GetLastError());
		return FALSE;
	}

	m_hSession = InternetConnect(m_hInet, 
								lpszHostName,
								(INTERNET_PORT)nPort,
								lpszUserName,
								lpszPassword,
								(DWORD)INTERNET_SERVICE_FTP,
								(bPassive ? INTERNET_FLAG_PASSIVE : 0),
								0);
	if(m_hSession == NULL)
	{
		PutLog(LOG_FATAL, _T("InternetConnect[%s] Failed (%d)\n"), lpszHostName, GetLastError());
		return FALSE;
	}

	return TRUE;
}

void CSimpleFTP::DestroyFtpSession()
{	
	if(m_hSession)
	{
		InternetCloseHandle(m_hSession);
		m_hSession = NULL;
	}

	if(m_hInet)
	{
		InternetCloseHandle(m_hInet);
		m_hInet = NULL;
	}
}

BOOL CSimpleFTP::SetCurrentDirectory(LPCTSTR lpszDir)
{
	if(m_hSession == NULL)
		return FALSE;

	if(!FtpSetCurrentDirectory(m_hSession, lpszDir))
	{
		DWORD dwErr = GetLastError();

		TCHAR tszInternetErr[256];
		DWORD dwErrStrLen = 256;
		InternetGetLastResponseInfo(&dwErr, tszInternetErr, &dwErrStrLen);

		PutLog(LOG_FATAL, _T("FtpSetCurrentDirectory[Dir: %s] Failed (%d, %s)\n"), lpszDir, dwErr, tszInternetErr);
		return FALSE;
	}

	return TRUE;
}

BOOL CSimpleFTP::GetCurrentDirectory(std::tstring& OUT strDir)
{
	if(m_hSession == NULL)
		return FALSE;

	TCHAR szCurrentDir[1024] = _T("");
	DWORD dwDirLen = 1024;

	if(!FtpGetCurrentDirectory(m_hSession, szCurrentDir, &dwDirLen))
	{
		DWORD dwErr = GetLastError();

		TCHAR tszInternetErr[256];
		DWORD dwErrStrLen = 256;
		InternetGetLastResponseInfo(&dwErr, tszInternetErr, &dwErrStrLen);

		PutLog(LOG_FATAL, _T("FtpGetCurrentDirectory Failed (%d, %s)\n"), dwErr, tszInternetErr);

		strDir = _T("");
		return FALSE;
	}

	strDir = szCurrentDir;
	return TRUE;
}

BOOL CSimpleFTP::CreateDirectory(LPCTSTR lpszDir)
{
	if(m_hSession == NULL)
		return FALSE;

	if(!FtpCreateDirectory(m_hSession, lpszDir))
	{
		DWORD dwErr = GetLastError();

		TCHAR tszInternetErr[256];
		DWORD dwErrStrLen = 256;
		InternetGetLastResponseInfo(&dwErr, tszInternetErr, &dwErrStrLen);

		PutLog(LOG_FATAL, _T("FtpCreateDirectory[Dir: %s] Failed (%d, %s)\n"), lpszDir, dwErr, tszInternetErr);
		return FALSE;
	}

	return TRUE;
}

BOOL CSimpleFTP::DeleteDirectory(LPCTSTR lpszDir)
{
	if(m_hSession == NULL)
		return FALSE;

	if(!FtpRemoveDirectory(m_hSession, lpszDir))
	{
		DWORD dwErr = GetLastError();

		TCHAR tszInternetErr[256];
		DWORD dwErrStrLen = 256;
		InternetGetLastResponseInfo(&dwErr, tszInternetErr, &dwErrStrLen);

		PutLog(LOG_FATAL, _T("FtpRemoveDirectory[Dir: %s] Failed (%d, %s)\n"), lpszDir, dwErr, tszInternetErr);
		return FALSE;
	}

	return TRUE;
}

BOOL CSimpleFTP::DeleteFile(LPCTSTR lpszFilename)
{
	if(m_hSession == NULL)
		return FALSE;

	if(!FtpDeleteFile(m_hSession, lpszFilename))
	{
		PutLog(LOG_FATAL, _T("FtpDeleteFile[File: %s] Failed (%d)\n"), lpszFilename, GetLastError());
		return FALSE;	
	}

	return TRUE;
}

BOOL CSimpleFTP::RenameFile(LPCTSTR lpszExisting, LPCTSTR lpszNew)
{
	if(m_hSession == NULL)
		return FALSE;

	if(!FtpRenameFile(m_hSession, lpszExisting, lpszNew))
	{
		PutLog(LOG_FATAL, _T("FtpRenameFile[Existing: %s, New: %s] Failed (%d)\n"), lpszExisting, lpszNew, GetLastError());
		return FALSE;	
	}

	return TRUE;
}

BOOL CSimpleFTP::FindFiles(LPCTSTR lpszSearchFile, OUT std::vector<std::tstring>& vecFileNames)
{
	HINTERNET		hFileConnection;
	WIN32_FIND_DATA sWFD; 

	hFileConnection = FtpFindFirstFile(	m_hSession,
										lpszSearchFile,
										&sWFD,
										0,
										0);

	if(hFileConnection == (HINTERNET)NULL)
	{
		DWORD dwError = GetLastError();
		if(ERROR_NO_MORE_FILES == dwError)
			return TRUE;

		PutLog(LOG_FATAL, _T("FtpFindFirstFile[search for: %s] Failed (%d)\n"), lpszSearchFile, dwError);
		return FALSE;
	}

	for(;;)
	{
		vecFileNames.push_back(sWFD.cFileName);

		if(!InternetFindNextFile(hFileConnection, &sWFD))
		{
			DWORD dwError = GetLastError();

			InternetCloseHandle(hFileConnection);

			if(ERROR_NO_MORE_FILES == dwError)
				return TRUE;

			PutLog(LOG_FATAL, _T("InternetFindNextFile[search for: %s] Failed (%d)\n"), lpszSearchFile, dwError);
			return FALSE;
		}
	}
}

BOOL CSimpleFTP::UploadMemoryFile(LPCTSTR lpszFilename, LPBYTE pMem, DWORD dwSize)
{
	HINTERNET	hInetFile = NULL;
	BOOL		bRet = FALSE;

	__try
	{
		if(m_hSession == NULL)
			__leave;

		hInetFile = FtpOpenFile(m_hSession, 
								lpszFilename, 
								GENERIC_WRITE, 
								FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, 
								0);
		if(hInetFile == NULL)
		{
			PutLog(LOG_FATAL, _T("FtpOpenFile[file: %s] Failed (%d)\n"), lpszFilename, GetLastError());
			__leave;
		}		

		DWORD	dwNumOfBytesWritten = 0;
		LPBYTE	pBuf = pMem;
		DWORD	dwErr;
		TCHAR	tszInternetErr[256] = _T("");
		DWORD	dwErrStrLen = 256;

		for(;;)
		{
			if(!InternetWriteFile(hInetFile, pBuf, dwSize, &dwNumOfBytesWritten))
			{				
				dwErr = GetLastError();

				if(dwErr == ERROR_INTERNET_EXTENDED_ERROR)
					InternetGetLastResponseInfo(&dwErr, tszInternetErr, &dwErrStrLen);

				PutLog(LOG_FATAL, _T("InternetWriteFile[file: %s] Failed (%d, %s)\n"), lpszFilename, dwErr, tszInternetErr);
				__leave;
			}

			pBuf	+= dwNumOfBytesWritten;
			dwSize	-= dwNumOfBytesWritten;

			if(dwSize == 0)
			{
				bRet = TRUE;

			//	PutLog(LOG_FATAL, _T("File: %s successfully uploaded!!\n"), lpszFilename);
				__leave;
			}
		}
	}

	__finally
	{
		if(hInetFile)
			InternetCloseHandle(hInetFile);
	}

	return bRet;
}

BOOL CSimpleFTP::UploadFile(LPCTSTR lpszLocalFile, LPCTSTR lpszRemoteFile)
{
	if(m_hSession == NULL)
		return FALSE;

	if(!FtpPutFile(	m_hSession, 
					lpszLocalFile, 
					lpszRemoteFile, 
					INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_TRANSFER_BINARY,
					0))
	{
		PutLog(LOG_FATAL, _T("FtpPutFile[Local: %s, Remote: %s] Failed (%d)\n"), lpszLocalFile, lpszRemoteFile, GetLastError());
		return FALSE;	
	}

	return TRUE;
}

BOOL CSimpleFTP::DownloadFile(LPCTSTR lpszLocalFile, LPCTSTR lpszRemoteFile, BOOL bBinaryMode /*= TRUE*/)
{
	if(m_hSession == NULL)
		return FALSE;

	if(!FtpGetFile(	m_hSession,
					lpszRemoteFile,
					lpszLocalFile,
					FALSE,
					FILE_ATTRIBUTE_NORMAL,
					(bBinaryMode ? FTP_TRANSFER_TYPE_BINARY : FTP_TRANSFER_TYPE_ASCII) | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD, 
					0))
	{
		PutLog(LOG_FATAL, _T("FtpGetFile[Local: %s, Remote: %s] Failed (%d)\n"), lpszLocalFile, lpszRemoteFile, GetLastError());
		return FALSE;
	}

	return TRUE;
}