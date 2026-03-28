#pragma once

/*
	-------------------------------------
		f		f		1byte
	-------------------------------------
	   XX01				NET_ENGINE
	   XX10				FRAMEWORK
	   XX11				APP MSG
	   XX00				PEER_NET

	   00XX				DUMMY
	   01XX				REQ
	   10XX				ACK

	   XXXX	  1XXX		SERVER ONLY MSG
	   XXXX   0001		APP from
	   XXXX	  0111		APP to
*/

#define MSG_FLAG_IDFIELD_NODIR				0
#define	MSG_FLAG_IDFIELD_FRAMEWORK			(WORD)0x2000

#define MAKE_FRAMEWORKMSG( dir, index)	(MSG_FLAG_IDFIELD_FRAMEWORK | (dir) | index)

#define IS_FRAMEWORK_MSG(MSGID)			( (((MSGID & 0x3000) & MSG_FLAG_IDFIELD_FRAMEWORK)) == (((MSGID & 0x3000)| MSG_FLAG_IDFIELD_FRAMEWORK)) )
//#define IS_FRAMEWORK_MSG(MSGID)			( ((MSGID) & MSG_FLAG_IDFIELD_FRAMEWORK) == MSG_FLAG_IDFIELD_FRAMEWORK )

#define RESULT_SUCCESS		(BYTE)1
#define RESULT_FAIL			(BYTE)2

