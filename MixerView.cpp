#include "MixerView.h"

using namespace ldaw;

namespace
{
    constexpr int stripWidth  = 96;
    constexpr int masterWidth = 168;
    constexpr int sideWidth   = 58;
    constexpr int labelBarH   = 15;

    const juce::Colour panelGrey  { 0xffc6c8cc };
    const juce::Colour panelLight { 0xffd5d7da };
    const juce::Colour panelEdge  { 0xff7d8086 };
    const juce::Colour labelBar   { 0xff2f3135 };
    const juce::Colour textDark   { 0xff1c1d20 };
    const juce::Colour rackBlack  { 0xff151618 };

    const char* sectionNames[]   = { "DYNAMICS", "EQ", "INSERTS", "FX SEND" };
    const juce::uint32 sectionInk[] = { 0xffe8a23a, 0xff7fd16b, 0xffe8c84a, 0xff4fd1c5 };

    juce::String sendShortName (te::Edit& e, int bus)
    {
        for (auto* t : getReturnTracks (e))
            if (returnBusOf (*t) == bus)
            {
                auto inserts = fx::getInserts (t->pluginList);
                if (! inserts.isEmpty())
                    return juce::String::charToString ((juce::juce_wchar) ('1' + bus)) + " "
                         + fx::displayName (*inserts.getFirst()).substring (0, 6);
            }
        return juce::String (bus + 1);
    }

    // send knob: 0..1 <-> gain (squared law, max +6 dB)
    double sendNormToDb (double n)  { const double g = n * n * 2.0; return g <= 0.001 ? -60.0 : 20.0 * std::log10 (g); }
    double sendDbToNorm (double db) { return db <= -59.9 ? 0.0 : std::sqrt (juce::Decibels::decibelsToGain (db) / 2.0); }
}

//==============================================================================
// ReasonLook
//==============================================================================
ReasonLook::ReasonLook()
{
    setColour (juce::TextButton::buttonColourId,       juce::Colour (0xffdcdee1));
    setColour (juce::TextButton::textColourOffId,      textDark);
    setColour (juce::TextButton::textColourOnId,       textDark);
    setColour (juce::Label::textColourId,              textDark);
    setColour (juce::Slider::textBoxTextColourId,      juce::Colour (0xff9df08a));
    setColour (juce::Slider::textBoxBackgroundColourId, rackBlack);
    setColour (juce::Slider::textBoxOutlineColourId,   juce::Colours::transparentBlack);
    setColour (juce::ScrollBar::thumbColourId,         juce::Colour (0xff8e9196));
}

void ReasonLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                   float startAngle, float endAngle, juce::Slider& s)
{
    const float size = (float) juce::jmin (w, h) - 4.0f;
    const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
    const float radius = size * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // scale dots
    g.setColour (juce::Colour (0xff4b4e54));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const float dx = std::sin (a) * (radius + 0.5f), dy = -std::cos (a) * (radius + 0.5f);
        g.fillEllipse (cx + dx - 0.9f, cy + dy - 0.9f, 1.8f, 1.8f);
    }

    // skirt
    const float skirt = radius * 0.86f;
    g.setColour (juce::Colour (0xff2a2b2e));
    g.fillEllipse (cx - skirt, cy - skirt, skirt * 2.0f, skirt * 2.0f);

    // coloured cap with highlight
    const float cap = radius * 0.72f;
    auto col = s.findColour (juce::Slider::rotarySliderFillColourId);
    juce::ColourGradient grad (col.brighter (0.55f), cx - cap * 0.5f, cy - cap * 0.6f,
                               col.darker (0.45f),  cx + cap * 0.6f, cy + cap * 0.8f, true);
    g.setGradientFill (grad);
    g.fillEllipse (cx - cap, cy - cap, cap * 2.0f, cap * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (cx - cap, cy - cap, cap * 2.0f, cap * 2.0f, 0.8f);

    // pointer
    juce::Path p;
    p.addRoundedRectangle (-1.2f, -skirt, 2.4f, skirt * 0.62f, 1.0f);
    p.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
    g.setColour (juce::Colours::white);
    g.fillPath (p);
}

