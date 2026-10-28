#include "VelvetEngine.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#endif

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++fails; } } while (0)

int main (int argc, char** argv)
{
    const char* outDir = argc > 1 ? argv[1] : ".";
    chaz::VelvetEngine eng;
    eng.prepare (48000.0, 512);

    chaz::VelvetParams p;
    p.epOn = true;
    p.padOn = true;
    p.sparkleOn = true;
    p.subOn = true;
    p.velvet = 0.5f;
    p.glow = 0.4f;
    p.space = 0.35f;
    p.dust = 0.2f;
    p.width = 0.5f;
    eng.setParams (p);

    // Cmin7-ish: C3 Eb3 G3 Bb3 then F and Bb
    eng.noteOn (48, 0.85f);
    eng.noteOn (51, 0.80f);
    eng.noteOn (55, 0.78f);
    eng.noteOn (58, 0.75f);

    const int N = 48000;
    std::vector<float> L ((size_t) N), R ((size_t) N);
    for (int off = 0; off < N; off += 256)
    {
        int n = std::min (256, N - off);
        eng.process (L.data() + off, R.data() + off, n);
        if (off == 8000)
        {
            eng.noteOff (48); eng.noteOff (51); eng.noteOff (55); eng.noteOff (58);
            eng.noteOn (53, 0.8f); // F
            eng.noteOn (57, 0.75f);
            eng.noteOn (60, 0.72f);
        }
        if (off == 16000)
        {
            eng.noteOff (53); eng.noteOff (57); eng.noteOff (60);
            eng.noteOn (46, 0.85f); // Bb2
            eng.noteOn (50, 0.78f);
            eng.noteOn (53, 0.75f);
            eng.noteOn (58, 0.70f);
        }
        if (off == 32000)
        {
            eng.allNotesOff();
        }
    }

    double peak = 0, energy = 0;
    for (int i = 0; i < N; ++i)
    {
        peak = std::max (peak, (double) std::abs (L[(size_t) i]));
        peak = std::max (peak, (double) std::abs (R[(size_t) i]));
        energy += (double) L[(size_t) i] * L[(size_t) i] + (double) R[(size_t) i] * R[(size_t) i];
    }
    std::printf ("peak=%.4f energy=%.2f meter=%.4f voices_end=%d\n",
                 peak, energy, eng.getMeter(), eng.getActiveVoices());
    CHECK (peak > 0.01);
    CHECK (peak < 8.0);
    CHECK (energy > 1.0);

    // Polyphony stress
    eng.reset();
    eng.setParams (p);
    for (int n = 36; n < 36 + 20; ++n)
        eng.noteOn (n, 0.7f);
    CHECK (eng.getActiveVoices() <= chaz::VelvetEngine::kMaxVoices);
    CHECK (eng.getActiveVoices() >= 8);

    std::string path = std::string (outDir) + "/chaz-velvet-engine.raw";
#ifdef _WIN32
    _mkdir (outDir);
#else
    std::string mk = std::string ("mkdir -p \"") + outDir + "\"";
    std::system (mk.c_str());
#endif
    if (FILE* f = std::fopen (path.c_str(), "wb"))
    {
        std::fwrite (L.data(), sizeof (float), (size_t) N, f);
        std::fclose (f);
        std::printf ("wrote %s\n", path.c_str());
    }

    if (fails) { std::printf ("%d FAIL(s)\n", fails); return 1; }
    std::printf ("EngineTests OK\n");
    return 0;
}