/*************************************************************************************************************
..............................................................................................................
......................SSSS...EEEEEE..PPPPP.....AA....RRRRR.....AA....TTTTTT...OOOO...RRRRR....................
.....................SS..SS..EE......PP..PP...AAAA...RR..RR...AAAA.....TT....OO..OO..RR..RR...................
.....................SS......EE......PP..PP..AA..AA..RR..RR..AA..AA....TT....OO..OO..RR..RR...................
......................SSSS...EEEEEE..PPPPP...AAAAAA..RRRR....AAAAAA....TT....OO..OO..RRRR.....................
.........................SS..EE......PP......AA..AA..RR.RR...AA..AA....TT....OO..OO..RR.RR....................
.....................SS..SS..EE......PP......AA..AA..RR..RR..AA..AA....TT....OO..OO..RR..RR...................
......................SSSS...EEEEEE..PP......AA..AA..RR..RR..AA..AA....TT.....OOOO...RR..RR...................
..............................................................................................................
*************************************************************************************************************/

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ServerCord생성
#define FRAMEWORKMSG_SETUPCORD									MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x001)
#define FRAMEWORKMSG_HEARTBEAT									MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x002)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// 인증관련
#define FRAMEWORKMSG_CERTIFICATE_REQ							MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x003)
#define FRAMEWORKMSG_CERTIFICATE_ACK							MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x003)
#	define CERTIFICATION_ERROR_UNIDENTIFIED		(BYTE)0x01		// 너는 누군지 모르겠다, Broker를 잘못잡아 놓았거나 불법서버다
#	define CERTIFICATION_ERROR_WRONGVERSION		(BYTE)0x02		// 패치가 필요하다
#	define CERTIFICATION_ERROR_UNKNOWN			(BYTE)0x03		// 아마도 DB질 하다 뻗었겠다
#	define CERTIFICATION_ERROR_NOT_SERVICE_TIME	(BYTE)0x04		// 서비스 시간이 아니다 (client전용)
#	define CERTIFICATION_ERROR_NEED_FULLVERSION	(BYTE)0x05		// patch를 해줄 수 없는 버전이다. full version을 받아라
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// 화일을 받을려고 하는 넘이 RecvFile걸어 놓고 요청 
#define FRAMEWORKMSG_TRANSFERFILE_REQ							MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x004)
#define FRAMEWORKMSG_TRANSFERFILE_ACK							MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x004)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// 다른 서버한테 상태변화를 알려주고 싶을때
// 하위 서버로 부터 받은 Notify정보를 위로 올릴지 않올릴지는 해당 서버에 옵션으로 존재한다
// 몇다리 걸쳐서 받는 경우도 생각준다 (ServerID같은건 까지 말고 그대로 Relay시켜준다)
#define FRAMEWORKMSG_NOTIFY										MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR,  0x005)
#define FRAMEWORKMSG_NOTIFY_REQ									MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x005) // ->notify를 요청할때 당장 현재의 상태를 알고 싶을때
#	define SFNOTIFY_DATA_MASK_VOID					(BYTE)0x00
#	define SFNOTIFY_DATA_MASK_ALL					(BYTE)0xFF
#	define SFNOTIFY_DATA_MASK_SERVERBODY_STATUS		(BYTE)0x01		// 서버 ExcutionStep이나 Status
#	define SFNOTIFY_DATA_MASK_SERVERCORD_STATUS		(BYTE)0x02		// 서버간 연결에 관한 상태
#	define SFNOTIFY_DATA_MASK_CUSTOM				(BYTE)0x04
#		define SFNOTIFY_DATA_SUB_MASK_SERVERBODY_STATUS_EXECUTIONSTEP	(BYTE)0x01
#		define SFNOTIFY_DATA_SUB_MASK_SERVERBODY_STATUS_STATUS			(BYTE)0x02
#	define SFNOTIFY_DATA_MASK_SERVERCORD_ESTABLISHED	(BYTE)0x08
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// shell execution
#define FRAMEWORKMSG_SHELL_COMMAND_REQ							MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0006)
#define FRAMEWORKMSG_SHELL_COMMAND_ACK							MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0006)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_TIMER										MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR,  0x0007)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
/*
	OVERLAPJOB은 포괄적인 개념으로써 사용되지만, 서버간의 메세지 전달에 있어서..

	응답시간 초과등의 오류상황에 적극적으로 대처가 가능한 형태로써, 서버간의 특정 메세지의

	전달과정에 매우 유용하게 쓰일 수 있다 (단순히 게임콘텐츠적인 측면뿐아니라..)
*/
#define FRAMEWORKMSG_OVERLAPJOB_REQ								MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0008)//	<< (DWORD)Index << (WORD)wRealMsgID <<data(...)
#define FRAMEWORKMSG_OVERLAPJOB_ACK								MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0008)//	>> (DWORD)index >> (WORD)wRealMsgId
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// 자체 overlap job에 대한 결과
#define FRAMEWORKMSG_OVERLAPJOB_LOCAL_REQ						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0009)// << (DWORD)ptrOverlapJob
#define FRAMEWORKMSG_OVERLAPJOB_LOCAL_ACK						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0009)// << (DWORD)ptrOverlapJob

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_APP_LOG									MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x000a)
#define FRAMEWORKMSG_APP_EXIT									MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x000b)
#define FRAMEWORKMSG_APP_INITIALIZE_LOCALDATA					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x000c)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//peer net
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//errors
#define DEDICATE_ERR_GOOD							(BYTE)0x00
#define DEDICATE_ERR_UNKNOWN						(BYTE)0x01
#define DEDICATE_ERR_GROUP_DUPLICATED				(BYTE)0x02
#define DEDICATE_ERR_INVALID_PEER_GROUP				(BYTE)0x03
#define DEDICATE_ERR_INVALID_PEER_SESSION			(BYTE)0x04

#define FRAMEWORKMSG_PEERNET_SESSION_UPDATED					MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_DUMMYMASK,	0x0011)
	#define	PEER_SESSION_ADDED						(BYTE)0x01
	#define	PEER_SESSION_DELETED					(BYTE)0x02

#define FRAMEWORKMSG_PEERNET_DEDICATED_SVR_JOB_REQ				MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_REQMSG,		0x0012)
#define FRAMEWORKMSG_PEERNET_DEDICATED_SVR_JOB_ACK				MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_ACKMSG,		0x0012)
	#define DEDICATED_SVR_JOB_CREATE_GAME_WORLD				(BYTE)0x01
	#define DEDICATED_SVR_JOB_DESTROY_PEER_GROUP			(BYTE)0x02
	#define DEDICATED_SVR_JOB_READY_FOR_BEGIN_GAME			(BYTE)0x03
	#define DEDICATED_SVR_JOB_BEGIN_GAME					(BYTE)0x04
	#define DEDICATED_SVR_JOB_READY_TO_NEXT_GAME			(BYTE)0x05

//dedicated server information to peers!!
#define FRAMEWORKMSG_PEERNET_DEDICATED_SERVER_INFO				MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_DUMMYMASK,	0x0013)

