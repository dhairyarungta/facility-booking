#include<iostream>
#include<stdio.h>
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
struct RequestMessage {
    char uid[8]; //8 bytes
    enum Day days[7]; //28 bytes
    //3 byte padding
    char op; //1 byte
    char facilityName[4]; //4 bytes
    char startTime[4]; // 4 bytes
    char endTime[4]; // 4 bytes
    char offset[4]; //4 bytes
};

//32 bytes
struct ReplyMessage {
    char availabilities[8]; //8 bytes
    char uid[8]; //8bytes
    unsigned int numAvail; //4 bytes
    unsigned int errorCode;  //4 bytes
    unsigned int capacity; //4 bytes
    enum Day day; //4bytes
};

int main(){
    std::cout << sizeof(Day) << std::endl;

    std::cout << sizeof(RequestMessage) << std::endl;
    RequestMessage req = {
        .uid = "USER123", 
        .days = {Monday,Tuesday},
        .op = 'B', 
        .facilityName = {'G','Y','M'},
        .startTime = {'0','9','0','0'},
        .endTime = {'0','9','0','0'},
    };
    
    __builtin_dump_struct(&req, &printf);
    std::cout << sizeof(ReplyMessage) << std::endl;


}