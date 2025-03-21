#ifndef FACILITY_H
#define FACILITY_H
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
#include "constants.hpp"


class Facility {
    std::string name;
    std::mutex mtx;
    int capacity;
    std::unordered_map<enum Day, std::set<bookStruct, decltype([](const bookStruct& a, const bookStruct& b) {
        if (a.second <= b.first) return true;
        return false;
    })>> reservations; //a reservation is of form (1200)

    int hourToTimestamp(hourminute time) {
        return 60*time.first + time.second;
    }
    hourminute timestampToHour(int time) {
        int hour = time/60, minute = time%60;
        return {hour, minute};
    }
    hourminute updateTime(hourminute a, hourminute b, bool add) {
        return add ? timestampToHour(hourToTimestamp(a)+hourToTimestamp(b)) : timestampToHour(hourToTimestamp(a)-hourToTimestamp(b));

    }
    bool isWellOrdered(bookStruct booking, enum Day day) {
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
    explicit Facility(std::string name) : name(name), capacity{} {}
    std::unordered_map<enum Day, std::vector<bookStruct>> queryAvailability(std::vector<enum Day>& days) {
        std::lock_guard<std::mutex> lock(mtx);
        std::unordered_map<enum Day, std::vector<bookStruct>> availabilities;
        for (auto day : days) {
            if (!reservations[day].size()) {
                availabilities[day] = {{{0, 0}, {23, 59}}};
            }
            else {
                auto it = reservations[day].begin();
                bookStruct prev = *it;
                it++;
                while (it != reservations[day].end()) {
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
};





#endif // !FACILITY_H