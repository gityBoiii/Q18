#pragma once
#include <WinSock2.h>
class PlayerData
{
public:
	SOCKET MySocket;
	int X;
	int Y;

	bool operator==(const PlayerData& RHS) // Right Hand Side
	{
		return this->MySocket == RHS.MySocket;
	}
};

