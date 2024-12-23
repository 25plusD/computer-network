#ifndef SR_RDT_SENDER_H
#define SR_RDT_SENDER_H
#include "RdtSender.h"
#include <deque>
struct waitPck {
	bool flag;     //��δ���յ�ACKʱ��Ϊfalse
	Packet winPck;
};

class SRRdtSender :public RdtSender
{
private:
	int expectSequenceNumberSend;	// ��һ��������� 
	bool waitingState;				// �Ƿ��ڵȴ�Ack��״̬
	int base;                       // ��ǰ���ڻ����
	int winlen;                     // ���ڴ�С
	int buflen;                     // ���������
	deque<waitPck> window;          // �ȴ�ack��Ŵ��ڶ���
	Packet packetWaitingAck;		// �ѷ��Ͳ��ȴ�Ack�����ݰ�

public:

	bool getWaitingState();
	bool send(const Message& message);					//����Ӧ�ò�������Message����NetworkServiceSimulator����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	void receive(const Packet& ackPkt);					//����ȷ��Ack������NetworkServiceSimulator����	
	void timeoutHandler(int seqNum);					//Timeout handler������NetworkServiceSimulator����

public:
	SRRdtSender();
	virtual ~SRRdtSender();
};

#endif


