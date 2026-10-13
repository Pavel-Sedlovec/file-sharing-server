#ifndef FABRIC_HPP
#define FABRIC_HPP

#include "protocol/protocol_handler.hpp"
#include "protocol/ftp_handler.hpp"

IProtocolHandler* make_ftpHandler(){
    FtpHandler* protocol = new FtpHandler();
    return protocol;
}

#endif