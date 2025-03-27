#include "../include/server.hpp"
#include <iostream>
#include <fmt/core.h>
#include <fmt/format.h>

std::unordered_map<std::string, Facility> facilities {};

int main(){
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
