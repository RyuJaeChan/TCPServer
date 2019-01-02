#include "GameRoomInfo.h"



GameRoomInfo::GameRoomInfo() : id(0), deadUser(), votedUser(), actionResult(false), userList(vector<UserInfo>())
{
}

GameRoomInfo::GameRoomInfo(int _id) : id(_id), deadUser(), votedUser(), actionResult(false), userList(vector<UserInfo>())
{
}

GameRoomInfo::~GameRoomInfo()
{
}

void GameRoomRepository::SetWebServerSocket(SOCKET socket)
{
	cout << "foo static called" << endl;
	webserverSocket = socket;
}

int GameRoomRepository::CreateRoom()
{
	mtx.lock();

	int res = ++id;

	gameRoomList.insert(make_pair(res, GameRoomInfo(res)));

	cout << "create room id : " << res << endl;

	mtx.unlock();
	return res;
}

bool GameRoomRepository::RequestJoin(int roomId)
{
	mtx.lock();

	GameRoomInfo gameRoom = GetGameRoomInfo(roomId);
	
	//�����ʰ�
	if (gameRoom.userList.size() >= MAX_ROOM_SIZE)
	{
		mtx.unlock();
		cout << "join req fail : MAX_ROOM" << endl;
		return false;
	}

	mtx.unlock();
	cout << "join req succ : MAX_ROOM" << endl;
	return true;
}

bool GameRoomRepository::JoinGame(GameRoomInfo& gameRoom, LPSOCKET_INFO userInfo)
{
	mtx.lock();

	if (gameRoom.userList.size() >= MAX_ROOM_SIZE)
	{
		mtx.unlock();
		cout << "join fail : MAX_ROOM" << endl;
		return false;
	}

	UserInfo user;
	user.socket = userInfo->clientSocket;
	user.userId = userInfo->userId;

	gameRoom.userList.push_back(user);

	mtx.unlock();


	cout << "join success : "  << gameRoom.userList.size() << endl;
	return true;
}

int GameRoomRepository::QuitGame(GameRoomInfo& gameRoom, LPSOCKET_INFO userInfo)
{
	mtx.lock();

	//���� ������ 1�̸� ���� �� �޽��� �����ʿ����.
	int ret = 0;
	do {
		if (gameRoom.userList.size() == 1)
		{
			gameRoomList.erase(id);
			break;
		}

		int i = 0;
		bool exist = false;
		for (auto it : gameRoom.userList)
		{
			if (userInfo->userId == it.userId)
			{
				exist = true;
				std::cout << "quit player : " << it.userId << std::endl;
				break;
			}
			i++;
		}

		if (exist == false)
		{
			std::cout << "not exist player : " << userInfo->userId << std::endl;
			break;
		}

		ret = 1;
	} while (false);
	
	mtx.unlock();
	return ret;
}

void GameRoomRepository::OnOffFlag(GameRoomInfo& gameRoom, LPSOCKET_INFO socketInfo)
{
	mtx.lock();
	for (auto& it : gameRoom.userList)
	{
		if (it.userId == socketInfo->userId)
		{
			it.flag ^= 1;//ready of/off
		}
	}
	mtx.unlock();
}

bool GameRoomRepository::CheckAllPlayerReady(GameRoomInfo gameRoom)
{
	mtx.lock();

	int cnt = 0;
	for (auto user : gameRoom.userList)
	{
		if (user.flag == true)
		{
			cnt++;
		}
	}

	mtx.unlock();
	cout << "CheckAllPlayerReady : return cnt == MAX_ROOM_SIZE = " << (cnt == MAX_ROOM_SIZE) << endl;
	return cnt == MAX_ROOM_SIZE;
}

void GameRoomRepository::SetPlayerJob(GameRoomInfo& gameRoom)
{
	mtx.lock();

	srand((unsigned)time(NULL));
	
	int polIndex = rand() % MAX_ROOM_SIZE;
	int mafiaIndex;
	while ((mafiaIndex = rand() % MAX_ROOM_SIZE) == polIndex);

	gameRoom.userList[polIndex].job = POLICE;
	gameRoom.userList[mafiaIndex].job = MAFIA;

	mtx.unlock();
}

bool GameRoomRepository::CheckReadyToGetResult(GameRoomInfo gameRoom)
{
	mtx.lock();

	if (gameRoom.actionResult == false)
	{
		cout << "CheckReadyToGetResult : POLIC don't send Action result" << endl;
		mtx.unlock();
		return false;
	}


	for (auto it : gameRoom.userList)
	{
		if (it.flag == false)
		{
			cout << "CheckReadyToGetResult : all player not vote" << endl;
			mtx.unlock();
			return false;
		}

		if (it.job == MAFIA && it.action == false)
		{
			cout << "CheckReadyToGetResult : MAFIA not Action" << endl;
			mtx.unlock();
			return false;
		}

		if (it.job == POLICE && it.action == false)
		{
			cout << "CheckReadyToGetResult : POLICE not Action" << endl;
			mtx.unlock();
			return false;
		}
	}

	mtx.unlock();
	cout << "CheckReadyToGetResult : return true" << endl;
	return true;
}

