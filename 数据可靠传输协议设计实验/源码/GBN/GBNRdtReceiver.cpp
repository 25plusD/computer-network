#define _CRT_SECURE_NO_WARNINGS 1
#include "stdafx.h"
#include "Global.h"
#include "GBNRdtReceiver.h"


GBNRdtReceiver::GBNRdtReceiver() : expectSequenceNumberRcvd(0), buflen(8)
{
	// ��ʼ���ϴη��͵�ȷ�ϰ�
	lastAckPkt.acknum = -1;    // ��ʼ״̬�£�ȷ�����Ϊ -1����ʾ����Чȷ��
	lastAckPkt.seqnum = -1;    // ȷ�ϰ��� seqnum �ֶ���Ч����ʼ��Ϊ -1
	lastAckPkt.checksum = 0;   // У��ͳ�ʼ��Ϊ 0�������ռ���У���֮ǰ

	// ��ʼ�� payload ����Ϊռλ���ַ� '.'
	std::fill(std::begin(lastAckPkt.payload), std::end(lastAckPkt.payload), '.');

	// ����ȷ�ϰ���У��ͣ�������洢�� checksum �ֶ���
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}



GBNRdtReceiver::~GBNRdtReceiver()
{
}

void GBNRdtReceiver::receive(const Packet& packet) {
    // ������ձ��ĵ�У���
    int calculatedCheckSum = pUtils->calculateCheckSum(packet);

    // У�����ȷ���ұ���������ڴ���������
    if (calculatedCheckSum == packet.checksum && expectSequenceNumberRcvd == packet.seqnum) {
        pUtils->printPacket("���շ�������ȷ�յ����ͷ��ı���", packet);

        // ������Ϣ�������ݵݽ���Ӧ�ò�
        Message receivedMessage;
        std::memcpy(receivedMessage.data, packet.payload, sizeof(packet.payload));
        pns->delivertoAppLayer(RECEIVER, receivedMessage);

        // ����ȷ�ϰ���ȷ����ź�У���
        lastAckPkt.acknum = packet.seqnum;
        lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
        pUtils->printPacket("���շ���������ȷ�ϱ���ACK", lastAckPkt);

        // ͨ������㷢��ȷ�ϱ��ĵ����ͷ�
        pns->sendToNetworkLayer(SENDER, lastAckPkt);

        // ���½��շ��ڴ�����ţ�ȡģ������֤����� 0 �� buflen-1 ֮��ѭ����
        expectSequenceNumberRcvd = (expectSequenceNumberRcvd + 1) % buflen;
    }
    else {
        // У��Ͳ���ȷ������Ų�ƥ��Ĵ���
        if (calculatedCheckSum != packet.checksum) {
            pUtils->printPacket("���շ�����δ��ȷ�յ����ģ�У��ʹ���", packet);
        }
        else {
            pUtils->printPacket("���շ�����δ��ȷ�յ����ģ�������Ų���", packet);
        }

        // ���·����ϴε�ȷ�ϱ���
        pUtils->printPacket("���շ��������·����ϴε�ȷ�ϱ���", lastAckPkt);
        pns->sendToNetworkLayer(SENDER, lastAckPkt);
    }
}