#include "PluginEditor.h"

//==============================================================================
MacroStrip::MacroStrip (juce::AudioProcessorValueTreeState& s)
{
    const char* ids[] = { "velvet", "glow", "space", "dust", "width" };
    const char* names[] = { "VELVET", "GLOW", "SPACE", "DUST", "WIDTH" };
    const juce::Colour fills[] = {
        chazui::col::accent2, chazui::col::accent3, chazui::col::accent,
        chazui::col::dim, chazui::col::accent
    };
    for (int i = 0; i < 5; ++i)
    {
        auto* sl = knobs.add (new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag,
                                                juce::Slider::TextBoxBelow));
        sl->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 14);
        sl->setColour (juce::Slider::rotarySliderFillColourId, fills[i]);
        addAndMakeVisible (sl);
        auto* lb = labels.add (new juce::Label ({}, names[i]));
        lb->setJustificationType (juce::Justification::centred);
        lb->setColour (juce::Label::textColourId, chazui::col::dim);
        lb->setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        addAndMakeVisible (lb);
        atts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (s, ids[i], *sl));
    }
}

void MacroStrip::resized()
{
    auto r = getLocalBounds();
    const int n = knobs.size();
    if (n == 0) return;
    const int w = r.getWidth() / n;
    for (int i = 0; i < n; ++i)
    {
        auto cell = r.removeFromLeft (w).reduced (4);
        labels[i]->setBounds (cell.removeFromTop (16));
        knobs[i]->setBounds (cell);
    }
}

//==============================================================================
ChazVelvetEditor::ChazVelvetEditor (ChazVelvetProcessor& p)
    : AudioProcessorEditor (p), proc (p), macros (p.apvts)
{
    setLookAndFeel (&lnf);
    setSize (880, 560);
    setResizable (false, false);

    title.setText ("CHAZ VELVET", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, chazui::col::accent);
    addAndMakeVisible (title);

    subtitle.setText ("Chzm  ·  neo-soul / quiet-storm keys  ·  expensive out of the box",
                      juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (13.0f)));
    subtitle.setColour (juce::Label::textColourId, chazui::col::dim);
    addAndMakeVisible (subtitle);

    addAndMakeVisible (macros);

    refreshPresetBox();
    addAndMakeVisible (presetBox);
    presetBox.onChange = [this]
    {
        const int i = presetBox.getSelectedItemIndex();
        if (i >= 0) proc.applyPreset (i);
    };

    prevBtn.onClick = [this]
    {
        int i = juce::jmax (0, proc.getCurrentProgram() - 1);
        proc.applyPreset (i);
        presetBox.setSelectedItemIndex (i, juce::dontSendNotification);
    };
    nextBtn.onClick = [this]
    {
        int i = juce::jmin (proc.getNumPrograms() - 1, proc.getCurrentProgram() + 1);
        proc.applyPreset (i);
        presetBox.setSelectedItemIndex (i, juce::dontSendNotification);
    };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    bindLayerToggle (epBtn, "epOn");
    bindLayerToggle (padBtn, "padOn");
    bindLayerToggle (sparkBtn, "sparkleOn");
    bindLayerToggle (subBtn, "subOn");
    addAndMakeVisible (epBtn);
    addAndMakeVisible (padBtn);
    addAndMakeVisible (sparkBtn);
    addAndMakeVisible (subBtn);

    auto sect = [&] (juce::Label& l, const char* t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        l.setColour (juce::Label::textColourId, chazui::col::accent3);
        addAndMakeVisible (l);
    };
    sect (sectionEp, "EP / RHODES");
    sect (sectionPad, "WARM PAD");
    sect (sectionAmp, "AMP / FILTER");

    buildKnob (epTone, epToneL, "TONE", "epTone");
    buildKnob (epDecay, epDecayL, "DECAY", "epDecay");
    buildKnob (epDrive, epDriveL, "DRIVE", "epDrive", chazui::col::accent2);
    buildKnob (epTrem, epTremL, "TREM", "epTrem");
    buildKnob (epFm, epFmL, "FM", "epFm");
    buildKnob (epBell, epBellL, "BELL", "epBell", chazui::col::accent3);

    buildKnob (padAtk, padAtkL, "ATTACK", "padAttack");
    buildKnob (padCut, padCutL, "CUTOFF", "padCutoff");
    buildKnob (padDet, padDetL, "DETUNE", "padDetune");
    buildKnob (padShape, padShapeL, "SHAPE", "padShape");

    buildKnob (ampA, ampAL, "ATK", "ampAttack");
    buildKnob (ampD, ampDL, "DEC", "ampDecay");
    buildKnob (ampS, ampSL, "SUS", "ampSustain");
    buildKnob (ampR, ampRL, "REL", "ampRelease");
    buildKnob (cutoff, cutoffL, "CUT", "cutoff");
    buildKnob (reso, resoL, "RES", "resonance");
    buildKnob (master, masterL, "OUT", "masterGainDb", chazui::col::accent3);

    status.setColour (juce::Label::textColourId, chazui::col::dim);
    status.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (status);

    syncLayerButtons();
    startTimerHz (24);
}

ChazVelvetEditor::~ChazVelvetEditor()
{
    setLookAndFeel (nullptr);
}

