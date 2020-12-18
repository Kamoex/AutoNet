#pragma once
#include "TypeDef.h"

namespace AutoNet
{
#pragma pack(push, 1)
    struct MsgHead
    {
        INT m_nMsgID;
        INT m_nMsgLen;

        void Clear()
        {
            m_nMsgID = 0;
            m_nMsgLen = 0;
        }
    };
#pragma pack(pop)
}
