#pragma once

// ------------------ include file ---------------------
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <set>
#include <queue>
#include <map>
#include <sstream>
#include <stdarg.h>


#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define MARKUP_STL
    #include <Windows.h>
    #include <WinSock2.h>
    #include <MSWSock.h>
    #include <ws2tcpip.h>
    #include <assert.h>
    #include <math.h>
    #include <string.h>
    #include <ATLComTime.h>
    #include <icrsint.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "Mswsock")
    #include <direct.h>
    #include <io.h>
    
    #pragma warning(disable:4311)
    #pragma warning(disable:4312)

    typedef int socklen_t;
#else
    #include <unistd.h>
    #include <pthread.h>
    #include <semaphore.h>
    #include <signal.h>
    #include <sys/stat.h>
    #include <sys/epoll.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <net/if.h>
    #include <arpa/inet.h>
    #include <errno.h>
    #include <netdb.h>
    #include <sys/ioctl.h>

    typedef unsigned int     SOCKET;
    #define INVALID_SOCKET   (SOCKET)(~0)
    #define SOCKET_ERROR     (-1)
    typedef struct sockaddr  SOCKADDR;
    #ifndef INADDR_ANY
        #define INADDR_ANY   (unsigned long)0x00000000
    #endif

    #ifndef min
        #define min(a,b)            (((a) < (b)) ? (a) : (b))
    #endif
#endif

#include <time.h>
#include <string.h>
#include <fcntl.h>

enum NetServiceErr {
	NSE_SUCCESS = 0,
	NSE_ILLEGAL_RECV_PACKET = 1,
	NSE_REMOTE_DISCONNECT = 2,
	NSE_SYSTEM_ERROR = 3,
	NSE_INVALID_SOCKET = 4,
	NSE_BE_CLOSED = 5,
	NSE_SEND_QUEUE_FULL = 6,
	NSE_ILLEGAL_SEND_PACKET = 7,
	NSE_BIND_ADDR_FAILED = 8,
	NSE_EXCEPTION = 9,
	NSE_RECV_FAILED = 10,
	NSE_RECV_QUEUE_FULL = 11,
	NSE_SEND_NOT_COMPLETE = 12,
	NSE_TIMEOUT = 13,
	NSE_CONNECT_FAILED = 14,
    NSE_INVALID_PARAM = 15,
};


// ------------------ Macro define ---------------------
#define WT_BEGIN namespace wt{
#define WT_END   };
#define WT_USE   using namespace wt;