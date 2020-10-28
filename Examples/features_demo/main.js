const audioContext = new AudioContext();
const audioElement = document.getElementById("audio");

let runtime = false;

Module.onRuntimeInitialized = function() {
  runtime = true;
  console.log('WASM Runtime Ready.');
};

async function startAnalyse() {
  if(audioContext.state != "running") {
    audioContext.resume().then(function() {
      console.log('audioContext resumed.')
    });
  }
  audioElement.play();

  if(runtime) {
    let pcm = await getAudioPCMData(audioElement);
    console.log("Audio PCM:", pcm);
    let features = getRNNoiseFeatures(pcm);
    console.log("RNNoise features:", features);
  } else {
    console.log('WASM Runtime ERROR!');
  }
}

async function getAudioPCMData(audio) {
  let request = new Request(audio.src);
  let response = await fetch(request);
  let audioFileData = await response.arrayBuffer();
  let audioDecodeData = await audioContext.decodeAudioData(audioFileData);
  let audioPCMData = audioDecodeData.getChannelData(0);

  return audioPCMData;
}

function getRNNoiseFeatures(pcm) {
  let pcmLength = 44100;
  let featuresLength = 4200;
  let pcmPtr = Module._malloc(4 * pcmLength);
  for (let i = 0; i < pcmLength; i++) {
    Module.HEAPF32[pcmPtr / 4 + i] = pcm[i];
  }
  let getFeatures = Module.cwrap('get_features', 'number', ['number']);
  let featuresPtr = getFeatures(pcmPtr);
  let features = [];

  for (let i = 0; i < featuresLength; i++) {
    features[i] = Module.HEAPF32[(featuresPtr >> 2) + i];
  }

  Module._free(pcmPtr, featuresPtr);

  return features;
}
