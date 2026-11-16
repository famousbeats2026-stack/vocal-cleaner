#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

//==============================================================================
FamousBeatsVocalCleanerProcessor::FamousBeatsVocalCleanerProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters",
                 {
                     std::make_unique<juce::AudioParameterFloat>(
                         kAmountParam, "Amount",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),
                     std::make_unique<juce::AudioParameterBool>(
                         kBypassParam, "Bypass", false),
                     std::make_unique<juce::AudioParameterFloat>(
                         kGainParam, "Output Gain",
                         juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
                         juce::AudioParameterFloatAttributes().withLabel("dB")),
                 })
{
    amountParam = parameters.getRawParameterValue(kAmountParam);
    bypassParam = parameters.getRawParameterValue(kBypassParam);
    gainParam = parameters.getRawParameterValue(kGainParam);
}

FamousBeatsVocalCleanerProcessor::~FamousBeatsVocalCleanerProcessor()
{
    freeDenoisers();
    if (sharedModel != nullptr)
    {
        rnnoise_model_free(sharedModel);
        sharedModel = nullptr;
    }
}

//==============================================================================
// RNNoise model: the embedded "sh" model (sh.rnnn) is stronger on hiss than
// the library's built-in 2017 model (in my tests: 39.6 dB vs 6.8 dB of
// reduction on white noise; both cut steady pink/room noise by 30+ dB).
// rnnoise_model_from_file() needs a FILE*, so the embedded bytes are
// written to a temp file once. Any failure falls back to the built-in model.
RNNModel* FamousBeatsVocalCleanerProcessor::loadEmbeddedModel()
{
    auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("fbs_vocalcleaner_sh.rnnn");

    if (!tempFile.existsAsFile() || tempFile.getSize() != BinaryData::sh_rnnnSize)
    {
        if (!tempFile.replaceWithData(BinaryData::sh_rnnn, (size_t)BinaryData::sh_rnnnSize))
            return nullptr;
    }

    FILE* f = fopen(tempFile.getFullPathName().toRawUTF8(), "r");
    if (f == nullptr)
        return nullptr;

    RNNModel* model = rnnoise_model_from_file(f);
    fclose(f);
    return model; // may be nullptr if the file was unreadable
}

void FamousBeatsVocalCleanerProcessor::initDenoisers()
{
    freeDenoisers();

    if (!modelTried)
    {
        sharedModel = loadEmbeddedModel();
        modelTried = true;
    }

    const int numCh = (int)channels.size();
    for (int ch = 0; ch < numCh; ++ch)
        channels[(size_t)ch].denoiser = rnnoise_create(sharedModel);
}

void FamousBeatsVocalCleanerProcessor::freeDenoisers()
{
    for (auto& ch : channels)
    {
        if (ch.denoiser != nullptr)
        {
            rnnoise_destroy(ch.denoiser);
            ch.denoiser = nullptr;
        }
    }
}

void FamousBeatsVocalCleanerProcessor::resetChannelState()
{
    for (auto& ch : channels)
    {
        ch.to48k.reset((double)kRnnoiseRate / hostRate);
        ch.from48k.reset(hostRate / (double)kRnnoiseRate);
        ch.in48k.clear();
        ch.out48k.clear();
        ch.pendingWet.clear();
        ch.dryDelay.assign((size_t)juce::jmax(1, latencySamples), 0.0f);
        ch.dryDelayPos = 0;
        std::fill(std::begin(ch.frameIn), std::end(ch.frameIn), 0.0f);
        std::fill(std::begin(ch.frameOut), std::end(ch.frameOut), 0.0f);
    }
}

//==============================================================================
void FamousBeatsVocalCleanerProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    hostRate = sampleRate;

    // RNNoise's overlap-add means the cleaned signal comes out one frame
    // (10 ms) late. The resamplers add about a sample each way.
    latencySamples = (int)std::ceil(0.010 * sampleRate) + 2;
    setLatencySamples(latencySamples);

    const int numCh = juce::jmax(1, getTotalNumInputChannels());
    channels.assign((size_t)numCh, ChannelState());
    resetChannelState();
    initDenoisers();
}

void FamousBeatsVocalCleanerProcessor::releaseResources()
{
    freeDenoisers();
    channels.clear();
}

