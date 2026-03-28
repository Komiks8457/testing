#pragma once

// netengine version ( heavy | middle | small )
const double NetEngine_Ver = 1.54;

/* 
ver 1.30 -> 1.40 
proactor ( event error backup ) middle update

ver 1.40 -> 1.41
handshake ( leck on )

ver 1.41 -> 1.42
handshake ( leck off )

ver 1.42 -> 1.43
handshke msg del

ver 1.43 -> 1.44
1/25 code return ( #define GLOBALTB_BSLIB_XXX )

ver 1.44 -> 1.54
define used

#define GLOBALTB_BSLIB_HANDSHAKE	
#define GLOBALTB_BSLIB_DISCONNECT	
#define	GLOBALTB_BSLIB_SETUPCORD

*/



///////////////////////////////////////////////////////
// some macros for Network code
///////////////////////////////////////////////////////
#define MTU_SIZE					1460
#define DEFAULT_BACK_LOG			SOMAXCONN
#define NERR_STRING_SIZE			512
#define TIMEOUT_INFINITE			0xffffffff

///////////////////////////////////////////////////////
// NetEngine Internal Error codes
///////////////////////////////////////////////////////
#define NERR_CRITICAL_MASK			(WORD)0x8000

#define NERR_GOOD					(WORD)0x0000
#define NERR_TIMEOUT				(WORD)0x8001
#define NERR_UNKNOWN				(WORD)0x8002
#define NERR_SESSION_LOST			(WORD)0x8003
#define NERR_BUFFER_FULL			(WORD)0x8004
#define NERR_TOO_MANY_OUTSTANDING	(WORD)0x8005
#define NERR_NODATA_TO_SEND			(WORD)0x0006
#define NERR_NO_MSGTARGET			(WORD)0x8007
#define NERR_INVALID_SOCKET			(WORD)0x8008
#define NERR_INVALID_SESSION		(WORD)0x8009
#define NERR_FAILED_TO_TRANSFILE	(WORD)0x800a
#define NERR_INVALID_SEQUENCE		(WORD)0x800b
#define NERR_INVALID_CHECKSUM		(WORD)0x800c
#define NERR_INVALID_MSGSIZE		(WORD)0x800d
#define NERR_FAILED_TO_ACCEPT		(WORD)0x800e
#define NERR_INVALID_TARGET_FILE	(WORD)0x800f
#define NERR_NET_ACCESSED_BEFORE_HANDSHAKE	(WORD)0x8010
#define NERR_INVALID_MSG_HEADER				(WORD)0x8011
#define NERR_COMPLETION_TARGET_MISMATCH		(WORD)0x0012
#define NERR_TARGET_TASK_LOST				(WORD)0x8013
#define NERR_FAILED_TO_CREATE_SESSION		(WORD)0x8014
#define NERR_PEER_DOESNT_RESPONSE			(WORD)0x8015

#define IS_CRITICAL(error_code)			((error_code) & NERR_CRITICAL_MASK)

///////////////////////////////////////////////////////
// FileSystem Internal Error codes
///////////////////////////////////////////////////////
#define	FSERR_GOOD							0
#define FSERR_ALREADY_OPEN					-1
#define FSERR_INVALID_FILEHANDLE			-2
#define FSERR_CANT_GET_FILESIZE				-3
#define FSERR_CANT_CREATE_MMFILE			-4
#define FSERR_CANT_GET_MM_FILEPOINTER		-5
#define FSERR_MUST_OPEN						-6
#define FSERR_MMSYS_NOT_INTIALIZED			-7
#define FSERR_FILESIZE_EXCEEDED				-8
#define FSERR_INVALID_PARAM					-9
#define FSERR_CANT_ACCESS_MULTIUSER			-10
#define FSERR_OPENMODE_CONFLICED			-11
#define FSERR_NODATA_TO_READ				-12
#define FSERR_CANT_OPEN_FILE				-13
#define FSERR_FILETYPE_MISMATCH				-14
#define FSERR_UNKNOWN						-15
#define FSERR_CANT_UNCOMP_NOTENOUGH_MEM		-16
#define FSERR_CANT_UNCOMP_CORRUPTED_FILE	-17
#define FSERR_FILE_ENTRY_ADD_FAILED			-18
#define FSERR_CANT_CREATE_FILE				-19

