#pragma once

#define IP_MAX 64
#define INVALID_VALUE -1
#define SUCCESS_VALUE 0

namespace AutoNet
{
	enum ENetRes
	{
        E_NET_INVALID_VALUE = -1,
        E_NET_SUC = 0,                  // �ɹ�
        E_NET_NONE_ERR,	                // û�ж�Ӧ�������߼�
        E_NET_BUF_OVER_FLOW,	        // ��������
        E_NET_DISCONNECTED,	            // �Ͽ�����
        E_NET_DISCONNECTED_HDL_FAILED,	// �Ͽ����Ӵ���ʧ��
        E_NET_SEND_FAILED,	            // ����ʧ��
        E_NET_KICK_FAILED,	            // �ߵ�ʧ��
    };
}