//open connection to dedicated server from peers
#define FRAMEWORKMSG_PEERNET_OPEN_CONNECTION_REQ				MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_REQMSG,		0x0014)
#define FRAMEWORKMSG_PEERNET_OPEN_CONNECTION_ACK				MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_ACKMSG,		0x0014)
	#define OPEN_RESULT_CONNECTING					(BYTE)0x01
	#define OPEN_RESULT_ESTABLISHED					(BYTE)0x02
	#define OPEN_RESULT_REJECTED					(BYTE)0x03

#define FRAMEWORKMSG_PEERNET_P2P_CONNECTING_FINISHED			MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_DUMMYMASK,	0x0015)

//peers to dedicated connection report
#define FRAMEWORKMSG_PEERNET_CONNECTION_REPORT					MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_DUMMYMASK,	0x0016)
	#define CONNECTION_PUNCHING_FINISHED			(BYTE)0x01
	#define CONNECTION_LOST							(BYTE)0x02
	#define CONNECTION_ACCEPTED						(BYTE)0x03

#define FRAMEWORKMSG_PEERNET_TIME_SYNC_REQ						MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_REQMSG,		0x0017)
#define FRAMEWORKMSG_PEERNET_TIME_SYNC_ACK						MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_ACKMSG,		0x0017)

// 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Dedicated <-> GameServer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define FRAMEWORKMSG_DEDICATED_REFDATA_REQ						MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_REQMSG,		0x0030)
#define FRAMEWORKMSG_DEDICATED_REFDATA_ACK						MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_ACKMSG,		0x0030)

//Dedicated -> GameServer
#define FRAMEWORKMSG_DEDICATED_LOAD_REFDATA_COMPLETED_REQ		MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_REQMSG,		0x0031)
#define FRAMEWORKMSG_DEDICATED_LOAD_REFDATA_COMPLETED_ACK		MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_ACKMSG,		0x0031)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*************************************************************************************************************
..............................................................................................................
......................SSSS...EEEEEE..PPPPP.....AA....RRRRR.....AA....TTTTTT...OOOO...RRRRR....................
.....................SS..SS..EE......PP..PP...AAAA...RR..RR...AAAA.....TT....OO..OO..RR..RR...................
.....................SS......EE......PP..PP..AA..AA..RR..RR..AA..AA....TT....OO..OO..RR..RR...................
......................SSSS...EEEEEE..PPPPP...AAAAAA..RRRR....AAAAAA....TT....OO..OO..RRRR.....................
.........................SS..EE......PP......AA..AA..RR.RR...AA..AA....TT....OO..OO..RR.RR....................
.....................SS..SS..EE......PP......AA..AA..RR..RR..AA..AA....TT....OO..OO..RR..RR...................
......................SSSS...EEEEEE..PP......AA..AA..RR..RR..AA..AA....TT.....OOOO...RR..RR...................
..............................................................................................................
*************************************************************************************************************/

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_CERTIFICATE_CLIENTMODULE_REQ				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0100)
#define FRAMEWORKMSG_CERTIFICATE_CLIENTMODULE_ACK				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0100)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_START_CONTENT_SERVICE_REQ				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0102)
#define FRAMEWORKMSG_START_CONTENT_SERVICE_ACK				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0102)
#define FRAMEWORKMSG_START_CONTENT_SERVICE					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_DUMMYMASK, 0x0102)
	#define START_CONTENT_SERVER_FAIL_GOOD						(BYTE)0x00
	#define START_CONTENT_SERVER_FAIL_INVALID_USER_INFO			(BYTE)0x01	// ID or PWD 틀림
	#define START_CONTENT_SERVER_FAIL_USER_DUPLICATED			(BYTE)0x02	// 중복접속
	#define START_CONTENT_SERVER_FAIL_INTERNAL_ERROR			(BYTE)0x03	// 서버 내부 문제가 생겼다
	#define START_CONTENT_SERVER_FAIL_NOT_IN_SERVICE			(BYTE)0x04	// 서버가 서비스 중이 아니다
	#define START_CONTENT_SERVER_FAIL_INSUFFICIENT_IP			(BYTE)0x05	// 해당 IP로는 더이상 접속이 불가능 하다
	#define START_CONTENT_SERVER_FAIL_BLOCK_ACCOUNT				(BYTE)0x06	// 블록된 계정이다
	#define START_CONTENT_SERVER_FAIL_NOT_SERVICE_AGREEMENT		(BYTE)0x07	// 서비스 동의하지 않은 계정
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define	FRAMEWORKMSG_LOGIN_CONTENT_SERVER_REQ				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0103)
#define FRAMEWORKMSG_LOGIN_CONTENT_SERVER_ACK				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0103)
	#	define LOGIN_CONTENT_SERVER_FAIL_INVALID_TOKEN		(BYTE)0x01	// 알 수 없는 Token이거나 Agent에 해당 Token이 존재하지 않는다
	#	define LOGIN_CONTENT_SERVER_FAIL_USER_DUPLICATED	(BYTE)0x02	// agent상에서 중복 접속이 되었다 ㅡ,,ㅡ
	#	define LOGIN_CONTENT_SERVER_FAIL_INTERNAL_ERROR		(BYTE)0x03	// 서버 내부에 문제가 생겼다
	#	define LOGIN_CONTENT_SERVER_FAIL_MAXUSER_EXCEEDED	(BYTE)0x04	// 허용인원 초과
	#	define LOGIN_CONTENT_SERVER_FAIL_INSUFFICIENT_IP	(BYTE)0x05	// IP 허용 인원 수 초과
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#define	FRAMEWORKMSG_LOGOUT_CONTENT_SERVER_REQ				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0104)
#define FRAMEWORKMSG_LOGOUT_CONTENT_SERVER_ACK				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0104)