void ReasonLook::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                                   juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, minPos, maxPos, style, s);
        return;
    }

    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float cx = r.getCentreX();

    // slot
    g.setColour (rackBlack);
    g.fillRoundedRectangle (cx - 3.0f, r.getY(), 6.0f, r.getHeight(), 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawRoundedRectangle (cx - 3.0f, r.getY(), 6.0f, r.getHeight(), 3.0f, 0.6f);

    // ticks
    g.setColour (juce::Colour (0xff5c5f65));
    for (int i = 0; i <= 10; ++i)
    {
        const float ty = r.getY() + (float) i / 10.0f * r.getHeight();
        g.drawHorizontalLine ((int) ty, cx - 12.0f, cx - 6.0f);
        g.drawHorizontalLine ((int) ty, cx + 6.0f, cx + 12.0f);
    }

    // metal cap
    const float capW = juce::jmin (r.getWidth() - 2.0f, 30.0f), capH = 28.0f;
    juce::Rectangle<float> capR (cx - capW * 0.5f, pos - capH * 0.5f, capW, capH);
    juce::ColourGradient grad (juce::Colour (0xfff4f5f6), 0.0f, capR.getY(),
                               juce::Colour (0xff868a90), 0.0f, capR.getBottom(), false);
    grad.addColour (0.48, juce::Colour (0xffc9ccd0));
    grad.addColour (0.52, juce::Colour (0xffa4a8ad));
    g.setGradientFill (grad);
    g.fillRoundedRectangle (capR, 3.0f);
    g.setColour (juce::Colour (0xff3c3e42));
    g.drawRoundedRectangle (capR, 3.0f, 1.0f);
    g.setColour (juce::Colours::black);
    g.fillRect (capR.getX() + 3.0f, capR.getCentreY() - 1.0f, capR.getWidth() - 6.0f, 2.0f);
}

void ReasonLook::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                       bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    auto base = on ? b.findColour (juce::TextButton::buttonOnColourId) : juce::Colour (0xffdcdee1);
    if (down)             base = base.darker (0.15f);
    else if (highlighted) base = base.brighter (0.06f);

    juce::ColourGradient grad (base.brighter (0.18f), 0.0f, r.getY(), base.darker (0.12f), 0.0f, r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 2.5f);
    g.setColour (panelEdge.darker (0.2f));
    g.drawRoundedRectangle (r, 2.5f, 1.0f);
}

juce::Font ReasonLook::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::Font (juce::FontOptions (juce::jmin (12.0f, (float) buttonHeight * 0.62f), juce::Font::bold));
}

//==============================================================================
// LevelMeter
//==============================================================================
void LevelMeter::attach (te::LevelMeterPlugin* p)
{
    if (p == meterPlugin.get()) return;
    detach();
    if (p == nullptr) return;

    meterPlugin = te::Plugin::Ptr (p);
    measurer = &p->measurer;
    measurer->addClient (client);
}

void LevelMeter::detach()
{
    if (measurer != nullptr)
        measurer->removeClient (client);
    measurer = nullptr;
    meterPlugin = nullptr;
}

void LevelMeter::tick()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        float db = -100.0f;
        if (measurer != nullptr)
            db = client.getAndClearAudioLevel (ch).dB;

        levels[ch] = db > levels[ch] ? db : juce::jmax (-100.0f, levels[ch] - 2.5f);

        if (levels[ch] >= peaks[ch]) { peaks[ch] = levels[ch]; peakHold[ch] = 30; }
        else if (--peakHold[ch] <= 0) peaks[ch] = juce::jmax (-100.0f, peaks[ch] - 1.5f);
    }
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float scaleW = 15.0f;
    auto bars = r.withTrimmedLeft (scaleW);

    g.setColour (rackBlack);
    g.fillRoundedRectangle (bars, 2.0f);

    auto toNorm = [] (float db) { return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f); };   // -60..+6 dB

    // dB scale (like Reason: 6, 0, 6, 12, 20, 30, 40, 60)
    g.setFont (juce::Font (juce::FontOptions (8.5f)));
    for (float db : { 6.0f, 0.0f, -6.0f, -12.0f, -20.0f, -30.0f, -40.0f, -60.0f })
    {
        const float y = bars.getBottom() - 3.0f - toNorm (db) * (bars.getHeight() - 6.0f);
        g.setColour (textDark);
        g.drawText (juce::String ((int) std::abs (db)), juce::Rectangle<float> (r.getX(), y - 5.0f, scaleW - 2.0f, 10.0f),
                    juce::Justification::centredRight);
    }

    constexpr int numLeds = 30;
    const float inner = bars.getHeight() - 6.0f;
    const float ledH = inner / (float) numLeds;
    const float colW = (bars.getWidth() - 6.0f) / 2.0f;

    for (int ch = 0; ch < 2; ++ch)
    {
        const float x = bars.getX() + 2.0f + (float) ch * (colW + 2.0f);
        const float level = toNorm (levels[ch]);
        const float peak  = toNorm (peaks[ch]);

        for (int i = 0; i < numLeds; ++i)
        {
            const float segNorm = ((float) i + 0.5f) / (float) numLeds;
            const float dbAt = segNorm * 66.0f - 60.0f;
            auto col = dbAt > 0.0f ? juce::Colour (0xffe33a2e) : dbAt > -12.0f ? juce::Colour (0xfff2c43a)
                                                                               : juce::Colour (0xff46d15a);
            const bool lit = segNorm <= level || std::abs (segNorm - peak) < 0.5f / (float) numLeds;
            g.setColour (lit ? col : col.withAlpha (0.12f));
            const float y = bars.getBottom() - 3.0f - (float) (i + 1) * ledH;
            g.fillRect (x, y + 0.6f, colW, ledH - 1.2f);
        }
    }
}

