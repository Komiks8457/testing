#include "stdafx.h"
#include "KeyExchange.h"
#include <objbase.h>

//---------------------------------------------------------------------------------------------------
CKeyExchangeBase::CKeyExchangeBase()
{
	m_n		= 0;
	m_g		= 0;
	m_Ka	= 0;
	m_Kb	= 0;
	m_K		= 0;
	ZeroMemory(m_Br, 8);
}

CKeyExchangeBase::~CKeyExchangeBase()
{
}

void CKeyExchangeBase::EncodeBlowfishKey(BYTE Key[], DWORD BK, BYTE Sub)
{
	LPBYTE pBK = (LPBYTE)&BK;
	for(int i = 0; i < 8; i++)
	{
		Key[i] ^= BYTE(Key[i] + *(pBK + i%4) + Sub);
	}
}

void CKeyExchangeBase::SetInnerBlowFishKey(DWORD K, DWORD Ka, DWORD Kb)
{
	_DHBlowfishKey BlowfishKey;
	CopyMemory(BlowfishKey.Key,		&Ka, sizeof(DWORD));
	CopyMemory(BlowfishKey.Key+4,	&Kb, sizeof(DWORD));
	EncodeBlowfishKey(BlowfishKey.Key, K, BYTE(K%4));

	m_BF.Initialize(BlowfishKey.Key, 8);
}

void CKeyExchangeBase::CreateSignature(_DHSignature& Signature, DWORD K, DWORD K1, DWORD K2)
{
	// 시그니처 만들자. 심플하게 만들어봤다.
	CopyMemory(Signature.S,		&K1, sizeof(DWORD));
	CopyMemory(Signature.S+4,	&K2, sizeof(DWORD));
	EncodeBlowfishKey(Signature.S, K, BYTE(K1 % 8));

	// 암호화하자... ^^
	m_BF.Encode((LPBYTE)&Signature, (LPBYTE)&Signature, 8);
}

bool CKeyExchangeBase::VerifySignature(_DHSignature& Signature, DWORD K, DWORD K1, DWORD K2)
{
	_DHSignature TempSignature;
	ZeroMemory(&TempSignature, sizeof(_DHSignature));

	CreateSignature(TempSignature, K, K1, K2);

	return (0 == memcmp(&Signature, &TempSignature, sizeof(_DHSignature)));
}

void CKeyExchangeBase::GetEncryptionKey(_DHBlowfishKey& OUT Key)
{
	CopyMemory(Key.Key, m_Br, 8);
	EncodeBlowfishKey(Key.Key, m_K);
}


//---------------------------------------------------------------------------------------------------
CKeySender::CKeySender()
{
}

void CKeySender::MakeSenderInfo(_DHSenderInfo& OUT SenderInfo)
{
	// Br을 생성하자.
	GUID RandomNumber;
	CoCreateGuid(&RandomNumber);
	CopyMemory(m_Br, RandomNumber.Data4, 8);

	// g, n, Ka 생성
	m_DH.CreateKeys(m_g, m_n);
	m_DH.CreateSenderInterKey(m_Ka);

	// SenderInfo setting!
	CopyMemory(SenderInfo.Br, m_Br, 8);
	SenderInfo.g = m_g;
	SenderInfo.n = m_n;
	SenderInfo.Ka = m_Ka;
}

bool CKeySender::SetRecipientInfo(_DHRecipientInfo& IN RecipientInfo)
{
	m_Kb = RecipientInfo.Kb;

	// DH Key 구하자.
	m_DH.CreateSenderEncryptionKey(m_K, m_Kb);

	// BF 초기화.
	SetInnerBlowFishKey(m_K, m_Ka, m_Kb);

	// Recipient Signature 검증
	return VerifySignature(RecipientInfo.Sb, m_K, m_Kb, m_Ka);
}

void CKeySender::MakeSenderSignature(_DHSignature& OUT SenderSignature)
{
	CreateSignature(SenderSignature, m_K, m_Ka, m_Kb);
}

//---------------------------------------------------------------------------------------------------
CKeyRepicient::CKeyRepicient()
{
}

void CKeyRepicient::SetSenderInfo(_DHSenderInfo& IN SenderInfo)
{
	m_g = SenderInfo.g;
	m_n = SenderInfo.n;
	m_Ka = SenderInfo.Ka;
	CopyMemory(m_Br, SenderInfo.Br, 8);

	// Kb 구하자.
	m_DH.CreateRecipientInterKey(m_Kb, m_g, m_n);

	// DH Key 구하자.
	m_DH.CreateRecipientEncryptionKey(m_K, m_Ka);

	// BF 초기화.
	SetInnerBlowFishKey(m_K, m_Ka, m_Kb);
}

void CKeyRepicient::MakeRecipientInfo(_DHRecipientInfo& OUT RecipientInfo)
{
	RecipientInfo.Kb = m_Kb;
	CreateSignature(RecipientInfo.Sb, m_K, m_Kb, m_Ka);
}

bool CKeyRepicient::VerifySenderSignature(_DHSignature& IN SenderSignature)
{
	// Sender Signature 검증
	return VerifySignature(SenderSignature, m_K, m_Ka, m_Kb);
}