#define	FRAMEWORKMSG_START_CONTENT_SERVICE_AGREEMENT_REQ	MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0105)
#define FRAMEWORKMSG_START_CONTENT_SERVICE_AGREEMENT_ACK	MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0105)
	#define START_CONTENT_SERVICE_AGREEMENT_FAIL_GOOD					(BYTE)0x00
	#define START_CONTENT_SERVICE_AGREEMENT_FAIL_INVALID_USER_INFO		(BYTE)0x01
	#define START_CONTENT_SERVICE_AGREEMENT_FAIL_INVALID_OPERATION		(BYTE)0x02
	#define START_CONTENT_SERVICE_AGREEMENT_FAIL_SERVICE_DENIED			(BYTE)0x03	// 서비스 동의 거부

#define FRAMEWORKMSG_CLIENTNET_FRAMEWORK_FORWARD_MSG		MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR,  0x0111)

/*************************************************************************************************************
..............................................................................................................
......................SSSS...EEEEEE..PPPPP.....AA....RRRRR.....AA....TTTTTT...OOOO...RRRRR....................
.....................SS..SS..EE......PP..PP...AAAA...RR..RR...AAAA.....TT....OO..OO..RR..RR...................
.....................SS......EE......PP..PP..AA..AA..RR..RR..AA..AA....TT....OO..OO..RR..RR...................
......................SSSS...EEEEEE..PPPPP...AAAAAA..RRRR....AAAAAA....TT....OO..OO..RRRR.....................
.........................SS..EE......PP......AA..AA..RR.RR...AA..AA....TT....OO..OO..RR.RR....................
.....................SS..SS..EE......PP......AA..AA..RR..RR..AA..AA....TT....OO..OO..RR..RR...................
......................SSSS...EEEEEE..PP......AA..AA..RR..RR..AA..AA....TT.....OOOO...RR..RR...................
..............................................................................................................
*************************************************************************************************************/

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_PRE_REGISTER_CONTENT_USER_REQ			MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0200)// << (DWORD)dwJID << (BYTE)nContentID
#define FRAMEWORKMSG_PRE_REGISTER_CONTENT_USER_ACK			MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0200)// << (DWORD)dwJID << (BYTE)nContentID
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// global manager한테 user등록
#define FRAMEWORKMSG_REGISTER_CONTENT_USER_REQ				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0201)// << (DWORD)dwJID << (BYTE)nContentID
#define	FRAMEWORKMSG_REGISTER_CONTENT_USER_ACK				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0201)// >> (DWORD)dwJID >> (BYTE)btResult
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#define REGISTER_CONTENT_USER_FAIL_USER_DUPLICATED			(BYTE)0x1
#define REGISTER_CONTENT_USER_FAIL_INSUFFICIENT_IP			(BYTE)0x2
#define REGISTER_CONTENT_USER_FAIL_NOT_SERVICE_TIME			(BYTE)0x3	// 서비스 시간이 아니다
#define REGISTER_CONTENT_USER_FAIL_BLOCK_GRATIS_USER		(BYTE)0x4	// 무료 사용자는 접속 불가다
#define REGISTER_CONTENT_USER_FAIL_UNKNOWN					(BYTE)0x5
#define REGISTER_CONTENT_USER_FAIL_BILLING_FAILED			(BYTE)0x6	// 빌링서버에게서 인증받지 못함.
#define REGISTER_CONTENT_USER_FAIL_BLOCK_ACCOUNT			(BYTE)0x7	// 정지된 계정이다

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_UNREGISTER_CONTENT_USER				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x0202)// << (DWORD)dwJID
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// agent한테 token등록
#define FRAMEWORKMSG_REGISTER_TOKEN_REQ						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0203)// << (DWORD)dwIP << (LPCSTR)szUserName << (LPCSTR)szPassword
#define FRAMEWORKMSG_REGISTER_TOKEN_ACK						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0203)// << (BYTE)btResult << (DWORD)dwTokenID
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// agent와 shard간의 메세지
// agent가 shard한테 user등록 요청
#define FRAMEWORKMSG_REGISTER_SHARD_USER_REQ				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0204)// << (DWORD)dwJID << (DWORD)dwSessionID << (BYTE)btPrimarySecurityID << (BYTE)btContentSecurityID
#define FRAMEWORKMSG_REGISTER_SHARD_USER_ACK				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0204)/* >> (BYTE)btResult

	if( btResult == RESULT_FAIL)
	{
		>> (BYTE)btErrorCode
	}
