#ifndef SR_RDT_RECEIVER_H
#define SR_RDT_RECEIVER_H
#include "RdtReceiver.h"
#include<deque>
struct rcvPck {
	bool flag;       //ָʾ��λ���Ƿ�ռ�ã�ture��ʾռ��
	Packet winPck;   //������ݰ�
};

class SRRdtReceiver :public RdtReceiver
{
private:
	int expectSequenceNumberRcvd;	//�ڴ��յ�����һ���������
	int base;                       //��ǰ���ڻ����
	int winlen;                     //���ڴ�С
	int buflen;                     //���������
	deque<rcvPck> window;           //��������
	Packet lastAckPkt;				//�ϴη��͵�ȷ�ϱ���

public:
	SRRdtReceiver();
	virtual ~SRRdtReceiver();

public:

	void receive(const Packet& packet);	//���ձ��ģ�����NetworkService����
};


#endif
