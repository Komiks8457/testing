#include "stdafx.h"

#include "blowfish.h"
#include "blowfish_data.h"

#define S(x, i) (m_pSBoxes[i][x.w.byte##i])
#define bf_F(x) (((S(x,0) + S(x,1)) ^ S(x,2)) + S(x,3))
#define ROUND(a, b, n) (a.dword ^= bf_F(b) ^ m_pArray[n])

CBlowFish::CBlowFish()
{
 	ZeroMemory(m_pArray, sizeof(m_pArray));
 	ZeroMemory(m_pSBoxes, sizeof(m_pSBoxes));
}

CBlowFish::~CBlowFish()
{
}

	
void CBlowFish::Blowfish_encipher(DWORD* xl, DWORD* xr)
{
	union aword  Xl, Xr;

	Xl.dword = *xl;
	Xr.dword = *xr;

	Xl.dword ^= m_pArray[0];

	ROUND(Xr, Xl, 1);  
	ROUND(Xl, Xr, 2);
	ROUND(Xr, Xl, 3);  
	ROUND(Xl, Xr, 4);
	ROUND(Xr, Xl, 5);  
	ROUND(Xl, Xr, 6);
	ROUND(Xr, Xl, 7);  
	ROUND(Xl, Xr, 8);
	ROUND(Xr, Xl, 9);  
	ROUND(Xl, Xr, 10);
	ROUND(Xr, Xl, 11); 
	ROUND(Xl, Xr, 12);
	ROUND(Xr, Xl, 13); 
	ROUND(Xl, Xr, 14);
	ROUND(Xr, Xl, 15); 
	ROUND(Xl, Xr, 16);

	Xr.dword ^= m_pArray[17];

	*xr = Xl.dword;
	*xl = Xr.dword;
}

void CBlowFish::Blowfish_decipher(DWORD* xl, DWORD* xr)
{
   union aword  Xl;
   union aword  Xr;

   Xl.dword = *xl;
   Xr.dword = *xr;

   Xl.dword ^= m_pArray[17];

   ROUND(Xr, Xl, 16);  
   ROUND(Xl, Xr, 15);
   ROUND(Xr, Xl, 14);  
   ROUND(Xl, Xr, 13);
   ROUND(Xr, Xl, 12);  
   ROUND(Xl, Xr, 11);
   ROUND(Xr, Xl, 10);  
   ROUND(Xl, Xr, 9);
   ROUND(Xr, Xl, 8);   
   ROUND(Xl, Xr, 7);
   ROUND(Xr, Xl, 6);   
   ROUND(Xl, Xr, 5);
   ROUND(Xr, Xl, 4);   
   ROUND(Xl, Xr, 3);
   ROUND(Xr, Xl, 2);   
   ROUND(Xl, Xr, 1);

   Xr.dword ^= m_pArray[0];

   *xl = Xr.dword;
   *xr = Xl.dword;
}

void CBlowFish::Initialize(BYTE key[], int keybytes)
{
	int  		i, j;
	DWORD  		data, 
				datal, 
				datar;
	union aword temp;

	
	for (i = 0; i < 18; i++)
		m_pArray[i] = bf_P[i];

	for (i = 0; i < 4; i++)
	{
	 	for (j = 0; j < 256; j++)
	 		m_pSBoxes[i][j] = bf_S[i][j];
	}


	j = 0;
	for (i = 0; i < NPASS + 2; ++i)
	{
		temp.dword		= 0;
		temp.w.byte0	= key[j];
		temp.w.byte1	= key[(j+1) % keybytes];
		temp.w.byte2	= key[(j+2) % keybytes];
		temp.w.byte3	= key[(j+3) % keybytes];
		data			= temp.dword;
		m_pArray [i]	^= data;
		j = (j + 4) % keybytes;
	}

	datal = 0;
	datar = 0;

	for (i = 0; i < NPASS + 2; i += 2)
	{
		Blowfish_encipher(&datal, &datar);
		m_pArray[i]		= datal;
		m_pArray[i + 1]	= datar;
	}

	for (i = 0; i < 4; ++i)
	{
		for (j = 0; j < 256; j += 2)
		{
		  Blowfish_encipher(&datal, &datar);
		  m_pSBoxes[i][j]		= datal;
		  m_pSBoxes[i][j + 1]	= datar;
		}
	}
}

