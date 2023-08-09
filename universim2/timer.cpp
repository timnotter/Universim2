#include "timer.hpp"

#ifdef _WIN32
int getTime(timespec *time, int arg){
    return std::timespec_get(time, arg);
}
#endif

#ifdef __unix
int getTime(timespec *time, int arg){
    return clock_gettime(arg, time);
}
#endif