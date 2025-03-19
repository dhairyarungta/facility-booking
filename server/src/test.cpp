#include<iostream>

enum Day {
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday,
};

//56 bytes
typedef struct RequestMessage {
    char uid[8];
    enum Day days[7];
    char op;
    char facilityName[4];
    char startTime[4];
    char endTime[4];
    char offset[4];
} RequestMessage;

//32 bytes
typedef struct ReplyMessage {
    char availabilities[8];
    char uid[8];
    unsigned int numAvail;
    unsigned int errorCode;
    unsigned int capacity;
    enum Day day;
} ReplyMessage;

int main(){
    std::cout << sizeof(Day) << std::endl;

    std::cout << sizeof(RequestMessage) << std::endl;

    std::cout << sizeof(ReplyMessage) << std::endl;


}