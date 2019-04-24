// VideoOnDemandPeer.h

#ifndef SCY_WebRTC_VideoOnDemandPeer_H
#define SCY_WebRTC_VideoOnDemandPeer_H

#include "scy/av/mediacapture.h"
#include "scy/webrtc/multiplexmediacapturer.h"
#include "scy/webrtc/peer.h"

namespace scy {
namespace wrtc {

// Peer connection class for Streaming local media file.

class VideoOnDemandPeer : public Peer
{
public:
    /// Create the streaming peer connection.
    VideoOnDemandPeer(PeerManager* manager,
		PeerFactoryContext* context, const std::string& peerid,
        const std::string& token, const std::string& file, bool looping);
    virtual ~VideoOnDemandPeer();

	void createStreaming();
	void startStreaming();
	void stopStreaming();

	// inherited from PeerObserver
	virtual void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

protected:
    std::string _file;
	bool _looping;
	bool _streamingStarted;
    MultiplexMediaCapturer _capturer;
	wrtc::PeerFactoryContext _context;
};


} } // namespace scy::wrtc


#endif
