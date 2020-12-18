#pragma once

#define IP_MAX 64
#define INVALID_VALUE -1
#define SUCCESS_VALUE 0

namespace AutoNet
{
	enum ENetRes
	{
        E_NET_INVALID_VALUE = -1,
        E_NET_SUC = 0,                  // 成功
        E_NET_NONE_ERR,	                // 没有对应错误处理逻辑
        E_NET_BUF_OVER_FLOW,	        // 超出缓存
        E_NET_DISCONNECTED,	            // 断开连接
        E_NET_DISCONNECTED_HDL_FAILED,	// 断开连接处理失败
        E_NET_SEND_FAILED,	            // 发送失败
        E_NET_KICK_FAILED,	            // 踢掉失败
    };
}