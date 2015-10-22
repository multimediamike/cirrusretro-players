/* Construct a namespace for the Cirrus Retro module */
var cr = {};

/* Module variables */

/* find the audio context */
cr.audioCtx = new (window.AudioContext || window.webkitAudioContext)();
/* for volume control */
cr.gainNode = cr.audioCtx.createGain();

/* other variables pertinent to audio processing */
cr.secondsPerNode = 1.0;
cr.channels = 2;
cr.frameCount = cr.audioCtx.sampleRate * cr.secondsPerNode;
cr.myArrayBuffer = cr.audioCtx.createBuffer(cr.channels, cr.frameCount, cr.audioCtx.sampleRate);
cr.samplesMalloc = null;
cr.samples = null;
cr.source = null;  /* AudioBufferSourceNode */
cr.FRAME_COUNT = 4096;

/* visualization */
cr.vizType = "oscope";
cr.vizTypeBeforeHiding = null;
cr.framesPerSecond = 30;
cr.vizEnabled = true;
cr.vizBufferSize = cr.audioCtx.sampleRate * cr.channels;
cr.vizBuffer = new Int16Array(cr.vizBufferSize);
cr.vizBufferIndex = 0;
cr.canvas = null;
cr.canvasCtx = null;
cr.canvasWidth = 0;
cr.canvasHeight = 0;
cr.loadingBarFadeOut = 0;
cr.audioStarted = false;
cr.firstAudioTimestamp = 0;
cr.nextTimestamp = 0;
cr.FRAMERATE_DELTA = 1.0/cr.framesPerSecond;
cr.currentRed = 250;
cr.currentGreen = 250;
cr.currentBlue = 250;
cr.rInc = -1;
cr.gInc = -2;
cr.bInc = -3;
cr.actualChannels = 2;
cr.vuGradient = null;
cr.vuLastLevel = [0, 0];
cr.vuMax = [0, 0];
cr.vuFallOffDelay = [cr.framesPerSecond / 4, cr.framesPerSecond / 4];
cr.vuFallOffVelocity = [0, 0];

cr.playerFile = null;
cr.currentTrack = 0;
cr.musicResponseBytes = null;
cr.playerContext = null;
cr.isPaused = false;
cr.failureState = false;
cr.playerIsReadyCallback = null;
cr.loadProgressCallback = null;
cr.tickCallback = null;
cr.tickCountdown = 0;  /* when this goes below 0, a second has elapsed */
cr.voiceCount = 0;

/*
 * Private function:
 *  musicLoadEvent(evt)
 *
 * This callback is invoked for music progress/load/error events. When
 * the music is fully loaded, go to the next phase of loading the player.
 */
cr.musicLoadEvent = function(evt)
{
    if (evt.type == "progress")
    {
        /* draw the progress bar on the canvas (if available) */
        if (cr.canvas)
        {
            cr.canvasCtx.fillStyle = 'rgb(180, 180, 180)';
            cr.canvasCtx.fillRect(0, 0,
            cr.canvasWidth * evt.loaded / evt.total,
            cr.canvasHeight);
        }

        /* let the client program know about the progress */
        if (cr.loadProgressCallback)
        {
            cr.loadProgressCallback(evt.loaded, evt.total);
        }
    }
    else if (evt.type == "load")
    {
        cr.loadingBarFadeOut = 180;

        /* copy the response bytes to a typed array */
        cr.musicResponseBytes = new Uint8Array(evt.target.response);

        /* request the player to be loaded */
        var script = document.createElement('script');
        script.src = cr.playerFile;
        script.onload = cr.crPlayerIsLoaded;
        document.head.appendChild(script);
    }
};

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
cr.crPlayerIsLoaded = function()
{
    /* create a private context for the player to use */
    var contextSize = _crPlayerContextSize();
    /* the context size really should be non-zero */
    if (contextSize <= 0)
    {
        cr.playerIsReadyCallback("Problem: context size is " + contextSize);
        return;
    }

    var contextMalloc = Module._malloc(contextSize);
    cr.playerContext = new Uint8Array(Module.HEAPU8.buffer, contextMalloc, contextSize);

    /* initialize the player */
    ret = _crPlayerInitialize(cr.playerContext.byteOffset, cr.audioCtx.sampleRate);
    if (ret != 1)
    {
        cr.playerIsReadyCallback("Problem: player initialization returned " + ret);
        return;
    }

    /* transfer the ArrayBuffer to a UInt8Array */
    var musicBytesMalloc = Module._malloc(cr.musicResponseBytes.byteLength);
    var musicBytes = new Uint8Array(Module.HEAPU8.buffer, musicBytesMalloc, cr.musicResponseBytes.byteLength);
    musicBytes.set(new Uint8Array(cr.musicResponseBytes));

    /* load the file into the player */
    ret = _crPlayerLoadFile(cr.playerContext.byteOffset, 0, musicBytes.byteOffset, musicBytes.length, 0 /* parm needs to go away */);
    if (ret != 1)
    {
        cr.playerIsReadyCallback("Problem: load file operation returned " + ret);
        return;
    }

    /* set the initial track */
    ret = _crPlayerSetTrack(cr.playerContext.byteOffset, cr.currentTrack);
    if (ret == 0)
    {
        cr.playerIsReadyCallback("Problem: set track operation returned " + ret);
        return;
    }
    cr.actualChannels = ret;

    /* validate that the voice count makes sense */
    cr.voiceCount = _crPlayerGetVoiceCount(cr.playerContext.byteOffset);
    if (cr.voiceCount == 0)
    {
        cr.playerIsReadyCallback("Problem: voice count is " + cr.voiceCount);
        return;
    }

    /* tell the host code that the player is ready */
    cr.playerIsReadyCallback(null);
};

