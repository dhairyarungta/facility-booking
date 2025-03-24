#include <string>
#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>
#include <set>
#include <utility>
#include <unordered_map>
#include <algorithm>
#include <atomic>
#include <cstring>    
#include <arpa/inet.h>  
#include <unistd.h>

#define PORT 3000
#define NUM_AVAIL 50 
#define FACILITY_NAME_LEN 30 

typedef std::pair<int, int> hourminute; // Time : {Hour, Minute}

typedef std::pair<hourminute, hourminute> bookStruct; 
//bookings are represented via {{startHour, StartMin}, {endHour, endMin}}

enum class InvocationSemantics {
    AT_LEAST_ONCE,
    AT_MOST_ONCE
};

enum class Day : char {
    Monday = 0,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday
};

InvocationSemantics stringToInvocationSemantics (std::string s) {
    if ( s == std::string_view{"AT_LEAST_ONCE"} ) {
        return InvocationSemantics::AT_LEAST_ONCE;
    }
    else if ( s == std::string_view{"AT_MOST_ONCE"} ) {
        return InvocationSemantics::AT_MOST_ONCE;
    }
    else {
        std::cerr << "Wrong or no invocation semantics provided";
        exit(1);
    }
}

auto compareBookStruct = [](const bookStruct& a, const bookStruct& b) {
    return (a.second <= b.first);  
};

class Facility {
    std::string name;
    // std::mutex mtx;
    int capacity;
    std::unordered_map< Day, 
        std::set<bookStruct, decltype(compareBookStruct)> > reservations; 
    //Day and the reservations for that day (for one facility)
    //a reservation is of form (1200)

    int hourToTimestamp(hourminute time) {
        return 60*time.first + time.second;
    }

    hourminute timestampToHour(int time) {
        int hour = time/60;
        int minute = time%60;
        return {hour, minute};
    }

    hourminute updateTime(hourminute a, hourminute b, bool add) {
        return add ? timestampToHour(hourToTimestamp(a)+hourToTimestamp(b)) 
            : timestampToHour(hourToTimestamp(a)-hourToTimestamp(b));
    }

    bool isWellOrdered(bookStruct booking, Day day) {
        if (!reservations[day].size()) {
            return true;
        }
        auto it = std::upper_bound(reservations[day].begin(), reservations[day].end(), bookTime);
        while (it != reservations[day].begin()) {
            if (it->second <= bookTime.first) {
                return true;
            }
            it--;
        }
        if (it->second <= bookTime.first) {
            return true;
        }
        return false;
    }

public:
    Facility(std::string name, int capacity) : name(name), capacity(capacity) 
    { }

    std::unordered_map< Day, std::vector<bookStruct> > bookingMap(std::vector<Day>& days) {
        std::unordered_map< Day, std::vector<bookStruct> > availabilities;
        for (auto day : days) {
            if (!reservations[day].size()) {
                availabilities[day] = {{0, 0}, {23, 59}};
            }
            else {
                auto it = reservations.begin();
                bookStruct prev = *it;
                it++;
                while (it != reservations.end()) {
                    hourminute startAvail = prev.second;
                    hourminute endAvail = it->first;
                    if (startAvail < endAvail) availabilities[day].push_back({startAvail, endAvail});
                }
            }
        }
        return availabilities;
    }
    bool bookFacility (Day day, bookStruct bookTime) {
        // std::lock_guard<std::mutex> lock(mtx);
        if (isWellOrdered(bookTime, day)) {
            reservations[day].insert(bookTime);
            return true;
        }
        return false;
        
    }
    bool updateBooking (Day day, bookStruct booking, bool delay, hourminute offset) {
        bookStruct updatedBooking = {updateTime(booking.first, offset, delay), updateTime(booking.second, offset, delay)};
        //can add additional logic for when offset forces day to spill over to next or previous

        if (isWellOrdered(updatedBooking, day)) {
            reservations[day].erase(booking);
            reservations[day].insert(updatedBooking);
            return true;
        }
        return false;
    }
    int queryCapacity() {
        return capacity; //idempotent service
    }
    bool cancelBooking(bookStruct bookTime, Day day) {
        //non-idempotent service, server side occurs via confirmation ID
        if (!reservations[day].contains(bookTime)) return false;
        reservations[day].erase(bookTime);
        return true;
    }
};

