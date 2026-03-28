#pragma once

#define	MSG_FLAG_IDFIELD_SERVICEMANAGER			(WORD)0x3000
#define MAKE_SMMSG( dir, index)	(MSG_FLAG_IDFIELD_SERVICEMANAGER | (dir) | index)

#define SMMSG_VERSION		(DWORD)0x18

// service manager는 module패치없이 protocol version만으로 인증하겠다

#define SMMSG_LOGIN_REQ		MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x001)
// << (STRING)szUserID << (STRING)szPassword
#define SMMSG_LOGIN_ACK		MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x001)
// >> (BYTE)btResult >> (BYTE)btErrorCode
#	define SMMSG_LOGIN_FAIL_WRONG_VERSION			(BYTE)0x01
#	define SMMSG_LOGIN_FAIL_INVALID_USERINFO		(BYTE)0x02
#	define SMMSG_LOGIN_FAIL_SERVER_INTERNAL_ERROR	(BYTE)0x03
#	define SMMSG_LOGIN_FAIL_ACCESS_DENIED			(BYTE)0x04

#define SMMSG_SERVER_ARCHITECTURE_INFO_REQ	MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x002)
#define SMMSG_SERVER_ARCHITECTURE_INFO_ACK	MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x002)

#define SMMSG_MODULEVERSION_REQ		MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x003)
#define SMMSG_MODULEVERSION_ACK		MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x003)

#define SMMSG_MODULEVERSIONFILE_REQ	MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x004)
// << (BYTE)nModuleID << (BYTE)nContentID
#define SMMSG_MODULEVERSIONFILE_ACK	MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x004)
// 

#define SMMSG_FACULTY_TEST_REQ			MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x005)
//
#define SMMSG_FACULTY_TEST_ACK			MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x005)
// >> (BYTE)btResult

#ifdef LAUNCH_TEEN_SERVER
#define SMMSG_LAUNCH_TEEN_SERVER_REQ		MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x020)
#endif // LAUNCH_TEEN_SERVER


#define SMMSG_LAUNCH_SERVER_REQ			MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x006)
// << (WORD)nServerID

#define SMMSG_LAUNCH_SERVER_ACK			MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x006)
// >> (BYTE)btResult >> (BYTE)btErrorCode >> (WORD)nServerID 
#	define SMMSG_LAUNCH_SERVER_FAIL_RELAY			(BYTE)0x03
#	define SMMSG_LAUNCH_SERVER_FAIL_INVALID_SERVER	(BYTE)0x04

#define SMMSG_SHUTDOWN_SERVER_REQ		MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x007)
// << (WORD)nServerID
#define SMMSG_SHUTDOWN_SERVER_ACK		MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x007)
// >> (BYTE)btResult >> (BYTE)btErrorCode >> (WORD)nServerID
#	define SMMSG_SHUTDOWN_SERVER_FAIL_RELAY				(BYTE)0x03
#	define SMMSG_SHUTDOWN_SERVER_FAIL_INVALID_SERVER	(BYTE)0x04

#define SMMSG_OPERATE_SERVICE_REQ		MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x008)
// << (BYTE)nOperate << (WORD)nServerID
#define SMMSG_OPERATE_SERVICE_ACK		MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x008)
// >> (BYTE)btResult >> (BYTE)btErrorCode >> (BYTE)nOperate >> (WORD)nServerID

#define SMMSG_QUERY_MACHINE_STATS_REQ	MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x009)
//
#define SMMSG_QUERY_MACHINE_STATS_ACK	MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x009)
//
#	define QUERY_MACHINE_STATS_FAIL_INVALID_SERVER			(BYTE)0x01
#	define QUERY_MACHINE_STATS_FAIL_RELAY					(BYTE)0x02
#	define QUERY_MACHINE_STATS_FAIL_MACHINEMANAGER_ERROR	(BYTE)0x03

#define SMMSG_CHANGE_SHARD_DATA_REQ		MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x010)
#define SMMSG_CHANGE_SHARD_DATA_ACK		MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x010)

