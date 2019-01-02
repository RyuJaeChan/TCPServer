#pragma once

#include <string>
#include <WinSock2.h>
#include <vector>
#include <map>
#include <iostream>
#include <mutex>
#include <cstdlib>
#include <ctime>
#include <algorithm>

#include "TCPServer.h"
#include "SingleTon.h"

using namespace std;

#define MAX_ROOM_SIZE 6
#define POLICE "POLICE"
#define MAFIA "MAFIA"

struct UserInfo
{
	int voteCnt = 0;
	SOCKET socket;
	int flag = false;
	string userId;
	string job = "CIVIL";
	bool dead = false;
	bool action = false;
};

class GameRoomInfo
{
public:
	GameRoomInfo();
	GameRoomInfo(int _id);
	~GameRoomInfo();

	int id;
	UserInfo deadUser;
	UserInfo votedUser;
	bool actionResult = false;
	vector<UserInfo> userList;

private:


};



class GameRoomRepository {
public:
	static void SetWebServerSocket(SOCKET socket);
	static int CreateRoom();
	static bool RequestJoin(int roomId);
	static bool JoinGame(GameRoomInfo& gameRoom, LPSOCKET_INFO userInfo);
	static int QuitGame(GameRoomInfo& gameRoom, LPSOCKET_INFO userInfo);

	static void OnOffFlag(GameRoomInfo& gameRoom, LPSOCKET_INFO socketInfo);
	static bool CheckAllPlayerReady(GameRoomInfo gameRoom);

	static void SetPlayerJob(GameRoomInfo& gameRoom);

	static bool CheckReadyToGetResult(GameRoomInfo gameRoom);
	static void VoteUser(GameRoomInfo& gameRoom, LPSOCKET_INFO voting, string votedUserId);
	static string ActUserJob(GameRoomInfo& gameRoom, LPSOCKET_INFO actUser, string selectedUserId);
	static void SetActionResult(GameRoomInfo& gameRoom);
	static UserInfo GetVoteResult(GameRoomInfo& gameRoom);
	static UserInfo GetGameResult(GameRoomInfo& gameRoom);

	static void SetFlagFalse(GameRoomInfo& gameRoom);
	static void BroadCastMessage(vector<UserInfo> userList, char* message, size_t size);
	static void SendToWebServer(char* message);
	static GameRoomInfo& GetGameRoomInfo(int roomId);
private:

};

static mutex mtx;
static mutex getUser;
static int id;
static map<int, GameRoomInfo> gameRoomList;
static SOCKET webserverSocket;