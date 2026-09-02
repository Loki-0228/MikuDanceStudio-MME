// ===========================================================================
// VA 0x00418500 - LoadWaveFile       (original: sub_418500, 0x241 bytes)
// VA 0x004C2660 - WaveCtxReset       (original: sub_4C2660)
// VA 0x004C26F0 - WaveFindDataChunk  (original: sub_4C26F0)
// VA 0x004C2760 - WaveStartPlayback  (original: sub_4C2760)
// VA 0x004C2960 - WaveStreamRead     (original: sub_4C2960)
// VA 0x004C2C90 - WaveStreamFeed     (original: sub_4C2C90)
// VA 0x004C2F70 - WaveLoadFile       (original: sub_4C2F70)
// Playback runtime trio, NOT in this TU (see the note below the field map):
// 0x4C2CE0 WaveFeedThread / 0x4C34A0 Sub4C34A0 / 0x4C3530 Sub4C3530.
// ===========================================================================
// The WAV open/play chain behind menu 0xCE ("load WAV file").
//
// 0x025C audio/timeline context object (reached as app+0xCC,
// app+0xCC (sub025c); ctor 0x4C2450, DirectSound init 0x4C2470 =
// InitTimelineAudio in src/window/ui_init.cpp).  Field map recovered from
// sub_4C2660/4C26F0/4C2760/4C2960/4C2F70:
//   +0x00/+0x04  waveform max/min byte arrays (malloc'd, +0x250 entries;
//                drawn by TimelineDrawTicks 0x4C2A00)
//   +0x08        timeline strip HDC
//   +0x0C        main HWND
//   +0x10        IDirectSound*
//   +0x14        IDirectSoundBuffer* (streaming buffer)
//   +0x18..+0x28 WAVEFORMATEX (tag@18, channels@1a, rate@1c, avgBytes@20,
//                blockAlign@24, bits@26, cbSize@28 = 0x12)
//   +0x2C        FILE* header-parse stream
//   +0x30        resolved wave path wchar_t[256]
//   +0x230       streaming buffer bytes (2 * nAvgBytesPerSec)
//   +0x234       _beginthread handle of the feed thread (0x4C34A0)
//   +0x238       'data' chunk size in bytes
//   +0x23C       write cursor (bytes fed from the data chunk)
//   +0x240       'data' chunk file offset (ftell after the size dword)
//   +0x244       CreateFileW read handle
//   +0x248       feed-thread stop flag (0 idle / 1 stop / 2 done)
//   +0x24C       feed-failure counter
//   +0x250       waveform column count
//   +0x254       English UI flag copy
//
// 0x418500 is __thiscall(app); the wave path is NOT a parameter - it reads
// app+0xD0 (kWcsWavpath), so this overload supersedes the old path-taking
// stub declaration (kept in stubs.cpp untouched, no longer referenced).
//
// Playback runtime trio - PORTED, but in src/app/subsystem_init.cpp (phase
// scaffolding cleanup TU), not here; do not re-port in this file or the link
// will see duplicate Sub4C34A0/Sub4C3530 symbols:
//   0x4C2CE0  WaveFeedThread(void*) - the _beginthread proc: per-half Lock/
//             WaveStreamFeed/Unlock loop (half = 1 s = nAvgBytesPerSec) with
//             DSERR_BUFFERLOST -> Restore -> single Lock retry, a play-cursor
//             chase (GetCurrentPosition + Sleep(10) x10), stop-flag checks
//             and the Stop/Release/ctx+0x14=0/stopFlag=2/_endthread epilogue.
//   0x4C34A0  Sub4C34A0(this, double t) - guard on the streaming buffer,
//             stopFlag/failureCount = 0, SetFilePointer(fileHandle,
//             dataOffset + (int)(avg*t) - (int)(avg*t)%nBlockAlign,
//             FILE_BEGIN), _beginthread(WaveFeedThread, 0, this) -> +0x234.
//   0x4C3530  Sub4C3530(this, double t) - KillTimer(hwnd, 100),
//             CloseDataFile, WaveStartPlayback, buffer->SetVolume(ctx+0x258)
//             [IDirectSoundBuffer vtable slot 15; the 2026-09 audit note
//             "SetCurrentPosition" was wrong - confirmed against both the
//             x86 vtable offset 0x3C and the x64 twin slot +120], then
//             Sub4C34A0(this, t), SetTimer(hwnd, 100, 33 ms, null).
// x64 twins (behavior basis): 0x7FF7CB4FAD40 / 0x7FF7CB4FAB80 /
// 0x7FF7CB4FAC20.  Call sites (already wired): Sub4C34A0 from
// src/app/playback_catchup.cpp and src/window/command_control_400.cpp
// (x86 0x46F383 / 0x4876CA, time = the +0x9E654 start-seconds float);
// Sub4C3530 from ui_frame_step.cpp StepFrame (covers x86 0x431296/0x431666),
// ui_editor_click.cpp (0x44A834) and ui_mouse_misc.cpp (0x44AEA8).
// =========================================================================//
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>   // WAVEFORMATEX for dsound.h
#include <dsound.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/ported_funcs.hpp"
#include "mikudancestudio/wave_audio_context.hpp"

