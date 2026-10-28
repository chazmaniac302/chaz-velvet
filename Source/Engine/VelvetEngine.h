// Chaz Velvet — neo-soul / quiet-storm keys DSP (pure C++17, no JUCE).
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace chaz
{

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float clamp01 (float v) noexcept { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }
inline float softClip (float x) noexcept { return std::tanh (x); }
inline float midiToHz (float note) noexcept
{
    return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f);
}
inline float fastSin (double x) noexcept
{
    // wrap to -pi..pi then cheap poly approx is overkill — std::sin is fine for keys
    return (float) std::sin (x);
}

//==============================================================================
struct VelvetParams
{
    // Layer enables / levels
    bool  epOn      = true;
    bool  padOn     = true;
    bool  sparkleOn = true;
    bool  subOn     = false;
    float epGainDb      = 0.0f;
    float padGainDb     = -6.0f;
    float sparkleGainDb = -12.0f;
    float subGainDb     = -3.0f;

    // EP / Rhodes-ish
    float epTone   = 0.55f;  // brightness
    float epDecay  = 1.8f;   // seconds (body)
    float epDrive  = 0.25f;  // soft saturation
    float epTrem   = 0.35f;  // tremolo depth 0..1
    float epTremRate = 4.5f; // Hz
    float epFm     = 0.45f;  // tine FM amount
    float epBell   = 0.30f;  // metallic tine mix

    // Warm pad
    float padAttack  = 0.35f;
    float padDecay   = 0.80f;
    float padSustain = 0.70f;
    float padRelease = 1.20f;
    float padDetune  = 0.12f; // semitones-ish spread
    float padCutoff  = 0.45f; // soft LPF 0..1
    float padShape   = 0.35f; // 0 triangle-ish, 1 saw

    // Sparkle / bell oneshot character
    float sparkleMix  = 0.45f; // how much sparkle on note-on
    float sparkleDecay = 0.55f;
    float sparkleTone = 0.65f;

    // Soft sub (left-hand / low keys)
    float subMix      = 0.55f;
    float subSplit    = 48.0f; // MIDI note: below this gets more sub
    float subTone     = 0.35f;

    // Amp ADSR (global amp for EP + sparkle primarily; pad has own)
    float ampAttack  = 0.008f;
    float ampDecay   = 0.45f;
    float ampSustain = 0.55f;
    float ampRelease = 0.55f;

    // Filter (voice + master)
    float cutoff    = 0.72f; // 0..1 mapped to Hz
    float resonance = 0.18f;

    // Macros
    float velvet = 0.45f; // LPF warmth
    float glow   = 0.40f; // presence / air
    float space  = 0.35f; // plate/room
    float dust   = 0.18f; // vinyl / noise character
    float width  = 0.45f; // stereo

    float masterGainDb = -3.0f;
};

//==============================================================================
class Smoother
{
public:
    void prepare (double sr, double rampSec) noexcept
    {
        rampLength = std::max (1, (int) std::lround (sr * rampSec));
    }
    void snap (float v) noexcept { current = target = v; remaining = 0; }
    void setTarget (float v) noexcept
    {
        if (v == target) return;
        target = v;
        remaining = rampLength;
        step = (target - current) / (float) rampLength;
    }
    float next() noexcept
    {
        if (remaining <= 0) return target;
        if (--remaining == 0) current = target;
        else current += step;
        return current;
    }
    float get() const noexcept { return current; }
private:
    float current = 0, target = 0, step = 0;
    int rampLength = 1, remaining = 0;
};

//==============================================================================
class OnePoleLP
{
public:
    void reset() noexcept { z = 0; }
    void setCutoff (float hz, double sr) noexcept
    {
        hz = std::max (20.0f, std::min (hz, (float) (sr * 0.45)));
        const float x = (float) std::exp (-2.0 * M_PI * (double) hz / sr);
        a = 1.0f - x;
        b = x;
    }
    float process (float x) noexcept { z = a * x + b * z; return z; }
private:
    float a = 1, b = 0, z = 0;
};

//==============================================================================
class OnePoleHP
{
public:
    void reset() noexcept { z = 0; x1 = 0; }
    void setCutoff (float hz, double sr) noexcept
    {
        hz = std::max (20.0f, std::min (hz, (float) (sr * 0.45)));
        const float x = (float) std::exp (-2.0 * M_PI * (double) hz / sr);
        a = x;
    }
    float process (float in) noexcept
    {
        z = a * (z + in - x1);
        x1 = in;
        return z;
    }
private:
    float a = 0, z = 0, x1 = 0;
};

//==============================================================================
class VelvetEngine
{
public:
    static constexpr int kMaxVoices = 16;

    void prepare (double sampleRate, int /*block*/) noexcept;
    void reset() noexcept;
    void setParams (const VelvetParams& p) noexcept { params = p; applyMacroTargets(); }
    const VelvetParams& getParams() const noexcept { return params; }

    void noteOn (int midiNote, float velocity /*0..1*/) noexcept;
    void noteOff (int midiNote) noexcept;
    void allNotesOff() noexcept;

    void process (float* left, float* right, int numSamples) noexcept;

    float getMeter() const noexcept { return meter; }
    int   getActiveVoices() const noexcept;

private:
    struct Voice
    {
        bool active = false;
        int note = 60;
        float vel = 1;
        int age = 0;
        bool releasing = false;

        // EP osc
        double epPhase = 0, epMod = 0, epBellPhase = 0;
        float epEnv = 0;
        int epStage = 0; // 0A 1D 2S 3R
        OnePoleLP epLp;

        // Pad osc (detuned pair)
        double padPhaseA = 0, padPhaseB = 0;
        float padEnv = 0;
        int padStage = 0;
        OnePoleLP padLp;

        // Sparkle oneshot envelope
        float sparkEnv = 0;
        double sparkPhase = 0, sparkMod = 0;

        // Sub
        double subPhase = 0;
        float subEnv = 0;

        // Per-voice filter
        OnePoleLP voiceLp;
        float tremPhase = 0;
    };

    void renderVoice (Voice& v, float& L, float& R) noexcept;
    void applyMacroTargets() noexcept;
    float dustNoise() noexcept;
    Voice* allocVoice (int note) noexcept;
    Voice* findVoice (int note) noexcept;

    VelvetParams params;
    double sr = 44100.0;
    float invSr = 1.0f / 44100.0f;

    std::array<Voice, kMaxVoices> voices {};

    OnePoleLP masterLpL, masterLpR;
    OnePoleHP airHpL, airHpR;
    Smoother velvetSm, glowSm, spaceSm, dustSm, widthSm, masterSm, cutSm;

    static constexpr int kDelayLen = 16384;
    std::array<float, kDelayLen> delayL {}, delayR {};
    int delayPos = 0;

    uint32_t rng = 0xC4A71ACEu;
    float meter = 0;
    float dustLp = 0;
};

} // namespace chaz
