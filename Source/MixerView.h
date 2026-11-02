#pragma once
#include "Effects.h"

//==============================================================================
// Simple stereo peak meter fed by a Tracktion LevelMeasurer
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
// Button that also reports right-clicks
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
// One mixer channel: music track, send-FX return or master
//==============================================================================
class MixerStrip : public juce::Component
{
public:
    enum class Kind { music, fxReturn, master };

    MixerStrip (te::Edit&, te::AudioTrack*, Kind, int index);

    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();                 // periodic refresh from the main timer

    te::AudioTrack* track;
    const Kind kind;

    std::function<void()> onStructureChanged;

private:
    te::Edit& edit;
    const int index;

    juce::Label nameLabel;
    juce::TextButton instrumentButton;
    juce::OwnedArray<SlotButton> insertButtons;
    juce::TextButton addFxButton { "+ FX" };
    juce::OwnedArray<juce::Slider> sendKnobs;
    juce::OwnedArray<juce::Label>  sendLabels;
    juce::Slider pan, fader;
    juce::TextButton muteButton { "M" }, soloButton { "S" };
    LevelMeter meter;
    juce::String insertSignature;

    te::PluginList& chain();
    te::VolumeAndPanPlugin* volume();
    void rebuildInserts();
    void showInsertMenu (te::Plugin*, juce::Component*);
    juce::String makeSignature();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerStrip)
};

//==============================================================================
class MixerView : public juce::Component
{
public:
    MixerView();

    void setEdit (te::Edit*);
    void rebuild();
    void tick();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    te::Edit* edit = nullptr;
    juce::Viewport viewport;
    juce::Component content;
    juce::OwnedArray<MixerStrip> strips;
    juce::String trackSignature;

    juce::String makeTrackSignature();
};
