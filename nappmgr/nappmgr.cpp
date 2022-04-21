/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 ** This sample demonstrates how to activate, make a get request and deactivate an rdkservice.
 **/
#include <iostream>
#include <string>
#include <unistd.h>

#include <core/JSONRPC.h>
#include <protocols/JSONRPCLink.h>
#include <plugins/Service.h>
#include "helpers/serviceconnector.h"

using namespace std;
using namespace WPEFramework;
#define DEFAULT_CALL_TIMEOUT 10000 // 10 seconds
#define APPS_URL "http://127.0.0.1:50050/amlresidentapp/index.html"
WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *ctrl_obj;
JSONRPC::LinkType<Core::JSON::IElement> *rdkshell;
void initialize()
{
    // Set THUNDER_ACCESS environment variable to set IP and port for Thunder (WPEFramework) Application
    WPEFramework::Core::SystemInfo::SetEnvironment("THUNDER_ACCESS", "127.0.0.1:9998");
    ctrl_obj = new WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>("", "", false);
}

int plugin_lifecycle(const char *callsign, const char *event)
{
    uint32_t status;
    int result = 1;
    JsonObject joParams;
    JsonObject joResult;
    // Get thunder controller client

    Core::ProxyType<Core::JSONRPC::Message> response;
    string request = " {\"callsign\":\"";
    request.append(callsign);
    request.append("\"}");
    cout << "plugin_lifecycle " << request.c_str() << endl;
    status = ctrl_obj->Invoke(DEFAULT_CALL_TIMEOUT, event, request, response);
    if (status != Core::ERROR_NONE)
    {
        cout << " Failed to  " << event << " " << callsign << endl;
        result = 0;
    }
    cout << "plugin_lifecycle succeded" << endl;

    return result;
}
int setFocus(const char *client)
{
    uint32_t status;
    JsonObject joParams;
    JsonObject joResult;

    string jsonOutput;
    int result = 1;
    joParams.Set("client", client);

    status = rdkshell->Invoke<JsonObject, JsonObject>(DEFAULT_CALL_TIMEOUT, "setFocus", joParams, joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        joResult.ToString(jsonOutput);
        cout << "Focus  Resident UI : status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
        cout << "result: " << jsonOutput << endl;
        result = 0;
        return result;
    }
    cout << "Waiting before setting visiblity" << endl;
    // sleep(5);
    joParams.Set("visible", true);
    status = rdkshell->Invoke<JsonObject, JsonObject>(DEFAULT_CALL_TIMEOUT, "setVisibility", joParams, joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        joResult.ToString(jsonOutput);
        cout << "Visibility for Resident UI : status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
        cout << "result: " << jsonOutput << endl;
        result = 0;
        return result;
    }

    return result;
}
int showResidentUI(const char *url)
{
    uint32_t status;
    JsonObject joParams;
    JsonObject joResult;
    string jsonOutput;

    int result = 1;
    joParams.Set("callsign", "ResidentApp");
    joParams.Set("type", "ResidentApp");
    joParams.Set("uri", url);

    status = rdkshell->Invoke<JsonObject, JsonObject>(DEFAULT_CALL_TIMEOUT, "launch", joParams, joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        joResult.ToString(jsonOutput);
        cout << "Launch Resident UI : status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
        cout << "result: " << jsonOutput << endl;
        result = 0;
    }
    cout << "Resident App Launched." << endl;
    return result;
}
int showwLightningUI(const char *url)
{
    uint32_t status;
    JsonObject joParams;
    JsonObject joResult;
    string jsonOutput;

    int result = 1;
    joParams.Set("callsign", "LightningApp");
    joParams.Set("type", "LightningApp");
    joParams.Set("uri", url);

    status = rdkshell->Invoke<JsonObject, JsonObject>(DEFAULT_CALL_TIMEOUT, "launch", joParams, joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        joResult.ToString(jsonOutput);
        cout << "Launch Resident UI : status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
        cout << "result: " << jsonOutput << endl;
        result = 0;
    }
    cout << "Resident App Launched." << endl;
    return result;
}

