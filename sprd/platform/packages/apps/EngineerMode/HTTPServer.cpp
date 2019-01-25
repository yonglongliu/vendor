/* HTTPServer.cpp */

/* An HTTP Server written in C++ */

#include<iostream>
#include<fstream>
#include<string>
#include<cstring>
#include<stdexcept>
#include<sstream>
#include<algorithm>
#include<ctime>

#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <cctype>

#include"HTTPServer.h"
#include <cutils/properties.h>
#include "ATProcesser.h"
#include <pthread.h>
//extern int errno;
#define FORK_PROCESS 1
//#define CUSTOM_UA "Mozilla/5.0 (Mobile; rv:48.0) Gecko/48.0 Firefox/48.0 KAIOS/2.0"
#define CUSTOM_UA "gecko"
#define TRUE     1
#define FALSE    0
using namespace std;
int initSocket();
int sockfd = -1;
struct sockaddr_in servAddr;
size_t svrPort;
int newsockfd;
socklen_t cliLen;
struct sockaddr_in cliAddr;

string m_url;
string m_mimeType;
HTTPRequest* m_httpRequest;
HTTPResponse* m_httpResponse;
static const int buf_sz = 32;

HTTPServer::HTTPServer()
{
    svrPort = 1380;
    MYLOG("------    svrPort 1380     ---------");
}

HTTPServer::HTTPServer(int port)
{
    string funcName = "HTTPServer::HTTPServer: ";
    MYLOG("---------- HTTPServer setPort ---------");
    if(setPort(port)){
        MYLOG("------setPort error---------");
        //cerr<<funcName<<"Failed to set port"<<endl;
    }
}

HTTPServer::~HTTPServer()
{
    close(newsockfd);
    close(sockfd);
    sockfd = -1;
}

int HTTPServer::setPort(size_t port)
{
    string funcName = "setPort: ";
    //Validation
    if(port<1024||port>65535) {
        //cerr<<funcName<<"Invalid port value. Cannot bind. Enter a value between 1024 and 65535"<<endl;
        return -1;
    }

    svrPort = port;

    return 0;
}

int initSocket()
{
    string funcName = "initSocket: ";
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;
    int errorCode;
    int z = 0;
    /* SPRD:854149 can not accept cmd from ap @{ */
    if (sockfd >= 0)
    {
        MYLOG("------socket already exist---------");
        return 0;
    }
    /* }@ */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        //cerr<<funcName<<"Failed to create socket"<<endl;
        MYLOG("------socket error---------");
        return -1;
    }
    z = setsockopt(sockfd,
        SOL_SOCKET,
        SO_LINGER,
        &so_linger,
        sizeof so_linger);

    if (z < 0) {
       MYLOG("setsockopt error");
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(svrPort);
    //servAddr.sin_addr.s_addr = INADDR_LOOPBACK;//INADDR_ANY;
    //servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_addr.s_addr = INADDR_ANY;
    //servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//zsw

    /* Bind */
    MYLOG("------bind&listen--------");
    ::bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr));
//errorCode = bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr));
//MYLOG("------bind errorCode = %d", errorCode);
    /* Set to listen on control socket */
    if(listen(sockfd, 30000)){
        //cerr<<funcName<<"Listen on port "<<svrPort<<" failed"<<endl;
        MYLOG("------listen error---------");
        return -1;
    }
    return 0;
}

void handler(int num) {
    int status;
    int pid = waitpid(-1, &status, WNOHANG);
    if (WIFEXITED(status)) {
        printf("The child %d exit with code %d\n", pid, WEXITSTATUS(status));
    }
}

