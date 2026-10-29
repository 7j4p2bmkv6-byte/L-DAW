#include "MidiEditors.h"

using namespace ldaw;

namespace
{
    bool isBlackKey (int n)
    {
        const int k = n % 12;
        return k == 1 || k == 3 || k == 6 || k == 8 || k == 10;
    }

    juce::Colour velocityColour (int vel)
    {
        return colours::accent.withMultipliedBrightness (0.55f + 0.45f * (float) vel / 127.0f);
    }

    double clipBeats (te::Edit* e, te::MidiClip* c)
    {
        if (e == nullptr || c == nullptr) return 4.0;
        auto p = c->getPosition();
        return juce::jmax (0.25, beatOf (*e, p.getEnd()) - beatOf (*e, p.getStart()));
    }

    // Playhead position relative to clip start (or -1 if outside)
    double playheadInClip (te::Edit* e, te::MidiClip* c)
    {
        if (e == nullptr || c == nullptr) return -1.0;
        const double now = beatOf (*e, e->getTransport().getPosition());
        const double s   = beatOf (*e, c->getPosition().getStart());
        const double rel = now - s;
        return (rel >= 0.0 && rel < clipBeats (e, c)) ? rel : -1.0;
    }
}

//==============================================================================
// NoteGrid
//==============================================================================
double NoteGrid::clipLengthBeats() const { return clipBeats (edit, clip()); }
double NoteGrid::pixelsPerBeat() const   { return juce::jmax (1.0, (getWidth() - keyWidth) / clipLengthBeats()); }

te::MidiNote* NoteGrid::noteAt (juce::Point<float> p, bool& onRightEdge) const
{
    onRightEdge = false;
    auto* c = clip();
    if (c == nullptr) return nullptr;

    const int pitch = yToNote (p.y);
    auto& notes = c->getSequence().getNotes();

    for (int i = notes.size(); --i >= 0;)
    {
        auto* n = notes[i];
        if (n->getNoteNumber() != pitch) continue;

        const float x1 = beatToX (n->getStartBeat().inBeats());
        const float x2 = beatToX (n->getEndBeat().inBeats());
        if (p.x >= x1 && p.x < juce::jmax (x2, x1 + 4.0f))
        {
            onRightEdge = p.x > x2 - 6.0f;
            return n;
        }
    }
    return nullptr;
}

void NoteGrid::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);
    auto* c = clip();
    const float w = (float) getWidth();

    // rows
    for (int n = 0; n < 128; ++n)
    {
        const float y = noteToY (n);
        g.setColour (isBlackKey (n) ? colours::background.darker (0.25f) : colours::background.brighter (0.04f));
        g.fillRect ((float) keyWidth, y, w - keyWidth, noteHeight);
        if (n % 12 == 0)
        {
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.drawHorizontalLine ((int) (y + noteHeight) - 1, (float) keyWidth, w);
        }
    }

    // vertical grid
    const double len = clipLengthBeats();
    for (double b = 0.0; b <= len + 1e-6; b += snap)
    {
        const bool bar  = std::fmod (b, 4.0) < 1e-6;
        const bool beat = std::fmod (b, 1.0) < 1e-6;
        g.setColour (juce::Colours::white.withAlpha (bar ? 0.22f : beat ? 0.10f : 0.04f));
        g.drawVerticalLine ((int) beatToX (b), 0.0f, (float) getHeight());
    }

    // notes
    if (c != nullptr)
    {
        for (auto* n : c->getSequence().getNotes())
        {
            const double s = n->getStartBeat().inBeats();
            const double l = n->getLengthBeats().inBeats();
            juce::Rectangle<float> r (beatToX (s), noteToY (n->getNoteNumber()) + 1.0f,
                                      juce::jmax (4.0f, (float) (l * pixelsPerBeat()) - 1.0f), noteHeight - 2.0f);
            g.setColour (velocityColour (n->getVelocity()));
            g.fillRoundedRectangle (r, 2.0f);
            g.setColour (n == dragNote ? juce::Colours::white : colours::accent.brighter (0.4f));
            g.drawRoundedRectangle (r, 2.0f, n == dragNote ? 1.5f : 0.8f);
        }

        const double ph = playheadInClip (edit, c);
        if (ph >= 0.0)
        {
            g.setColour (juce::Colours::white);
            g.drawVerticalLine ((int) beatToX (ph), 0.0f, (float) getHeight());
        }
    }

    // keyboard
    for (int n = 0; n < 128; ++n)
    {
        const float y = noteToY (n);
        g.setColour (isBlackKey (n) ? juce::Colour (0xff111111) : juce::Colour (0xffe9e6df));
        g.fillRect (0.0f, y, (float) keyWidth, noteHeight - 1.0f);
        if (n % 12 == 0)
        {
            g.setColour (juce::Colours::black);
            g.setFont (juce::Font (juce::FontOptions (10.0f)));
            g.drawText (juce::MidiMessage::getMidiNoteName (n, true, true, 3),
                        2, (int) y, keyWidth - 4, (int) noteHeight, juce::Justification::centredRight);
        }
    }

    if (c == nullptr)
    {
        g.setColour (colours::dimText);
        g.drawText ("Double-click a clip (or an empty spot in a track) to edit notes here.",
                    getLocalBounds().withTrimmedLeft (keyWidth).withHeight (400).withY ((int) noteToY (72)),
                    juce::Justification::centredTop);
    }
}

