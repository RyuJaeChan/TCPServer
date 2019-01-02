#pragma once

#include <WinSock2.h>
#include <process.h>
#include <cstdlib>
#include <iostream>
#include <string>

#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

#include "SingleTon.h"

using namespace std;

#define PORT_NUM 3302

#define BUF_SIZE 152
#define SEND	3
#define	RECV	5

typedef struct _SOCKET_INFO
{
	SOCKET clientSocket;
	SOCKADDR_IN clientAddr;
	int roomId;
	string userId;
	bool flag = false;
	string test;
} SOCKET_INFO, *LPSOCKET_INFO;

typedef struct _IO_DATA : public OVERLAPPED
{
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rwMode;
} IO_DATA, *LPIO_DATA;


struct MESSAGE
{
	int type;
	int roomId;
	char writer[16];
	char text[128];
};

using RecvFuncType = void(*)(LPSOCKET_INFO, char*);
using SocketCloseCallBack = void(*)(LPSOCKET_INFO);

struct Param
{
	HANDLE comPort;
	RecvFuncType recvCallBack;
	SocketCloseCallBack socketClosetCallBack;
};

class ThreadParameter {
public:
	HANDLE comPort;
	virtual void ReceiveDataCallBack() = 0;
	virtual void CloseSocketCallBack() = 0;
private:
};


class TCPServer
{
public:
	TCPServer();
	TCPServer(ThreadParameter* param);
	~TCPServer();


	void run();
	static unsigned WINAPI ThreadProcess(void* param);
	unsigned  (WINAPI *SendProcess)(void* paramt);

	static void send(SOCKET sock, char* message, size_t size);



private:
	void Initialize();
	void AcceptSocket();

	//unsigned  (WINAPI *ThreadProcess)(void* param);

	SOCKET serverSocket;
	HANDLE comPort;

	ThreadParameter* parameter;

};

