#pragma once

#include <JuceHeader.h>
#include "SynthVoice.h"

/**
    Synthesiser with Poly / Mono / Legato voice modes.

    Mono and Legato keep a held-note stack with last-note priority: new notes
    retarget the single sounding voice, and releasing the top note falls back
    to whatever is still held (never retriggering envelopes on the way back).
    In Legato mode, overlapping notes don't retrigger envelopes at all — the
    voice just glides to the new pitch.
*/
class AstralSynth : public juce::Synthesiser
{
public:
    enum VoiceMode { polyMode = 0, monoMode = 1, legatoMode = 2 };

    void setVoiceMode (int newMode)
    {
        if (newMode == voiceMode)
            return;

        const juce::ScopedLock sl (lock);
        voiceMode = newMode;
        heldNotes.clearQuick();
        allNotesOff (0, true);
    }

    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (voiceMode == polyMode)
        {
            juce::Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
            return;
        }

        const juce::ScopedLock sl (lock);

        auto sound = getSound (0);

        if (sound == nullptr)
            return;

        const bool wasHeld = ! heldNotes.isEmpty();
        removeHeld (midiNoteNumber);
        heldNotes.add ({ midiNoteNumber, velocity });

        auto* voice = findPlayingVoice();

        if (voice == nullptr)
        {
            juce::Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
            return;
        }

        if (voiceMode == legatoMode && wasHeld)
            if (auto* sv = dynamic_cast<SynthVoice*> (voice))
                sv->setLegatoPending();

        startVoice (voice, sound.get(), midiChannel, midiNoteNumber, velocity);
    }

    void noteOff (int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override
    {
        if (voiceMode == polyMode)
        {
            juce::Synthesiser::noteOff (midiChannel, midiNoteNumber, velocity, allowTailOff);
            return;
        }

        const juce::ScopedLock sl (lock);

        const bool wasTop = ! heldNotes.isEmpty()
                         && heldNotes.getLast().note == midiNoteNumber;
        removeHeld (midiNoteNumber);

        if (! wasTop)
            return;   // a note lower in the stack was lifted — nothing sounding changes

        if (heldNotes.isEmpty())
        {
            juce::Synthesiser::noteOff (midiChannel, midiNoteNumber, velocity, allowTailOff);
            return;
        }

        // fall back to the most recent still-held note, without retriggering
        const auto previous = heldNotes.getLast();
        auto* voice = findPlayingVoice();
        auto sound = getSound (0);

        if (voice != nullptr && sound != nullptr)
        {
            if (auto* sv = dynamic_cast<SynthVoice*> (voice))
                sv->setLegatoPending();

            startVoice (voice, sound.get(), midiChannel, previous.note, previous.velocity);
        }
    }

private:
    struct HeldNote
    {
        int note = 0;
        float velocity = 0.0f;
    };

    juce::SynthesiserVoice* findPlayingVoice() const
    {
        for (int i = 0; i < getNumVoices(); ++i)
            if (auto* v = getVoice (i); v->getCurrentlyPlayingNote() >= 0)
                return v;

        return nullptr;
    }

    void removeHeld (int note)
    {
        for (int i = heldNotes.size(); --i >= 0;)
            if (heldNotes.getReference (i).note == note)
                heldNotes.remove (i);
    }

    juce::Array<HeldNote> heldNotes;
    int voiceMode = polyMode;
};
