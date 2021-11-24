var remoteUserSettings;
var mediaListObj = [];
var playingmediaItem;
var thumbsContainerNode;
var sortByGlobal = "nameAsc";
var showGlobal = "All";
var settingsNode;
var thumbSizeGlobal = 0;
var videoNode;
var videoSourceNode;
var externalStreaming;
var resizeObserver;
var webSocket;
var deviceAddress;
var funscriptChannels = [];
//var loadedFunscripts;
var currentChannelIndex = 0;
var deviceConnectionStatusNode;
var deviceConnectionStatusInterval;
var deviceConnectionStatusRetryNode;
//var funscriptSyncWorker;
//var useDeoWeb;
//var deoVideoNode;
//var deoSourceNode;
document.addEventListener("DOMContentLoaded", function() {
  loadPage();
});
function loadPage()
{
	settingsNode = document.getElementById("settingsModal");
	thumbsContainerNode = document.getElementById("thumbsContainer");

	sortByGlobal = JSON.parse(window.localStorage.getItem("sortBy"));
	showGlobal = JSON.parse(window.localStorage.getItem("show"));
	thumbSizeGlobal = JSON.parse(window.localStorage.getItem("thumbSize"));
	var volume = JSON.parse(window.localStorage.getItem("volume"));
	externalStreaming = JSON.parse(window.localStorage.getItem("externalStreaming"));
	//deviceAddress =  JSON.parse(window.localStorage.getItem("webSocketAddress"));
	// if(!deviceAddress)
	// 	deviceAddress = "tcode.local";
		
	//document.getElementById("webSocketAddress").value = deviceAddress;

	videoNode = document.getElementById("videoPlayer");
	videoSourceNode = document.getElementById("videoSource");
	videoNode.addEventListener("timeupdate", onVideoTimeUpdate); 
	videoNode.addEventListener("loadeddata", onVideoLoad); 
	videoNode.addEventListener("play", onVideoPlay); 
	videoNode.addEventListener("pause", onVideoPause); 
	videoNode.addEventListener("volumechange", onVolumeChange); 
	videoNode.addEventListener("ended", onVideoEnd); 
	videoNode.volume = volume ? volume : 0.5;
	// Fires on load?
	// videoSourceNode.addEventListener('error', function(event) { 
	// 	alert("There was an issue loading media.");
	// }, true);
	// videoNode.addEventListener('error', function(event) { 
	// 	alert("There was an issue loading media.");
	// }, true);

	deviceConnectionStatusNode = document.getElementById("deviceStatus");
	deviceConnectionStatusRetryNode = document.getElementById("deviceConnect");
	
	toggleExternalStreaming(externalStreaming, false);
	
/* 	
	deoVideoNode = document.getElementById("deoVideoPlayer");
	deoSourceNode = document.getElementById("deoVideoSource");
	deoVideoNode.addEventListener("end", onVideoStop); 
	useDeoWeb = JSON.parse(window.localStorage.getItem("useDeoWeb"));

	if(useDeoWeb) {
		toggleUseDeo(useDeoWeb, false);
		new ResizeObserver(onResizeDeo).observe(deoVideoNode)
	} 
*/
	getServerSettings();
	userGetDeviceConnectionStatus();
	
}

function onResizeVideo() {
	thumbsContainerNode.style.maxHeight = "calc(100vh - "+ (+videoNode.offsetHeight + 121) + "px)";
} 

/* 
function onResizeDeo() {
	if(useDeoWeb) {
		thumbsContainerNode.style.maxHeight = "calc(100vh - "+ (+deoVideoNode.offsetHeight + 120) + "px)";
	}
} 
*/

function getServerSettings() {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', "/settings", true);
	xhr.responseType = 'json';
	xhr.onload = function() {
	  var status = xhr.status;
	  if (status === 200) {
		remoteUserSettings = xhr.response;
		funscriptChannels = Object.keys(remoteUserSettings["availableAxis"])
			.map(function (k) {
				return remoteUserSettings["availableAxis"][k]["channel"];
			});
		funscriptChannels.sort();
		getServerLibrary();
	  } else {
		alert("Error getting settings");
	  }
	};
	xhr.send();
}

