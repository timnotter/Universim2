#include "helpers/date.hpp"
// Add 0s to make all dates the same length
#define DATE_ELONG true

Date::Date(int day, int month, int year){
    // printf("ConstructorVar - Sec: %f, min: %d, hour: %d, day: %d. month: %d, year: %d\n", second, minute, hour, day, month, year);
    second = 0;
    minute = 0;
    hour = 0;
    this->day = day;
    this->month = month;
    this->year = year;
    if(!checkDate()){
        this->day = 0;
        this->month = 0;
        this->year = 0;
    }
    // printf("ConstructorVarEnd - Sec: %f, min: %d, hour: %d, day: %d. month: %d, year: %d\n", second, minute, hour, day, month, year);
}

Date::Date() : Date(0, 0, 0){
    // printf("Constructor - Sec: %f, min: %d, hour: %d, day: %d. month: %d, year: %d\n", second, minute, hour, day, month, year);
    // printf("ConstructorEnd - Sec: %f, min: %d, hour: %d, day: %d. month: %d, year: %d\n", second, minute, hour, day, month, year);
}

const char *Date::toString(bool onlyDate){
    // printf("Sec: %f, min: %d, hour: %d, day: %d. month: %d, year: %d\n", second, minute, hour, day, month, year);
    // Update date string
    std::ostringstream dateStream;
    if(!onlyDate){
        switch(hour>=10){
            case true: dateStream << hour << ":"; break;
            case false: dateStream << "0" << hour << ":"; break;
        }
        // date = dateStream.str();
        switch(minute>=10){
            case true: dateStream /*<< date */<< minute << " "; break;
            case false: dateStream/*<< date*/ << "0" << minute << " "; break;
        }
    }
    if(DATE_ELONG){
        // date = dateStream.str();
        switch(day>=10){
            case true: dateStream/* << date*/ << day << "."; break;
            case false: dateStream/* << date*/ << "0" << day << "."; break;
        }
        // date = dateStream.str();
        switch(month>=10){
            case true: dateStream/* << date*/ << month << " "; break;
            case false: dateStream/* << date*/ << "0" << month << " "; break;
        }
        // date = dateStream.str();
        int yearTemp = year;
        if(year==0) dateStream << "0000";
        else{
            while(yearTemp<1000){
                yearTemp*=10;
                dateStream/* << yearString*/ << "0";
                // yearString = temp.str();
            }
            dateStream << year;
        }
    }
    else{
        dateStream << day << "." << month << " " << year;
    }
    
    date = dateStream.str();

    //Return char* of string
    return date.c_str();
}

const char *Date::timeToString(){
    std::ostringstream timeStream;
    switch(hour>=10){
        case true: timeStream << hour << ":"; break;
        case false: timeStream << "0" << hour << ":"; break;
    }
    switch(minute>=10){
        case true: timeStream /*<< date */<< minute << " "; break;
        case false: timeStream/*<< date*/ << "0" << minute << " "; break;
    }
    time = timeStream.str();

    //Return char* of string
    return time.c_str();
}

void Date::incSecond(long double delta){
    second += delta;
    if(second/60>=1){
        temp = 0;
        while(second/60>=1){
            second -= 60;
            temp++;
        }
        incMinute(temp);
    }
    return;
}

void Date::incMinute(long double delta){
    minute += delta;
    temp = minute/60;
    if(temp>=1){
        minute = minute % 60;
        incHour(temp);
    }
    // printf("Incremented minute - Current date: %s\n", toString());
    return;
}

void Date::incHour(long double delta){
    hour += delta;
    temp = hour/24;
    if(temp>=1){
        hour = hour % 24;
        incDay(temp);
    }
    // printf("Incremented hour - Current date: %s, delta was %f\n", toString(), delta);
    return;
}

void Date::incDay(long double delta){
    day += delta;
    int numDays = dayCount();
    temp = day/(numDays+1);
    if(temp>=1){
        if(day>58){
            printf("Program could behave unexpectedly! Daycount exeeded 2-month period\n");
        }
        day = day % numDays;
        incMonth(temp);
    }
    // printf("Incremented day - Current date: %s\n", toString());
    return;
}

void Date::incMonth(long double delta){
    month += delta;
    temp = month/13;
    if(temp>=1){
        month = month % 12;
        incYear(temp);
    }
    // printf("Incremented month - Current date: %s\n", toString());
    return;
}

void Date::incYear(long double delta){
    year += delta;
    return;
}

int Date::dayCount(){
    switch(month) {
        case 2:
            if(checkLeapYear()) return 29;
            return 28;
        case 4:
        case 6:
        case 9:
        case 11:
            return 30;
        default: 
            return 31;
    }
}

bool Date::checkDate(){
    if(day<0||day>31||month<1||month>12||year<0) {
			return false;
    }
    switch(month) {
        case 2:
            if(checkLeapYear()) {
                if(day>29)
                    return false;
                return true;
            }
            else if(day>28)
                return false;
            return true;
        case 4:
        case 6:
        case 9:
        case 11:
            if(day>30)
                return false;
            return true;
        default: 
            return true;
    }
    return true;
}

bool Date::checkLeapYear(){
    if(year < 0) printf("Program could behave unexpectedly! Year is negative\n");
    if (year % 4 != 0) return false;
    else if (year % 400 == 0) return true;
    else if (year % 100 == 0) return false;
    return true;
}
