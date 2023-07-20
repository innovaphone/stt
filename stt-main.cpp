
/*-----------------------------------------------------------------------------------------------*/
/* Based on innovaphone App template                                                             */
/* copyright (c) innovaphone 2016                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

#include "platform/platform.h"
#include "common/build/release.h"
#include "common/os/iomux.h"
#include "common/interface/task.h"
#include "common/interface/socket.h"
#include "common/interface/webserver_plugin.h"
#include "common/interface/database.h"
#include "common/interface/json_api.h"
#include "common/interface/random.h"
#include "common/interface/translation.h"
#include "common/lib/appservice.h"
#include "common/lib/config.h"
#include "common/lib/tasks_postgresql.h"
#include "common/lib/appwebsocket.h"
#include "common/lib/app_updates.h"
#include "stt/stt.h"

extern "C" const char* __asan_default_options() {
    return "log_path=/var/log/core_dumps/stt/stt.asan:detect_stack_use_after_return=1:strict_string_checks=1:verbosity=1:"
        "symbolize_vs_style=true:halt_on_error=false:start_deactivated=true:print_cmdline=true:detect_leaks=false:log_exe_name=1:log_to_syslog=false:"
        "fast_unwind_on_malloc=true:malloc_context_size=30:quarantine_size_mb=512";
}

extern "C" void __sanitizer_report_error_summary(const char* error_summary)
{
    printf("ERROR: Address Sanatizer caught an error - please check the log under /var/log/core_dumps/stt/stt.asan.*\n%s", error_summary);
}


int main(int argc, char *argv[])
{
    IRandom::Init(time(nullptr));
    class IIoMux * iomux = IIoMux::Create();
	ISocketProvider * localSocketProvider = CreateLocalSocketProvider();
    IWebserverPluginProvider * webserverPluginProvider = CreateWebserverPluginProvider();
    IDatabaseProvider * databaseProvider = CreatePostgreSQLDatabaseProvider();
    ISocketProvider* tcpSocketProvider = CreateTCPSocketProvider();
    ISocketProvider* tlsSocketProvider = CreateTLSSocketProvider(tcpSocketProvider);

    AppServiceArgs  serviceArgs;
    serviceArgs.serviceID = "stt";
    serviceArgs.Parse(argc, argv);
    AppInstanceArgs instanceArgs;
    instanceArgs.appName = "stt";
    instanceArgs.appDomain = "example.com";
    instanceArgs.appPassword = "pwd";
    instanceArgs.webserver = "/var/run/webserver/webserver";
    instanceArgs.webserverPath = "/stt";
    instanceArgs.dbHost = "";
    instanceArgs.dbName = "stt";
    instanceArgs.dbUser = "stt";
    instanceArgs.dbPassword = "stt";
    instanceArgs.Parse(argc, argv);
	
    STTService * service = new STTService(iomux, localSocketProvider, tcpSocketProvider, tlsSocketProvider, webserverPluginProvider, databaseProvider, &serviceArgs);
    if (!serviceArgs.manager) service->AppStart(&instanceArgs);
    iomux->Run();

    delete service;
	delete localSocketProvider;
    delete webserverPluginProvider;
    delete databaseProvider;
    delete tcpSocketProvider;
    delete tlsSocketProvider;
    delete iomux;
    delete debug;
    
    return 0;
}
