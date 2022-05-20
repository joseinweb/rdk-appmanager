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

#include <iostream>
#include "NativeAppManager.h"
using namespace std;

#define SERVER_DETAILS "127.0.0.1:9998"

static const char * VERSION = "1.4";

int main(int argc, char * argv[])
{
    cout <<"Native App Manager : "<<VERSION<<endl;
    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), _T(SERVER_DETAILS));
    NativeAppManager *nappmgr = NativeAppManager::getInstance();
    
    nappmgr->initialize();
    nappmgr->registerForEvents();
    nappmgr->waitForTermSignal();
    return 0;
}