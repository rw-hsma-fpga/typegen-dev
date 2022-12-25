#ifndef APPLOG_H
#define APPLOG_H

#include <iostream>
#include <string>

#define APPLOG_INFO 1
#define APPLOG_WARNING 2
#define APPLOG_ERROR 3

class AppLog {
    bool opened;

    public:
        AppLog();
        AppLog(std::string log_name);
        ~AppLog();

};


#endif // APPLOG_H