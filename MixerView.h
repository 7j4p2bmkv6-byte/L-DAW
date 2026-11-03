#pragma once
// L-DAW 0.4
#include "Effects.h"

//==============================================================================
// Reason-style look: light grey panels, coloured knobs, metal fader caps
//==============================================================================
class ReasonLook : public juce::LookAndFeel_V4
{
public:
    ReasonLook();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& background,
                               bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};

//==============================================================================
// LED-ladder stereo meter with dB scale
//==============================================================================
class LevelMeter : public juce::Component
{
public:
    ~LevelMeter() override { detach(); }

    void attach (te::LevelMeterPlugin*);
    void detach();
    void tick();
    void paint (juce::Graphics&) override;

private:
    te::Plugin::Ptr meterPlugin;
    te::LevelMeasurer* measurer = nullptr;
    te::LevelMeasurer::Client client;
    float levels[2] { -100.0f, -100.0f }, peaks[2] { -100.0f, -100.0f };
    int peakHold[2] { 0, 0 };
};

//==============================================================================
// A knob bound to an automatable parameter (gestures -> automation recording)
//==============================================================================
class ParamKnob : public juce::Component
{
public:
    ParamKnob (juce::Colour, const juce::String& label, double defaultNorm);

    std::function<te::AutomatableParameter* (bool create)> resolve;   // bound parameter, or...
    std::function<double()>             getter;                       // ...plain value access (0..1)
    std::function<void (double)>        setter;
    std::function<juce::String (double)> textFn;
    void tick();

    void paint (juce::Graphics&) override;
    void resized() override;

    juce::Slider slider;

private:
    juce::String label;
    double defaultNorm;
};

//==============================================================================
struct SlotButton : public juce::TextButton
{
    using juce::TextButton::TextButton;
    std::function<void()> onRightClick;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); return; }
        juce::TextButton::mouseDown (e);
    }
};

//==============================================================================
// One channel strip: music track, FX return or master
//==============================================================================
class MixerStrip : public juce::Component
{
public:
    enum class Kind { music, fxReturn, master };
    enum Section { secDyn = 0, secEq, secInserts, secSends, numSections };

    MixerStrip (te::Edit&, te::AudioTrack*, Kind, int index, juce::Colour);

    static int sectionHeight (int section);
    static constexpr int headerHeight = 24;
    static constexpr int bottomMinHeight = 214;

    void setSections (const bool visible[numSections]);
    void tick();

    void paint (juce::Graphics&) override;
    void resized() override;

    te::AudioTrack* track;
    const Kind kind;

private:
    te::Edit& edit;
    const int index;
    const juce::Colour colour;
    bool show[numSections] { true, true, true, true };
    juce::Rectangle<int> sectionArea[numSections], headerArea, bottomArea;

    juce::Label nameLabel;
    juce::TextButton instrumentButton, dynOn { "ON" }, eqOn { "ON" }, addFxButton { "+ FX" },
                     muteButton { "MUTE" }, soloButton { "SOLO" };
    juce::OwnedArray<ParamKnob> dynKnobs, eqKnobs, sendKnobs;
    juce::OwnedArray<juce::Label> sendLabels;
    juce::OwnedArray<SlotButton> insertButtons;
    ParamKnob pan { juce::Colour (0xff4a4d55), "PAN", 0.5 };
    juce::Slider fader;
    LevelMeter meter;
    juce::String insertSignature;

    bool hasSection (int s) const;
    te::PluginList& chain();
    te::VolumeAndPanPlugin* volume();
    void rebuildInserts();
    void showInsertMenu (te::Plugin*, juce::Component*);
    juce::String makeSignature();
    void toggleChannelPlugin (const juce::String& which);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerStrip)
};

//==============================================================================
class MixerView : public juce::Component
{
public:
    MixerView();
    ~MixerView() override;

    void setEdit (te::Edit*);
    void rebuild();
    void tick();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ReasonLook look;
    te::Edit* edit = nullptr;
    juce::Viewport viewport;
    juce::Component content;
    juce::OwnedArray<MixerStrip> strips;
    juce::String trackSignature;

    bool userShow[MixerStrip::numSections] { true, true, true, true };
    juce::TextButton sectionButtons[MixerStrip::numSections];

    juce::String makeTrackSignature();
};
