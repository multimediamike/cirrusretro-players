var fs = require('fs');
var crypto = require('crypto');

var outputFile = "out.wav";
var SAMPLE_RATE = 44100;
var WAV_HEADER_SIZE = 44;

if (process.argv.length < 3)
{
    console.log("USAGE: nodejs cr-test-harness.js <test-spec.json> [keep]");
    console.log("  [keep] optional argument keeps the output wav file");
    process.exit(1);
}

var testSpecFilename = process.argv[2];
var keepOutput = false;
if (process.argv.length >= 4 && process.argv[3] == "keep")
{
    keepOutput = true;
    console.log("keeping the output file (" + outputFile + ")");
}

testSpec = JSON.parse(fs.readFileSync(testSpecFilename).toString());

var player = require(testSpec['player']);

var musicBuffer;
var seconds;
var currentChannelDisabled = -1;

function writeSamples()
{
    var j;

    /* check if it's time to complete the test */
    if (seconds === 0)
    {
        /* finished writing the file */
        outStream.end();

        /* checksum the file */
        var md5sum = crypto.createHash("md5");
        var output = fs.readFileSync(outputFile);
        md5sum.update(output);
        var digest = md5sum.digest('hex');
        if (digest != testSpec['expected-md5'])
        {
            console.log("md5 hash of output differs");
            console.log(" expected: " + testSpec['expected-md5']);
            console.log("      got: " + digest);
            process.exit(1);
        }

        /* delete the output file unless instructed otherwise */
        if (!keepOutput)
        {
            fs.unlinkSync(outputFile);
        }

        /* cleanly shutdown the player */
        player._crPlayerCleanup(context.byteOffset);
        return;
    }

    ret = player._crPlayerGenerateStereoFrames(context.byteOffset, samples.byteOffset, SAMPLE_RATE);
    if (ret != 1)
    {
        console.log("crPlayerGenerateStereoFrames() returned " + ret);
        process.exit(1);
    }
    seconds -= 1;
    for (j = 0; j < samplesCount; j++)
    {
        outBuffer.writeInt16LE(samples[j], j * 2);
    }
    outStream.write(outBuffer, null, writeSamples);

    /* deal with channel toggles */
    if (currentChannelDisabled >= 0)
    {
        player._crPlayerSetVoiceState(context.byteOffset, currentChannelDisabled, 1);
    }
    currentChannelDisabled += 1;
    player._crPlayerSetVoiceState(context.byteOffset, currentChannelDisabled, 0);
}

if (!fs.existsSync(testSpec['filename']))
{
    console.log(testSpec['filename'] + " does not exist");
    process.exit(1);
}

musicBuffer = fs.readFileSync(testSpec['filename']);

/* create memory for the context */
var contextSize = player._crPlayerContextSize();
var contextMalloc = player._malloc(contextSize);
var context = new Uint8Array(player.HEAPU8.buffer, contextMalloc, contextSize);

/* initialize the player */
ret = player._crPlayerInitialize(context.byteOffset, SAMPLE_RATE);
if (ret != 1)
{
    console.log("crPlayerInitialize() returned " + ret);
    process.exit(1);
}

/* create memory for the music */
var musicBufferBytes = new Uint8Array(musicBuffer);
var bytesMalloc = player._malloc(musicBufferBytes.length);
var bytes = new Uint8Array(player.HEAPU8.buffer, bytesMalloc, musicBufferBytes.length);
bytes.set(new Uint8Array(musicBufferBytes.buffer));

/* load the file */
ret = player._crPlayerLoadFile(context.byteOffset, 0, bytes.byteOffset, bytes.length);
if (ret != 1)
{
    console.log("crPlayerLoadFile() returned " + ret);
    process.exit(1);
}

/* start the track */
ret = player._crPlayerSetTrack(context.byteOffset, testSpec['track']);
if (ret != testSpec['expected-output-channels'])
{
    console.log("crPlayerSetTrack() returned " + ret + " (expected " + testSpec['expected-output-channels'] + ")");
    process.exit(1);
}

/* how many voices? that count + 1 is the number of seconds to run */
var voiceCount = player._crPlayerGetVoiceCount(context.byteOffset);
if (voiceCount != testSpec['expected-channel-count'])
{
    console.log("crPlayerGetVoiceCount() returned " + voiceCount + " (expected " + testSpec['expected-channel-count'] + ")");
    process.exit(1);
}
seconds = voiceCount + 1;

/* create an array for the player to use for sample generation */
var samplesCount = SAMPLE_RATE * 2;  /* 2 channels / frame */
var samplesCountInBytes = samplesCount * 2;  /* 2 bytes/sample */
var samplesMalloc = player._malloc(samplesCountInBytes);
var samples = new Int16Array(player.HEAP16.buffer, samplesMalloc, samplesCount);

/* create the WAV header and write to disk */
byteCount = SAMPLE_RATE * 2 * 2 * seconds;
var header = new Buffer(WAV_HEADER_SIZE);

header.writeUInt8(0x52, 0);  /* R */
header.writeUInt8(0x49, 1);  /* I */
header.writeUInt8(0x46, 2);  /* F */
header.writeUInt8(0x46, 3);  /* F */
header.writeUInt32LE(byteCount + WAV_HEADER_SIZE - 8, 4);  /* header size */
header.writeUInt8(0x57, 8);  /* W */
header.writeUInt8(0x41, 9);  /* A */
header.writeUInt8(0x56, 10);  /* V */
header.writeUInt8(0x45, 11);  /* E */
header.writeUInt8(0x66, 12);  /* f */
header.writeUInt8(0x6D, 13);  /* m */
header.writeUInt8(0x74, 14);  /* t */
header.writeUInt8(0x20, 15);  /*   */
header.writeUInt32LE(0x10, 16);  /* size of 'fmt ' header */
header.writeUInt16LE(0x01, 20);  /* integer PCM */
header.writeUInt16LE(0x02, 22);  /* channels */
header.writeUInt32LE(SAMPLE_RATE, 24);  /* sample rate */
header.writeUInt32LE(SAMPLE_RATE * 2 * 2, 28);  /* bytes/second */
header.writeUInt16LE(4, 32);  /* block alignment */
header.writeUInt16LE(16, 34);  /* bits/sample */
header.writeUInt8(0x64, 36);  /* d */
header.writeUInt8(0x61, 37);  /* a */
header.writeUInt8(0x74, 38);  /* t */
header.writeUInt8(0x61, 39);  /* a */
header.writeUInt32LE(byteCount, 40);  /* byte count */

var outStream = fs.createWriteStream(outputFile)
outStream.write(header, null, writeSamples);

var outBuffer = new Buffer(SAMPLE_RATE * 2 * 2);

