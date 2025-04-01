#include "../include/server.hpp"
#include <iostream>
#include <fmt/core.h>
#include <fmt/format.h>


int main(){
    Facility gym("Fitness Center", 50);
    Facility pool("Swimming Pool", 30);
    Facility conference("Conference Hall", 100);
    Facility library("Research Library", 75);
    Facility cafeteria("Main Cafeteria", 200);
    Facility lab("Computer Lab", 40);
    Facility theater("Auditorium", 350);
    Facility studio("Art Studio", 25);
    Facility lounge("Student Lounge", 60);
    Facility field("Sports Field", 120);

    std::vector<Facility> facility_vec = {
        gym, pool, conference, library, cafeteria,
        lab, theater, studio, lounge, field
    };

    std::unordered_map<std::string, Facility> facilities {};

    for (auto& facility : facility_vec) {
        facilities.emplace(facility.getName(), facility);
    }
    
    fmt::print("Welcome to the SC4051 Server\n"); 

    while(true) {
        fmt::print("Choose Server start mode\n 1. At Least Once\n 2. At Most Once\n");
        fmt::print("Enter your choice, 1 or 2:\n");
        int option = 0;
        std::cin >> option;

        InvocationSemantics semantics = InvocationSemantics::AT_MOST_ONCE; 

        if (option == 1) {
            semantics = InvocationSemantics::AT_LEAST_ONCE;
            Server n_server(facilities, semantics);
            n_server.serve();
        }
        else if (option == 2) {
            semantics = InvocationSemantics::AT_MOST_ONCE;
            Server n_server(facilities, semantics);
            n_server.serve();
        }
        else {
            fmt::print("Wrong input, please retry.\n");
        }
    }
    return 0;
}