*/
#	define REGISTER_SHARD_USER_FAIL_CREATE_USER			(BYTE)0x01		// 신규유저 생성 실패
#	define REGISTER_SHARD_USER_FAIL_DUPLICATED			(BYTE)0x02		// 이미 로그인 되어 있거나 아직 지워지지 않았다.
#	define REGISTER_SHARD_USER_FAIL_MAXUSER_EXCEEDED	(BYTE)0x03		// 꽉 찼다
#	define REGISTER_SHARD_USER_FAIL_SERVER_STATE		(BYTE)0x04		// 서버 상태
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// agent에서 client와의 접속이 끊어져 버렸다, 또는 유저가 정상 종료 했다고 서버가 알려왔다
#define FRAMEWORKMSG_UNREGISTER_SHARD_USER					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x0205)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// shard가 명시적으로 agent에 요청해서 client를 끊을때
#define FRAMEWORKMSG_BAN_SHARD_USER							MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x0206)// << (WORD)nCount << (DWORD)dwSessionID ...
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// shard가 agent한테 migrtion요청 하는것
#define FRAMEWORKMSG_SHARD_USER_CHANGE_CONTENT_SERVER_REQ	MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0207)// << (DWORD)dwSessionID <<(DWORD)dwJID << (WORD)nServerID
#define FRAMEWORKMSG_SHARD_USER_CHANGE_CONTENT_SERVER_ACK	MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0207)// >> (DWORD)dwJID >> (BYTE)btResult >> (BYTE)btErrorCode
#	define CHANGE_CONTENT_USER_FAILED_INVALID_SESSION	(BYTE)0x01// 현재 접속 중이지 않은 session이닷
#	define CHANGE_CONTENT_USER_FAILED_INVALID_SERVER	(BYTE)0x02// 서버 ID가 이상하거나, 다운된 서버다
#	define CHANGE_CONTENT_USER_FAILED_ALREADY_BOUND		(BYTE)0x03// 이미 했어
#	define CHANGE_CONTENT_USER_FAILED_NOTIFY			(BYTE)0x04// 해당 서버가 거부 했음
#	define CHANGE_CONTENT_USER_FAILED_SESSIONID_JID_MISMATCH (BYTE)0x05 // 보내준 sessionid와 jid가 짝이 않맞음 -> 완전 제앙 
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_RELAY_MSG_TO_CLIENT_SINGLE				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x0209)
#define FRAMEWORKMSG_RELAY_MSG_TO_CLIENT_MULTI_LIST			MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x020a)

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_CONCURRENT_SHARD_USER				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR,	0x020c)

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_CONTENT_USER_BAN_REQ				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x020d)
#define FRAMEWORKMSG_CONTENT_USER_BAN_ACK				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x020d)