#define SMMSG_CHANGE_SHARD_SERVICE_TYPE_REQ	MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x011)
#define SMMSG_CHANGE_SHARD_SERVICE_TYPE_ACK MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x011)

#define SMMSG_QUERY_SHARD_SERVICE_TYPE_REQ MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x012)
#define SMMSG_QUERY_SHARD_SERVICE_TYPE_ACK MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x012)

/*
	1) GlobalManager한테 화일 목록을 보냄
	2) GlobalManager가 화일을 받음
	3) GlobalManager가 화일을 받을
	4) GlobalManager가 Relay시작 한다고 알려줌
	5) GlobalManager가 끝났다고 알려줌
*/

#define SMMSG_PATCH_START_UPLOAD_REQ					MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x100)
#define SMMSG_PATCH_START_UPLOAD_ACK					MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x100)

#define SMMSG_PATCH_NOTIFY							MAKE_SMMSG( MSG_FLAG_IDFIELD_NODIR, 0x101)
#	define PATCH_NOTIFY_MASK_RELAY							(BYTE)0x01// relay관련 notify
#		define PATCH_NOTIFY_RELAY_START							(BYTE)0x01
#		define PATCH_NOTIFY_RELAY_PROGRESS						(BYTE)0x02
#	define PATCH_NOTIFY_MASK_RECORDING_DB					(BYTE)0x02// recording관련 notify
#		define PATCH_NOTIFY_RECORDING_DB_START					(BYTE)0x01
#		define PATCH_NOTIFY_RECORDING_DB_PROGRESS				(BYTE)0x02

#define SMMSG_PATCH_END_UPLOAD						MAKE_SMMSG( MSG_FLAG_IDFIELD_NODIR, 0x102)
// >> (BYTE)result >> (BYTE)error_code
#	define PATCH_UPLOAD_FAIL_RELAYSERVER_OFFLINE		(BYTE)0x01		// relay받을 서버가 offline이다
#	define PATCH_UPLOAD_FAIL_RELAY_FILE_TRANSFER		(BYTE)0x02		// 화일 전송중에 오류가 발생했다
#	define PATCH_UPLOAD_FAIL_SERVICE_OPERATING			(BYTE)0x03		// gateway나 shard가 켜져 있다
#	define PATCH_UPLOAD_FAIL_RECORDING_DB				(BYTE)0x04		// version file을 DB에 쓰다가 오류가 났다

#define SMMSG_REQUEST_BLOCKED						MAKE_SMMSG( MSG_FLAG_IDFIELD_NODIR, 0x103)
#	define REQUEST_BLOCKED_PATCH_INPROGRESS				(BYTE)0x01		// 패치 중이다
#	define REQUEST_BLOCKED_PERMISSION_DENIED			(BYTE)0x02		// 넌 그럴 자격 없다

#define SMMSG_PREPARE_PATCH_REQ						MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x104)
#define SMMSG_PREPARE_PATCH_ACK						MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x104)
#	define PREPARE_PATCH_FAIL_ANOTHER_USER_PATCH_IN_PROGRESS (BYTE)0x01	// 다른 SMC에서 패치를 진행 중이다
#	define PREPARE_PATCH_FAIL_FRONT_SERVER_PREPARE_FAIL		 (BYTE)0x02	// DownloadServer, GatewayServer준비가 않됐다
#	define PREPARE_PATCH_FAIL_DB_ERROR						 (BYTE)0x03 // 최신 버전을 긁어 오지 못했다

// account관리 메세지

// 요건 동접자 관리
#define SMMSG_CONCURRENT_USER_QUERY_REQ				MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x200)
#define SMMSG_CONCURRENT_USER_QUERY_ACK				MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x200)
#	define CONCURRENT_USER_QUERY_FAIL_NODATA				(BYTE)0x01

#define SMMSG_CONCURRENT_USER_BAN_REQ					MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x201)	// 유저 내보내기
#define SMMSG_CONCURRENT_USER_BAN_ACK					MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x201)
#	define CONCURRENT_USER_BAN_FAIL_CANNOT_FIND_USER			(BYTE)0x01
#	define CONCURRENT_USER_BAN_FAIL_RELAY_FAILED				(BYTE)0x02

