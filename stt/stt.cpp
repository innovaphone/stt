
/*-----------------------------------------------------------------------------------------------*/
/* Based on innovaphone App template                                                             */
/* copyright (c) innovaphone 2018                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

#include "platform/platform.h"
#include "common/os/iomux.h"
#include "common/interface/task.h"
#include "common/interface/socket.h"
#include "common/interface/webserver_plugin.h"
#include "common/interface/database.h"
#include "common/interface/json_api.h"
#include "common/interface/time.h"
#include "common/interface/translation.h"
#include "common/ilib/str.h"
#include "common/ilib/json.h"
#include "common/lib/appservice.h"
#include "common/lib/config.h"
#include "common/lib/tasks_postgresql.h"
#include "common/lib/appwebsocket.h"
#include "common/lib/app_updates.h"

#include "stt.h"
#include "rccapi.h"

#define DBG(x) debug->printf x

/*-----------------------------------------------------------------------------------------------*/
/* class STTService                                                                        */
/*-----------------------------------------------------------------------------------------------*/

STTService::STTService(class IIoMux * const iomux, class ISocketProvider * localSocketProvider, ISocketProvider* tcpSocketProvider, ISocketProvider* tlsSocketProvider, class IWebserverPluginProvider * const webserverPluginProvider, class IDatabaseProvider * databaseProvider, AppServiceArgs * args) : AppService(iomux, localSocketProvider, args)
{
    this->iomux = iomux;
    this->localSocketProvider = localSocketProvider;
    this->webserverPluginProvider = webserverPluginProvider;
    this->databaseProvider = databaseProvider;
    this->tcpSocketProvider = tcpSocketProvider;
    this->tlsSocketProvider = tlsSocketProvider;
}

STTService::~STTService()
{

}

void STTService::AppServiceApps(istd::list<AppServiceApp> * appList)
{
    appList->push_back(new AppServiceApp("innovaphone-stt"));
}

void STTService::AppInstancePlugins(istd::list<AppInstancePlugin> * pluginList)
{
    pluginList->push_back(new AppInstancePlugin("innovaphone.sttmanager", "innovaphone-stt.png", "innovaphone.sttmanagertexts"));
}

class AppInstance * STTService::CreateInstance(AppInstanceArgs * args)
{
    return new STT(iomux, this, args);
}

/*-----------------------------------------------------------------------------------------------*/
/* class STT                                                                               */
/*-----------------------------------------------------------------------------------------------*/

STT::STT(IIoMux * const iomux, class STTService * service, AppInstanceArgs * args) : AppInstance(service, args), AppUpdates(iomux),
    ConfigContext(nullptr, this),
    taskConfigInit(this, &STT::ConfigInitComplete),
    configParameterTextNum(this, "textNum", ""),
    configParameterTextPBX(this, "textPBX", ""),
    configParameterTextAPIKey(this, "textAPIKey", "", false, true),
    configParameterTextLocation(this, "textLocation", ""),
    configParameterTextInstanceId(this, "textInstanceId", ""),
    configParameterTranslateAPIKey(this, "translateAPIKey", "", false, true),
    configParameterTranslateLocation(this, "translateLocation", ""),
    configParameterTranslateInstanceId(this, "translateInstanceId", "")
{
    DBG((__FUNCTION__));
    this->stopping = false;
    this->iomux = iomux;
    this->service = service;
    this->webserverPlugin = service->webserverPluginProvider->CreateWebserverPlugin(iomux, service->localSocketProvider, this, args->webserver, args->webserverPath, this);
    this->database = service->databaseProvider->CreateDatabase(iomux, this, this);
    this->database->Connect(args->dbHost, args->dbName, args->dbUser, args->dbPassword);
    this->logFlags |= LOG_APP;
    this->logFlags |= LOG_APP_WEBSOCKET;
    this->logFlags |= LOG_DATABASE;
    this->currentTask = nullptr;
    this->currentId = 0;
    this->rccapi = 0;

    this->rccSession = 0;

    RegisterJsonApi(this);

    socketContext = service->tlsSocketProvider->CreateSocketContext(this);
    socketContext->DisableClientCertificate();
    
    Log("App instance started");
}

