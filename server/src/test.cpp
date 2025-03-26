#include<iostream>
#include<stdio.h>
#include<unordered_map>
#include<string>
#include "../include/server.hpp"

int main(){
    std::unordered_map<std::string, Facility> facilities{};
    Server newServer{facilities, InvocationSemantics::AT_LEAST_ONCE};
    newServer.serve();
}