void GameRoomRepository::VoteUser(GameRoomInfo & gameRoom, LPSOCKET_INFO voting, string votedUserId)
{
	mtx.lock();
	for (auto& it : gameRoom.userList)
	{
		if (it.userId == voting->userId)
		{
			it.flag = true;
		}

		if (it.userId == votedUserId)
		{
			it.voteCnt++;
			std::cout << " " << it.userId << " voted : " << it.voteCnt << std::endl;
		}
	}
	mtx.unlock();
}

string GameRoomRepository::ActUserJob(GameRoomInfo & gameRoom, LPSOCKET_INFO actUser, string selectedUserId)
{
	for (auto& user : gameRoom.userList)
	{
		if (user.userId == actUser->userId)
		{
			if (user.job == MAFIA)
			{
				//���� ����
				for (auto& it : gameRoom.userList)
				{
					if (it.userId == selectedUserId)
					{
						std::cout << user.userId << "(MAFIA) kills [" << selectedUserId << "]" << std::endl;
						gameRoom.deadUser = it;
						it.dead = true;
						user.action = true;
						return "ok";
					}
				}
				return "ok";
			}
			else if (user.job == POLICE)
			{
				//�ش� ������ ���Ǿ�����
				user.action = true;
				for (auto it : gameRoom.userList)
				{
					if (it.userId == selectedUserId)
					{
						if (it.job == MAFIA)
						{
							std::cout << user.userId << "(POLICE) select  " << selectedUserId << "(MAFIA)" << std::endl;
							return "MAFIA";
						}
					}
				}
				std::cout << user.userId << "(POLICE) select  " << selectedUserId << "(CIVIL)" << std::endl;
				return "CIVIL";
			}
		}
	}

	std::cout << actUser->userId << " act  " << selectedUserId << " FAIL" << std::endl;
	return "FAIL";
}

void GameRoomRepository::SetActionResult(GameRoomInfo& gameRoom)
{
	mtx.lock();
	gameRoom.actionResult = true;
	mtx.unlock();
}

UserInfo GameRoomRepository::GetVoteResult(GameRoomInfo& gameRoom)
{
	sort(gameRoom.userList.begin(), gameRoom.userList.end(),
		[](const UserInfo& a, const UserInfo b)->bool {
		return a.voteCnt > b.voteCnt;
	});

	//debug
	std::cout << "vote sorted result : ";
	for (auto user : gameRoom.userList)
	{
		std::cout << user.userId << " : " << user.voteCnt << " , ";
	}
	std::cout << std::endl;

	if (gameRoom.userList.front().voteCnt
		== gameRoom.userList[1].voteCnt)
	{
		UserInfo temp;
		temp.voteCnt = -1;
		return temp;
	}

	gameRoom.votedUser.dead = true;
	gameRoom.votedUser = gameRoom.userList.front();

	for (auto& user : gameRoom.userList)
	{
		if (gameRoom.votedUser.userId == user.userId)
		{
			user.dead = true;
		}
		user.flag = false;
		user.voteCnt = 0;
	}

	return gameRoom.votedUser;
}

UserInfo GameRoomRepository::GetGameResult(GameRoomInfo & gameRoom)
{
	std::cout << "GetGameResult Called" << std::endl;
	int userCnt = 0;

	UserInfo civil = gameRoom.userList.back();
	UserInfo mafia;

	for (auto& user : gameRoom.userList)
	{
		if (user.job == MAFIA)
		{
			mafia = user;
			if (gameRoom.votedUser.userId == user.userId)
			{
				std::cout << "return victory civil:" << user.userId << std::endl;
				UserInfo temp;
				temp.userId = "CIVIL";
				temp.job = "CIVIL";
				return temp;//�ùν¸�
			}
		}
		civil = user;


		if (user.dead == false)
		{
			userCnt++;
		}
	}

	if (userCnt <= 3) //���Ǿ�1 ����2 ����
	{
		std::cout << "retur mafia:" << mafia.userId << std::endl;
		return mafia;//���Ǿƽ¸�
	}

	gameRoom.deadUser.voteCnt = -1;

	std::cout << "retur deadUser:" << gameRoom.deadUser.userId << std::endl;
	return gameRoom.deadUser;//�ٽý���
}

void GameRoomRepository::SetFlagFalse(GameRoomInfo & gameRoom)
{
	mtx.lock();
	for (auto& user : gameRoom.userList)
	{
		user.flag = false;
	}
	mtx.unlock();
}

void GameRoomRepository::BroadCastMessage(vector<UserInfo> userList, char* message, size_t size)
{
	for (auto user : userList)
	{
		TCPServer::send(user.socket, message, size);
	}
}

void GameRoomRepository::SendToWebServer(char * message)
{
	TCPServer::send(webserverSocket, message, BUF_SIZE);
}

GameRoomInfo& GameRoomRepository::GetGameRoomInfo(int roomId)
{
	getUser.lock();
	auto element = gameRoomList.find(roomId);

	if (element == gameRoomList.end())
	{
		getUser.unlock();
		return GameRoomInfo();
	}
	cout << "GetGameRoomInfo room id : " << element->first << endl;
	
	getUser.unlock();
	return element->second;
}
