#ifndef SERVER_HPP
#include <string>
#include <vector>
#include <iostream>
#include <set>
#include <utility>
#include <unordered_map>
#include <algorithm>
#include <atomic>
#include <cstring>    
#include <arpa/inet.h>  
#include <unistd.h>
#include "constants.hpp"
#include "facility.hpp"
#include "mux.hpp"

#define PORT 3000

//add to facility hpp
// typedef std::pair<std::pair<std::string, enum Day>, bookStruct> serverBooking; //booking for (facility name, day, bookingTime)

//add as utility function
    // char* bookstructToTime(bookStruct a) {
    //     char* time = new char[8];
    //     hourminute start = a.first, end = a.second;
    //     time[0] = start.first / 10 + '0', time[1] = start.first % 10 + '0';
    //     time[2] = start.second / 10 + '0', time[3] = start.second % 10 + '0';
    //     time[4] = end.first / 10 + '0', time[5] = end.first % 10 + '0';
    //     time[6] = end.second / 10 + '0', time[7] = end.second % 10 + '0';
    //     return time;
    // }


//initialise in main
    // std::unordered_map<std::string, Facility> facilities;
    // std::unordered_map<std::string, serverBooking> bookings; //by confirmationID


class Server {
    Mux requestMultiplexer;

    char* marshal(ReplyMessage* reply) {
        char* buf = (char*) malloc(sizeof(reply));

        uint64_t availBuff{};
        uint64_t uidBuff{};
        memcpy(&availBuff,reply->availabilities,sizeof(reply->availabilities));
        memcpy(&uidBuff,reply->uid,sizeof(reply->uid));

        uint64_t availabilities = htonll(availBuff);
        uint64_t uid = htonll(uidBuff);
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
    Server(Mux &mux) : requestMultiplexer(mux)  {};
    int serve() {
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

            std::vector<ReplyMessage> replies = requestMultiplexer.handle(msg);
            char *dataBuffer = (char*) malloc(replies.size()*sizeof(ReplyMessage));
            int dataIndex = 0;
            for(ReplyMessage reply : replies){
                char *data = marshal(&reply);
                memcpy(dataBuffer+dataIndex,data,sizeof(reply));
                dataIndex+=sizeof(reply);
            }
            sendto(sockfd,dataBuffer,sizeof(dataBuffer),0,(struct sockaddr*)&client_addr,len);

            // ReplyMessage* reply = (ReplyMessage*) malloc(sizeof(ReplyMessage));
            // switch (msg.op) {
            //     case 'A' : {
            //         std::string facility = std::string(msg.facilityName);
            //         if (facilities.find(facility) == facilities.end()) {
            //             ReplyMessage* reply = (ReplyMessage*) malloc(sizeof(ReplyMessage));
            //             reply->errorCode = 200;
            //             char data[sizeof(reply)] = marshal(reply);
            //             sendto(sockfd, data, sizeof(reply), 0, (struct sockaddr *)&client_addr, len);
            //         }
            //         else {
            //             Facility f = facilities[facility];
            //             std::vector<enum Day> days(msg.days);
            //             auto avails = f.queryAvailability(days);
            //             char* data = (char*) malloc(days.size()*sizeof(ReplyMessage));
            //             int dataIndex = 0;
            //             for (auto day : avails) {
            //                 ReplyMessage reply;
            //                 reply.day = day;
            //                 reply.errorCode = 100;
            //                 reply.numAvail = avails[day].size();
            //                 char** availabilities = (char**) malloc(8*numAvail);
            //                 int index = 0;
            //                 for (auto booking : avails[day]) {
            //                     availabilities[index++] = bookstructToTime(booking);
            //                 }
            //                 reply.availabilities = availabilities;
            //                 char* marshalledData = marshal(&reply);
            //                 memcpy(data+dataIndex, marshalledData, sizeof(reply));
            //                 dataIndex += sizeof(reply);
            //             }
            //             sendto(sockfd, data, days.size()*sizeof(reply), 0, (struct sockaddr *)&client_addr, len);
            //         }
            //     }
            //     case 'B' : {
                    
            //     }
            // }
            // Echo back the received message
            sendto(sockfd, buffer, n, 0, (struct sockaddr *)&client_addr, len);
        }

        return 0;
    }


};



#endif // !SERVER_HPP