/*
    Request Message
    OP TYPES:
    101 - QUERY
    //facility name length (uint32_t), facility name (char), non '\0' ending
    // days to query for single byte for each, 0 = Monday, 1 = Tuesday  ....

    102 - CREATE
    //facility name length (uint32_t), facility name (char), non '\0' ending
    //single byte for day of booking as a eg 0 for monday
    // 4 bytes for start time, eg: times are represented as {1, 1, 0, 9} for 11:09
    // 4 bytes for end time 

    103 - UPDATE
    104 - MONITOR

    105 - QUERY_CAPACITY
    //facility name length (uint32_t), facility name (char), non '\0' ending

    106 - DELETE/CANCEL 

    107 - GET ALL FACILITY NAMES
    //payload len = 0
*/
struct __attribute__ ((packed)) MarshalledMessage {
    uint32_t uid; //confirmation UID
    uint32_t op; //operation to perform
    uint32_t payloadLen;
    char payload [];
};

struct UnmarshalledRequestMessage {
    uint32_t uid; //confirmation id
    uint32_t op; //operation to perform
    std::vector<Day> days; //at most 7 days of the week
    std::string facilityName ;
    hourminute startTime; //times are represented as {1 1, 59} for 11:59
    hourminute endTime;

    int32_t offset; 
    //signed, in minutes
    //monitoring interval for a callback (max over a week)
    //otherwise update time in case update 
}; 

/*
    ERROR CODES:
    100 - OK
    200 - INVALID Facility Name
    300 - Facility completely or partially unavailable during requested period
    400 - INVALID confirmation ID

*/

// struct __attribute__ ((packed)) ReplyMessage {
//     uint32_t uid; //8 byte confirmation ID given by server
//     uint32_t errorCode; //error code if revelant
//     uint32_t payloadLen;
//     char payload [];
// }

struct UnmarshalledReplyMessage {
    uint32_t uid; //confirmation ID given by server
    uint32_t op; //operation to perform
    uint32_t errorCode; //error code if revelant
    uint32_t capacity; //returns capacity, if relevant
    std::vector< std::string > facilityNames; // for op type '107'
    std::vector< std::pair< Day, std::bitset<1440> > > availabilities;
    // bitset represents the minutes of a day, for a particular facility
    // 1 set bit means busy, 0 set bit means not busy 
};


typedef std::pair<std::pair<std::string, enum Day>, bookStruct> serverBooking; 
//booking for (facility name, day, bookingTime)

class Server {
    std::unordered_map<std::string, Facility> facilities;
    // facility name, facility

    std::unordered_map<uint32_t, serverBooking> bookings; 
    //uid, server booking

    char* bookstructToTime(bookStruct a) {
        char time[8];
        hourminute start = a.first, end = a.second;
        time[0] = start.first / 10 + '0', time[1] = start.first % 10 + '0';
        time[2] = start.second / 10 + '0', time[3] = start.second % 10 + '0';
        time[4] = end.first / 10 + '0', time[5] = end.first % 10 + '0';
        time[6] = end.second / 10 + '0', time[7] = end.second % 10 + '0';
        return time;
    }

    void query_request_handle (UnmarshalledRequestMessage& msg, char* payload, 
        int payloadLen) {
        
        uint32_t facilityNameLen = htonl(*reinterpret_cast< uint32_t* > (payload));
        std::string facilityName (reinterpret_cast< char* > (payload+4), facilityNameLen);
        msg.facilityName = std::move(facilityName);
    
        payload = payload + 4 + facilityNameLen;
        payloadLen = payloadLen - 4 - facilityNameLen; 

        while (payloadLen != 0) {
            char c = *(reinterpret_cast< char* > (payload));
            msg.days.push_back(static_cast<Day>(c));
            payload++;
            payloadLen--;
        }
    }
     
