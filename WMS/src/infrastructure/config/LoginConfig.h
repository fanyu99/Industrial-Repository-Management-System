#pragma once
#include <QString>
struct LoginConfig {
    int maxUserNameLength { 50 };
    int minUserNameLength { 2 };
    int maxPasswordLength { 128 };
    int minPasswordLength { 8 };
    bool isValid() const
    {
        return minUserNameLength > 0 && maxUserNameLength >= minUserNameLength && minPasswordLength > 0 && maxPasswordLength >= minPasswordLength;
    }
};