/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
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

#include "NativeAppManager.h"
#include <csignal>

#define SUBSCRIPTION_LOW_MEMORY_EVENT "onDeviceCriticallyLowRamWarning"
#define SUBSCRIPTION_ONKEY_EVENT "onKeyEvent"
#define SUBSCRIPTION_ONLAUNCHED_EVENT "onLaunched"
#define SUBSCRIPTION_ONDESTROYED_EVENT "onDestroyed"
#define SERVER_DETAILS "127.0.0.1:9998"
#define HOME_KEY 36
using namespace std;
using std::string;
using namespace WPEFramework;

const string NativeAppManager::LAUNCH_URL = "http://127.0.0.1:50050/lxresui/index.html#menu";
NativeAppManager *NativeAppManager::_instance = nullptr;
const char *NativeAppManager::resCallsign = "ResidentApp";

const int THUNDER_TIMEOUT = 2000; // milliseconds

void printResults(const JsonObject &joResult)
{
    string jsonOutput;
    joResult.ToString(jsonOutput);
    cerr << "[printResults] : " << jsonOutput << endl;
}

void NativeAppManager::handleTermSignal(int _signal)
{
    cerr << "Received term signal.." << endl;

    unique_lock<std::mutex> ulock(m_lock);
    m_isActive = false;
    unRegisterForEvents();
    m_act_cv.notify_one();
}

void NativeAppManager::waitForTermSignal()
{
    thread termThread([&, this]()
                      {
    while (m_isActive)
    {
        unique_lock<std::mutex> ulock(m_lock);
        m_act_cv.wait(ulock);
    }
    cerr << "Received term signal." <<endl; });
    termThread.join();
}
NativeAppManager::NativeAppManager() : m_remoteObject(_T("org.rdk.RDKShell.1")), m_isActive(false),
                                       m_isResAppRunning(false), m_launchInitiated(false)
{
    cerr << "[NativeAppManager]Constructor.. " << endl;
}
NativeAppManager *NativeAppManager::getInstance()
{
    if (nullptr == _instance)
    {
        _instance = new NativeAppManager();
    }
    return _instance;
}
int NativeAppManager::initialize()
{
    int status = false;
    // Remote reference of RDKShell instance
    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), _T(SERVER_DETAILS));

    {
        lock_guard<mutex> lkgd(m_lock);
        m_isActive = true;
    }
    signal(SIGTERM, [](int x)
           { NativeAppManager::getInstance()->handleTermSignal(x); });
    m_isResAppRunning = isResidentAppRunning();
    status = true;
    return status;
}
int NativeAppManager::launchResidentApp()
{
    uint32_t status;
    JsonObject joParams;
    JsonObject joResult;
    string jsonOutput;

    int result = 0;
    joParams.Set("callsign", resCallsign);
    joParams.Set("type", resCallsign);
    joParams.Set("uri", LAUNCH_URL.c_str());
    cerr << "Launching resident App \n";
    status = m_remoteObject.Invoke<JsonObject, JsonObject>(THUNDER_TIMEOUT, "launch", joParams, joResult);
    printResults(joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        m_launchInitiated = true;
        result = 1;
    }
    cerr << "Resident App Launched." << endl;
    return result;
}
int NativeAppManager::offloadActiveApp(const char *callSign, bool suspend)
{
    uint32_t status;
    JsonObject joParams;
    JsonObject joResult;

    int result = 0;

    cerr << "Offloading app: " << callSign << "Suspend ? " << suspend << endl;

    joParams.Set("callsign", callSign);

    status = m_remoteObject.Invoke<JsonObject, JsonObject>(THUNDER_TIMEOUT,
                                                           suspend ? "suspend" : "destroy", joParams, joResult);
    printResults(joResult);
    if (status == WPEFramework::Core::ERROR_NONE)
    {
        cerr << "Suspended  " << callSign << endl;
        result = 1;
    }
    return result;
}
void NativeAppManager::registerForEvents()
{
    uint32_t status = Core::ERROR_NONE;

    status = m_remoteObject.Subscribe<JsonObject>(THUNDER_TIMEOUT, _T(SUBSCRIPTION_LOW_MEMORY_EVENT), [&, this](const JsonObject &parameters)
                                                  { handleLowMemoryEvent(parameters); });
    cerr << " Subscribed to Low Memory events..  status " << (status == Core::ERROR_NONE ? "Success" : "Failed") << endl;

    status = m_remoteObject.Subscribe<JsonObject>(THUNDER_TIMEOUT, _T(SUBSCRIPTION_ONKEY_EVENT), [&,this ](const JsonObject &parameters)
                                                  { handleKeyEvent( parameters); });
    cerr << " Subscribed to onKey events..  status " << (status == Core::ERROR_NONE ? "Success" : "Failed") << endl;

    status = m_remoteObject.Subscribe<JsonObject>(THUNDER_TIMEOUT, _T(SUBSCRIPTION_ONLAUNCHED_EVENT), [&, this](const JsonObject &parameters)
                                                  { handleLaunchEvent(parameters);} );
    cerr << " Subscribed to onLaunched events..  status " << (status == Core::ERROR_NONE ? "Success" : "Failed") << endl;

    status = m_remoteObject.Subscribe<JsonObject>(THUNDER_TIMEOUT, _T(SUBSCRIPTION_ONDESTROYED_EVENT), [&, this ](const JsonObject &parameters)
                                                  { handleDestroyEvent(parameters); });
    cerr << " Subscribed to onDestroyed events..  status " << (status == Core::ERROR_NONE ? "Success" : "Failed") << endl;
}
int NativeAppManager::unRegisterForEvents()
{
    m_remoteObject.Unsubscribe(THUNDER_TIMEOUT, _T(SUBSCRIPTION_LOW_MEMORY_EVENT));
    m_remoteObject.Unsubscribe(THUNDER_TIMEOUT, _T(SUBSCRIPTION_ONKEY_EVENT));
    m_remoteObject.Unsubscribe(THUNDER_TIMEOUT, _T(SUBSCRIPTION_ONLAUNCHED_EVENT));
    m_remoteObject.Unsubscribe(THUNDER_TIMEOUT, _T(SUBSCRIPTION_ONDESTROYED_EVENT));
    return 0;
}

