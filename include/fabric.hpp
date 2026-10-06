#ifndef FABRIC_HPP
#define FABRIC_HPP

#include "protocol_handler.hpp"
#include "ftp_handler.hpp"

IProtocolHandler* make_ftpHandler(){
    FtpHandler* protocol = new FtpHandler();
    return protocol;
}

#endif