// 공시사항 refresh
#define SMMSG_REFRESH_ARTICLE_REQ					MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x202)
#define SMMSG_REFRESH_ARTICLE_ACK					MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x202)

// shard manager와 직접 통신
#define SMMSG_SHARD_STARTUP_SERVER_MSG_REQ			MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x203)
#define SMMSG_SHARD_STARTUP_SERVER_MSG_ACK			MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x203)

//
#define SMMSG_CONCURRENT_USER_STATISTICS_REQ					MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x204)
#define SMMSG_CONCURRENT_USER_STATISTICS_ACK					MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x204)

#define SMMSG_REFRESH_FRANCHISE_IP_DATA_REQ						MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x205)
#define SMMSG_REFRESH_FRANCHISE_IP_DATA_ACK						MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x205)

//----------- article 직접 편집 관련
#define SMMSG_QUERY_ARTICLE_REQ									MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x206)
#define SMMSG_QUERY_ARTICLE_ACK									MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x206)

#define SMMSG_ADD_ARTICLE_REQ									MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x207)
#define SMMSG_ADD_ARTICLE_ACK									MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x207)

#define SMMSG_DELETE_ARTICLE_REQ								MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x208)
#define SMMSG_DELETE_ARTICLE_ACK								MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x208)

#define SMMSG_EDIT_ARTICLE_REQ									MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x209)
#define SMMSG_EDIT_ARTICLE_ACK									MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x209)

//--------------------------------- security 관련
//
#define SMMSG_ADD_SECURITY_DESCRIPTION_REQ					MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x300)
#define SMMSG_ADD_SECURITY_DESCRIPTION_ACK					MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x300)

#define SMMSG_DELETE_SECURITY_DESCRIPTION_REQ				MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x301)
#define SMMSG_DELETE_SECURITY_DESCRIPTION_ACK				MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x301)

#define SMMSG_ADD_SECURITY_DESCRIPTION_GROUP_REQ			MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x302)
#define SMMSG_ADD_SECURITY_DESCRIPTION_GROUP_ACK			MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x302)

#define SMMSG_DELETE_SECURITY_DESCRIPTION_GROUP_REQ			MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x303)
#define SMMSG_DELETE_SECURITY_DESCRIPTION_GROUP_ACK			MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x303)

#define SMMSG_ADD_SECURITY_DESCRIPTION_GROUP_ASSIGN_REQ		MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x304)
#define SMMSG_ADD_SECURITY_DESCRIPTION_GROUP_ASSIGN_ACK		MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x304)

#define SMMSG_DELETE_SECURITY_DESCRIPTION_GROUP_ASSIGN_REQ	MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x305)
#define SMMSG_DELETE_SECURITY_DESCRIPTION_GROUP_ASSIGN_ACK	MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x305)

#define SMMSG_REFRESH_SECURITY_DATA							MAKE_SMMSG( MSG_FLAG_IDFIELD_NODIR, 0x306)

#define SMMSG_MODIFY_SECURITY_DESCRIPTION_REQ				MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x307)
#define SMMSG_MODIFY_SECURITY_DESCRIPTION_ACK				MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x307)

#define SMMSG_LAZED_SHUTDOWN_SERVER_REQ		MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x308)
#define SMMSG_LAZED_SHUTDOWN_SERVER_ACK		MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x308)
#	define SMMSG_LAZED_SHUTDOWN_SERVER_FAIL_RELAY				(BYTE)0x03
#	define SMMSG_LAZED_SHUTDOWN_SERVER_FAIL_INVALID_SERVER	(BYTE)0x04


// db에 저장된걸 긁어다가 준다
#define SMMSG_QUERY_USER_REQ								MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x400)
#define SMMSG_QUERY_USER_ACK								MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x400)
#	define SMMSG_QUERY_USER_FAIL_DB_ERROR			(BYTE)0x01