STT::~STT()
{
    DBG(("STT::~STT()"));
    if(rccapi) delete rccapi;
    if (database) delete database;
    if (socketContext) delete socketContext;
}

void STT::DatabaseConnectComplete(IDatabase * const database)
{
    DBG(("STT::DatabaseConnectComplete()"));
    class ITask* task = CreateInitTask(database);
    task->Start(&taskConfigInit);
}

void STT::ConfigInitComplete(class ITask* task)
{
    delete task;

    webserverPlugin->HttpListen(nullptr, nullptr, nullptr, nullptr, _BUILD_STRING_);
    webserverPlugin->WebsocketListen();
    Log("App instance initialized");
}

void STT::STTSessionClosed(class STTSession * session)
{
    DBG((__FUNCTION__));
    sessionList.remove(session);
    delete session;
    if (stopping) {
        TryStop();
    }
}

void STT::DatabaseShutdown(IDatabase * const database, db_error_t reason)
{
    delete this->database;
    this->database = nullptr;
    TryStop();
}

void STT::DatabaseError(IDatabase * const database, db_error_t error)
{

}

void STT::ServerCertificateUpdate(const byte * cert, size_t certLen)
{
    Log("STT::ServerCertificateUpdate cert:%x certLen:%u", cert, certLen);
}

void STT::WebserverPluginWebsocketListenResult(IWebserverPlugin * plugin, const char * path, const char * registeredPathForRequest, const char * host)
{
    DBG((__FUNCTION__));
    if (!this->stopping) {
        class STTSession * session = new STTSession(this);
        this->sessionList.push_back(session);
    }
    else {
        plugin->Cancel(wsr_cancel_type_t::WSP_CANCEL_UNAVAILABLE);
    }
}

void STT::WebserverPluginHttpListenResult(IWebserverPlugin * plugin, ws_request_type_t requestType, char * resourceName, const char * registeredPathForRequest, ulong64 dataSize)
{
    DBG((__FUNCTION__));
    if (requestType == WS_REQUEST_GET) {
        if (plugin->BuildRedirect(resourceName, _BUILD_STRING_, strlen(_BUILD_STRING_))) {
            return;
        }
    }
    plugin->Cancel(WSP_CANCEL_NOT_FOUND);
}

void STT::WebserverPluginClose(IWebserverPlugin * plugin, wsp_close_reason_t reason, bool lastUser)
{
    Log("WebserverPlugin closed");
    if (lastUser) {
        delete webserverPlugin;
        webserverPlugin = nullptr;
        TryStop();
    }
}

void STT::Stop()
{
    TryStop();
}

void STT::TryStop()
{
    printf("TryStop, stopping: %i\r\n", stopping);
    if (!stopping) {
        Log("App instance stopping");
        stopping = true;
        for (class transcriptionConnection* tc = transcriptionConnections.front(); tc; tc = tc->goNext()) {
            rccSession->HangUp(tc->call);
            tc->translation->Shutdown();
        }
        if (webserverPlugin) webserverPlugin->Close();
        if (database) database->Shutdown();
        for (std::list<STTSession *>::iterator it = sessionList.begin(); it != sessionList.end(); ++it) {
            (*it)->Close();
        }
    }
    if (!webserverPlugin && !database && sessionList.empty() && transcriptionConnections.empty()) appService->AppStopped(this);
}

