/************************************************************************

 Copyright (c) 2005-2017 by Spreadtrum, Inc.
 All rights reserved.

 This software is confidential and proprietary to Spreadtrum,
 Inc. No part of this software may be reproduced, stored, transmitted,
 disclosed or used in any form or by any means other than as expressly
 provided by the written license agreement between Spreadtrum and its
 licensee.

 ************************************************************************/
#ifndef _VOWIFI_CONN_SERVER_H__
#define _VOWIFI_CONN_SERVER_H__

#include "VowifiLocalConn.h"

#include <pthread.h>

using namespace std;

namespace mozilla {
namespace dom {
namespace vowifiadapter {

class VowifiConnServer: public VowifiLocalConn {

public:
    VowifiConnServer();
    ~VowifiConnServer();

    bool InitConnect();
    void ReConnect();
    void CloseConn();
    void ReadLoop();
    bool Send(const char *pBuff, size_t len);
    bool Send(string& msg);

private:
    VowifiConnServer(const VowifiConnServer&);
    VowifiConnServer& operator=(const VowifiConnServer&);

    void AcceptConnect();

    pthread_mutex_t mMuxConn_;
};

} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
#endif
