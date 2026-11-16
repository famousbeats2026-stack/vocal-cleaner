#pragma once

#include <JuceHeader.h>
#include <deque>
#include <vector>

// RNNoise C API (sources in Source/RNNoise)
extern "C" {
#include "rnnoise.h"
}

// Famous Beats Vocal Cleaner — removes background noise from vocals
// with the RNNoise neural denoiser (same engine as Discord/Zoom voice
// cleanup). RNNoise itself is mono, 48 kHz, 10 ms frames, so this
// processor resamples to/from 48 kHz and runs one denoiser per channel.
class FamousBeatsVocalCleanerProcessor : public juce::AudioProcessor
{
public:
    FamousBeatsVocalCleanerProcessor();
    ~FamousBeatsVocalCleanerProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Famous Beats Vocal Cleaner"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParametersState() { return parameters; }

    static constexpr const char* kAmountParam = "amount";   // 0..100 %
    static constexpr const char* kBypassParam = "bypass";   // on/off
    static constexpr const char* kGainParam   = "outputGain"; // dB

private:
    // RNNoise runs on 480-sample frames at 48 kHz, and expects samples
    // at int16 scale (about +/-32767), NOT normalised +/-1.0 floats.
    // (The official rnnoise_demo.c passes raw short values as float.)
    static constexpr int kRnnoiseRate = 48000;
    static constexpr int kFrameSize = 480; // 10 ms at 48 kHz

    // Small streaming linear resampler (any rate <-> 48 kHz).
    struct LinearResampler
    {
        void reset(double ratio); // ratio = outputRate / inputRate
        // Push input samples; returns resampled output appended to `out`.
        void process(const float* input, int numSamples, std::vector<float>& out);

    private:
        double ratio = 1.0;
        double readPos = 0.0; // position (in input samples) of next output
        std::deque<float> history; // input samples not yet consumed
    };

    // Per-channel denoise chain state.
    struct ChannelState
    {
        DenoiseState* denoiser = nullptr;
        LinearResampler to48k;     // host rate -> 48 kHz
        LinearResampler from48k;   // 48 kHz -> host rate
        std::deque<float> in48k;   // resampled input waiting for a frame
        std::deque<float> out48k;  // denoised 48 kHz output waiting to go out
        std::deque<float> pendingWet; // cleaned host-rate samples waiting for a block
        std::vector<float> dryDelay; // delay line so dry/wet mix stays in time
        int dryDelayPos = 0;
        float frameIn[kFrameSize] = {};
        float frameOut[kFrameSize] = {};
    };

    void initDenoisers();
    void freeDenoisers();
    void resetChannelState();

    // Load the embedded "sh" RNNoise model (sh.rnnn) once, via a temp file,
    // because rnnoise_model_from_file() needs a FILE*. Returns nullptr to
    // use the built-in default model if anything goes wrong.
    static RNNModel* loadEmbeddedModel();

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* amountParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* gainParam = nullptr;

    std::vector<ChannelState> channels;
    int latencySamples = 0; // reported to the host (10 ms + resampler slack)
    double hostRate = 48000.0;
    bool modelTried = false;
    RNNModel* sharedModel = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FamousBeatsVocalCleanerProcessor)
};
