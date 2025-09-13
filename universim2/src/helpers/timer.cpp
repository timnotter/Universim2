#include <ctime>
#include "timer.hpp"

#ifdef _WIN32
int getTime(timespec *time, int arg){
    // We just ignore the argument. Most of the type I put in 0, and with 0 the function does not work somehow
    return std::timespec_get(time, TIME_UTC);
}
#endif

#ifdef __unix
int getTime(timespec *time, int arg){
    return clock_gettime(arg, time);
}
#endif
