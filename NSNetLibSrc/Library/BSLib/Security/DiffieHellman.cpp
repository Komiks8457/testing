#include "stdafx.h"
#include "DiffieHellman.h"
#include <objbase.h>
//#include <stl_algobase.h>
//#include <stl\_algobase.h>

void CDiffieHellman::Clear()
{
	m_g = 0; // generator
	m_n = 0; // modulus
	m_a = 0; // value a
	m_b = 0; // value b
	m_X = 0; // interkey for a
	m_Y = 0; // interkey for b
	m_K = 0; // key!! real key!!
}

DWORD CDiffieHellman::XpowYmodN(DWORD x, DWORD y, DWORD n)
{
	unsigned __int64 s = 1, t = x, u = y;

	while(u)
	{
		if(u & 1) 
			s = (s * t) % n;
		
		u >>= 1;
		t = (t * t) % n;
	}

	return (DWORD)s;
}

DWORD CDiffieHellman::GeneratePrime()
{
	GUID RandomNumber;
	CoCreateGuid(&RandomNumber);

	DWORD dwVal = 0;
	dwVal = (RandomNumber.Data1 % MAX_PRIME_NUMBER);

	// ensure it is an odd number
	if ((dwVal & 1) == 0) ++dwVal;

	for(;;)
	{
		if(MillerRabin(dwVal, 5)) 
			break;

		dwVal += 2;
	}
		
	return dwVal;
}

bool CDiffieHellman::IsPrime(DWORD n, DWORD a) 
{ 
	return (1 == XpowYmodN(a, n-1, n)); 
}

bool CDiffieHellman::MillerRabin(DWORD n, DWORD dwTrial)
{
	DWORD a = 0; 

	for(DWORD i = 0; i < dwTrial; i++)
	{ 
		a = (rand() % (n - 3)) + 2;// gets random value in [2..n-1] 

		if(!IsPrime(n, a)) //n composite, return false 
			return false;
	} 
	
	return true; // n probably prime 
}

void CDiffieHellman::CreateKeys(DWORD& Generator, DWORD& Modulus)
{
	m_g = GeneratePrime();
	m_n = GeneratePrime();

	/*
	if(m_g > m_n)
		std::swap(m_g, m_n);
	*/
	if(m_g > m_n)
	{
		DWORD temp = m_g;
		m_g = m_n;
		m_n = temp;
	}

	Generator = m_g;
	Modulus = m_n;
}

void CDiffieHellman::CreateSenderInterKey(DWORD& InterKey)
{
	GUID RandomNumber;
	CoCreateGuid(&RandomNumber);

	m_a = RandomNumber.Data1 % MAX_RANDOM_INTEGER;
	m_X = XpowYmodN(m_g, m_a, m_n);	
	InterKey = m_X;
}

void CDiffieHellman::CreateSenderEncryptionKey(DWORD& EncKey, DWORD RecipientInterKey)
{
	m_Y = RecipientInterKey;
	m_K = XpowYmodN(m_Y, m_a, m_n);
	EncKey = m_K;
}

void CDiffieHellman::CreateRecipientInterKey(DWORD& InterKey, DWORD Generator, DWORD Modulus)
{
	GUID RandomNumber;
	CoCreateGuid(&RandomNumber);

	m_b = RandomNumber.Data1 % MAX_RANDOM_INTEGER;
	m_g = Generator;
	m_n = Modulus;
	m_Y = XpowYmodN(m_g,m_b,m_n);	
	InterKey = m_Y;
}

void CDiffieHellman::CreateRecipientEncryptionKey(DWORD& EncKey, DWORD SendersInterKey)
{
	m_X = SendersInterKey;
	m_K = XpowYmodN(m_X, m_b, m_n);
	EncKey = m_K;
}