function getServerLibrary() {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', "/media", true);
	xhr.responseType = 'json';
	xhr.onload = function() {
	  var status = xhr.status;
	  if (status === 200) {
		onMediaLoad(null, xhr.response);
	  } else {
		onMediaLoad(status, xhr.response);
	  }
	};
	xhr.onerror = function() {
		alert("Error getting media");
	};
	xhr.send();
}

function getDeviceConnectionStatus() {
	if(deviceConnectionStatusInterval)
		clearTimeout(deviceConnectionStatusInterval);
	deviceConnectionStatusRetryNode.style.display = "none";
	var xhr = new XMLHttpRequest();
	xhr.open('GET', "/settings/deviceConnectionStatus", true);
	xhr.responseType = 'json';
	xhr.onload = function() {
	  var status = xhr.status;
	  if (status === 200) {
		connectionStatusEvent = xhr.response;
		if(connectionStatusEvent.status == ConnectionStatus.Connected) {
			deviceConnectionStatusNode.className = "device-status device-status-connected";
			deviceConnectionStatusNode.innerText = "TCode: Connected"
			pollDeviceConnectionStatus(30000);
		} else if(connectionStatusEvent.status == ConnectionStatus.Connecting) {
			deviceConnectionStatusNode.className = "device-status device-status-connecting";
			deviceConnectionStatusNode.innerText = "TCode: Connecting..."
			pollDeviceConnectionStatus(1000);
		} else if(connectionStatusEvent.status == ConnectionStatus.Error) {
			deviceConnectionStatusNode.className = "device-status device-status-error";
			deviceConnectionStatusNode.innerText = "TCode: Error"
			deviceConnectionStatusRetryNode.style.display = "block";
			deviceConnectionStatusNode.title = connectionStatusEvent.message;
			pollDeviceConnectionStatus(30000);
		} else if(connectionStatusEvent.status == ConnectionStatus.Disconnected) {
			deviceConnectionStatusNode.className = "device-status device-status-disconnected";
			deviceConnectionStatusNode.innerText = "TCode: Disconnected"
			deviceConnectionStatusRetryNode.style.display = "block";
			pollDeviceConnectionStatus(30000);
		}
	  } else {
		alert("Error getting device connection status: "+ status);
		if(deviceConnectionStatusInterval)
			clearTimeout(deviceConnectionStatusInterval);
		deviceConnectionStatusNode.className = "device-status device-status-error";
		deviceConnectionStatusNode.innerText = "TCode: Error"
		deviceConnectionStatusRetryNode.style.display = "block";
	  }
	};
	xhr.onerror = function() {
		var status = xhr.status;
		alert("Error getting device connection status: "+ status);
		if(deviceConnectionStatusInterval)
			clearTimeout(deviceConnectionStatusInterval);
		deviceConnectionStatusNode.className = "device-status device-status-error";
		deviceConnectionStatusNode.innerText = "TCode: Error"
		deviceConnectionStatusRetryNode.style.display = "block";
	};
	xhr.send();
}

function pollDeviceConnectionStatus(pollInterval) {
	if(deviceConnectionStatusInterval) {
		clearTimeout(deviceConnectionStatusInterval);
		deviceConnectionStatusInterval = null;
	}
	deviceConnectionStatusInterval = setTimeout(function () {
		getDeviceConnectionStatus();
	}, pollInterval);
}

function userGetDeviceConnectionStatus() {
	deviceConnectionStatusNode.className = "device-status device-status-connecting";
	deviceConnectionStatusNode.innerText = "Getting device connection status..."
	deviceConnectionStatusNode.title = ""
	getDeviceConnectionStatus();
}

function getMediaFunscripts(path, isMFS) {
	var channel = funscriptChannels[currentChannelIndex];
	var trackName = remoteUserSettings["availableAxis"][channel]["trackName"];
	var xhr = new XMLHttpRequest();
	var channelPath = trackName === "" ? trackName : "."+trackName;
	xhr.open('GET', path + channelPath + ".funscript", true);
	xhr.responseType = 'json';
	xhr.onload = function() {
		var status = xhr.status;
		if (status === 200) {
			loadedFunscripts.push({"channel": channel, "atList": []});
			var index = loadedFunscripts.length - 1;
			var items = xhr.response;
			for(var i=0; i<items.actions.length;i++)
			{
				var item = items.actions[i];
				var at = item["at"];
				loadedFunscripts[index][at] = item["pos"];
				loadedFunscripts[index].atList.push(at);
			}
		}
		currentChannelIndex++;
		if(currentChannelIndex < funscriptChannels.length)
		{
			getMediaFunscripts(path, isMFS);
		} 
		else 
		{
			currentChannelIndex = 0;
			videoNode.play();
			console.log("Funscripts load finish");
		}
	};
	xhr.send();
}

