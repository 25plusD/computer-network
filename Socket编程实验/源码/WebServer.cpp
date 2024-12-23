#define _CRT_SECURE_NO_WARNINGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#pragma once

#include<iostream>
#include<cstdio>
#include<WinSock2.h>


using namespace std;

#pragma comment(lib,"ws2_32.lib")


void sendhead(const char* filename, const char* extname, SOCKET s);
void senddata(const char* filename, SOCKET s);

//A test
//port number: 5050
//IP address: 127.0.0.1
//home directory path: E:/WebLabTest
//http://127.0.0.1:5050/index.html

int main()
{
	//��ʼ��WinSock
	WSADATA wsaData;
	fd_set rfds;
	fd_set wfds;
	bool first_connetion = true;

	int nRc = WSAStartup(0x0202, &wsaData);

	if (nRc) {
		printf("Winsock startup failed with error!\n");
		printf("Error code is %d\n", WSAGetLastError());
		return 1;
	}

	if (wsaData.wVersion != 0x0202) {
		printf("Winsock version is not correct!\n");
		printf("Error code is %d\n", WSAGetLastError());
		return 1;
	}
	printf("Winsock startup Ok!\n");


	//����socket
	SOCKET srvSocket;
	//��������ַ�Ϳͻ��˵�ַ
	sockaddr_in srvAddr, clientAddr;
	//�Ựsocket�������client����ͨ��
	SOCKET sessionSocket;
	//ip��ַ����
	int addrLen;
	//��������socket
	//AF_INET��ʾIPv4����Э�飬SOCK_STREAM��ʾ��ʽ�׽��֣�����TCP����0��ʾ��ʽЭ��ĵ�0��Э�飬��TCP
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket != INVALID_SOCKET) printf("Socket create Ok!\n");
	else {
		printf("Socket create failed with error!\n");
		printf("Error code is %d\n", WSAGetLastError());
		return 1;
	}


	//���÷������Ķ˿�,��ַ,��Ŀ¼·��
	int srvport;
	char srvip[50] = { 0 };
	char homeaddr[50] = { 0 };
	printf("Enter the port number��");
	scanf("%d", &srvport);
	printf("Enter the IP address��");
	scanf("%s", srvip);
	printf("Enter the home directory path��");
	scanf("%s", homeaddr);

	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(srvport);
	srvAddr.sin_addr.s_addr = inet_addr(srvip);

	//��
	int rtn = bind(srvSocket, (LPSOCKADDR)&srvAddr, sizeof(srvAddr));
	if (rtn != SOCKET_ERROR) printf("Socket bind Ok!\n");
	else {
		printf("Socket bind failed!\n");
		printf("Error code is %d\n", WSAGetLastError());
		return 1;
	}

	//����
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR) printf("Socket listen Ok!\n");
	else {
		printf("Socket listen failed!\n");
		printf("Error code is %d\n", WSAGetLastError());
		return 1;
	}


	clientAddr.sin_family = AF_INET;
	addrLen = sizeof(clientAddr);
	//��������
	while (true) {
		SOCKET clientSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
		if (clientSocket != INVALID_SOCKET)
			printf("Socket connected successfully with client!\n");
		else {
			printf("Socket connected failed with client!\n");
			printf("Error code is %d\n", WSAGetLastError());
		}
		printf("Client IP address��%s\n", inet_ntoa(clientAddr.sin_addr));
		printf("Client port number��%u\n", htons(clientAddr.sin_port));

		//���ý��ջ�����
		char rcvdata[4096] = { 0 };
		/*
		int WSAAPI recv(//���ض�����ֽڴ�С
		  SOCKET s,//��ʶ�������׽��ֵ���������
		  char   *buf,//ָ�򻺳�����ָ�룬�Խ��մ�������ݡ�
		  int    len,//buf����ָ��Ļ������ĳ��ȣ����ֽ�Ϊ��λ����
		  int    flags//һ��Ӱ��˹�����Ϊ�ı�־�� 
		);
		*/
		nRc = recv(clientSocket, rcvdata, 4096, 0); //��������
		if (nRc != SOCKET_ERROR)
			printf("Data receive successfully!\n");
		else {
			printf("Data receive failed!\n");
			printf("Error code is %d\n", WSAGetLastError());
		}
		printf("Received %d bits data from the client:\n%s\n", nRc, rcvdata);

		//�������� �ļ�������չ��
		//���յ��������ʽ����
		//	GET /path/to/file.html HTTP/1.1
		//	Host: example.com

		char requestname[30] = "";
		char extendname[15] = "";
		for (int i = 0; i < nRc; i++) {
			if (rcvdata[i] == '/') {
				for (int j = 0; j < nRc - i; j++) {
					if (rcvdata[i] != ' ') {
						requestname[j] = rcvdata[i];
						i++;
					}
					else {
						requestname[j + 1] = '\0';
						break;
					}
				}
				break;
			}
		}

		for (int i = 0; i < nRc; i++) {
			if (rcvdata[i] == '.') {
				for (int j = 0; j < nRc - i; j++) {
					if (rcvdata[i + 1] != ' ') {
						extendname[j] = rcvdata[i + 1];
						i++;
					}
					else {
						extendname[j + 1] = '\0';
						break;
					}
				}
				break;
			}
		}
		//��ȡ���ļ���: path/to/file.html
		//��ȡ����չ��: html
		char fileName[100] = { 0 };
		strcpy(fileName, homeaddr);
		printf("request file name��%s\n", requestname);
		strcat(fileName, requestname);
		printf("full path��%s\n", fileName);
		sendhead(fileName, extendname, clientSocket); //�����ײ�
		senddata(fileName, clientSocket); //����ʵ��
		printf("\n\n\n");

		closesocket(clientSocket);
	}

	//�رշ�����
	closesocket(srvSocket);
	WSACleanup();
	return 0;
}