cr.setTrack = function(track)
{
    cr.currentTrack = track;
    cr.tickCountdown = cr.audioCtx.sampleRate;

    ret = _crPlayerSetTrack(cr.playerContext.byteOffset, cr.currentTrack);
};

/*
 * Public function:
 *  startAudio()
 *
 * Start the playback, including the visualizer if enabled.
 *
 * Input:
 *  - none
 *
 * Output:
 *  - undefined
 */
cr.startAudio = function()
{
    /* script processor drives the dynamic audio generation */
    scriptNode = cr.audioCtx.createScriptProcessor(cr.FRAME_COUNT, 2, 2);
    scriptNode.onaudioprocess = cr.generateAudioCallback;

    /* Get an AudioBufferSourceNode to play an AudioBuffer */
    cr.source = cr.audioCtx.createBufferSource();

    /* set the buffer in the AudioBufferSourceNode */
    cr.source.buffer = cr.myArrayBuffer;

    /* connect the nodes:
         AudioBufferSourceNode -> ScriptProcessorNode
         ScriptProcessorNode -> GainNode
         GainNode -> audio context destination
    */
    cr.gainNode.gain.value = 1.0;
    cr.source.connect(scriptNode);
    scriptNode.connect(cr.gainNode);
    cr.gainNode.connect(cr.audioCtx.destination);

    cr.vizBufferIndex = 0;
    cr.isPaused = false;

    /* signal the source node to start triggering callbacks which will
     * start generating audio */
    cr.source.start(0);
};

/*
 * Private function:
 *  callback for generating more audio
 */
cr.generateAudioCallback = function(audioProcessingEvent)
{
    /* The output buffer contains the samples that will be modified and played */
    var outputBuffer = audioProcessingEvent.outputBuffer;

    if (cr.isPaused && !cr.failureState)
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
        cr.vizBufferIndex = (outputBuffer.length * 2) % cr.vizBufferSize;

        return;
    }

    /* create an array for the player to use for sample generation */
    var samplesCount = outputBuffer.length * cr.channels;
    var samplesCountInBytes = samplesCount * 2;  /* 2 bytes per sample */
    if (!cr.samplesMalloc || !cr.samples)
    {
        cr.samplesMalloc = Module._malloc(samplesCountInBytes);
        cr.samples = new Int16Array(Module.HEAP16.buffer, cr.samplesMalloc, samplesCount);
    }
    var ret = _crPlayerGenerateStereoFrames(cr.playerContext.byteOffset, cr.samples.byteOffset, outputBuffer.length);
    if (ret == 0)
    {
        console.log("failed to generate frames");
        cr.failureState = true;
        return;
    }

    /* tick accounting */
    cr.tickCountdown -= outputBuffer.length;
    if (cr.tickCountdown < 0)
    {
        cr.tickCountdown += cr.audioCtx.sampleRate;
        if (cr.tickCallback)
            cr.tickCallback();
    }

    /* Loop through the output channels */
    for (var channel = 0; channel < outputBuffer.numberOfChannels; channel++)
    {
        var outputData = outputBuffer.getChannelData(channel);
        var index = cr.vizBufferIndex + channel;

        /* Loop through the input samples */
        for (var sample = 0; sample < outputBuffer.length; sample++)
        {
            var currentSample = cr.samples[channel + sample * 2];

            /* convert data from int16 -> float32 [-1.0 .. 1.0] */
            outputData[sample] = currentSample / 32767.0;

            /* stash data for visualization */
            cr.vizBuffer[index] = currentSample;
            index = (index + 2) % cr.vizBufferSize;
        }
    }
    cr.vizBufferIndex = index;
    if (!cr.audioStarted)
    {
        cr.firstAudioTimestamp = audioProcessingEvent.playbackTime;
        cr.audioStarted = true;
        cr.drawViz(0);
    }
};

