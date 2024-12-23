#define _CRT_SECURE_NO_WARNINGS 1
#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"


SRRdtSender::SRRdtSender() :expectSequenceNumberSend(0), waitingState(false), base(0), winlen(4), buflen(8)
{
}


SRRdtSender::~SRRdtSender()
{
}



bool SRRdtSender::getWaitingState() {
	//如果窗口满了就改变waitingState为等待ack
	if (window.size() == winlen) {
		waitingState = true;
	}
	else {
		waitingState = false;
	}
	return waitingState;
}




bool SRRdtSender::send(const Message& message) {
	//如果Sender方窗口已满，拒绝发送请求
	if (this->getWaitingState()) {
		return false;
	}

	//准备发送包
	this->packetWaitingAck.acknum = -1; //acknum在发送包中不使用
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;//设置为要发送的序号
	this->packetWaitingAck.checksum = 0; //初始化校验和为0

	//将payload拷贝到发送包中
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));

	//计算checksum
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);

	//构造一个临时的结构去标记未收到ack的包
	waitPck tempPck;
	tempPck.flag = false;
	tempPck.winPck = packetWaitingAck;

	//将其放入等待ack序号窗口队列
	window.push_back(tempPck);

	//打印发送信息
	pUtils->printPacket("发送方——发送报文", this->packetWaitingAck);

	//启动发送方定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);

	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);

	//更新期待发包信号
	this->expectSequenceNumberSend = (this->expectSequenceNumberSend + 1) % this->buflen;

	return true;
}

void SRRdtSender::receive(const Packet& ackPkt) {
		//检查校验和是否正确
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		// 计算确认序号相对于发送窗口起始位置的偏移量
		int offseqnum = (ackPkt.acknum - this->base + this->buflen) % this->buflen;


		//如果校验和正确，且该确认号在发送窗口的未确认数据包范围内
		if (checkSum == ackPkt.checksum && offseqnum < window.size() && window.at(offseqnum).flag == false) {
			//标记收到的ACK，确认该包已经被接收方确认
			window.at(offseqnum).flag = true;
			//停止该确认号对应的数据包的定时器
			pns->stopTimer(SENDER, ackPkt.acknum);

			//打印当前发送方窗口的状态（接收到ACK之前）
			printf("发送方——正确收到确认，滑动前窗口:[ ");
			for (int i = 0; i < this->winlen; i++) {
				if (i < window.size()) {
					if (window.at(i).flag == true)
						printf("%d-Y ", (this->base + i) % buflen);  // Y表示该位置的包已被确认
					else
						printf("%d-N ", (this->base + i) % buflen);  // N表示该位置的包尚未被确认
				}
				else {
					printf("%d ", (this->base + i) % buflen);  // 空白位置
				}
			}
			printf("]\n");  // 窗口状态打印结束

			//打印已正确收到的确认包的信息
			pUtils->printPacket("发送方——正确收到确认", ackPkt);

			//如果窗口队列中最前面的包已经被确认，滑动窗口
			while (!window.empty() && window.front().flag == true) {
				window.pop_front();  // 移除已确认的包
				this->base = (this->base + 1) % this->buflen;  // 滑动窗口
			}

			//打印滑动窗口后的状态
			printf("发送方——正确收到确认，滑动后窗口:[ ");
			for (int i = 0; i < this->winlen; i++) {
				if (i < window.size()) {
					if (window.at(i).flag == true)
						printf("%d-Y ", (this->base + i) % buflen);
					else
						printf("%d-N ", (this->base + i) % buflen);
				}
				else {
					printf("%d ", (this->base + i) % buflen);
				}
			}
			printf("]\n");  // 滑动窗口后的状态打印结束

		}
		//如果校验和不正确，说明数据包被损坏
		else if (checkSum != ackPkt.checksum) {
			pUtils->printPacket("发送方——没有正确收到该报文确认，数据校验错误", ackPkt);
		}
		//如果确认包已被接收过，说明是重复的ACK
		else {
			pUtils->printPacket("发送方——已正确收到过该报文确认", ackPkt);
		}
}

void SRRdtSender::timeoutHandler(int seqNum) {
	//计算超时的序号相对于窗口起始位置的偏移量
	int offseqnum = (seqNum - this->base + this->buflen) % this->buflen;

	//首先停止超时序号对应的数据包的定时器
	pns->stopTimer(SENDER, seqNum);

	//重新启动定时器，以便再次监测该数据包的超时
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);

	//重新发送该超时的数据包到接收方
	pns->sendToNetworkLayer(RECEIVER, window.at(offseqnum).winPck);

	//打印关于重发操作的调试信息
	pUtils->printPacket("发送方——定时器超时，重发报文", window.at(offseqnum).winPck);

}
