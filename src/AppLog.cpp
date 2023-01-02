#include "AppLog.h"
#include <streambuf>

teebuf::teebuf() {}

void teebuf::connect(std::streambuf * sbuf1, std::streambuf * sbuf2)
    {
        sb1 = sbuf1;
        sb2 = sbuf2;
    }

 int teebuf::overflow(int c)
{
    if (c == EOF)
    {
        return !EOF;
    }
    else
    {
        int const r1 = sb1->sputc(c);
        int const r2 = sb2->sputc(c);
        return r1 == EOF || r2 == EOF ? EOF : c;
    }
}

// Sync both teed buffers.
 int teebuf::sync()
{
    int const r1 = sb1->pubsync();
    int const r2 = sb2->pubsync();
    return r1 == 0 && r2 == 0 ? 0 : -1;
}   


teestream::teestream()
  : std::ostream(&tbuf)
{
}

void teestream::connect(std::ostream & o1, std::ostream & o2)
{
    tbuf.connect(o1.rdbuf(), o2.rdbuf());
}



//AppLog::AppLog() : opened(false), stream_id(APPLOG_NONE), log_stream(NULL) {}

AppLog::AppLog(std::string log_name, uint32_t printmask) : opened(false), stream_id(APPLOG_NONE) 
{
    filename = log_name + ".log";
    // open stream
    log_stream.open(filename, std::ios::out);
    if (!log_stream.is_open())
        opened = true;

    if (printmask & LOGMASK_PRINT)
        print.connect(std::cout, log_stream);
    else
        print.connect(nullstream, log_stream);

    if (printmask & LOGMASK_INFO)
        info.connect(std::cout, log_stream);
    else
        info.connect(nullstream, log_stream);

    if (printmask & LOGMASK_WARNING)
        warning.connect(std::cerr, log_stream);
    else
        warning.connect(nullstream, log_stream);

    if (printmask & LOGMASK_ERROR)
        error.connect(std::cerr, log_stream);
    else
        error.connect(nullstream, log_stream);

}

AppLog::~AppLog()
{
    log_stream.close();
}

teestream &AppLog::PRINT()
{
    if (stream_id != APPLOG_PRINT) {
        info.flush();
        warning.flush();
        error.flush();
    }

    return print;
}
teestream &AppLog::INFO()
{
    if (stream_id != APPLOG_INFO) {
        print.flush();
        warning.flush();
        error.flush();
    }

    info << "INFO: ";

    return info;
}

teestream &AppLog::WARNING()
{
    if (stream_id != APPLOG_WARNING) {
        print.flush();
        info.flush();
        error.flush();
    }

    warning << "WARNING: ";

    return warning;
}

teestream &AppLog::ERROR()
{
    if (stream_id != APPLOG_ERROR) {
        print.flush();
        warning.flush();
        info.flush();
    }

    error << "ERROR: ";

    return error; 
}


