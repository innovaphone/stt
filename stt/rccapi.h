/*-----------------------------------------------------------------------------------------------*/
/* rccapi.h                                                                                      */
/* copyright (c) innovaphone 2019                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

class RccApiSession;
class RccApi {
    class istd::list<RccApiSession> sessions;
    class IJsonApiConnection* connection;
public:
    RccApi(class IJsonApiConnection* connection);
    class JsonApi* CreateJsonApi(class STT* sttInstance);
};

class RccApiSession : public JsonApi, public istd::listElement<RccApiSession> {
    const char* Name() { return "RCC"; };
    void Message(class json_io& msg, word base, const char* mt, const char* src);
    void JsonApiConnectionClosed();
    void SendUUI(int call, const char* language);
    class RccApi* rccapi;
    class IJsonApiConnection* connection;
    int userId;
    long srcId;
    class STT* sttInstance;

public:
    RccApiSession(class RccApi* rccapi, class IJsonApiConnection* connection, class STT* sttInstance);
    int CallUser(const char* e164, const char* text_e164);
    void HangUp(int call);
    ~RccApiSession();

};