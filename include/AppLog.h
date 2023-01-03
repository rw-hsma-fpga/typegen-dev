#ifndef APPLOG_H
#define APPLOG_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <streambuf>
using namespace std;

class teebuf: public std::streambuf
{
public:
    // Construct a streambuf which tees output to both input
    // streambufs.
    teebuf();
    void connect(std::streambuf * sbuf1, std::streambuf * sbuf2);
    void connect(std::streambuf * sbuf1);
private:
    // This tee buffer has no buffer. So every character "overflows"
    // and can be put directly into the teed buffers.
    virtual int overflow(int c);

    // Sync both teed buffers.
    virtual int sync();
private:
    std::streambuf * sb1;
    std::streambuf * sb2;
};



class teestream : public std::ostream
{
public:
    // Construct an ostream which tees output to the supplied
    // ostreams.
    teestream();
    void connect(std::ostream & o1, std::ostream & o2);
    void connect(std::ostream & o1);
private:
    teebuf tbuf;
};



#define APPLOG_NONE 0
#define APPLOG_PRINT 1
#define APPLOG_INFO 2
#define APPLOG_WARNING 3
#define APPLOG_ERROR 4

#define LOGMASK_PRINT 1
#define LOGMASK_INFO 2
#define LOGMASK_WARNING 4
#define LOGMASK_ERROR 8
#define LOGMASK_ALL (LOGMASK_PRINT | LOGMASK_INFO | LOGMASK_WARNING | LOGMASK_ERROR)
#define LOGMASK_NOINFO (LOGMASK_PRINT | LOGMASK_WARNING | LOGMASK_ERROR)

class AppLog {
    bool opened;
    int stream_id;
    std::string filename;
    std::ofstream log_stream;
    teestream print;
    teestream info;
    teestream warning;
    teestream error;


    public:
        //AppLog();
        AppLog(std::string log_name, uint32_t printmask);
        ~AppLog();
        teestream &PRINT();
        teestream &INFO();
        teestream &WARNING();
        teestream &ERROR();

};


#endif // APPLOG_H