void STT::ReceivedTranscriptedUUI(int call, const char* uui_ch) {
    char* uui_buf = (char*)malloc(strlen(uui_ch) + 1);
    memcpy(uui_buf, uui_ch, strlen(uui_ch));
    uui_buf[strlen(uui_ch)] = 0x0;
    float highest_confidence = 0;

    DBG(("Received UUI: %s, call: %i", uui_buf, call));

    json_io uui(uui_buf);
    uui.decode();
    word uui_base = uui.get_object(JSON_ID_ROOT, 0);

    for (class transcriptionConnection* tc = transcriptionConnections.front(); tc; tc = tc->goNext()) {
        char sb[10000];
        class json_io send(sb);
        char tstmp[128];
        char* tst = tstmp;

        if (tc->call == call) {
            ulong64 ts = ITime::TimeStampMilliseconds();
            if (tc->lastTranscription) {
                delete tc->lastTranscription;
            }
            tc->lastTranscription = new class transcriptionEntry(uui.get_double(uui_base, "confidence"), ts);

            if (!strncmp("en", tc->language, 2)) {
                tc->lastTranscription->setConfidence(tc->lastTranscription->getConfidence() + 0.2);
            }

            word base = send.add_object(0xffff, 0);
            send.add_string(base, "mt", "uui");
            send.add_string(base, "language", tc->language);
            send.add_ulong64(base, "timestamp", ts, tst);
            send.add_string(base, "uui", uui_ch);
            for (class appSession* as = tc->appSessions.front(); as; as = as->goNext()) {
                as->session->SendUpdate(send, sb);
            }

            for (class transcriptionConnection* tcs = transcriptionConnections.front(); tcs; tcs = tcs->goNext()) {
                if (tcs->lastTranscription) {
                    if (tcs->lastTranscription->getTimestamp() < (ts + 300) && (ts - 300) < tcs->lastTranscription->getTimestamp()) {
                        if (tcs->lastTranscription->getConfidence() > highest_confidence) {
                            highest_confidence = tcs->lastTranscription->getConfidence();
                        }
                    }
                }
            }
            for (class transcriptionConnection* tcs = transcriptionConnections.front(); tcs; tcs = tcs->goNext()) {
                if (tcs->lastTranscription) {
                    if (tcs->lastTranscription->getTimestamp() < (ts + 300) && (ts - 300) < tcs->lastTranscription->getTimestamp()) {
                        class json_io send(sb);
                        word base = send.add_object(0xffff, 0);
                        send.add_string(base, "mt", "txtHiglight");
                        send.add_string(base, "language", tcs->language);
                        send.add_ulong64(base, "timestamp", tcs->lastTranscription->getTimestamp(), tst);
                        if (tcs->lastTranscription->getConfidence() >= highest_confidence) {
                            send.add_bool(base, "highlight", true);
                        }
                        else {
                            send.add_bool(base, "highlight", false);
                        }
                        for (class appSession* as = tcs->appSessions.front(); as; as = as->goNext()) {
                            as->session->SendUpdate(send, sb);
                        }
                    }
                }
            }
            if (tc->translationConnected == true) {
                for (class transcriptionConnection* other_tc = transcriptionConnections.front(); other_tc; other_tc = other_tc->goNext()) {
                    if (strcmp(tc->language, other_tc->language)) {
                        DBG(("Translate src: %s, target: %s, text: %s", tc->language, other_tc->language, uui.get_string(uui_base, "transcript")));
                        tc->translation->Translate(new class TranslationObject(uui.get_string(uui_base, "transcript"), tc->language, other_tc->language));
                    }
                }
            }
            break;
        }
    }
    free(uui_buf);
}

void STT::TranslationConnected(const ITranslation* translator) {
    printf("Translation connected\r\n");
    for (class transcriptionConnection* tc = transcriptionConnections.front(); tc; tc = tc->goNext()) {
        if (translator == tc->translation) {
            tc->translationConnected = true;
            break;
        }
    }
}