int offloadApp(const char *appname)
{
    uint32_t status;
    JsonObject joParams;
    JsonObject joResult;
    string jsonOutput;

    int result = 1;
    joParams.Set("callsign", appname);
    joParams.Set("type", appname);

    status = rdkshell->Invoke<JsonObject, JsonObject>(DEFAULT_CALL_TIMEOUT, "destroy", joParams, joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        joResult.ToString(jsonOutput);
        cout << "offload  Resident UI : status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
        cout << "result: " << jsonOutput << endl;
        result = 0;
    }
    cout << "Resident App offloaded." << endl;
    return result;
}
int isPluginActive(const char *callsign)
{
    uint32_t status;
    int result = 1;
    Core::JSON::ArrayType<PluginHost::MetaData::Service> joResult;

    string request = "status@";
    request.append(callsign);
    cout << "plugin_status " << request.c_str() << endl;
    status = ctrl_obj->Get<Core::JSON::ArrayType<PluginHost::MetaData::Service>>(DEFAULT_CALL_TIMEOUT, request.c_str(), joResult);
    if (status != Core::ERROR_NONE)
    {
        cout << " Failed to get the status of " << callsign << endl;
        result = 0;
    }
    result = (joResult[0].JSONState == PluginHost::IShell::ACTIVATED);
    cout << "Is " << callsign << " active :" << result << endl;

    return result;
}
int addHotKeyListeners()
{
    JsonObject joParams;
    JsonObject joResult;
    uint32_t status;

    int result = 1;
    joParams.Set("client", "ResidentApp");
    joParams.Set("keyCode", 179);

    status = rdkshell->Invoke<JsonObject, JsonObject>(DEFAULT_CALL_TIMEOUT, "addKeyIntercept", joParams, joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        string jsonOutput;
        joResult.ToString(jsonOutput);
        cout << "Home Key Intercept : " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
        cout << "result: " << jsonOutput << endl;
        result = 0;
    }
    return result;
}
int launchCobalt()
{

    uint32_t status;
    JsonObject joParams;
    JsonObject joResult;
    string jsonOutput;

    int result = 1;
    joParams.Set("callsign", "Cobalt");
    joParams.Set("type", "Cobalt");

    status = rdkshell->Invoke<JsonObject, JsonObject>(DEFAULT_CALL_TIMEOUT, "launch", joParams, joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        joResult.ToString(jsonOutput);
        cout << "Launch Resident UI : status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
        cout << "result: " << jsonOutput << endl;
        result = 0;
    }
    cout << "Resident App Launched." << endl;
    return result;
}
bool isClientActive(char *callsign)
{
    uint32_t status;
    JsonObject joParams;
    JsonObject joResult;
    string jsonOutput;

    bool result = false;

    status = rdkshell->Invoke<JsonObject, JsonObject>(DEFAULT_CALL_TIMEOUT, "getClients", joParams, joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        joResult.ToString(jsonOutput);
        cout << "Launch Resident UI : status: " << (status == Core::ERROR_NONE ? "Success" : "Failure") << endl;
        cout << "result: " << jsonOutput << endl;
        string clients = joResult.Get("clients").String();
        if (clients.find(callsign) != std::string::npos)
        {
            result = true;
        }
        cout << "Residentapp is running ?" << result << endl;
    }
    return result;
}
int main(int argc, char **argv)
{
    JsonObject joParams;
    JsonObject joResult;
    uint32_t status;
    string callsign = "org.rdk.RDKShell";

    initialize();
    if (!isPluginActive(callsign.c_str()))
    {
        status = plugin_lifecycle(callsign.c_str(), "activate");
        if (!status)
        {
            cout << "Failed to activate plugin" << endl;
            delete ctrl_obj;
            return 0;
        }
        // Give few seconds for displaysettings plugin to get activated.
        sleep(5);
    }
    string callSignWithVersion = callsign + ".1";
    // Create instance of JSONRPC client and pass in the plugin callSign.
    rdkshell = new JSONRPC::LinkType<Core::JSON::IElement>(callSignWithVersion.c_str(), nullptr, false);
    getDisplayEnv(&rdkshell);
    registerForLifeEvents(&rdkshell);
    cout << "Launching HOME UI if not present." << endl;
    if (!isClientActive("residentapp"))
        showResidentUI(APPS_URL);
    addHotKeyListeners();
    setFocus("ResidentApp");

    cout << "Press any key when ready to launch Youtube application... ";
    cin.get();
    launchCobalt();
    setFocus("Cobalt");
    offloadApp("ResidentApp");
    cout << "Press any key when ready... to launch home UI back";
    cin.get();
    showResidentUI(APPS_URL);
    setFocus("ResidentApp");
    offloadApp("Cobalt");

    cout << "Press any key  when ready... to launch Lightning app";
    cin.get();

    showwLightningUI("https://widgets.metrological.com/lightning/rdk/d431ce8577be56e82630650bf701c57d#app:com.metrological.app.CNN");
    setFocus("LightningApp");
    offloadApp("ResidentApp");
    cout << "Press any number  when ready... to launch home UI back";
    cin.get();
    showResidentUI(APPS_URL);
    setFocus("ResidentApp");
    offloadApp("LightningApp");

    delete rdkshell;
    delete ctrl_obj;
    return 0;
}
