#include "TCPServer.h"

TCPServer::TCPServer()
{

}

TCPServer::TCPServer(ThreadParameter* param)
{
	parameter = param;
}

TCPServer::~TCPServer()
{
}

void TCPServer::run()
{
	Initialize();
	AcceptSocket();
}

void TCPServer::send(SOCKET socket, char* message, size_t size)
{
	LPIO_DATA tempIoData = (LPIO_DATA)calloc(1, sizeof(IO_DATA));
	tempIoData->wsaBuf.len = size;
	memcpy(tempIoData->buffer, message, size);
	tempIoData->wsaBuf.buf = tempIoData->buffer;
	tempIoData->rwMode = SEND;
	WSASend(socket,
		&(tempIoData->wsaBuf),
		1,
		NULL,
		0,
		tempIoData,
		NULL);
}


void TCPServer::Initialize()
{
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		//error
	}

	//CP 오브젝트 생성
	comPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	parameter->comPort = comPort;

	// 코어 개수를 받아오기 위함
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	for (int i = 0; i < sysInfo.dwNumberOfProcessors * 2; i++)
	{
		_beginthreadex(NULL, 0, ThreadProcess, (LPVOID)parameter, 0, NULL);
	}

	serverSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(PORT_NUM);

	bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
}

void TCPServer::AcceptSocket()
{
	listen(serverSocket, 1);	//요청 대기할 수 있는 클라이언트 수 설정

	std::cout << "before accept scoket loop : " << std::endl;

	while (true)
	{
		SOCKET clientSocket;
		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(clientAddr);

		std::cout << "before accept : " << std::endl;
		clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &addrLen);
		std::cout << "accept" << std::endl;

		//동적할당해서 메모리에 있어야 이 후 recv/send하고 getqueue했을 때 정보를 가져올 수 있다.
		LPSOCKET_INFO socketInfo = (LPSOCKET_INFO)calloc(1, sizeof(SOCKET_INFO));
		socketInfo->clientSocket = clientSocket;
		socketInfo->clientAddr = clientAddr;

		CreateIoCompletionPort((HANDLE)clientSocket,
			comPort,			//지정할 포트
			(DWORD)socketInfo,	//comport key
			0);

		LPIO_DATA ioData = (LPIO_DATA)calloc(1, sizeof(IO_DATA));
		ioData->wsaBuf.len = BUF_SIZE;
		ioData->wsaBuf.buf = ioData->buffer;
		ioData->rwMode = RECV;
		DWORD recvBytes, flags = 0;

		//Create message wait
		WSARecv(socketInfo->clientSocket,
			&ioData->wsaBuf,
			1,
			&recvBytes,
			&flags,
			ioData,
			NULL);
	}
}


unsigned __stdcall TCPServer::ThreadProcess(void * param)
{
	ThreadParameter* p = (ThreadParameter*)param;

	//RecvFuncType recvFunc = p.recvCallBack;
	//SocketCloseCallBack closeCallBack = p.socketClosetCallBack;



	HANDLE hComPort = p->comPort;

	DWORD bytesTrans;
	LPSOCKET_INFO socketInfo;
	LPIO_DATA ioData;
	DWORD flags = 0;

	while (1)
	{
		GetQueuedCompletionStatus(hComPort, 
						&bytesTrans,
						(LPDWORD)&socketInfo, 
						(LPOVERLAPPED*)&ioData, 
						INFINITE);
		SOCKET clientSocket = socketInfo->clientSocket;

		if (bytesTrans == 0)    // EOF 전송 시
		{
			cout << "close socket : " << socketInfo->roomId << " user : ";
			printf("%s\n", socketInfo->userId);


			//closeCallBack(socketInfo);
			p->CloseSocketCallBack();

			closesocket(clientSocket);
			free(socketInfo);
			free(ioData);
			continue;
		}

		if (ioData->rwMode == SEND)
		{
			puts("message sent!");
			MESSAGE transMessage = *(MESSAGE*)ioData->wsaBuf.buf;
			printf("========transMessage============\n");
			printf("transMessage.type : %d\n", transMessage.type);
			printf("transMessage.roomId : %d\n", transMessage.roomId);
			printf("transMessage.writer : %s\n", transMessage.writer);
			printf("transMessage.text : %s\n", transMessage.text);
			printf("================================\n");
			free(ioData);
			continue;
		}

		if (ioData->rwMode == RECV)
		{
			puts("message received!");
			//recvFunc(socketInfo, ioData->buffer);
			p->ReceiveDataCallBack();


			//set Recv
			ioData = (LPIO_DATA)calloc(1, sizeof(IO_DATA));
			ioData->wsaBuf.len = BUF_SIZE;
			ioData->wsaBuf.buf = ioData->buffer;
			ioData->rwMode = RECV;
			WSARecv(clientSocket,
				&(ioData->wsaBuf),
				1,
				NULL,
				&flags,
				ioData,
				NULL);
		}



	}
}