/*
 * Private function:
 *  Initialize variables used for the assorted visualizations.
 */
cr.initViz = function()
{
    if (!cr.canvas)
        return;

    cr.canvasCtx = cr.canvas.getContext("2d");
    cr.canvasWidth = cr.canvasCtx.canvas.width;
    cr.canvasHeight = cr.canvasCtx.canvas.height;

    /* wipe canvas */
    cr.canvasCtx.fillStyle = 'rgb(0, 0, 0)';
    cr.canvasCtx.fillRect(0, 0, cr.canvasWidth, cr.canvasHeight);

    /* pertinent to VU meter viz */
    cr.vuGradient = cr.canvasCtx.createLinearGradient(0, 0,
        cr.canvasWidth, 0);
    cr.vuGradient.addColorStop(0.0, "green");
    cr.vuGradient.addColorStop(0.6, "yellow");
    cr.vuGradient.addColorStop(1.0, "red");
};

/*
 * Private function:
 *  Draw a frame of the oscilloscope visualization.
 */
cr.drawOscope = function(timestamp)
{
    if (cr.actualChannels > 1)
    {
        cr.canvasCtx.lineWidth = 1;
        cr.canvasCtx.strokeStyle = 'rgb(255, 255, 255)';
        cr.canvasCtx.beginPath();
        cr.canvasCtx.moveTo(0, cr.canvasHeight / 2);
        cr.canvasCtx.lineTo(cr.canvasWidth, cr.canvasHeight / 2);
        cr.canvasCtx.stroke();
    }

    cr.canvasCtx.beginPath();
    cr.canvasCtx.lineWidth = 2;
    cr.canvasCtx.strokeStyle = 'rgb(' + cr.currentRed + ', ' + cr.currentGreen + ', ' + cr.currentBlue + ')';
    var segment = Math.round((cr.audioCtx.currentTime - cr.firstAudioTimestamp) * cr.vizBufferSize % cr.vizBufferSize);
    var divisor = 32768.0 / (cr.canvasHeight / (2 * cr.actualChannels));
    for (var i = 0; i < cr.actualChannels; i++)
    {
        var index = segment + i;
        var center = (cr.canvasHeight / (2 * cr.actualChannels)) + (i * cr.canvasHeight / cr.actualChannels);

        /* handle sample 0 */
        var y = center - (cr.vizBuffer[index] / divisor);
        index += 2;
        cr.canvasCtx.moveTo(0, y);

        /* plot the rest of the samples */
        for (var x = 1; x < cr.canvasWidth; x++)
        {
            y = center - (cr.vizBuffer[index] / divisor);
            index += 2;
            cr.canvasCtx.lineTo(x, y);
        }
        cr.canvasCtx.stroke();
    }

    /* play with colors */
    if (cr.currentRed < 150 || cr.currentRed > 250)
        cr.rInc *= -1;
    if (cr.currentGreen < 150 || cr.currentGreen > 250)
        cr.gInc *= -1;
    if (cr.currentBlue < 150 || cr.currentBlue > 250)
        cr.bInc *= -1;
    cr.currentRed += cr.rInc;
    cr.currentGreen += cr.gInc;
    cr.currentBlue += cr.bInc;
};

/*
 * Private function:
 *  Draw a frame of the VU visualization.
 */