//==============================================================================
// ParamKnob
//==============================================================================
ParamKnob::ParamKnob (juce::Colour c, const juce::String& l, double def) : label (l), defaultNorm (def)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (0.0, 1.0, 0.0);
    slider.setColour (juce::Slider::rotarySliderFillColourId, c);
    slider.setDoubleClickReturnValue (true, def);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setValue (def, juce::dontSendNotification);

    slider.textFromValueFunction = [this] (double v) -> juce::String
    {
        if (resolve)
            if (auto* p = resolve (false))
                return p->getCurrentValueAsString();
        if (textFn) return textFn (v);
        return juce::String (v, 2);
    };

    slider.onDragStart = [this] { if (resolve) if (auto* p = resolve (true)) p->parameterChangeGestureBegin(); };
    slider.onDragEnd   = [this] { if (resolve) if (auto* p = resolve (false)) p->parameterChangeGestureEnd(); };
    slider.onValueChange = [this]
    {
        if (resolve)
        {
            if (auto* p = resolve (true))
            {
                p->setNormalisedParameter ((float) slider.getValue(), juce::sendNotification);
                return;
            }
        }
        if (setter) setter (slider.getValue());
    };
    addAndMakeVisible (slider);
}

void ParamKnob::tick()
{
    if (slider.isMouseButtonDown()) return;

    double v = defaultNorm;
    bool live = false;

    if (resolve)
        if (auto* p = resolve (false))
        {
            v = p->getCurrentNormalisedValue();
            live = true;
        }

    if (! live && getter)
    {
        v = getter();
        live = true;
    }

    slider.setValue (v, juce::dontSendNotification);
    slider.setAlpha (live ? 1.0f : 0.8f);
}

void ParamKnob::paint (juce::Graphics& g)
{
    g.setColour (textDark);
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.drawText (label, getLocalBounds().removeFromBottom (11), juce::Justification::centred, true);
}

void ParamKnob::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedBottom (11));
}

//==============================================================================
// MixerStrip
//==============================================================================
int MixerStrip::sectionHeight (int s)
{
    switch (s)
    {
        case secDyn:     return labelBarH + 52;
        case secEq:      return labelBarH + 4 * 42;
        case secInserts: return labelBarH + 4 * 19 + 24;
        case secSends:   return labelBarH + 2 * 48;
        default:         return 0;
    }
}

bool MixerStrip::hasSection (int s) const
{
    switch (s)
    {
        case secDyn:     return kind != Kind::fxReturn;
        case secEq:      return kind == Kind::music;
        case secInserts: return true;
        case secSends:   return kind == Kind::music;
        default:         return false;
    }
}