#define SMMSG_ADD_USER_REQ								MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x401)
#define SMMSG_ADD_USER_ACK								MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x401)
#	define SMMSG_ADD_USER_FAIL_SAME_NAME_EXIST		(BYTE)0x01

#define SMMSG_DELETE_USER_REQ							MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x402)
#define SMMSG_DELETE_USER_ACK							MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x402)
#	define SMMSG_DELETE_USER_FAIL_CANNOT_FIND_USER	(BYTE)0x01

#define SMMSG_MODIFY_USER_REQ							MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x403)
#define SMMSG_MODIFY_USER_ACK							MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x403)

#define SMMSG_QUERY_SMLOG_REQ							MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x404)
#define SMMSG_QUERY_SMLOG_ACK							MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x404)

// 
/*
	쉩따박 졸라 햇갈리지만. 일단 적어보자

	ServiceManager계정 및 사내 테스트 계정 : Division의 _User테이블
	
	게임 계정중에 사내 테스트 계정 : Division의 _User테이블

	게임 계정중에 서비스 중인 계정(일명 유통사 계정) : AccountDB의 sr_member1 (또는 customizing) 헉

*/
#define SMMSG_QUERY_CONTENT_USER_ACCOUNT_REQ			MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x405)
#	define QUERY_CONTENT_USER_ACCOUNT_CONDITION_NORMAL	(BYTE)0x01
#	define QUERY_CONTENT_USER_ACCOUNT_CONDITION_BLOCKED	(BYTE)0x02
#	define QUERY_CONTENT_USER_ACCOUNT_CONDITION_BLOCK_EXPIRED (BYTE)0x03
#define SMMSG_QUERY_CONTENT_USER_ACCOUNT_ACK			MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x406)

#define SMMSG_QUERY_PUNISHMENT_REQ						MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x407)
#define SMMSG_QUERY_PUNISHMENT_ACK						MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x407)

// 추가, update (삭제는 불가)
#define SMMSG_PUNISHMENT_JOB_REQ						MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x408)
#define SMMSG_PUNISHMENT_JOB_ACK						MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x408)

#	define PUNISHMENT_JOB_FAIL_DB_ERROR					(BYTE)0x01
#	define PUNISHMENT_JOB_FAIL_USER_ALREADY_BLOCKED		(BYTE)0x02	// 아직 징계가 풀리지 않았다
#	define PUNISHMENT_JOB_FAIL_INVALID_SERIALNO			(BYTE)0x03	// 변경할려는 징계의 일련 번호가 이상하다
#	define PUNISHMENT_JOB_FAIL_INVALID_JOB				(BYTE)0x04	// 멀 하라고?

#define SMMSG_QUERY_CHAR_OWNED_BY_USER_REQ				MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x409)
#define SMMSG_QUERY_CHAR_OWNED_BY_USER_ACK				MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x409)

#define SMMSG_QUERY_USER_BY_USERID_REQ					MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x410)
#define SMMSG_QUERY_USER_BY_USERID_ACK					MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x410)

// BY DEEPDARK. 유저 계정으로 JID를 받아온다... ㅠㅠ
#define	SMMSG_QUERY_USER_JID_REQ						MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x411)
#define	SMMSG_QUERY_USER_JID_ACK						MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x411)
#	define QUERY_USER_JID_FAIL_INTERNAL					(BYTE)0x00
#	define QUERY_USER_JID_FAIL_NOUSER					(BYTE)0x01

#define	SMMSG_QUERY_CONCURRENT_USER_LOG_REQ				MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x412)
#define	SMMSG_QUERY_CONCURRENT_USER_LOG_ACK				MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x412)
#	define QUERY_PERIOD_HOUR							(BYTE)0x00
#	define QUERY_PERIOD_DAY								(BYTE)0x01
#	define QUERY_PERIOD_WEEK							(BYTE)0x02
#	define QUERY_PERIOD_MONTH							(BYTE)0x03
#	define QUERY_STATISTICS								(BYTE)0x10



/////////////////////////////////////////////////////////////////////////////////////////////////////
// CAS 에서 사용하는 넘들... -_-