function postServerSettings() {
	var xhr = new XMLHttpRequest();
	xhr.open('POST', "/settings", true);
	xhr.setRequestHeader('Content-Type', 'application/json');
	xhr.onreadystatechange = function() {
	  if (xhr.readyState === 4) {
		var status = xhr.status;
		if (status !== 200) 
			alert('Error saving server settings: '+status)
	  }
	}
	xhr.onerror = function() {
		var status = xhr.status;
		alert('Error saving server settings: '+status)
	};
	xhr.send(JSON.stringify(remoteUserSettings));
}

function postDeviceConnectRetry() {
	var xhr = new XMLHttpRequest();
	xhr.open('POST', "/settings/connectDevice", true);
	xhr.onreadystatechange = function() {
	  if (xhr.readyState === 4) {
		var status = xhr.status;
		if (status === 200) 
			userGetDeviceConnectionStatus();
		else
			alert('Error sending retry device connection: '+status)
	  }
	}
	xhr.onerror = function() {
		var status = xhr.status;
		alert('Error sending retry device connection: '+status)
	};
	xhr.send();
}

function postMediaState(mediaState) {
	var xhr = new XMLHttpRequest();
	xhr.open("POST", "/xtpweb", true);
	xhr.setRequestHeader('Content-Type', 'application/json');
	xhr.send(JSON.stringify(mediaState));
	xhr.oneload = function() {
		var status = xhr.status;
		if (status !== 200) 
			console.log('Error sending mediastate: '+status)
	};
	xhr.onerror = function() {
		var status = xhr.status;
		console.log('Error sending sending mediastate: '+status)
	};
}

function onMediaLoad(err, mediaList)
{
  if (err !== null) {
    alert('Whoops!: ' + err);
  } else {
	mediaListObj = [];
	for(var i=0; i<mediaList.length;i++)
	{
		var obj = {
			id: mediaList[i]["name"].replace(/^[^a-z]+|[^\w:.-]+/gi, "")+"item"+i,
			name: mediaList[i]["name"],
			displayName: mediaList[i]["name"],
			thumbSize: mediaList[i]["thumbSize"],
			path: mediaList[i]["path"],
			relativePath: mediaList[i]["relativePath"],
			thumb: mediaList[i]["thumb"],
			relativeThumb: mediaList[i]["relativeThumb"],
			modifiedDate: new Date(mediaList[i]["modifiedDate"]),
			isStereoscopic: mediaList[i]["isStereoscopic"],
			isMFS: mediaList[i]["isMFS"],
			hasScript: mediaList[i]["hasScript"],
			scriptNoExtensionRelativePath: mediaList[i]["scriptNoExtensionRelativePath"],
			loaded: false,
			playing: false
		}
		if(obj.isMFS)
			obj.displayName = "(MFS) " + obj.name;
		mediaListObj.push(obj);
	}
/* 	
	if(useDeoWeb && mediaListObj.length > 0)
		loadVideo(mediaListObj[0]); 
*/
	updateSettingsUI();
  }
}

function updateSettingsUI() {
	setThumbSize(thumbSizeGlobal, false);
	sort(sortByGlobal, false);
	loadMedia(show(showGlobal, false))
}

