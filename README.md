# iocp-netlib

A **simple, robust, and high-performance C++ TCP networking library for Windows**,  
built on **IOCP (I/O Completion Port)** and designed for **real-world server and client applications**.

`iocp-netlib` focuses on **ease of use on Windows**, explicit control, and predictable behavior,  
without forcing users to understand IOCP internals.

---

## Overview

This repository provides **two static libraries**:

- **BaseLib**  
  Common infrastructure: threading, timers, logging, queues, buffers, utilities.

- **NetLib**  
  A lightweight **IOCP-based TCP networking layer**, usable as:
  - a **high-performance server**
  - a **TCP client**

The API is intentionally **minimal and intuitive**, hiding IOCP complexity behind a small set of clear interfaces.

---

## Key Characteristics

### ✔ Windows-first Design

- Native **Windows IOCP**
- WinSock2-based
- No cross-platform abstraction overhead
- Optimized for Windows server workloads

### ✔ Extremely Low Entry Barrier

- No need to understand IOCP, OVERLAPPED, or completion ports
- One central class: `NetService`
- One callback interface: `IIOCallback`
- Same API for **server and client**

### ✔ Server and Client in One Library

- Listen and accept connections
- Actively connect to remote servers
- Reuse the same callback and packet flow model

### ✔ Production-Oriented

- Explicit object lifetime
- No hidden threads
- Deterministic shutdown
- Designed for long-running services

---

## Supported Platforms

- **Windows only**
- **x86 and x64**
- Visual Studio:
  - **VS2005**
  - **VS2022**
- Language level:
  - **C++03 or later**

---

## Project Structure

```text
iocp-netlib/
├── BaseLib/
├── NetLib/
├── TestServer/
├── TestClient/
├── include/
├── VS2005/
├── VS2022/
└── README.md
```

---

## Core API

### IIOCallback

```cpp
class IIOCallback
{
public:
    virtual void OnConnected(
        unsigned int socket,
        unsigned int ip,
        unsigned short port) = 0;

    virtual int OnReceived(
        unsigned int socket,
        unsigned int ip,
        unsigned short port,
        void* data,
        int length) = 0;

    virtual void OnClosed(
        unsigned int socket,
        unsigned int ip,
        unsigned short port,
        unsigned int errorCode) = 0;
};
```

---

### NetService

```cpp
class NetService
{
public:
    int StartNetService(
        const char* ip,
        unsigned short listenPort,
        int listenBacklog,
        int maxConnection,
        int maxPacketSize,
        int packetSizeOffset,
        int clientTimeoutSec,
        int logLevel,
        NetCore* netCore,
        IIOCallback* callback);

    int Send(unsigned int socketID, const void* data, int length);
    unsigned int ConnectServer(const char* ip, unsigned short port, int waitMS);
    int CloseSocket(unsigned int socketID);
    void StopNetService();
};
```

---

## Usage

### Use as server

```cpp
// first: create core
NetCore   netCore;
if (netCore.Initialize(0) != 0) {
	return false;
}

std::string ip = "0.0.0.0";
unsigned short listenPort = 6805;
int listenBacklog = 16;
int maxConnection = 1000;
int maxPacketSize = 10240;
int packetSizeOffset = 0;
int clientTimeoutSec = 180;
int nLogLevel = LOG_LEVEL_INFO;
IIOCallback* callback = this;

// second: here you can start many service and each service listen a port,all of them use a same core.
int nRet = mNetService.StartNetService(ip.c_str(), listenPort, listenBacklog,
	maxConnection, maxPacketSize,
	packetSizeOffset, clientTimeoutSec, nLogLevel, &netCore, callback);
if (nRet != 0)
{
	LOG("error", LOG_LEVEL_ERROR, "Net service start failed, error: %d", nRet);
}
else
{
	printf("[listenPort:%d], server is running!\n", mPort);
}
```
---

### Use as client
```cpp
// first: create core
NetCore   netCore;
if (netCore.Initialize(0) != 0) {
	return false;
}

std::string ip = "0.0.0.0";
unsigned short listenPort = 0; // as client, dont't need port, just start service
int listenBacklog = 16;
int maxConnection = 1;
int maxPacketSize = 10240;
int packetSizeOffset = 0;
int clientTimeoutSec = 180;
int nLogLevel = LOG_LEVEL_INFO;
IIOCallback* callback = this;

// second: here you can start many service and each service listen a port,all of them use a same core.
int nRet = mNetService.StartNetService(ip.c_str(), listenPort, listenBacklog,
	maxConnection, maxPacketSize,
	packetSizeOffset, clientTimeoutSec, nLogLevel, &netCore, callback);
if (nRet != 0)
{
	LOG("error", LOG_LEVEL_ERROR, "Net service start failed, error: %d", nRet);
}
else
{
	printf("[listenPort:%d], server is running!\n", mPort);
}

// third: connect server
unsigned int socketID = mNetService.ConnectServer(mRemoteIP.c_str(), mRemotePort, 1500);
if (socketID != 0){
    printf("connect success\n");
}else{
    printf("connect failed\n");
}

```

---

## Build Instructions

1. Open the solution in `VS2005/` or `VS2022/`
2. Select configuration:
   - Debug / Release
   - x86 / x64
3. Build the solution

Generated static libraries:

- `BaseLib.lib`
- `NetLib.lib`

---

## Integration Notes

- Add `include/` to your include paths
- Link against `BaseLib.lib` and `NetLib.lib`
- Ensure consistent runtime (`/MT` or `/MTd`)
- Define `_WT_NO_MAIN` if you want to use your own `main()`

---

## License

MIT License
