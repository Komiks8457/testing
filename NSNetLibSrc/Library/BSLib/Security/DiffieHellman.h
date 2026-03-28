#pragma once

//-------------------------------------------------------------------------------
class CDiffieHellman
{
public:
	CDiffieHellman() : m_g(0), m_n(0), m_a(0), m_b(0), m_X(0), m_Y(0), m_K(0) {}
	~CDiffieHellman() {}

	#define MAX_PRIME_NUMBER	2147483648
	#define MAX_RANDOM_INTEGER	2147483648
	
	void	Clear();

	// for Sender Only
	void	CreateKeys(DWORD& Generator, DWORD& Modulus);
	void	CreateSenderInterKey(DWORD& InterKey);
	void	CreateSenderEncryptionKey(DWORD& EncKey, DWORD RecipientInterKey);

	// for Recipient Only
	void	CreateRecipientInterKey(DWORD& InterKey, DWORD Generator, DWORD Modulus);
	void	CreateRecipientEncryptionKey(DWORD& EncKey, DWORD SendersInterKey);

protected:
	DWORD	m_g; // generator
	DWORD	m_n; // modulus
	DWORD	m_a; // value a
	DWORD	m_b; // value b
	DWORD	m_X; // private key for a
	DWORD	m_Y; // private key for b
	DWORD	m_K; // key!! real key!!

public:
	static DWORD	XpowYmodN(DWORD x, DWORD y, DWORD n);
	static DWORD	GeneratePrime();
	static bool		IsPrime(DWORD n, DWORD a);
	static bool		MillerRabin(DWORD n, DWORD dwTrial = 5);
};