cr.drawVUMeter = function(timestamp)
{
    var MAX_VU = 18000;

    /* compute the RMS of the amplitude samples */
    var segment = Math.round((cr.audioCtx.currentTime - cr.firstAudioTimestamp) * cr.vizBufferSize % cr.vizBufferSize);
    var sumOfSquares = [0, 0];
    var rmsWidth = [];
    for (var i = 0; i < cr.actualChannels; i++)
    {
        var index = segment + i;
        var center = (cr.canvasHeight / (2 * cr.actualChannels)) + (i * cr.canvasHeight / cr.actualChannels);
        var periodSize = cr.audioCtx.sampleRate / cr.framesPerSecond;
        for (var x = 0; x < periodSize; x++)
        {
            sumOfSquares[i] += cr.vizBuffer[index] * cr.vizBuffer[index];
            index += 2;
        }

        /* compute the RMS and width of VU bar */
        var rms = Math.sqrt(sumOfSquares[i] / periodSize)
        rmsWidth[i] = rms * cr.canvasWidth / MAX_VU;

        /* if the calculation came up with NaN, use the previous level in
         * order to mitigate blanking effects in the viz */
        if (isNaN(rmsWidth[i]))
            rmsWidth[i] = cr.vuLastLevel[i];
        cr.vuLastLevel[i] = rmsWidth[i];
    }

    /* fill the gradients */
    cr.canvasCtx.fillStyle = cr.vuGradient;
    if (cr.actualChannels === 1)
    {
        cr.canvasCtx.fillRect(
            0, 0,
            rmsWidth[0],
            cr.canvasHeight);
    }
    else
    {
        cr.canvasCtx.fillRect(
            0, 0,
            rmsWidth[0],
            cr.canvasHeight / 2);
        cr.canvasCtx.fillRect(
            0,
            cr.canvasHeight / 2,
            rmsWidth[1],
            cr.canvasHeight);
    }

    /* remainder of the drawing will be white lines */
    cr.canvasCtx.lineWidth = 2;
    cr.canvasCtx.strokeStyle = 'rgb(255, 255, 255)';

    /* draw the max bars */
    for (var i = 0; i < cr.actualChannels; i++)
    {
        /* check if there's a new maximum */
        if (rmsWidth[i] >= cr.vuMax[i])
        {
            cr.vuMax[i] = rmsWidth[i];
            cr.vuFallOffDelay[i] = cr.framesPerSecond / 4;
            cr.vuFallOffVelocity[i] = 1.2;
        }

        /* check if it's time to start letting the bar fall */
        if (cr.vuFallOffDelay[i] > 0)
            cr.vuFallOffDelay[i]--;
        else
        {
            cr.vuMax[i] -= Math.round(cr.vuFallOffVelocity[i]);
            if (cr.vuMax[i] < 0)
                cr.vuMax[i] = 0;
            cr.vuFallOffVelocity[i] *= 1.2;
        }

        cr.canvasCtx.beginPath();
        cr.canvasCtx.moveTo(cr.vuMax[i], i * (cr.canvasHeight / 2));
        cr.canvasCtx.lineTo(cr.vuMax[i], (i + 1) * cr.canvasHeight / (1 * cr.actualChannels));
        cr.canvasCtx.stroke();
    }

    /* draw the channel dividing line last, if the audio is stereo */
    if (cr.actualChannels > 1)
    {
        cr.canvasCtx.beginPath();
        cr.canvasCtx.moveTo(0, cr.canvasHeight / 2);
        cr.canvasCtx.lineTo(cr.canvasWidth, cr.canvasHeight / 2);
        cr.canvasCtx.stroke();
    }
};

/*
 * Private function:
 *  Draw a frame of the selected visualization.
 */
cr.drawViz = function(timestamp)
{
    if (!cr.vizEnabled || !cr.canvas || cr.isPaused)
        return;

    timestamp /= 1000;
    drawVisual = requestAnimationFrame(cr.drawViz);

    if (cr.nextTimestamp)
    {
        if (timestamp <= cr.nextTimestamp)
            return;
    }
    else if (!cr.audioStarted || timestamp < cr.firstAudioTimestamp)
    {
        return;
    }

    if (cr.loadingBarFadeOut > 0)
    {
        cr.loadingBarFadeOut -= 6;
        var c = cr.loadingBarFadeOut.toString();
        var colorStr = 'rgb(' + c + ',' + c + ',' + c + ')';
        cr.canvasCtx.fillStyle = colorStr;
    }
    else
    {
        cr.canvasCtx.fillStyle = 'rgb(0, 0, 0)';
    }
    cr.canvasCtx.fillRect(0, 0, cr.canvasWidth, cr.canvasHeight);

    /* draw the visualization */
    if (cr.vizType == "oscope")
        cr.drawOscope(timestamp);
    else if (cr.vizType == "vumeter")
        cr.drawVUMeter(timestamp);

    /* figure out the next timestamp */
    cr.nextTimestamp = timestamp + cr.FRAMERATE_DELTA;
};

/*
 * Public function:
 *  changeViz(viz)
 *
 * Change the current visualization.
 *
 * Input:
 *  - vizType: A string with either "oscope", "vumeter", or "none". Any other
 *     values have the same effect as "none".
 *
 * Output:
 *  - undefined.
 */
