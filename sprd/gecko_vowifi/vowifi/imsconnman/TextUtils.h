#ifndef __SPRD_VOWIFI_IMS_CONNMAN_TEXT_UTILS_H__
#define __SPRD_VOWIFI_IMS_CONNMAN_TEXT_UTILS_H__

#include <string>
#include <vector>

using std::string;
using std::vector;


BEGIN_VOWIFI_NAMESPACE

class TextUtils {
public:
    static void splitString(const string& srcString, const string& delimiter, vector<string>& vect);
};

END_VOWIFI_NAMESPACE

#endif
