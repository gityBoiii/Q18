#pragma once
enum class MessagePacket
{
	//Server To Client
	S2C_RegisterID			= 100,
	S2C_Spawn				= 200,
	S2C_Destroy				= 300,
	C2S_Move				= 400,
	S2C_Move				= 500,
};
