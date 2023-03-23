#ifndef DATE_HPP
#define DATE_HPP
#include <string>
#include <sstream>

class Date{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int temp;           //Variable for temporary storage
    double second;
public:
    std::string date;

    Date(int day, int month, int year);
    Date();
    const char *toString(bool onlyDate = false);
    void incSecond(double delta);
    void incMinute(double delta);
    void incHour(double delta);
    void incDay(double delta);
    void incMonth(double delta);
    void incYear(double delta);

    int dayCount();
    bool checkDate();
    bool checkLeapYear();
};


#endif