#include "GameManager.h"



GameManager::GameManager()
{
	/*
	Param p;
	p.recvCallBack = GameProcess;
	p.socketClosetCallBack = CloseSocketCallBack;
	*/
	gameServer = TCPServer(&parara);
	gameServer.run();
}


GameManager::~GameManager()
{

}

void GameManager::GameProcess(LPSOCKET_INFO socketInfo, char* message)
{
	cout << "GameProcess called!!" << endl;

	MESSAGE recvMessage = *(MESSAGE*)message;
	printf("========recv message============\n");
	printf("recvMessage.type : %d\n", recvMessage.type);
	printf("recvMessage.roomId : %d\n", recvMessage.roomId);
	printf("recvMessage.writer : %s\n", recvMessage.writer);
	printf("recvMessage.text : %s\n", recvMessage.text);

	int id;
	MESSAGE transMessage;
	string resultString;
	UserInfo resultUser;


	switch (recvMessage.type)
	{
	
	}


	GameRoomInfo& gameRoom = GameRoomRepository::GetGameRoomInfo(recvMessage.roomId);
	if (gameRoom.id == -1)
	{
		//cout << "gameROom ID: " << gameRoom.id << " not found" << endl;
		//return;
	}
	cout << "gameROom ID: " << gameRoom.id << " size : " << gameRoom.userList.size() << endl;

	switch(recvMessage.type){
	case POLLING:
		std::cout << ">> POLLING" << std::endl;
		GameRoomRepository::SetWebServerSocket(socketInfo->clientSocket);
		break;
	case CREATE:
		std::cout << ">> CREATE" << std::endl;

		id = GameRoomRepository::CreateRoom();

		transMessage = {
			CREATE_ACK,
			id,
			"server",
			"room created"
		};

		//send to webserver
		//GameRoomRepository::SendToWebServer((char*)&transMessage);
		TCPServer::send(socketInfo->clientSocket, (char*)&transMessage, BUF_SIZE);
		break;
	case JOIN_REQ:
		std::cout << ">> JOIN_REQ" << std::endl;

		if (GameRoomRepository::RequestJoin(recvMessage.roomId))//
		{
			resultString = "success";
		}
		else
		{
			resultString = "fail";
		}

		transMessage = {
			JOIN_REQ_ACK,
			recvMessage.roomId,
			"server"
		};
		memcpy(transMessage.text, resultString.c_str(), resultString.size());

		//send to webserver
		GameRoomRepository::SendToWebServer((char*)&transMessage);
		break;
	case JOIN://사용자가 TCP서버 접속
		std::cout << ">> JOIN" << std::endl;

		//setUserInfo
		socketInfo->roomId = recvMessage.roomId;
		socketInfo->userId = string(recvMessage.writer);

		transMessage = recvMessage;

		GameRoomRepository::JoinGame(gameRoom, socketInfo);
		GameRoomRepository::BroadCastMessage(gameRoom.userList, (char*)&transMessage, BUF_SIZE);
		//send to web
		GameRoomRepository::SendToWebServer((char*)&transMessage);
		break;
	case CHAT:
		std::cout << ">> CHAT" << std::endl;

		GameRoomRepository::BroadCastMessage(gameRoom.userList, message, BUF_SIZE);
		break;
	case READY:
		std::cout << ">> READY" << std::endl;

		GameRoomRepository::OnOffFlag(gameRoom, socketInfo);

		if (GameRoomRepository::CheckAllPlayerReady(gameRoom))
		{
			cout << " All Player Ready" << endl;

			//send job
			GameRoomRepository::SetPlayerJob(gameRoom);

			for (auto user : gameRoom.userList)
			{
				MESSAGE msg{
					JOB,
					recvMessage.roomId,
					"server"
				};
				memcpy(msg.text, user.job.c_str(), user.job.size());
				TCPServer::send(user.socket, (char*)&msg, BUF_SIZE);
			}


			//game start
			_beginthreadex(NULL, 0, GameThread, (LPVOID)&gameRoom, 0, NULL);
		}

		break;
	case VOTE:
		std::cout << ">> VOTE" << std::endl;
		GameRoomRepository::VoteUser(gameRoom, socketInfo, string(recvMessage.text));

		transMessage = {
			ACTION,
			recvMessage.roomId,
			"server"
		};
		TCPServer::send(socketInfo->clientSocket, (char*)&transMessage, BUF_SIZE);

		if (GameRoomRepository::CheckReadyToGetResult(gameRoom))
		{
			transMessage = {
				VOTE_RESULT,
				recvMessage.roomId,
			};

			resultUser = GameRoomRepository::GetVoteResult(gameRoom);
			if (resultUser.voteCnt == -1)//동률
			{
				memcpy(transMessage.writer, std::string("same").c_str(), 4);
			}
			else
			{
				memcpy(transMessage.writer, resultUser.userId.c_str(), resultUser.userId.size());
				memcpy(transMessage.text, resultUser.job.c_str(), resultUser.job.size());
			}

			GameRoomRepository::SetFlagFalse(gameRoom);

			GameRoomRepository::BroadCastMessage(gameRoom.userList, (char*)&transMessage, BUF_SIZE);
		}



		break;
	case ACTION:
		std::cout << ">> ACTION" << std::endl;

		resultString = GameRoomRepository::ActUserJob(gameRoom, socketInfo, string(recvMessage.text));


		//경찰일때만
		if (resultString != "ok")
		{
			transMessage = {
				ACTION_RESULT,
				recvMessage.roomId,
			};
			memcpy(transMessage.writer, recvMessage.text, 16);
			memcpy(transMessage.text, resultString.c_str(), resultString.size());

			TCPServer::send(socketInfo->clientSocket, (char*)&transMessage, BUF_SIZE);
		}


		if (GameRoomRepository::CheckReadyToGetResult(gameRoom))
		{
			transMessage = {
				VOTE_RESULT,
				recvMessage.roomId,
			};

			resultUser = GameRoomRepository::GetVoteResult(gameRoom);
			if (resultUser.voteCnt == -1)//동률
			{
				memcpy(transMessage.writer, std::string("same").c_str(), 4);
			}
			else
			{
				memcpy(transMessage.writer, resultUser.userId.c_str(), resultUser.userId.size());
				memcpy(transMessage.text, resultUser.job.c_str(), resultUser.job.size());
			}

			GameRoomRepository::SetFlagFalse(gameRoom);

			GameRoomRepository::BroadCastMessage(gameRoom.userList, (char*)&transMessage, BUF_SIZE);
		}

		break;
	case ACTION_RESULT:
		std::cout << ">> ACTION_RESULT" << std::endl;
		
		GameRoomRepository::SetActionResult(gameRoom);

		if (GameRoomRepository::CheckReadyToGetResult(gameRoom))
		{
			transMessage = {
				VOTE_RESULT,
				recvMessage.roomId,
			};

			resultUser = GameRoomRepository::GetVoteResult(gameRoom);
			if (resultUser.voteCnt == -1)//동률
			{
				memcpy(transMessage.writer, std::string("same").c_str(), 4);
			}
			else
			{
				memcpy(transMessage.writer, resultUser.userId.c_str(), resultUser.userId.size());
				memcpy(transMessage.text, resultUser.job.c_str(), resultUser.job.size());
			}

			GameRoomRepository::SetFlagFalse(gameRoom);

			GameRoomRepository::BroadCastMessage(gameRoom.userList, (char*)&transMessage, BUF_SIZE);
		}


		break;
	case VOTE_RESULT:
		std::cout << ">> VOTE_RESULT" << std::endl;
		GameRoomRepository::OnOffFlag(gameRoom, socketInfo);

		if (!GameRoomRepository::CheckAllPlayerReady(gameRoom))
		{
			cout << "vote result not yet" << endl;
			break;
		}


		GameRoomRepository::SetFlagFalse(gameRoom);

		resultUser = GameRoomRepository::GetGameResult(gameRoom);

		
		transMessage = {
			GAME_RESULT,
			recvMessage.roomId,
		};
		if (resultUser.voteCnt == -1)//다시 시작
		{
			memcpy(transMessage.writer, resultUser.userId.c_str(), resultUser.userId.size());
			memcpy(transMessage.text, resultUser.job.c_str(), resultUser.job.size());
		}
		else 
		{
			memcpy(transMessage.writer, resultUser.job.c_str(), resultUser.job.size());
			memcpy(transMessage.text, resultUser.userId.c_str(), resultUser.userId.size());
		}

		std::cout << "before send result " << std::endl;

		GameRoomRepository::BroadCastMessage(gameRoom.userList, (char*)&transMessage, BUF_SIZE);


		//repo->InitPlayerCheck(gameRoom);/

		if (resultUser.voteCnt == -1)//다시 시작
		{
			std::cout << "!!!!!!!!!! restart !!!!!!!!!" << std::endl;
			//repo->InitPlayerCheck(gameRoom);
			//game start
			//PlayingProcess((void*)&gameRoom);


			//_beginthreadex(NULL, 0, GameThread, (LPVOID)&gameRoom, 0, NULL);
		}
		else
		{
			//초기화
			for (auto& user : gameRoom.userList)
			{
				user.action = false;
				user.dead = false;
				user.flag = false;
				user.job = "CIVIL";
				user.voteCnt = 0;
			}
		}
		
		break;
	case QUIT:
		std::cout << ">> QUIT " << std::endl;

		if (GameRoomRepository::QuitGame(gameRoom, socketInfo)) {
			MESSAGE message = {
				QUIT,
				socketInfo->roomId
			};
			string sz = to_string(gameRoom.userList.size());
			memcpy(message.writer, socketInfo->userId.c_str(), socketInfo->userId.size());
			memcpy(message.text, sz.c_str(), sz.size());

			GameRoomRepository::BroadCastMessage(gameRoom.userList, (char*)&message, BUF_SIZE);

		}

		transMessage = {
			QUIT,
			socketInfo->roomId,
			"server",
		};
		memcpy(transMessage.text, socketInfo->userId.c_str(), socketInfo->userId.size());
		GameRoomRepository::SendToWebServer((char*)&transMessage);


		break;
	default:

		break;
	}





	//TCPServer::send(socketInfo->clientSocket, message, 152);
}

