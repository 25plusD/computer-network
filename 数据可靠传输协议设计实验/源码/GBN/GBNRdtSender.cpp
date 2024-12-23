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
	// �жϵ�ǰ�����Ƿ��Ѵﵽ�������
	if (window.size() >= winlen) {
		waitingState = true;  // ��������������ȴ�״̬
	}
	else {
		waitingState = false; // ����δ�������Լ�������
	}

	return waitingState;  // ���ص�ǰ�ĵȴ�״̬
}




bool GBNRdtSender::send(const Message& message) {
	// ��鷢�ͷ������Ƿ���
	if (this->waitingState) {
		return false;  // ���������޷������±���
	}

	// ���������͵����ݰ�
	this->packetWaitingAck.acknum = -1;  // ����ACK���Ϊ-1������ʱ����Ҫʹ�ø��ֶ�
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;  // �������
	this->packetWaitingAck.checksum = 0;  // ��ʼ��У���Ϊ0

	// ������Ϣ���ݵ����ݰ���Ч�غ�
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));

	// �������ݰ���У���
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);

	// �������͵İ����봰�ڶ���
	window.push_back(this->packetWaitingAck);
	pUtils->printPacket("���ͷ��������ͱ���", this->packetWaitingAck);  // ��ӡ���͵ı�����Ϣ

	// �ڴ�����ǰ�İ�������ʱ��������ʱ��
	if (this->base == this->expectSequenceNumberSend) {
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->base);
	}

	// ͨ������㷢�����ݰ������շ�
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);

	// ���ݻ��������ȸ�����һ���������
	this->expectSequenceNumberSend = (this->expectSequenceNumberSend + 1) % this->buflen;

	return true;  // ���ݰ����ͳɹ�
}

void GBNRdtSender::receive(const Packet& ackPkt) {
    // ������յ���ACK���ĵ�У���
    int computedCheckSum = pUtils->calculateCheckSum(ackPkt);

    // ����ACK�������ڷ��ͷ���ǰ���ڻ���ŵ�ƫ����
    int ackOffset = (ackPkt.acknum - this->base + this->buflen) % this->buflen;

    // ���У����Ƿ���ȷ����ȷ��ACK����ڷ��ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ���Χ��
    if (computedCheckSum == ackPkt.checksum && ackOffset < window.size()) {
        // �����ǰ���ͷ����ڵ�״̬
        printf("���ͷ��������յ�ȷ�ϣ�����ǰ����: [ ");
        for (int i = 0; i < this->winlen; i++) {
            printf("%d ", (this->base + i) % this->buflen);
        }
        printf("]\n");  // ��ӡ���յ�ACKǰ�Ĵ�������

        // ��ӡȷ�Ͻ��յ���ACK
        pUtils->printPacket("���ͷ������ɹ����յ�ȷ��", ackPkt);

        // ֹͣ�뵱ǰ���ڻ������صĶ�ʱ��
        pns->stopTimer(SENDER, this->base);

        // �������ڣ����»���źʹ���״̬
        while (this->base != (ackPkt.acknum + 1) % this->buflen) {
            window.pop_front();  // �Ƴ���ȷ�ϵ����ݰ�
            this->base = (this->base + 1) % this->buflen;  // ���»����
        }

        // ���������Ĵ���״̬
        printf("���ͷ��������յ�ȷ�ϣ������󴰿�: [ ");
        for (int i = 0; i < this->winlen; i++) {
            printf("%d ", (this->base + i) % this->buflen);
        }
        printf("]\n");  // ��ӡ�����󴰿ڵ�����

        // ������������д�ȷ�ϵ����ݰ�������������ʱ��
        if (window.size() != 0) {
            pns->startTimer(SENDER, Configuration::TIME_OUT, this->base);
        }
    }
    // ���У��Ͳ�ƥ�䣬���������Ϣ
    else if (computedCheckSum != ackPkt.checksum) {
        pUtils->printPacket("���ͷ��������յ���ȷ������У�����", ackPkt);
    }
    // ����Ѿ��յ�����ACK����������Ϣ
    else {
        pUtils->printPacket("���ͷ������Ѿ���ȷ���յ���ȷ��", ackPkt);
    }
}

void GBNRdtSender::timeoutHandler(int seqNum) {
    // �ر��뵱ǰ���к���صĶ�ʱ������ֹ�ظ�����
    pns->stopTimer(SENDER, seqNum);

    // ����������ʱ����ȷ������һ�γ�ʱ�¼�����Ȼ�ܴ���
    pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);

    // �������ʹ����е����д�ȷ�����ݰ������·���
    for (size_t i = 0; i < window.size(); ++i) {
        // ��ӡÿ���ط������ݰ���Ϣ���������
        pUtils->printPacket("���ͷ�������ʱ����ʱ�����·��ʹ����еı���", window[i]);

        // �����ݰ����·��͵����շ�
        pns->sendToNetworkLayer(RECEIVER, window[i]);
    }

}

