#pragma once

#include "DiffieHellman.h"
#include "blowfish.h"

#pragma pack(push, 1)

struct _DHSignature
{
	BYTE	S[8];
};

struct _DHSenderInfo
{
	BYTE	Br[8];
	DWORD	g;
	DWORD	n;
	DWORD	Ka;
};

struct _DHRecipientInfo
{
	DWORD			Kb;
	_DHSignature	Sb;
};

union _DHBlowfishKey
{
	BYTE			Key[8];
	ULARGE_INTEGER	lKey;
};

#pragma pack(pop)

//-------------------------------------------------------------------------------
class CKeyExchangeBase
{
public:
	CKeyExchangeBase();
	virtual ~CKeyExchangeBase();

	void	GetEncryptionKey(_DHBlowfishKey& OUT Key);

protected:

	void	SetInnerBlowFishKey(DWORD K, DWORD Ka, DWORD Kb);
	void	CreateSignature(_DHSignature& Signature, DWORD K, DWORD K1, DWORD K2);
	bool	VerifySignature(_DHSignature& Signature, DWORD K, DWORD K1, DWORD K2);
	void	EncodeBlowfishKey(BYTE Key[], DWORD KB, BYTE Sub = 3);

	CDiffieHellman	m_DH;
	CBlowFish		m_BF;
	DWORD			m_n;
	DWORD			m_g;
	DWORD			m_Ka;
	DWORD			m_Kb;
	DWORD			m_K;
	BYTE			m_Br[8];
};

//-------------------------------------------------------------------------------
class CKeySender : public CKeyExchangeBase
{
public:
	CKeySender();
	~CKeySender(){}

	void		MakeSenderInfo(_DHSenderInfo& OUT SenderInfo);
	bool		SetRecipientInfo(_DHRecipientInfo& IN RecipientInfo);
	void		MakeSenderSignature(_DHSignature& OUT SenderSignature);

protected:

};

//-------------------------------------------------------------------------------
class CKeyRepicient : public CKeyExchangeBase
{
public:
	CKeyRepicient();
	~CKeyRepicient(){}

	void		SetSenderInfo(_DHSenderInfo& IN SenderInfo);
	void		MakeRecipientInfo(_DHRecipientInfo& OUT RecipientInfo);
	bool		VerifySenderSignature(_DHSignature& IN SenderSignature);

protected:

};
//--------------------------------------------------------------------------

//
//	HOW TO USE KEY EXCHANGE BETWEEN SERVER AND CLIENT.
//
/*
	CKeySender Sender;
	CKeyRepicient Recipient;

	// 1. Sender는 첫번째 패킷을 생성하고 Recipient에 송신한다.
	_DHSenderInfo SenderInfo;
	Sender.MakeSenderInfo(SenderInfo);

	// 2. Recipient는 수신한 첫번째 패킷을 적용한다.
	Recipient.SetSenderInfo(SenderInfo);

	// 3. Recipient는 Sender에 응답합니다.
	_DHRecipientInfo RecipientInfo;
	Recipient.MakeRecipientInfo(RecipientInfo);
	
	// 4. Sender는 Recipient 정보를 받아 Recipient Signature를 검증합니다.
	if(!Sender.SetRecipientInfo(RecipientInfo))
	{
		MessageBox("Failed to verify Recipient Signature!!");
		return;
	}

	// 5. Sender는 Sender Signature를 생성해 Recipient에 전송합니다.
	_DHSignature ServerSignature;
	Sender.MakeSenderSignature(ServerSignature);

	// 6. Recipient는 수신받은 Server Signature를 검증합니다.
	if(!Recipient.VerifySenderSignature(ServerSignature))
	{
		MessageBox("Failed to verify Sender Signature!!");
		return;
	}
	
	// 7. 이제 양측은 제대로된 Blowfish 최종 키를 얻습니다.
	_DHBlowfishKey KeySender, KeyRecipient;
	Sender.GetEncryptionKey(KeySender);
	Recipient.GetEncryptionKey(KeyRecipient);

	// 8. 양측은 각자 Blowfish 키를 세팅합니다.
	//
	//
*/
