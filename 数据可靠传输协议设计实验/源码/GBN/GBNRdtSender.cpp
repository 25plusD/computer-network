#include "stdafx.h"
#include "Global.h"
#include "GBNRdtSender.h"


GBNRdtSender::GBNRdtSender():expectSequenceNumberSend(0), waitingState(false), base(0), winlen(4), buflen(8)
{
}


GBNRdtSender::~GBNRdtSender()
{
}



bool GBNRdtSender::getWaitingState() {
	// 判断当前窗口是否已达到最大容量
	if (window.size() >= winlen) {
		waitingState = true;  // 窗口已满，进入等待状态
	}
	else {
		waitingState = false; // 窗口未满，可以继续发送
	}

	return waitingState;  // 返回当前的等待状态
}




bool GBNRdtSender::send(const Message& message) {
	// 检查发送方窗口是否满
	if (this->waitingState) {
		return false;  // 窗口满，无法发送新报文
	}

	// 构建待发送的数据包
	this->packetWaitingAck.acknum = -1;  // 设置ACK序号为-1，发送时不需要使用该字段
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;  // 设置序号
	this->packetWaitingAck.checksum = 0;  // 初始化校验和为0

	// 复制消息数据到数据包有效载荷
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));

	// 计算数据包的校验和
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);

	// 将待发送的包加入窗口队列
	window.push_back(this->packetWaitingAck);
	pUtils->printPacket("发送方——发送报文", this->packetWaitingAck);  // 打印发送的报文信息

	// 在窗口最前的包被发送时，启动定时器
	if (this->base == this->expectSequenceNumberSend) {
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->base);
	}

	// 通过网络层发送数据包到接收方
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);

	// 依据缓冲区长度更新下一个发送序号
	this->expectSequenceNumberSend = (this->expectSequenceNumberSend + 1) % this->buflen;

	return true;  // 数据包发送成功
}

void GBNRdtSender::receive(const Packet& ackPkt) {
    // 计算接收到的ACK报文的校验和
    int computedCheckSum = pUtils->calculateCheckSum(ackPkt);

    // 计算ACK序号相对于发送方当前窗口基序号的偏移量
    int ackOffset = (ackPkt.acknum - this->base + this->buflen) % this->buflen;

    // 检查校验和是否正确，并确认ACK序号在发送方已发送并等待确认的数据包范围内
    if (computedCheckSum == ackPkt.checksum && ackOffset < window.size()) {
        // 输出当前发送方窗口的状态
        printf("发送方——接收到确认，滑动前窗口: [ ");
        for (int i = 0; i < this->winlen; i++) {
            printf("%d ", (this->base + i) % this->buflen);
        }
        printf("]\n");  // 打印接收到ACK前的窗口序列

        // 打印确认接收到的ACK
        pUtils->printPacket("发送方——成功接收到确认", ackPkt);

        // 停止与当前窗口基序号相关的定时器
        pns->stopTimer(SENDER, this->base);

        // 滑动窗口，更新基序号和窗口状态
        while (this->base != (ackPkt.acknum + 1) % this->buflen) {
            window.pop_front();  // 移除已确认的数据包
            this->base = (this->base + 1) % this->buflen;  // 更新基序号
        }

        // 输出滑动后的窗口状态
        printf("发送方——接收到确认，滑动后窗口: [ ");
        for (int i = 0; i < this->winlen; i++) {
            printf("%d ", (this->base + i) % this->buflen);
        }
        printf("]\n");  // 打印滑动后窗口的序列

        // 如果窗口中仍有待确认的数据包，重新启动定时器
        if (window.size() != 0) {
            pns->startTimer(SENDER, Configuration::TIME_OUT, this->base);
        }
    }
    // 如果校验和不匹配，输出错误信息
    else if (computedCheckSum != ackPkt.checksum) {
        pUtils->printPacket("发送方——接收到的确认数据校验错误", ackPkt);
    }
    // 如果已经收到过该ACK，输出相关信息
    else {
        pUtils->printPacket("发送方——已经正确接收到该确认", ackPkt);
    }
}

void GBNRdtSender::timeoutHandler(int seqNum) {
    // 关闭与当前序列号相关的定时器，防止重复触发
    pns->stopTimer(SENDER, seqNum);

    // 重新启动定时器，确保在下一次超时事件中依然能处理
    pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);

    // 遍历发送窗口中的所有待确认数据包并重新发送
    for (size_t i = 0; i < window.size(); ++i) {
        // 打印每个重发的数据包信息，方便调试
        pUtils->printPacket("发送方——定时器超时，重新发送窗口中的报文", window[i]);

        // 将数据包重新发送到接收方
        pns->sendToNetworkLayer(RECEIVER, window[i]);
    }

}