    void query_capacity_handle (UnmarshalledRequestMessage& msg, char* payload, 
        int payloadLen) {
            uint32_t facilityNameLen = htonl(*reinterpret_cast< uint32_t* >(payload));
            std::string facilityName (reinterpret_cast< char* >(payload+4), facilityNameLen);
            msg.facilityName = std::move(facilityName);
    }

    void monitor_handle (UnmarshalledRequestMessage& msg, char* payload, 
        int payloadLen) {
            uint32_t facilityNameLen = htonl(*reinterpret_cast< uint32_t* >(payload));
            std::string facilityName (reinterpret_cast< char* >(payload+4), facilityNameLen);
            msg.facilityName = std::move(facilityName);
            //register callback
            //TODO 
    }

    void create_request_handle (UnmarshalledRequestMessage& msg, char* payload, 
        int payloadLen) {
            uint32_t facilityNameLen = htonl(*reinterpret_cast< uint32_t* >(payload));
            std::string facilityName (reinterpret_cast< char* >(payload+4), facilityNameLen);
            msg.facilityName = std::move(facilityName);

            payload = payload + 4 + facilityNameLen;
            payloadLen = payloadLen - 4 - facilityNameLen; 

            char c = *(reinterpret_cast< char* > (payload));
            msg.days.push_back(static_cast<Day>(c)); 
            payload++;
            payloadLen--; 

            assert(payloadLen == 8); // 4 bytes : start time, 4 bytes : end time
            std::string start_hour (reinterpret_cast< char* >(payload), 2);
            std::string start_minute (reinterpret_cast< char* >(payload+2), 2);
            msg.startTime = {std::stoi(start_hour), std::stoi(start_minute)};
            std::string end_hour (reinterpret_cast< char* >(payload+4), 2); 
            std::string end_minute (reinterpret_cast< char* >(payload+6), 2); 
            msg.endTime = {std::stoi(end_hour), std::stoi(end_minute)};

    }

    char* marshal(ReplyMessage* reply, int bufferSize) {

        uint32_t numAvail = htonl(reply->uid);
        uint32_t errorCode = htonl(reply->errorCode);
        uint32_t payloadLen = htonl(reply->payloadLen);

        char* data = (char*) malloc (bufferSize);
        char* dataBegin = data;

        // assert (sizeof(reply->availabilities) == (NUM_AVAIL*8));
        // memcpy(dataBegin, reply->availabilities, sizeof(reply->availabilities));
        // dataBegin += sizeof(reply->availabilities); 

        // memcpy(dataBegin, numAvail, sizeof(uint32_t));
        // dataBegin += sizeof(uint32_t);

        memcpy(dataBegin, &numAvail, sizeof(uint32_t));
        dataBegin += sizeof(uint32_t);

        memcpy(dataBegin, &errorCode, sizeof(uint32_t));
        dataBegin += sizeof(uint32_t);

        memcpy(dataBegin, &payloadLen, sizeof(uint32_t));
        dataBegin += sizeof(uint32_t);

        memcpy(dataBegin, reply->payload, bufferSize - 3*sizeof(uint32_t)); 
        return data;
    }

