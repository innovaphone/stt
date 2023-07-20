
/*-----------------------------------------------------------------------------------------------*/
/* Based on innovaphone App template                                                             */
/* copyright (c) innovaphone 2018                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

class TaskDbInit : public ITask, public UTask {
    class TaskPostgreSQLInitTable initUsers;
    class TaskPostgreSQLInitTable initCounter;
    class TaskPostgreSQLInitTable initConfig;

    void TaskComplete(class ITask * const task) override;
    void TaskFailed(class ITask * const task) override;

public:
    TaskDbInit(IDatabase * database);
    virtual ~TaskDbInit();

    void Start(class UTask * user) override;
};

class TaskReadWriteCount : public ITask, public UDatabase {
    void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;
    void WriteCount();
    void DatabaseInsertSQLResult(IDatabase * const database, ulong64 id) override;
    void DatabaseError(IDatabase * const database, db_error_t error) override;

    class IDatabase * database;
    const char * sip;
    const char * dn;
    ulong64 id;
    long64 count;
    bool update;
public:
    TaskReadWriteCount(IDatabase * database, const char * sip, const char * dn, long64 count, bool update);
    ~TaskReadWriteCount();

    void Start(class UTask * user) override;
    ulong64 GetId() { return id; };
    long64 GetCount() { return count; };
};

class TaskReadUserCount : public ITask, public UDatabase {
    void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;
    void DatabaseError(IDatabase * const database, db_error_t error) override;

    class IDatabase * database;
    const char * sip;
    long64 count;
public:
    TaskReadUserCount(IDatabase * database, const char * sip);
    ~TaskReadUserCount();

    void Start(class UTask * user) override;
    long64 GetCount() { return count; };
};

class TaskReadWriteConfig : public ITask, public UDatabase {
    void DatabaseExecSQLResult(IDatabase* const database, class IDataSet* dataset) override;
    void DatabaseInsertSQLResult(IDatabase* const database, ulong64 id) override;
    void DatabaseError(IDatabase* const database, db_error_t error) override;

    class IDatabase* database;
    const char* name;
    const char* value;
    bool update;
public:
    TaskReadWriteConfig(IDatabase* database, const char* name, const char* value, bool update);
    ~TaskReadWriteConfig();
    const char* getValue() { return value; };

    void Start(class UTask* user) override;
};