void NoteGrid::mouseMove (const juce::MouseEvent& e)
{
    bool edge = false;
    auto* n = noteAt (e.position, edge);
    setMouseCursor (n == nullptr ? juce::MouseCursor::NormalCursor
                                 : edge ? juce::MouseCursor::LeftRightResizeCursor
                                        : juce::MouseCursor::DraggingHandCursor);
}

void NoteGrid::mouseDown (const juce::MouseEvent& e)
{
    mode = Mode::none;
    dragNote = nullptr;

    auto* c = clip();
    if (c == nullptr || e.x < keyWidth) return;

    edit->getUndoManager().beginNewTransaction();

    bool edge = false;
    auto* n = noteAt (e.position, edge);

    if (n != nullptr && e.mods.isPopupMenu())      // right-click = delete
    {
        c->getSequence().removeNote (*n, um());
        repaint();
        return;
    }

    if (n == nullptr)
    {
        if (e.mods.isPopupMenu()) return;

        // add a new note, then let the drag set its length
        const double len = clipLengthBeats();
        double start = std::floor (xToBeat (e.position.x) / snap) * snap;
        if (start >= len) return;
        const double l = juce::jmin (lastLength, len - start);

        n = c->getSequence().addNote (yToNote (e.position.y), te::BeatPosition::fromBeats (start),
                                      te::BeatDuration::fromBeats (l), 100, 0, um());
        mode = Mode::resize;
    }
    else
    {
        mode = e.mods.isShiftDown() ? Mode::velocity : edge ? Mode::resize : Mode::move;
    }

    dragNote = n;
    if (n != nullptr)
    {
        start0   = n->getStartBeat().inBeats();
        len0     = n->getLengthBeats().inBeats();
        pitch0   = n->getNoteNumber();
        vel0     = n->getVelocity();
        downBeat = xToBeat (e.position.x);
    }
    repaint();
}

void NoteGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragNote == nullptr || clip() == nullptr) return;

    const double len = clipLengthBeats();
    const double free = e.mods.isAltDown() ? 1.0 / 64.0 : snap;   // Alt = fine

    if (mode == Mode::move)
    {
        const double delta = std::round ((xToBeat (e.position.x) - downBeat) / free) * free;
        const double s = juce::jlimit (0.0, juce::jmax (0.0, len - free), start0 + delta);
        const int dy = (int) std::round (e.getDistanceFromDragStartY() / noteHeight);
        dragNote->setStartAndLength (te::BeatPosition::fromBeats (s), te::BeatDuration::fromBeats (len0), um());
        dragNote->setNoteNumber (juce::jlimit (0, 127, pitch0 - dy), um());
    }
    else if (mode == Mode::resize)
    {
        const double end = std::round (xToBeat (e.position.x) / free) * free;
        const double l = juce::jlimit (free, juce::jmax (free, len - start0), end - start0);
        dragNote->setStartAndLength (te::BeatPosition::fromBeats (start0), te::BeatDuration::fromBeats (l), um());
        lastLength = l;
    }
    else if (mode == Mode::velocity)
    {
        dragNote->setVelocity (juce::jlimit (1, 127, vel0 - e.getDistanceFromDragStartY() / 2), um());
    }
    repaint();
}

void NoteGrid::mouseUp (const juce::MouseEvent&)
{
    mode = Mode::none;
    dragNote = nullptr;
    repaint();
}

