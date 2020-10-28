#pragma once
#include "TypeDef.h"

namespace AutoNet
{
#pragma pack(push, 1)
    struct MsgHead
    {
        INT m_nLen;

        void Clear()
        {
            m_nLen = 0;
        }
    };
#pragma pack(pop)
}