int HTTPServer::run()
{
    MYLOG("------------ run ---------");
    string funcName = "run: ";
    signal(SIGCHLD, handler);
    while(1)
    {
        char buf[PROPERTY_VALUE_MAX] = {0};
        property_get("persist.sys.engmoded.enable", buf, "disable");
        if (!strcmp("enable", buf))
        {
            MYLOG("run ----------- pthread_main persist.sys.engmoded.enable  sockfd %d", sockfd);
            if (sockfd < 0)
            {
                if (initSocket())
                {
                    MYLOG("------------ Failed to initialize socket ---------");//cerr<<funcName<<"Failed to initialize socket"<<endl;
                }
                else
                {
                    MYLOG("------------ Succeed to initialize socket ---------");
                }
            }
            cliLen = sizeof(cliAddr);
            if((newsockfd = accept(sockfd, (struct sockaddr *)&cliAddr, &cliLen)) < 0)
            {
                //cerr<<funcName<<"Accept call failed"<<endl;
                MYLOG("------------ engmoded: error: Accept call failed");
            }
            else
            {
                MYLOG("------------ engmoded: ok: Accept call successful");
            }
#ifdef FORK_PROCESS
            if(fork() == 0){
                MYLOG("------handleRequest----fork-----");
                if(handleRequest()){
                    //cerr<<funcName<<"Failed handling request"<<endl;
                    MYLOG("engmoded: error: Failed handling request");
                    exit(-1);
                }

                close(newsockfd);
                exit(0);
            }else{
                close(newsockfd);
            }
#else
            MYLOG("------handleRequest---------");
            if(handleRequest()){
                //cerr<<funcName<<"Failed handling request"<<endl;
                MYLOG("engmoded: error: Failed handling request");
            }
#endif
            //close(newsockfd);
        }
        else
        {
            //MYLOG("run ----------- pthread_main persist.sys.engmoded.disable  sockfd %d", sockfd);
            if (sockfd >= 0)
            {
                close(sockfd);
                sockfd = -1;
            }
            sleep(1);
        }
    }
    //return 0;
}

int HTTPServer::handleRequest()
{
    string funcName = "handleRequest: ";
    MYLOG("------handleRequest---new HTTPRequest&HTTPResponse------");
    m_httpRequest = new HTTPRequest();
    m_httpResponse = new HTTPResponse();

    if(recvRequest()){
        //cerr<<funcName<<"Receiving request failed"<<endl;
        MYLOG("engmoded: error: Receiving request failed");
        return -1;
    }

    m_httpRequest->printRequest();

    if(parseRequest()){
        //cerr<<funcName<<"Parsing HTTP Request failed"<<endl;
        MYLOG("engmoded: error: Parsing HTTP Request failed");
        return -1;
    }

    if(processRequest()){
        //cerr<<funcName<<"Processing HTTP Request failed"<<endl;
        MYLOG("engmoded: Processing HTTP Request failed");
        return -1;
    }

    if(prepareResponse()){
        //cerr<<funcName<<"Preparing reply failed"<<endl;
        return -1;
    }

    m_httpResponse->printResponse();

    if(sendResponse()){
        //cerr<<funcName<<"Sending reply failed"<<endl;
        LOG("engmoded: error: Sending reply failed");
        return -1;
    }

    delete m_httpRequest;
    m_httpRequest = NULL;
    delete m_httpResponse;
    m_httpResponse = NULL;
    return 0;
}

int HTTPServer::recvRequest()
{
    string funcName = "recvRequest: ";
    int recvLength;
    char* buf = new char[buf_sz];
    memset(buf, '\0', buf_sz);

    if(!(recvLength = recv(newsockfd, buf, buf_sz, 0))){
        //cerr<<funcName<<"Failed to receive request (blocking)"<<endl;
        return -1;
    }
    m_httpRequest->addData(buf, recvLength);
    while(1){
        memset(buf, '\0', buf_sz);
        recvLength = recv(newsockfd, buf, buf_sz, MSG_DONTWAIT);
        if(recvLength < 0){
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                break;
            } else {
                cerr<<funcName<<"Failed receiving request (nonblocking)"<<endl;
                return -1;
            }
        }

        m_httpRequest->addData(buf, recvLength);

        if(recvLength<buf_sz)
        {
            break;
        }
    }

    return 0;
}

int HTTPServer::parseRequest()
{
    string funcName = "parseRequest: ";

    if(m_httpRequest->parseRequest()){
        //cerr<<funcName<<"Failed parsing request"<<endl;
        return -1;
    }

    return 0;
}

