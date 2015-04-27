/* find the audio context */
var audioCtx = new (window.AudioContext || window.webkitAudioContext)();

/* other variables pertinent to audio processing */
var secondsPerNode = 1.0;
var channels = 2;
var frameCount = audioCtx.sampleRate * secondsPerNode;
var myArrayBuffer = audioCtx.createBuffer(channels, frameCount, audioCtx.sampleRate);
var samplesMalloc;
var samples;
var source;

/* visualization */
var vizBufferSize = audioCtx.sampleRate * channels;
var vizBuffer = new Int16Array(vizBufferSize);
var vizBufferIndex = 0;
var canvas;
var canvasCtx;
var canvasWidth;
var canvasHeight;
var color = 0;
var audioStarted = false;
var firstAudioTimestamp = 0;
var nextTimestamp = 0;
var FRAMERATE_DELTA = 1.0/30;

var playerFile;
var currentTrack = 2;
var musicResponseBytes;
var playerContext;
var isLoaded = false;
var isPaused = false;
var playerIsReadyCallback = null;

function musicLoadEvent(evt)
{
    if (evt.type == "progress")
    {
        console.log("progress event: " + evt.loaded + " / " + evt.total);
    }
    else if (evt.type == "load")
    {
        /* copy the response bytes to a typed array */
        musicResponseBytes = new Uint8Array(evt.target.response);

        /* request the player to be loaded */
        var script = document.createElement('script');
        script.src = playerFile;
        document.head.appendChild(script);
    }
}

/* the player calls this function to signal that it is loaded and ready */
function crPlayerIsReady()
{
console.log("Player is ready!");

    /* create a private context for the player to use */
    var contextSize = _crPlayerContextSize();
    var contextMalloc = Module._malloc(contextSize);
    playerContext = new Uint8Array(Module.HEAPU8.buffer, contextMalloc, contextSize);

    /* initialize the player */
    ret = _crPlayerInitialize(playerContext.byteOffset, audioCtx.sampleRate);
    console.log("_crPlayerInitialize() returned " + ret);

    /* transfer the ArrayBuffer to a UInt8Array */
    var musicBytesMalloc = Module._malloc(musicResponseBytes.byteLength);
    var musicBytes = new Uint8Array(Module.HEAPU8.buffer, musicBytesMalloc,musicResponseBytes.byteLength);
    musicBytes.set(new Uint8Array(musicResponseBytes));

    /* load the file into the player */
    ret = _crPlayerLoadFile(playerContext.byteOffset, 0, musicBytes.byteOffset, musicBytes.length);
    console.log("_crPlayerLoadFile() returned " + ret);

    /* set the initial track */
    ret = _crPlayerSetTrack(playerContext.byteOffset, currentTrack);
    console.log("set track returned " + ret);
    console.log("song has " + _crPlayerGetVoiceCount(playerContext.byteOffset) + " channels");

    initOscope();

    /* tell the host code that the player is ready */
    playerIsReadyCallback();
}

function startAudio()
{
    /* script processor drives the dynamic audio generation */
    scriptNode = audioCtx.createScriptProcessor(8192, 2, 2);
    scriptNode.onaudioprocess = generateAudioCallback;

    /* Get an AudioBufferSourceNode to play an AudioBuffer */
    source = audioCtx.createBufferSource();

    /* set the buffer in the AudioBufferSourceNode */
    source.buffer = myArrayBuffer;

    /* connect the AudioBufferSourceNode to ScriptProcessorNode, and the
     * ScriptProcessorNode to the audio context destination */
    source.connect(scriptNode);
    scriptNode.connect(audioCtx.destination);

    /* start the source playing */
    source.loop = true;

    vizBufferIndex = 0;
    source.start(0);

    isLoaded = true;
    isPaused = false;
}

function pauseAudio()
{
console.log("pausing audio");
    isPaused = true;
}

function resumeAudio()
{
console.log("resuming audio");
    isPaused = false;
    requestAnimationFrame(drawOscope);
}

