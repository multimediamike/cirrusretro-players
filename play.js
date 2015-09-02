/* find the audio context */
var audioCtx = new (window.AudioContext || window.webkitAudioContext)();

/* other variables pertinent to audio processing */
var secondsPerNode = 1.0;
var channels = 2;
var frameCount = audioCtx.sampleRate * secondsPerNode;
var myArrayBuffer = audioCtx.createBuffer(channels, frameCount, audioCtx.sampleRate);
var samplesMalloc;
var samples;
var source;  /* AudioBufferSourceNode */
var FRAME_COUNT = 4096;

/* visualization */
var vizEnabled = true;
var vizBufferSize = audioCtx.sampleRate * channels;
var vizBuffer = new Int16Array(vizBufferSize);
var vizBufferIndex = 0;
var canvas = null;
var canvasCtx;
var canvasWidth;
var canvasHeight;
var audioStarted = false;
var firstAudioTimestamp = 0;
var nextTimestamp = 0;
var FRAMERATE_DELTA = 1.0/30;
var currentRed = 250;
var currentGreen = 250;
var currentBlue = 250;
var rInc = -1;
var gInc = -2;
var bInc = -3;

var playerFile;
var crCurrentTrack = 0;
var musicResponseBytes;
var playerContext;
var isPaused = false;
var failureState = false;
var playerIsReadyCallback = null;
var loadProgressCallback = null;
var tickCallback = null;
var tickCountdown;  /* when this goes below 0, a second has elapsed */

/*
 * Private function:
 *  musicLoadEvent(evt)
 *
 * This callback is invoked for music progress/load/error events. When
 * the music is fully loaded, go to the next phase of loading the player.
 */
function musicLoadEvent(evt)
{
    if (evt.type == "progress")
    {
        if (loadProgressCallback)
        {
            loadProgressCallback(evt.loaded, evt.total);
        }
    }
    else if (evt.type == "load")
    {
        /* copy the response bytes to a typed array */
        musicResponseBytes = new Uint8Array(evt.target.response);

        /* request the player to be loaded */
        var script = document.createElement('script');
        script.src = playerFile;
        script.onload = crPlayerIsLoaded;
        document.head.appendChild(script);
    }
}

/*
 * Private function:
 *  crPlayerIsLoaded()
 *
 * This function is called after various resources (music and players)
 * have been loaded. It attempts to initialize the player using the music
 * file.
 *
 * Afterwards, it invokes the client's callback. The parameter it passes
 * to the callback is either null if everything initialized correctly, or
 * a string to describe what failed.
 */
function crPlayerIsLoaded()
{
    /* create a private context for the player to use */
    var contextSize = _crPlayerContextSize();
    /* the context size really should be non-zero */
    if (contextSize <= 0)
    {
        playerIsReadyCallback("Problem: context size is " + contextSize);
        return;
    }

    var contextMalloc = Module._malloc(contextSize);
    playerContext = new Uint8Array(Module.HEAPU8.buffer, contextMalloc, contextSize);

    /* initialize the player */
    ret = _crPlayerInitialize(playerContext.byteOffset, audioCtx.sampleRate);
    if (ret != 1)
    {
        playerIsReadyCallback("Problem: player initialization returned " + ret);
        return;
    }

    /* transfer the ArrayBuffer to a UInt8Array */
    var musicBytesMalloc = Module._malloc(musicResponseBytes.byteLength);
    var musicBytes = new Uint8Array(Module.HEAPU8.buffer, musicBytesMalloc,musicResponseBytes.byteLength);
    musicBytes.set(new Uint8Array(musicResponseBytes));

    /* load the file into the player */
    ret = _crPlayerLoadFile(playerContext.byteOffset, 0, musicBytes.byteOffset, musicBytes.length, 0 /* parm needs to go away */);
    if (ret != 1)
    {
        playerIsReadyCallback("Problem: load file operation returned " + ret);
        return;
    }

    /* set the initial track */
    ret = _crPlayerSetTrack(playerContext.byteOffset, crCurrentTrack);
    if (ret == 0)
    {
        playerIsReadyCallback("Problem: set track operation returned " + ret);
        return;
    }

    /* validate that the voice count makes sense */
    ret = _crPlayerGetVoiceCount(playerContext.byteOffset);
    if (ret == 0)
    {
        playerIsReadyCallback("Problem: voice count is " + ret);
        return;
    }

    /* initialize the visualization */
    initOscope();

    /* tell the host code that the player is ready */
    playerIsReadyCallback(null);
}

function setCrTrack(track)
{
    crCurrentTrack = track;
    tickCountdown = audioCtx.sampleRate;

    ret = _crPlayerSetTrack(playerContext.byteOffset, crCurrentTrack);
}

/*
 * Public function:
 *  startCrAudio()
 *
 * Start the playback, including the visualizer if enabled.
 *
 * Input:
 *  - none
 *
 * Output:
 *  - undefined
 */
function startCrAudio()
{
    /* script processor drives the dynamic audio generation */
    scriptNode = audioCtx.createScriptProcessor(FRAME_COUNT, 2, 2);
    scriptNode.onaudioprocess = generateAudioCallback;

    /* Get an AudioBufferSourceNode to play an AudioBuffer */
    source = audioCtx.createBufferSource();

    /* set the buffer in the AudioBufferSourceNode */
    source.buffer = myArrayBuffer;

    /* connect the AudioBufferSourceNode to ScriptProcessorNode, and the
     * ScriptProcessorNode to the audio context destination */
    source.connect(scriptNode);
    scriptNode.connect(audioCtx.destination);

    vizBufferIndex = 0;
    isPaused = false;

    /* start the source playing */
    source.start(0);
}

