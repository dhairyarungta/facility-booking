#include <string>
#include <list>
#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>
#include <set>
#include <utility>
#include <unordered_map>
#include <algorithm>
#include <cstring>    
#include <arpa/inet.h>  
#include <unistd.h>
#include <string_view>
#include <cstdlib>
#include <chrono>
#include <fmt/core.h>

#define TESTMODE 1
#define PORT 3000
#define NUM_AVAIL 50 
#define FACILITY_NAME_LEN 30
#define BUFFER_LEN 10000 
//need to define length of buffer as MarshalledMessage size is indeterminate 

typedef std::pair<int, int> hourminute; // Time : {Hour, Minute}

typedef std::pair<hourminute, hourminute> bookStruct; 
//bookings are represented via {{startHour, StartMin}, {endHour, endMin}}

using sys_time = std::chrono::time_point<std::chrono::high_resolution_clock> ;

enum class InvocationSemantics {
    AT_LEAST_ONCE,
    AT_MOST_ONCE
};

enum class Day : char {
    Monday = '0',
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday
};

InvocationSemantics stringToInvocationSemantics (std::string s) {
    if ( s == std::string_view("AT_LEAST_ONCE") ) {
        return InvocationSemantics::AT_LEAST_ONCE;
    }
    else if ( s == std::string_view("AT_MOST_ONCE")) {
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
    Day decDay(Day day) {
        switch (day) {
            case Day::Tuesday : {
                return Day::Monday;
            }
            case Day::Wednesday : {
                return Day::Tuesday;
            }
            case Day::Thursday : {
                return Day::Wednesday;
            }
            case Day::Friday : {
                return Day::Thursday;
            }
            case Day::Saturday : {
                return Day::Friday;
            }
            case Day::Sunday : {
                return Day::Saturday;
            }
            default : {
                return Day::Monday;
            }
        }
    }
    Day incDay(Day day) {
        switch (day) {
            case Day::Monday : {
                return Day::Tuesday;
            }
            case Day::Tuesday : {
                return Day::Wednesday;
            }
            case Day::Wednesday : {
                return Day::Thursday;
            }
            case Day::Thursday : {
                return Day::Friday;
            }
            case Day::Friday : {
                return Day::Saturday;
            }
            case Day::Saturday : {
                return Day::Sunday;
            }
            default : {
                return Day::Monday;
            }
        }
    }
    bool isWellOrdered(bookStruct booking, Day day) {
        if (!reservations[day].size()) {
            return true;
        }

        auto it = std::upper_bound(reservations[day].begin(), reservations[day].end(), booking);

        while (it != reservations[day].begin()) {
            if (it->second <= booking.first) {
                return true;
            }
            it--;
        }
        if (it->second <= booking.first) {
            return true;
        }
        return false;
    }

public:
    Facility(std::string name, int capacity) : name(name), capacity(capacity) 
    { }

    std::vector<std::pair<Day, std::vector<hourminute>>> 
        queryAvail(std::vector<Day>& days) {

        std::vector<std::pair<Day, std::vector<hourminute>>> availabilities;
        for (auto day : days) {
            if (!reservations[day].size()) {
                availabilities.push_back({day, {{0, 1439}}});
            }
            else {
                std::vector<hourminute> avails;
                auto it = reservations[day].begin();
                bookStruct prev = *it;
                if (prev.first > std::make_pair(0, 0)) {
                    avails.push_back({0, hourToTimestamp(prev.first)});
                }
                it++;
                while (it != reservations[day].end()) {
                    if (prev.second < it->first) {
                        avails.push_back(
                        {hourToTimestamp(prev.second), hourToTimestamp(it->first)});
                    }
                    prev = *it;
                    it++;
                }
                if (prev.second < std::make_pair(23, 59)) 
                    avails.push_back({hourToTimestamp(prev.second), 1439});

                availabilities.push_back({day, avails});
            }
        }
        return availabilities;
    }
    bool bookFacility(Day day, bookStruct bookTime) {
        if (isWellOrdered(bookTime, day)) {
            reservations[day].insert(bookTime);
            return true;
        }
        return false;
        
    }
    bool updateBooking(Day day, bookStruct booking, int32_t offset) {
        int startTime = hourToTimestamp(booking.first)+offset;
        int endTime = hourToTimestamp(booking.second)+offset;
        bookStruct updatedBooking = {timestampToHour(startTime), timestampToHour(endTime)};
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

    Note: all fields like uint32_t or int32_t in the request messages are  
    expected in network byte order

    OP TYPES:
    101 - QUERY
    Facility name length (uint32_t) 
    Facility name (char), non '\0' ending
    Days to query for single byte for each, 0 = Monday, 1 = Tuesday  ....
    =========================================

    102 - CREATE
    Facility name length (uint32_t)
    Facility name (char), non '\0' ending
    Single byte for day of booking as a eg 0 for monday
    4 bytes for start time, eg: times are represented as {1, 1, 0, 9} for 11:09
    4 bytes for end time 
    =========================================

    103 - UPDATE
    Dependent on uid, sent previously on create booking (102) reply
    Payload: 
    int32_t : offset in minutes (signed)
    =========================================

    104 - MONITOR
    Facility name length (uint32_t)
    Facility name (char), non '\0' ending
    int32_t : offset in minutes (monitor interval, signed but always > 0)
    =========================================

    105 - QUERY_CAPACITY
    Facility name length (uint32_t)
    Facility name (char), non '\0' ending
    =========================================

    106 - DELETE/CANCEL 
    Dependent on uid, sent previously on create booking (102) reply
    Payload: empty
    =========================================

    107 - GET ALL FACILITY NAMES
    Payload len = 0
*/
/*
    Reply Message
    ERROR CODES:
    100 - OK
    200 - INVALID Facility Name
    300 - Facility completely or partially unavailable during requested period
    400 - INVALID confirmation ID

    OP TYPES:

    101 - QUERY
    numDays - 4 bytes, number of days in msg
    INFO PER DAY
    Day - 1 byte char (0-MON, 1-TUE...)
    numAvail - 4 byte (number of availabilities pairs)
    EACH availability given as :
    startMinutes - 4 byte, starttime in minutes
    endMinutes - 4 byte, endtime in minutes
    ==================================================

    102 - CREATE
    There is no payload, only UID is returned
    =========================================

    103 - UPDATE
    There is no payload, successful error code is the acknowledgement
    =================================================================

    104 - MONITOR
    Two types of replies:
        Callback for facility is registered:
            Empty Payload

        When registered callback is triggered:
            Payload same as 101.
    ========================

    105 - QUERY_CAPACITY
    capacity - 4 bytes
    ==================

    106 - DELETE
    No payload. Only acknowledgement in form of error code.
    ==================
     
    107 - GET ALL FACILITY NAMES
    Total number of facilities (uint32_t)
    For each facility: ,
        Facility name length (uint32_t) 
        Facility name (char), non '\0' ending
*/

struct __attribute__ ((packed)) MarshalledMessage {
    uint32_t reqID; //request ID
    uint32_t uid; //confirmation UID
    uint32_t op; //operation to perform
    uint32_t payloadLen;
    char payload [];
};

struct UnmarshalledRequestMessage {
    uint32_t reqID;
    uint32_t uid; //confirmation id
    uint32_t op; //operation to perform
    std::vector<Day> days; //at most 7 days of the week
    std::string facilityName ;
    hourminute startTime; //times are represented as {1 1, 59} for 11:59
    hourminute endTime;

    int32_t offset; 
    //signed, in minutes
    //monitoring interval for a callback (max over a week = 1080 mins)
    //otherwise update time in case update 
}; 

struct UnmarshalledReplyMessage {
    uint32_t reqID; //field for testing to remove later
    uint32_t uid; //confirmation ID given by server
    uint32_t op; //operation that was performed
    uint32_t errorCode; //error code if revelant
    uint32_t capacity; //returns capacity, if relevant
    std::vector< std::string > facilityNames; // for op type '107'
    std::vector<std::pair<Day, std::vector<hourminute>>> availabilities; 
};


typedef std::pair<std::pair<std::string, Day>, bookStruct> serverBooking; 
//booking for (facility name, day, bookingTime)

struct CallbackInfo {
    struct sockaddr_in client_addr;
    sys_time recv_time;
    int32_t monitorInterval;

    CallbackInfo(struct sockaddr_in client_addr, sys_time recv_time, int32_t monitorInterval)
        : client_addr(client_addr), recv_time(recv_time), monitorInterval(monitorInterval) 
    { }

    bool operator < (const CallbackInfo& c1) const {
        return this->recv_time <= c1.recv_time;
    }
};

class Server {
    std::unordered_map<std::string, Facility> facilities;
    // facility name, facility

    std::unordered_map<uint32_t, serverBooking> bookings; 
    //uid, server booking

    InvocationSemantics semantics;
    //invocation semantics to use

    std::unordered_map<uint32_t, UnmarshalledReplyMessage> replyCache;
    //reply cache for t most once semantics

    std::unordered_map<std::string, std::set<CallbackInfo>> callbackMap;
    // facility_name,  Callbackinfo

    void query_request_handle (UnmarshalledRequestMessage& msg, char* payload, 
        int payloadLen) {
        
        uint32_t facilityNameLen = ntohl(*reinterpret_cast< uint32_t* > (payload));
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

    int getPayloadSize(const UnmarshalledReplyMessage* msg) {
        if (msg->errorCode != 100) return 0;
        // if (msg->op != 101) {
        //     if (msg->op == 105) return 4;
        //     return 0;
        // }
        // int size = 0;
        // size += sizeof(uint32_t); //numDays
        // for (auto val : msg->availabilities) {
        //     size += sizeof(Day); //day
        //     size += sizeof(uint32_t); //numAvail
        //     size += 2*sizeof(uint32_t)*val.second.size();
        // }
        // return size;


        switch (msg->op) {
            case 105:
                return 4;

            case 101:
            {
                int size = 0;
                size += sizeof(uint32_t); //numDays
                for (auto val : msg->availabilities) {
                    size += sizeof(Day); //day
                    size += sizeof(uint32_t); //numAvail
                    size += 2*sizeof(uint32_t)*val.second.size();
                }
                return size;
            }

            case 107:
            {
                int size = 0;
                size += sizeof(uint32_t); 
                // Total number of facilities (uint32_t)

                // uint32_t numFacilities = msg->facilityNames.size();
                for (const auto& name : msg->facilityNames) {
                    size += sizeof(uint32_t); // Facility name length (uint32_t) 
                    size += name.size(); // Facility name (char)
                }
                return size;
            }

            default:
                return 0;
        }
    }

    void query_facility_names_handle(const UnmarshalledReplyMessage* msg, char* payload) { 
        int payloadIdx = 0;
        uint32_t numFacilities = htonl(msg->facilityNames.size());
        memcpy(payload + payloadIdx, &numFacilities, sizeof(uint32_t));
        payloadIdx += sizeof(uint32_t);

        for (const auto& name : msg->facilityNames) {
            uint32_t len = name.size();
            uint32_t len_big_endian = htonl(len);
            memcpy(payload + payloadIdx, &len_big_endian, sizeof(uint32_t));
            payloadIdx += sizeof(uint32_t);

            const char* c_str_name = name.c_str();
            memcpy (payload + payloadIdx, c_str_name, len);
            payloadIdx += len;
        }

    }

    void query_request_handle(const UnmarshalledReplyMessage* msg, char* payload) {
        int payloadIdx = 0;
        uint32_t numDays = htonl(msg->availabilities.size());
        memcpy(payload+payloadIdx, &numDays, sizeof(uint32_t));
        payloadIdx += sizeof(uint32_t);
        for (auto res : msg->availabilities) {
            Day day = res.first;
            memcpy(payload+payloadIdx, &day, sizeof(Day));
            payloadIdx += sizeof(Day);
            uint32_t numAvail = htonl(res.second.size());
            memcpy(payload+payloadIdx, &numAvail, sizeof(uint32_t));
            payloadIdx += sizeof(uint32_t);
            for (auto time : res.second) {
                uint32_t start = htonl(time.first);
                uint32_t end = htonl(time.second);
                memcpy(payload+payloadIdx, &start, sizeof(uint32_t));
                payloadIdx += sizeof(uint32_t);
                memcpy(payload+payloadIdx, &end, sizeof(uint32_t));
                payloadIdx += sizeof(uint32_t);
            }
        }
        *payloadLen = htonl(payloadIdx);
        
    }

    void query_capacity_handle (UnmarshalledRequestMessage& msg, char* payload, 
        int payloadLen) {
            uint32_t facilityNameLen = ntohl(*reinterpret_cast< uint32_t* >(payload));
            std::string facilityName (reinterpret_cast< char* >(payload+4), facilityNameLen);
            msg.facilityName = std::move(facilityName);
    }

    void query_capacity_handle(const UnmarshalledReplyMessage* msg, char* payload) {
        uint32_t capacity = htonl(msg->capacity);
        memcpy(payload, &capacity, sizeof(uint32_t));
    }

    void monitor_handle (UnmarshalledRequestMessage& msg, char* payload, 
        int payloadLen) {

            int payloadIdx = 0;
            uint32_t facilityNameLen = ntohl(*reinterpret_cast< uint32_t* >(payload));
            payloadIdx += sizeof(uint32_t);

            std::string facilityName (reinterpret_cast< char* >(payload + payloadIdx), 
                facilityNameLen);
            msg.facilityName = std::move(facilityName);
            payloadIdx += facilityNameLen;

            msg.offset = ntohl(*reinterpret_cast< int32_t* >(payload + payloadIdx));
    }

    void update_request_handle (UnmarshalledRequestMessage& msg, char* payload, 
        int payloadLen) {
            msg.offset = ntohl(*reinterpret_cast< int32_t* >(payload));
    }

    void create_request_handle (UnmarshalledRequestMessage& msg, char* payload, 
        int payloadLen) {
            uint32_t facilityNameLen = ntohl(*reinterpret_cast< uint32_t* >(payload));
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

    MarshalledMessage* marshal(const UnmarshalledReplyMessage* msg, int* ptrTotalMsgSize) {

        int size = getPayloadSize(msg);
        MarshalledMessage* egressMsg = 
            (MarshalledMessage*) malloc(sizeof(MarshalledMessage) + size);

        *ptrTotalMsgSize = sizeof(MarshalledMessage) + size;
        uint32_t op = htonl(msg->errorCode);
        if (msg->errorCode != 100) return egressMsg;
        uint32_t uid = htonl(msg->uid);
        uint32_t reqId = htonl(msg->reqID);
        egressMsg->op = op;
        egressMsg->uid = uid;
        egressMsg->reqID = reqId;
        if (msg->errorCode != 100) return egressMsg;
        switch (msg->op) {
            case 101 : 
                query_request_handle(msg, egressMsg->payload,&egressMsg->payloadLen);
                break;
            case 105 :
                query_capacity_handle(msg, egressMsg->payload);
                break;
            case 107:
                query_facility_names_handle(msg, egressMsg->payload);
            default :
                //do nothing
                break;
        }
        return egressMsg;

    }
    
    UnmarshalledRequestMessage unmarshal(MarshalledMessage* msg) {

        UnmarshalledRequestMessage localMsg;
        localMsg.reqID = ntohl(msg->reqID);
        localMsg.uid = ntohl(msg->uid);
        localMsg.op = ntohl(msg->op);
        uint32_t payloadLen = ntohl(msg->payloadLen);
        
        int payloadIndex = 4*sizeof(uint32_t);
        
        switch (localMsg.op) {
            case 101:
                query_request_handle(localMsg, msg->payload, payloadLen);
                break;
            case 102:
                create_request_handle(localMsg, msg->payload, payloadLen);
                break;
            case 103:
                update_request_handle(localMsg, msg->payload, payloadLen);
                break;
            case 104:
                monitor_handle(localMsg, msg->payload, payloadLen);
                break;
            case 105:
                query_capacity_handle(localMsg, msg->payload, payloadLen);
                break;
            case 106:
                // nothing to do, only uses uid
                break;
            case 107:
                // No need to modify the unmarshalled request msg
                break;
            default:
                std::cerr << "wrong op type";
                // handle error
                // exit(1);
        }
        return localMsg;
    }

public:
    Server(std::unordered_map<std::string,Facility>& facilities, InvocationSemantics semantics) 
        : facilities(facilities), semantics(semantics) {}

    void handleQuery(UnmarshalledRequestMessage& msg, UnmarshalledReplyMessage& replyMsg) {
        replyMsg.op = 101;
        std::vector<Day> days = msg.days;
        std::string facilityName = msg.facilityName;
        if (facilities.find(facilityName) == facilities.end()) {
            replyMsg.errorCode = 200;
            return;
        }
        auto facility = facilities.at(facilityName);
        replyMsg.availabilities = std::move(facility.queryAvail(days));
        replyMsg.errorCode = 100;
    }

    void handleBooking(UnmarshalledRequestMessage& msg, UnmarshalledReplyMessage& replyMsg) {
        replyMsg.op = 102;
        Day day = msg.days[0];
        std::string facilityName = msg.facilityName;
        if (facilities.find(facilityName) == facilities.end()) {
            replyMsg.errorCode = 200;
            return;
        }
        Facility facility = facilities.at(facilityName);
        hourminute startTime = msg.startTime;
        hourminute endTime = msg.endTime;
        bookStruct booking = {startTime, endTime};
        bool success = facility.bookFacility(day, booking);
        if (!success) {
            replyMsg.errorCode = 300;
            return;
        }

        //assign UID
        uint32_t uid = getUniqueId();
        replyMsg.uid = uid;
        replyMsg.errorCode = 100;
    }

    int getUniqueId() {
        static uint32_t some_random_number = 101299;
        return ++some_random_number;
    }

    void handleUpdate(UnmarshalledRequestMessage& msg, UnmarshalledReplyMessage& replyMsg) {
        replyMsg.op = 103;
        uint32_t uid = msg.uid;
        if (bookings.find(uid) == bookings.end()) {
            replyMsg.errorCode = 400;
            return;
        }
        serverBooking booking = bookings[uid];
        std::string facilityName = booking.first.first;
        Day day = booking.first.second;
        bookStruct time = booking.second;
        Facility facility = facilities.at(facilityName);
        int32_t offset = msg.offset;
        bool success = facility.updateBooking(day, time, offset);
        if (!success) {
            replyMsg.errorCode = 400;
            return;
        }
        replyMsg.errorCode = 100;
        bookings[uid] = {{facilityName, day}, time};
    }

    void handleCallback(UnmarshalledRequestMessage& msg, UnmarshalledReplyMessage& replyMsg, 
        struct sockaddr_in client_addr, sys_time recv_time) {
        replyMsg.op = 104;
        if (facilities.find(msg.facilityName) == facilities.end()) {
            replyMsg.errorCode = 200;
            return;
        }
        callbackMap[msg.facilityName].insert( CallbackInfo(client_addr, recv_time, msg.offset) );
        // insert callback in the map, for the particular facility name
        replyMsg.errorCode = 100;
        // callback is registered successfully
    }

    void handleCapacity(UnmarshalledRequestMessage& msg, UnmarshalledReplyMessage& replyMsg) {
        replyMsg.op = 105;
        std::string facilityName = msg.facilityName;
        if (facilities.find(facilityName) == facilities.end()) {
            replyMsg.errorCode = 200;
            return;
        }
        Facility facility = facilities.at(facilityName);
        replyMsg.capacity = facility.queryCapacity();
        replyMsg.errorCode = 100;
    }

    void handleFacilityNames(UnmarshalledRequestMessage& msg, UnmarshalledReplyMessage& replyMsg) {
        replyMsg.op = 107; 
        for (const auto& [name, _] : facilities) {
           replyMsg.facilityNames.push_back(name);
        }
        replyMsg.errorCode = 100; 
    }

    void handleDelete(UnmarshalledRequestMessage& msg, UnmarshalledReplyMessage& replyMsg) {

        replyMsg.op = 106;
        uint32_t uid = msg.uid;
        if (bookings.find(uid) == bookings.end()) {
            replyMsg.errorCode = 400;
            return;
        }
        bookStruct time = bookings[uid].second;
        Day day = bookings[uid].first.second;
        std::string facilityName = bookings[uid].first.first;
        Facility facility = facilities.at(facilityName);
        facility.cancelBooking(time, day);
        bookings.erase(uid);
        replyMsg.errorCode = 100;
    }


    void triggerCallback(int& sockfd, const UnmarshalledRequestMessage& reqMsg, 
            const UnmarshalledReplyMessage& replyMsg) {
        
        const std::string& facilityName = reqMsg.facilityName;
        if (callbackMap.contains(facilityName) == false)
                return;

        sys_time curTime = std::chrono::high_resolution_clock::now(); 

        auto& facilityCallbacks = callbackMap[facilityName];
        for (auto it = facilityCallbacks.begin(); it != facilityCallbacks.end() ; ) {
            auto duration = curTime - it->recv_time;
            auto minutesDuration = std::chrono::duration_cast<std::chrono::minutes>
                (duration);
            int32_t durationInt = minutesDuration.count();
            if (it->monitorInterval >= durationInt) {

                UnmarshalledRequestMessage localIngress;
                UnmarshalledReplyMessage localEgress;
                localIngress.facilityName = facilityName;
                localIngress.op = 101;

                const auto day_list = std::list<Day>{Day::Monday, Day::Tuesday, Day::Wednesday, 
                    Day::Thursday, Day::Friday, Day::Saturday, Day::Sunday};

                #ifdef __cpp_lib_containers_ranges
                    localIngress.days.append_range(day_list);
                #else
                    localIngress.days.insert(localIngress.days.end(), 
                        day_list.cbegin(), day_list.cend());
                    #endif
                

                handleQuery(localIngress, localEgress);
                int totalMsgSize = 0; 
                MarshalledMessage* egressMsg = marshal(&localEgress, &totalMsgSize);
                memcpy (buffer, egressMsg, totalMsgSize);
                sendto(sockfd, buffer, totalMsgSize, 0, 
                        ( struct sockaddr* ) &it->client_addr,
                        (socklen_t) sizeof(it->client_addr));

                it++;
            }
            else {
                it = facilityCallbacks.erase(it);
            }
        }
    }

    char buffer[BUFFER_LEN];
    int serve() {
        int sockfd;
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
            int n = recvfrom(sockfd, buffer, BUFFER_LEN, 0, (struct sockaddr *)&client_addr, &len);
            if (n < 0) {
                perror("Receive failed");
                continue;
            }
            sys_time recv_time = std::chrono::high_resolution_clock::now(); 
            std::cout << "Received: " << buffer << " from " <<
                inet_ntoa(client_addr.sin_addr) << ":" << 
                ntohs(client_addr.sin_port) << "\n";

            const char* res = "ACK";
            sendto(sockfd, res, 3, 0, (struct sockaddr *)&client_addr, len);  
            //server sends ACK to client for at least once invocation semantics

            UnmarshalledRequestMessage localMsg = 
                unmarshal( reinterpret_cast< MarshalledMessage* >(buffer) );

            //plan maybe add a handler class here? handler class
            UnmarshalledReplyMessage localEgress;
            if (replyCache.find(localMsg.reqID) == replyCache.end() 
                || semantics == InvocationSemantics::AT_LEAST_ONCE) {

                switch (localMsg.op) {
                    case 101 : 
                        handleQuery(localMsg, localEgress);
                        break;
                    case 102 : 
                        handleBooking(localMsg, localEgress);
                        //check success error code and insert callback reply
                        break;
                    case 103 :
                        handleUpdate(localMsg, localEgress);
                        //check success error code and insert callback reply
                        break;
                    case 104 :
                        handleCallback(localMsg, localEgress, client_addr, recv_time);
                        break;
                    case 105 :
                        handleCapacity(localMsg, localEgress);
                        break;
                    case 106 :
                        handleDelete(localMsg, localEgress);
                        break;
                    case 107 : 
                        handleFacilityNames(localMsg, localEgress);
                        break;
                    default :
                        //do nothing
                        break;
                }
            }
            else {
                localEgress = replyCache[localMsg.reqID];
            }

            // Echo back the received message
            if ( !TESTMODE && semantics == InvocationSemantics::AT_MOST_ONCE) {
                replyCache[localMsg.reqID] = localEgress; //cache the reply for AT MOST ONCE
            }

            int totalMsgSize = 0; 
            MarshalledMessage* egressMsg = marshal(&localEgress, &totalMsgSize);
            memcpy(buffer, egressMsg, totalMsgSize);
            sendto(sockfd, buffer, totalMsgSize, 0, (struct sockaddr*) &client_addr, len);
            if ( localEgress.errorCode == 100 &&  
                ( localMsg.op == 102 || localMsg.op == 103 || localMsg.op == 106 ) ) {
                triggerCallback(sockfd, localMsg, localEgress);
            }
        }
    }

};