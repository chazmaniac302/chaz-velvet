#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine/VelvetEngine.h"

namespace chaz
{

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using AP = juce::AudioParameterFloat;
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto addF = [&] (const char* id, const char* name, Range r, float def)
    {
        p.push_back (std::make_unique<AP> (juce::ParameterID { id, 1 }, name, r, def));
    };

    // Discrete 0/1 floats (not AudioParameterBool): pluginval state restore is reliable
    addF ("epOn",      "EP On", Range (0.0f, 1.0f, 1.0f), 1.0f);
    addF ("padOn",     "Pad On", Range (0.0f, 1.0f, 1.0f), 1.0f);
    addF ("sparkleOn", "Sparkle On", Range (0.0f, 1.0f, 1.0f), 1.0f);
    addF ("subOn",     "Sub On", Range (0.0f, 1.0f, 1.0f), 0.0f);

    addF ("epGainDb",      "EP Gain", Range (-24.0f, 12.0f, 0.1f), 0.0f);
    addF ("padGainDb",     "Pad Gain", Range (-24.0f, 12.0f, 0.1f), -6.0f);
    addF ("sparkleGainDb", "Sparkle Gain", Range (-24.0f, 12.0f, 0.1f), -12.0f);
    addF ("subGainDb",     "Sub Gain", Range (-24.0f, 12.0f, 0.1f), -3.0f);

    addF ("epTone",     "EP Tone", Range (0.0f, 1.0f, 0.001f), 0.55f);
    addF ("epDecay",    "EP Decay", Range (0.2f, 5.0f, 0.001f), 1.8f);
    addF ("epDrive",    "EP Drive", Range (0.0f, 1.0f, 0.001f), 0.25f);
    addF ("epTrem",     "EP Trem", Range (0.0f, 1.0f, 0.001f), 0.35f);
    addF ("epTremRate", "Trem Rate", Range (0.5f, 10.0f, 0.01f), 4.5f);
    addF ("epFm",       "EP FM", Range (0.0f, 1.0f, 0.001f), 0.45f);
    addF ("epBell",     "EP Bell", Range (0.0f, 1.0f, 0.001f), 0.30f);

    addF ("padAttack",  "Pad Atk", Range (0.01f, 3.0f, 0.001f), 0.35f);
    addF ("padDecay",   "Pad Dec", Range (0.05f, 3.0f, 0.001f), 0.80f);
    addF ("padSustain", "Pad Sus", Range (0.0f, 1.0f, 0.001f), 0.70f);
    addF ("padRelease", "Pad Rel", Range (0.05f, 5.0f, 0.001f), 1.20f);
    addF ("padDetune",  "Pad Detune", Range (0.0f, 1.0f, 0.001f), 0.12f);
    addF ("padCutoff",  "Pad Cutoff", Range (0.0f, 1.0f, 0.001f), 0.45f);
    addF ("padShape",   "Pad Shape", Range (0.0f, 1.0f, 0.001f), 0.35f);

    addF ("sparkleMix",   "Sparkle Mix", Range (0.0f, 1.0f, 0.001f), 0.45f);
    addF ("sparkleDecay", "Sparkle Dec", Range (0.05f, 2.0f, 0.001f), 0.55f);
    addF ("sparkleTone",  "Sparkle Tone", Range (0.0f, 1.0f, 0.001f), 0.65f);

    addF ("subMix",   "Sub Mix", Range (0.0f, 1.0f, 0.001f), 0.55f);
    addF ("subSplit", "Sub Split", Range (36.0f, 60.0f, 1.0f), 48.0f);
    addF ("subTone",  "Sub Tone", Range (0.0f, 1.0f, 0.001f), 0.35f);

    addF ("ampAttack",  "Amp Atk", Range (0.001f, 1.0f, 0.001f), 0.008f);
    addF ("ampDecay",   "Amp Dec", Range (0.01f, 3.0f, 0.001f), 0.45f);
    addF ("ampSustain", "Amp Sus", Range (0.0f, 1.0f, 0.001f), 0.55f);
    addF ("ampRelease", "Amp Rel", Range (0.01f, 4.0f, 0.001f), 0.55f);

    addF ("cutoff",    "Cutoff", Range (0.0f, 1.0f, 0.001f), 0.72f);
    addF ("resonance", "Resonance", Range (0.0f, 1.0f, 0.001f), 0.18f);

    addF ("velvet", "Velvet", Range (0.0f, 1.0f, 0.001f), 0.45f);
    addF ("glow",   "Glow", Range (0.0f, 1.0f, 0.001f), 0.40f);
    addF ("space",  "Space", Range (0.0f, 1.0f, 0.001f), 0.35f);
    addF ("dust",   "Dust", Range (0.0f, 1.0f, 0.001f), 0.18f);
    addF ("width",  "Width", Range (0.0f, 1.0f, 0.001f), 0.45f);
    addF ("masterGainDb", "Master", Range (-24.0f, 12.0f, 0.1f), -3.0f);

    return { p.begin(), p.end() };
}

inline VelvetParams readVelvetParams (juce::AudioProcessorValueTreeState& apvts)
{
    auto f = [&] (const char* id) -> float
    {
        if (auto* p = apvts.getRawParameterValue (id)) return p->load();
        return 0.0f;
    };
    auto b = [&] (const char* id) -> bool { return f (id) >= 0.5f; };

    VelvetParams vp;
    vp.epOn = b ("epOn"); vp.padOn = b ("padOn");
    vp.sparkleOn = b ("sparkleOn"); vp.subOn = b ("subOn");
    vp.epGainDb = f ("epGainDb"); vp.padGainDb = f ("padGainDb");
    vp.sparkleGainDb = f ("sparkleGainDb"); vp.subGainDb = f ("subGainDb");
    vp.epTone = f ("epTone"); vp.epDecay = f ("epDecay"); vp.epDrive = f ("epDrive");
    vp.epTrem = f ("epTrem"); vp.epTremRate = f ("epTremRate");
    vp.epFm = f ("epFm"); vp.epBell = f ("epBell");
    vp.padAttack = f ("padAttack"); vp.padDecay = f ("padDecay");
    vp.padSustain = f ("padSustain"); vp.padRelease = f ("padRelease");
    vp.padDetune = f ("padDetune"); vp.padCutoff = f ("padCutoff"); vp.padShape = f ("padShape");
    vp.sparkleMix = f ("sparkleMix"); vp.sparkleDecay = f ("sparkleDecay"); vp.sparkleTone = f ("sparkleTone");
    vp.subMix = f ("subMix"); vp.subSplit = f ("subSplit"); vp.subTone = f ("subTone");
    vp.ampAttack = f ("ampAttack"); vp.ampDecay = f ("ampDecay");
    vp.ampSustain = f ("ampSustain"); vp.ampRelease = f ("ampRelease");
    vp.cutoff = f ("cutoff"); vp.resonance = f ("resonance");
    vp.velvet = f ("velvet"); vp.glow = f ("glow"); vp.space = f ("space");
    vp.dust = f ("dust"); vp.width = f ("width"); vp.masterGainDb = f ("masterGainDb");
    return vp;
}

} // namespace chaz
