/*-----------------------------------------------------------------------------------------------*/
/* rccapi.cpp                                                                                    */
/* copyright (c) innovaphone 2019                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

#include "platform/platform.h"
#include "common/os/iomux.h"
#include "common/interface/task.h"
#include "common/interface/socket.h"
#include "common/interface/webserver_plugin.h"
#include "common/interface/database.h"
#include "common/interface/json_api.h"
#include "common/interface/translation.h"
#include "common/ilib/str.h"
#include "common/ilib/json.h"
#include "common/lib/appservice.h"
#include "common/lib/config.h"
#include "common/lib/tasks_postgresql.h"
#include "common/lib/appwebsocket.h"
#include "common/lib/app_updates.h"

#include "STT.h"
#include "rccapi.h"

#define DBG(x) //debug->printf x

/*-----------------------------------------------------------------------------------------------*/
/* RccApi                                                                                        */
/*-----------------------------------------------------------------------------------------------*/

RccApi::RccApi(class IJsonApiConnection* connection)
{
    this->connection = connection;
}

class JsonApi* RccApi::CreateJsonApi(class STT * sttInstance)
{
    DBG(("RccApi::CreateJsonApi()"));
    class RccApiSession* session = new RccApiSession(this, connection, sttInstance);
    sessions.push_back(session);
    return session;
}

/*-----------------------------------------------------------------------------------------------*/
/* RccApiSession                                                                                 */
/*-----------------------------------------------------------------------------------------------*/

RccApiSession::RccApiSession(class RccApi* rccapi, class IJsonApiConnection* connection, class STT* sttInstance)
{
    DBG(("RccApiSession::RccApiSession()"));
    this->rccapi = rccapi;
    this->connection = connection;
    this->sttInstance = sttInstance;
    char sb[1000];
    class json_io send(sb);
    word base = send.add_object(0xffff, 0);
    send.add_string(base, "mt", "Initialize");
    send.add_string(base, "api", "RCC");
    send.add_bool(base, "grp", true);
    send.add_bool(base, "calls", true);

    srcId = 0;

    connection->JsonApiMessage(send, sb);
    connection->RegisterJsonApi(this);
}

RccApiSession::~RccApiSession()
{
    DBG(("RccApiSession::~RccApiSession()"));
    connection->UnRegisterJsonApi(this);
}

int RccApiSession::CallUser(const char* e164, const char* text_e164) {
    char sb[4000];
    char tmp[128];
    char* t = tmp;
    class json_io send(sb);
    srcId += 1;
    std::string src = std::to_string(srcId);

    word base = send.add_object(0xffff, 0);
    send.add_string(base, "mt", "UserCall");
    send.add_string(base, "api", "RCC");
    send.add_string(base, "src", src.c_str());
    send.add_bool(base, "audio", true);
    send.add_string(base, "e164", e164);
    send.add_string(base, "srce164", text_e164);
    send.add_int(base, "user", userId, t);

    connection->JsonApiMessage(send, sb);

    return srcId;
}

void RccApiSession::HangUp(int call) {
    DBG(("RccApiSession::HangUp()"));
    char sb[1000];
    class json_io send(sb);
    char tmp[128];
    char* t = tmp;

    word base = send.add_object(0xffff, 0);
    send.add_string(base, "mt", "UserClear");
    send.add_string(base, "api", "RCC");
    send.add_int(base, "call", call, t);

    connection->JsonApiMessage(send, sb);
}

void RccApiSession::SendUUI(int call, const char *language) {
    char sb[10000];
    class json_io send(sb);
    char uuib[10000];
    class json_io uui(uuib);
    char tmp[128];
    char* t = tmp;
    word base = send.add_object(0xffff, 0);
    send.add_string(base, "mt", "UserUUI");
    send.add_string(base, "api", "RCC");
    send.add_bool(base, "recv", true);
    send.add_int(base, "call", call, t);
    word uui_base = uui.add_object(0xffff, 0);
    uui.add_string(uui_base, "textservice", "ibm-watson");
    word textservice_params = uui.add_object(uui_base, "params");
    uui.add_string(textservice_params, "apikey", sttInstance->getTextAPIKey());
    uui.add_string(textservice_params, "location", sttInstance->getTextLocation());
    uui.add_string(textservice_params, "instance_id", sttInstance->getTextInstanceId());
    uui.add_string(textservice_params, "language", language);
    uui.encode();

    DBG(("UUI: %s", uuib));

    send.add_string(base, "uui", uuib);
    
    connection->JsonApiMessage(send, sb);
}