// -- CAS에서 대기 리스트를 갱신
#define SMMSG_CAS_QUERY_WAIT_LIST_REQ					MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x413)
#define SMMSG_CAS_QUERY_WAIT_LIST_ACK					MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x413)

// -- 아이템의 상태 및 내용을 변경 요청/알림(굳이 req/ack은 불필요.)
#define SMMSG_CAS_UPDATE_ITEM							MAKE_SMMSG( MSG_FLAG_IDFIELD_NODIR, 0x414)
#define SMMSG_CAS_UPDATE_ITEM_NOTIFY					MAKE_SMMSG( MSG_FLAG_IDFIELD_NODIR, 0x415)
#	define CAS_UPDATE_ADD_ITEM			(BYTE)0x00  // ITEM의 추가
#	define CAS_UPDATE_CATEGORY			(BYTE)0x01	// ITEM의 카타고리를 변경
#	define CAS_UPDATE_TAKE_CHARGE		(BYTE)0x02	// ITEM의 상태를 "처리중"으로 변경
#	define CAS_UPDATE_DROP_CHARGE		(BYTE)0x03	// ITEM의 상태를 "대기중"으로 변경
#	define CAS_UPDATE_UNABLE			(BYTE)0x04	// ITEM의 상태를 "처리불가"로 변경
#	define CAS_UPDATE_PROCESSED			(BYTE)0x05	// ITEM의 상태를 "처리완료"로 변경

// -- 유저와 GM간의 채팅 세션 생성
#define SMMSG_CAS_CHAT_SESSION_REQ						MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x416)
#define SMMSG_CAS_CHAT_SESSION_ACK						MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x416)
#	define CAS_CHAT_SESSION_INVITE		(BYTE)0x00	// 세션 생성하고, 유저를 초청
#	define CAS_CHAT_SESSION_CLOSE		(BYTE)0x01	// 세션을 닫음
#	define CAS_CHAT_SEND_MESSAGE		(BYTE)0x02	// 채팅메시지

// -- 유저로부터의 채팅 관련 변화 Notify.
#define SMMSG_CAS_CHAT_NOTIFY							MAKE_SMMSG( MSG_FLAG_IDFIELD_NODIR, 0x417)
#	define CAS_CHAT_SESSION_ACCEPT		(BYTE)0x00	// 유저가 세션 수락
#	define CAS_CHAT_SESSION_DENY		(BYTE)0x01	// 유저가 세션 거절
#	define CAS_CHAT_SESSION_CLOSED		(BYTE)0x02	// 유저가 세션 닫음
#	define CAS_CHAT_RECV_MESSAGE		(BYTE)0x03	// 유저의 채팅 메시지

#define SMMSG_CAS_DIRECTGMCALL_LOG_REQ					MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x418)
#define SMMSG_CAS_DIRECTGMCALL_LOG_ACK					MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x418)

#define SMMSG_CAS_SEARCH_REQ							MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x419)
#define SMMSG_CAS_SEARCH_ACK							MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x419)

#define SMMSG_CAS_STATISTICS_REQ						MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x41a)
#define SMMSG_CAS_STATISTICS_ACK						MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x41a)
#	define CAS_STATISTICS_YESTERDAY		(BYTE)0x00	// 전일 통계
#	define CAS_STATISTICS_LASTWEEK		(BYTE)0x01	// 7일간 통계
#	define CAS_STATISTICS_BY_HOUR		(BYTE)0x02	// 시간별 통계
#	define CAS_STATISTICS_BY_SHARD		(BYTE)0x03	// 샤드별 통계
#	define CAS_STATISTICS_BY_CATEGORY	(BYTE)0x04	// 신고 카테고리별 통계
#	define CAS_STATISTICS_BY_GM			(BYTE)0x05	// 처리 gm별 통계
#	define CAS_STATISTICS_BY_GM_PROCESS	(BYTE)0x06	// 처리 gm별 통계(처리시간 기준)