// available file types
enum FILE_TYPE
{
	FILE_TYPE_INVALID = -1,
	FILE_TYPE_MEM = 0,
	FILE_TYPE_LEGACY,
	FILE_TYPE_MEMORYMAPPED,

	FILE_TYPE_NUM
};

////////////////////////////////
// Maintenence Configuration
////////////////////////////////
enum _MAINTAIN_TARGET
{
	MAINTAIN_TARGET_STALE_SESSIONS,
	MAINTAIN_TARGET_RARELY_USED_FILES,
	MAINTAIN_TARGET_KEEP_ALIVE_SESSION,

	MAINTAIN_TARGET_NUM
};

#define MT_MASK_STALE_SESSIONS			DWORD(1 << MAINTAIN_TARGET_STALE_SESSIONS)
#define MT_MASK_RARELY_USED_FILES		DWORD(1 << MAINTAIN_TARGET_RARELY_USED_FILES)
#define MT_MASK_KEEP_ALIVE_SESSION		DWORD(1 << MAINTAIN_TARGET_KEEP_ALIVE_SESSION)
#define MT_MASK_ALL						DWORD(0xffffffff)

#define MAINTAINER_POLL_RESOLUTION		DWORD(1000)
#define MAINTAINER_STALE_SESSION_KEEP_ALIVE 15000
#define MAINTAINER_STALE_SESSION_INTERVAL 60000
#define MAINTAINER_STALE_SESSION_TIMEOUT 180000

////////////////////////////////
// winsock.h overrides
////////////////////////////////
struct BSSOCKADDR_IN 
{
    short			sin_family;
    unsigned short	sin_port;
    unsigned long	sin_addr;
    char			sin_zero[8];
};

/*
typedef struct _BSWSABUF 
{
    unsigned long	len;
    char FAR *		buf;

} BSWSABUF, FAR * LPBSWSABUF;
*/

#define BS_SOCK_STREAM     1               /* stream socket */
#define BS_SOCK_DGRAM      2               /* datagram socket */
#define BS_SOCK_RAW        3               /* raw-protocol interface */
#define BS_SOCK_RDM        4               /* reliably-delivered message */
#define BS_SOCK_SEQPACKET  5               /* sequenced packet stream */

////////////////////////////////////////////////
// netengine resource usage info
////////////////////////////////////////////////
struct _USAGE
{
	size_t Total;
	size_t Used;
	union
	{
		struct
		{		
			size_t Free;
		};
		
		struct
		{
			size_t MaxCache;
			size_t CacheUsed;
			
			size_t CacheHitPercentage;
		};
	};
};

struct SYS_RES_USAGE
{
	enum SYS_RES_USAGE_TYPE
	{
		SYS_RES_USAGE_MSG,
		SYS_RES_USAGE_SOCKET,
		SYS_RES_USAGE_SESSION,
		SYS_RES_USAGE_FILESYS,
		SYS_RES_USAGE_ACCEPT,
		SYS_RES_USAGE_SESSION_CLASSLINK,

		SYS_RES_TYPE_NUM
	};

	_USAGE	UsageData[SYS_RES_TYPE_NUM];
};

#define SYSRES_USAGE_MASK_MSG		DWORD(1 << SYS_RES_USAGE::SYS_RES_USAGE_MSG)
#define SYSRES_USAGE_MASK_SOCKET	DWORD(1 << SYS_RES_USAGE::SYS_RES_USAGE_SOCKET)
#define SYSRES_USAGE_MASK_SESSION	DWORD(1 << SYS_RES_USAGE::SYS_RES_USAGE_SESSION)
#define SYSRES_USAGE_MASK_FILESYS	DWORD(1 << SYS_RES_USAGE::SYS_RES_USAGE_FILESYS)
#define SYSRES_USAGE_MASK_ACCEPT	DWORD(1 << SYS_RES_USAGE::SYS_RES_USAGE_ACCEPT)
#define SYSRES_USAGE_MASK_SESSION_CLASSLINK		DWORD(1 << SYS_RES_USAGE::SYS_RES_USAGE_SESSION_CLASSLINK)