int HTTPServer::processRequest()
{
    string funcName = "processRequest: ";
    Method method = m_httpRequest->getMethod();
    string ua = m_httpRequest ->getHTTPHeader("User-Agent").c_str();


    ifstream ifs, errfs;
    ofstream ofs;
    size_t contentLength;
    ostringstream os;

    if(m_httpRequest->getProtocol() == HTTP_UNSUPPORTED){
        m_httpResponse->setStatusCode(505);
        return 0;
    }

  MYLOG("--processRequest---User Agent =%s\n------",ua.c_str());
  transform(ua.begin(),ua.end(),ua.begin(),::tolower);
  string::size_type idx;
  idx = ua.find(CUSTOM_UA);
  if(idx == string::npos)
  {
     MYLOG("--processRequest---User Agent Error,UA =%s\n------",ua.c_str());
     m_httpResponse->setStatusCode(403);
     return 0;
  }

  if (m_httpRequest->getURL().find("/eng")>=0){
     return processEngRequest();
  }else{
     return processEngRequest();
  }
    /*switch(method){
        case GET:
            m_url = SVR_ROOT + m_httpRequest->getURL();
            m_mimeType = getMimeType(m_url);

            ifs.open(m_url.c_str(), ifstream::in);
            if(ifs.is_open()){
                ifs.seekg(0, ifstream::end);
                contentLength = ifs.tellg();
                ifs.seekg(0, ifstream::beg);
                os<<contentLength;

                if(m_httpResponse->copyFromFile(ifs, contentLength)){
                    //cerr<<funcName<<"Failed to copy file to Response Body"<<endl;
                    m_httpResponse->setStatusCode(500);
                    return 0;
                }

                m_httpResponse->setHTTPHeader("Content-Length", os.str());
            }else{
                ifs.close();

                string file404 = SVR_ROOT;
                file404 += "/404.html";
                errfs.open(file404.c_str(), ifstream::in);
                if(errfs.is_open()){
                    errfs.seekg(0, ifstream::end);
                    contentLength = errfs.tellg();
                    errfs.seekg(0, ifstream::beg);
                    os<<contentLength;

                    if(m_httpResponse->copyFromFile(errfs, contentLength)){
                        //cerr<<funcName<<"Failed to copy file to Response Body"<<endl;
                        m_httpResponse->setStatusCode(500);
                        return 0;
                    }

                    m_httpResponse->setHTTPHeader("Content-Length", os.str());
                    m_httpResponse->setStatusCode(404);
                    return 0;
                }else{
                    //cerr<<"Critical error. Shutting down"<<endl;
                    return -1;
                }
            }

            ifs.close();
            m_httpResponse->setStatusCode(200);
            break;
        case PUT:
            m_url = SVR_ROOT + m_httpRequest->getURL();
            m_mimeType = getMimeType(m_url);

            ofs.open(m_url.c_str(), ofstream::out|ofstream::trunc);

            if(ofs.is_open()){
                if(m_httpRequest->copyToFile(ofs))
                    m_httpResponse->setStatusCode(411);
                else
                    m_httpResponse->setStatusCode(201);
            }
            else{
                m_httpResponse->setStatusCode(403);
            }
            ofs.close();

            break;
        default:
            m_httpResponse->setStatusCode(501);
            break;
    }

    return 0;*/
}

int HTTPServer::processEngRequest()
{
    string funcName = "processEngRequest: ";
    Method method = m_httpRequest->getMethod();
    string url = m_httpRequest->getURL();
    //string url ="/engmode/?cmd=shell&shellcommand=wcnd_cli%20eng%20iwnpi%20wlan0%20insmod";
    ifstream ifs, errfs;
    ofstream ofs;
    size_t contentLength;
    ostringstream os;
    string content="{cmd:response,data:2222}";

    if(m_httpRequest->getProtocol() == HTTP_UNSUPPORTED){
        m_httpResponse->setStatusCode(505);
        return 0;
    }
    m_mimeType = getMimeType(url);
    string cmd = HTTPRequest::URL::getParameter(url, "cmd");
    cout << "engrequest url " << url << ", param="<<cmd<< endl;
    LOG("engmoded: info engrequest(url=%s)", url.c_str());
    MYLOG("engmoded: info engrequest(url=%s)", url.c_str());

    content=ATProcesser(url).response();
    LOG("engmoded: info: response:content=%s",content.c_str());
    os<<content.length();
    m_httpResponse->setResponseBody(&content);
    m_httpResponse->setHTTPHeader("Content-Length", os.str());
    m_httpResponse->setHTTPHeader("Access-Control-Allow-Origin", "*");
    m_httpResponse->setHTTPHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    m_httpResponse->setHTTPHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    m_httpResponse->setHTTPHeader("Access-Control-Allow-Headers", "Origin, No-Cache, X-Requested-With, If-Modified-Since, Pragma, Last-Modified, Cache-Control, Expires, Content-Type, X-E4M-With");
    m_httpResponse->setStatusCode(200);
    return 0;
}

