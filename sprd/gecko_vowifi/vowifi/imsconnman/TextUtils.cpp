#include "TextUtils.h"


BEGIN_VOWIFI_NAMESPACE

void TextUtils::splitString(const string& srcString, const string& delimiter, vector<string>& vect) {
    if (srcString.empty() || delimiter.empty()) {
        return;
    }

    string::size_type beginIndex = 0;
    string::size_type findIndex = srcString.find_first_of(delimiter);

    while (string::npos != findIndex) {
        vect.push_back(srcString.substr(beginIndex, findIndex - beginIndex));
        beginIndex = findIndex + delimiter.size();
        findIndex = srcString.find_first_of(delimiter, beginIndex);
    }

    if (beginIndex != srcString.size() && !srcString.substr(beginIndex).empty()) {
        vect.push_back(srcString.substr(beginIndex));
    }
}

END_VOWIFI_NAMESPACE