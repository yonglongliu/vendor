/* HTTPServer.h */

#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include<arpa/inet.h>
#include<sys/socket.h>

#include"HTTPRequest.h"
#include"HTTPResponse.h"

#include <cutils/log.h>
#include <android/log.h>
#define MYLOG(args...)  __android_log_print(ANDROID_LOG_DEBUG,  "HAOYOUGEN", ## args)


#define SVR_ROOT "www"

using namespace std;

class HTTPServer{
	public:
		HTTPServer();
		HTTPServer(int );
		~HTTPServer();

		int run(void );
		int setPort(size_t );
		//int initSocket(void );

		int handleRequest(void );
		int recvRequest(void );
		int parseRequest(void );
		int prepareResponse(void );
		int sendResponse(void );
		int processRequest(void );
        int processEngRequest(void);
		//static void *pthread_main(void*);
	private:
		string getMimeType(string );


};

#endif	/* _HTTP_SERVER_H_ */
