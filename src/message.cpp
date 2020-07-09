//#############################################################################
//#                           Copyright Paripath, Inc.
//#                            All Rights Reserved
//#############################################################################
//# Created by: Rohit Sharma
//#
//# Revision $Revision: 1.84 $, last checked in by $Author: srohit $
//# $Date: 2017/11/11 23:36:19 $.
//#
//# CVS ID: $Id: message.cpp,v 1.84 2017/11/11 23:36:19 srohit Exp $
//#############################################################################
//#
//# Description:
//
//
#include "message.h"
pbs_print* pbs_print::_pbs_printer = 0x0;
map<const char*, pbs_print::message, ltstr> pbs_print::messages;

using namespace std;

extern char* PRODUCT_NAME;
extern char* CHAR_COMMAND;
void
pbs_print::logstream() {
    // try opening log files in read mode, until you can't open one.
    // failure to open in read mode indicates, file does not exists.
    char logfile[32];
    filebuf buf;
    int index=1;
    strlen(CHAR_COMMAND) ? sprintf(logfile, "%schar.log", PRODUCT_NAME) : sprintf(logfile, "%s.log", PRODUCT_NAME);
    buf.open(logfile, ios::in);
    while ( buf.is_open() ) {
        strlen(CHAR_COMMAND) ? sprintf(logfile, "%schar%d.log", PRODUCT_NAME, index++) : sprintf(logfile, "%s%d.log", PRODUCT_NAME, index++) ;
        buf.close();
        buf.open(logfile, ios::in);
    }
    buf.close();

    _outbuf.open(logfile, ios::out);
    _out = _outbuf.is_open() ? new ostream(&_outbuf) : 0x0;
}

void
pbs_print::init() {

logstream();

// License message
pbs_print::messages["LIC-101"] = "could not open file." ;
pbs_print::messages["LIC-102"] = "wrong number of tokens ";
pbs_print::messages["LIC-103"] = "duplicate feature line ";
pbs_print::messages["LIC-104"] = "no license file found at ";
pbs_print::messages["LIC-105"] = "license file size is zero at ";
pbs_print::messages["LIC-106"] = "failed to checkin license. Please check- \t\t1. license file, \t\t2. license server\n" ;
pbs_print::messages["LIC-107"] = "using PARIPATH_LIC env var to find license details.\n";

}

void pbs_info(char* code, const char* msg) { pbs_print::pbs_printer()->print(pbs_print::INFO, code, msg); }
void pbs_warning(char* code, const char* msg) { pbs_print::pbs_printer()->print(pbs_print::WARNING, code, msg); }
void pbs_error(char* code, const char* msg) { pbs_print::pbs_printer()->print(pbs_print::ERROR, code, msg); }
void pbs_fatal(char* code, const char* msg) { pbs_print::pbs_printer()->print(pbs_print::FATAL, code, msg); }
void pbs_msg(const char* msg) { pbs_print::pbs_printer()->print(pbs_print::NOTAG, 0x0, msg); }

void
pbs_print::print(msg_type type, char* msg_code, const char* info) const
{
    message* msg_info = 0x0 ;
    string txt ;
    size_t limit = UINT_MAX ;
    if ( msg_code )
    {
#ifndef _NO_CONFIG_MGR
        txt = configManager::get("message_level", msg_code) ;
        string limitStr = configManager::get("message_limit", msg_code) ;
        if ( limitStr.size() )
            limit = atoi(limitStr.c_str());
#endif

        map<const char*, message, ltstr>::iterator miter = messages.find(msg_code) ;
        if ( miter != messages.end() )
            msg_info = &(*miter).second; 

        if ( msg_info && msg_info->is_valid() && msg_info->times_printed >= limit )
            return ;
    }
    if ( txt.empty() )
    {
        switch (type) {
            case INFO:
                txt = "info";
                break;
            case WARNING:
                txt = "warn";
                break;
            case ERROR:
                txt = "Error";
                break;
            case FATAL:
                txt = "FATAL";
                break;
            case NOTAG:
            default:
                break;
        }
    }
    if ( msg_code )
    {
        txt += string("(") + msg_code + string("): ");

        string generic_msg = msg_info && msg_info->is_valid() ?
            msg_info->verbiage : "message not registered." ;

        if ( generic_msg.empty() )
            msg_code = 0x0;
        else
            txt += generic_msg + "\n";

    } else if ( type != NOTAG ) {
        txt += ":";
    }
    if (info) {
        txt += msg_code ? "               " : "";
        txt += info;
        txt += "\n";
    }

    if ( _out ) (*_out) << txt;
    cout << txt;

    if ( msg_code && msg_info && msg_info->is_valid() && ++msg_info->times_printed == limit )
    {
        char limit_reached_msg[512] ;
        sprintf(limit_reached_msg, "Message limit for tag %s has reached the preset limit of %zu.\n\tThis message will not be printed again.\n", msg_code, msg_info->times_printed);
        if ( _out )
            (*_out) << limit_reached_msg;
        cout << limit_reached_msg;
    }
}
