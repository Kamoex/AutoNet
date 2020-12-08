#pragma once
#include <iostream>
#include <string.h>
#include "TypeDef.h"

namespace AutoNet
{
    class RingBuffer
    {
    public:
        RingBuffer(const DWORD nSize)
        {
            m_pBuf = new CHAR[nSize];
            memset(m_pBuf, 0, nSize);
            m_pEnd = m_pBuf + nSize;
            m_pRead = m_pBuf;
            m_pWrite = m_pBuf;
            m_nMaxSize = nSize;

            m_dwUnReadSize = 0;
            m_dwUnWriteSize = nSize;
        }

        ~RingBuffer()
        {
            if (m_pBuf)
            {
                delete m_pBuf;
                m_pBuf = NULL;
            }
            m_pEnd = NULL;
            m_pRead = NULL;
            m_pWrite = NULL;
            m_dwUnReadSize = 0;
            m_dwUnWriteSize = 0;
        }

        BOOL Write(CHAR* pBuffer, const DWORD dwSize)
        {
            if (!pBuffer || dwSize == 0)
                return FALSE;

            if (m_dwUnWriteSize <= 0)
                return FALSE;
            
            if (dwSize > m_dwUnWriteSize)
                return FALSE;

            if (m_pWrite + dwSize > m_pEnd)
            {
                INT64 nFirstSize = m_pEnd - m_pWrite;
                INT64 nSecondSize = dwSize - nFirstSize;
                memcpy(m_pWrite, pBuffer, nFirstSize);
                memcpy(m_pBuf, pBuffer + nFirstSize, nSecondSize);
                m_pWrite = m_pBuf + nSecondSize;
            }
            else
            {
                memcpy(m_pWrite, pBuffer, dwSize);
                m_pWrite = m_pWrite + dwSize;
            }

            m_dwUnWriteSize -= dwSize;
            m_dwUnReadSize += dwSize;
            return true;
        }

        BOOL Read(CHAR* pBuf, const DWORD nSize)
        {
            if (!pBuf || nSize == 0)
                return FALSE;

            if (m_dwUnReadSize <= 0)
                return FALSE;

            if (nSize > m_dwUnReadSize)
                return FALSE;

            if (m_pRead + nSize > m_pEnd)
            {
                INT64 nFirstSize = m_pEnd - m_pRead;
                INT64 nSecondSize = nSize - nFirstSize;
                memcpy(pBuf, m_pRead, nFirstSize);
                memcpy(pBuf + nFirstSize, m_pBuf, nSecondSize);
                m_pRead = m_pBuf + nSecondSize;
            }
            else
            {
                memcpy(pBuf, m_pRead, nSize);
                m_pRead = m_pRead + nSize;
            }

            m_dwUnReadSize -= nSize;
            m_dwUnWriteSize += nSize;
            return true;
        }

        // 用于直接获取了内存写入的情况
        BOOL SkipWrite(DWORD dwSize)
        {
            if (dwSize == 0)
                return FALSE;

            if (m_dwUnWriteSize <= 0)
                return FALSE;

            if (dwSize > m_dwUnWriteSize)
                return FALSE;

            if (m_pWrite + dwSize > m_pEnd)
            {
                INT64 nFirstSize = m_pEnd - m_pWrite;
                INT64 nSecondSize = dwSize - nFirstSize;
                m_pWrite = m_pBuf + nSecondSize;
            }
            else
            {
                m_pWrite = m_pWrite + dwSize;
            }

            m_dwUnWriteSize -= dwSize;
            m_dwUnReadSize += dwSize;
            return true;
        }

        CHAR* GetWritePos(DWORD& uLen)
        {
            if (m_dwUnWriteSize == 0)
            {
                uLen = 0;
                return NULL;
            }

            if (m_pWrite == m_pEnd)
            {
                m_pWrite = m_pBuf;
            }

            INT64 nLen = 0;
            if (m_pWrite < m_pRead)
                nLen = m_pRead - m_pWrite;
            else
                nLen = m_pEnd - m_pWrite;

            uLen = (DWORD)nLen;
            return m_pWrite;
        }

        void Clear()
        {
            memset(m_pBuf, 0, m_nMaxSize);
            m_pRead = m_pBuf;
            m_pWrite = m_pBuf;
            m_dwUnReadSize = 0;
            m_dwUnWriteSize = m_nMaxSize;
        }

        CHAR* GetBuf()
        {
            return m_pBuf;
        }

        DWORD GetUnReadSize()
        {
            return m_dwUnReadSize;
        }

    private:
        CHAR* m_pBuf;
        CHAR* m_pRead;
        CHAR* m_pWrite;
        CHAR* m_pEnd;
        DWORD m_dwUnReadSize;
        DWORD m_dwUnWriteSize;
        DWORD m_nMaxSize;
    };
}