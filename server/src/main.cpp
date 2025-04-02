#include "../include/server.hpp"
#include <iostream>
#include <boost/program_options.hpp>
#include <fmt/core.h>
#include <fmt/format.h>


int main(int argc, char* argv[]){

    namespace po = boost::program_options;
    bool atMost = false;
    bool atLeast = true;
    bool simulateFailure = false;
    
    po::options_description desc("Allowed Options");
    
    //bool_swtich sets the vlaue to "true", always
    desc.add_options()
        ("atmost,am", po::bool_switch(&atMost), 
            "Use at-most semantics")
        ("atleast,al", po::bool_switch(&atLeast) ,
            "Use at-least semantics (default)")
        ("failure,f", po::bool_switch(&simulateFailure), 
            "Simulate Ack drop by server");
        
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const po::error &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        std::cerr << desc << "\n";
        return 1;
    }

    if (atMost && atLeast) {
        std::cerr << "Error: You cannot specify both --atmost and --atleast together.\n";
        return 1; // Exit with error
    }

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
        // fmt::print("Choose Server start mode\n 1. At Least Once\n 2. At Most Once\n");
        // fmt::print("Enter your choice, 1 or 2:\n");
        // std::cin >> option;

        InvocationSemantics semantics = InvocationSemantics::AT_MOST_ONCE; 

        if (atMost == true) {
            semantics = InvocationSemantics::AT_MOST_ONCE;
            Server n_server(facilities, semantics);
            n_server.serve();
        }
        else if (atLeast == true) {
            semantics = InvocationSemantics::AT_LEAST_ONCE;
            Server n_server(facilities, semantics);
            n_server.serve();
        }
        else {
            fmt::print("Wrong input, please retry.\n");
        }
    }
    return 0;
}