MixerStrip::MixerStrip (te::Edit& e, te::AudioTrack* t, Kind k, int i, juce::Colour c)
    : track (t), kind (k), edit (e), index (i), colour (c)
{
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    nameLabel.setColour (juce::Label::textColourId, colour.getPerceivedBrightness() > 0.6f ? textDark : juce::Colours::white);
    if (kind == Kind::music)
    {
        nameLabel.setEditable (false, true, false);
        nameLabel.onTextChange = [this] { if (track) track->setName (nameLabel.getText()); };
    }
    addAndMakeVisible (nameLabel);

    if (kind == Kind::music)
    {
        instrumentButton.setTooltip ("Open the instrument panel");
        instrumentButton.onClick = [this]
        {
            if (track == nullptr) return;
            for (auto p : track->pluginList.getPlugins())
                if (dynamic_cast<te::ExternalPlugin*> (&*p) != nullptr && p->isSynth())
                { fx::openWindow (*p); return; }
        };
        addAndMakeVisible (instrumentButton);
    }

    // ---- dynamics ---------------------------------------------------------
    if (hasSection (secDyn))
    {
        dynOn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xfff0a53a));
        dynOn.setTooltip ("Channel compressor on/off");
        dynOn.onClick = [this] { toggleChannelPlugin ("dyn"); };
        addAndMakeVisible (dynOn);

        auto addDyn = [this] (const juce::String& label, juce::StringArray keys, double def)
        {
            auto* knob = dynKnobs.add (new ParamKnob (juce::Colour (0xff3f9a46), label, def));
            knob->resolve = [this, keys] (bool create) -> te::AutomatableParameter*
            {
                auto* p = create ? fx::ensureChannelPlugin (edit, chain(), "dyn") : fx::findChannelPlugin (chain(), "dyn");
                return fx::paramByName (p, keys);
            };
            addAndMakeVisible (knob);
        };

        addDyn ("THRES", { "thresh" }, 1.0);
        addDyn ("RATIO", { "ratio" }, 0.2);
        if (kind == Kind::master)
        {
            addDyn ("ATTACK",  { "attack" }, 0.2);
            addDyn ("RELEASE", { "release" }, 0.3);
        }
    }

    // ---- EQ -------------------------------------------------------------------
    if (hasSection (secEq))
    {
        eqOn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff7fd16b));
        eqOn.setTooltip ("Channel EQ on/off");
        eqOn.onClick = [this] { toggleChannelPlugin ("eq"); };
        addAndMakeVisible (eqOn);

        struct Band { const char* name; juce::uint32 col; int band; };
        const Band bands[] = { { "HF", 0xffcf3b30, 3 }, { "HMF", 0xff3d9a3f, 2 }, { "LMF", 0xff3a63c4, 1 }, { "LF", 0xff3b3b3e, 0 } };

        for (auto& b : bands)
        {
            for (int f = 0; f < 2; ++f)
            {
                const bool isFreq = f == 1;
                auto* knob = eqKnobs.add (new ParamKnob (juce::Colour (b.col), isFreq ? juce::String ("FREQ") : juce::String (b.name), 0.5));
                const int band = b.band;
                knob->resolve = [this, band, isFreq] (bool create) -> te::AutomatableParameter*
                {
                    auto* p = create ? fx::ensureChannelPlugin (edit, chain(), "eq") : fx::findChannelPlugin (chain(), "eq");
                    return fx::eqParam (p, band, isFreq);
                };
                addAndMakeVisible (knob);
            }
        }
    }

    // ---- inserts --------------------------------------------------------------
    addFxButton.setTooltip ("Add an insert effect");
    addFxButton.onClick = [this]
    {
        fx::showAddEffectMenu (edit, chain(), &addFxButton, [safe = juce::Component::SafePointer<MixerStrip> (this)]
        {
            if (safe != nullptr) safe->rebuildInserts();
        });
    };
    addAndMakeVisible (addFxButton);

    // ---- sends ----------------------------------------------------------------
    if (hasSection (secSends))
    {
        for (int bus = 0; bus < numSendBuses; ++bus)
        {
            auto* knob = sendKnobs.add (new ParamKnob (colours::send, "LEVEL", 0.0));
            knob->getter = [this, bus] { return track != nullptr ? sendDbToNorm (fx::getSendDb (*track, bus)) : 0.0; };
            knob->setter = [this, bus] (double v) { if (track != nullptr) fx::setSendDb (edit, *track, bus, (float) sendNormToDb (v)); };
            knob->textFn = [] (double v) { const double db = sendNormToDb (v); return db <= -59.9 ? juce::String ("-inf") : juce::String (db, 1) + " dB"; };
            addAndMakeVisible (knob);

            auto* l = sendLabels.add (new juce::Label());
            l->setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
            l->setJustificationType (juce::Justification::centred);
            l->setColour (juce::Label::textColourId, textDark);
            l->setText (sendShortName (edit, bus), juce::dontSendNotification);
            l->setInterceptsMouseClicks (false, false);
            addAndMakeVisible (l);
        }
    }

    // ---- pan / fader / mute / solo ---------------------------------------------
    pan.resolve = [this] (bool) -> te::AutomatableParameter* { return fx::paramByName (volume(), { "pan" }); };
    pan.getter  = [this] { auto* v = volume(); return v != nullptr ? (v->getPan() + 1.0) * 0.5 : 0.5; };
    pan.setter  = [this] (double n) { if (auto* v = volume()) v->setPan ((float) (n * 2.0 - 1.0)); };
    addAndMakeVisible (pan);

    fader.setSliderStyle (juce::Slider::LinearVertical);
    fader.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 16);
    fader.setRange (-60.0, 6.0, 0.1);
    fader.setSkewFactorFromMidPoint (-12.0);
    fader.setDoubleClickReturnValue (true, 0.0);
    fader.setTextValueSuffix (" dB");
    fader.onDragStart = [this] { if (auto* p = fx::paramByName (volume(), { "vol" })) p->parameterChangeGestureBegin(); };
    fader.onDragEnd   = [this] { if (auto* p = fx::paramByName (volume(), { "vol" })) p->parameterChangeGestureEnd(); };
    fader.onValueChange = [this] { if (auto* v = volume()) v->setVolumeDb ((float) fader.getValue()); };
    addAndMakeVisible (fader);

    if (kind != Kind::master)
    {
        muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffeec23e));
        soloButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff6fd16a));
        muteButton.setClickingTogglesState (true);
        soloButton.setClickingTogglesState (true);
        muteButton.onClick = [this] { if (track) track->setMute (muteButton.getToggleState()); };
        soloButton.onClick = [this] { if (track) track->setSolo (soloButton.getToggleState()); };
        addAndMakeVisible (muteButton);
        addAndMakeVisible (soloButton);
    }

    addAndMakeVisible (meter);
    if (kind == Kind::master)      meter.attach (fx::ensureMasterMeter (edit));
    else if (track != nullptr)     meter.attach (track->getLevelMeterPlugin());

    rebuildInserts();
    tick();
}

