#pragma once	
/*
//################################################
// for Message Handler
//################################################

//////////////////////////////////////////////////
// Message Map 선언
//////////////////////////////////////////////////
#define	DECLARE_SRMSG_HANDLER(_class_name)							\
protected:															\
	typedef void (_class_name::*MSG_HANDLER)(CMsg&);				\
	static MSG_HANDLER	_class_name##MsgHandler[MAX_SRMSG_COUNT];	\
	friend class _class_name##MsgMapper;

//////////////////////////////////////////////////
// Message Map Binding
//////////////////////////////////////////////////
#define BEGIN_SRMSG_MAP(_class_name, except_handler)					\
	_class_name::MSG_HANDLER _class_name::_class_name##MsgHandler[MAX_SRMSG_COUNT] = { NULL, }; \
	class _class_name##MsgMapper	\
	{								\
	public:							\
		_class_name##MsgMapper()	\
		{							\
			for (int i = 0; i < MAX_SRMSG_COUNT; ++i)					\
			{															\
				_class_name::_class_name##MsgHandler[i] = _class_name::except_handler;	\
			}

		
#define BIND_SRMSG_HANDLER(_class_name, _id, _handler)	\
		_class_name::_class_name##MsgHandler[(_id & SR_MSG_MASK)] = _class_name::_handler;


#define END_SRMSG_MAP(_class_name)	\
		}							\
	};								\
	_class_name##MsgMapper	g##_class_name##MsgMapper;

//////////////////////////////////////////////////
// Message Handler 호출
//////////////////////////////////////////////////
#define CALL_SRMSG_HANDLER(_class_name, _msg)		\
	static WORD wID;								\
	wID = (_msg->GetMsgID() & SR_MSG_MASK);			\
	((this)->*_class_name::_class_name##MsgHandler[wID])(*_msg);


//################################################
// for Event Handler
//################################################

//////////////////////////////////////////////////
// Message Map 선언
//////////////////////////////////////////////////
#define	DECLARE_SREVENT_HANDLER(_class_name, _T, _handler_count)		\
protected:																\
	typedef int (_class_name::*EVENT_HANDLER)(_T);						\
	static EVENT_HANDLER _class_name##EventHandler[_handler_count];		\
	friend class _class_name##EventMapper;

//////////////////////////////////////////////////
// Message Map Binding
//////////////////////////////////////////////////
#define BEGIN_SREVENT_MAP(_class_name, _handler_count)					\
	_class_name::EVENT_HANDLER _class_name::_class_name##EventHandler[_handler_count] = { NULL, }; \
	class _class_name##EventMapper	\
	{								\
	public:							\
		_class_name##EventMapper()	\
		{							


#define BIND_SREVENT_HANDLER(_class_name, _id, _handler)	\
		_ASSERT((_id) < (sizeof(_class_name::_class_name##EventHandler) / sizeof(_class_name::_class_name##EventHandler[0])));	\
		_class_name::_class_name##EventHandler[(_id)] = _class_name::_handler;



#define END_SREVENT_MAP(_class_name)	\
		}								\
	};									\
	_class_name##EventMapper	g##_class_name##EventMapper;

//////////////////////////////////////////////////
// Message Handler 호출
//////////////////////////////////////////////////
#define CALL_SREVENT_HANDLER(_class_name, _id, _data)							\
	_ASSERT((_id) < (sizeof(_class_name::_class_name##EventHandler) / sizeof(_class_name::_class_name##EventHandler[0])));	\
	if (((this)->*_class_name::_class_name##EventHandler[(_id)]) != NULL)		\
		return ((this)->*_class_name::_class_name##EventHandler[(_id)])(_data);	\
	else																		\
		return (this)->OnUnhandledEvent((_id), (_data));

*/