//����HTTP��Ӧͷ���ļ����ݸ��ͻ���

void sendhead(const char* filename, const char* extendname, SOCKET s) {
	char content_Type[60] = "Content-Type: ";
	const char* ok_find = "HTTP/1.1 200 OK\r\n";
	const char* not_find = "HTTP/1.1 404 NOT FOUND\r\n";
	const char* forbidden_find = "HTTP/1.1 403 FORBIDDEN\r\n";

	// ����Content-Type
	if (strcmp(extendname, "html") == 0) {
		strcat(content_Type, "text/html; charset=UTF-8");
	}
	else if (strcmp(extendname, "gif") == 0) {
		strcat(content_Type, "image/gif");
	}
	else if (strcmp(extendname, "jpg") == 0 || strcmp(extendname, "jpeg") == 0) {
		strcat(content_Type, "image/jpeg");
	}
	else if (strcmp(extendname, "png") == 0) {
		strcat(content_Type, "image/png");
	}
	else {
		strcat(content_Type, "application/octet-stream");
	}

	// ����ļ��Ƿ��ֹ���� 403 FORBIDDEN
	if (strcmp(filename, "E:/WebLabTest/OIP.jpg") == 0) {
		if (send(s, forbidden_find, strlen(forbidden_find), 0) == SOCKET_ERROR) {
			printf("send failed��\n");
			return;
		}
		if (send(s, content_Type, strlen(content_Type), 0) == SOCKET_ERROR) {
			printf("send failed��\n");
			return;
		}
		printf("403 FORBIDDEN��\n");
		return;
	}


	// ����ļ��Ƿ���� 404 NOT FOUND / 200 OK
	FILE* fp = fopen(filename, "rb");
	if (fp == NULL) {
		// ����404��Ӧ
		if (send(s, not_find, strlen(not_find), 0) == SOCKET_ERROR) {
			printf("send failed!\n");
			return;
		}

		const char* contentType = "Content-Type: text/html\r\n";
		if (send(s, contentType, strlen(contentType), 0) == SOCKET_ERROR) {
			printf("send failed!\n");
			return;
		}

		// ��Ӧͷ����
		if (send(s, "\r\n", 2, 0) == SOCKET_ERROR) {
			printf("send failed!\n");
			return;
		}

		// ����404ҳ������
		const char* not_found_content = "<html><body><h1>404 Not Found</h1><p>The requested URL was not found on this server.</p></body></html>";
		if (send(s, not_found_content, strlen(not_found_content), 0) == SOCKET_ERROR) {
			printf("send failed!\n");
			return;
		}
		printf("404 Not Found: %s\n", filename);
		return;
	}
	else {//200 �ļ����ҳɹ�
		if (send(s, ok_find, strlen(ok_find), 0) == SOCKET_ERROR) {
			printf("send failed��\n");
			return;
		}
		printf("200 OK��\n");
	}

	// �����ļ���С
	fseek(fp, 0, SEEK_END);
	long long dataLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// ������Ӧͷ
	send(s, ok_find, strlen(ok_find), 0);
	send(s, content_Type, strlen(content_Type), 0);
	char content_length[50];
	sprintf(content_length, "Content-Length: %lld\r\n", dataLen);
	send(s, content_length, strlen(content_length), 0);
	send(s, "\r\n", 2, 0); // ��Ӧͷ����
}




void senddata(const char* filename, SOCKET s) {
	FILE* fp_data = fopen(filename, "rb");
	if (fp_data == NULL) {
		printf("Failed to open file: %s\n", filename);
		return;
	}

	// �����ļ���С
	fseek(fp_data, 0L, SEEK_END);
	long long dataLen = ftell(fp_data);
	fseek(fp_data, 0L, SEEK_SET);

	// �����㹻�Ļ���������ȷ����������β��\0
	char* buffer = (char*)malloc(dataLen + 1);
	fread(buffer, dataLen, 1, fp_data);
	buffer[dataLen] = '\0'; // ȷ���ַ�������
	fclose(fp_data);

	// �����ļ�����
	if (send(s, buffer, dataLen, 0) == SOCKET_ERROR) {
		printf("Failed to send file data!\n");
	}
	else {
		printf("Sent %lld bytes of data.\n", dataLen);
	}

	free(buffer);
}


