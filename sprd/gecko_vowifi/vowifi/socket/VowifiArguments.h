/************************************************************************

 Copyright (c) 2005-2017 by Spreadtrum, Inc.
 All rights reserved.

 This software is confidential and proprietary to Spreadtrum,
 Inc. No part of this software may be reproduced, stored, transmitted,
 disclosed or used in any form or by any means other than as expressly
 provided by the written license agreement between Spreadtrum and its
 licensee.

 ************************************************************************/
#ifndef _VOWIFI_ARGUMENTS_H__
#define _VOWIFI_ARGUMENTS_H__

//#include <typeinfo>
#include <vector>
#include <string>

#include "json/json.h"

using namespace std;

namespace mozilla {
namespace dom {
namespace vowifiadapter {

class VowifiArguments {
    int mIndex_;
    Json::Value mArgums_;

    // the "args" only support maximum 16 arguments
    static const int mMaxInd = 16;
    static const char* mIndName[mMaxInd];

public:
    VowifiArguments()
    :mIndex_(0)
    {}

    ~VowifiArguments(){}

    void GetJsonArgs(Json::Value& arguments)
    {
        //arguments["args"] = mArgums_;
        arguments = mArgums_;
        mIndex_ = 0;
        mArgums_.clear();
    }

    template <class ValueT>
    bool PushJsonArg(ValueT &arg)
    {
        if (mIndex_>= mMaxInd-1) return false;
        mArgums_[mIndName[mIndex_++]] = arg;
        return true;
    }

    template <class ValueT>
    bool PushJsonArrayArg(ValueT &arg, bool isEnd)
    {   /* isEnd, indicates the array last arg to be added */
        Json::Value currArg;

        if (mIndex_>= mMaxInd-1) return false;
        if(!mArgums_.isMember(mIndName[mIndex_]))
        {
            currArg.append(arg);
        }else{
            currArg = mArgums_[mIndName[mIndex_]];
            currArg.append(arg);
        }
        if (isEnd) mArgums_[mIndName[mIndex_++]] = currArg;
        else mArgums_[mIndName[mIndex_]] = currArg;
        return true;
    }

    template <class ValueT>
    bool PushJsonArg(const string &argName, ValueT &arg)
    {
        //if (mIndex_++>= 9) return false;
        mArgums_[argName] = arg;
        return true;
    }

    template <class ValueT>
    bool PushJsonArg(const char* argName, ValueT &arg)
    {
        //if (mIndex_++>= 9) return false;
        mArgums_[argName] = arg;
        return true;
    }
/*
    template <class ValueT1, class ValueT2>
    static bool GetOneArg(Json::Value &arg, ValueT1 &name, ValueT2 &var)
    {
        if(!arg.isMember(name)) return false;

        if (typeid(var) == typeid(int) ){
            var = arg[name].asInt();
        }else if(typeid(var) == typeid(bool)) {
            var = arg[name].asBool();
        }else{
            //TODO print unknown type error log
            return false;
        }
        return true;
    }
*/
    template <class ValueT1>
    static bool GetOneArg(Json::Value &arg, ValueT1 &name, int &var)
    {
        if(!arg.isMember(name)) return false;
        var = arg[name].asInt();
        return true;
    }

    template <class ValueT1>
    static bool GetOneArg(Json::Value &arg, ValueT1 &name, unsigned int &var)
    {
        if(!arg.isMember(name)) return false;
        var = arg[name].asInt();
        return true;
    }

    template <class ValueT1>
    static bool GetOneArg(Json::Value &arg, ValueT1 &name, bool &var)
    {
        if(!arg.isMember(name)) return false;
        var = arg[name].asBool();
        return true;
    }

    template <class ValueT1>
    static bool GetOneArg(Json::Value &arg, ValueT1 &name, double &var)
    {
        if(!arg.isMember(name)) return false;
        var = arg[name].asDouble();
        return true;
    }

    template <class ValueT1>
    static bool GetOneArg(Json::Value &arg, ValueT1 &name, string &var)
    {
        if(!arg.isMember(name)) return false;
        var = arg[name].asString();
        return true;
    }

    template <class ValueT1>
    static bool GetArrayArgs(Json::Value &arg, ValueT1 &name, vector<int> &var)
    {
        if(!arg.isMember(name)) return false;
        Json::Value mArgs = arg[name];

        for (unsigned int i=0; i< mArgs.size(); ++i){
            var.push_back(mArgs[i].asInt());
        }

        return true;
    }

    template <class ValueT1>
    static bool GetArrayArgs(Json::Value &arg, ValueT1 &name, vector<string> &var)
    {
        if(!arg.isMember(name)) return false;
        Json::Value mArgs = arg[name];

        for (unsigned int i=0; i< mArgs.size(); ++i){
            var.push_back(mArgs[i].asString());
        }

        return true;
    }

private:
    VowifiArguments(const VowifiArguments&);
    VowifiArguments& operator=(const VowifiArguments&);

};
} // namespace vowifiadapter
} // namespace dom
} // namespace mozilla
#endif
