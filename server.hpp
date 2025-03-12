#include <string>
#include <vector>
#include <set>
#include <utility>
#include <unordered_map>

typedef std::pair<int, int> hourminute

enum Day {
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday,
}

class Facility {
    std::string name;
    std::unordered_map<enum Day, std::set<hourminute, [](const hourminute& a, const hourminute& b) {
        if (a.second <= b.first) return true;
        return false;
    }>> reservations;

public:
    Facility(std::string name) : name(name) {}
    std::unordered_map<enum Day, std::vector<hourminute>> queryAvailability(std::vector<enum Day>& days) {
        std::unordered_map<enum Day, std::vector<hourminute>> availabilities;
        for (auto day : days) {
            if (!reservations[day].size()) {
                availabilities[day] = {{0, 0}, {23, 59}}
            }
            else {
                auto it = reservations.begin();
                hourminute prev = *it;
                it++;
                while (it != reservations.end()) {
                    int startAvail = prev.second;
                    int endAvail = it->first;
                    availabilities[day].push_back({startAvail, endAvail});
                }
            }
        }
        return availabilities;
    }
    
}