namespace mikudancestudio {
namespace {

// .rdata double constants used by the 0x4C2F70 waveform maths.
constexpr double kDbl531720 = 0.03333299979567528;   // ~1/30 (dbl @0x531720)
constexpr double kDbl52EAB0 = 13.0;                  // dbl @0x52EAB0
constexpr double kDbl531718 = -390.0;                // dbl @0x531718
constexpr float  kFlt52B9F0 = 4294967296.0f;         // 2^32 unsigned fixup

// Diagnostic-only file trace (gated by MIKUDANCESTUDIO_TRACE_WAVE).
bool MIKUDANCESTUDIO_WAVE_TRACE() { return getenv("MIKUDANCESTUDIO_TRACE_WAVE") != nullptr; }
void WaveTrace(const char* fmt, ...) {
    FILE* tf = fopen(getenv("MIKUDANCESTUDIO_TRACE_WAVE"), "a");
    if (!tf) return;
    va_list ap; va_start(ap, fmt);
    vfprintf(tf, fmt, ap);
    va_end(ap);
    fclose(tf);
}

// 0x52BB2C: "DirectSoundの初期化に失敗しているため、WAVは鳴らせません"
static const char kMsgWaveNoSoundJp[] =
    "DirectSound\x82\xCC\x8F\x89\x8A\xFA\x89\xBB\x82\xC9\x8E\xB8\x94\x73"
    "\x82\xB5\x82\xC4\x82\xA2\x82\xE9\x82\xBD\x82\xDF\x81\x41WAV\x82\xCD"
    "\x96\xC2\x82\xE7\x82\xB9\x82\xDC\x82\xB9\x82\xF1";
// 0x52BB68: "DirectSoundエラー"
static const char kMsgWaveErrCaptionJp[] =
    "DirectSound\x83\x47\x83\x89\x81\x5B";
// 0x531728: "WAVEファイルが見つかりません"
static const char kMsgWaveNotFoundJp[] =
    "WAVE\x83\x74\x83\x40\x83\x43\x83\x8B\x82\xAA\x8C\xA9\x82\xC2\x82\xA9"
    "\x82\xE8\x82\xDC\x82\xB9\x82\xF1";
// 0x531748: "WAVE読込"
static const char kMsgWaveCaptionJp[] = "WAVE\x93\xC7\x8D\x9E";
// 0x5316D0: "WAVE(PCM)ファイルではありません"
static const char kMsgWaveNotPcmJp[] =
    "WAVE(PCM)\x83\x74\x83\x40\x83\x43\x83\x8B\x82\xC5\x82\xCD\x82\xA0"
    "\x82\xE8\x82\xDC\x82\xB9\x82\xF1";

}  // namespace

// ---------------------------------------------------------------------------
// VA 0x004C2660 - WaveCtxReset: zero the four read-context fields.
// __thiscall(ctx); the tail (jmp) target of CloseDataFile (0x4C2680).
// ---------------------------------------------------------------------------
void WaveCtxReset(void* obj) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    audio->parseStream = nullptr;               // 0x4C2662 (+0x2C)
    audio->feedThread = 0;                      // 0x4C2665 (+0x234)
    audio->streamingBuffer = nullptr;           // 0x4C266B (+0x14)
    audio->fileHandle = nullptr;                // 0x4C266E (+0x244)
}

