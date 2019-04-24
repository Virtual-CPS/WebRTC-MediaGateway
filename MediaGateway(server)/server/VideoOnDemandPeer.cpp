// VideoOnDemandPeer.cpp

#include "VideoOnDemandPeer.h"
#include "scy/logger.h"
#include "scy/webrtc/peermanager.h"
#include "scy/webrtc/videopacketsource.h"

using std::endl;

namespace scy {
namespace wrtc {

VideoOnDemandPeer::VideoOnDemandPeer(PeerManager* manager,
			PeerFactoryContext* context, const std::string& peerid,
            const std::string& token, const std::string& file, bool looping)
	: _file(file)
	, _looping(looping)
	, _capturer()
	, _context(_capturer.getAudioModule())
	, Peer(manager, &_context, peerid, token, Peer::Offer)
	, _streamingStarted(false)
{
    // https://bugs.chromium.org/p/webrtc/issues/detail?id=2388
    _constraints.SetMandatoryReceiveAudio(false);
    _constraints.SetMandatoryReceiveVideo(false);
    _constraints.SetAllowDtlsSctpDataChannels(); // allow cross-browser
}

VideoOnDemandPeer::~VideoOnDemandPeer()
{
}

//rtc::scoped_refptr<webrtc::MediaStreamInterface> VideoOnDemandPeer::createMediaStream()
//{
//    assert(_mode == Offer);
//    assert(_factory);
//    assert(!_stream);
//    // assert(!_capture);
//    _stream = _factory->CreateLocalMediaStream(kStreamLabel);
//
//    _capturer.openFile(_file);
//    _capturer.addMediaTracks(_factory, _stream);
//
//    return _stream;
//}

// void VideoOnDemandPeer::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
// {
//     switch(new_state) {
//     case webrtc::PeerConnectionInterface::kStable:
//         break;
//     case webrtc::PeerConnectionInterface::kClosed:
//         _capture->stop();
//         break;
//     case webrtc::PeerConnectionInterface::kHaveLocalOffer:
//     case webrtc::PeerConnectionInterface::kHaveRemoteOffer:
//         break;
//     case webrtc::PeerConnectionInterface::kHaveLocalPrAnswer:
//     case webrtc::PeerConnectionInterface::kHaveRemotePrAnswer:
//         _capture->start();
//         break;
//     }
//
//     PeerConnection::OnSignalingChange(new_state);
// }

void VideoOnDemandPeer::createStreaming()
{
	if (_streamingStarted)
		return;

	_capturer.openFile(_file, _looping);

	// Create the media stream and attach decoder
	// output to the peer connection
	_capturer.addMediaTracks(_context.factory, createMediaStream());
}

void VideoOnDemandPeer::startStreaming()
{
	if (_streamingStarted)
		return;

	_capturer.start();
	_streamingStarted = true;
}

void VideoOnDemandPeer::stopStreaming()
{
	if (!_streamingStarted)
		return;

	_capturer.stop();
	_streamingStarted = false;
}

void VideoOnDemandPeer::OnIceConnectionChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state)
{
    LDebug(_peerid, ": On ICE gathering change: ", new_state)

    switch (new_state) {
        case webrtc::PeerConnectionInterface::kIceConnectionNew:
        case webrtc::PeerConnectionInterface::kIceConnectionChecking:
        case webrtc::PeerConnectionInterface::kIceConnectionConnected:
			//_capturer.openFile(_file, _looping);
			//createStreaming();
			break;
        case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
            //_capturer.start();
			startStreaming();
            break;
        case webrtc::PeerConnectionInterface::kIceConnectionFailed:
        case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
        case webrtc::PeerConnectionInterface::kIceConnectionClosed:
        case webrtc::PeerConnectionInterface::kIceConnectionMax:
            //_capturer.stop();
			stopStreaming();
            break;
    }

	Peer::OnIceConnectionChange(new_state);
}

// void PeerConnection::createPeer()
// {
//     assert(_factory);
//
//     _peerPeer = _factory->CreatePeerConnection(
//         _config, &_constraints, nullptr, nullptr, this);
//
//     if (_stream) {
//         if (!_peerPeer->AddStream(_stream)) {
//             throw std::runtime_error("Adding stream to PeerConnection
//             failed");
//         }
//     }
// }
//
// void PeerConnection::closePeer()
// {
//     LDebug(_peerid, ": Closing")
//
//     if (_peerPeer) {
//         _peerPeer->Close();
//     }
//     else {
//         // Call onClosed if no connection has been
//         // made so callbacks are always run.
//         _manager->onClosed(this);
//     }
// }
//
// void PeerConnection::createOffer()
// {
//     assert(_mode == Offer);
//     assert(_peerPeer);
//
//     _peerPeer->CreateOffer(this, &_constraints);
// }
//
// void PeerConnection::recvSDP(const std::string& type, const std::string& sdp)
// {
//     LDebug(_peerid, ": Receive ", type, ": ", sdp)
//
//     webrtc::SdpParseError error;
//     webrtc::SessionDescriptionInterface*
//     desc(webrtc::CreateSessionDescription(type, sdp, &error));
//     if (!desc) {
//         throw std::runtime_error("Can't parse remote SDP: " +
//         error.description);
//     }
//     _peerPeer->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(),
//     desc);
//
//     if (type == "offer") {
//         assert(_mode == Answer);
//         _peerPeer->CreateAnswer(this, &_constraints);
//     }
//     else {
//         assert(_mode == Offer);
//     }
// }
//
// void PeerConnection::recvCandidate(const std::string& mid, int mlineindex,
// const std::string& sdp)
// {
//     webrtc::SdpParseError error;
//     std::unique_ptr<webrtc::IceCandidateInterface>
//     candidate(webrtc::CreateIceCandidate(mid, mlineindex, sdp, &error));
//     if (!candidate) {
//         throw std::runtime_error("Can't parse remote candidate: " +
//         error.description);
//     }
//     _peerPeer->AddIceCandidate(candidate.get());
// }
//
// void
// PeerConnection::OnIcePeerChange(webrtc::PeerConnectionInterface::IcePeerState
// new_state)
// {
//     LDebug(_peerid, ": On ICE connection change: ", new_state)
// }
//
// void
// PeerConnection::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState
// new_state)
// {
//     LDebug(_peerid, ": On ICE gathering change: ", new_state)
// }
//
// void PeerConnection::OnRenegotiationNeeded()
// {
//     LDebug(_peerid, ": On renegotiation needed")
// }
//
// void PeerConnection::OnAddStream(webrtc::MediaStreamInterface* stream)
// {
//     assert(_mode == Answer);
//
//     LDebug(_peerid, ": On add stream")
//     _manager->onAddRemoteStream(this, stream);
// }
//
// void PeerConnection::OnRemoveStream(webrtc::MediaStreamInterface* stream)
// {
//     assert(_mode == Answer);
//
//     LDebug(_peerid, ": On remove stream")
//     _manager->onRemoveRemoteStream(this, stream);
// }
//
// void PeerConnection::OnIceCandidate(const webrtc::IceCandidateInterface*
// candidate)
// {
//     std::string sdp;
//     if (!candidate->ToString(&sdp)) {
//         LError(_peerid, ": Failed to serialize candidate")
//         assert(0);
//         return;
//     }
//
//     _manager->sendCandidate(this, candidate->sdp_mid(),
//     candidate->sdp_mline_index(), sdp);
// }
//
// void PeerConnection::OnSuccess(webrtc::SessionDescriptionInterface* desc)
// {
//     LDebug(_peerid, ": Set local description")
//     _peerPeer->SetLocalDescription(
//         DummySetSessionDescriptionObserver::Create(), desc);
//
//     // Send an SDP offer to the peer
//     std::string sdp;
//     if (!desc->ToString(&sdp)) {
//         LError(_peerid, ": Failed to serialize local sdp")
//         assert(0);
//         return;
//     }
//
//     _manager->sendSDP(this, desc->type(), sdp);
// }
//
// void PeerConnection::OnFailure(const std::string& error)
// {
//     LError << _peerid(": On failure: ", error)
//
//     _manager->onFailure(this, error);
// }
//
// void
// PeerConnection::setPeerConnectionFactory(rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
// factory)
// {
//     assert(!_factory); // should not be already set via PeerConnectionManager
//     _factory = factory;
// }
//
// std::string PeerConnection::peerid() const
// {
//     return _peerid;
// }
//
// webrtc::FakeConstraints& PeerConnection::constraints()
// {
//     return _constraints;
// }
//
// webrtc::PeerConnectionFactoryInterface* PeerConnection::factory() const
// {
//     return _factory.get();
// }
//
// rtc::scoped_refptr<webrtc::PeerConnectionInterface>
// PeerConnection::peerPeer() const
// {
//     return _peerPeer;
// }
//
// rtc::scoped_refptr<webrtc::MediaStreamInterface> PeerConnection::stream()
// const
// {
//     return _stream;
// }
//
// // Dummy Set Session Description Observer
//
//
// void DummySetSessionDescriptionObserver::OnSuccess()
// {
//     LDebug("On SDP parse success")
// }
//
// void DummySetSessionDescriptionObserver::OnFailure(const std::string& error)
// {
//     LError("On SDP parse error: ", error)
//     assert(0);
// }

} } // namespace scy::wrtc