#define SMMSG_CAS_SEARCH_GMCALL_REQ						MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x41b)
#define SMMSG_CAS_SEARCH_GMCALL_ACK						MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x41b)
#	define CAS_GMCALL_SEARCH_BY_GM			(BYTE)0x00
#	define CAS_GMCALL_SEARCH_BY_CHAR		(BYTE)0x01
#	define CAS_GMCALL_SEARCH_BY_CAS_SERIAL	(BYTE)0x02

//////////////////////////////////////////////////////////////////////////
// for Regional IP Registry statistics
// by novice.
#define SMMSG_CONCURRENT_USER_RIR_STATISTICS_REQ		MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x41c)
#define SMMSG_CONCURRENT_USER_RIR_STATISTICS_ACK		MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x41c)
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// For Searching Login IP Informations
#define SMMSG_USER_LOGIN_IP_INFORMATION_REQ				MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x41D)
#define SMMSG_USER_LOGIN_IP_INFORMATION_ACK				MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x41D)
//////////////////////////////////////////////////////////////////////////

#ifdef __ENABLE_BRUTAL_IP_BLOCK__
#define SMMSG_QUERY_LOGIN_IP_LIST_REQ					MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x41e)
#define SMMSG_QUERY_LOGIN_IP_LIST_ACK					MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x41e)
	#define IPTYPE_PRIVILEGED		(BYTE)0
	#define IPTYPE_BLOCKED			(BYTE)1
#define SMMSG_UPDATE_LOGIN_IP_REQ						MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x41f)
#define SMMSG_UPDATE_LOGIN_IP_ACK						MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x41f)
	#define UPDATE_IP_INSERT		(BYTE)0
	#define UPDATE_IP_DELETE		(BYTE)1
#endif // __ENABLE_BRUTAL_IP_BLOCK__

//////////////////////////////////////////////////////////////////////////
// 이제 Reload 기능은 안씀
//#define SMMSG_CONCURRENT_USER_RIR_RELOAD_REQ			MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x42c)
//#define SMMSG_CONCURRENT_USER_RIR_RELOAD_ACK			MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x42c)
//////////////////////////////////////////////////////////////////////////

#ifdef APPLY_GNGWC_SYSTEM
#define SMMSG_QUERY_USERBLOCKED_REFRESH_REQ				MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x420)
#define SMMSG_QUERY_USERBLOCKED_REFRESH_ACK				MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x420)
#define TYPE_BLOCKEDUSER			(BYTE)0
#define TYPE_NONBLOCKEDUSER			(BYTE)11

#define SMMSG_QUERY_USERBLOCKED_BLOCKUSER_REQ			MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x421)
#define SMMSG_QUERY_USERBLOCKED_BLOCKUSER_ACK			MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x421)

#define SMMSG_QUERY_USERBLOCKED_NONBLOCKUSER_REQ		MAKE_SMMSG( MSG_FLAG_IDFIELD_REQMSG, 0x422)
#define SMMSG_QUERY_USERBLOCKED_NONBLOCKUSER_ACK		MAKE_SMMSG( MSG_FLAG_IDFIELD_ACKMSG, 0x422)
#endif // APPLY_GNGWC_SYSTEM

// gateway server와 주고 받기
#ifdef ALARM_PUNISHER
// Alarm type 유저의 대한 통지
#define SMMSG_ALARM_PUNISHER_NOTIFY						MAKE_SMMSG(MSG_FLAG_IDFIELD_DUMMYMASK, 0x423)
#endif // ALARM_PUNISHER


// 키키키키키
#pragma pack( push, 1)

struct _User
{
	DWORD m_dwID;
	char  m_szUserID[ 128];
	char  m_szPassword[ 32];
	char  m_szName[ 16];
	char  m_SSNumber1[ 32];
	char  m_SSNumber2[ 32];
	char  m_szOrganization[ 128];
	char  m_szRoleDesc[ 128];
	BYTE  m_nPrimarySecurityID;
	BYTE  m_nContentSecurityID;
	char  m_szGrantedSecurityIP[ 16];
};

struct _SMLog
{
	char		m_szUserID[ 128];
	BYTE		m_btCatagory;
	char		m_szLog[ 256];
	STIMESTAMP	m_EventTime;
};

