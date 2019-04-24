///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <http://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
///


#include "scy/idler.h"
#include "scy/logger.h"
#include "signaler.h"

#include "webrtc/base/ssladapter.h"
#include "webrtc/base/thread.h"


using std::endl;
using namespace scy;


// Test this demo with the code in the `client` directory


int main(int argc, char** argv)
{
	Logger::instance().add(new ConsoleChannel("debug", Level::Trace));
	// Logger::instance().setWriter(new AsyncLogWriter);

//#if USE_SSL
	// net::SSLManager::initNoVerifyClient();
//#endif

    // Setup WebRTC environment
	rtc::LogMessage::LogToDebug(rtc::LS_INFO); // LS_VERBOSE, LS_INFO, LERROR
	// rtc::LogMessage::LogTimestamps();
	// rtc::LogMessage::LogThreads();

	rtc::InitializeSSL();

    {
        smpl::Client::Options options;
        options.host = SERVER_HOST;
        options.port = SERVER_PORT;
        options.name = "Media Server";
        options.user = "mediaserver";
        options.type = "mediaserver";

        // NOTE: The server must enable anonymous authentication for this demo.
        // options.token = ""; token based authentication

        Signaler app(options);

/*        auto rtcthread = rtc::Thread::Current();
        Idler rtc([=]() {
            // TraceL << "Running WebRTC loop" << endl;
            rtcthread->ProcessMessages(10);
        });
*/
		// Process WebRTC threads on the main loop.
		auto rtcthread = rtc::Thread::Current();
		Idler rtc([rtcthread]() {
			rtcthread->ProcessMessages(3);
		});

        app.waitForShutdown();
    }

//#if USE_SSL
    //net::SSLManager::destroy();
//#endif
    rtc::CleanupSSL();
    Logger::destroy();

    return 0;
}
