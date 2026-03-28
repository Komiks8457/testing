#pragma once

#include <string>
#include <vector>
#include <WinInet.h>

/*/////////////////////////////////////////////////////////////////////////////////

	한가지만 주의해 주시기 바랍니다.. 
	CreateFtpSession(LPCTSTR lpszFullUrl) 함수의 인자인 lpszFullUrl은 
	ftp 서버의 주소와 포트, 계정, 비번을 포함한 말 그대로 full url이어야 합니다.
	Full url이 아니라면 다른 녀석을 사용해주시기 바랍니다.

/////////////////////////////////////////////////////////////////////////////////*/

////////////////////////////////////////////////////////////////////////////////////
class CSimpleFTP
{
public:
	CSimpleFTP() : m_hInet(NULL), m_hSession(NULL) {}
	virtual ~CSimpleFTP() { DestroyFtpSession(); }

	// session related!
	BOOL		CreateFtpSession(LPCTSTR lpszFullUrl, BOOL bPassive = FALSE);
	BOOL		CreateFtpSession(LPCTSTR lpszHostName, int nPort, LPCTSTR lpszUserName, LPCTSTR lpszPassword, BOOL bPassive = FALSE);
	void		DestroyFtpSession();

	// directory operations
	BOOL		CreateDirectory(LPCTSTR lpszDir);
	BOOL		DeleteDirectory(LPCTSTR lpszDir);
	BOOL		SetCurrentDirectory(LPCTSTR lpszDir);
	BOOL		GetCurrentDirectory(std::tstring& OUT strDir);

	// ftp file operations
	BOOL		DeleteFile(LPCTSTR lpszFilename);
	BOOL		RenameFile(LPCTSTR lpszExisting, LPCTSTR lpszNew);

	// transfering operations
	BOOL		FindFiles(LPCTSTR lpszSearchFile, OUT std::vector<std::tstring>& vecFileNames);
	BOOL		UploadMemoryFile(LPCTSTR lpszFilename, LPBYTE pMem, DWORD dwSize);
	BOOL		UploadFile(LPCTSTR lpszLocalFile, LPCTSTR lpszRemoteFile);
	BOOL		DownloadFile(LPCTSTR lpszLocalFile, LPCTSTR lpszRemoteFile, BOOL bBinaryMode = TRUE);

private:
	HINTERNET	m_hInet;
	HINTERNET	m_hSession;
};
////////////////////////////////////////////////////////////////////////////////////

/*

// NON-UNICODE의 예제 코드입니다... :)

{
  	LPBYTE pMem = new BYTE[10240000];
	for(int i = 0; i < 10240000; i++)
		pMem[i] = i % 256;

	CSimpleFTP Uploader;

	VERIFY(Uploader.CreateFtpSession(("ftp://joymax:169841@nProtect.joymax.co.kr/"));

	std::tstring szCurDir;
	VERIFY(Uploader.GetCurrentDirectory(szCurDir));

	OutputDebugString("Current Directory: ");
	OutputDebugString(szCurDir.c_str());
	OutputDebugString("\n");

	VERIFY(Uploader.CreateDirectory("TestDirectory"));
	VERIFY(Uploader.SetCurrentDirectory("TestDirectory"));
	VERIFY(Uploader.GetCurrentDirectory(szCurDir));

	OutputDebugString("Current Directory: ");
	OutputDebugString(szCurDir.c_str());
	OutputDebugString("\n");

	VERIFY(Uploader.UploadMemoryFile("testfile_Huge.bin", pMem, 10240000));
	VERIFY(Uploader.UploadFile("d:/ftp_test/FTPTransferDlg_src.zip", "FTPTransferDlg_src.zip"));
	VERIFY(Uploader.RenameFile("FTPTransferDlg_src.zip", "renamed.zip"));
	VERIFY(Uploader.DownloadFile("d:/ftp_test/renamed.zip", "renamed.zip"));
	VERIFY(Uploader.DeleteFile("renamed.zip"));
	VERIFY(Uploader.DeleteFile("testfile_Huge.bin"));

	VERIFY(Uploader.SetCurrentDirectory(".."));
	VERIFY(Uploader.GetCurrentDirectory(szCurDir));
	OutputDebugString("Current Directory: ");
	OutputDebugString(szCurDir.c_str());
	OutputDebugString("\n");

	VERIFY(Uploader.DeleteDirectory("TestDirectory"));

	delete [] pMem;
}

*/