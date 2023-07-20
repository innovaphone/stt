
/*-----------------------------------------------------------------------------------------------*/
/* Based on innovaphone App template                                                             */
/* copyright (c) innovaphone 2018                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

class appSession : public istd::listElement<class appSession> {

public:
    class STTSession* session;
    appSession(class STTSession* session);
};

class transcriptionEntry {
private:
    float confidence;
    ulong64 timestamp;

public:
    transcriptionEntry(float confidence, ulong64 timestamp) {
        this->confidence = confidence;
        this->timestamp = timestamp;
    }
    float getConfidence() { return confidence; }
    void setConfidence(float confidence) { this->confidence = confidence; }
    ulong64 getTimestamp() { return timestamp; }
};

class transcriptionConnection : public istd::listElement<class transcriptionConnection> {

public:
    char* destNum;
    char* language;
    int srcId;
    int call;
    class istd::list<class appSession> appSessions;
    class transcriptionEntry *lastTranscription;
    class ITranslation* translation;
    bool translationConnected;

    transcriptionConnection(const char* num, const char* language, int srcId, class STTSession * session);
    ~transcriptionConnection();
};

class STTService : public AppService {
    class AppInstance * CreateInstance(AppInstanceArgs * args) override;
    void AppServiceApps(istd::list<AppServiceApp> * appList) override;
    void AppInstancePlugins(istd::list<AppInstancePlugin> * pluginList) override;

public:
    STTService(class IIoMux * const iomux, class ISocketProvider * localSocketProvider, ISocketProvider* tcpSocketProvider, ISocketProvider* tlsSocketProvider, IWebserverPluginProvider * const webserverPluginProvider, IDatabaseProvider * databaseProvider, AppServiceArgs * args);
    ~STTService();

    class IIoMux * iomux;
    class ISocketProvider * localSocketProvider;
    class IWebserverPluginProvider * webserverPluginProvider;
    class IDatabaseProvider * databaseProvider;
    class ISocketProvider* tcpSocketProvider;
    class ISocketProvider* tlsSocketProvider;
};

class STT : public AppInstance, public AppUpdates, public UDatabase, public UWebserverPlugin, public JsonApiContext, public ConfigContext, public UTranslation
{
    void DatabaseConnectComplete(IDatabase * const database) override;
    void DatabaseShutdown(IDatabase * const database, db_error_t reason) override;
    void DatabaseError(IDatabase * const database, db_error_t error) override;

    void ConfigInitComplete(class ITask* task);
    class UTaskTemplate<STT, class ITask> taskConfigInit;

    void WebserverPluginClose(IWebserverPlugin * plugin, wsp_close_reason_t reason, bool lastUser) override;
    void WebserverPluginWebsocketListenResult(IWebserverPlugin * plugin, const char * path, const char * registeredPathForRequest, const char * host) override;
    void WebserverPluginHttpListenResult(IWebserverPlugin * plugin, ws_request_type_t requestType, char * resourceName, const char * registeredPathForRequest, ulong64 dataSize) override;

    void ServerCertificateUpdate(const byte * cert, size_t certLen) override;
    void Stop() override;

    class ConfigString configParameterTextNum;
    class ConfigString configParameterTextPBX;
    class ConfigString configParameterTextAPIKey;
    class ConfigString configParameterTextLocation;
    class ConfigString configParameterTextInstanceId;
    class ConfigString configParameterTranslateAPIKey;
    class ConfigString configParameterTranslateLocation;
    class ConfigString configParameterTranslateInstanceId;

    class ITask * currentTask;
    std::list<class STTSession *> sessionList;

    class ISocketContext* socketContext;


public:
    STT(IIoMux * const iomux, STTService * service, AppInstanceArgs * args);
    ~STT();

    void STTSessionClosed(class STTSession * session);
    void ReceivedTranscriptedUUI(int call, const char* uui_ch);

    void TryStop();

    void TranslationConnected(const ITranslation* translator);
    void TranslationCompleted(const ITranslation* translator, class TranslationObject* translationObject);
    void TranslationShutdown(const ITranslation* translator);

    void ConfigChanged() override;

    const char * appPwd() { return args.appPassword; };
    const char* getTextNum() { return configParameterTextNum.Value(); };
    const char* getTextPbx() { return configParameterTextPBX.Value(); };
    const char* getTextAPIKey() { return configParameterTextAPIKey.Value(); };
    const char* getTextLocation() { return configParameterTextLocation.Value(); };
    const char* getTextInstanceId() { return configParameterTextInstanceId.Value(); };
    const char* getTranslateAPIKey() { return configParameterTranslateAPIKey.Value(); };
    const char* getTranslateLocation() { return configParameterTranslateLocation.Value(); };
    const char* getTranslateInstanceId() { return configParameterTranslateInstanceId.Value(); };

    class IIoMux * iomux;
    class STTService * service;
    class IWebserverPlugin * webserverPlugin;
    class IDatabase * database;

    ulong64 currentId;
    bool stopping;

    class RccApi* rccapi;
    class RccApiSession* rccSession;

    std::list<class STTSession*> pbxSessions;
  
    istd::list<transcriptionConnection> transcriptionConnections;
};

class STTSession : public AppUpdatesSession {
    void AppWebsocketAccept(class UWebsocket * uwebsocket) { instance->webserverPlugin->WebsocketAccept(uwebsocket); };
    char * AppWebsocketPassword() override { return (char *)instance->appPwd(); };
    void AppWebsocketMessage(class json_io & msg, word base, const char * mt, const char * src) override;
    void AppWebsocketAppInfo(const char * app, class json_io & msg, word base) override;
    bool AppWebsocketConnectComplete(class json_io & msg, word info) override;
    void AppWebsocketClosed() override;

    void ResponseSent() override;

    void TryClose();

    const char* textNum;

    bool closed;
    bool closing;
    bool admin;

public:
    STTSession(class STT * instance);
    ~STTSession();

    bool CheckSession(class ITask * task);

    class STT * instance;
    class ITask * currentTask;
    void Close();
};