struct _CashLog
{
	STIMESTAMP	m_EventTime;
	char		m_szItemCodeName[128];
	int			m_nPriceSilk;
	DWORD		m_nIP;
	LONGLONG	m_lSerial;
	//int			m_nCount;
	//char		m_szItemName[128];
};

struct _ConcurrentUserLog
{
	STIMESTAMP	m_timeBegin;
	STIMESTAMP	m_timeEnd;
	WORD		m_wShardID;
	int			m_nCount;
};

inline std::wstring MakeDateTimeString(STIMESTAMP& time)
{
	wchar_t buff[ MAX_PATH];
	if (SF_sprintf( buff, MAX_PATH, L"%d-%d-%d %02d:%02d:%02d %d", time.year, time.month, time.day, time.hour, time.minute, time.second, time.fraction) != S_OK)
	{
		ASSERT(FALSE);
	}
	return std::wstring( buff);
}

inline std::wstring MakeSimpleDateTimeString(STIMESTAMP& time)
{
	wchar_t buff[ MAX_PATH];
	if (SF_sprintf( buff, MAX_PATH, L"%d-%02d-%02d %02d:%02d:%02d", time.year, time.month, time.day, time.hour, time.minute, time.second) != S_OK)
	{
		ASSERT(FALSE);
	}
	return std::wstring( buff);
}

struct _ContentUserCurInfo // 이름을 짓는데도 한계가 있다. 한글 변수가 정말로 필요하다 썅
{
	DWORD		m_dwID;
#ifdef PMANG_CHANNELING_SERVICE
	char		m_szServiceCompany[32];
#endif // #ifdef PMANG_CHANNELING_SERVICE
	char		m_szUserID[ 128];
	BYTE		m_BlockType;
	STIMESTAMP	m_BlockBeginTime;
	STIMESTAMP	m_BlockEndTime;
};

struct _Punishment
{
	int			m_SerialNo;
	int			m_UserJID;
	BYTE		m_Type;
	char		m_Executor[128];
	short		m_Shard;
	char		m_CharName[64];
	char		m_CharInfo[256];
	char		m_PosInfo[ 64];
	char		m_Guide[512];
	char		m_Description[1024];
	STIMESTAMP	m_RaiseTime;
	STIMESTAMP	m_BlockStartTime;
	STIMESTAMP	m_BlockEndTime;
	STIMESTAMP	m_PunishTime;
	BYTE		m_Status;
};

#ifdef SMC_CHARMOVE_WITHIN_ACCOUNT
const int MAX_STATE = 8;
#endif //SMC_CHARMOVE_WITHIN_ACCOUNT

struct _CharOwnedUser
{
	WORD		m_ShardID;
#ifdef PMANG_CHANNELING_SERVICE
	char		m_szServiceCompany[32];
#endif // #ifdef PMANG_CHANNELING_SERVICE
	char		m_UserID[128];
#ifdef	 SMC_SR_USERDATA_SHOW_JID
	int			m_nJID;	
#endif //SMC_SR_USERDATA_SHOW_JID
	char		m_CharID[64];
#ifdef SMC_CHARMOVE_WITHIN_ACCOUNT
	char		m_szMoveState[MAX_STATE];
#endif //SMC_CHARMOVE_WITHIN_ACCOUNT
};

//
/*
	유저 정보가 필요한 모든 메시지에 이넘들이 따란간다.
	DivisionID를 통해서 구별 되겠다
*/
struct _AccountInfo_Yahoo
{
	char		m_szUserID[ 128];
	char		m_szName[ 20];
	char		m_szPostNum[ 15];
	char		m_szAddress1[ 150];
	char		m_szAddress2[ 150];
	char		m_szTelP[ 6];
	char		m_szTel[ 16];
	char		m_szMobile[ 16];
	char		m_szEMail[ 70];
};