// ---------------------------------------------------------------------------
// VA 0x004C26F0 - WaveFindDataChunk(this, FILE*): scan the RIFF stream with
// getc() until the literal bytes 'd','a','t','a' are consumed, then fread
// the chunk size into ctx+0x238, reset the write cursor and record ftell()
// (the data-chunk file offset) in ctx+0x240.  No error checking in the
// original: a stream without a 'data' chunk spins forever on EOF (-1).
// ---------------------------------------------------------------------------
void WaveFindDataChunk(void* obj, FILE* stream) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    for (;;) {
        do {
        } while (getc(stream) != 'd');          // 0x4C26F8
        if (getc(stream) != 'a') continue;      // 0x4C2701
        if (getc(stream) != 't') continue;      // 0x4C270F
        if (getc(stream) != 'a') continue;      // 0x4C271D
        break;
    }
    std::fread(&audio->dataSize, 4, 1, stream);                    // 0x4C273C
    audio->readCursor = 0;                                        // 0x4C2742
    audio->dataOffset = static_cast<std::int32_t>(std::ftell(stream));
}

// ---------------------------------------------------------------------------
// VA 0x004C2960 - WaveStreamRead(this, buf, bytes): ReadFile at most `bytes`
// (clamped to the remaining data-chunk bytes) into buf, advance the write
// cursor and zero-fill the tail (0 for 16-bit, 0x80 for 8-bit silence).
// Returns 1 on a full read.
// ---------------------------------------------------------------------------
bool WaveStreamRead(void* obj, void* buf, int bytes) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    const std::int32_t cursor = audio->readCursor;
    const std::int32_t size = audio->dataSize;
    std::int32_t toRead;                                   // 0x4C2975..0x4C2982
    if (cursor + bytes < size)
        toRead = bytes;
    else
        toRead = size - cursor;
    DWORD got = 0;
    if (!ReadFile(audio->fileHandle, buf,
                  static_cast<DWORD>(toRead), &got, nullptr))       // 0x4C2996
        got = 0;
    audio->readCursor += static_cast<std::int32_t>(got);           // 0x4C29A0
    if (static_cast<std::int32_t>(got) >= bytes)
        return true;                                               // 0x4C29ED
    const unsigned short bits = audio->format.wBitsPerSample;
    if (bits == 16)                                                // 0x4C29B2
        memset(static_cast<unsigned char*>(buf) + got, 0,
               static_cast<size_t>(bytes - got));
    else if (bits == 8)                                            // 0x4C29CC
        memset(static_cast<unsigned char*>(buf) + got, 0x80,
               static_cast<size_t>(bytes - got));
    return false;
}

// ---------------------------------------------------------------------------
// VA 0x004C2C90 - WaveStreamFeed(this, buf, bytes, buf2, bytes2): read
// (buf, bytes) and then, when buf2 != 0, (buf2, bytes2); bumps the failure
// counter ctx+0x24C and returns 0 on any short read.
// ---------------------------------------------------------------------------
bool WaveStreamFeed(void* obj, void* buf, int bytes, void* buf2,
                    int bytes2) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    if (!WaveStreamRead(audio, buf, bytes)) {                      // 0x4C29C1
        ++audio->failureCount;                                     // 0x4C2CA8
        return false;
    }
    if (buf2 != nullptr) {                                         // 0x4C2CBB
        if (!WaveStreamRead(audio, buf2, bytes2)) {                // 0x4C2CC5
            ++audio->failureCount;
            return false;
        }
    }
    return true;                                                   // 0x4C2CCE
}

