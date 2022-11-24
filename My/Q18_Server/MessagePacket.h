#pragma once

enum class MessagePacket
{
	S2C_RegisterID = 100,
	S2C_Spawn = 200,
	S2C_Destroy = 300,
	S2C_Move = 400,
	C2S_Move = 500,
};