void STT::TranslationCompleted(const ITranslation* translator, class TranslationObject* translationObject) {
    printf("Translation: %s\r\n", translationObject->getTranslation());
    for (class transcriptionConnection* tc = transcriptionConnections.front(); tc; tc = tc->goNext()) {
        if (translator == tc->translation) {
            char sb[10000];
            class json_io send(sb);
            char tstmp[128];
            char* tst = tstmp;
            word base = send.add_object(0xffff, 0);
            send.add_string(base, "mt", "translation");
            send.add_string(base, "sourceLanguage", translationObject->getSource_Language());
            send.add_string(base, "targetLanguage", translationObject->getTarget_Language());
            send.add_string(base, "translation", translationObject->getTranslation());
            send.add_ulong64(base, "timestamp", tc->lastTranscription->getTimestamp(), tst);
            for (class appSession* as = tc->appSessions.front(); as; as = as->goNext()) {
                as->session->SendUpdate(send, sb);
            }
            break;
        }
    }
}

void STT::TranslationShutdown(const ITranslation* translator) {
    for (class transcriptionConnection* tc = transcriptionConnections.front(); tc; tc = tc->goNext()) {
        if (tc->translation == translator) {
            delete tc->translation;
            tc->translation = nullptr;
            tc->remove();
            delete tc;
            break;
        }
    }
}

void STT::ConfigChanged() {
    if (this->configParameterTextPBX.Changed()) {
        for (class AppUpdatesSession * s = pbxSessions.front(); s; s = s->goNext()) {
            s->AppWebsocketClose();
        }
    }
}

/*-----------------------------------------------------------------------------------------------*/
/* class STTSession                                                                        */
/*-----------------------------------------------------------------------------------------------*/

STTSession::STTSession(class STT * instance) : AppUpdatesSession(instance, instance->webserverPlugin, instance, instance)
{
    DBG((__FUNCTION__));

    this->instance = instance;

    this->currentTask = nullptr;
    this->closed = false;
    this->closing = false;

    this->admin = false;
}

STTSession::~STTSession()
{
    DBG((__FUNCTION__));
}

void STTSession::AppWebsocketMessage(class json_io & msg, word base, const char * mt, const char * src)
{
    DBG((__FUNCTION__));
    if (!strcmp(mt, "PbxInfo")) {
        DBG(("PbxInfo"));
        instance->pbxSessions.push_back(this);
        if (!strcmp(msg.get_string(base, "pbx"), instance->getTextPbx())) {
            DBG(("Correct PBX"));
            instance->rccapi = new RccApi(this);
            instance->rccSession = (class RccApiSession*)instance->rccapi->CreateJsonApi(instance);
        }
        AppWebsocketMessageComplete();
    }
    else if (!strcmp(mt, "Dial")) {
        const char* num = msg.get_string(base, "dest");
        const char* language = msg.get_string(base, "language");
        DBG(("Dial %s, dest: %s, language: %s", num, instance->getTextNum(), language));
        bool existing = false;

        for (class transcriptionConnection* tc = instance->transcriptionConnections.front(); tc; tc = tc->goNext()) {
            if (!strcmp(tc->destNum, num) && !strcmp(tc->language, language)) {
                tc->appSessions.push_front(new appSession(this));
                char sb[10000];
                class json_io send(sb);
                word sbase = send.add_object(0xffff, 0);
                send.add_string(sbase, "mt", "DialResult");
                this->SendUpdate(send, sb);
                existing = true;
                break;
            }
        }

        if (!existing) {
            if (instance->rccSession) {
                const char* url_prefix = "https://api.";
                const char* url_constant = ".language-translator.watson.cloud.ibm.com";
                const char* endpoint_const = "/instances/";
                char url[100];
                char endpoint[100];
                strcpy(url, url_prefix);
                strcat(url, instance->getTranslateLocation());
                strcat(url, url_constant);
                strcpy(endpoint, endpoint_const);
                strcat(endpoint, instance->getTranslateInstanceId());
                int srcId = instance->rccSession->CallUser(num, instance->getTextNum());
                class transcriptionConnection* tc = new class transcriptionConnection(num, language, srcId, this);
                tc->translation = CreateIBMTranslation(instance->iomux, instance->service->tcpSocketProvider, instance->service->tlsSocketProvider, NULL, instance, instance, url, endpoint, instance->getTranslateAPIKey());

                instance->transcriptionConnections.push_front(tc);

            }
        }
        AppWebsocketMessageComplete();
    }
    else if (!strcmp(mt, "HangUp")) {
        const char* language = msg.get_string(base, "language");
        const char* num = msg.get_string(base, "dest");
        for (class transcriptionConnection* tc = instance->transcriptionConnections.front(); tc; tc = tc->goNext()) {
            if (!strcmp(tc->destNum, num) && !strcmp(tc->language, language)) {
                char sb[10000];
                class json_io send(sb);
                word sbase = send.add_object(0xffff, 0);
                send.add_string(sbase, "mt", "callDel");
                send.add_string(sbase, "language", tc->language);
                for (class appSession* as = tc->appSessions.front(); as; as = as->goNext()) {
                    if (as->session == this) {
                        as->session->SendUpdate(send, sb);
                        as->remove();
                        delete as;
                        break;
                    }
                }
                if (tc->appSessions.empty()) {
                    instance->rccSession->HangUp(tc->call);
                    tc->translation->Shutdown();
                }
                break;
            }
        }
        AppWebsocketMessageComplete();
    }
    else {
        AppWebsocketMessageComplete();
    }
}

