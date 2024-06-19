#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unordered_map>
#include <thread>
#include <fstream>



std::string clrf = "\r\n";
void appendHeaders(std::string& responsestring, std::string key, std::string value) {
  responsestring.append(key);
  responsestring.append(value);
  responsestring.append(clrf);
}

std::unordered_map<std::string, std::string> getAllHeaders(std:: string httpHeaders) {
  std::unordered_map<std::string, std::string> headerMaps;
  std::string del =  clrf;
  int start, end = -1*del.size();
    do {
        start = end + del.size();
        end = httpHeaders.find(del, start);

        if(start >= end) {
          break;
        }

        std:: string httpHeaderKeyValue = httpHeaders.substr(start, end - start);
        std:: string headerKey = httpHeaderKeyValue.substr(0, httpHeaderKeyValue.find(":"));
        std:: string headerValue= httpHeaderKeyValue.substr(httpHeaderKeyValue.find(":") + 2);
        // std:: cout << headerKey <<"\n";
        // std:: cout << headerValue<< "\n";
        headerMaps.insert({headerKey, headerValue});
    } while (end != -1);

  return headerMaps;
}


void processClient(int client_fd, std:: string directoryName) {
  char buffer[1000000] = { 0 }; 
  recv(client_fd, buffer, sizeof(buffer), 0); 
  std::string temp(buffer);
  std::cout << "Message from client: " << temp <<"\n";  

  std::string okMessage = "HTTP/1.1 200 OK\r\n\r\n";
  std::string notFoundMessage = "HTTP/1.1 404 Not Found\r\n\r\n";
  std::string plainStringResponseMessage = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n";
  std::string multimediaStringResponseMessage = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n";   

  // Process the request
  std::string httpRequestLine = temp.substr(0, temp.find("\r\n", 0));
  std::string httpRequestMethod = httpRequestLine.substr(0, httpRequestLine.find(" ", 0));
  std::string httpRequestURL = httpRequestLine.substr(httpRequestMethod.size() + 1, httpRequestLine.find("HTTP", httpRequestMethod.size()) - 5);
  std::string httpHeaders = temp.substr(httpRequestLine.size() + 1, temp.find_last_of(clrf) - clrf.size());  
  
  std:: cout<< httpRequestLine <<"\n";
  std:: cout<< httpRequestMethod <<"\n";
  std:: cout<< httpRequestURL <<"\n";
  std:: cout<< httpHeaders<<"\n";

  
  std::unordered_map<std::string, std::string> headersMap = getAllHeaders(httpHeaders);

  if(httpRequestURL.rfind("/files/", 0) == 0) {
    std::string fileName = httpRequestURL.substr(7);
    std::cout<<"File Name retieved: " << fileName <<"\n";

    if(directoryName[directoryName.size() - 1] != '/' ) {
      directoryName.append("/");
    }

    std::string finalPath = directoryName + fileName;
    std:: cout << "Final File Path: " << finalPath << "\n";


      std::ifstream ifs(finalPath);

      if(ifs.is_open()) {
        std::string contents((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        
        appendHeaders(multimediaStringResponseMessage, "Content-Length: ", std::to_string(contents.size()));
        multimediaStringResponseMessage.append(clrf);

        multimediaStringResponseMessage.append(contents);
        
        std:: cout << "Sending Reply: " << multimediaStringResponseMessage <<"\n";
        send(client_fd, multimediaStringResponseMessage.c_str(), multimediaStringResponseMessage.length(), 0);
      }
      else{
        std:: cout << "Sending Reply: " << notFoundMessage <<"\n";
        send(client_fd, notFoundMessage.c_str(), notFoundMessage.length(), 0);
      }
  }
  else if(httpRequestURL.rfind("/user-agent",0) == 0) {
    appendHeaders(plainStringResponseMessage, "Content-Length: ", std::to_string(headersMap["User-Agent"].size()));
    
    plainStringResponseMessage.append(clrf);
    plainStringResponseMessage.append(headersMap["User-Agent"]);
    
    std:: cout << "Sending Reply: " << plainStringResponseMessage <<"\n";
    send(client_fd, plainStringResponseMessage.c_str(), plainStringResponseMessage.length(), 0);
  }
  else if(httpRequestURL.rfind("/echo/", 0) == 0) {
    std::string stringFromEchoURL = httpRequestURL.substr(6);
    std::cout<<"String retieved: " << stringFromEchoURL <<"\n";
    appendHeaders(plainStringResponseMessage, "Content-Length: ", std::to_string(stringFromEchoURL.size()));

    plainStringResponseMessage.append(clrf);
    plainStringResponseMessage.append(stringFromEchoURL);
    
    std:: cout << "Sending Reply: " << plainStringResponseMessage <<"\n";
    send(client_fd, plainStringResponseMessage.c_str(), plainStringResponseMessage.length(), 0);
  }
  else if(httpRequestURL.size() == 1 && httpRequestURL.compare("/") == 0) {
    std:: cout << "Sending Reply: " << okMessage <<"\n";
    send(client_fd, okMessage.c_str(), okMessage.length(), 0);
  }
  else {
    std:: cout << "Sending Reply: " << notFoundMessage <<"\n";
    send(client_fd, notFoundMessage.c_str(), notFoundMessage.length(), 0);
  }

  close(client_fd);
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string directoryName("NotFound");

  for(int i= 0; i < argc; i++) {
    std::cout <<argv[i] <<"\n";

    if(std::strcmp(argv[i], "--directory") == 0) {
        directoryName.assign(argv[i+1]);
    }
  }

  std::cout << "Directory Name is: " << directoryName <<"\n";

  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  
  
  while (true){
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    std::cout << "Waiting for a client to connect...\n";
  
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n";

    std::thread t1(processClient, client_fd, directoryName);
    t1.detach();
  }
  

  close(server_fd);

  return 0;
}