#define SYSRES_USAGE_MASK_ALL		DWORD(0xffffffff)

////////////////////////////////////////////////
// enums for io_context 
////////////////////////////////////////////////

enum
{
	IO_ACCEPT = 0,
	IO_READ,
	IO_WRITE,
	IO_DISCONNECT,
	IO_CLOSED,

	IOTYPE_NUM
};

enum IO_TARGET
{
	IO_TARGET_UNKNOWN = -1,
	IO_TARGET_DATA = 0,
	IO_TARGET_FILE,

	IO_TARGET_NUM
};

enum IO_MODE
{
	IO_MODE_INVALID = -1,
	IO_MODE_DIRECT  = 0,
	IO_MODE_BUFFERED,

	IO_MODE_NUM
};

////////////////////////////////////////////////
// session state
////////////////////////////////////////////////
enum SESSION_STATE
{
	BEFORE_HANDSHAKE,	// session encoding nego가 이루어지기 전
	AFTER_HANDSHAKE		// session encoding nego가 완료된 이후
};

////////////////////////////////////////////////
// session type
////////////////////////////////////////////////
enum SESSION_TYPE
{
	SESSION_TYPE_INVALID = -1,
	SESSION_TYPE_PASSIVE = 0,
	SESSION_TYPE_ACTIVE,

	SESSION_TYPE_NUM
};

////////////////////////////////////////////////
// << The Anatomy of SRO network msg Header >>
//
// 1. Composition
//		[MsgSize] + [MsgID] + [optional: Seq NO + Checksum]
//
// 2. Each Field Detail
//		1) MsgSize (8 Bytes)
//			- consist of msg size and 4bit option flags
//			1000000 00000000 00000000 00000000 : is encrypted msg
//			01111111 11111111 11111111 11111111 : bits for size (only for data doesn't includes header size)
//
//		2) MsgID (2 Bytes)
//			- consist of msg id and 6bit option flags
//			1000xxxx xxxxxxxx : is ack msg (in case of bi-directional msg)
//			0100xxxx xxxxxxxx : is req msg (in case of bi-directional msg)
//			0011xxxx xxxxxxxx : application id (netengine, framework or game,... etc)
//			0000xxxx xxxxxxxx : user defined msg id
//
//		3) seq (1 Byte)
//
//		4) checksum (1 Byte)

//////////////////////////////////////
// (1) MsgSize field
//////////////////////////////////////

// masks for size field
#define MSG_MASK_SIZE_ONLY					static_cast<DWORD>(0x000FFFFF)
#define MSG_MASK_SIZEFIELD_FLAGS			static_cast<DWORD>(0x80000000)

// flags for size field
#define MSG_FLAG_SIZEFIELD_ENCRYPTED		static_cast<DWORD>(0x80000000)

//////////////////////////////////////
// (2) MsgID field
//////////////////////////////////////
#define MSG_FLAG_IDFIELD_ACKMSG				(WORD)0x8000
#define MSG_FLAG_IDFIELD_REQMSG				(WORD)0x4000
#define MSG_FLAG_IDFIELD_DUMMYMASK			(WORD)0x0000

#define	MSG_FLAG_IDFIELD_APP_ENGINE			(WORD)0x1000
#define MSG_FLAG_IDFIELD_APP_SERVER			(WORD)0x0800

//////////////////////////////////////
// Flag Checker
//////////////////////////////////////
#define IS_USERMSG(MSGID)		(((MSGID & 0x3000) | 0x0000) == 0x0000)

