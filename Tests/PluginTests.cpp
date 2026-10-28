#include "../Source/PluginProcessor.h"
#include <cstdio>

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++fails; } } while (0)

int main (int, char**)
{
    ChazVelvetProcessor proc;
    CHECK (proc.getName().isNotEmpty());
    CHECK (proc.acceptsMidi());
    CHECK (proc.hasEditor());
    CHECK (proc.getNumPrograms() >= 20);

    proc.prepareToPlay (44100.0, 512);
    proc.applyPreset (0); // VELVET
    CHECK (proc.getProgramName (0).containsIgnoreCase ("VELVET"));

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0);
    midi.addEvent (juce::MidiMessage::noteOn (1, 51, (juce::uint8) 90), 5);
    midi.addEvent (juce::MidiMessage::noteOn (1, 55, (juce::uint8) 88), 10);
    midi.addEvent (juce::MidiMessage::noteOn (1, 58, (juce::uint8) 85), 15);
    proc.processBlock (buf, midi);

    float peak = 0;
    for (int c = 0; c < buf.getNumChannels(); ++c)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            peak = std::max (peak, std::abs (buf.getSample (c, i)));
    std::printf ("plugin peak=%.4f programs=%d\n", peak, proc.getNumPrograms());
    CHECK (peak > 0.0001f);

    juce::MemoryBlock state;
    proc.getStateInformation (state);
    CHECK (state.getSize() > 10);
    proc.setStateInformation (state.getData(), (int) state.getSize());

    if (fails) { std::printf ("%d FAIL(s)\n", fails); return 1; }
    std::printf ("PluginTests OK\n");
    return 0;
}
