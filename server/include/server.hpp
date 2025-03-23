#include <string>
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
typedef std::pair<int, int> hourminute;
typedef std::pair<hourminute, hourminute> bookStruct; //bookings are represented via {{startHour, StartMin}, {endHour, endMin}}

enum Day : char {
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday,
};

class Facility {
    std::string name;
    std::mutex mtx;
    int capacity;
    std::unordered_map<enum Day, std::set<bookStruct, [](const bookStruct& a, const bookStruct& b) {
        if (a.second <= b.first) return true;
        return false;
    }>> reservations; //a reservation is of form (1200)

    int hourToTimestamp(hourminute time) {
        return 60*time.first + time.second;
    }
    hourminute timestampToHour(int time) {
        int hour = time.first/60, minute = time.second%60;
        return {hour, minute};
    }
    hourminute updateTime(hourminute a, hourminute b, bool add) {
        return add ? timestampToHour(hourToTimestamp(a)+hourToTimestamp(b)) : timestampToHour(hourToTimestamp(a)-hourToTimestamp(b));

    }
    bool isWellOrdered(bookStruct booking, enum Day day) {
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
    Facility(std::string name) : name(name), capacity(capacity) {}
    std::unordered_map<enum Day, std::vector<bookStruct>> queryAvailability(std::vector<enum Day>& days) {
        std::lock_guard<std::mutex> lock(mtx);
        std::unordered_map<enum Day, std::vector<bookStruct>> availabilities;
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
    bool bookFacility(enum Day day, bookStruct bookTime) {
        std::lock_guard<std::mutex> lock(mtx);
        if (isWellOrdered(bookTime, day)) {
            reservations[day].insert(bookTime);
            return true;
        }
        return false;
        
    }
    bool updateBooking(enum Day day, bookStruct booking, bool delay, hourminute offset) {
        std::lock_guard<std::mutex> lock(mtx);
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
    bool cancelBooking(bookStruct bookTime, enum Day day) {
        //non-idempotent service, server side occurs via confirmation ID
        if (!reservations[day].contains(bookTime)) return false;
        reservations[day].erase(bookTime);
        return true;
    }
}
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



typedef std::pair<std::pair<std::string, enum Day>, bookStruct> serverBooking; //booking for (facility name, day, bookingTime)
class Server {
    std::unordered_map<std::string, Facility> facilities;
    std::unordered_map<std::string, serverBooking> bookings; //by confirmationID

    char* bookstructToTime(bookStruct a) {
        char time[8];
        hourminute start = a.first, end = a.second;
        time[0] = start.first / 10 + '0', time[1] = start.first % 10 + '0';
        time[2] = start.second / 10 + '0', time[3] = start.second % 10 + '0';
        time[4] = end.first / 10 + '0', time[5] = end.first % 10 + '0';
        time[6] = end.second / 10 + '0', time[7] = end.second % 10 + '0';
        return time;
    }
    char* marshal(ReplyMessage* reply) {
        char* buf = (char*) malloc(sizeof(reply));
        uint32_t availabilities = htonl(reply->availabilities);
        uint32_t uid = htonl(reply->uid);
        uint32_t numAvail = htonl(reply->numAvail);
        uint32_t errorCode = htonl(reply->errorCode);
        uint32_t capacity = htonl(reply->capacity);
        uint32_t day = htonl(reply->day);

        char* data = (char*) malloc(sizeof(reply));
        char* dataBegin = data;
        memcpy(dataBegin, &availabilities, sizeof(availabilities));
        dataBegin += sizeof(availabilities); 
        memcpy(dataBegin, &uid, sizeof(uid));
        dataBegin += sizeof(uid);
        memcpy(dataBegin, &numAvail, sizeof(numAvail));
        dataBegin += sizeof(numAvail);
        memcpy(dataBegin, &errorCode, sizeof(errorCode));
        dataBegin += sizeof(errorCode);
        memcpy(dataBegin, &capacity, sizeof(capacity));
        dataBegin += sizeof(capacity);
        memcpy(dataBegin, &day, sizeof(day));
        return data;
    }
    RequestMessage unmarshal(char data[sizeof(RequestMessage)]) {
        uint32_t uid = ntohl(data[0]);
        uint32_t days = ntohl(data[8]);
        // uint32_t op = ntohl(data[15]); 
        uint8_t op = data[15]; 
        uint32_t facilityName = ntohl(data[16]);
        uint32_t startTime = ntohl(data[20]);
        uint32_t endTime = ntohl(data[24]);
        uint32_t offset = ntohl(data[28]);
        RequestMessage* msg = (RequestMessage*) malloc(sizeof(RequestMessage));

        memcpy(msg->uid, &uid, sizeof(uid));
        memcpy(msg->days, &days, sizeof(days));
        memcpy(&msg->op, &op, sizeof(op));
        memcpy(msg->facilityName, &facilityName, sizeof(facilityName));
        memcpy(msg->startTime, &startTime, sizeof(startTime));
        memcpy(msg->endTime, &endTime, sizeof(endTime));
        memcpy(msg->offset, &offset, sizeof(offset));
        return *msg;
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
                        auto avails = f.queryAvailability(days);
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