#define IS_REQMSG(MSGID)		(((MSGID) & MSG_FLAG_IDFIELD_REQMSG) == MSG_FLAG_IDFIELD_REQMSG)
#define IS_ACKMSG(MSGID)		(((MSGID) & MSG_FLAG_IDFIELD_ACKMSG) == MSG_FLAG_IDFIELD_ACKMSG)

#define IS_ENGINEMSG(MSGID)		(((MSGID) & MSG_FLAG_IDFIELD_APP_ENGINE) == MSG_FLAG_IDFIELD_APP_ENGINE)
#define IS_APP_SERVERMSG(MSGID)	(((MSGID) & MSG_FLAG_IDFIELD_APP_SERVER) == MSG_FLAG_IDFIELD_APP_SERVER)

//////////////////////////////////////
// Msg Builder
//////////////////////////////////////
#define MAKE_ENGINEMSG(ID, DIR)	(WORD)(MSG_FLAG_IDFIELD_APP_ENGINE | (DIR) | (ID))

#define CONVERT_ACKTOREQ(ID)	(WORD)(((~MSG_FLAG_IDFIELD_ACKMSG) & (ID)) | (MSG_FLAG_IDFIELD_REQMSG))
#define CONVERT_REQTOACK(ID)	(WORD)(((~MSG_FLAG_IDFIELD_REQMSG) & (ID)) | (MSG_FLAG_IDFIELD_ACKMSG))

/////////////////////////////////////
// Each Filed Length (in bytes)
/////////////////////////////////////
// size field
#define MSG_OFFSET_SIZE			0
#define MSG_FIELDLEN_SIZE		4

// id field
#define MSG_OFFSET_ID			4
#define MSG_FIELDLEN_ID			2

// seq field
#define MSG_OFFSET_SEQ			6
#define MSG_FIELDLEN_SEQ		8

// checksum field
#define MSG_OFFSET_CHECKSUM		14
#define MSG_FIELDLEN_CHECKSUM	1


////////////////////////////////////
// Encrypt msg from here (in bytes)
////////////////////////////////////
#define MSG_ENC_BEGIN			4

// disconnect func ( return value )
#define DISCONNECT_RESULT_UNKNOWN		0		// transmit error
#define DISCONNECT_RESULT_ERROR			0		// transmit error
#define DISCONNECT_RESULT_SUCCESS		1		// transmit success 
#define DISCONNECT_RESULT_PENDING		2		// transmit wait
#define DISCONNECT_RESULT_SESSION		3		// session empty

//	GetQueuedCompletionStatus ( func error )
#define GQCS_FUNC_ERROR		64

//////////////////////////////////////
// reserved system msgs
//////////////////////////////////////
#define ENGINEMSG_HANDSHAKING_REQ		MAKE_ENGINEMSG(0, MSG_FLAG_IDFIELD_REQMSG)
#define ENGINEMSG_HANDSHAKING_ACK		MAKE_ENGINEMSG(0, MSG_FLAG_IDFIELD_ACKMSG)

#define ENGINEMSG_FILE_DATA				MAKE_ENGINEMSG(1, MSG_FLAG_IDFIELD_DUMMYMASK)

#define ENM_SESSION_CREATED				MAKE_ENGINEMSG(1, 0)
#define ENM_SESSION_LOST				MAKE_ENGINEMSG(2, 0)
	#define DISCONNECT_REASON_NET_ENGINE_ERROR		0x0001
	#define	DISCONNECT_REASON_INTENTIONAL_KILL		0x0002
	#define DISCONNECT_REASON_STALE_SESSION			0x0003
	#define DISCONNECT_REASON_CLOSED_BY_PEER		0x0004
	#define DISCONNECT_REASON_ENGINE_SHUTDOWN		0x0005
	#define DISCONNECT_REASON_OVERLAPPED			0x0006
	#define DISCONNECT_REASON_SESSION_LOST			0x0007

#define DISCONNECT_REASON_EXTRACTOR(reason)		LOWORD((reason))
#define NETENGINE_ERROR_CODE_EXTRACTOR(reason)	HIWORD((reason))