function loadMedia(mediaList) {
	var medialistNode = document.getElementById("mediaList");
	removeAllChildNodes(medialistNode);
	if(!mediaList || mediaList.length == 0)
	{
		var divnode = document.createElement("div"); 
		divnode.id = "NoMedia";
		divnode.innerText = "No media found, it may be loading.";
		medialistNode.appendChild(divnode);
		//deoVideoNode.style.display = "none"
		return;
	}
	var noMediaElement = document.getElementById("NoMedia");
	if(noMediaElement)
		medialistNode.removeChild(noMediaElement)
	
	var createClickHandler = function(obj) { 
		return function() { 
			//loadVideo(obj); 
			playVideo(obj); 
		} 
	};
	
	var textHeight = 0
	var width = 0;
	var height = 0;
	var fontSize = 0;
	for(var i=0; i<mediaList.length;i++)
	{
		var obj = mediaList[i];
		if(!thumbSizeGlobal) {
			setThumbSize(obj.thumbSize, true);
			setThumbSize(obj.thumbSize, false);
		}
		if(!textHeight)
		{
			textHeight = (thumbSizeGlobal * 0.25);
			width = thumbSizeGlobal + (thumbSizeGlobal * 0.15) + "px";
			height = thumbSizeGlobal + textHeight + "px";
			fontSize = (textHeight * 0.4) + "px";
		}
		var divnode = document.createElement("div"); 
		divnode.id = obj.id
		divnode.className += "media-item"
		divnode.style.width = width;
		divnode.style.height = height;
		divnode.title = obj.name;
		var anode = document.createElement("a"); 
		anode.className += "media-link"
		if(obj.isMFS) {
			divnode.className += " media-item-mfs"
		}
		if(!obj.hasScript) {
			divnode.className += " media-item-noscript"
		}
		//anode.style.width = width;
		//anode.style.height = height;
		anode.onclick = createClickHandler(obj);
		var image = document.createElement("img"); 
		image.src = "/thumb/" + obj.relativeThumb;
		image.style.maxWidth = thumbSizeGlobal + "px";
		image.style.maxHeight = thumbSizeGlobal + "px";
		image.id = obj.id;
		var namenode = document.createElement("div");
		namenode.innerText = obj.displayName;
		namenode.className += "name"
		namenode.style.width = width;
		namenode.style.height = textHeight + "px";
		namenode.style.fontSize = fontSize;
		
		divnode.appendChild(anode);
		anode.appendChild(image);
		anode.appendChild(namenode);
		medialistNode.appendChild(divnode);
		if(playingmediaItem && playingmediaItem.id === obj.id)
			setPlayingMediaItemColor(obj);
	}
}

function removeAllChildNodes(parent) {
    while (parent.firstChild) {
        parent.removeChild(parent.firstChild);
    }
}

function sort(value, userClick) {
	if(!value)
		value = "nameAsc";
	switch(value) {
		case "dateDesc":
			mediaListObj.sort(function(a,b){
			  return new Date(b.modifiedDate) - new Date(a.modifiedDate);
			});
		break;
		case "dateAsc":
			mediaListObj.sort(function(a,b){
			  return new Date(a.modifiedDate) - new Date(b.modifiedDate);
			});
		break;
		case "nameAsc":
			mediaListObj.sort(function(a,b){
			  var nameA = a.displayName.toUpperCase(); // ignore upper and lowercase
			  var nameB = b.displayName.toUpperCase(); // ignore upper and lowercase
			  if (nameA < nameB) {
				return -1;
			  }
			  if (nameA > nameB) {
				return 1;
			  }
			});
		break;
		case "nameDesc":
			mediaListObj.sort(function(a,b){
			  var nameA = a.displayName.toUpperCase(); // ignore upper and lowercase
			  var nameB = b.displayName.toUpperCase(); // ignore upper and lowercase
			  if (nameB < nameA) {
				return -1;
			  }
			  if (nameB > nameA) {
				return 1;
			  }
			});
		break;
	}
	if(!userClick)
		document.getElementById("sortBy").value = value;
	else {
		window.localStorage.setItem("sortBy", JSON.stringify(value));
		sortByGlobal = value;
	}
}

function show(value, userClick) {
	if(!value)
		value = "All";
	filteredMedia = [];
	switch(value) {
		case "All":
			filteredMedia = mediaListObj;
		break;
		case "3DOnly":
			filteredMedia = mediaListObj.filter(x => x.isStereoscopic);
		break;
		case "2DAndAudioOnly":
			filteredMedia = mediaListObj.filter(x => !x.isStereoscopic);
		break;
	}
	if(!userClick)
		document.getElementById("show").value = value;
	else {
		window.localStorage.setItem("show", JSON.stringify(value));
		showGlobal = value;
	}
	return filteredMedia;
}

