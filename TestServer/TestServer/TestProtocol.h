#pragma once

#pragma pack(1)

enum TestCmd {
	CMD_LOGIN_REQUEST = 1,
	CMD_LOGIN_RESPONSE = 2,
};

struct BaseMsgHead {
	int   packetSize;
	int   cmd;
};

struct LoginRequest {
	int   userId;
	char  reserved[1024];
};

struct LoginResponse {
	int   errorCode;
	int   userId;
	char  reserved[1024];
};

#pragma pack()