void GameManager::CloseSocketCallBack(LPSOCKET_INFO socketInfo)
{
	GameRoomInfo gameRoom = GameRoomRepository::GetGameRoomInfo(socketInfo->roomId);
	if (GameRoomRepository::QuitGame(gameRoom, socketInfo)) {
		MESSAGE message = {
			QUIT,
			socketInfo->roomId
		};
		string sz = to_string(gameRoom.userList.size());
		memcpy(message.writer, socketInfo->userId.c_str(), socketInfo->userId.size());
		memcpy(message.text, sz.c_str(), sz.size());

		GameRoomRepository::BroadCastMessage(gameRoom.userList, (char*)&message, BUF_SIZE);

	}

	MESSAGE message = {
		QUIT,
		socketInfo->roomId,
		"server",
	};
	memcpy(message.text, socketInfo->userId.c_str(), socketInfo->userId.size());
	GameRoomRepository::SendToWebServer((char*)&message);

}



unsigned WINAPI GameManager::GameThread(void* para)
{
	std::cout << "PlayingProcess Start!" << std::endl;
	std::string userList = "";

	GameRoomInfo* gameRoom = (GameRoomInfo*)para;

	//init /*
	/*
	gameRoom->actionResult = false;
	gameRoom->deadUser = UserInfo();
	gameRoom->votedUser = UserInfo();
	gameRoom->voteCnt = 0;
	gameRoom->liveCnt = 0;
	*/
	for (auto& user : gameRoom->userList)
	{
		user.action = false;
		user.flag = false;
		user.voteCnt = 0;
		if (user.dead == false) {
			userList += user.userId + ",";
		}
	}
	userList.erase(userList.end() - 1);
	//std::this_thread::sleep_for(std::chrono::milliseconds(100));


	MESSAGE msg = {
		GAME_START,
		0,
		"server"
	};
	memcpy(msg.text, userList.c_str(), userList.size());
	GameRoomRepository::BroadCastMessage(gameRoom->userList, (char*)&msg, BUF_SIZE);


	std::cout << "wait timer" << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(PLAYING_TIME));
	std::cout << "end of sleep_for========= " << std::endl;

	msg = {
		VOTE,
		0,
		"server",
		"vote_start"
	};
	GameRoomRepository::BroadCastMessage(gameRoom->userList, (char*)&msg, BUF_SIZE);

	std::cout << "send vote message" << std::endl;
	return 0;
}