// ---------------------------------------------------------------------------
// VA 0x004C2F70 - WaveLoadFile(this, path, paths): the actual WAV load.
//   1. CloseDataFile if a parse stream is open; zero the format block.
//   2. ResolveUserFilePath(paths, path) -> wcscpy_s into ctx+0x30.
//   3. fopen(path, "r"); RIFF/'PCM tag == 1' checks; WAVEFORMATEX field
//      reads; WaveFindDataChunk; CreateFileW + SetFilePointer(data offset);
//      fclose.
//   4. Build the waveform min/max display arrays (ctx+0x00/+0x04,
//      ctx+0x250 entries) by streaming the whole file through the scratch
//      buffer, one column per `colBytes` bytes.
// Returns 1 on success (the message boxes report the failure paths).
// ---------------------------------------------------------------------------
bool WaveLoadFile(void* obj, const wchar_t* path,
                  PathResolutionWorkspace& paths) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    HWND hwnd = audio->mainWindow;
    const unsigned char english = audio->englishUI;

    if (audio->parseStream != nullptr)                              // 0x4C2F8B
        CloseDataFile(audio);                                      // 0x4C2680
    std::memset(&audio->format, 0, sizeof audio->format);           // 0x4C2FA0

    ResolveUserFilePath(paths, path);                                  // 0x4089F0
    wcscpy_s(audio->path, 0x100, paths.resolvedPath);              // 0x506292

    FILE*& stream = audio->parseStream;
    _wfopen_s(&stream, audio->path, L"rb");                         // 0x50801A
    if (stream == nullptr) {                                       // 0x4C2FDE
        MessageBoxA(hwnd,
                    english ? "Cannot find Wave file!!"
                            : kMsgWaveNotFoundJp,                  // 0x531728
                    english ? "open wave" : kMsgWaveCaptionJp,     // 0x531748
                    0);
        return false;                                              // 0x4C3489
    }

    char riff[4] = {};
    std::fread(riff, 4, 1, stream);                                // 0x4C3017
    bool pcm = riff[0] == 'R' && riff[1] == 'I' &&                 // 0x4C301F
               riff[2] == 'F' && riff[3] == 'F';
    if (pcm) {
        std::fseek(stream, 0x10, SEEK_CUR);                        // 0x4C3052
        unsigned short tag = 0;
        std::fread(&tag, 2, 1, stream);                            // 0x4C3063
        pcm = tag == 1;                                            // 0x4C306B
    }
    if (!pcm) {
        // 0x4C3455: "This is not WAVE(PCM) file" (0x5316F0/0x5316D0),
        // caption "WAVE" (0x53170C)
        MessageBoxA(hwnd,
                    english ? "This is not WAVE(PCM) file"
                            : kMsgWaveNotPcmJp,
                    "WAVE", 0);
        return false;                                              // 0x4C3489
    }

    std::memset(&audio->format, 0, sizeof audio->format);           // 0x4C3081
    audio->format.wFormatTag = WAVE_FORMAT_PCM;                     // 0x4C309A
    audio->format.cbSize = 0x12;                                    // 0x4C30A0
    std::fread(&audio->format.nChannels, 2, 1, stream);             // 0x4C30A6
    std::fread(&audio->format.nSamplesPerSec, 4, 1, stream);        // 0x4C30B6
    std::fread(&audio->format.nAvgBytesPerSec, 4, 1, stream);
    std::fread(&audio->format.nBlockAlign, 2, 1, stream);
    std::fread(&audio->format.wBitsPerSample, 2, 1, stream);        // 0x4C30EE
    WaveFindDataChunk(audio, stream);                               // 0x4C30F6

    audio->bufferBytes = static_cast<std::int32_t>(
        audio->format.nAvgBytesPerSec * 2);                         // 0x4C310C
    HANDLE file = CreateFileW(audio->path,
                              GENERIC_READ, 1, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);                            // 0x4C311C
    audio->fileHandle = file;                                      // 0x4C312E
    SetFilePointer(file, audio->dataOffset, nullptr,
                   FILE_BEGIN);                                    // 0x4C3134
    std::fclose(stream);                                           // 0x4C313D
    stream = nullptr;
    if (file == nullptr)                                           // 0x4C3145
        return false;                                              // 0x4C347F

    free(audio->waveformMax);                                      // 0x4C3158
    free(audio->waveformMin);                                      // 0x4C3167

    // bytes per waveform column: (int)(avgBytes * (1/30) / 13 / blockAlign)
    // then * blockAlign (x87 sequence 0x4C317B..0x4C31A4; _ftol truncates).
    double avgD = static_cast<double>(audio->format.nAvgBytesPerSec);
    if (avgD < 0.0)
        avgD += kFlt52B9F0;                                        // 0x4C327C fixup
    const int blockAlign = audio->format.nBlockAlign;
    const int colUnits =
        static_cast<int>(avgD * kDbl531720 / kDbl52EAB0 / blockAlign);
    const int colBytes = colUnits * blockAlign;

    unsigned char* scratch = static_cast<unsigned char*>(malloc(colBytes));
    memset(scratch, 0, static_cast<size_t>(colBytes));              // 0x4C31BE

    const std::int32_t columns =
        audio->dataSize / colBytes + 1;                             // 0x4C31C5
    audio->waveformColumns = columns;                              // 0x4C31CB
    unsigned char* arrMax = static_cast<unsigned char*>(malloc(columns));
    memset(arrMax, 0, static_cast<size_t>(columns));
    audio->waveformMax = arrMax;                                   // 0x4C31E0
    unsigned char* arrMin = static_cast<unsigned char*>(malloc(columns));
    memset(arrMin, 0, static_cast<size_t>(columns));
    audio->waveformMin = arrMin;                                   // 0x4C31FD

    // channel stride 0x4C3208: mono -> (blockAlign != 1), else
    // (blockAlign != 2) + 2  (0/1 = 8/16-bit mono, 2/3 = 8/16-bit stereo).
    const unsigned short channels = audio->format.nChannels;
    const int k = channels == 1 ? (blockAlign != 1 ? 1 : 0)
                                : (blockAlign != 2 ? 3 : 2);

    if (columns > 0) {                                             // 0x4C323C
        int i = 0;                                                 // 0x4C3238
        int bytesSoFar = 0;                                        // ebp
        do {
            // x87 0x4C3264..0x4C32A6: t = trunc(avg * (i / -390.0));
            // skip = (unsigned)(-t - bytesSoFar) / blockAlign
            double d = static_cast<double>(audio->format.nAvgBytesPerSec);
            if (d < 0.0)
                d += kFlt52B9F0;
            const long long t = static_cast<long long>(
                d * (static_cast<double>(i) / kDbl531718));
            const unsigned skipBlocks =
                static_cast<unsigned>(-static_cast<int>(t) - bytesSoFar) /
                static_cast<unsigned>(blockAlign);                 // 0x4C32AA
            if (static_cast<int>(skipBlocks) > 0) {                // 0x4C32B0
                const int skip = blockAlign * static_cast<int>(skipBlocks);
                WaveStreamFeed(audio, scratch, skip, nullptr, 0);  // 0x4C32C1
                bytesSoFar += skip;                                // 0x4C32C6
            }
            const bool ok =
                WaveStreamFeed(audio, scratch, colBytes, nullptr, 0); // 0x4C32D4
            bytesSoFar += colBytes;                                // 0x4C32D9

            int maxV = 0, minV = 0;                                // 0x4C32DF
            for (int off = 0; off < colBytes; off += blockAlign) { // 0x4C3300
                int v1, v2;
                if (k == 3) {          // 16-bit stereo 0x4C3309
                    v1 = (static_cast<signed char>(scratch[off + 1]) << 8) +
                         static_cast<signed char>(scratch[off]);
                    v2 = (static_cast<signed char>(scratch[off + 3]) << 8) +
                         static_cast<signed char>(scratch[off + 2]);
                } else if (k == 2) {   // 8-bit stereo 0x4C334D
                    v1 = static_cast<signed char>(scratch[off]);
                    v2 = static_cast<signed char>(scratch[off + 1]);
                } else if (k == 1) {   // 16-bit mono 0x4C3379
                    v1 = (static_cast<signed char>(scratch[off + 1]) << 8) +
                         static_cast<signed char>(scratch[off]);
                    v2 = v1;
                } else {               // 8-bit mono 0x4C3389
                    v1 = static_cast<signed char>(scratch[off]);
                    v2 = v1;
                }
                if (maxV < v1) maxV = v1;                          // 0x4C3326
                if (minV > v1) minV = v1;
                if (maxV < v2) maxV = v2;
                if (minV > v2) minV = v2;
            }
            // 0x4C33AF..0x4C33F6: value*25, >>7 (8-bit modes k==0/k==2) or
            // >>15 (16-bit k==1/k==3), + 25; max -> ctx+0x00 array, min ->
            // ctx+0x04 array.  The 0x4C3242 flag is (k==2)|(k==0), NOT a
            // blockAlign test.
            const int shift = (k == 0 || k == 2) ? 7 : 15;
            arrMax[i] = static_cast<unsigned char>(
                (maxV * 0x19 >> shift) + 0x19);
            arrMin[i] = static_cast<unsigned char>(
                (minV * 0x19 >> shift) + 0x19);
            if (!ok)                                                // 0x4C33F9
                break;
            ++i;                                                    // 0x4C3400
        } while (i < audio->waveformColumns);
    }
    free(scratch);                                                  // 0x4C3413
    return true;                                                    // 0x4C341F
}

