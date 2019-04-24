// Define the `MediaGatewayApp` module
var MediaGatewayApp = angular.module('MediaGatewayApp', []);

// Define the `MediaGatewayAppController` controller on the `MediaGatewayApp` module
MediaGatewayApp.controller('MediaGatewayAppController', function MediaGatewayAppController($scope) {

    //
    //= Variables =============================================================

    // Client
    $scope.client;
    $scope.handle;
    $scope.peers = [];

    // Interface
    $scope.hideLogo = false;
    $scope.hideComments = true;
    $scope.errorText = "";
    $scope.isLoading = false;

    // Messaging
    $scope.directUser;
    $scope.messages = [];
    $scope.messageText = "";

    // Video Chat
    $scope.localPlayer;
    $scope.remotePlayer;
    $scope.remoteVideoPeer;
    $scope.disableLocalAudio = false;  // set true to prevent local feedback loop

    // Media Server
    $scope.mediaServerPeer;
    $scope.streamableFiles = [];       // list of streamable files that can be played
    $scope.currentStreamingFile;       // current file being streamed
    $scope.currentRecordingPeers = []; // list of user streams being recorded


    //
    //= Initialize the Symple client ==========================================

    $scope.client = new Symple.Client(CLIENT_OPTIONS);

    $scope.client.on('announce', function(peer) {
        console.log('Authentication success:', peer)

        $scope.client.join('public'); // join the public room
        $scope.isLoading = false;
        $scope.$apply();
    });

    $scope.client.on('presence', function(p) {
        console.log('Recv presence:', p)
    });

    $scope.client.on('message', function(m) {
        console.log('Recv message:', m)

        // Normal Message
        if (!m.direct || m.direct == $scope.handle) {
            //$scope.messages.push({
            $scope.messages.unshift({
                user: m.from.user,
                data: m.data,
                to: m.to,
                direct: m.direct,
                time: Symple.formatTime(new Date)
            });
            $scope.$apply();
        }
        else {
            console.log('dropping message:', m, m.direct)
        }
    });

    $scope.client.on('command', function(c) {
        console.log('Recv command:', c)

        if (c.node == 'call:init') {

            // Receive a call request
            if (!c.status) {

                // Show a dialog asking the user if they want to accept the call
                var e = $('#incoming-call-modal');
                e.find('.caller').text('@' + c.from.user);
                e.find('.accept').unbind('click').click(function() {
                    c.status = 200;
                    $scope.remoteVideoPeer = c.from;
                    $scope.client.respond(c);
                    $scope.$apply();
                    e.modal('hide');
                })
                e.find('.reject').unbind('click').click(function() {
                    c.status = 500;
                    $scope.client.respond(c);
                    e.modal('hide');
                })
                e.modal('show');
            }

            // Handle call accepted
            else if (c.status == 200) {
                $scope.remoteVideoPeer = c.from;
                $scope.startLocalVideo();
                $scope.$apply();
            }

            // Handle call rejected or failure
            else if (c.status == 500) {
                alert('Call failed');
            }
            else {
                alert('Unknown response status');
            }
        }

        else if (c.node == 'streaming:start') {
            // Handle streaming start response
            if (c.status == 200) {
                $scope.remoteVideoPeer = c.from;
                $scope.$apply();
            }
        }

        else if (c.node == 'streaming:files') {
            // Handle streaming filed response
            if (c.status == 200) {
                $scope.streamableFiles = c.data.files;
                $scope.$apply();
            }
        }
    });

    $scope.client.on('event', function(e) {
        console.log('Recv event:', e)

        // Only handle events from the remoteVideoPeer
        if (!$scope.remoteVideoPeer || $scope.remoteVideoPeer.id != e.from.id) {
            console.log('Dropping mismatch event:', e.from, $scope.remoteVideoPeer)
            return
        }

        // ICE SDP
        if (e.name == 'ice:sdp') {
            if (e.sdp.type == 'offer') {
            	  try {
                    console.log('Recv offer:', JSON.stringify(e.sdp))

                    // Create the remote player on offer
                    if (!$scope.remotePlayer) {
                        $scope.remotePlayer = createPlayer($scope, '#video-location .remote-video', false);
                    }
                   
                    $scope.remotePlayer.setup();
                    $scope.remotePlayer.play();
                    $scope.remotePlayer.engine.recvRemoteSDP(e.sdp);
                    sendPlayerLocalSdpAndCandidate($scope, true, 'answerer')
            	  }
                catch (ee) {
                    console.log("Failed to create PeerConnection:", ee);
                }
            }
            else if (e.sdp.type == 'answer') {
                $scope.localPlayer.engine.recvRemoteSDP(e.sdp);
            }
        }

        // ICE Candidate
        else if (e.name == 'ice:candidate') {
            if (e.origin == 'answerer')
                $scope.remotePlayer.engine.recvRemoteCandidate(e.candidate);
            else if (e.origin == 'caller')
                $scope.localPlayer.engine.recvRemoteCandidate(e.candidate);
            // else
            //     alert('Unknown candidate origin');
        }

        else {
            alert('Unknown event: ' + e.name);
        }
    });

    $scope.client.on('disconnect', function() {
        console.log('Disconnected from the server')
        $scope.isLoading = false;
        $scope.errorText = 'Disconnected from the server';
        $scope.peers = [];
        $scope.$apply();
    });

    $scope.client.on('error', function(error, message) {
        console.log('Connection error:', error, message)
        $scope.isLoading = false;
        $scope.errorText = 'Cannot connect to the server.';
        $scope.$apply();
    });

    $scope.client.on('addPeer', function(peer) {
        console.log('Adding peer:', peer)

        if (peer.type == 'mediaserver') {
            $scope.mediaServerPeer = peer;

            // get a list of available files
            $scope.client.sendCommand({
                node: 'streaming:files',
                to: peer
            });
        } else {
            $scope.peers.push(peer);
        }
        $scope.$apply();
    });

    $scope.client.on('removePeer', function(peer) {
        console.log('Removing peer:', peer)
        if (peer.type == 'mediaserver') {
            $scope.mediaServerPeer = '';
            $scope.$apply();
        } else {
            for (var i = 0; i < $scope.peers.length; i++) {
                if ($scope.peers[i].id === peer.id) {
                    $scope.peers.splice(i,1);
                    $scope.$apply();
                    break;
                }
            }
        }
    });

    // Init handle from URL if specified
    var handle = getHandleFromURL();
    if (handle && handle.length) {
        $scope.handle = handle;
        $scope.login();
    }


    //
    //= Session ===============================================================

    $scope.login = function() {
        if (!$scope.handle || $scope.handle.length < 3) {
            alert('Please enter 3 or more alphanumeric characters.');
            return;
        }
        
        // TODO: check in redis database (peers is null before login)
        //console.log('peers length:', $scope.peers.length);
        //for (var i=0; i<$scope.peers.length; i++) {
        //    if (peer.user == $scope.peers[i].user) {
        //        alert('The user has logged. Please enter another handler.');
        //        window.location.reload();
        //        return;
        //    }
        //}

        $scope.client.options.peer.user = $scope.handle;
        $scope.client.connect();
        $scope.isLoading = true;
        // $scope.$apply(); // apply already in progress
    }


    //
    //= Messaging =============================================================

    $scope.setMessageTarget = function(user) {
        console.log('setMessageTarget', user);
        //$scope.directUser = user ? user : '';
        $scope.directUser = user;			// undefined for broadcast
        if(user)
            $('#post-message .direct-user').text('@' + $scope.directUser);
        else
            $('#post-message .direct-user').text('@All');
        $('#post-message .message-text')[0].focus();
    }

    $scope.sendMessage = function() {
        if (!$scope.messageText || $scope.messageText.length <= 0) {
            alert('Please enter message.');
            $('#post-message .message-text')[0].focus();
            return;
        }
        
        console.log('sendMessage', $scope.messageText);
        $scope.client.sendMessage({
            data: $scope.messageText,
            to: $scope.directUser,
            direct: $scope.directUser
        });
        //$scope.messages.push({
        $scope.messages.unshift({
            to: $scope.directUser,
            direct: $scope.directUser,
            user: $scope.handle,
            data: $scope.messageText,
            time: Symple.formatTime(new Date)
        });
        $scope.messageText = "";
        $('#post-message .message-text')[0].focus();
    };


    //
    //= Media Streaming (Media Server) ========================================

    $scope.refreshStreamingFiles = function() {
        $scope.client.sendCommand({
            node: 'streaming:files',
            to: $scope.mediaServerPeer
        });
    };

    $scope.startStreamingFile = function(file) {
        destroyPlayers($scope);

        $scope.currentStreamingFile = file;
        $scope.client.sendCommand({
            node: 'streaming:start',
            to: $scope.mediaServerPeer,
            data: {
                file: file
            }
        });
    };

    $scope.stopStreamingFile = function(file) {
        destroyPlayers($scope);

        $scope.currentStreamingFile = null;
        $scope.client.sendCommand({
            node: 'streaming:stop',
            to: $scope.mediaServerPeer,
            data: {
                file: file
            }
        });
    };

    $scope.startRecordingPeer = function(peer) {
        console.log('start recording peer:', peer);
        $scope.currentRecordingPeers.push(peer);
        $scope.$apply();

        $scope.client.sendCommand({
            node: 'recording:start',
            to: $scope.mediaServerPeer,
            data: {
                peer: peer
            }
        });
    }

    $scope.stopRecordingPeer = function(peer) {
        console.log('stop recording peer:', peer);
        for (var i = 0; i < $scope.currentRecordingPeers.length; i++) {
            if ($scope.currentRecordingPeers[i].id === peer.id) {
                $scope.currentRecordingPeers.splice(i,1);
                $scope.$apply();
                break;
            }
        }

        $scope.client.sendCommand({
            node: 'recording:stop',
            to: $scope.mediaServerPeer,
            data: {
                peer: peer
            }
        });
    };


    //
    //= Video Call ============================================================

    $scope.startVideoCall = function(peer) {
        if (assertGetUserMedia()) {
            console.log('startVideoCall', peer.user)
            if (peer.user == $scope.handle) {
                alert('Cannot video chat with yourself. Please open a new browser window and login with a different handle.');
                return;
            }

            //destroyPlayers($scope);
            $scope.client.sendCommand({
                node: 'call:init',
                to: peer.user
            });
        }
    }

    $scope.startLocalVideo = function() {
        destroyPlayers($scope);
        
        if (assertGetUserMedia()) {
            // Init local video player
            $scope.localPlayer = createPlayer($scope, '#video-location .local-video', true);
            $scope.localPlayer.setup();
            sendPlayerLocalSdpAndCandidate($scope, false, 'caller');
            $scope.localPlayer.play({ localMedia: true, disableAudio: $scope.disableLocalAudio });
            $scope.localVideoPlaying = true;
        }
    }

    $scope.stopLocalVideo = function() {
        destroyPlayers($scope);
    }


    //
    //= Helpers

    $scope.isLoggedIn = function() {
        return $scope.handle != null && $scope.client.online();
    }
    
    $scope.isRecording = function(peer) {
        for (var i = 0; i < $scope.currentRecordingPeers.length; i++) {
            if ($scope.currentRecordingPeers[i].id === peer.id) {
                return true;
            }
        }
        return false;
    }

    $scope.hasMediaServer = function() {
        return !!$scope.mediaServerPeer;
    }

    $scope.getMessageClass = function(m) {
        if (m.direct)
            return 'list-group-item-warning';
        return '';
    }
// }
});
