if (process.argv.length < 6)
{
    console.log("USAGE: nodejs cr-test-harness.js <cr-player.js> <music file> <track to play> <out.wav>");
    process.exit(1);
}

var playerLibrary = process.argv[2];
var filename = process.argv[3];
var trackToPlay = process.argv[4];
var outWav = process.argv[5];

var fs = require('fs');
var player = require(process.argv[2]);

var SAMPLE_RATE = 44100;
var WAV_HEADER_SIZE = 44;

var musicBuffer;
var seconds;
var currentChannelDisabled = -1;

function writeSamples()
{
    var j;

    if (seconds === 0)
    {
        outStream.end();
        return;
    }

    ret = player._crPlayerGenerateStereoFrames(context.byteOffset, samples.byteOffset, SAMPLE_RATE);
    console.log(seconds + " sec; crPlayerGenerateStereoFrames() returned " + ret);
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

stat = fs.statSync(filename);
fd = fs.openSync(filename, "r");
musicBuffer = new Buffer(stat.size);
bytesRead = fs.readSync(fd, musicBuffer, 0, stat.size, null);
fs.closeSync(fd);

/* create memory for the context */
var contextSize = player._crPlayerContextSize();
var contextMalloc = player._malloc(contextSize);
var context = new Uint8Array(player.HEAPU8.buffer, contextMalloc, contextSize);

/* initialize the player */
ret = player._crPlayerInitialize(context.byteOffset, SAMPLE_RATE);
console.log("crPlayerInitialize() returned " + ret);

/* create memory for the music */
var musicBufferBytes = new Uint8Array(musicBuffer);
var bytesMalloc = player._malloc(musicBufferBytes.length);
var bytes = new Uint8Array(player.HEAPU8.buffer, bytesMalloc, musicBufferBytes.length);
bytes.set(new Uint8Array(musicBufferBytes.buffer));

/* load the file */
ret = player._crPlayerLoadFile(context.byteOffset, 0, bytes.byteOffset, bytes.length);
console.log("crPlayerLoadFile() returned " + ret);

/* start the track */
ret = player._crPlayerSetTrack(context.byteOffset, trackToPlay);
console.log("crPlayerSetTrack() returned " + ret);

/* how many voices? that count + 1 is the number of seconds to run */
seconds = player._crPlayerGetVoiceCount(context.byteOffset) + 1;
console.log("run the player for " + seconds + " sec");

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

var outStream = fs.createWriteStream(outWav)
outStream.write(header, null, writeSamples);

var outBuffer = new Buffer(SAMPLE_RATE * 2 * 2);

/*
var i, j;
for (i = 0; i < SECONDS; i++)
{
    ret = player._crPlayerGenerateStereoFrames(context.byteOffset, samples.byteOffset, SAMPLE_RATE);

    for (j = 0; j < samplesCount; j++)
    {
        outBuffer.writeInt16LE(samples[j], j * 2);
    }
    outStream.write(outBuffer);
    console.log((i+1) + " sec; crPlayerGenerateStereoFrames() returned " + ret);
}

outStream.end();
*/