// ---------------------------------------------------------------------------
// VA 0x004C2760 - WaveStartPlayback(this): re-parse the header from the
// path at ctx+0x30 and (re)create the 2-second streaming DirectSound
// buffer.  The feed thread itself is spawned by Sub4C34A0 (real body in
// src/app/subsystem_init.cpp); neither the x86 original (ret at 0x4C2955)
// nor the x64 twin 0x7FF7CB4FA930 spawns it from here.
// ---------------------------------------------------------------------------
bool WaveStartPlayback(void* obj) {
    auto* audio = static_cast<WaveAudioContext*>(obj);
    FILE*& stream = audio->parseStream;
    if (stream != nullptr)                                          // 0x4C2779
        CloseDataFile(audio);

    std::memset(&audio->format, 0, sizeof audio->format);            // 0x4C2782
    _wfopen_s(&stream, audio->path, L"rb");                          // 0x4C27DC
    if (stream == nullptr)                                          // 0x4C27E6
        return false;
    {
        char riff[8] = {};                                          // 0x4C27F8
        std::fread(riff, 4, 1, stream);                             // result unchecked
        std::fseek(stream, 0x10, SEEK_CUR);                         // 0x4C2804
        std::fread(riff, 2, 1, stream);   // dummy: consume wFormatTag
        std::memset(&audio->format, 0, sizeof audio->format);        // 0x4C281A
        audio->format.wFormatTag = WAVE_FORMAT_PCM;                 // 0x4C2836
        audio->format.cbSize = 0x12;                                // 0x4C283B
        std::fread(&audio->format.nChannels, 2, 1, stream);
        std::fread(&audio->format.nSamplesPerSec, 4, 1, stream);
        std::fread(&audio->format.nAvgBytesPerSec, 4, 1, stream);
        std::fread(&audio->format.nBlockAlign, 2, 1, stream);
        std::fread(&audio->format.wBitsPerSample, 2, 1, stream);
    }
    WaveFindDataChunk(audio, stream);                               // 0x4C2895

    const std::int32_t bufBytes = static_cast<std::int32_t>(
        audio->format.nAvgBytesPerSec * 2);                         // 0x4C28A8
    audio->bufferBytes = bufBytes;                                  // 0x4C28B0
    HANDLE file = CreateFileW(audio->path,
                              GENERIC_READ, 1, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);                             // 0x4C28BF
    audio->fileHandle = file;                                      // 0x4C28D1
    SetFilePointer(file, audio->dataOffset, nullptr,
                   FILE_BEGIN);                                     // 0x4C28D7
    std::fclose(stream);                                            // 0x4C28E0
    stream = nullptr;                                               // 0x4C28EF
    if (file == nullptr)                                            // 0x4C28E8
        return false;

    DSBUFFERDESC desc = {};                                         // 0x4C27C4..
    desc.dwSize = sizeof(desc);                                     // 0x24
    desc.dwFlags = 0x81E0;
    desc.dwBufferBytes = static_cast<DWORD>(bufBytes);
    desc.lpwfxFormat = &audio->format;                              // ctx+0x18
    IDirectSound*& dsound = audio->directSound;
    IDirectSoundBuffer*& buffer = audio->streamingBuffer;
    const HRESULT csbHr = dsound->CreateSoundBuffer(&desc, &buffer,
                                                     nullptr);      // 0x4C290B
    if (MIKUDANCESTUDIO_WAVE_TRACE())
        WaveTrace("CSB hr=%08lx ds=%p tag=%02x%02x ch=%02x%02x "
                  "rate=%08lx avg=%08lx align=%04x bits=%04x cb=%04x "
                  "bytes=%lu\n",
                  (unsigned long)csbHr, (void*)dsound,
                  reinterpret_cast<unsigned char*>(&audio->format)[0],
                  reinterpret_cast<unsigned char*>(&audio->format)[1],
                  reinterpret_cast<unsigned char*>(&audio->format)[2],
                  reinterpret_cast<unsigned char*>(&audio->format)[3],
                  (unsigned long)audio->format.nSamplesPerSec,
                  (unsigned long)audio->format.nAvgBytesPerSec,
                  (unsigned)audio->format.nBlockAlign,
                  (unsigned)audio->format.wBitsPerSample,
                  (unsigned)audio->format.cbSize,
                  (unsigned long)desc.dwBufferBytes);
    if (FAILED(csbHr)) {  // 0x4C290B
        buffer = nullptr;                                           // 0x4C2915
        if (stream != nullptr) {                                    // 0x4C2911
            std::fclose(stream);
            stream = nullptr;
        }
        return false;                                               // 0x4C292F
    }
    return true;                                                    // 0x4C294B
}

