#include <iostream>
#include <string>
#include <unistd.h>
#include "serviceconnector.h"

using namespace std;
using namespace WPEFramework;

const char * app_name = "rnesample";
string callSignWithVersion= "org.rdk.RDKShell.1";

const char *  getDisplayEnv(JSONRPC::LinkType<Core::JSON::IElement> ** client_)
{
    JsonObject joParams;
    JsonObject joResult;
    uint32_t status;
    char *display="rnedisplay";
    JSONRPC::LinkType<Core::JSON::IElement> * client = *client_;

    // Set THUNDER_ACCESS environment variable to set IP and port for Thunder (WPEFramework) Application
//    WPEFramework::Core::SystemInfo::SetEnvironment("THUNDER_ACCESS", "127.0.0.1:9998");

    // Create instance of JSONRPC client and pass in the plugin callSign.
//    JSONRPC::LinkType<Core::JSON::IElement> * client = new JSONRPC::LinkType<Core::JSON::IElement> (callSignWithVersion.c_str(), nullptr, false);

    //Getter
    string jsonOutput;
    joParams.Set("callsign", callSignWithVersion.c_str()); 
    joParams.Set("client",app_name);
    joParams.Set("displayName",display);
    status = (client)->Invoke<JsonObject,JsonObject>(~0,"createDisplay", joParams, joResult);
    if ( status == WPEFramework::Core::ERROR_NONE)
    {
    	joResult.ToString(jsonOutput);
    	cout << "Creating display with rdkshell: status: " << (status == Core::ERROR_NONE? "Success": "Failure") << endl;
    }
    else{
    	cout<<"Create display failed"<<endl;
	display=NULL;
    }
 //   delete client;
    return display;
}

int requestFocusForApp(JSONRPC::LinkType<Core::JSON::IElement> ** client_, char * app)
{
    JsonObject joParams;
    JsonObject joResult;
    uint32_t status;
    int result = 1;
    JSONRPC::LinkType<Core::JSON::IElement> * client = *client_;

    // Set THUNDER_ACCESS environment variable to set IP and port for Thunder (WPEFramework) Application
//    WPEFramework::Core::SystemInfo::SetEnvironment("THUNDER_ACCESS", "127.0.0.1:9998");

    // Create instance of JSONRPC client and pass in the plugin callSign.
//    JSONRPC::LinkType<Core::JSON::IElement> * client = new JSONRPC::LinkType<Core::JSON::IElement> (callSignWithVersion.c_str(), nullptr, false);

    //Getter
    string jsonOutput;
    joParams.Set("callsign", callSignWithVersion.c_str()); 
    joParams.Set("client",app);
    status = client->Invoke<JsonObject,JsonObject>(~0,"moveToFront", joParams, joResult);
    if ( status == WPEFramework::Core::ERROR_NONE)
    {
    	joResult.ToString(jsonOutput);
    	cout << "Pushing app to front  with rdkshell: status: " << (status == Core::ERROR_NONE? "Success": "Failure") << endl;
        status = (client)->Invoke<JsonObject,JsonObject>(~0,"setFocus", joParams, joResult);
	if ( status == WPEFramework::Core::ERROR_NONE)
	{
	     cout << "Adding focus  with rdkshell: status: " << (status == Core::ERROR_NONE? "Success": "Failure") << endl;
	}
	else
	{
	    result=0;
	}
    }
    else{
	cout<<"Failed to bring app to focus"<<endl;
	result = 0;
    }
  //  delete client;
    return result;
}

void thunderEventHandler(const char * event, const JsonObject& params)
{
    string eventParams;
    params.ToString(eventParams);

    cout << "eventParams: " << eventParams << endl;
}

int registerForLifeEvents(JSONRPC::LinkType<Core::JSON::IElement> ** client_)
{
    uint32_t status;
    int result = 1;
    JSONRPC::LinkType<Core::JSON::IElement> * client = *client_;

//    // Set THUNDER_ACCESS environment variable to set IP and port for Thunder (WPEFramework) Application
//    WPEFramework::Core::SystemInfo::SetEnvironment("THUNDER_ACCESS", "127.0.0.1:9998");

    // Create instance of JSONRPC client and pass in the plugin callSign.
//    JSONRPC::LinkType<Core::JSON::IElement> * client = new JSONRPC::LinkType<Core::JSON::IElement> (callSignWithVersion.c_str(), nullptr, false);
    char * method="onLaunched";
    status = (client)->Subscribe<JsonObject>(~0, method,std::bind( &thunderEventHandler, method, std::placeholders::_1));
    cout << "Adding suspend listeners  with rdkshell: status: " << (status == Core::ERROR_NONE? "Success": "Failure") << endl;
    method = "onDestroyed";
    status = (client)->Subscribe<JsonObject>(~0, method,std::bind( &thunderEventHandler, method, std::placeholders::_1));
    cout << "Adding resume listeners  with rdkshell: status: " << (status == Core::ERROR_NONE? "Success": "Failure") << endl;

//    delete client;
    return result;

}