void STTSession::AppWebsocketAppInfo(const char * app, class json_io & msg, word base)
{
    DBG((__FUNCTION__));

}

bool STTSession::AppWebsocketConnectComplete(class json_io & msg, word info)
{
    DBG((__FUNCTION__));
    const char * appobj = msg.get_string(info, "appobj");
    if (appobj && !strcmp(appobj, sip)) admin = true;
    return true;
}

void STTSession::AppWebsocketClosed()
{
    DBG((__FUNCTION__));
    closed = true;
    if (currentTask && currentTask->complete) currentTask->Finished();
    for (class transcriptionConnection* tc = instance->transcriptionConnections.front(); tc; tc = tc->goNext()) {
        for (class appSession* as = tc->appSessions.front(); as; as = as->goNext()) {
            if (as->session == this) {
                as->remove();
                delete as;
                break;
            }
        }
        if (tc->appSessions.empty()) {
            if (!instance->stopping) {
                instance->rccSession->HangUp(tc->call);
            }
            tc->translation->Shutdown();
            break;
        }
    }
    
    TryClose();
}

void STTSession::ResponseSent()
{
    DBG((__FUNCTION__));
    if (currentTask) {
        currentTask->Finished();
    }
    else {
        AppWebsocketMessageComplete();
    }
}

void STTSession::TryClose()
{
    DBG((__FUNCTION__));
    closing = true;
    if (!closed) {
        AppWebsocketClose();
        return;
    }
    if (currentTask) {
        currentTask->Stop();
        return;
    }
    if (closed && !currentTask) {
        instance->STTSessionClosed(this);
    }
}

void STTSession::Close()
{
    DBG((__FUNCTION__));
    TryClose();
}

bool STTSession::CheckSession(class ITask * task)
{
    DBG((__FUNCTION__));
    if (closing) {
        if (task) delete task;
        currentTask = nullptr;
        TryClose();
        return false;
    }
    return true;
}

appSession::appSession(class STTSession* session) {
    this->session = session;
}

transcriptionConnection::~transcriptionConnection() {
    if (destNum) free(destNum);
    if (lastTranscription) {
        delete lastTranscription;
    }
    if (language) {
        free(language);
    }
}

transcriptionConnection::transcriptionConnection(const char* num, const char*language, int srcId, class STTSession * session) {
    DBG(("transcriptionConnection::transcriptionConnection() num: %s, language: %s", num, language));
    class appSession* srcSession = new appSession(session);
    this->srcId = srcId;
    appSessions.push_back(srcSession);
    this->destNum = _strdup(num);
    this->language = _strdup(language);
    this->lastTranscription = 0;
    this->call = 0;
    this->translationConnected = false;
}
