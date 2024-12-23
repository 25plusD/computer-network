#ifndef GBN_RDT_SENDER_H
#define GBN_RDT_SENDER_H
#include "RdtSender.h"
#include "deque"
class GBNRdtSender :public RdtSender
{
private:
	int expectSequenceNumberSend;	// 下一个发送序号 
	bool waitingState;				// 是否处于等待Ack的状态
	Packet packetWaitingAck;		// 已发送并等待Ack的数据包
	int base;                       // 窗口开始位置序号
	int winlen;                     // 窗口长度大小
	int buflen;                     // 发送缓冲区宽度
	deque<Packet> window;           // 窗口队列

public:

	bool getWaitingState();					//返回RdtSender是否处于等待状态，如果发送方正等待确认或者发送窗口已满，返回true
	bool send(const Message& message);		//发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(const Packet& ackPkt);		//接受确认Ack，将被NetworkServiceSimulator调用	
	void timeoutHandler(int seqNum);		//Timeout handler，将被NetworkServiceSimulator调用

public:
	GBNRdtSender();
	virtual ~GBNRdtSender();
};

#endif