    UnmarshalledRequestMessage unmarshal(MarshalledMessage* msg, int msgSize) {

        UnmarshalledRequestMessage localMsg;
        localMsg.uid = htonl(msg->uid);
        localMsg.op = htonl(msg->op);
        uint32_t payloadLen = htonl(msg->payloadLen);
        
        int payloadIndex = 3*sizeof(uint32_t);
        
        switch (localMsg.uid) {
            case 101:
                query_request_handle(localMsg, msg->payload, payloadLen);
                break;
            case 102:
                create_request_handle(localMsg, msg->payload, payloadLen);
                break;
            case 103:
                break;
            case 104:
                monitor_handle(localMsg, msg->payload, payloadLen);
                break;
            case 105:
                query_capacity_handle(localMsg, msg->payload, payloadLen);
                break;
            case 106:
                break;
            case 107:
                // No need to modify the unmarshalled request msg
                break;
            default:
                std::cerr << "wrong op type";
                // handle error
                // exit(1);
        }
        
        memcpy (msg, data, sizeof(RequestMessage));

        // memcpy(msg->uid, data->uid, sizeof(data->uid));
        // memcpy(msg->days, data->days, sizeof(data->days));
        // memcpy(msg->op, data->op, sizeof(data->op));
        // memcpy(msg->facilityName, data->facilityName, sizeof(data->facilityName));
        // memcpy(msg->startTime, data->startTime, sizeof(data->startTime));
        // memcpy(msg->endTime, data->endTime, sizeof(data->endTime));
        // memcpy(msg->offset, data->offset, sizeof(data->offset));
        return localMsg;
    }

public:
    Server(std::unordered_map<std::string, Facility>& facilities) : facilities(facilities) {}
    void serve() {
        int sockfd;
        char buffer[sizeof(RequestMessage)];
        struct sockaddr_in server_addr, client_addr;

        // Create UDP socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("Socket creation failed");
            return EXIT_FAILURE;
        }

        // Configure server address structure
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PORT);

        // Bind the socket to the port
        if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Bind failed");
            close(sockfd);
            return EXIT_FAILURE;
        }

        std::cout << "UDP Server listening on port " << PORT << "...\n";

        while (true) {
            socklen_t len = sizeof(client_addr);
            int n = recvfrom(sockfd, buffer, sizeof(RequestMessage), 0, (struct sockaddr *)&client_addr, &len);
            if (n < 0) {
                perror("Receive failed");
                continue;
            }
            std::cout << "Received: " << buffer << " from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "\n";
            char* res = "ACK";
            sendto(sockfd, res, sizeof(res), 0, (struct sockaddr *)&client_addr, len);  //server sends ACK to client for at least once invocation semantics
            RequestMessage msg = unmarshal(buffer);
            ReplyMessage* reply = (ReplyMessage*) malloc(sizeof(ReplyMessage));

            //plan maybe add a handler class here? handler class 
            switch (msg.op) {
                case 'A' : {
                    std::string facility = std::string(msg.facilityName);
                    if (facilities.find(facility) == facilities.end()) {
                        ReplyMessage* reply = (ReplyMessage*) malloc(sizeof(ReplyMessage));
                        reply->errorCode = 200;
                        char data[sizeof(reply)] = marshal(reply);
                        sendto(sockfd, data, sizeof(reply), 0, (struct sockaddr *)&client_addr, len);
                    }
                    else {
                        Facility f = facilities[facility];
                        std::vector<enum Day> days(msg.days);
                        auto avails = f.bookingMap(days);
                        char* data = (char*) malloc(days.size()*sizeof(ReplyMessage));
                        int dataIndex = 0;
                        for (auto day : avails) {
                            ReplyMessage reply;
                            reply.day = day;
                            reply.errorCode = 100;
                            reply.numAvail = avails[day].size();
                            char** availabilities = (char**) malloc(8*numAvail);
                            int index = 0;
                            for (auto booking : avails[day]) {
                                availabilities[index++] = bookstructToTime(booking);
                            }
                            reply.availabilities = availabilities;
                            char* marshalledData = marshal(&reply);
                            memcpy(data+dataIndex, marshalledData, sizeof(reply));
                            dataIndex += sizeof(reply);
                        }
                        sendto(sockfd, data, days.size()*sizeof(reply), 0, (struct sockaddr *)&client_addr, len);
                    }
                }
                case 'B' : {
                    
                }
            }
            // Echo back the received message
            sendto(sockfd, buffer, n, 0, (struct sockaddr *)&client_addr, len);
        }
    }

}
