#define _CRT_SECURE_NO_WARNINGS 1
#include "stdafx.h"
#include "Global.h"
#include "TCPRdtReceiver.h"

const int INITIAL_ACKNUM = -1;

TCPRdtReceiver::TCPRdtReceiver() : expectSequenceNumberRcvd(0), buflen(8) {
    lastAckPkt.acknum = INITIAL_ACKNUM; // ��ʼ״̬ȷ�����Ϊ-1
    lastAckPkt.checksum = 0;
    lastAckPkt.seqnum = INITIAL_ACKNUM; // ���Ը��ֶ�

    // ��ʼ������Ϊ'.'
    for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
        lastAckPkt.payload[i] = '.';
    }

    // ����У���
    lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}

TCPRdtReceiver::~TCPRdtReceiver()
{
}

void TCPRdtReceiver::receive(const Packet& packet) {
    // ���У����Ƿ���ȷ
    int checkSum = pUtils->calculateCheckSum(packet);

    // У�����ȷ�ұ������ƥ��
    if (checkSum == packet.checksum && this->expectSequenceNumberRcvd == packet.seqnum) {
        pUtils->printPacket("���շ�������ȷ�յ����ͷ��ı���", packet);

        // ȡ��Message�����ϵݽ���Ӧ�ò�
        Message msg;
        memcpy(msg.data, packet.payload, sizeof(packet.payload));
        pns->delivertoAppLayer(RECEIVER, msg);

        // ����ȷ�ϱ���
        lastAckPkt.acknum = packet.seqnum; // ȷ����ŵ����յ��ı������
        lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt); // ����ȷ�ϱ��ĵ�У���

        pUtils->printPacket("���շ���������ȷ�ϱ���", lastAckPkt);
        pns->sendToNetworkLayer(SENDER, lastAckPkt); // ����ȷ�ϱ���

        // �����������յ����
        this->expectSequenceNumberRcvd = (this->expectSequenceNumberRcvd + 1) % this->buflen; // �л��������
    }
    else {
        // ����������
        if (checkSum != packet.checksum) {
            pUtils->printPacket("���շ�����û����ȷ�յ����ͷ��ı���, ����У�����", packet);
        }
        else {
            pUtils->printPacket("���շ�����û����ȷ�յ����ͷ��ı���, ������Ų���", packet);
        }

        pUtils->printPacket("���շ��������·����ϴε�ȷ�ϱ���", lastAckPkt);
        pns->sendToNetworkLayer(SENDER, lastAckPkt); // �����ϴε�ȷ�ϱ���
    }
}