#define FRAMEWORKMSG_KICKED								MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x020d)
	#define KICKED_BY_GM								(BYTE)0x01
	#define KICKED_BY_DUPLICATED_LOGIN_USER				(BYTE)0x02
	#define KICKED_BY_GM_COMMAND_ACCOUNT_BLOCK			(BYTE)0x03
	#define KICKED_BY_GAME_GUARD						(BYTE)0x04
	#define KICKED_BY_SYSTEM							(BYTE)0x05
	#define KICKED_BY_GAME_GUARD_ACCOUNT_BAN			(BYTE)0x06
	#define KICKED_BY_GAME_GUARD_HWID_BAN				(BYTE)0x07

#define FRAMEWORKMSG_DEDICATED_COORDINATE_INFO			MAKE_FRAMEWORKMSG(MSG_FLAG_IDFIELD_NODIR, 0x020e)

#define FRAMEWORKMSG_COLLECT_MONITORING_DATA_REQ		MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x020f)

/*************************************************************************************************************
..............................................................................................................
......................SSSS...EEEEEE..PPPPP.....AA....RRRRR.....AA....TTTTTT...OOOO...RRRRR....................
.....................SS..SS..EE......PP..PP...AAAA...RR..RR...AAAA.....TT....OO..OO..RR..RR...................
.....................SS......EE......PP..PP..AA..AA..RR..RR..AA..AA....TT....OO..OO..RR..RR...................
......................SSSS...EEEEEE..PPPPP...AAAAAA..RRRR....AAAAAA....TT....OO..OO..RRRR.....................
.........................SS..EE......PP......AA..AA..RR.RR...AA..AA....TT....OO..OO..RR.RR....................
.....................SS..SS..EE......PP......AA..AA..RR..RR..AA..AA....TT....OO..OO..RR..RR...................
......................SSSS...EEEEEE..PP......AA..AA..RR..RR..AA..AA....TT.....OOOO...RR..RR...................
..............................................................................................................
*************************************************************************************************************/

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_FACULTY_TEST_REQ						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0300)
#define FRAMEWORKMSG_FACULTY_TEST_ACK						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0300)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_LAUNCH_SERVER_REQ						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0301)
#define FRAMEWORKMSG_LAUNCH_SERVER_ACK						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0301)
#	define LAUNCH_SERVER_FAIL_INVALID_MODULE				(BYTE)0x01	// 알 수 없는 ModuleID
#	define LAUNCH_SERVER_FAIL_INVALID_PATH					(BYTE)0x02	// 실행 파일을 찾을 수 없다
#	define LAUNCH_SERVER_FAIL_SERVER_IS_NOT_CERTIFICATED	(BYTE)0x03	// 난 아직 인증 받는 중이다
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_SHUTDOWN_SERVER_REQ					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0302)
#define FRAMEWORKMSG_SHUTDOWN_SERVER_ACK					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0302)
#	define SHUTDOWN_SERVER_FAIL_INVALID_MODULE			(BYTE)0x01
#	define SHUTDOWN_SERVER_FAIL_CANNOT_FIND_PROCESS		(BYTE)0x02	// 알 수 없는 process
#	define SHUTDOWN_SERVER_FAIL_SERVER_IS_NOT_CERTIFICATED	(BYTE)0x03
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_OPERATE_SERVICE_REQ					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0303)
#	define OS_START_SERVICE			(BYTE)0x01
#	define OS_STOP_SERVICE			(BYTE)0x02
#define FRAMEWORKMSG_OPERATE_SERVICE_ACK					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0303)
#	define OPERATE_SERVICE_FAIL_RELAYMSG			(BYTE)0x01
#	define OPERATE_SERVICE_FAIL_INVALID_SERVER		(BYTE)0x02
#	define OPERATE_SERVICE_FAIL_ABNORMAL_CONDITION	(BYTE)0x03	// 서비스를 시작할 수 있는 상태가 아니다
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_QUERY_MACHINE_STATS_REQ				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0304)
#define FRAMEWORKMSG_QUERY_MACHINE_STATS_ACK				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0304)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_PATCH_START_RELAY_REQ					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0305)
#define FRAMEWORKMSG_PATCH_START_RELAY_ACK					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0305)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_MODULE_VERSION_DATA_REQ				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0306)
#define FRAMEWORKMSG_MODULE_VERSION_DATA_ACK				MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0306)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_PREPARE_PATCH_REQ						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0308)
#define FRAMEWORKMSG_PREPARE_PATCH_ACK						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0309) // 헉 왜 9지?
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_CHANGE_SHARD_DATA_REQ					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0310)
#define FRAMEWORKMSG_CHANGE_SHARD_DATA_ACK					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0310)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_REFRESH_SHARD_DATA						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x0311)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define FRAMEWORKMSG_REFRESH_SHARD_SERVICE_TYPE_REQ			MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0312)
#define FRAMEWORKMSG_REFRESH_SHARD_SERVICE_TYPE_ACK			MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0312)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//#define FRAMEWORKMSG_LOG_GM_COMMAND							MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x0313)
#define FRAMEWORKMSG_LOG_GM_COMMAND_REQ						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0313)
#define FRAMEWORKMSG_LOG_GM_COMMAND_ACK						MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0313)