int HTTPServer::prepareResponse()
{
    string funcName = "prepareResponse: ";
    time_t curTime;
    time(&curTime);
    string curTimeStr = ctime(&curTime);
    replace(curTimeStr.begin(), curTimeStr.end(), '\n', '\0');

    m_httpResponse->setProtocol(m_httpRequest->getProtocol());
    m_httpResponse->setReasonPhrase();

    m_httpResponse->setHTTPHeader("Date", curTimeStr);
    m_httpResponse->setHTTPHeader("Server", "Awesome HTTP Server");
    m_httpResponse->setHTTPHeader("Accept-Ranges", "bytes");
    m_httpResponse->setHTTPHeader("Content-Type", m_mimeType);
    m_httpResponse->setHTTPHeader("Connection", "close");

    if(m_httpResponse->prepareResponse()){
        //cerr<<funcName<<"Failed to prepare response"<<endl;
        return -1;
    }

    return 0;
}

int HTTPServer::sendResponse()
{
    string funcName = "sendResponse: ";

    size_t responseSize = m_httpResponse->getResponseSize();
    string* responseDataPtr = m_httpResponse->getResponseDataPtr();

    char *buf = new char[responseSize];
    memset(buf, '\0', responseSize);
    memcpy(buf, responseDataPtr->data(), responseSize);

    if((send(newsockfd, buf, responseSize, 0))<0){
        //cerr<<funcName<<"Sending response failed"<<endl;
    }

    delete[] buf;
    return 0;
}

string HTTPServer::getMimeType(string fileName)
{
    size_t extPos = fileName.find_last_of(".");
    string extension;
    string mimeType = "text/plain, charset=us-ascii";

    if(extPos == string::npos){
        extension = "";
    }else{
        extension = fileName.substr(extPos+1);
    }

    /* Compare and return mimetype */
    switch(extension[0]){
        case 'b':
            if(extension == "bmp")
                mimeType = "image/bmp";
            if(extension == "bin")
                mimeType = "application/octet-stream";

            break;
        case 'c':
            if(extension == "csh")
                mimeType = "application/csh";
            if(extension == "css")
                mimeType = "text/css";

            break;
        case 'd':
            if(extension == "doc")
                mimeType = "application/msword";
            if(extension == "dtd")
                mimeType = "application/xml-dtd";
            break;
        case 'e':
            if(extension == "exe")
                mimeType = "application/octet-stream";
            break;
        case 'h':
            if(extension == "html" || extension == "htm")
                mimeType = "text/html";
            break;
        case 'i':
            if(extension == "ico")
                mimeType = "image/x-icon";
            break;
        case 'g':
            if(extension == "gif")
                mimeType = "image/gif";
            break;
        case 'j':
            if(extension == "jpeg" || extension == "jpg")
                mimeType = "image/jpeg";
            break;
        case 'l':
            if(extension == "latex")
                mimeType = "application/x-latex";
            break;
        case 'p':
            if(extension == "png")
                mimeType = "image/png";
            if(extension == "pgm")
                mimeType = "image/x-portable-graymap";
            break;
        case 'r':
            if(extension == "rtf")
                mimeType  = "text/rtf";
            break;
        case 's':
            if(extension == "svg")
                mimeType = "image/svg+xml";
            if(extension == "sh")
                mimeType = "application/x-sh";
            break;
        case 't':
            if(extension == "tar")
                mimeType = "application/x-tar";
            if(extension == "tex")
                mimeType = "application/x-tex";
            if(extension == "tif" || extension == "tiff")
                mimeType = "image/tiff";
            if(extension == "txt")
                mimeType = "text/plain";
            break;
        case 'x':
            if(extension == "xml")
                mimeType = "application/xml";
            break;
        default:
            mimeType = "application/json";
            break;
    }

    return mimeType;
}

void HTTPLog(char *format, ...)
{

}