//==============================================================================
void FamousBeatsVocalCleanerProcessor::LinearResampler::reset(double r)
{
    ratio = r;
    readPos = 0.0;
    history.clear();
}

void FamousBeatsVocalCleanerProcessor::LinearResampler::process(const float* input, int numSamples,
                                                                std::vector<float>& out)
{
    for (int i = 0; i < numSamples; ++i)
        history.push_back(input[i]);

    out.clear();
    const double step = 1.0 / ratio;
    while (true)
    {
        const int idx = (int)std::floor(readPos);
        const double frac = readPos - idx;
        if (idx + 1 >= (int)history.size())
            break;
        const float s0 = history[(size_t)idx];
        const float s1 = history[(size_t)idx + 1];
        out.push_back(s0 + (float)((s1 - s0) * frac));
        readPos += step;
    }

    const int drop = (int)std::floor(readPos);
    for (int i = 0; i < drop; ++i)
        history.pop_front();
    readPos -= drop;
}

//==============================================================================
void FamousBeatsVocalCleanerProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if ((int)channels.size() != numChannels)
    {
        freeDenoisers();
        channels.assign((size_t)numChannels, ChannelState());
        resetChannelState();
        initDenoisers();
    }

    const float amount = juce::jlimit(0.0f, 1.0f, amountParam->load() * 0.01f);
    const bool bypassed = bypassParam->load() > 0.5f;
    const float gain = juce::Decibels::decibelsToGain(gainParam->load());

    std::vector<float> resampled48k;
    std::vector<float> resampledHost;
    std::vector<float> frame48k;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto& state = channels[(size_t)ch];
        float* channelData = buffer.getWritePointer(ch);

        // 1. Resample host rate -> 48 kHz and collect complete 480-sample frames.
        state.to48k.process(channelData, numSamples, resampled48k);
        for (float s : resampled48k)
            state.in48k.push_back(s);

        while (state.in48k.size() >= (size_t)kFrameSize)
        {
            for (int i = 0; i < kFrameSize; ++i)
            {
                // RNNoise wants int16-scale floats (+/-32767), not +/-1.0.
                state.frameIn[i] = state.in48k.front() * 32768.0f;
                state.in48k.pop_front();
            }
            if (state.denoiser != nullptr)
                rnnoise_process_frame(state.denoiser, state.frameOut, state.frameIn);
            else
                std::copy(std::begin(state.frameIn), std::end(state.frameIn), std::begin(state.frameOut));

            for (int i = 0; i < kFrameSize; ++i)
                state.out48k.push_back(state.frameOut[i] * (1.0f / 32768.0f));
        }

        // 2. Resample the cleaned 48 kHz audio back to the host rate.
        // Keep any extra samples for the next block so nothing is lost
        // when the resampler makes one sample more or less than we need.
        frame48k.assign(state.out48k.begin(), state.out48k.end());
        state.out48k.clear();
        state.from48k.process(frame48k.data(), (int)frame48k.size(), resampledHost);
        for (float s : resampledHost)
            state.pendingWet.push_back(s);

        // 3. Mix the cleaned signal with the (latency-matched) dry signal.
        const int delayLen = (int)state.dryDelay.size();
        for (int n = 0; n < numSamples; ++n)
        {
            const float dryIn = channelData[n];

            // Delayed dry signal, so the mix stays in time with the
            // denoiser's 10 ms latency.
            float dryDelayed = 0.0f;
            if (delayLen > 0)
            {
                dryDelayed = state.dryDelay[(size_t)state.dryDelayPos];
                state.dryDelay[(size_t)state.dryDelayPos] = dryIn;
                state.dryDelayPos = (state.dryDelayPos + 1) % delayLen;
            }

            float wet = 0.0f;
            if (!state.pendingWet.empty())
            {
                wet = state.pendingWet.front();
                state.pendingWet.pop_front();
            }
            // Any missing wet samples at the very start (pipeline priming)
            // stay silent — that is the reported latency.

            float out = bypassed ? dryDelayed
                                 : (wet * amount + dryDelayed * (1.0f - amount));
            channelData[n] = out * gain;
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* FamousBeatsVocalCleanerProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void FamousBeatsVocalCleanerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FamousBeatsVocalCleanerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FamousBeatsVocalCleanerProcessor();
}
