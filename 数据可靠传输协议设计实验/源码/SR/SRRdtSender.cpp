#define _CRT_SECURE_NO_WARNINGS 1
#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"


SRRdtSender::SRRdtSender() :expectSequenceNumberSend(0), waitingState(false), base(0), winlen(4), buflen(8)
{
}


SRRdtSender::~SRRdtSender()
{
}



bool SRRdtSender::getWaitingState() {
	//����������˾͸ı�waitingStateΪ�ȴ�ack
	if (window.size() == winlen) {
		waitingState = true;
	}
	else {
		waitingState = false;
	}
	return waitingState;
}




bool SRRdtSender::send(const Message& message) {
	//���Sender�������������ܾ���������
	if (this->getWaitingState()) {
		return false;
	}

	//׼�����Ͱ�
	this->packetWaitingAck.acknum = -1; //acknum�ڷ��Ͱ��в�ʹ��
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;//����ΪҪ���͵����
	this->packetWaitingAck.checksum = 0; //��ʼ��У���Ϊ0

	//��payload���������Ͱ���
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));

	//����checksum
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);

	//����һ����ʱ�Ľṹȥ���δ�յ�ack�İ�
	waitPck tempPck;
	tempPck.flag = false;
	tempPck.winPck = packetWaitingAck;

	//�������ȴ�ack��Ŵ��ڶ���
	window.push_back(tempPck);

	//��ӡ������Ϣ
	pUtils->printPacket("���ͷ��������ͱ���", this->packetWaitingAck);

	//�������ͷ���ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);

	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);

	//�����ڴ������ź�
	this->expectSequenceNumberSend = (this->expectSequenceNumberSend + 1) % this->buflen;

	return true;
}

void SRRdtSender::receive(const Packet& ackPkt) {
		//���У����Ƿ���ȷ
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		// ����ȷ���������ڷ��ʹ�����ʼλ�õ�ƫ����
		int offseqnum = (ackPkt.acknum - this->base + this->buflen) % this->buflen;


		//���У�����ȷ���Ҹ�ȷ�Ϻ��ڷ��ʹ��ڵ�δȷ�����ݰ���Χ��
		if (checkSum == ackPkt.checksum && offseqnum < window.size() && window.at(offseqnum).flag == false) {
			//����յ���ACK��ȷ�ϸð��Ѿ������շ�ȷ��
			window.at(offseqnum).flag = true;
			//ֹͣ��ȷ�ϺŶ�Ӧ�����ݰ��Ķ�ʱ��
			pns->stopTimer(SENDER, ackPkt.acknum);

			//��ӡ��ǰ���ͷ����ڵ�״̬�����յ�ACK֮ǰ��
			printf("���ͷ�������ȷ�յ�ȷ�ϣ�����ǰ����:[ ");
			for (int i = 0; i < this->winlen; i++) {
				if (i < window.size()) {
					if (window.at(i).flag == true)
						printf("%d-Y ", (this->base + i) % buflen);  // Y��ʾ��λ�õİ��ѱ�ȷ��
					else
						printf("%d-N ", (this->base + i) % buflen);  // N��ʾ��λ�õİ���δ��ȷ��
				}
				else {
					printf("%d ", (this->base + i) % buflen);  // �հ�λ��
				}
			}
			printf("]\n");  // ����״̬��ӡ����

			//��ӡ����ȷ�յ���ȷ�ϰ�����Ϣ
			pUtils->printPacket("���ͷ�������ȷ�յ�ȷ��", ackPkt);

			//������ڶ�������ǰ��İ��Ѿ���ȷ�ϣ���������
			while (!window.empty() && window.front().flag == true) {
				window.pop_front();  // �Ƴ���ȷ�ϵİ�
				this->base = (this->base + 1) % this->buflen;  // ��������
			}

			//��ӡ�������ں��״̬
			printf("���ͷ�������ȷ�յ�ȷ�ϣ������󴰿�:[ ");
			for (int i = 0; i < this->winlen; i++) {
				if (i < window.size()) {
					if (window.at(i).flag == true)
						printf("%d-Y ", (this->base + i) % buflen);
					else
						printf("%d-N ", (this->base + i) % buflen);
				}
				else {
					printf("%d ", (this->base + i) % buflen);
				}
			}
			printf("]\n");  // �������ں��״̬��ӡ����

		}
		//���У��Ͳ���ȷ��˵�����ݰ�����
		else if (checkSum != ackPkt.checksum) {
			pUtils->printPacket("���ͷ�����û����ȷ�յ��ñ���ȷ�ϣ�����У�����", ackPkt);
		}
		//���ȷ�ϰ��ѱ����չ���˵�����ظ���ACK
		else {
			pUtils->printPacket("���ͷ���������ȷ�յ����ñ���ȷ��", ackPkt);
		}
}

void SRRdtSender::timeoutHandler(int seqNum) {
	//���㳬ʱ���������ڴ�����ʼλ�õ�ƫ����
	int offseqnum = (seqNum - this->base + this->buflen) % this->buflen;

	//����ֹͣ��ʱ��Ŷ�Ӧ�����ݰ��Ķ�ʱ��
	pns->stopTimer(SENDER, seqNum);

	//����������ʱ�����Ա��ٴμ������ݰ��ĳ�ʱ
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);

	//���·��͸ó�ʱ�����ݰ������շ�
	pns->sendToNetworkLayer(RECEIVER, window.at(offseqnum).winPck);

	//��ӡ�����ط������ĵ�����Ϣ
	pUtils->printPacket("���ͷ�������ʱ����ʱ���ط�����", window.at(offseqnum).winPck);

}
