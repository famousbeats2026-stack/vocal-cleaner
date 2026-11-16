/* Test harness: feed a noisy 48 kHz mono 16-bit PCM file through the
   embedded RNNoise and report RMS levels of the noise-only gap (1-2 s)
   vs the voice section (2-3 s), before and after. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "rnnoise.h"

static double region_rms(const short *pcm, int total, int sr, double t0, double t1)
{
    int start = (int)(t0 * sr), end = (int)(t1 * sr);
    if (end > total) end = total;
    double sum = 0.0;
    int n = 0;
    for (int i = start; i < end; i++) {
        double v = pcm[i] / 32768.0;
        sum += v * v;
        n++;
    }
    return n ? sqrt(sum / n) : 0.0;
}

static double db(double rms) { return 20.0 * log10(rms + 1e-12); }

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s in.raw out.raw [model.rnnn]   (48kHz mono s16le)\n", argv[0]);
        return 1;
    }
    FILE *fin = fopen(argv[1], "rb");
    if (!fin) { perror("open input"); return 1; }
    fseek(fin, 0, SEEK_END);
    long bytes = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    int total = (int)(bytes / 2);
    short *in = malloc(bytes);
    short *out = malloc(bytes);
    if (fread(in, 1, bytes, fin) != (size_t)bytes) { fprintf(stderr, "short read\n"); return 1; }
    fclose(fin);

    int frame = rnnoise_get_frame_size();
    printf("RNNoise frame size: %d samples\n", frame);
    RNNModel *model = NULL;
    if (argc >= 4) {
        FILE *mf = fopen(argv[3], "r");
        if (!mf) { perror("open model"); return 1; }
        model = rnnoise_model_from_file(mf);
        fclose(mf);
        if (!model) { fprintf(stderr, "failed to load model\n"); return 1; }
        printf("Loaded model: %s\n", argv[3]);
    } else {
        printf("Using built-in default model\n");
    }
    DenoiseState *st = rnnoise_create(model);
    if (!st) { fprintf(stderr, "rnnoise_create failed\n"); return 1; }

    /* NOTE: the old RNNoise library expects raw int16-scale floats
       (range +/-32767), NOT normalized +/-1.0 floats. The official
       rnnoise_demo.c passes short values straight through as float.
       Normalizing to +/-1.0 starves the feature energies and the
       network outputs gains ~1 (no denoising). */
    float *fin_f = malloc(frame * sizeof(float));
    float *fout_f = malloc(frame * sizeof(float));
    int nframes = total / frame;
    for (int f = 0; f < nframes; f++) {
        for (int i = 0; i < frame; i++) fin_f[i] = (float)in[f * frame + i];
        rnnoise_process_frame(st, fout_f, fin_f);
        for (int i = 0; i < frame; i++) {
            float v = fout_f[i];
            if (v > 32767.0f) v = 32767.0f;
            if (v < -32768.0f) v = -32768.0f;
            out[f * frame + i] = (short)v;
        }
    }
    /* copy any tail unprocessed */
    for (int i = nframes * frame; i < total; i++) out[i] = in[i];
    rnnoise_destroy(st);

    FILE *fout = fopen(argv[2], "wb");
    fwrite(out, 1, bytes, fout);
    fclose(fout);

    int sr = 48000;
    double gap_in = region_rms(in, total, sr, 1.0, 2.0);
    double gap_out = region_rms(out, total, sr, 1.0, 2.0);
    double vox_in = region_rms(in, total, sr, 2.0, 3.0);
    double vox_out = region_rms(out, total, sr, 2.0, 3.0);
    printf("Noise-only gap (1-2s):  in %7.1f dB  ->  out %7.1f dB   (reduction %5.1f dB)\n",
           db(gap_in), db(gap_out), db(gap_in) - db(gap_out));
    printf("Voice section  (2-3s):  in %7.1f dB  ->  out %7.1f dB   (voice change %5.1f dB)\n",
           db(vox_in), db(vox_out), db(vox_out) - db(vox_in));
    printf("Frames processed: %d\n", nframes);
    free(in); free(out); free(fin_f); free(fout_f);
    return 0;
}
