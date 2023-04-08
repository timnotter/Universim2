#ifndef DATE_HPP
#define DATE_HPP
#include <string>
#include <sstream>

class Date{
private:
    long year;
    int month;
    int day;
    int hour;
    int minute;
    int temp;           //Variable for temporary storage
    long double second;
    std::string date;
    std::string time;
public:
    Date(int day, int month, int year);
    Date();
    const char *toString(bool onlyDate = false);
    const char *timeToString();
    void incSecond(long double delta);
    void incMinute(long double delta);
    void incHour(long double delta);
    void incDay(long double delta);
    void incMonth(long double delta);
    void incYear(long double delta);

    int dayCount();
    bool checkDate();
    bool checkLeapYear();
};


#endif