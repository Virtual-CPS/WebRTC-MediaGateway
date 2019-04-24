//

#ifndef SCY_OpenCVVideoCapturer_H
#define SCY_OpenCVVideoCapturer_H


#include <iostream>
#include <string>


#include "./videocapture.h"

#include "webrtc/media/base/videocapturer.h"

// OpenCVVideoCapturer implements a simple cricket::VideoCapturer which
// gets decoded remote video frames from media channel.
// It's used as the remote video source's VideoCapturer so that the remote
// video can be used as a cricket::VideoCapturer and in that way a remote
// video stream can implement the MediaStreamSourceInterface.

namespace scy {
namespace wrtc {


class OpenCVVideoCapturer : public cricket::VideoCapturer
{
public:
    OpenCVVideoCapturer(int deviceId);
    virtual ~OpenCVVideoCapturer();

    // cricket::VideoCapturer implementation.
    virtual cricket::CaptureState
				Start(const cricket::VideoFormat& capture_format);
    virtual void Stop();
    virtual bool IsRunning();
    virtual bool GetPreferredFourccs(std::vector<uint32_t>* fourccs);
    virtual bool GetBestCaptureFormat(const cricket::VideoFormat& desired,
                                      cricket::VideoFormat* best_format);
    virtual bool IsScreencast() const;

private:
    void onFrameCaptured(void* sender, MatrixPacket& packet);

    VideoCapture::Ptr capture;
};


// class OpenCVVideoCapturerFactory : public cricket::VideoDeviceCapturerFactory
// {
// public:
//     OpenCVVideoCapturerFactory() {}
//     virtual ~OpenCVVideoCapturerFactory() {}
//
//     virtual cricket::VideoCapturer* Create(const cricket::Device& device) {
//
//         // XXX: WebRTC uses device name to instantiate the capture, which is
//         always 0.
//         return new OpenCVVideoCapturer(util::strtoi<int>(device.id));
//     }
// };


} } // namespace scy::wrtc


#endif
