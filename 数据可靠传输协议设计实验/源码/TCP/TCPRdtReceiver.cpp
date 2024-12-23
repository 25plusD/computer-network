#define _CRT_SECURE_NO_WARNINGS 1
#include "stdafx.h"
#include "Global.h"
#include "TCPRdtReceiver.h"

const int INITIAL_ACKNUM = -1;

TCPRdtReceiver::TCPRdtReceiver() : expectSequenceNumberRcvd(0), buflen(8) {
    lastAckPkt.acknum = INITIAL_ACKNUM; // 初始状态确认序号为-1
    lastAckPkt.checksum = 0;
    lastAckPkt.seqnum = INITIAL_ACKNUM; // 忽略该字段

    // 初始化负载为'.'
    for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
        lastAckPkt.payload[i] = '.';
    }

    // 计算校验和
    lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}

TCPRdtReceiver::~TCPRdtReceiver()
{
}

void TCPRdtReceiver::receive(const Packet& packet) {
    // 检查校验和是否正确
    int checkSum = pUtils->calculateCheckSum(packet);

    // 校验和正确且报文序号匹配
    if (checkSum == packet.checksum && this->expectSequenceNumberRcvd == packet.seqnum) {
        pUtils->printPacket("接收方——正确收到发送方的报文", packet);

        // 取出Message，向上递交给应用层
        Message msg;
        memcpy(msg.data, packet.payload, sizeof(packet.payload));
        pns->delivertoAppLayer(RECEIVER, msg);

        // 更新确认报文
        lastAckPkt.acknum = packet.seqnum; // 确认序号等于收到的报文序号
        lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt); // 计算确认报文的校验和

        pUtils->printPacket("接收方——发送确认报文", lastAckPkt);
        pns->sendToNetworkLayer(SENDER, lastAckPkt); // 发送确认报文

        // 更新期望接收的序号
        this->expectSequenceNumberRcvd = (this->expectSequenceNumberRcvd + 1) % this->buflen; // 切换接收序号
    }
    else {
        // 处理错误情况
        if (checkSum != packet.checksum) {
            pUtils->printPacket("接收方——没有正确收到发送方的报文, 数据校验错误", packet);
        }
        else {
            pUtils->printPacket("接收方——没有正确收到发送方的报文, 报文序号不对", packet);
        }

        pUtils->printPacket("接收方——重新发送上次的确认报文", lastAckPkt);
        pns->sendToNetworkLayer(SENDER, lastAckPkt); // 发送上次的确认报文
    }
}