#define _CRT_SECURE_NO_WARNINGS 1
#include "stdafx.h"
#include "Global.h"
#include "GBNRdtReceiver.h"


GBNRdtReceiver::GBNRdtReceiver() : expectSequenceNumberRcvd(0), buflen(8)
{
	// 初始化上次发送的确认包
	lastAckPkt.acknum = -1;    // 初始状态下，确认序号为 -1，表示无有效确认
	lastAckPkt.seqnum = -1;    // 确认包的 seqnum 字段无效，初始化为 -1
	lastAckPkt.checksum = 0;   // 校验和初始设为 0，在最终计算校验和之前

	// 初始化 payload 数据为占位符字符 '.'
	std::fill(std::begin(lastAckPkt.payload), std::end(lastAckPkt.payload), '.');

	// 计算确认包的校验和，并将其存储在 checksum 字段中
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}



GBNRdtReceiver::~GBNRdtReceiver()
{
}

void GBNRdtReceiver::receive(const Packet& packet) {
    // 计算接收报文的校验和
    int calculatedCheckSum = pUtils->calculateCheckSum(packet);

    // 校验和正确并且报文序号与期待的序号相符
    if (calculatedCheckSum == packet.checksum && expectSequenceNumberRcvd == packet.seqnum) {
        pUtils->printPacket("接收方——正确收到发送方的报文", packet);

        // 创建消息并将数据递交给应用层
        Message receivedMessage;
        std::memcpy(receivedMessage.data, packet.payload, sizeof(packet.payload));
        pns->delivertoAppLayer(RECEIVER, receivedMessage);

        // 更新确认包的确认序号和校验和
        lastAckPkt.acknum = packet.seqnum;
        lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
        pUtils->printPacket("接收方——发送确认报文ACK", lastAckPkt);

        // 通过网络层发送确认报文到发送方
        pns->sendToNetworkLayer(SENDER, lastAckPkt);

        // 更新接收方期待的序号（取模操作保证序号在 0 到 buflen-1 之间循环）
        expectSequenceNumberRcvd = (expectSequenceNumberRcvd + 1) % buflen;
    }
    else {
        // 校验和不正确或报文序号不匹配的处理
        if (calculatedCheckSum != packet.checksum) {
            pUtils->printPacket("接收方——未正确收到报文，校验和错误", packet);
        }
        else {
            pUtils->printPacket("接收方——未正确收到报文，报文序号不符", packet);
        }

        // 重新发送上次的确认报文
        pUtils->printPacket("接收方——重新发送上次的确认报文", lastAckPkt);
        pns->sendToNetworkLayer(SENDER, lastAckPkt);
    }
}