//==============================================================================
// StepGrid
//==============================================================================
StepGrid::StepGrid()
{
    // General MIDI drum map - right-click a name to change the note for your drum machine
    lanes = { { "Kick",        36 }, { "Rim shot",   37 }, { "Snare",     38 }, { "Clap",      39 },
              { "Closed hat",  42 }, { "Open hat",   46 }, { "Low tom",   45 }, { "Hi tom",    50 },
              { "Cowbell",     56 }, { "Cymbal",     49 } };
}

int StepGrid::numBars() const
{
    auto* c = clip();
    return juce::jmax (1, (int) std::ceil (clipBeats (edit, c) / 4.0 - 1e-6));
}

float StepGrid::rowH() const { return juce::jmax (12.0f, (float) getHeight() / (float) lanes.size()); }

juce::Rectangle<float> StepGrid::cellBounds (int row, int step) const
{
    const float cw = (getWidth() - labelWidth) / 16.0f;
    return { labelWidth + step * cw + 2.0f, row * rowH() + 2.0f, cw - 4.0f, rowH() - 4.0f };
}

bool StepGrid::cellAt (juce::Point<int> p, int& row, int& step) const
{
    row = (int) (p.y / rowH());
    step = (int) ((p.x - labelWidth) / ((getWidth() - labelWidth) / 16.0f));
    return p.x >= labelWidth && juce::isPositiveAndBelow (row, (int) lanes.size()) && juce::isPositiveAndBelow (step, 16);
}

te::MidiNote* StepGrid::findStep (int row, int step) const
{
    auto* c = clip();
    if (c == nullptr) return nullptr;

    const double b0 = page * 4.0 + step * 0.25;
    for (auto* n : c->getSequence().getNotes())
    {
        const double s = n->getStartBeat().inBeats();
        if (n->getNoteNumber() == lanes[(size_t) row].note && s >= b0 - 1e-4 && s < b0 + 0.25 - 1e-4)
            return n;
    }
    return nullptr;
}

void StepGrid::setStep (int row, int step, bool on, bool accent)
{
    auto* c = clip();
    if (c == nullptr) return;

    const double beat = page * 4.0 + step * 0.25;
    if (beat >= clipBeats (edit, c)) return;

    auto* um = &edit->getUndoManager();
    auto* existing = findStep (row, step);

    if (on && existing == nullptr)
        c->getSequence().addNote (lanes[(size_t) row].note, te::BeatPosition::fromBeats (beat),
                                  te::BeatDuration::fromBeats (0.25), accent ? 127 : 96, 0, um);
    else if (on && existing != nullptr)
        existing->setVelocity (accent ? 127 : 96, um);
    else if (! on && existing != nullptr)
        c->getSequence().removeNote (*existing, um);
}

