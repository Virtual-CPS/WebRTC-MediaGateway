// signaler.cpp

#include "signaler.h"

#include "scy/av/codec.h"
#include "scy/av/format.h"
#include "scy/filesystem.h"
#include "scy/util.h"
#include "scy/webrtc/recordingpeer.h"
#include "VideoOnDemandPeer.h"

#include <iostream>
#include <string>

using std::endl;


namespace scy {


Signaler::Signaler(const smpl::Client::Options& options)
    : _client(options)
{
	// Setup the signalling client
	_client.StateChange += slot(this, &Signaler::onClientStateChange);
    _client.roster().ItemAdded += slot(this, &Signaler::onPeerConnected);
    _client.roster().ItemRemoved += slot(this, &Signaler::onPeerDiconnected);
    _client += packetSlot(this, &Signaler::onPeerCommand);
    _client += packetSlot(this, &Signaler::onPeerEvent);
    _client += packetSlot(this, &Signaler::onPeerMessage);
    _client.connect();
}

Signaler::~Signaler()
{
}

void Signaler::sendSDP(wrtc::Peer* conn, const std::string& type,
                       const std::string& sdp)
{
	assert(type == "offer" || type == "answer");
	smpl::Event e;
	e.setName("ice:sdp");
	auto& desc = e["sdp"];
	desc[wrtc::kSessionDescriptionTypeName] = type;
	desc[wrtc::kSessionDescriptionSdpName] = sdp;

	postMessage(e);
}

void Signaler::sendCandidate(wrtc::Peer* conn, const std::string& mid,
                             int mlineindex, const std::string& sdp)
{
	smpl::Event e;
	e.setName("ice:candidate");
	auto& desc = e["candidate"];
	desc[wrtc::kCandidateSdpMidName] = mid;
	desc[wrtc::kCandidateSdpMlineIndexName] = mlineindex;
	desc[wrtc::kCandidateSdpName] = sdp;

	postMessage(e);
}

void Signaler::onPeerConnected(smpl::Peer& peer)
{
	if (peer.id() == _client.ourID())
		return;
	LDebug("Peer connected: ", peer.id())

	if (wrtc::PeerManager::exists(peer.id())) {
		LDebug("Peer already has session: ", peer.id())
			return;
	}
}

void Signaler::onPeerCommand(smpl::Command& c)
{
	LDebug("Peer command: ", c.from().toString())

    // List available files for streaming
    if (c.node() == "streaming:files")
	{
        json::value files;
        StringVec nodes;
        fs::readdir(getDataDirectory(), nodes);
        for (auto node : nodes)
		{
            //files.append(node);
			std::string str = node;
			files.push_back(str);
        }

        c.setData("files", files);
        c.setStatus(200);
        _client.respond(c);
    }

    // Start a streaming session
    else if (c.node() == "streaming:start")
	{
        //std::string file = c.data("file").asString();
		std::string file = c.data("file");
		std::string filePath(getDataDirectory());
        fs::addnode(filePath, file);
		//filePath = "RTSP://192.168.0.20";
		filePath = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov";
		std::string peerId = c.from().id;
		std::string messageId = c.id();
		auto conn = new wrtc::VideoOnDemandPeer(this, &_context, peerId, c.id(), filePath, true);
		//auto conn = new wrtc::VideoOnDemandPeer(this, &_context, peerId, "", filePath, true);
		conn->createStreaming();

		// Send the Offer SDP
		conn->createConnection();
        conn->createOffer();

		wrtc::PeerManager::add(peerId, conn);

        c.setStatus(200);
        _client.respond(c);
	}

	// Stop a streaming session
	else if (c.node() == "streaming:stop")
	{
		std::string peerId = c.from().id;	// should before _client.respond()

		c.setStatus(200);
		_client.respond(c);					// swap c's from and to

		auto conn = wrtc::PeerManager::get(peerId, false);
		if (conn)
		{
			((wrtc::VideoOnDemandPeer *)conn)->stopStreaming();
			//conn->closeConnection();
			wrtc::PeerManager::remove(peerId);
		}
	}

    // Start a recording session
    else if (c.node() == "recording:start")
	{
        av::EncoderOptions options;
        options.ofile = OUTPUT_FILENAME;
        options.oformat = OUTPUT_FORMAT;
		std::string peerId = c.from().id;
		std::string messageId = c.id();

        //auto conn = new wrtc::RecordingPeerConnection(this, c.from().id, c.id(), options);
		auto conn = new wrtc::RecordingPeer(this, &_context, peerId, c.id(), options);
		conn->constraints().SetMandatoryReceiveVideo(true);
        conn->constraints().SetMandatoryReceiveAudio(true);
        conn->createConnection();
        wrtc::PeerManager::add(peerId, conn);

        c.setStatus(200);
        _client.respond(c);
        // _client.persistence().add(c.id(), reinterpret_cast<smpl::Message *>(c.clone()), 0);
    }
}

void Signaler::onPeerEvent(smpl::Event& e)
{
	LDebug("Peer message: ", e.from().toString())

   if (e.name() == "ice:sdp")
	{
        recvSDP(e.from().id, e["sdp"]);
    }
    else if (e.name() == "ice:candidate")
	{
        recvCandidate(e.from().id, e["candidate"]);
    }
}

void Signaler::onPeerMessage(smpl::Message& m)
{
	LDebug("Peer message: ", m.from().toString())
/*
	if (m.find("offer") != m.end()) {
		recvSDP(m.from().id, m["offer"]);
	}
	else if (m.find("answer") != m.end()) {
		assert(0 && "answer not supported");
	}
	else if (m.find("candidate") != m.end()) {
		recvCandidate(m.from().id, m["candidate"]);
	}
	// else assert(0 && "unknown event");
*/
}

void Signaler::onPeerDiconnected(const smpl::Peer& peer)
{
	LDebug("Peer disconnected")

    // TODO: Loop all and close for peer
/*
	auto conn = get(peer.id());
	if (conn) {
		LDebug("Closing peer connection: ", peer.id())
			conn->closeConnection(); // will be deleted via callback
	}
*/}

void Signaler::onClientStateChange(void* sender, sockio::ClientState& state,
                                   const sockio::ClientState& oldState)
{
	SDebug << "Client state changed from " << oldState << " to " << state << endl;

	switch (state.id())
	{
	case sockio::ClientState::Closed:
			break;
	case sockio::ClientState::Connecting:
			break;
		case sockio::ClientState::Connected:
			break;
		case sockio::ClientState::Online:
			break;
		case sockio::ClientState::Error:
			throw std::runtime_error("Cannot connect to Symple server. "
									"Did you start the demo app and the "
									"Symple server is running on port 4551?");
	}
}

void Signaler::onAddRemoteStream(wrtc::Peer* conn, webrtc::MediaStreamInterface* stream)
{
}

void Signaler::onRemoveRemoteStream(wrtc::Peer* conn, webrtc::MediaStreamInterface* stream)
{
    assert(0 && "free streams");
}

void Signaler::onStable(wrtc::Peer* conn)
{
}

void Signaler::onClosed(wrtc::Peer* conn)
{
	// _recorder.reset(); // shutdown the recorder
    wrtc::PeerManager::onClosed(conn);
}

void Signaler::onFailure(wrtc::Peer* conn, const std::string& error)
{
	// _recorder.reset(); // shutdown the recorder
    wrtc::PeerManager::onFailure(conn, error);
}

void Signaler::postMessage(const smpl::Message& m)
{
    _ipc.push(new ipc::Action(
        std::bind(&Signaler::syncMessage, this, std::placeholders::_1),
        m.clone()));
}

void Signaler::syncMessage(const ipc::Action& action)
{
    auto m = reinterpret_cast<smpl::Message*>(action.arg);
    _client.send(*m);
    delete m;
}

std::string Signaler::getDataDirectory() const
{
    // TODO: Make configurable
    std::string dir(getCwd());
    fs::addnode(dir, "data");
    return dir;
}


} // namespace scy
