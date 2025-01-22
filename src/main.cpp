#include "version.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <optional>

#include <SDL.h>
#include <SDL_net.h>

#define MAX_REQUEST_SIZE 8192

uint16_t port = 8080;
bool isRunning = true;

std::map<std::string, std::string> contentTypes = {
    {"txt", "text/plain"},
    {"html", "text/html"},
    {"htm", "text/html"},
    {"css", "text/css"},
    {"js", "text/javascript"},
    {"json", "application/json"},
    {"xml", "application/xml"},
    {"pdf", "application/pdf"},
    {"bmp", "image/bmp"},
    {"gif", "image/gif"},
    {"jpeg", "image/jpeg"},
    {"jpg", "image/jpeg"},
    {"png", "image/png"},
    {"svg", "image/svg+xml"},
    {"tif", "image/tiff"},
    {"tiff", "image/tiff"},
    {"ico", "image/vnd.microsoft.icon"}
};

TCPsocket server = nullptr, client = nullptr;

void splitString(std::string str, char delim, std::vector<std::string>& args) {
    std::stringstream parse(str);
    std::string temp;
    while(std::getline(parse, temp, delim)) {
        args.push_back(temp);
    }
}

// They both need to return a string representation.

// Load Text
std::optional<std::string> getTextContent(std::string path) {
    std::ifstream in(path);
    std::optional<std::string> content;

    if(!in.is_open()) {
        return content;
    }

    std::string temp;

    in.seekg(0, std::ios::end);
    temp.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read((char*)temp.data(), temp.size());
    in.close();

    content = temp;

    return content;
}

// Load Binary
std::optional<std::string> getBinaryContent(std::string path) {
    std::ifstream in(path, std::ios::binary);
    std::optional<std::string> content;

    if(!in.is_open()) {
        return content;
    }

    std::string temp;

    in.seekg(0, std::ios::end);
    temp.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read((char*)temp.data(), temp.size());
    in.close();

    content = temp;
    
    return content;
}

std::string getContentType(std::string path) {
    std::vector<std::string> args;
    splitString(path, '.', args);

    std::string contentType = "";

    if(contentTypes.find(args.at(args.size() - 1)) != contentTypes.end()) {
        contentType = contentTypes.at(args.at(args.size() - 1));
    }

    return contentType;
}

std::optional<std::string> getResource(std::string path) {
    std::stringstream filePath;
    filePath << "htdocs" << path;

    path = filePath.str();

    std::optional<std::string> ret;
    std::string contentType = getContentType(path);

    if(contentType == "text/plain") {
        ret = getTextContent(path);
    } else if(contentType == "text/html") {
        ret = getTextContent(path);
    } else if(contentType == "text/css") {
        ret = getTextContent(path);
    } else if(contentType == "text/javascript") {
        ret = getTextContent(path);
    } else if(contentType == "application/json") {
        ret = getTextContent(path);
    } else if(contentType == "application/xml") {
        ret = getTextContent(path);
    } else if(contentType == "application/pdf") {
        ret = getBinaryContent(path);
    } else if(contentType == "image/bmp") {
        ret = getBinaryContent(path);
    } else if(contentType == "image/gif") {
        ret = getBinaryContent(path);
    } else if(contentType == "image/jpeg") {
        ret = getBinaryContent(path);
    } else if(contentType == "image/png") {
        ret = getBinaryContent(path);
    } else if(contentType == "image/svg+xml") {
        ret = getTextContent(path);
    } else if(contentType == "image/tiff") {
        ret = getBinaryContent(path);
    } else if(contentType == "image/vnd.microsoft.icon") {
        ret = getBinaryContent(path);
    }

    return ret;
}

void getHttpRequestHeader(
    std::string request,
    std::string& method, 
    std::string& path, 
    std::string& http, 
    std::map<std::string, std::string>& header) 
{
    std::vector<std::string> lines;

    splitString(request, '\n', lines);

    std::vector<std::string> mph;

    splitString(lines[0], ' ', mph);

    method = mph[0];
    path = mph[1];
    http = mph[2];

    // GET / HTTP/1.1
    for(int i = 1; i < lines.size(); i++) {
        std::vector<std::string> args;

        splitString(lines[i], ':', args);

        if(args.size() == 2) {
            header[args[0]] = args[1];
        }
    }
}


void sendHttpResponse(std::string content, std::string contentType) {
        std::stringstream ss;

        ss << "HTTP/1.1 200 OK\n";
        ss << "Server: Basic HTML Server\n";
        ss << "Cache-Control: no-store\n";
        ss << "Content-Type: " << contentType << "\n";
        ss << "\n";
        ss << content << "\n";
        ss << " ";

        std::string response = ss.str();

        //std::cout << response << "\n";

        SDLNet_TCP_Send(client, (void*)response.data(), response.length());
}

void send404Response() {
    std::stringstream ss;
    ss << "HTTP/1.1 404 Not Found\n";
    ss << "Server: Basic HTML Server\n";
    ss << "Content-Type: text/html; charset=UTF-8\n";
    ss << "\n";

    ss << getTextContent("error/404.html").value() << "\n";

    ss << " ";

    std::string response = ss.str();
    //std::cout << response << "\n";

    SDLNet_TCP_Send(client, (void*)response.data(), response.length());
}

int main(int argc, char** argv) {
    IPaddress ip;


    std::cout << VERSION_FULL_NAME << "\n";

    SDL_Init(0);
    SDLNet_Init();

    SDLNet_ResolveHost(&ip, nullptr, port);

    server = SDLNet_TCP_Open(&ip);

    if(!server) {
        std::cout << "SDLNet_TCP_Open: " << SDLNet_GetError() << "\n";
        exit(-2);
    }

    std::cout << "Listening on port: " << port << "\n";


    while(isRunning) {

        client = SDLNet_TCP_Accept(server);

        if(!client) {
            SDL_Delay(100);
            continue;
        }

        IPaddress* remoteip;
        remoteip = SDLNet_TCP_GetPeerAddress(client);


        if(!remoteip) {
            continue;
        }

        uint32_t ipaddr = SDL_Swap32(remoteip->host);

        std::string request;
        request.resize(MAX_REQUEST_SIZE);

        int len = SDLNet_TCP_Recv(client, (void*)request.data(), request.length());

        if(!len) {
            continue;
        }

        //std::cout << request << "\n";

        std::string method, path, http;
        std::map<std::string, std::string> header;

        getHttpRequestHeader(request, method, path, http, header);

        if(path == "/") {
            path = "/index.html";
        }

        std::string contentType = getContentType(path);

        std::optional<std::string> content = getResource(path);

        if(content.has_value()) {
            sendHttpResponse(content.value(), contentType);
        } else {
            send404Response();
        }

        SDLNet_TCP_Close(client);

        client = nullptr;
    }

    if(!client) {
        SDLNet_TCP_Close(client);
    }

    SDLNet_TCP_Close(server);

    SDLNet_Quit();
    SDL_Quit();

    return 0;
}