void StepGrid::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);
    auto* c = clip();
    const double len = clipBeats (edit, c);
    const double ph = playheadInClip (edit, c);
    const int playStep = (ph >= page * 4.0 && ph < page * 4.0 + 4.0) ? (int) ((ph - page * 4.0) / 0.25) : -1;

    g.setFont (juce::Font (juce::FontOptions (13.0f)));

    for (int r = 0; r < (int) lanes.size(); ++r)
    {
        const float y = r * rowH();
        g.setColour (colours::panel);
        g.fillRect (0.0f, y + 1.0f, (float) labelWidth - 4.0f, rowH() - 2.0f);
        g.setColour (colours::text);
        g.drawText (lanes[(size_t) r].name, 8, (int) y, labelWidth - 50, (int) rowH(), juce::Justification::centredLeft);
        g.setColour (colours::dimText);
        g.drawText (juce::String (lanes[(size_t) r].note), labelWidth - 44, (int) y, 36, (int) rowH(), juce::Justification::centredRight);

        for (int s = 0; s < 16; ++s)
        {
            auto cell = cellBounds (r, s);
            const bool valid = page * 4.0 + s * 0.25 < len;
            auto* n = valid ? findStep (r, s) : nullptr;

            juce::Colour base = (s / 4) % 2 == 0 ? colours::panelLight : colours::panel;
            if (! valid) base = colours::background.brighter (0.02f);

            g.setColour (n != nullptr ? (n->getVelocity() >= 115 ? colours::accent.brighter (0.35f) : colours::accent)
                                      : base);
            g.fillRoundedRectangle (cell, 3.0f);

            if (s == playStep)
            {
                g.setColour (juce::Colours::white.withAlpha (0.6f));
                g.drawRoundedRectangle (cell, 3.0f, 1.5f);
            }
        }
    }

    if (c == nullptr)
    {
        g.setColour (colours::dimText);
        g.drawText ("Double-click a clip on the drum track to program it here.",
                    getLocalBounds().withTrimmedLeft (labelWidth), juce::Justification::centred);
    }
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    lastRow = lastStep = -1;
    if (clip() == nullptr) return;

    const int row = (int) (e.y / rowH());
    if (e.x < labelWidth)
    {
        if (juce::isPositiveAndBelow (row, (int) lanes.size()))
            chooseLaneNote (row);
        return;
    }

    int r = 0, s = 0;
    if (! cellAt (e.getPosition(), r, s)) return;

    edit->getUndoManager().beginNewTransaction();

    auto* existing = findStep (r, s);
    const bool accent = e.mods.isShiftDown();

    if (accent && existing != nullptr)         // Shift on a lit step toggles accent
    {
        setStep (r, s, true, existing->getVelocity() < 115);
        paintAdds = true;
    }
    else
    {
        paintAdds = existing == nullptr;
        setStep (r, s, paintAdds, accent);
    }

    lastRow = r; lastStep = s;
    repaint();
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    int r = 0, s = 0;
    if (clip() == nullptr || ! cellAt (e.getPosition(), r, s)) return;
    if (r == lastRow && s == lastStep) return;

    setStep (r, s, paintAdds, e.mods.isShiftDown());
    lastRow = r; lastStep = s;
    repaint();
}

void StepGrid::chooseLaneNote (int row)
{
    juce::PopupMenu m;
    const int current = lanes[(size_t) row].note;
    for (int n = 24; n <= 96; ++n)
        m.addItem (n + 1, juce::String (n) + "   " + juce::MidiMessage::getMidiNoteName (n, true, true, 3),
                   true, n == current);

    m.showMenuAsync (juce::PopupMenu::Options().withMaximumNumColumns (4),
                     [safe = juce::Component::SafePointer<StepGrid> (this), row] (int r)
    {
        if (safe == nullptr || r == 0) return;
        safe->lanes[(size_t) row].note = r - 1;
        safe->repaint();
    });
}

//==============================================================================
// MidiEditorPanel
//==============================================================================
MidiEditorPanel::MidiEditorPanel()
{
    for (auto* b : { &pianoTab, &stepTab })
    {
        b->setClickingTogglesState (false);
        b->setColour (juce::TextButton::buttonOnColourId, colours::accent);
        b->setColour (juce::TextButton::buttonColourId, colours::panelLight);
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (b);
    }
    for (auto* b : { &prevBar, &nextBar, &clearButton, &closeButton })
    {
        b->setColour (juce::TextButton::buttonColourId, colours::panelLight);
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (b);
    }

    pianoTab.onClick    = [this] { setMode (false); };
    stepTab.onClick     = [this] { setMode (true); };
    closeButton.onClick = [this] { if (onClose) onClose(); };
    prevBar.onClick     = [this] { stepGrid.page = juce::jmax (0, stepGrid.page - 1); updateLabels(); stepGrid.repaint(); };
    nextBar.onClick     = [this] { stepGrid.page = juce::jmin (stepGrid.numBars() - 1, stepGrid.page + 1); updateLabels(); stepGrid.repaint(); };
    clearButton.onClick = [this]
    {
        if (auto* c = findMidiClip (edit, clipID))
        {
            edit->getUndoManager().beginNewTransaction();
            c->getSequence().clear (&edit->getUndoManager());
            noteGrid.repaint(); stepGrid.repaint();
        }
    };

    snapBox.addItem ("Grid 1/4",  4);
    snapBox.addItem ("Grid 1/8",  8);
    snapBox.addItem ("Grid 1/16", 16);
    snapBox.addItem ("Grid 1/32", 32);
    snapBox.setSelectedId (16, juce::dontSendNotification);
    snapBox.onChange = [this] { noteGrid.snap = 4.0 / snapBox.getSelectedId(); noteGrid.repaint(); };
    snapBox.setWantsKeyboardFocus (false);
    addAndMakeVisible (snapBox);

    clipLabel.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    clipLabel.setColour (juce::Label::textColourId, colours::accent);
    barLabel.setColour (juce::Label::textColourId, colours::text);
    barLabel.setJustificationType (juce::Justification::centred);
    hintLabel.setColour (juce::Label::textColourId, colours::dimText);
    hintLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (clipLabel);
    addAndMakeVisible (barLabel);
    addAndMakeVisible (hintLabel);

    pianoView.setViewedComponent (&noteGrid, false);
    pianoView.setScrollBarsShown (true, false);
    addAndMakeVisible (pianoView);
    addChildComponent (stepGrid);

    setMode (false);
}

