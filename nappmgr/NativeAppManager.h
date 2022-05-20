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

#include <iostream>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <core/JSONRPC.h>
#include <protocols/JSONRPCLink.h>
#include <plugins/Service.h>
using std::string;

using namespace WPEFramework;

class NativeAppManager
{
    // RDKShell remote connector
    WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>   m_remoteObject;
    // Variables for exiting on TERM signal.
    std::condition_variable m_act_cv;
    volatile bool m_isActive;
    std::mutex m_lock;

    // Handlles app manager state.
    bool m_isResAppRunning;
    // Handles launch to launch notification transistion.
    bool m_launchInitiated;
    // Holds the currently active app
    std::string m_activeApp;

    static NativeAppManager *_instance;
    static const char * resCallsign;
    const static string LAUNCH_URL;

    NativeAppManager();
    void handleTermSignal(int _sig);
    void handleLowMemoryEvent(const JsonObject &parameters);
    void handleKeyEvent(const JsonObject &parameters);
    void handleLaunchEvent(const JsonObject &parameters);
    void handleDestroyEvent(const JsonObject &parameters);

    bool isResidentAppRunning();

    static bool equalsIgnoreCase(const char *a, const char *b)
    {
        return (strncasecmp(a, b,strlen(a)) == 0);
    }

public:
    int initialize();

    int launchResidentApp();
    int offloadActiveApp(const char *callSign, bool suspend);
    void registerForEvents();
    int unRegisterForEvents();
    void waitForTermSignal();
    static NativeAppManager *getInstance();

    // no copying allowed
    NativeAppManager(const NativeAppManager &) = delete;
    NativeAppManager &operator=(const NativeAppManager &) = delete;
};