#include <iostream>
#include <string>
#include <unistd.h>
#include "serviceconnector.h"

using namespace std;
using namespace WPEFramework;

const char *app_name = "rnesample";
string callSignWithVersion = "org.rdk.RDKShell.1";

const char *getDisplayEnv(JSONRPC::LinkType<Core::JSON::IElement> **client_)
{
    JsonObject joParams;
    JsonObject joResult;
    uint32_t status;
    const char *display = "rnedisplay";
    JSONRPC::LinkType<Core::JSON::IElement> *client = *client_;

    string jsonOutput;
    joParams.Set("callsign", callSignWithVersion.c_str());
    joParams.Set("client", app_name);
    joParams.Set("displayName", display);
    status = (client)->Invoke<JsonObject, JsonObject>(~0, "createDisplay", joParams, joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        joResult.ToString(jsonOutput);
        cout << "Creating display with rdkshell: status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
    }
    else
    {
        cout << "Create display failed" << endl;
        display = NULL;
    }
    return display;
}

int requestFocusForApp(JSONRPC::LinkType<Core::JSON::IElement> **client_, char *app)
{
    JsonObject joParams;
    JsonObject joResult;
    uint32_t status;
    int result = 1;
    JSONRPC::LinkType<Core::JSON::IElement> *client = *client_;

    string jsonOutput;
    joParams.Set("callsign", callSignWithVersion.c_str());
    joParams.Set("client", app);
    status = client->Invoke<JsonObject, JsonObject>(~0, "moveToFront", joParams, joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        joResult.ToString(jsonOutput);
        cout << "Pushing app to front  with rdkshell: status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
        status = (client)->Invoke<JsonObject, JsonObject>(~0, "setFocus", joParams, joResult);
        if (status == WPEFramework::Core::ERROR_NONE)
        {
            cout << "Adding focus  with rdkshell: status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
        }
        else
        {
            result = 0;
        }
    }
    else
    {
        cout << "Failed to bring app to focus" << endl;
        result = 0;
    }
    return result;
}

void thunderEventHandler(const char *event, const JsonObject &params)
{
    string eventParams;
    params.ToString(eventParams);

    cout << "eventParams: " << eventParams << endl;
}

void registerForLifeEvents(JSONRPC::LinkType<Core::JSON::IElement> **client_)
{
    uint32_t status;
    JSONRPC::LinkType<Core::JSON::IElement> *client = *client_;

    string method = "onLaunched";
    status = (client)->Subscribe<JsonObject>(~0, method, std::bind(&thunderEventHandler, method.c_str(), std::placeholders::_1));
    cout << "Adding launched listeners  with rdkshell: status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;

    method = "onDestroyed";
    status = (client)->Subscribe<JsonObject>(~0, method, std::bind(&thunderEventHandler, method.c_str(), std::placeholders::_1));
    cout << "Adding destroyed listeners  with rdkshell: status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;

    method = "onKeyEvent";
    status = (client)->Subscribe<JsonObject>(~0, method, std::bind(&thunderEventHandler, method.c_str(), std::placeholders::_1));
    cout << "Adding onKeyEvent listeners  with rdkshell: status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
}

void removeListeners(JSONRPC::LinkType<Core::JSON::IElement> *client)
{

    string method = "onLaunched";
    client->Unsubscribe(~0, method.c_str());

    method = "onDestroyed";
    client->Unsubscribe(~0, method.c_str());

    method = "onKeyEvent";
    client->Unsubscribe(~0, method.c_str());
}