DWORD CBlowFish::GetOutputLength(DWORD lInputLong)
{
	DWORD	lVal = lInputLong % 8;	

	return((lVal != 0) ? lInputLong + 8 - lVal : lInputLong);	
}

// job: Encode pInput into pOutput.  
// Input length in lSize.  
// Returned value is length of output which will be even MOD 8 bytes.  
// Input buffer and output buffer can be the same, but be sure buffer length is even MOD
DWORD CBlowFish::Encode(BYTE* pInput, BYTE* pOutput, DWORD lSize)
{
	DWORD 	lCount, 
			lOutSize, 
			lGoodBytes;
	BYTE	*pi, *po;
	int		i, j;
	BOOL	bSameDest = (pInput == pOutput ? TRUE : FALSE);
	int		nCount = 0;

	lOutSize = GetOutputLength(lSize);

	for (lCount = 0; lCount < lOutSize; lCount += 8)
	{
		if (bSameDest)	
		{
		 	if (lCount < lSize - 7)
		 	{
		 	 	Blowfish_encipher((DWORD*)pInput, (DWORD*)(pInput + 4));
		 	}
		 	else
		 	{
				po	= pInput + (lSize - (nCount * 8));
				j	= (int)(lOutSize - lSize);
				for (i = 0; i < j; i++)
				{
					*po++ = 0;
				}

		 	 	Blowfish_encipher((DWORD*)pInput, (DWORD*)(pInput + 4));
		 	}
		 	pInput += 8;
			nCount++;
		}
		else 	
		{       
		 	if (lCount < lSize - 7)
		 	{
		 		pi = pInput;
		 		po = pOutput;
		 		for (i = 0; i < 8; i++)
				{
		 			*po++ = *pi++;
				}
		 	 	Blowfish_encipher((DWORD*)pOutput, (DWORD*)(pOutput + 4));
		 	}
		 	else
		 	{
		 		lGoodBytes	= lSize - lCount;
		 		po			= pOutput;

		 		for (i = 0; i < (int)lGoodBytes; i++)
				{
		 			*po++ = *pInput++;
				}

		 		for (j = i; j < 8; j++)
				{
		 			*po++ = 0;
				}

		 	 	Blowfish_encipher((DWORD*)pOutput,	(DWORD*)(pOutput + 4));
		 	}
		 	pInput	+= 8;
		 	pOutput += 8;
		}
	}
	return lOutSize;
}

// job: Decode pIntput into pOutput. 
// Input length in lSize.  
// Input buffer and output buffer can be the same, but be sure buffer length is even MOD 8.
void CBlowFish::Decode(BYTE* pInput, BYTE* pOutput, DWORD lSize)
{
	DWORD 	lCount;
	BYTE	*pi, *po;
	int		i;
	BOOL	bSameDest = (pInput == pOutput ? TRUE : FALSE);

	for (lCount = 0; lCount < lSize; lCount += 8)
	{
		if (bSameDest)
		{
	 	 	Blowfish_decipher((DWORD*)pInput, (DWORD*)(pInput + 4));
		 	pInput += 8;
		}
		else 		
		{           
	 		pi = pInput;
	 		po = pOutput;
	 		for (i = 0; i < 8;  i++)
			{
	 			*po++ = *pi++;
			}

	 	 	Blowfish_decipher((DWORD*)pOutput,	(DWORD*)(pOutput + 4));
		 	pInput	+= 8;
		 	pOutput += 8;
		}
	}
}