/* 
function onClickUseDeoWebCheckbox(checkbox)
{
	toggleUseDeo(checkbox.checked, true);
}

function toggleUseDeo(value, userClicked)
{
	if(userClicked) {
		useDeoWeb = value;
		window.localStorage.setItem("useDeoWeb", JSON.stringify(useDeoWeb));
	}
	else
		document.getElementById("useDeoWebCheckbox").checked = value;
	if(!useDeoWeb)
	{
		deoVideoNode.style.display = "none";
		thumbsContainerNode.style.maxHeight = "";
	} else {
		deoVideoNode.style.display = "block";
		onResizeDeo();
	}
}

function loadVideo(obj) {
	if(useDeoWeb) {
		deoVideoNode.setAttribute("format", obj.isStereoscopic ? "LR" : "mono");
		deoSourceNode.setAttribute("src", "/video/" + obj.relativePath);
		deoVideoNode.setAttribute("title", obj.name);
		deoVideoNode.setAttribute("cover-image", "/thumb/" + obj.relativeThumb);
		
		if(!DEO.isStarted(deoVideoNode))
			DEO.setVolume(deoVideoNode, 0.3);
		DEO.play(deoVideoNode);
	}
} 
*/
function onClickExternalStreamingCheckbox(checkbox)
{
	toggleExternalStreaming(checkbox.checked, true);
}
function toggleExternalStreaming(value, userClicked)
{
	if(userClicked) {
		externalStreaming = value;
		window.localStorage.setItem("externalStreaming", JSON.stringify(value));
	}
	else
		document.getElementById("externalStreamingCheckbox").checked = value;
	if(value) {
		videoNode.pause();
		videoNode.style.display = "none";
		thumbsContainerNode.style.maxHeight = "";
		if(resizeObserver)
			resizeObserver.unobserve(videoNode);
	} else {
		onResizeVideo();
		resizeObserver = new ResizeObserver(onResizeVideo);
		resizeObserver.observe(videoNode);
	}
}

function playVideo(obj) {
	if(playingmediaItem) {
		if(playingmediaItem.id === obj.id)
			return;
		var mediaItemNode = document.getElementById(playingmediaItem.id);
		mediaItemNode.classList.remove("playing-media-item");
	}
	playingmediaItem = obj;
	setPlayingMediaItemColor(obj);
	if(!externalStreaming) {
		videoNode.style.display = "block";
		videoSourceNode.setAttribute("src", "/video" + obj.relativePath);
		videoNode.setAttribute("title", obj.name);
		videoNode.setAttribute("poster", "/thumb/" + obj.relativeThumb);
		videoNode.load();
		// loadedFunscripts = [];
		// if(playingmediaItem.hasScript)
		// 	loadMediaFunscript(playingmediaItem.scriptNoExtensionRelativePath, playingmediaItem.isMFS);
		// else
			videoNode.play();

	} else { 
		window.open("/video"+ obj.relativePath)
	}
}
function setPlayingMediaItemColor(obj) {
	var mediaItemNode = document.getElementById(obj.id);
	mediaItemNode.classList.add("playing-media-item");
}
var timer1 = 0;
var timer2 = Date.now();
function onVideoTimeUpdate(event) {
	if (timer2 - timer1 >= 1000) {
		timer1 = timer2;
		sendMediaState();
	}
	timer2 = Date.now();
}
function onVideoLoad(event) {
	console.log("Data loaded")
	console.log("Duration: "+ videoNode.duration )
	playingmediaItem.loaded = true;
}
function onVideoPlay(event) {
	console.log("Video play")
	playingmediaItem.playing = true;
	// if(!funscriptSyncWorker && loadedFunscripts && loadedFunscripts.length > 0)
	// 	startFunscriptSync(loadedFunscripts);
	//sendMediaState();
}
function onVideoPause(event) {
	console.log("Video pause")
	playingmediaItem.playing = false;
	//setTimeout(function() {
		sendMediaState();// Sometimes a timeupdate is sent after this event fires?
	//}, 500);
}
function onVolumeChange() {
	window.localStorage.setItem("volume", videoNode.volume);
}
function onVideoEnd(event) {
	// if(funscriptSyncWorker) {
	// 	funscriptSyncWorker.postMessage(JSON.stringify({"command": "terminate"}));
	// }
	var playingIndex = mediaListObj.findIndex(x => x.path === playingmediaItem.path);
	playingIndex++;
	if(playingIndex < mediaListObj.length)
		playVideo(mediaListObj[playingIndex]);
	else
		playVideo(mediaListObj[0]);
}
function setThumbSize(value, userClick) {
	if(!userClick) {
		if(value)
			document.getElementById("thumbSize").value = value.toString();
	} else {
		window.localStorage.setItem("thumbSize", parseInt(value, 10));
		thumbSizeGlobal = parseInt(value, 10);
	}
}

