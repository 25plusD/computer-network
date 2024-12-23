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
        return false; // ���������ܾ�����
    }

    packetWaitingAck.acknum = -1; // ���Ը��ֶ�
    packetWaitingAck.seqnum = expectSequenceNumberSend;
    packetWaitingAck.checksum = 0;
    memcpy(packetWaitingAck.payload, message.data, sizeof(message.data));
    packetWaitingAck.checksum = pUtils->calculateCheckSum(packetWaitingAck);
    window.push_back(packetWaitingAck); // �������͵İ����봰�ڶ���
    pUtils->printPacket("���ͷ��������ͱ���", packetWaitingAck);

    if (base == expectSequenceNumberSend) {
        pns->startTimer(SENDER, Configuration::TIME_OUT, base); // ������ʱ��
    }

    pns->sendToNetworkLayer(RECEIVER, packetWaitingAck); // ���͵������
    expectSequenceNumberSend = (expectSequenceNumberSend + 1) % buflen;

    return true;
}

void TCPRdtSender::receive(const Packet& ackPkt) {
    // ���У���
    int checkSum = pUtils->calculateCheckSum(ackPkt);
    int offacknum = (ackPkt.acknum - base + buflen) % buflen;

    // У�����ȷ��ACK�ڴ�����
    if (checkSum == ackPkt.checksum && offacknum < window.size()) {
        printf("���ͷ�������ȷ�յ�ȷ�ϣ�����ǰ����:[ ");
        for (int i = 0; i < this->winlen; i++) {
            printf("%d ", (this->base + i) % this->buflen);
        }
        printf("]\n");  //����ACKǰ�Ĵ�������

        pUtils->printPacket("���ͷ�������ȷ�յ�ȷ��", ackPkt);
        pns->stopTimer(SENDER, base); // ֹͣ��ʱ��

        // ��������
        while (base != (ackPkt.acknum + 1) % buflen) {
            window.pop_front();
            base = (base + 1) % buflen;
        }

        printf("���ͷ�������ȷ�յ�ȷ�ϣ������󴰿�:[ ");
        for (int i = 0; i < this->winlen; i++) {
            printf("%d ", (this->base + i) % this->buflen);
        }
        printf("]\n");  //���ճɹ���Ĵ���ֵ

        Rdnum = 0; // �����������

        if (!window.empty()) {
            pns->startTimer(SENDER, Configuration::TIME_OUT, base); // ����������ʱ��
        }
    }
    //������
    else if (checkSum != ackPkt.checksum)
        pUtils->printPacket("���ͷ�����û����ȷ�յ��ñ���ȷ��,����У�����", ackPkt);
    else if (ackPkt.acknum == (this->base + this->buflen - 1) % this->buflen) {
        pUtils->printPacket("���ͷ���������ȷ�յ����ñ���ȷ��", ackPkt);
        this->Rdnum++;
        //3����ͬack
        if (this->Rdnum == 3 && window.size() > 0) {
            pUtils->printPacket("���ͷ��������������ش����ƣ��ش����緢����û��ȷ�ϵı��Ķ�", window.front());
            pns->sendToNetworkLayer(RECEIVER, window.front());
            this->Rdnum = 0;
        }
    }
}

void TCPRdtSender::timeoutHandler(int seqNum) {
    pUtils->printPacket("���ͷ�������ʱ��ʱ�䵽���ط����緢����û��ȷ�ϵı��Ķ�", window.front());
    pns->stopTimer(SENDER, seqNum); // �رն�ʱ��
    pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum); // ����������ʱ��
    pns->sendToNetworkLayer(RECEIVER, window.front()); // �ط�����δȷ�ϵı���
}



