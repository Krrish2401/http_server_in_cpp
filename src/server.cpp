#include <iostream>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <filesystem>


using namespace std;

#pragma comment(lib, "ws2_32.lib")

string dir;//global directory for file handling


int main(int argc, char **argv) {
  
  for(int i = 1; i < argc; i++){
    if(string(argv[i]) == "--directory" && i+1 < argc){
      dir = argv[i+1];
      break;
    }
  }

  WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed\n";
        return 1;
    }
  cout << unitbuf;// Flush after every cout / cerr
  cerr << unitbuf;
  
  cout << "Logs from your program will appear here!\n";
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
  cerr << "Failed to create server socket\n";
  return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
    cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 100;
  if (listen(server_fd, connection_backlog) != 0) {
    cerr << "listen failed\n";
    return 1;
  }

  vector<thread> threads;
  mutex log_mutex;
  
  while(true){
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    
    cout << "Waiting for a client to connect...\n";
    
    SOCKET client_fd = accept(server_fd, (struct sockaddr *) &client_addr,(socklen_t *) &client_addr_len);
    if(client_fd==INVALID_SOCKET){
      lock_guard<mutex> lock(log_mutex);
      cerr<<"Could not accept the connection! "<<WSAGetLastError()<<endl;
      continue;
    }
    
    lock_guard<mutex> lock(log_mutex);
    cout << "Client connected\n";

    threads.emplace_back(
      [client_fd, &log_mutex](){
        char recvbuf[1024]={0};
        int rec = recv(client_fd,recvbuf,1024,0);
        if (rec<=0)
        {
          cerr<<"Could not recieve request! "<<WSAGetLastError()<<endl;
          closesocket(client_fd);
          return;
        }
        
        
        string req(recvbuf,rec);
        
        string r;
        
        if(req.find("GET / ")==0){
          r = "HTTP/1.1 200 OK\r\n\r\n";
        }
        else if(req.find("GET /echo/")==0){
          size_t i = req.find("/echo/") + 6;
          size_t i2 = req.find(' ', i);
          string echo = req.substr(i, i2 - i);

          r = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length:" + to_string(echo.length()) + "\r\n" + "\r\n" + echo;
        }
        else if(req.find("GET /user-agent")==0){
          size_t i = req.find("User-Agent: ")+12;
          if(i!=string::npos){
            size_t i2 = req.find("\r\n",i);
            string us = req.substr(i,i2-i);

            r = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length:" + to_string(us.length()) + "\r\n" + "\r\n" + us;
          }
        }
        else if(req.find("GET /files/")==0){
          size_t i = req.find("/files/")+7;
          size_t i2 = req.find(' ', i);
          string filename = req.substr(i, i2 - i);

          string path = dir;
          if (dir.back() != '/' && dir.back() != '\\') {
            path += '\\';  // Using Windows path separator
          }
          path += filename;

          ifstream file(path, ios::binary | ios::ate);
          if(file){
            streamsize size = file.tellg();
            file.seekg(0, ios::beg);
            
            vector<char> buffer(size);

            if(file.read(buffer.data(),size)){
              string content(buffer.begin(), buffer.end());
              r = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: " + to_string(size) + "\r\n\r\n" + content;
            }
            else{
              r = "HTTP/1.1 404 Not Found\r\n\r\n";
            }
            file.close();
          }
          else{
            r = "HTTP/1.1 404 Not Found\r\n\r\n";
          }
        }
        else if(req.find("POST /files/")==0){
          size_t i = req.find("/files/")+7;
          size_t i2 = req.find(' ', i);
          string filename = req.substr(i, i2-i);

          string path = dir;
          if(dir.back()!= '/' && dir.back()!='\\'){
            path+='\\';
          }

          path+=filename;
          cout<<"creating file: "<<path<<endl;

          //find content-length
          size_t contentLengthPos = req.find("Content-Length: ");
          size_t contentLengthstart = req.find("\r\n\r\n");
          if(contentLengthPos!=string::npos && contentLengthstart!=string::npos){
            contentLengthstart += 4;
            size_t contentLengthend = req.find("\r\n",contentLengthPos);
            int ctl = stoi(req.substr(contentLengthPos+16, contentLengthend - contentLengthPos - 16));
            
            string body = req.substr(contentLengthstart, contentLengthend);

            ofstream outfile(path, ios::binary);
            if(outfile){
              outfile.write(body.c_str(), body.size());
              outfile.close();

              r = "HTTP/1.1 201 Created\r\n\r\n";
            }
            else{
              cerr<<"failed to create file!";
              r = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            }
          }
          else{
            r = "HTTP/1.1 400 Bad Request\r\n\r\n";
          }

        }
        else{
          r = "HTTP/1.1 404 Not Found\r\n\r\n";
        }

        send(client_fd,r.c_str(),r.length(),0);
        closesocket(client_fd);
      }
    );
  }

  for(auto &t : threads){
    if(t.joinable()){
      t.join();
    };
  }

  closesocket(server_fd);
  WSACleanup();
  
  return 0;
}
