
#define	MSG_FLAG_IDFIELD_TOOL	(WORD)0x3100
#define MAKE_TOOLMESSAGE(dir, index)	(MSG_FLAG_IDFIELD_TOOL | (dir) | index)
#define REG_MSG_HANDLE(classname, msgid) m_MsgHandle[msgid] = (MSG_HANDLE)&classname::On##msgid