void RccApiSession::Message(class json_io& msg, word base, const char* mt, const char* src)
{
    char sb[10000];
    class json_io send(sb);

    DBG(("RccApiSession::Message(%s)", mt));
    if (!strcmp(mt, "UserInfo")) {
    }
    else if (!strcmp(mt, "InitializeResult")) {
        DBG(("InitializeResult"));
        word base = send.add_object(0xffff, 0);
        send.add_string(base, "mt", "UserInitialize");
        send.add_string(base, "api", "RCC");
        send.add_string(base, "cn", "text");
        send.add_bool(base, "xfer", true);
        send.add_bool(base, "disc", false);
        send.add_string(base, "hw", "text");
        connection->JsonApiMessage(send, sb);
    }
    else if (!strcmp(mt, "UserInitializeResult")) {
        userId = msg.get_int(base, "user");
    }
    else if (!strcmp(mt, "CallInfo")) {
        const char* callmsg = msg.get_string(base, "msg");
        if (!strcmp(callmsg, "r-user-info")) {
            word info = msg.get_array(base, "info");
            word o0 = msg.get_object(info, 0);
            const char* vals = msg.get_string(o0, "vals");
            int call = msg.get_int(base, "call");
            
            if (vals) {
                sttInstance->ReceivedTranscriptedUUI(call, vals);
            }
        }
        else if (!strcmp(callmsg, "del")) {
            word sbase = send.add_object(0xffff, 0);
            DBG(("Callmsg del"));
            int call = msg.get_int(base, "call");


            for (class transcriptionConnection* tc = sttInstance->transcriptionConnections.front(); tc; tc = tc->goNext()) {
                DBG(("Del Call: %i, this call: %i", call, tc->call));
                if (tc->call == call) {
                    class appSession* prevas = nullptr;
                    send.add_string(sbase, "mt", "callDel");
                    send.add_string(sbase, "language", tc->language);
                    for (class appSession* as = tc->appSessions.front(); as; as = as->goNext()) {
                        if (prevas) {
                            delete prevas;
                        }
                        as->session->SendUpdate(send, sb);
                        as->remove();
                        prevas = as;
                    }
                    if (prevas) {
                        delete prevas;
                    }
                    tc->remove();
                    delete tc;
                    break;
                }
            }
        }
        else if (!strcmp(callmsg, "r-conn")) {
            int call = msg.get_int(base, "call");
            for (class transcriptionConnection* tc = sttInstance->transcriptionConnections.front(); tc; tc = tc->goNext()) {
                DBG(("r-conn: %i, this call: %i, tc-language: %s", call, tc->call, tc->language));
                if (tc->call == call) {
                    SendUUI(call, tc->language);
                }
            }
        }
    }
    else if (!strcmp(mt, "UserCallResult")) {
        DBG(("UserCallResult"));
        if (src) {
            for (class transcriptionConnection* tc = sttInstance->transcriptionConnections.front(); tc; tc = tc->goNext()) {
                DBG(("UserCallResult src: %s, tc->srcID: %i, tc->language: %s", src, tc->srcId, tc->language));
                if (!strcmp(std::to_string(tc->srcId).c_str(), src)) {
                    word sbase = send.add_object(0xffff, 0);
                    tc->call = msg.get_int(base, "call");

                    send.add_string(sbase, "mt", "DialResult");
                    for (class appSession* as = tc->appSessions.front(); as; as = as->goNext()) {
                        as->session->SendUpdate(send, sb);
                    }

                    break;
                }
            }
        }
    }

    connection->JsonApiMessageComplete();
}

void RccApiSession::JsonApiConnectionClosed()
{
    delete this;
}