/* callback for generating more audio */
function generateAudioCallback(audioProcessingEvent)
{
    // The output buffer contains the samples that will be modified and played
    var outputBuffer = audioProcessingEvent.outputBuffer;

    if (isPaused)
    {
        /* Loop through the output channels */
        for (var channel = 0; channel < outputBuffer.numberOfChannels; channel++)
        {
            var outputData = outputBuffer.getChannelData(channel);
            var index = vizBufferIndex + channel;

            /* Loop through the input samples */
            for (var sample = 0; sample < outputBuffer.length; sample++)
            {
                outputData[sample] = 0.0;
                /* TODO: figure out the right formula to advance this once, outside of loop */
                index = (index + 2) % vizBufferSize;
            }
        }
        vizBufferIndex = index;

        return;
    }

    /* create an array for the player to use for sample generation */
    var samplesCount = outputBuffer.length * channels;
    var samplesCountInBytes = samplesCount * 2;
    if (!samplesMalloc || !samples)
    {
        samplesMalloc = Module._malloc(samplesCountInBytes);
        samples = new Int16Array(Module.HEAP16.buffer, samplesMalloc, samplesCount);
    }
    var ret = _crPlayerGenerateStereoFrames(playerContext.byteOffset, samples.byteOffset, outputBuffer.length);

    /* Loop through the output channels */
    for (var channel = 0; channel < outputBuffer.numberOfChannels; channel++)
    {
        var outputData = outputBuffer.getChannelData(channel);
        var index = vizBufferIndex + channel;

        /* Loop through the input samples */
        for (var sample = 0; sample < outputBuffer.length; sample++)
        {
            var currentSample = samples[channel + sample * 2];

            /* convert data from int16 -> float32 [-1.0 .. 1.0] */
            outputData[sample] = currentSample / 32767.0;

            /* stash data for visualization */
            vizBuffer[index] = currentSample;
            index = (index + 2) % vizBufferSize;
        }
    }
    vizBufferIndex = index;
    if (!audioStarted)
    {
        firstAudioTimestamp = audioProcessingEvent.playbackTime;
        audioStarted = true;
        drawOscope(0);
    }
}

function initOscope()
{
    canvas = document.querySelector('.visualizer');
    canvasCtx = canvas.getContext("2d");
    canvasWidth = canvasCtx.canvas.width;
    canvasHeight = canvasCtx.canvas.height;

    /* wipe canvas */
    canvasCtx.fillStyle = 'rgb(0, ' + color + ', 0)';
    canvasCtx.fillRect(0, 0, canvasWidth, canvasHeight);
    /* dividing center line */
    canvasCtx.strokeStyle = 'rgb(255, 255, 255)';
    canvasCtx.beginPath();
    canvasCtx.moveTo(0, canvasHeight / 2);
    canvasCtx.lineTo(canvasWidth, canvasHeight / 2);
    canvasCtx.stroke();
}

function drawOscope(timestamp)
{
    if (isPaused)
        return;
    timestamp /= 1000;
    drawVisual = requestAnimationFrame(drawOscope);

    if (nextTimestamp)
    {
        if (timestamp <= nextTimestamp)
            return;
    }
    else if (!audioStarted || timestamp < firstAudioTimestamp)
    {
        return;
    }

    canvasCtx.fillStyle = 'rgb(0, ' + color + ', 0)';
    canvasCtx.fillRect(0, 0, canvasWidth, canvasHeight);

    canvasCtx.lineWidth = 1;
    canvasCtx.strokeStyle = 'rgb(255, 255, 255)';
    canvasCtx.beginPath();
    canvasCtx.moveTo(0, canvasHeight / 2);
    canvasCtx.lineTo(canvasWidth, canvasHeight / 2);
    canvasCtx.stroke();

    var segment = Math.round((audioCtx.currentTime - firstAudioTimestamp) * vizBufferSize % vizBufferSize);
    var divisor = 32768.0 / (canvasHeight / 4);
    for (var i = 0; i < channels; i++)
    {
        var index = segment + i;
        var center = (canvasHeight / 4) + (i * canvasHeight / 2);
        for (var x = 0; x < canvasWidth; x++)
        {
            var y = center - (vizBuffer[index] / divisor);
            index += 2;

            if(x === 0)
            {
                canvasCtx.moveTo(x, y);
            }
            else
            {
                canvasCtx.lineTo(x, y);
            }
        }
        canvasCtx.stroke();
    }

    nextTimestamp = timestamp + FRAMERATE_DELTA;
}

function initializeCrPlayer(player, fileList, playerIsReady)
{
    playerFile = player[0];
    playerIsReadyCallback = playerIsReady;

    /* load the music file first */
    var musicFile = new XMLHttpRequest();
    musicFile.addEventListener("progress", musicLoadEvent);
    musicFile.addEventListener("load", musicLoadEvent);
    musicFile.addEventListener("error", musicLoadEvent);
    musicFile.addEventListener("abort", musicLoadEvent);
    musicFile.open("GET", fileList[0]);
    musicFile.responseType = "arraybuffer";
    musicFile.send();
}

function enableCrPlayerAudio(enabled)
{
    if (enabled)
    {
        resumeAudio();
    }
    else
    {
        pauseAudio();
    }
}