te::PluginList& MixerStrip::chain()
{
    return track != nullptr ? track->pluginList : edit.getMasterPluginList();
}

te::VolumeAndPanPlugin* MixerStrip::volume()
{
    if (kind == Kind::master)
        return edit.getMasterVolumePlugin().get();
    return track != nullptr ? track->getVolumePlugin() : nullptr;
}

void MixerStrip::toggleChannelPlugin (const juce::String& which)
{
    edit.getUndoManager().beginNewTransaction();
    if (auto* p = fx::findChannelPlugin (chain(), which))
        p->setEnabled (! p->isEnabled());
    else if (auto* np = fx::ensureChannelPlugin (edit, chain(), which))
        np->setEnabled (true);
}

juce::String MixerStrip::makeSignature()
{
    juce::String s;
    for (auto* p : fx::getInserts (chain()))
        s << fx::displayName (*p) << (p->isEnabled() ? "1" : "0") << "|";
    return s;
}

void MixerStrip::rebuildInserts()
{
    insertButtons.clear();
    auto inserts = fx::getInserts (chain());

    for (int i = 0; i < juce::jmin (4, inserts.size()); ++i)
    {
        auto* p = inserts[i];
        auto* b = insertButtons.add (new SlotButton (fx::displayName (*p)));
        b->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe8c84a));
        b->setToggleState (p->isEnabled(), juce::dontSendNotification);
        b->setTooltip ("Click = open, right-click = bypass / remove");

        te::Plugin::Ptr ptr (p);
        b->onClick      = [ptr] { fx::openWindow (*ptr); };
        b->onRightClick = [this, ptr, b] { showInsertMenu (ptr.get(), b); };
        addChildComponent (b);
    }

    addFxButton.setEnabled (inserts.size() < 4);
    insertSignature = makeSignature();
    resized();
}

