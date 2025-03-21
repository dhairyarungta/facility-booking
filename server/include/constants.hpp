#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <utility>
#include <functional>
#include <vector>

typedef std::pair<int, int> hourminute;
typedef std::pair<hourminute, hourminute> bookStruct; //bookings are represented via {{startHour, StartMin}, {endHour, endMin}}


enum Day {
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday,
};
/*
    OP TYPES:
    A - QUERY
    B - CREATE
    C - UPDATE
    D - MONITOR
    E - QUERY_CAP
    F - DELETE
*/
struct RequestMessage {
    char uid[8]; //8 byte confirmation UID
    enum Day days[7]; //at most 7 days of the week
    char op; //operation to perform
    char facilityName[4]; //4 bytes for character array - alignment constraint
    char startTime[4]; //times are represented as {1, 1, 5, 9} for 11:59
    char endTime[4];
    char offset[4]; //will act as monitoring interval in event of callback
};
/*
    ERROR CODES:
    100 - OK
    200 - INVALID Facility Name
    300 - Facility completely or partially unavailable during requested period
    400 - INVALID confirmation ID

*/
struct ReplyMessage {
    char availabilities[8]; //each availability as (start time, 4 chars)(endTime 4 chars)
    char uid[8]; //8 byte confirmation ID given by server
    unsigned int numAvail; //number of availabilities
    unsigned int errorCode; //error code if revelant
    unsigned int capacity; //returns capacity, if relevant
    enum Day day; //particular day of interest
};


using Handler = std::function<std::vector<ReplyMessage>(RequestMessage)>;
using Middleware = std::function<Handler(Handler)>;


#endif // !CONSTANTS_H