#define ENM_SESSION_HANDSHAKED			MAKE_ENGINEMSG(3, 0)
#define ENM_FILEDATA_RECEIVED			MAKE_ENGINEMSG(4, 0)
#define ENM_FILE_TRANSFER_COMPLETED		MAKE_ENGINEMSG(5, 0)
#define ENM_FILE_TRANSFER_FAILED		MAKE_ENGINEMSG(6, 0)

//////////////////////////////////////
// session protection scheme
//////////////////////////////////////
#define ENC_KEY_LEN						8

#define	BSNET_MSG_OPT_NOT_INITIALIZED			0x00
#define BSNET_MSG_DONT_ENCODE					0x01
#define	BSNET_MSG_OPT_ENC						0x02
#define	BSNET_MSG_OPT_SEQ_n_CHECKSUM			0x04

#define BSNET_MSG_OPT_SECURITY_SENDER			0x08
#define BSNET_MSG_OPT_SECURITY_RECIPIENT		0x10

// BSNET SECURITY MODE
enum BSNET_SECURITY
{
	BSNET_DEFAULT = 0x01,
	BSNET_DIFFIEHELLMAN_DEEPDARK = 0x02,
};
	
struct EncodeCtx
{
	EncodeCtx()
	{
		options  = BSNET_MSG_OPT_NOT_INITIALIZED;
		seq_seed = 0;
		crc_seed = 0;

		::ZeroMemory(bf_key, sizeof(bf_key));
	}

	BYTE options;

	DWORD seq_seed;
	DWORD crc_seed;
	BYTE bf_key[ENC_KEY_LEN];
};


//////////////////////////////////////////////////////////
// ID Pool
//////////////////////////////////////////////////////////

#define ID_POOL_INCREMENT_SIZE		5000			// 5천개씩
#define ID_POOL_KEEP_MINIMAL_RATIO	(float)0.2f		// 만일을 대비해 여분이 ID_POOL_INCREMENT_SIZE 의 20% 이하로 떨어지면 확장 해라

class CIDPool
{
public:
	CIDPool()
	{
		m_dwLastAllocID = 1;
		m_dwSpanSize = ID_POOL_INCREMENT_SIZE;
	}

	~CIDPool() {};

protected:
	DWORD	m_dwSpanSize;
	DWORD	m_dwLastAllocID;

	std::queue<DWORD>	m_Pool;

public:
	BOOL Create(DWORD pool_span_size = ID_POOL_INCREMENT_SIZE)
	{
		m_dwSpanSize = pool_span_size;
		Span();
		
		return TRUE;
	}

	void	FreeID(DWORD ID) 
	{ 
		if (ID == 0)
			return;

		m_Pool.push(ID); 
	}

	DWORD	AllocID()
	{
		DWORD NewID;
		
		if (m_Pool.empty() == TRUE ||	// 이럴 수도 있을까? -_-a
			m_Pool.size() < (m_dwSpanSize * ID_POOL_KEEP_MINIMAL_RATIO))	// 완전히 바닥나기 전에 pool 사이즈 늘린다.
			Span();

//		_ASSERT(m_Pool.size() > 0);

		NewID = m_Pool.front();
		m_Pool.pop();

		_ASSERT(NewID != NULL);

		return NewID;
	}
	
	size_t	GetAllocatedIDNum() 
	{ 
		return (m_dwLastAllocID - 1); 
	}

	void	GetPoolUsage(size_t& Allocated, size_t& InUse)
	{
		Allocated	= m_dwLastAllocID - 1;
		InUse		= (Allocated - m_Pool.size());
	}

	DWORD	GetSpanSize() { return m_dwSpanSize; }
	
protected:
	void Span()
	{
		for (DWORD i = m_dwLastAllocID; i < (m_dwLastAllocID + m_dwSpanSize); ++i)
		{
			if (i == 0)
				continue;

			m_Pool.push(i);
		}
		
		m_dwLastAllocID += m_dwSpanSize;
	}
};