// function startFunscriptSync() {
// 	if (window.Worker) {
// 		if(funscriptSyncWorker)
// 			funscriptSyncWorker.terminate();
// 		funscriptSyncWorker = new Worker('syncFunscript.js');
// 		funscriptSyncWorker.postMessage(JSON.stringify({
// 			"command": "startThread", 
// 			"funscripts": loadedFunscripts, 
// 			"remoteUserSettings": remoteUserSettings
// 		}));
// 		funscriptSyncWorker.onmessage = onFunscriptWorkerThreadRecieveMessage;
// 	}
// }

function sendMediaState() {
	//console.log("sendMediaState")
	postMediaState({
		"path": playingmediaItem.path,
		"playing": playingmediaItem.playing, 
		"currentTime": videoNode.currentTime, 
		"duration": videoNode.duration,
		"playbackSpeed": videoNode.speed
	});
}

function onFunscriptWorkerThreadRecieveMessage(e) {
    isMediaFunscriptPlaying = true;
    var data;
    if(typeof e.data === "string")
        data = JSON.parse(e.data);
    else
        data = e.data;
    switch(data["command"]) {
        case "sendTcode":
            sendTcode(data["tcode"])
            break;
		case "getMediaState":
			sendMediaState();
			break;
        case "end":
			funscriptSyncWorker.terminate();
			funscriptSyncWorker = null;
            break;
    }
}
//Settings
function openSettings() {
  settingsNode.style.display = "flex";
}

function closeSettings() {
  settingsNode.style.display = "none";
}

function showChange(selectNode) {
	sort(sortByGlobal, true);
	loadMedia(show(selectNode.value, true));
}

function sortChange(selectNode) {
	sort(selectNode.value, true);
	loadMedia(show(showGlobal, true));
}

function thumbSizeChange(selectNode) {
	setThumbSize(selectNode.value, true);
	loadMedia(show(showGlobal, true));
}

var debouncer;
function webSocketAddressChange(e) {
	if(debouncer) {
		clearTimeout(debouncer);
	}
	debouncer = setTimeout(function () {
		debouncer = null;
		deviceAddress = document.getElementById("webSocketAddress").value;
		window.localStorage.setItem("webSocketAddress", value);
	}, 500);
}

function defaultSettings() {
	var r = confirm("Are you sure you want to reset ALL settings to default?");
	if (r) {
		window.localStorage.clear();
		window.location.reload();
	} 
}

function deleteSettings() {
	var r = confirm("Are you sure you want to delete ALL settings from localStorage and close the window?");
	if (r) {
		window.localStorage.clear();
		window.close();//Does not work
	} 
}


DeviceType =
{
    Serial: 0,
    Network: 1,
    Deo: 2,
    Whirligig: 3,
    Gamepad: 4,
    XTPWeb: 5
};

ConnectionStatus =
{
    Connected: 0,
    Disconnected: 1,
    Connecting: 2,
    Error: 3
};

ChannelType = 
{
    None: 0,
    Range: 1,
    Switch: 2,
    HalfRange: 3
}

AxisDimension =
{
    None: 0,
    Heave: 1,
    Surge: 2,
    Sway: 3,
    Pitch: 4,
    Roll: 5,
    Yaw: 6
};
// function connectToTcodeDevice() {
// 	webSocket = new WebSocket("ws://"+deviceAddress+"/ws");
// 	webSocket.onopen = function (event) {
// 		webSocket.send("D1");
// 	};
// 	webSocket.onmessage = function (event) {
// 		console.log(event.data);
// 		document.getElementById("webSocketStatus").innerHTML = "Connected";
// 	}
// 	webSocket.onerror = function (event) {
// 		console.log(event.data);
// 		webSocket = null;
// 		document.getElementById("webSocketStatus").innerHTML = "Error";
// 	}
// }

// function sendTcode(tcode) {
// 	console.log("Send tcode: "+tcode);
// 	if(webSocket)
// 	{
// 		webSocket.send(tcode);
// 	}
// }
