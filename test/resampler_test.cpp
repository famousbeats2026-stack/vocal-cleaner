// Standalone check of the LinearResampler algorithm used in PluginProcessor.
// (Same code text as the nested struct; JUCE-free so it can run here.)
// Round-trips a sine from 44.1k -> 48k -> 44.1k and reports reconstruction error.
#include <cstdio>
#include <cmath>
#include <deque>
#include <vector>

struct LinearResampler
{
    void reset(double r) { ratio = r; readPos = 0.0; history.clear(); }
    void process(const float* input, int numSamples, std::vector<float>& out)
    {
        for (int i = 0; i < numSamples; ++i) history.push_back(input[i]);
        out.clear();
        const double step = 1.0 / ratio;
        while (true)
        {
            const int idx = (int)std::floor(readPos);
            const double frac = readPos - idx;
            if (idx + 1 >= (int)history.size()) break;
            const float s0 = history[(size_t)idx];
            const float s1 = history[(size_t)idx + 1];
            out.push_back(s0 + (float)((s1 - s0) * frac));
            readPos += step;
        }
        const int drop = (int)std::floor(readPos);
        for (int i = 0; i < drop; ++i) history.pop_front();
        readPos -= drop;
    }
    double ratio = 1.0, readPos = 0.0;
    std::deque<float> history;
};

int main()
{
    const double srIn = 44100.0, srMid = 48000.0;
    const int N = 44100; // 1 second of 440 Hz sine at -6 dB
    std::vector<float> in(N);
    for (int i = 0; i < N; ++i) in[i] = 0.5f * (float)std::sin(2.0 * M_PI * 440.0 * i / srIn);

    LinearResampler up, down;
    up.reset(srMid / srIn);
    down.reset(srIn / srMid);

    // Stream it in 512-sample blocks, like a DAW would.
    std::vector<float> mid, back, blockOut;
    std::vector<float> allBack;
    for (int pos = 0; pos < N; pos += 512)
    {
        int n = (pos + 512 <= N) ? 512 : (N - pos);
        up.process(in.data() + pos, n, blockOut);
        mid.insert(mid.end(), blockOut.begin(), blockOut.end());
        down.process(mid.data(), (int)mid.size(), blockOut);
        mid.clear();
        allBack.insert(allBack.end(), blockOut.begin(), blockOut.end());
    }
    // Flush the downsampler with a few zero samples.
    std::vector<float> zeros(64, 0.0f);
    down.process(zeros.data(), 64, blockOut);
    allBack.insert(allBack.end(), blockOut.begin(), blockOut.end());

    // Compare, allowing a small constant delay (resamplers delay <= ~2 samples).
    double bestErr = 1e9;
    int bestLag = 0;
    for (int lag = 0; lag <= 4; ++lag)
    {
        double err = 0; int cnt = 0;
        for (int i = 100; i + lag < N && i + lag < (int)allBack.size(); ++i)
        {
            double d = allBack[i + lag] - in[i];
            err += d * d; ++cnt;
        }
        err = std::sqrt(err / cnt);
        if (err < bestErr) { bestErr = err; bestLag = lag; }
    }
    printf("44.1k -> 48k -> 44.1k round trip:\n");
    printf("  input samples: %d, output samples: %zu\n", N, allBack.size());
    printf("  best lag: %d samples, RMS reconstruction error: %.5f (signal RMS ~0.354)\n",
           bestLag, bestErr);
    printf("  %s\n", bestErr < 0.01 ? "PASS: resampler reconstructs cleanly" : "FAIL");
    return bestErr < 0.01 ? 0 : 1;
}
