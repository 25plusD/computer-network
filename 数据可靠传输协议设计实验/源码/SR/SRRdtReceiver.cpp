#define _CRT_SECURE_NO_WARNINGS 1
#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"

SRRdtReceiver::SRRdtReceiver() : expectSequenceNumberRcvd(0), buflen(8), base(0), winlen(4) {
    // 初始化上次发送的ACK包，使得在第一个接收的数据包出错时可以发送确认号为 -1 的ACK
    lastAckPkt.acknum = -1; 
    lastAckPkt.checksum = 0;
    lastAckPkt.seqnum = -1;
    
    std::fill(std::begin(lastAckPkt.payload), std::end(lastAckPkt.payload), '.'); // 用 '.' 填充载荷
    lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt); // 计算校验和

    // 初始化接收窗口，填充空包并将 flag 标记为 false
    for (int i = 0; i < winlen; i++) {
        rcvPck blank;
        blank.flag = false;
        blank.winPck.seqnum = -1;
        window.push_back(blank);
    }//将窗口填满，但都为空包，flag置为false，方便后续操作
}

SRRdtReceiver::~SRRdtReceiver()
{
}

void SRRdtReceiver::receive(const Packet& packet) {
    // 检查校验和
    int checkSum = pUtils->calculateCheckSum(packet);
    int offseqnum = (packet.seqnum - this->base + this->buflen) % this->buflen;

    // 校验和正确且序号在接收窗口范围内且未被占用
    if (checkSum == packet.checksum && offseqnum < winlen && !window[offseqnum].flag) {
        // 标记收到的包并保存到窗口中
        window[offseqnum] = { true, packet };

        // 打印窗口状态（接收到数据包前）
        printf("接收方——正确收到报文，滑动前窗口:[ ");
        for (int i = 0; i < winlen; i++) {
            printf("%d-%s ", (base + i) % buflen, window[i].flag ? "Y" : "N");
        }
        printf("]\n");
        pUtils->printPacket("接收方——正确收到报文", packet);

        // 将已按序接收的数据包传递到上层并滑动窗口
        while (window.front().flag) {
            Message msg;
            memcpy(msg.data, window.front().winPck.payload, sizeof(window.front().winPck.payload));
            pns->delivertoAppLayer(RECEIVER, msg);

            // 滑动窗口并重置新位置
            base = (base + 1) % buflen;
            rcvPck blank;
            blank.flag = false;
            blank.winPck.seqnum = -1;
            window.pop_front();
            window.push_back(blank); //窗口向右滑动一格
        }

        // 打印滑动窗口后的状态
        printf("接收方——正确收到报文，滑动后窗口:[ ");
        for (int i = 0; i < winlen; i++) {
            printf("%d-%s ", (base + i) % buflen, window[i].flag ? "Y" : "N");
        }
        printf("]\n");

        // 发送确认报文
        lastAckPkt.acknum = packet.seqnum;
        lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
        pUtils->printPacket("接收方——发送确认报文ACK", lastAckPkt);
        pns->sendToNetworkLayer(SENDER, lastAckPkt);

    }
    else {
        // 校验和错误
        if (checkSum != packet.checksum) {
            pUtils->printPacket("接收方——没有正确收到发送方的报文，数据校验错误", packet);
        }
        // 已接收过该序号的确认包
        else {
            pUtils->printPacket("接收方——未正确接收到新的报文，报文已确认", packet);
            lastAckPkt.acknum = packet.seqnum;
            lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
            pUtils->printPacket("接收方——重新发送上次的确认报文", lastAckPkt);
            pns->sendToNetworkLayer(SENDER, lastAckPkt);
        }
    }
}