void MixerStrip::showInsertMenu (te::Plugin* p, juce::Component* target)
{
    if (p == nullptr) return;

    juce::PopupMenu m;
    m.addItem (1, "Open");
    m.addItem (2, p->isEnabled() ? "Bypass" : "Enable");
    m.addSeparator();
    m.addItem (3, "Remove");

    te::Plugin::Ptr ptr (p);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                     [safe = juce::Component::SafePointer<MixerStrip> (this), ptr] (int r)
    {
        if (safe == nullptr || ptr == nullptr) return;
        if (r == 1) fx::openWindow (*ptr);
        if (r == 2) ptr->setEnabled (! ptr->isEnabled());
        if (r == 3)
        {
            safe->edit.getUndoManager().beginNewTransaction();
            ptr->deleteFromParent();
        }
        safe->rebuildInserts();
    });
}

void MixerStrip::setSections (const bool visible[numSections])
{
    for (int s = 0; s < numSections; ++s)
        show[s] = visible[s];
    resized();
    repaint();
}

void MixerStrip::tick()
{
    if (kind == Kind::master)
        nameLabel.setText ("MASTER", juce::dontSendNotification);
    else if (track != nullptr && ! nameLabel.isBeingEdited())
        nameLabel.setText (track->getName(), juce::dontSendNotification);

    if (kind == Kind::music && track != nullptr)
    {
        juce::String inst = isAudioTrack (*track) ? "AUDIO" : "- no instrument -";
        for (auto p : track->pluginList.getPlugins())
            if (dynamic_cast<te::ExternalPlugin*> (&*p) != nullptr && p->isSynth())
                inst = p->getName();
        instrumentButton.setButtonText (inst);
    }

    if (auto* dyn = fx::findChannelPlugin (chain(), "dyn")) dynOn.setToggleState (dyn->isEnabled(), juce::dontSendNotification);
    else                                                     dynOn.setToggleState (false, juce::dontSendNotification);
    if (auto* eq = fx::findChannelPlugin (chain(), "eq"))    eqOn.setToggleState (eq->isEnabled(), juce::dontSendNotification);
    else                                                     eqOn.setToggleState (false, juce::dontSendNotification);

    for (auto* k : dynKnobs)  k->tick();
    for (auto* k : eqKnobs)   k->tick();
    for (auto* k : sendKnobs) k->tick();
    pan.tick();

    if (auto* v = volume())
        if (! fader.isMouseButtonDown())
            fader.setValue (v->getVolumeDb(), juce::dontSendNotification);

    if (track != nullptr && kind != Kind::master)
    {
        muteButton.setToggleState (track->isMuted (false), juce::dontSendNotification);
        soloButton.setToggleState (track->isSolo (false), juce::dontSendNotification);
    }

    if (makeSignature() != insertSignature)
        rebuildInserts();

    meter.tick();
}

void MixerStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f, 0.0f);

    g.setColour (panelGrey);
    g.fillRect (r);
    g.setColour (panelEdge);
    g.drawRect (r, 1.0f);

    // header
    g.setColour (colour);
    g.fillRect (headerArea.toFloat().reduced (2.0f, 2.0f));

    for (int s = 0; s < numSections; ++s)
    {
        if (! show[s]) continue;
        auto area = sectionArea[s];
        auto bar = area.removeFromTop (labelBarH);

        g.setColour (labelBar);
        g.fillRect (bar.reduced (1, 1));
        g.setColour (colour);
        g.fillRect (bar.reduced (1, 1).removeFromLeft (4));

        if (! hasSection (s))
        {
            g.setColour (panelLight.darker (0.06f));
            g.fillRect (area.reduced (2, 1));
            continue;
        }

        g.setColour (juce::Colour (sectionInk[s]));
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        auto text = (kind == Kind::master && s == secDyn) ? juce::String ("MASTER COMP") : juce::String (sectionNames[s]);
        g.drawText (text, bar.withTrimmedLeft (8), juce::Justification::centredLeft, false);

        g.setColour (panelLight);
        g.fillRect (area.reduced (2, 1));
        g.setColour (panelEdge.withAlpha (0.6f));
        g.drawHorizontalLine (area.getBottom() - 1, (float) area.getX() + 2.0f, (float) area.getRight() - 2.0f);
    }

    // bottom panel
    g.setColour (panelLight.darker (0.03f));
    g.fillRect (bottomArea.reduced (2, 1));

    if (kind == Kind::music)
    {
        g.setColour (textDark.withAlpha (0.6f));
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText (juce::String (index + 1), bottomArea.getX() + 4, bottomArea.getBottom() - 16, 20, 14, juce::Justification::left);
    }
}

