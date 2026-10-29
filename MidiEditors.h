#pragma once
#include "Theme.h"

//==============================================================================
// Piano roll grid (lives inside a Viewport for vertical scrolling)
//==============================================================================
class NoteGrid : public juce::Component
{
public:
    static constexpr int   keyWidth = 48;
    static constexpr float noteHeight = 12.0f;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

    float noteToY (int note) const { return (127 - note) * noteHeight; }

    te::Edit* edit = nullptr;
    te::EditItemID clipID;
    double snap = 0.25;          // in beats (0.25 = 1/16)

private:
    enum class Mode { none, move, resize, velocity };
    Mode mode = Mode::none;
    te::MidiNote* dragNote = nullptr;
    double start0 = 0, len0 = 0, downBeat = 0;
    int pitch0 = 60, vel0 = 100;
    double lastLength = 0.25;

    te::MidiClip* clip() const { return ldaw::findMidiClip (edit, clipID); }
    double clipLengthBeats() const;
    double pixelsPerBeat() const;
    float  beatToX (double b) const   { return keyWidth + (float) (b * pixelsPerBeat()); }
    double xToBeat (float x) const    { return juce::jmax (0.0, (x - keyWidth) / pixelsPerBeat()); }
    int    yToNote (float y) const    { return juce::jlimit (0, 127, 127 - (int) (y / noteHeight)); }
    te::MidiNote* noteAt (juce::Point<float>, bool& onRightEdge) const;
    juce::UndoManager* um() const     { return edit != nullptr ? &edit->getUndoManager() : nullptr; }
};

//==============================================================================
// Drum step sequencer (16 steps per bar, one row per drum sound)
//==============================================================================
class StepGrid : public juce::Component
{
public:
    StepGrid();

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

    int numBars() const;

    te::Edit* edit = nullptr;
    te::EditItemID clipID;
    int page = 0;                // which bar is shown

    struct Lane { juce::String name; int note; };
    std::vector<Lane> lanes;

private:
    static constexpr int labelWidth = 130;
    int lastRow = -1, lastStep = -1;
    bool paintAdds = true;

    te::MidiClip* clip() const { return ldaw::findMidiClip (edit, clipID); }
    te::MidiNote* findStep (int row, int step) const;
    void setStep (int row, int step, bool on, bool accent);
    bool cellAt (juce::Point<int>, int& row, int& step) const;
    juce::Rectangle<float> cellBounds (int row, int step) const;
    float rowH() const;
    void chooseLaneNote (int row);
};

//==============================================================================
// Bottom panel hosting both editors
//==============================================================================
class MidiEditorPanel : public juce::Component
{
public:
    MidiEditorPanel();

    void setClip (te::Edit*, te::EditItemID, bool stepMode);
    void tick();                       // called from the main timer
    te::EditItemID getClipID() const   { return clipID; }
    bool isStepMode() const            { return stepMode; }

    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void()> onClose;

private:
    te::Edit* edit = nullptr;
    te::EditItemID clipID;
    bool stepMode = false;

    juce::TextButton pianoTab { "Piano roll" }, stepTab { "Step (drums)" },
                     prevBar { "<" }, nextBar { ">" }, clearButton { "Clear" }, closeButton { "Close" };
    juce::ComboBox   snapBox;
    juce::Label      clipLabel, barLabel, hintLabel;
    juce::Viewport   pianoView;
    NoteGrid         noteGrid;
    StepGrid         stepGrid;

    void setMode (bool step);
    void updateLabels();
};
