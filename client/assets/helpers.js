function createPlayer($scope, selector, isInitiator) {
    var player = new Symple.Player({
        element: selector,
        engine: 'WebRTC',
        rtcConfig: WEBRTC_CONFIG,
        initiator: isInitiator,
        mediaConstraints: {
          'mandatory': {
            'OfferToReceiveAudio':true,
            'OfferToReceiveVideo':true
          }
        },
        onStateChange:  function(player, state) {
            player.displayStatus(state);
        }
    });
    
    return player;
}

function sendPlayerLocalSdpAndCandidate($scope, forRemote, origin) {
	  if(forRemote) {
		    $scope.remotePlayer.engine.sendLocalSDP = function(desc) {
		        $scope.client.send({
		            name: 'ice:sdp',
		            to: $scope.remoteVideoPeer,
		            origin: origin,
		            type: 'event',
		            sdp: desc
		        });
		    }
		    $scope.remotePlayer.engine.sendLocalCandidate = function(cand) {
		        $scope.client.send({
		            name: 'ice:candidate',
		            to: $scope.remoteVideoPeer,
		            origin: origin,
		            type: 'event',
		            candidate: cand
		        });
        }
	  }
	  else {
		    $scope.localPlayer.engine.sendLocalSDP = function(desc) {
		        $scope.client.send({
		            name: 'ice:sdp',
		            to: $scope.remoteVideoPeer,
		            origin: origin,
		            type: 'event',
		            sdp: desc
		        });
		    }
		    $scope.localPlayer.engine.sendLocalCandidate = function(cand) {
		        $scope.client.send({
		            name: 'ice:candidate',
		            to: $scope.remoteVideoPeer,
		            origin: origin,
		            type: 'event',
		            candidate: cand
		        });
	      }
	  }
}

function destroyPlayers($scope) {
    if ($scope.remotePlayer) {
        $scope.remotePlayer.destroy();
        $scope.remotePlayer = null;
    }
    if ($scope.localPlayer) {
        $scope.localPlayer.destroy();
        $scope.localPlayer = null;
        $scope.localVideoPlaying = false;
    }
    if ($scope.remoteVideoPeer) {
        $scope.remoteVideoPeer = null;
    }
    //$scope.$apply();		// zrb. del. 2017.07
}

function getHandleFromURL() {
    return location.search.split('handle=')[1] ? location.search.split('handle=')[1] : '';
}

function assertGetUserMedia() {
    if (navigator.getUserMedia || navigator.webkitGetUserMedia ||
        navigator.mozGetUserMedia || navigator.msGetUserMedia) {
        return true;
    }
    else {
        alert('getUserMedia() is not supported in your browser. Please upgrade to the latest Chrome or Firefox.');
        return false;
    }
}
