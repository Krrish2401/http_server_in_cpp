# HTTP Server for Windows

This is a simple HTTP server implemented in C++.

## Features

- Handles GET requests for the root path `/`
- Echoes back messages sent to `/echo/`
- Returns the `User-Agent` header from requests to `/user-agent`
- Serves files from a specified directory via `/files/`
- Allows file uploads via POST requests to `/files/`

## Prerequisites

- C++ compiler with C++17 support
- Cmake (version 3.10 or higher)
- Windows OS (uses WinSock2)

## Usage

### Building the Server

To build the server, you can use CMake. Run the following commands in the root directory of the project:

```sh
mkdir build
cd build
cmake ..
make
```

### Running the server
After building, run the server with:
```sh
./server --directory <path>
```
Replace `<path>` with the directory you want to serve files from/to

### API Endpoints


| **Endpoint**        | **Method** | **Description**                 |
|---------------------|------------|---------------------------------|
| `/`                 | GET        | Returns 200 OK                  |
| `/echo/<message>`   | GET        | Echoes back the message         |
| `/user-agent`       | GET        | Returns the client's User-Agent |
| `/files/<filename>` | GET        | Serves the requested file       |
| `/files/<filename>` | POST       | Uploads a file                  |


### Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a new Pull Request
