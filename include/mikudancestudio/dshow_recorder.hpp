#pragma once

#include <cstdint>

struct IBaseFilter;
struct IFileSinkFilter;
struct IGraphBuilder;
struct IAMVfwCompressDialogs;
struct IMediaControl;
struct IMediaEvent;
struct IPin;

namespace mikudancestudio {

// DirectShow recording graph owned by MMDApp.  The original x86 object is
// 27 four-byte slots (0x6C); on x64 the COM members use their native width.
struct DShowRecorder {
    IBaseFilter* compressor;
    IAMVfwCompressDialogs* compressorDialogs;
    void* compressorState;
    IGraphBuilder* graph;
    IBaseFilter* videoSource;
    IBaseFilter* videoGrabber;
    IBaseFilter* aviMux;
    IFileSinkFilter* fileWriter;
    IPin* sourceOutput;
    IPin* grabberInput;
    IPin* grabberOutput;
    IPin* compressorInput;
    IPin* compressorOutput;
    IPin* muxVideoInput;
    IPin* reservedInput0;
    IPin* reservedInput1;
    IPin* muxAudioInput;
    IMediaControl* mediaControl;
    IBaseFilter* waveSource;
    IPin* waveOutput;
    IBaseFilter* audioGrabber;
    IPin* audioGrabberInput;
    IPin* audioGrabberOutput;
    IMediaEvent* mediaEvent;
    std::int32_t compressorStateSize;
    std::int32_t selectedCodec;
    void* framePush;
};

#if INTPTR_MAX == INT32_MAX
static_assert(sizeof(DShowRecorder) == 0x6C);
#endif

}  // namespace mikudancestudio
