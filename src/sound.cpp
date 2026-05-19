#include "sound.h"

#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#include <dispatch/dispatch.h>
#include <cmath>

// Called by the audio thread when a queued buffer has been consumed.
// Dispatches cleanup to a background thread (AudioQueueDispose must not be
// called from within its own callback).
static void audioCallback(void* /*unused*/, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
    AudioQueueFreeBuffer(queue, buffer);
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0), ^{
        AudioQueueStop(queue, true);
        AudioQueueDispose(queue, true);
    });
}

// Synthesise and fire-and-forget a sine-wave tone.
//   freqStart / freqEnd  — start/end frequency in Hz (set freqEnd=0 for no sweep)
//   duration             — seconds
//   volume               — 0..1
//   decay                — exponential decay rate (higher = faster fade)
static void playTone(float freqStart, float freqEnd,
                     float duration,  float volume, float decay)
{
    const int kRate = 22050;
    int n = (int)(kRate * duration);
    if (n <= 0) return;

    AudioStreamBasicDescription fmt{};
    fmt.mSampleRate       = kRate;
    fmt.mFormatID         = kAudioFormatLinearPCM;
    fmt.mFormatFlags      = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    fmt.mBitsPerChannel   = 16;
    fmt.mChannelsPerFrame = 1;
    fmt.mFramesPerPacket  = 1;
    fmt.mBytesPerFrame    = 2;
    fmt.mBytesPerPacket   = 2;

    AudioQueueRef q;
    if (AudioQueueNewOutput(&fmt, audioCallback, nullptr,
                            nullptr, nullptr, 0, &q) != noErr)
        return;

    AudioQueueBufferRef buf;
    if (AudioQueueAllocateBuffer(q, (UInt32)(n * 2), &buf) != noErr) {
        AudioQueueDispose(q, true);
        return;
    }

    auto* pcm = (int16_t*)buf->mAudioData;
    const float twoPi = 6.28318530f;
    float phase = 0.0f;
    for (int i = 0; i < n; i++) {
        float t    = (float)i / kRate;
        float prog = (float)i / n;
        float freq = (freqEnd > 0.0f) ? freqStart + (freqEnd - freqStart) * prog : freqStart;
        float env  = expf(-t * decay);
        phase += twoPi * freq / kRate;
        pcm[i] = (int16_t)(sinf(phase) * 32767.0f * volume * env);
    }
    buf->mAudioDataByteSize = (UInt32)(n * 2);

    AudioQueueEnqueueBuffer(q, buf, 0, nullptr);
    AudioQueueStart(q, nullptr);
}

void soundInit() {}

// Sharp descending chirp — brick shattered
void soundPlayBrickBreak()  { playTone(800.0f, 200.0f, 0.12f, 0.35f, 18.0f); }

// Mid-range blip — ball kissed the paddle
void soundPlayPaddleHit()   { playTone(440.0f, 0.0f,   0.06f, 0.22f, 28.0f); }

// Quick high blip — wall deflection
void soundPlayWallBounce()  { playTone(700.0f, 0.0f,   0.04f, 0.14f, 35.0f); }

// Sad descending tone — life lost
void soundPlayLifeLost()    { playTone(320.0f, 80.0f,  0.55f, 0.35f,  4.0f); }

#else
// ---- stub implementations for non-Apple platforms ----
void soundInit()            {}
void soundPlayBrickBreak()  {}
void soundPlayPaddleHit()   {}
void soundPlayWallBounce()  {}
void soundPlayLifeLost()    {}
#endif