struct _AccountInfo_China
{
	char		m_szUserID[ 25];
	BYTE		m_Status;
	BYTE		m_GMRank;
	char		m_szName[ 25];
	char		m_szEMail[ 50];
	char		m_szSex[ 2];
	char		m_szCertificateNum[30]; // 권한 필요
	char		m_szAddress[100];		// 권한 필요
	char		m_szPostCode[10];		// 권한 필요
	char		m_szPhone[20];
	char		m_szMobile[20];
};

struct _CAS_DATA
{
	int			nSerial;
	BYTE		btCategory;
	STIMESTAMP	dReportedTime;
	WORD		wShardID;
	DWORD		dwReporterJID;
	char		szReporter[64];
	char		szTargetUser[64];
	char		szMailAddr[40];
	char		szStatement[512];
	BYTE		btStatus;
	STIMESTAMP	dProcessedTime;
	char		szGM[20];
	char		szMemo[128];
	char		szAnswer[1024];
	BYTE		btUserChecked;

	char		szChatLog[4000];
};

struct _CAS_ANSWER_DATA
{
	int			nSerial;
	char		szGM[20];
	char		szAnswer[1024];
	STIMESTAMP	dProcessedTime;
};

struct _CAS_GMCALLLOG_DATA
{
	int			nSerial;
	char		szGM[20];
	WORD		wShardID;
	char		szCharacter[64];
	int			nCasSerial;
	char		szLog[4000];
	STIMESTAMP	dWritten;
};

struct _CAS_STATISTICS_CATEGORY
{
	BYTE		btCategory;
	int			nIssued;
	int			nNotProcessed;
	int			nProcessing;
	int			nProcessed;
	int			nUnableToProcess;	
};

struct _CAS_STATISTICS_PERIOD
{
	STIMESTAMP	dBegin;
	STIMESTAMP	dEnd;
	int			nIssued;
	int			nNotProcessed;
	int			nProcessing;
	int			nProcessed;
	int			nUnableToProcess;
};

struct _CAS_STATISTICS_SHARD
{
	WORD		wShardID;
	int			nIssued;
	int			nNotProcessed;
	int			nProcessing;
	int			nProcessed;
	int			nUnableToProcess;
};

struct _CAS_STATISTICS_GM
{
	char		szGM[20];
	int			nTotalProcessed;
	int			nProcessed;
	int			nUnableToProcess;
	int			nOver2Hour;
	int			nOver1Hour;
};

#pragma pack( pop)

inline SYSTEMTIME MakeSystemTime(STIMESTAMP& time)
{
	SYSTEMTIME result;
	ZeroMemory( &result, sizeof( SYSTEMTIME));

	result.wYear = time.year;
	result.wMonth = time.month;
	result.wDay = time.day;
	result.wHour = time.hour;
	result.wMinute = time.minute;
	result.wSecond = time.second;

	return result;
}

inline STIMESTAMP MakeDateTime(SYSTEMTIME& time)
{
	STIMESTAMP result;
	ZeroMemory( &result, sizeof( SYSTEMTIME));

	result.year	= time.wYear;
	result.month	= time.wMonth;
	result.day	= time.wDay;
	result.hour	= time.wHour;
	result.minute= time.wMinute;
	result.second= time.wSecond;

	return result;
}

#ifdef SEARCHING_USER_LOGIN_IP_INFO
struct _UserLoginIPInfo
{
	#ifdef SEARCHING_USER_LOGIN_IP_INFO_BUG_FIX
		char		szAccountName[128];
	#else
		char		szAccountName[64];
	#endif //#ifdef SEARCHING_USER_LOGIN_IP_INFO_BUG_FIX
	
	DWORD		dwIP;
	sDateTime	dBeginDate;
	sDateTime	dEndDate;
};
#endif // #ifdef SEARCHING_USER_LOGIN_IP_INFO

#ifdef __ENABLE_BRUTAL_IP_BLOCK__
struct _LoginIPInfo
{
	DWORD		Idx;
	char		szBlockBegin[16];
	char		szBlockEnd[16];
	char		szGM[64];
	sDateTime	dIssueDate;
	char		szISP[256];
	char		szDesc[512];
};
#endif