void ChazVelvetEditor::buildKnob (juce::Slider& s, juce::Label& l, const char* name, const char* paramId,
                                  juce::Colour fill)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 14);
    s.setColour (juce::Slider::rotarySliderFillColourId, fill);
    addAndMakeVisible (s);
    l.setText (name, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, chazui::col::dim);
    l.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    addAndMakeVisible (l);
    sliderAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (proc.apvts, paramId, s));
}

void ChazVelvetEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& ps = chaz::getFactoryPresets();
    for (int i = 0; i < (int) ps.size(); ++i)
        presetBox.addItem (ps[(size_t) i].name, i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
}

void ChazVelvetEditor::bindLayerToggle (juce::ToggleButton& btn, const char* paramId)
{
    btn.setClickingTogglesState (true);
    btn.onClick = [this, &btn, paramId]
    {
        if (auto* p = proc.apvts.getParameter (paramId))
            p->setValueNotifyingHost (btn.getToggleState() ? 1.0f : 0.0f);
    };
}

void ChazVelvetEditor::syncLayerButtons()
{
    auto on = [&] (const char* id) -> bool
    {
        if (auto* p = proc.apvts.getRawParameterValue (id))
            return p->load() >= 0.5f;
        return false;
    };
    epBtn.setToggleState (on ("epOn"), juce::dontSendNotification);
    padBtn.setToggleState (on ("padOn"), juce::dontSendNotification);
    sparkBtn.setToggleState (on ("sparkleOn"), juce::dontSendNotification);
    subBtn.setToggleState (on ("subOn"), juce::dontSendNotification);
}

void ChazVelvetEditor::timerCallback()
{
    meterSmooth = meterSmooth * 0.85f + proc.engine.getMeter() * 0.15f;
    syncLayerButtons();
    status.setText ("voices " + juce::String (proc.engine.getActiveVoices())
                    + "  ·  meter " + juce::String (meterSmooth, 3)
                    + "  ·  " + juce::String (proc.getNumPrograms()) + " presets",
                    juce::dontSendNotification);
    repaint (getLocalBounds().removeFromBottom (28));
}

void ChazVelvetEditor::paint (juce::Graphics& g)
{
    g.fillAll (chazui::col::bg);
    auto header = getLocalBounds().removeFromTop (70).toFloat().reduced (8.0f, 4.0f);
    g.setColour (chazui::col::panel);
    g.fillRoundedRectangle (header, 10.0f);
    g.setColour (chazui::col::accent2.withAlpha (0.55f));
    g.drawRoundedRectangle (header, 10.0f, 1.5f);

    // meter bar
    auto meterR = juce::Rectangle<float> ((float) getWidth() - 120.0f, 18.0f, 100.0f, 10.0f);
    g.setColour (chazui::col::panel2);
    g.fillRoundedRectangle (meterR, 3.0f);
    g.setColour (chazui::col::accent);
    g.fillRoundedRectangle (meterR.withWidth (meterR.getWidth() * juce::jlimit (0.0f, 1.0f, meterSmooth * 2.0f)), 3.0f);
}

void ChazVelvetEditor::resized()
{
    auto r = getLocalBounds().reduced (12);
    auto top = r.removeFromTop (56);
    title.setBounds (top.removeFromLeft (220).reduced (4));
    subtitle.setBounds (top.removeFromLeft (360).reduced (4));
    nextBtn.setBounds (top.removeFromRight (36).reduced (2));
    prevBtn.setBounds (top.removeFromRight (36).reduced (2));
    presetBox.setBounds (top.reduced (4));

    auto layers = r.removeFromTop (36);
    epBtn.setBounds (layers.removeFromLeft (70).reduced (2));
    padBtn.setBounds (layers.removeFromLeft (70).reduced (2));
    sparkBtn.setBounds (layers.removeFromLeft (80).reduced (2));
    subBtn.setBounds (layers.removeFromLeft (70).reduced (2));

    macros.setBounds (r.removeFromTop (110));

    auto body = r.removeFromTop (280);
    auto colW = body.getWidth() / 3;

    auto placeRow = [] (juce::Rectangle<int> area, std::initializer_list<std::pair<juce::Slider*, juce::Label*>> items)
    {
        const int n = (int) items.size();
        if (n == 0) return;
        const int w = area.getWidth() / n;
        for (auto& it : items)
        {
            auto cell = area.removeFromLeft (w).reduced (2);
            it.second->setBounds (cell.removeFromTop (14));
            it.first->setBounds (cell);
        }
    };

    {
        auto c = body.removeFromLeft (colW).reduced (4);
        sectionEp.setBounds (c.removeFromTop (18));
        placeRow (c, {
            { &epTone, &epToneL }, { &epDecay, &epDecayL }, { &epDrive, &epDriveL },
            { &epTrem, &epTremL }, { &epFm, &epFmL }, { &epBell, &epBellL }
        });
    }
    {
        auto c = body.removeFromLeft (colW).reduced (4);
        sectionPad.setBounds (c.removeFromTop (18));
        placeRow (c, {
            { &padAtk, &padAtkL }, { &padCut, &padCutL },
            { &padDet, &padDetL }, { &padShape, &padShapeL }
        });
    }
    {
        auto c = body.reduced (4);
        sectionAmp.setBounds (c.removeFromTop (18));
        placeRow (c, {
            { &ampA, &ampAL }, { &ampD, &ampDL }, { &ampS, &ampSL }, { &ampR, &ampRL },
            { &cutoff, &cutoffL }, { &reso, &resoL }, { &master, &masterL }
        });
    }

    status.setBounds (r.removeFromBottom (22));
}
