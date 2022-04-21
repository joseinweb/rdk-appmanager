#include <core/JSONRPC.h>
#include <protocols/JSONRPCLink.h>
#include <plugins/Service.h>
using namespace WPEFramework;

const char *  getDisplayEnv(JSONRPC::LinkType<Core::JSON::IElement> **);
int requestFocusForApp(JSONRPC::LinkType<Core::JSON::IElement> ** , char *);
int registerForLifeEvents(JSONRPC::LinkType<Core::JSON::IElement> **);

