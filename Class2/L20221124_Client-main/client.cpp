#define _WINSOCK_DEPRECATED_NO_WARNINGS 

#include <iostream>
#include <WinSock2.h>
#include <process.h>
#include <Windows.h>
#include <map>
#include <conio.h>
#include "MessagePacket.h"
#include "PlayerData.h"


using namespace std;

#pragma comment(lib, "WS2_32.lib")

//		2			8		  4         4
//  Code	   SOCKET_ID      X         Y
//[][]    [][][][][][][][]  [][][][] [][][][]
//0       2                 10       14
char Data[18] = { 0, };

map<SOCKET, PlayerData*> PlayerList;

SOCKET MySocketID = 0L;


void ProcessPacket(char* Packet)
{
	unsigned short Code = 0;
	memcpy(&Code, &Data[0], sizeof(Code));
	SOCKET FromID = 0;
	memcpy(&FromID, &Data[2], sizeof(FromID));

	Code = ntohs(Code);
	FromID = ntohll(FromID);
	PlayerData* NewPlayer = nullptr;
	switch ((MessagePacket)Code)
	{
		case MessagePacket::S2C_RegisterID:
//			cout << "MessagePacket::S2C_RegisterID " << endl;
			NewPlayer = new PlayerData();
			NewPlayer->MySocket = FromID;
			MySocketID = FromID;
			PlayerList[FromID] = NewPlayer;
		break;

		case MessagePacket::S2C_Spawn:
//			cout << "MessagePacket::S2C_Spawn " << endl;

			NewPlayer = new PlayerData();
			NewPlayer->MySocket = FromID;
			PlayerList[FromID] = NewPlayer;
		break;

		case MessagePacket::S2C_Destroy:
			PlayerList.erase(PlayerList.find(FromID));
		break;

		case MessagePacket::S2C_Move:
		{
			int X;
			int Y;
			memcpy(&X, &Data[10], sizeof(X));
			X = ntohl(X);
			memcpy(&Y, &Data[14], sizeof(Y));
			Y = ntohl(Y);

			//update PlayerList
			auto UpdatePlayer = PlayerList.find(FromID);
			UpdatePlayer->second->X = X;
			UpdatePlayer->second->Y = Y;
		}
		break;


		default:
			cout << "Not found code." << Code << endl;
		break;

	}

	for (auto Player : PlayerList)
	{
		cout << "Player ID : " << Player.second->MySocket << " : "
			<< Player.second->X << ", " << Player.second->Y << endl;
	}
}


unsigned WINAPI WorkThread(void* Arg)
{
	SOCKET ServerSocket;
	char Buffer[1024] = { 0, };

	ServerSocket = *(SOCKET*)Arg;

	while (true)
	{
		int RecvBytes = recv(ServerSocket, Data, sizeof(Data), 0);
		cout << "RecvBytes : " << RecvBytes << endl;
		if (RecvBytes <= 0)
		{
			//서버랑 끊김
			break;
		}
		else
		{
			//패킷 처리(받은 자료 크기가 정확하다)
			ProcessPacket(Data);
		}
	}

	return 0;
}




int main()
{
	WSAData wsaData;

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN ServerSockAddr;
	memset(&ServerSockAddr, 0, sizeof(ServerSockAddr));
	ServerSockAddr.sin_family = PF_INET;
	ServerSockAddr.sin_port = htons(4949);
	ServerSockAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	connect(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr));

	HANDLE WorkHandle = (HANDLE)_beginthreadex(nullptr, 0, WorkThread, (void*)&ServerSocket, 0, nullptr);

	while (true)
	{
		int KeyCode = _getch();

		map<SOCKET, PlayerData*>::iterator MyPlayer = PlayerList.find(MySocketID);
		switch (KeyCode)
		{
			case 'W':
			case 'w':
				MyPlayer->second->Y--;
				break;

			case 'S':
			case 's':
				MyPlayer->second->Y++;
				break;

			case 'A':
			case 'a':
				MyPlayer->second->X--;
				break;

			case 'D':
			case 'd':
				MyPlayer->second->X++;
				break;
		}

		unsigned short Code = htons((unsigned short)MessagePacket::C2S_Move);
		memcpy(&Data[0], &Code, sizeof(Code));
		SOCKET SendID = htonll(MyPlayer->second->MySocket);
		memcpy(&Data[2], &SendID, sizeof(SendID));
		int Temp = htonl(MyPlayer->second->X);
		memcpy(&Data[10], &Temp, sizeof(Temp));
		Temp = htonl(MyPlayer->second->Y);
		memcpy(&Data[14], &Temp, sizeof(Temp));

		int SentBytes = send(ServerSocket, Data, sizeof(Data), 0);
	}

	WSACleanup();
}