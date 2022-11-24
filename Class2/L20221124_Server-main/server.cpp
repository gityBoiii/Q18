#define _WINSOCK_DEPRECATED_NO_WARNINGS 

#include <iostream>
#include <WinSock2.h>
#include <process.h>
#include <Windows.h>
#include <vector>
#include <map>
#include "MessagePacket.h"
#include "PlayerData.h"
#include <string>

using namespace std;

#pragma comment(lib, "WS2_32.lib")


vector<PlayerData*> UserList;

map<SOCKET, PlayerData*> PlayerList;


CRITICAL_SECTION ServerCS;

//		2			8		  4         4
//  Code	   SOCKET_ID      X         Y
//[][]    [][][][][][][][]  [][][][] [][][][]
//0       2                 10       14
char Data[18] = { 0, };


unsigned WINAPI WorkThread(void* Arg)
{
	SOCKET ClientSocket = *(SOCKET*)Arg;

	while (true)
	{
		int RecvLength = recv(ClientSocket, Data, sizeof(Data), 0);

		if (RecvLength <= 0)
		{
			//disconnect
			closesocket(ClientSocket);
			EnterCriticalSection(&ServerCS);
			PlayerList.erase(PlayerList.find(ClientSocket));
			LeaveCriticalSection(&ServerCS);
			break;
		}
		else
		{
			//packet parse, deserialize
			unsigned short Code = 0;
			memcpy(&Code, &Data[0], sizeof(Code));
			SOCKET FromID = 0;
			memcpy(&FromID, &Data[2], sizeof(FromID));

			Code = ntohs(Code);
			FromID = ntohll(FromID);

			int X = 0;
			int Y = 0;

			switch ((MessagePacket)Code)
			{
				case MessagePacket::C2S_Move:
				{
					memcpy(&X, &Data[10], sizeof(X));
					X = ntohl(X);
					memcpy(&Y, &Data[14], sizeof(Y));
					Y = ntohl(Y);

					//update PlayerList
					auto UpdatePlayer = PlayerList.find(FromID);
					UpdatePlayer->second->X = X;
					UpdatePlayer->second->Y = Y;

					unsigned short Code = htons((unsigned short)MessagePacket::S2C_Move);
					memcpy(&Data[0], &Code, sizeof(Code));
					SOCKET SendID = htonll(FromID);
					memcpy(&Data[2], &SendID, sizeof(SendID));
					int Temp = htonl(X);
					memcpy(&Data[10], &Temp, sizeof(Temp));
					Temp = htonl(Y);
					memcpy(&Data[14], &Temp, sizeof(Temp));

					//send
					for (auto Player : PlayerList)
					{
						int SentBytes = send(Player.second->MySocket, Data, sizeof(Data), 0);
					}
				}
				break;

			default:
				break;
			}
		}
	}

	return 0;
}

int main()
{
	InitializeCriticalSection(&ServerCS);

	WSAData wsaData;

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN ListenSocketAddr;
	memset(&ListenSocketAddr, 0, sizeof(ListenSocketAddr));
	ListenSocketAddr.sin_family = PF_INET;
	ListenSocketAddr.sin_port = htons(4949);
	ListenSocketAddr.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(ListenSocket, (SOCKADDR*)&ListenSocketAddr, sizeof(ListenSocketAddr));

	listen(ListenSocket, 0);

	while (true)
	{
		//연결 처리
		SOCKADDR_IN ClientSockAddr;
		memset(&ClientSockAddr, 0, sizeof(ClientSockAddr));
		int ClientSockAddrLength = sizeof(ClientSockAddr);
		SOCKET NewClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientSockAddr, &ClientSockAddrLength);

		cout << "connect : " << NewClientSocket << endl;
		EnterCriticalSection(&ServerCS);
		PlayerData* NewPlayer = new PlayerData();
		NewPlayer->MySocket = NewClientSocket;
		PlayerList[NewClientSocket] = NewPlayer;
		LeaveCriticalSection(&ServerCS);

		//S2C_RegisterID

		unsigned short Code = htons((unsigned short)MessagePacket::S2C_RegisterID);
		memcpy(&Data[0], &Code, sizeof(Code));
		SOCKET SendID = htonll(NewClientSocket);
		memcpy(&Data[2], &SendID, sizeof(SendID));
		int SentBytes = send(NewClientSocket, Data, sizeof(Data), 0);
		cout << "MessagePacket::S2C_RegisterID " << endl;


		EnterCriticalSection(&ServerCS);
		//신규 접속한 유저를 다른 모든 유저 생성 시킴
		for (auto Player : PlayerList)
		{
			//S2C_Spawn
			if (Player.first != NewClientSocket)
			{
				unsigned short Code = htons((unsigned short)MessagePacket::S2C_Spawn);
				memcpy(&Data[0], &Code, sizeof(Code));
				SOCKET SpawnID = htonll(NewClientSocket);
				memcpy(&Data[2], &SpawnID, sizeof(SpawnID));
				send(Player.second->MySocket, Data, sizeof(Data), 0);
				//cout << "MessagePacket::S2C_Spawn 1" << endl;
				//cout << "Spawn " << NewClientSocket << endl;
				//cout << "to " << Player.second->MySocket << endl;
			}
		}

		//현재 접속한 유저 정보를 신규 유저한테 전달
		for (auto Player : PlayerList)
		{
			if (Player.first != NewClientSocket)
			{
				//S2C_Spawn
				unsigned short Code = htons((unsigned short)MessagePacket::S2C_Spawn);
				memcpy(&Data[0], &Code, sizeof(Code));
				SOCKET SpawnID = htonll(Player.second->MySocket);
				memcpy(&Data[2], &SpawnID, sizeof(SpawnID));
				send(NewClientSocket, Data, sizeof(Data), 0);
				//cout << "MessagePacket::S2C_Spawn 2" << endl;
				//cout << "Spawn " << Player.first << endl;
				//cout << "to " << NewClientSocket << endl;
			}
		}

		cout << "Player Count : " << PlayerList.size() << endl;
		for (auto Player : PlayerList)
		{
			cout << "Player ID : " << Player.second->MySocket;
		}
		LeaveCriticalSection(&ServerCS);





		HANDLE ThreadHandle = (HANDLE)_beginthreadex(nullptr,
			0,
			WorkThread,
			(void*)&NewClientSocket,
			0,
			nullptr
		);
	}


	closesocket(ListenSocket);

	DeleteCriticalSection(&ServerCS);


	WSACleanup();
}