void MixerStrip::resized()
{
    const int w = getWidth();
    int y = 0;

    headerArea = { 0, y, w, headerHeight };
    nameLabel.setBounds (headerArea.reduced (4, 3));
    y += headerHeight;

    for (int s = 0; s < numSections; ++s)
    {
        sectionArea[s] = show[s] ? juce::Rectangle<int> (0, y, w, sectionHeight (s)) : juce::Rectangle<int>();
        if (show[s]) y += sectionHeight (s);
    }
    bottomArea = { 0, y, w, juce::jmax (0, getHeight() - y) };

    // ---- dynamics
    const bool dynVis = show[secDyn] && hasSection (secDyn);
    dynOn.setVisible (dynVis);
    for (auto* k : dynKnobs) k->setVisible (dynVis);
    if (dynVis)
    {
        auto a = sectionArea[secDyn];
        auto bar = a.removeFromTop (labelBarH);
        dynOn.setBounds (bar.removeFromRight (30).reduced (2, 2));
        a = a.reduced (4, 2);
        const int kw = a.getWidth() / juce::jmax (1, dynKnobs.size());
        for (auto* k : dynKnobs)
            k->setBounds (a.removeFromLeft (kw));
    }

    // ---- EQ
    const bool eqVis = show[secEq] && hasSection (secEq);
    eqOn.setVisible (eqVis);
    for (auto* k : eqKnobs) k->setVisible (eqVis);
    if (eqVis)
    {
        auto a = sectionArea[secEq];
        auto bar = a.removeFromTop (labelBarH);
        eqOn.setBounds (bar.removeFromRight (30).reduced (2, 2));
        a = a.reduced (4, 1);
        for (int row = 0; row < 4; ++row)
        {
            auto line = a.removeFromTop (42);
            if (row * 2 + 1 < eqKnobs.size())
            {
                eqKnobs[row * 2]->setBounds (line.removeFromLeft (line.getWidth() / 2));
                eqKnobs[row * 2 + 1]->setBounds (line);
            }
        }
    }

    // ---- inserts
    const bool insVis = show[secInserts];
    addFxButton.setVisible (insVis);
    for (auto* b : insertButtons) b->setVisible (insVis);
    if (insVis)
    {
        auto a = sectionArea[secInserts].withTrimmedTop (labelBarH).reduced (4, 3);
        for (int i = 0; i < 4; ++i)
        {
            auto slot = a.removeFromTop (19).reduced (0, 1);
            if (i < insertButtons.size()) insertButtons[i]->setBounds (slot);
        }
        addFxButton.setBounds (a.removeFromTop (22).reduced (6, 2));
    }

    // ---- sends
    const bool sendVis = show[secSends] && hasSection (secSends);
    for (auto* k : sendKnobs)  k->setVisible (sendVis);
    for (auto* l : sendLabels) l->setVisible (sendVis);
    if (sendVis)
    {
        auto a = sectionArea[secSends].withTrimmedTop (labelBarH).reduced (2, 1);
        const int cw = a.getWidth() / 2;
        for (int i = 0; i < sendKnobs.size(); ++i)
        {
            juce::Rectangle<int> cell (a.getX() + (i % 2) * cw, a.getY() + (i / 2) * 48, cw, 48);
            sendLabels[i]->setBounds (cell.removeFromTop (11));
            sendKnobs[i]->setBounds (cell);
        }
    }

    // ---- bottom: instrument, pan, fader + meter, mute/solo
    auto b = bottomArea.reduced (4, 4);
    if (kind == Kind::music)
        instrumentButton.setBounds (b.removeFromTop (20));
    b.removeFromTop (2);
    pan.setBounds (b.removeFromTop (44).withSizeKeepingCentre (48, 44));

    if (kind != Kind::master)
    {
        auto ms = b.removeFromBottom (22);
        b.removeFromBottom (14);   // room for the track number
        muteButton.setBounds (ms.removeFromLeft (ms.getWidth() / 2).reduced (1, 0));
        soloButton.setBounds (ms.reduced (1, 0));
    }

    b.removeFromBottom (2);
    const int meterW = kind == Kind::master ? 46 : 36;
    meter.setBounds (b.removeFromRight (meterW).withTrimmedBottom (18));
    fader.setBounds (b);
}

