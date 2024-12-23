#define _CRT_SECURE_NO_WARNINGS 1
#include "stdafx.h"
#include "Global.h"
#include "TCPRdtSender.h"
#include <deque>

const int WINDOW_LENGTH = 4;
const int BUFFER_LENGTH = 8;

TCPRdtSender::TCPRdtSender()
    : expectSequenceNumberSend(0), waitingState(false), base(0), winlen(WINDOW_LENGTH), buflen(BUFFER_LENGTH), Rdnum(0) {}

TCPRdtSender::~TCPRdtSender() {}

bool TCPRdtSender::getWaitingState() {
    waitingState = (window.size() == winlen);
    return waitingState;
}

bool TCPRdtSender::send(const Message& message) {
    if (getWaitingState()) {
        return false; // 窗口满，拒绝发送
    }

    packetWaitingAck.acknum = -1; // 忽略该字段
    packetWaitingAck.seqnum = expectSequenceNumberSend;
    packetWaitingAck.checksum = 0;
    memcpy(packetWaitingAck.payload, message.data, sizeof(message.data));
    packetWaitingAck.checksum = pUtils->calculateCheckSum(packetWaitingAck);
    window.push_back(packetWaitingAck); // 将待发送的包加入窗口队列
    pUtils->printPacket("发送方——发送报文", packetWaitingAck);

    if (base == expectSequenceNumberSend) {
        pns->startTimer(SENDER, Configuration::TIME_OUT, base); // 启动定时器
    }

    pns->sendToNetworkLayer(RECEIVER, packetWaitingAck); // 发送到网络层
    expectSequenceNumberSend = (expectSequenceNumberSend + 1) % buflen;

    return true;
}

void TCPRdtSender::receive(const Packet& ackPkt) {
    // 检查校验和
    int checkSum = pUtils->calculateCheckSum(ackPkt);
    int offacknum = (ackPkt.acknum - base + buflen) % buflen;

    // 校验和正确且ACK在窗口内
    if (checkSum == ackPkt.checksum && offacknum < window.size()) {
        printf("发送方——正确收到确认，滑动前窗口:[ ");
        for (int i = 0; i < this->winlen; i++) {
            printf("%d ", (this->base + i) % this->buflen);
        }
        printf("]\n");  //接收ACK前的窗口序列

        pUtils->printPacket("发送方——正确收到确认", ackPkt);
        pns->stopTimer(SENDER, base); // 停止定时器

        // 滑动窗口
        while (base != (ackPkt.acknum + 1) % buflen) {
            window.pop_front();
            base = (base + 1) % buflen;
        }

        printf("发送方——正确收到确认，滑动后窗口:[ ");
        for (int i = 0; i < this->winlen; i++) {
            printf("%d ", (this->base + i) % this->buflen);
        }
        printf("]\n");  //接收成功后的窗口值

        Rdnum = 0; // 重置冗余计数

        if (!window.empty()) {
            pns->startTimer(SENDER, Configuration::TIME_OUT, base); // 重新启动计时器
        }
    }
    //错误处理
    else if (checkSum != ackPkt.checksum)
        pUtils->printPacket("发送方——没有正确收到该报文确认,数据校验错误", ackPkt);
    else if (ackPkt.acknum == (this->base + this->buflen - 1) % this->buflen) {
        pUtils->printPacket("发送方——已正确收到过该报文确认", ackPkt);
        this->Rdnum++;
        //3个相同ack
        if (this->Rdnum == 3 && window.size() > 0) {
            pUtils->printPacket("发送方——启动快速重传机制，重传最早发送且没被确认的报文段", window.front());
            pns->sendToNetworkLayer(RECEIVER, window.front());
            this->Rdnum = 0;
        }
    }
}

void TCPRdtSender::timeoutHandler(int seqNum) {
    pUtils->printPacket("发送方——定时器时间到，重发最早发送且没被确认的报文段", window.front());
    pns->stopTimer(SENDER, seqNum); // 关闭定时器
    pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum); // 重新启动定时器
    pns->sendToNetworkLayer(RECEIVER, window.front()); // 重发最早未确认的报文
}