/*************************************************************************************************************
*************************************************************************************************************/
#define FRAMEWORKMSG_LAZED_SHUTDOWN_SERVER_REQ					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x031D)
#define FRAMEWORKMSG_LAZED_SHUTDOWN_SERVER_ACK					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x031D)
#	define LAZED_SHUTDOWN_SERVER_FAIL_INVALID_MODULE			(BYTE)0x01
#	define LAZED_SHUTDOWN_SERVER_FAIL_CANNOT_FIND_PROCESS		(BYTE)0x02	// 알 수 없는 process
#	define LAZED_SHUTDOWN_SERVER_FAIL_SERVER_IS_NOT_CERTIFICATED	(BYTE)0x03
//------------------------------------------------------------------------------------------------------------------

#define FRAMEWORKMSG_SETUP_MSG_ROUTING_TABLE					MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x031e)

//@redpixel {14-06-27} Client<->Agent 간 KeepAlive 메시지. BlackSquad 국내서비스의 경우, Nprotect Gameguard 보안처리용 패킷으로 사용.
#define FRAMEWORKMSG_KEEPALIVE_CLIENT_REQ								MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x031F)
#define FRAMEWORKMSG_KEEPALIVE_CLIENT_ACK								MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x031F)

#define FRAMEWORKMSG_PUBLISHER_COMMAND_REQ									MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_REQMSG, 0x0321)
#define FRAMEWORKMSG_PUBLISHER_COMMAND_ACK									MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x0321)
#define FRAMEWORKMSG_PUBLISHER_COMMAND										MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x0321)
	#define PUBLISHER_COMMAND_SHUTDOWN_NOTICE					(BYTE)0x00		// 셧다운 공지
	#define PUBLISHER_COMMAND_SHUTDOWN_DISCONNECT				(BYTE)0x01		// 셧다운 접속 종료
	#define PUBLISHER_COMMAND_SHUTDOWN_KICKOUT					(BYTE)0x02		// 셧다운 특정 유저 접속 종료

//@redpixel {17-08-11} Match->Agent 전송 메시지. 사용자 상태를 전달해서 User::user_status 값을 update. (예: InLobbyOrMatchroom. inGame. LoggingOrExiting)
#define FRAMEWORKMSG_NOTIFY_USER_STATUS									MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x0322)

// @redpixel {17-08-21} blade->agent 로 전송. 새로 등록된 nick을 알려주기 위함. (베틀아이에서 필요)
#define FRAMEWORKMSG_LOBBY_JOB											MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x0323)

#define FRAMEWORKMSG_CLIENT_PROFILE										MAKE_FRAMEWORKMSG( MSG_FLAG_IDFIELD_NODIR, 0x0324)	

// End of Billing Framework Msg.
//--------------------------------------------------------------------------------------------------------------

/*
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
	0x0A00 부터는 다른 데서 사용하고 있으니 절대 사용하지 말것!!!!!!!!!!!!!
*/

