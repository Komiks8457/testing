#pragma once

#include "ChunkAllocator.h"
#include "../NetEngine/Msg.h"

template <class T>
class CChunkAllocatorMT_For_CMsg : public ChunkAllocatorMT<T>
{
public:
	CChunkAllocatorMT_For_CMsg(long nChunkSize = DEFAULT_CHUNK_SIZE, INIT_CALLBACK lpInitializer = NULL, DWORD dwParam = 0, LPCTSTR lpszName = NULL)
		 : ChunkAllocatorMT<T>(nChunkSize, lpInitializer, dwParam, lpszName) 
	{
	}

protected:
	BOOL AllocItemBlock()
	{
		if (!m_FreeItemQueue.IsEmpty())
		{
			return FALSE;
		}		
		
		CMsg* pItemBlock = new CMsg[m_nChunkSize];
		if (pItemBlock)
		{
			m_ItemChunkArray.push_back(pItemBlock);
			
			for (int i = 0; i < m_nChunkSize; i++)
				m_FreeItemQueue.Enque(&pItemBlock[i]);
			
			m_nCapacity = m_ItemChunkArray.size() * m_nChunkSize;

			return TRUE;
		}
		
		return FALSE;
	}

	CMsg* ThereIsNoFreeItem()
	{
		AllocItemBlock();
		return PopFreeItem();
	}
	
	CMsg* PopFreeItem()
	{
		CMsg* pT = NULL;
		
		while (1)
		{
			m_FreeItemQueue.Deque(pT);
			if (pT == NULL)
				break;
			
			if (pT->IsInUse())
			{
				_ASSERT( FALSE );
				continue;
			}

			pT->Reset(0);
			pT->SetMsgInUse(TRUE);

#ifdef _DUMP_ALLOCATED_MSG
			// dump msg usage
			m_AllocatedMsgs.insert(pT);
#endif
			break;
		}

		return pT;
	}

public:
	CMsg* NewItem()
	{
		SCOPED_LOCK_SINGLE(&m_CS);

		T* pT = PopFreeItem();
		if (pT == NULL)
			pT = ThereIsNoFreeItem();

		++m_nAllocatedItemNum;

		return pT;
	}

	void FreeItem(CMsg* pT)
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		
		if (!pT)
			return;
		
		if (pT->IsInUse() == FALSE)
			return;

		pT->SetMsgInUse(FALSE);
		m_FreeItemQueue.Enque(pT);	

		--m_nAllocatedItemNum;

#ifdef _DUMP_ALLOCATED_MSG
		MSGLIST::iterator it = m_AllocatedMsgs.find(pT);
		if (it == m_AllocatedMsgs.end())
			return;

		m_AllocatedMsgs.erase(it);
#endif
	}

protected:
	typedef std::set<CMsg*>	MSGLIST;
	MSGLIST		m_AllocatedMsgs;

public:
	void Dump(int count = -1)
	{
		typedef std::map<DWORD, DWORD>	DWMAP;
		DWMAP EachMsgCount;
		
		DWORD id;
		if (m_lpfDumpCallback != NULL)
		{
			//SCOPED_LOCK_SINGLE(&m_CS);

			CMsg* pMsg = NULL;
			DWMAP::iterator it_count;
			for (MSGLIST::iterator it = m_AllocatedMsgs.begin(); it != m_AllocatedMsgs.end(); ++it)
			{
				pMsg = (*it);

				id = pMsg->GetMsgID();

				if (id == 0x2209) // FRAMEWORKMSG_RELAY_MSG_TO_CLIENT_SINGLE
				{
					int offset = MSG_HEADER_SIZE + 4; // skip (header size + client session id)
					BYTE* pBuffer = pMsg->GetBufferAt(offset);

					id = *(WORD*)pBuffer;
				}

				it_count = EachMsgCount.find(id);
				if (it_count == EachMsgCount.end())
					EachMsgCount.insert(DWMAP::value_type(id, 1));
				else
					(*it_count).second++;
			}

			int total = 0;
			for (it_count = EachMsgCount.begin(); it_count != EachMsgCount.end(); ++it_count)
			{
				(m_lpfDumpCallback)((*it_count).first, (*it_count).second);
				total += ((*it_count).second);
			}

			(m_lpfDumpCallback)(0xffff, total);
		}
	}

};

