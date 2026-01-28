# iocp-netlib

A lightweight, high-performance C++ network library based on Windows IOCP  
designed for Visual Studio 2005/2022 static library projects.

This repository contains two static libraries:

- **BaseLib** – common infrastructure and utilities
- **NetLib** – IOCP-based TCP networking library built on top of BaseLib

The project is intended for **Windows-only**, performance-sensitive server
applications.

---

## Features

- Windows IOCP (I/O Completion Port)
- High-performance asynchronous TCP server
- Static library build (`.lib`)
- Designed for Visual Studio 2005/2022
- Explicit object lifetime management
- No external runtime dependencies

---

## Project Structure

```text
iocp-netlib/
├── Baselib/        # Base utilities and infrastructure
├── Netlib/         # IOCP network implementation
├── TestClient/     # Test Client
├── TestServer/     # Test Server
├── include/        # Public headers
├── VS2005/         # Visual Studio 2005 solution and projects
├── VS2022/         # Visual Studio 2022 solution and projects
├── .gitignore
└── README.md
```
---

## Requirements

- Windows 10 / Windows Server
- Visual Studio 2005/2022
- C++03 or later
- WinSock2

---

## Build Instructions

1. Open the solution file in the `VS2005/VS2022` directory:
2. Select configuration:
- `Debug` or `Release`
- `x86`

3. Build the solution.

The output will be static libraries:

- `BaseLib.lib`
- `NetLib.lib`

---

## Usage

1. Add `include/` to your project's include directories.
2. Link against:
- `BaseLib.lib`
- `NetLib.lib`
3. Ensure your project uses the same runtime library settings (`/MT` or `/MTd`).
4. When you use the lib in server, you don't need main function, because the Baselib contains main function. Or 
   you can define _WT_NO_MAIN to use your own main funciton.
5. Run the TestServer in cmd as "TestServer.exe -d".
6. Use "TestServer.exe -i" to install as service and "TestrServer.exe -u" to remove service.

NetLib depends on BaseLib and must be linked after it.

---

## Design Notes

- The library uses explicit ownership rules for IOCP-related objects
(`OVERLAPPED`, connection contexts, buffers).
- No hidden background threads are created implicitly.
- The API is designed to make object lifetime and threading behavior explicit.

This library prioritizes **clarity, correctness, and performance** over
maximum abstraction.

---

## Limitations

- Windows only
- TCP only (currently)
- No SSL/TLS support
- No cross-platform abstraction layer

---

## License

MIT License