//==============================================================================
// MixerView
//==============================================================================
MixerView::MixerView()
{
    setLookAndFeel (&look);

    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (false, true);
    addAndMakeVisible (viewport);

    const char* names[] = { "DYN", "EQ", "INS", "SEND" };
    for (int s = 0; s < MixerStrip::numSections; ++s)
    {
        auto& b = sectionButtons[s];
        b.setButtonText (names[s]);
        b.setClickingTogglesState (true);
        b.setToggleState (true, juce::dontSendNotification);
        b.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe8c84a));
        b.setTooltip ("Show / hide this section (sections fold away automatically when the mixer is small)");
        b.onClick = [this, s] { userShow[s] = sectionButtons[s].getToggleState(); resized(); };
        addAndMakeVisible (b);
    }
}

MixerView::~MixerView()
{
    strips.clear();
    setLookAndFeel (nullptr);
}

void MixerView::setEdit (te::Edit* e)
{
    strips.clear();
    edit = e;
    rebuild();
}

juce::String MixerView::makeTrackSignature()
{
    juce::String s;
    if (edit != nullptr)
        for (auto* t : te::getAudioTracks (*edit))
            s << t->itemID.toString() << ",";
    return s;
}

void MixerView::rebuild()
{
    strips.clear();
    trackSignature = makeTrackSignature();
    if (edit == nullptr) return;

    int i = 0;
    for (auto* t : getMusicTracks (*edit))
    {
        content.addAndMakeVisible (strips.add (new MixerStrip (*edit, t, MixerStrip::Kind::music, i, trackColour (i))));
        ++i;
    }

    for (auto* t : getReturnTracks (*edit))
        content.addAndMakeVisible (strips.add (new MixerStrip (*edit, t, MixerStrip::Kind::fxReturn, i++, colours::send)));

    content.addAndMakeVisible (strips.add (new MixerStrip (*edit, nullptr, MixerStrip::Kind::master, i++, colours::accent)));

    resized();
}

void MixerView::tick()
{
    if (! isShowing()) return;

    if (makeTrackSignature() != trackSignature)
        rebuild();

    for (auto* s : strips)
        s->tick();
}

void MixerView::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff3a3c41));

    auto side = getLocalBounds().removeFromLeft (sideWidth);
    g.setColour (panelGrey.darker (0.25f));
    g.fillRect (side.reduced (2));
    g.setColour (textDark);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("SHOW", side.removeFromTop (22), juce::Justification::centred);
}

void MixerView::resized()
{
    auto r = getLocalBounds();
    auto side = r.removeFromLeft (sideWidth).reduced (6, 4);
    side.removeFromTop (18);
    for (auto& b : sectionButtons)
    {
        b.setBounds (side.removeFromTop (26).reduced (0, 2));
    }

    viewport.setBounds (r);

    const int minH = MixerStrip::headerHeight + MixerStrip::bottomMinHeight;
    const int avail = juce::jmax (minH, viewport.getHeight() - viewport.getScrollBarThickness());

    // fold sections away (dynamics first, then EQ, inserts, sends) until it fits
    bool vis[MixerStrip::numSections];
    int need = minH;
    for (int s = 0; s < MixerStrip::numSections; ++s)
    {
        vis[s] = userShow[s];
        if (vis[s]) need += MixerStrip::sectionHeight (s);
    }
    for (int s = 0; s < MixerStrip::numSections && need > avail; ++s)
        if (vis[s]) { vis[s] = false; need -= MixerStrip::sectionHeight (s); }

    for (int s = 0; s < MixerStrip::numSections; ++s)
        sectionButtons[s].setAlpha (userShow[s] && ! vis[s] ? 0.45f : 1.0f);

    int x = 2, numMusic = 0;
    for (auto* s : strips)
        if (s->kind == MixerStrip::Kind::music) ++numMusic;

    for (int i = 0; i < strips.size(); ++i)
    {
        auto* s = strips[i];
        if (i == numMusic) x += 10;
        if (s->kind == MixerStrip::Kind::master) x += 10;
        const int w = s->kind == MixerStrip::Kind::master ? masterWidth : stripWidth;
        s->setSections (vis);
        s->setBounds (x, 0, w, avail);
        x += w;
    }

    content.setSize (juce::jmax (x + 2, viewport.getMaximumVisibleWidth()), avail);
}