/* callback for generating more audio */
function generateAudioCallback(audioProcessingEvent)
{
    // The output buffer contains the samples that will be modified and played
    var outputBuffer = audioProcessingEvent.outputBuffer;

    if (isPaused && !failureState)
    {
        /* Loop through the output channels */
        for (var channel = 0; channel < outputBuffer.numberOfChannels; channel++)
        {
            var outputData = outputBuffer.getChannelData(channel);

            /* Loop through the input samples */
            for (var sample = 0; sample < outputBuffer.length; sample++)
            {
                outputData[sample] = 0.0;
            }
        }
        vizBufferIndex = (outputBuffer.length * 2) % vizBufferSize;

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
    if (ret == 0)
    {
        console.log("failed to generate frames");
        failureState = true;
        return;
    }

    /* tick accounting */
    tickCountdown -= outputBuffer.length;
    if (tickCountdown < 0)
    {
        tickCountdown += audioCtx.sampleRate;
        if (tickCallback)
            tickCallback();
    }

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
    if (!canvas)
        return;

    canvasCtx = canvas.getContext("2d");
    canvasWidth = canvasCtx.canvas.width;
    canvasHeight = canvasCtx.canvas.height;

    /* wipe canvas */
    canvasCtx.fillStyle = 'rgb(0, 0, 0)';
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
    if (!vizEnabled || !canvas || isPaused)
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

    canvasCtx.fillStyle = 'rgb(0, 0, 0)';
    canvasCtx.fillRect(0, 0, canvasWidth, canvasHeight);

    canvasCtx.lineWidth = 1;
    canvasCtx.strokeStyle = 'rgb(255, 255, 255)';
    canvasCtx.beginPath();
    canvasCtx.moveTo(0, canvasHeight / 2);
    canvasCtx.lineTo(canvasWidth, canvasHeight / 2);
    canvasCtx.stroke();

    canvasCtx.beginPath();
    canvasCtx.lineWidth = 2;
    canvasCtx.strokeStyle = 'rgb(' + currentRed + ', ' + currentGreen + ', ' + currentBlue + ')';
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

    /* play with colors */
    if (currentRed < 64 || currentRed > 250)
        rInc *= -1;
    if (currentGreen < 64 || currentGreen > 250)
        gInc *= -1;
    if (currentBlue < 64 || currentBlue > 250)
        bInc *= -1;
    currentRed += rInc;
    currentGreen += gInc;
    currentBlue += bInc;

    nextTimestamp = timestamp + FRAMERATE_DELTA;
}

/*
 * Public function:
 *  initializeCrPlayer(player, musicUrl, hostCanvas, playerIsReady, firstTrack)
 *
 * Initialize a Cirrus Retro player.
 *
 * Input:
 *  - player: URL of the player JavaScript
 *  - musicUrl: URL of the music file to be played
 *  - hostCanvas: A canvas for visualization, or null for no viz
 *  - loadProgress: A callback to indicate how much music has loaded; has
 *     2 parameters: (bytesLoaded, bytesTotal)
 *  - playerIsReady: A callback for when playback is ready to occur
 *  - tick: A callback that is called once per second during playback
 *  - firstTrack: 0-based track to start with
 *
 * Output:
 *  undefined: this doesn't fail; it merely sets events in motion
 */
function initializeCrPlayer(player, musicUrl, hostCanvas, loadProgress, playerIsReady, tick, firstTrack)
{
    playerFile = player;
    playerIsReadyCallback = playerIsReady;
    loadProgressCallback = loadProgress
    tickCallback = tick;
    canvas = hostCanvas;
    crCurrentTrack = firstTrack;

    tickCountdown = audioCtx.sampleRate;

    /* load the music file first */
    var musicFile = new XMLHttpRequest();
    musicFile.addEventListener("progress", musicLoadEvent);
    musicFile.addEventListener("load", musicLoadEvent);
    musicFile.addEventListener("error", musicLoadEvent);
    musicFile.addEventListener("abort", musicLoadEvent);
    musicFile.open("GET", musicUrl);
    musicFile.responseType = "arraybuffer";
    musicFile.send();
}

/*
 * Public function:
 *  enableCrPlayerAudio(enabled)
 *
 * Play/pause function for the player audio.
 *
 * Input:
 *  - enabled: false to pause audio; true to enable audio
 *
 * Output:
 *  - undefined
 */
function enableCrPlayerAudio(enabled)
{
    if (enabled)
    {
        isPaused = false;
        requestAnimationFrame(drawOscope);
    }
    else
    {
        isPaused = true;
    }
}

/*
 * Public function:
 *  enableCrPlayerViz(enabled)
 *
 * Enable/disable the audio visualization.
 *
 * Input:
 *  - enabled: true to display viz; false to disable display
 *
 * Output:
 *  - undefined
 */
function enableCrPlayerViz(enabled)
{
    vizEnabled = enabled;
    if (vizEnabled)
    {
        requestAnimationFrame(drawOscope);
    }
    else
    {
        canvasCtx.fillStyle = 'rgb(0, 0, 0)';
        canvasCtx.fillRect(0, 0, canvasWidth, canvasHeight);
    }
}