cr.changeViz = function(vizType)
{
    cr.vizType = vizType;

    if (cr.vizType == "none")
    {
        /* if the parameter asks to disable the visualization, set a flag
         * that will disable animation callbacks; then, clear the canvas */
        cr.vizEnabled = false;
        cr.canvasCtx.fillStyle = 'rgb(0, 0, 0)';
        cr.canvasCtx.fillRect(0, 0, cr.canvasWidth, cr.canvasHeight);
    }
    else if (!cr.vizEnabled)
    {
        /* if the viz was previously disabled, it is now being turned back
         * on; thus, ask for animation callbacks again */
        cr.vizEnabled = true;
        requestAnimationFrame(cr.drawViz);
    }
}

/*
 * Public function:
 *  hideViz(viz)
 *
 * Hide the visualization. While the drawViz() function is provided for
 * allowing the user the manually disable the visualization, this
 * function is intended to be invoked when the tab hosting the visualization
 * is hidden from user view, thus making it a pointlessly expensive
 * operation to continue rendering the viz.
 *
 * This function remembers the currently selected viz and will restore it
 * when the hideViz(true) is called.
 *
 * Input:
 *  - hidden: true or false
 *
 * Output:
 *  - undefined.
 */
cr.hideViz = function(hidden)
{
    if (hidden)
    {
        cr.vizTypeBeforeHiding = cr.vizType;
        cr.changeViz("none");
    }
    else
        cr.changeViz(cr.vizTypeBeforeHiding);
}

/*
 * Public function:
 *  initializePlayer(player, musicUrl, hostCanvas, loadProgress, playerIsReady, tick, firstTrack)
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
cr.initializePlayer = function(player, musicUrl, hostCanvas, loadProgress, playerIsReady, tick, firstTrack)
{
    cr.playerFile = player;
    cr.playerIsReadyCallback = playerIsReady;
    cr.loadProgressCallback = loadProgress
    cr.tickCallback = tick;
    cr.canvas = hostCanvas;
    cr.currentTrack = firstTrack;

    cr.tickCountdown = cr.audioCtx.sampleRate;

    /* initialize the visualization */
    cr.initViz();

    /* load the music file first */
    var musicFile = new XMLHttpRequest();
    musicFile.addEventListener("progress", cr.musicLoadEvent);
    musicFile.addEventListener("load", cr.musicLoadEvent);
    musicFile.addEventListener("error", cr.musicLoadEvent);
    musicFile.addEventListener("abort", cr.musicLoadEvent);
    musicFile.open("GET", musicUrl);
    musicFile.responseType = "arraybuffer";
    musicFile.send();
};

/*
 * Public function:
 *  enablePlayerAudio(enabled)
 *
 * Play/pause function for the player audio.
 *
 * Input:
 *  - enabled: false to pause audio; true to enable audio
 *
 * Output:
 *  - undefined
 */
cr.enablePlayerAudio = function(enabled)
{
    if (enabled)
    {
        cr.isPaused = false;
        requestAnimationFrame(cr.drawViz);
    }
    else
    {
        cr.isPaused = true;
    }
};

/*
 * Public function:
 *  setVolume(volumeLevel)
 *
 * Set the audio output volume.
 *
 * Input:
 *  - volumeLevel: volume on a scale of 0..255
 *
 * Output:
 *  - undefined
 */
cr.setVolume = function(volumeLevel)
{
    if (volumeLevel < 1)
        volumeLevel = 1;
    else if (volumeLevel > 255)
        volumeLevel = 255;

    cr.gainNode.gain.value = volumeLevel / 255.0;
};

/*
 * Public function:
 *  getVoiceInfo()
 *
 * Input:
 *  - None
 *
 * Output:
 *  - Returns an object with the following attributes:
 *     canBeToggled: a Boolean indicating whether voices can be toggled
 *     voiceCount: the number of voices comprising the music
 */
cr.getVoiceInfo = function()
{
    var info = Object();

    info.canBeToggled = _crPlayerVoicesCanBeToggled(cr.playerContext.byteOffset);
    info.voiceCount = _crPlayerGetVoiceCount(cr.playerContext.byteOffset);

    return info;
};

/*
 * Public function:
 *  getVoiceName()
 *
 * Input:
 *  - voice number, indexed from 0
 *
 * Output:
 *  - a string containing the voice name
 */
cr.getVoiceName = function(voice)
{
    return Pointer_stringify(_crPlayerGetVoiceName(cr.playerContext.byteOffset, voice));
};

/*
 * Public function:
 *  setVoiceState(voice, enabled)
 *
 * Input:
 *  - voice: a voice number, indexed from 0
 *  - enabled: Boolean to enable the voice
 *
 * Output:
 *  - undefined
 */
cr.setVoiceState = function(voice, enabled)
{
    if (voice >= 0 && voice < cr.voiceCount)
        _crPlayerSetVoiceState(cr.playerContext.byteOffset, voice, enabled ? 1 : 0);
};

