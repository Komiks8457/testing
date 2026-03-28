#pragma once

/////////////////////////////////////////////
// CRC32 - for generating checksum value
/////////////////////////////////////////////
#define CRC32_TABLE_SIZE		256

class CRC32
{
public:	
	CRC32();
	~CRC32() {}
	
	void	Initialize(DWORD dwSeed = 0);

	BYTE	Generate(LPBYTE pData, DWORD dwLength);
	DWORD	Generate32(LPBYTE pData, DWORD dwLength);

	void	Reset() { m_dwTable = 0; }
	
protected:	
	DWORD	m_dwTable;

public:
	static DWORD	s_Crc32Seed[CRC32_TABLE_SIZE];
	static DWORD	s_CrcTable[CRC32_TABLE_SIZE][CRC32_TABLE_SIZE];
	static BOOL		s_CrcInitialized;
};


/////////////////////////////////////////////
// SeqGenerator
/////////////////////////////////////////////
#define SEQ_GEN_DUMMY_PARAM		BYTE(0x00)

class CSeqNumGenerator
{
public:   
	CSeqNumGenerator() : m_btSeq(0), m_btAdder(SEQ_GEN_DUMMY_PARAM), m_btMultiplier(SEQ_GEN_DUMMY_PARAM)
	{
	}

	void	SetParams(BYTE adder, BYTE multiplier) 
	{ 
		if (adder == 0)
			adder = 1;
		if (multiplier == 0)
			multiplier = 1;

		m_btSeq			= adder ^ multiplier; 
		m_btAdder		= adder; 
		m_btMultiplier	= multiplier; 	
	}

	BYTE	GetCurSeqNum() { return m_btSeq; }
	void	SetCurSeqNum(BYTE seq) { m_btSeq = seq; }

	BYTE	GenSeqNum()
	{
		_ASSERT(m_btAdder != 0 || m_btMultiplier != 0);

		m_btSeq = ((~m_btSeq) + m_btAdder) * m_btMultiplier;
		m_btSeq = m_btSeq ^ (m_btSeq >> 4);
		
		return m_btSeq; 
	}
	
	void	Reset() { m_btAdder = m_btMultiplier = SEQ_GEN_DUMMY_PARAM; }
	
private:
	BYTE	m_btSeq;
	BYTE	m_btAdder;
	BYTE	m_btMultiplier;
};

//////////////////////////////////////////////
// CRndGenerator - 이게 뭔 수작이냐고? 나도 몰라... 그러니깐 랜덤이지...
//////////////////////////////////////////////
class CRndGenerator
{
public:
	CRndGenerator(DWORD seed)
	{
		if (seed == 0)
			seed = 0x9abfb3b6; // 이게 무슨 숫자냐고? 나도 몰라... 

		m_Seed = seed;
	}

	DWORD Generate()
	{
		DWORD Value1 = SampleValue();
		DWORD Value2 = SampleValue();
		
		Value1 ^= Value2;
		return Value1;
	}

private:
	DWORD m_Seed;

private:
	DWORD SampleValue()
	{
		DWORD temp	= m_Seed;
		DWORD bit	= 0;

		for (DWORD n = 0; n < 32; ++n)
		{
			bit  = ((temp >> 0) ^ (temp >> 1) ^ (temp >> 2) ^ (temp >> 3) ^ (temp >> 5) ^ (temp >> 7)) & 1;
			temp = (((temp >> 1) | (temp << 31)) & ~1) | bit;
		}

		m_Seed = temp;

		return temp;
	}
};

