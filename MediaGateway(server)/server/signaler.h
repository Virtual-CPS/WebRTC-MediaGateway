// signaler.h

#ifndef WEBRTC_LABS_SIGNALER_H_
#define WEBRTC_LABS_SIGNALER_H_

#include "scy/application.h"
#include "scy/ipc.h"
#include "scy/net/sslmanager.h"
#include "scy/net/sslsocket.h"
#include "scy/symple/client.h"
#include "scy/util.h"
#include "scy/webrtc/peermanager.h"
#include "scy/webrtc/streamrecorder.h"
#include "scy/webrtc/multiplexmediacapturer.h"

//#include "scy/webrtc/recordingpeerconnection.h"

#include "config.h"


namespace scy {


class Signaler : public wrtc::PeerManager, public Application
{
public:
    Signaler(const smpl::Client::Options& options);
    ~Signaler();

protected:

    /// PeerConnectionManager interface
    void sendSDP(wrtc::Peer* conn, const std::string& mid, const std::string& sdp);
    void sendCandidate(wrtc::Peer* conn, const std::string& mid, int mlineindex, const std::string& sdp);
    void onAddRemoteStream(wrtc::Peer* conn, webrtc::MediaStreamInterface* stream);
    void onRemoveRemoteStream(wrtc::Peer* conn, webrtc::MediaStreamInterface* stream);
    void onStable(wrtc::Peer* conn);
    void onClosed(wrtc::Peer* conn);
    void onFailure(wrtc::Peer* conn, const std::string& error);

    void postMessage(const smpl::Message& m);
    void syncMessage(const ipc::Action& action);

    void onPeerConnected(smpl::Peer& peer);
    void onPeerCommand(smpl::Command& m);
    void onPeerEvent(smpl::Event& m);
    void onPeerMessage(smpl::Message& m);
    void onPeerDiconnected(const smpl::Peer& peer);

    void onClientStateChange(void* sender, sockio::ClientState& state, const sockio::ClientState& oldState);

    std::string getDataDirectory() const;

protected:
	ipc::SyncQueue<> _ipc;
#if USE_SSL
	smpl::SSLClient _client;
#else
	smpl::TCPClient _client;
#endif

	wrtc::PeerFactoryContext _context;
};


} // namespace scy


#endif	// WEBRTC_LABS_SIGNALER_H_