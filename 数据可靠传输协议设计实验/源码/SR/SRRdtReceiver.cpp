#define _CRT_SECURE_NO_WARNINGS 1
#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"

SRRdtReceiver::SRRdtReceiver() : expectSequenceNumberRcvd(0), buflen(8), base(0), winlen(4) {
    // ��ʼ���ϴη��͵�ACK����ʹ���ڵ�һ�����յ����ݰ�����ʱ���Է���ȷ�Ϻ�Ϊ -1 ��ACK
    lastAckPkt.acknum = -1; 
    lastAckPkt.checksum = 0;
    lastAckPkt.seqnum = -1;
    
    std::fill(std::begin(lastAckPkt.payload), std::end(lastAckPkt.payload), '.'); // �� '.' ����غ�
    lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt); // ����У���

    // ��ʼ�����մ��ڣ����հ����� flag ���Ϊ false
    for (int i = 0; i < winlen; i++) {
        rcvPck blank;
        blank.flag = false;
        blank.winPck.seqnum = -1;
        window.push_back(blank);
    }//����������������Ϊ�հ���flag��Ϊfalse�������������
}

SRRdtReceiver::~SRRdtReceiver()
{
}

void SRRdtReceiver::receive(const Packet& packet) {
    // ���У���
    int checkSum = pUtils->calculateCheckSum(packet);
    int offseqnum = (packet.seqnum - this->base + this->buflen) % this->buflen;

    // У�����ȷ������ڽ��մ��ڷ�Χ����δ��ռ��
    if (checkSum == packet.checksum && offseqnum < winlen && !window[offseqnum].flag) {
        // ����յ��İ������浽������
        window[offseqnum] = { true, packet };

        // ��ӡ����״̬�����յ����ݰ�ǰ��
        printf("���շ�������ȷ�յ����ģ�����ǰ����:[ ");
        for (int i = 0; i < winlen; i++) {
            printf("%d-%s ", (base + i) % buflen, window[i].flag ? "Y" : "N");
        }
        printf("]\n");
        pUtils->printPacket("���շ�������ȷ�յ�����", packet);

        // ���Ѱ�����յ����ݰ����ݵ��ϲ㲢��������
        while (window.front().flag) {
            Message msg;
            memcpy(msg.data, window.front().winPck.payload, sizeof(window.front().winPck.payload));
            pns->delivertoAppLayer(RECEIVER, msg);

            // �������ڲ�������λ��
            base = (base + 1) % buflen;
            rcvPck blank;
            blank.flag = false;
            blank.winPck.seqnum = -1;
            window.pop_front();
            window.push_back(blank); //�������һ���һ��
        }

        // ��ӡ�������ں��״̬
        printf("���շ�������ȷ�յ����ģ������󴰿�:[ ");
        for (int i = 0; i < winlen; i++) {
            printf("%d-%s ", (base + i) % buflen, window[i].flag ? "Y" : "N");
        }
        printf("]\n");

        // ����ȷ�ϱ���
        lastAckPkt.acknum = packet.seqnum;
        lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
        pUtils->printPacket("���շ���������ȷ�ϱ���ACK", lastAckPkt);
        pns->sendToNetworkLayer(SENDER, lastAckPkt);

    }
    else {
        // У��ʹ���
        if (checkSum != packet.checksum) {
            pUtils->printPacket("���շ�����û����ȷ�յ����ͷ��ı��ģ�����У�����", packet);
        }
        // �ѽ��չ�����ŵ�ȷ�ϰ�
        else {
            pUtils->printPacket("���շ�����δ��ȷ���յ��µı��ģ�������ȷ��", packet);
            lastAckPkt.acknum = packet.seqnum;
            lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
            pUtils->printPacket("���շ��������·����ϴε�ȷ�ϱ���", lastAckPkt);
            pns->sendToNetworkLayer(SENDER, lastAckPkt);
        }
    }
}

