#ifndef MUX_H
#define MUX_H
#include "constants.hpp"
#include <unordered_map>
#include <string>
#include <vector>


//mux to handle routing via op codes, middleware feature to make it easy to add features such as packet loss and cacheing
class Mux {
    public:
    std::unordered_map<std::string,Handler> handlerMap;
    std::vector<Handler> middlewares;
    Mux() : handlerMap{}{};
    void insert(std::string route,Handler func){
        handlerMap.insert({route,func});
    }

    void use(Middleware func){
        for( auto &[_,handler] : handlerMap){
            handler = func(handler);
        };
    }

    //return multiple reply messages for the case of query avail
    std::vector<ReplyMessage> handle(RequestMessage incomingMessage){
        std::string route{incomingMessage.op};
        return handlerMap[route](incomingMessage);
    }

};



#endif // !MUX_HPP
