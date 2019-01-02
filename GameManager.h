#pragma once

#include <iostream>
#include <map>


#include "TCPServer.h"
#include "GameRoomInfo.h"


using namespace std;

#define PLAYING_TIME 10000

enum MESSAGE_TYPE
{
	CREATE = 1,
	CREATE_ACK,
	JOIN_REQ,
	JOIN_REQ_ACK,
	JOIN,
	JOIN_ACK,
	CHAT,
	VOTE,
	POLLING,
	QUIT,
	JOB,
	GAME_START,
	ACTION,
	ACTION_RESULT,
	VOTE_RESULT,
	GAME_RESULT,
	READY
};


class Para : public ThreadParameter {

public:
	virtual void ReceiveDataCallBack() {
		
	}
	virtual void CloseSocketCallBack() {

	}
};


class GameManager
{
public:
	GameManager();
	~GameManager();

	Para parara;
private:
	TCPServer gameServer;
	static void GameProcess(LPSOCKET_INFO socketInfo, char* message);
	static void CloseSocketCallBack(LPSOCKET_INFO socketInfo);
	static unsigned WINAPI GameThread(void* para);
};

//SOCKET GameManager::webserverSocket = 0;