void MidiEditorPanel::setClip (te::Edit* e, te::EditItemID id, bool step)
{
    const bool changed = ! (id == clipID) || e != edit;
    edit = e;
    clipID = id;
    noteGrid.edit = stepGrid.edit = e;
    noteGrid.clipID = stepGrid.clipID = id;

    if (changed)
    {
        stepGrid.page = 0;

        // scroll the piano roll to the notes in the clip (or around C3-C5)
        int centre = 66;
        if (auto* c = findMidiClip (e, id))
        {
            auto& notes = c->getSequence().getNotes();
            if (notes.size() > 0)
            {
                int sum = 0;
                for (auto* n : notes) sum += n->getNoteNumber();
                centre = sum / notes.size();
            }
        }
        resized();
        pianoView.setViewPosition (0, juce::jmax (0, (int) noteGrid.noteToY (centre) - pianoView.getHeight() / 2));
    }

    setMode (step);
}

void MidiEditorPanel::setMode (bool step)
{
    stepMode = step;
    pianoTab.setToggleState (! step, juce::dontSendNotification);
    stepTab.setToggleState (step, juce::dontSendNotification);
    pianoView.setVisible (! step);
    stepGrid.setVisible (step);
    snapBox.setVisible (! step);
    prevBar.setVisible (step);
    nextBar.setVisible (step);
    barLabel.setVisible (step);
    updateLabels();
    resized();
}

void MidiEditorPanel::updateLabels()
{
    auto* c = findMidiClip (edit, clipID);
    clipLabel.setText (c != nullptr ? c->getName() + "  (" + c->getTrack()->getName() + ")" : "No clip selected",
                       juce::dontSendNotification);

    stepGrid.page = juce::jlimit (0, stepGrid.numBars() - 1, stepGrid.page);
    barLabel.setText ("Bar " + juce::String (stepGrid.page + 1) + " / " + juce::String (stepGrid.numBars()),
                      juce::dontSendNotification);

    hintLabel.setText (stepMode
        ? "Click/drag = set steps.  Shift = accent.  Right-click a drum name to change its MIDI note."
        : "Click = add note, drag = length.  Drag note = move.  Edge = resize.  Shift-drag = velocity.  Right-click = delete.  Alt = fine.",
        juce::dontSendNotification);
}

void MidiEditorPanel::tick()
{
    if (! isVisible()) return;
    updateLabels();
    if (stepMode) stepGrid.repaint();
    else          noteGrid.repaint();
}

void MidiEditorPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::panel);
    g.setColour (colours::accent);
    g.fillRect (0, 0, getWidth(), 2);
}

void MidiEditorPanel::resized()
{
    auto r = getLocalBounds().withTrimmedTop (2);
    auto header = r.removeFromTop (34).reduced (6, 4);

    pianoTab.setBounds (header.removeFromLeft (100));
    header.removeFromLeft (4);
    stepTab.setBounds (header.removeFromLeft (110));
    header.removeFromLeft (12);
    clipLabel.setBounds (header.removeFromLeft (230));

    closeButton.setBounds (header.removeFromRight (70));
    header.removeFromRight (6);
    clearButton.setBounds (header.removeFromRight (70));
    header.removeFromRight (10);

    if (stepMode)
    {
        nextBar.setBounds (header.removeFromRight (30));
        barLabel.setBounds (header.removeFromRight (90));
        prevBar.setBounds (header.removeFromRight (30));
    }
    else
    {
        snapBox.setBounds (header.removeFromRight (110));
    }
    header.removeFromRight (10);
    hintLabel.setBounds (header);

    pianoView.setBounds (r);
    noteGrid.setSize (juce::jmax (200, pianoView.getMaximumVisibleWidth()), (int) (128 * NoteGrid::noteHeight));
    stepGrid.setBounds (r.reduced (6, 4));
}