// ---------------------------------------------------------------------------
// VA 0x00418500 - LoadWaveFile(this=app).  Menu 0xCE tail (0x487890) and
// the PMM v2 loader (0x456C5F) / drop-file flow reach it after storing the
// chosen path at app+0xD0.
// ---------------------------------------------------------------------------
void LoadWaveFile(MMDApp* app) {
    auto& s = *app;
    HWND hwnd = static_cast<HWND>(s.Hwnd());

    if (s.DirectSoundAvailable() == 0) {                            // 0x418506
        if (s.EnglishUI() != 0)                                     // 0x41850F
            MessageBoxA(hwnd,
                        "You cannot play WAVE because failed "
                        "initialization of DirectSound!",            // 0x52BB80
                        "DirectSound error", 0);                    // 0x52BBC4
        else
            MessageBoxA(hwnd, kMsgWaveNoSoundJp, kMsgWaveErrCaptionJp,
                        0);                                         // 0x52BB2C/68
        return;
    }

    auto* audio = s.Audio();                                      // app+0xCC
    unsigned char* ctx = reinterpret_cast<unsigned char*>(audio);
    WaveCtxReset(ctx);                                              // 0x4C2660
    const bool ok = WaveLoadFile(ctx, s.WavePath(),
                                 app->PathWorkspace());             // 0x4C2F70

    if (!ok) {                                                      // 0x41857F
        s.WaveEnabled() = 0;                                        // 0x41858C
        swprintf_s(s.WavePath(), 0x100, L"");                       // 0x507499
        // blank the timeline strip (hdc = app+0x2E0) 0x41859A..0x418667
        HDC hdc = s.TimelineDC();
        HPEN white = CreatePen(PS_SOLID, 1, 0xFFFFFF);
        HBRUSH brush = CreateSolidBrush(0xFFFFFF);
        HGDIOBJ oldPen = SelectObject(hdc, white);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        Rectangle(hdc, 0, 0, 0xEF, 0x31);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(white);
        DeleteObject(brush);
        HPEN black = CreatePen(PS_SOLID, 1, 0);
        HGDIOBJ oldPen2 = SelectObject(hdc, black);
        MoveToEx(hdc, 0, 0x19, nullptr);
        LineTo(hdc, 0xEF, 0x19);
        SelectObject(hdc, oldPen2);
        DeleteObject(black);
        RECT rect{6, 0x64, 0xF7, 0x97};                             // 0x41866A..
        InvalidateRect(hwnd, &rect, FALSE);
        return;
    }

    wcscpy_s(s.WavePath(), 0x100, audio->path);                    // 0x4186B6
    WaveStartPlayback(ctx);                                         // 0x4C2760
    s.WaveEnabled() = 1;                                            // 0x4186DD
    TimelineDrawTicks(s.TimelineStartFrame(), s.SidebarWidth());
    RECT rect{6, 0x5F,
              s.SidebarWidth() - 3,
              0x92};                                                // 0x4186F5..
    InvalidateRect(hwnd, &rect, FALSE);
    PanelPaint(app);                                                // 0x414610
    SetFrameNormalized(s.FrameNormalization());                     // 0x4C2B80
}

}  // namespace mikudancestudio