void NativeAppManager::handleLowMemoryEvent(const JsonObject &parameters)
{
    printResults(parameters);
    // if residentapp is running , let us offload them.
    if (m_isResAppRunning)
    {
        offloadActiveApp(resCallsign, false);
    }
}
void NativeAppManager::handleKeyEvent(const JsonObject &parameters)
{
    printResults(parameters);

    if (parameters.HasLabel("keycode"))
    {
        if (!parameters["keyDown"].Boolean() && parameters["keycode"].Number() == HOME_KEY)
        {
            cerr << "Active app is empty " << m_isResAppRunning << "::  " << m_launchInitiated << endl;
            cerr << "checking for active app :: \n";
            // Is there an active app ?
            if (!m_activeApp.empty())
            {
                cerr << "Offloading active app \n";
                // destroy active app.
                offloadActiveApp(m_activeApp.c_str(), false);
            }
            else
            {
                cerr << "Active app is empty " << m_isResAppRunning << "::  " << m_launchInitiated << endl;
                if (!m_isResAppRunning && !m_launchInitiated)
                {
                    cerr << "Hot key ... Launching resident app " << endl;
                    launchResidentApp();
                }
                else
                {
                    cerr << "Hot key ... Resident app status: " << m_isResAppRunning
                         << ", islaunching : " << m_launchInitiated << endl;
                }
            }
        }
    }
}
void NativeAppManager::handleLaunchEvent(const JsonObject &parameters)
{
    printResults(parameters);
    if (parameters.HasLabel("client"))
    {
        m_activeApp = parameters["client"].String();
        cerr << "[Jose] Launch notification  ... " << m_activeApp << endl;

        if (equalsIgnoreCase(m_activeApp.c_str(), resCallsign))
        {
            m_isResAppRunning = true;
            m_launchInitiated = false;
        }
    }
}

void NativeAppManager::handleDestroyEvent(const JsonObject &parameters)
{
    printResults(parameters);
    if (parameters.HasLabel("client"))
    {
        string destroyedApp = parameters["client"].String();
        bool isResApp = equalsIgnoreCase(destroyedApp.c_str(), resCallsign);
        cerr << "Found a match " << isResApp << endl;

        if (isResApp)
        {
            m_isResAppRunning = false;
            m_launchInitiated = false;
        }
        else if (!m_isResAppRunning && !m_launchInitiated)
        {
            launchResidentApp();
        }
    }
}
bool NativeAppManager::isResidentAppRunning()
{
    JsonObject req, res;
    uint32_t status = Core::ERROR_NONE;
    bool isRunning = false;

    JSONRPC::LinkType<Core::JSON::IElement> remoteObject("org.rdk.RDKShell.1");

    status = remoteObject.Invoke<JsonObject, JsonObject>(THUNDER_TIMEOUT, "getClients", req, res);
    printResults(res);

    if (Core::ERROR_NONE == status)
    {
        string clients = res["clients"].String();
        isRunning = (clients.find("residentapp") != std::string::npos);
    }
    return isRunning;
}