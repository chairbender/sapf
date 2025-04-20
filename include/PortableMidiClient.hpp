#pragma once

#ifdef SAPF_COREMIDI
    #include <CoreMidi/CoreMidi.h>
#else
    #include <vector>
    #include "RtMidi.h"
#endif
#include <memory>

const int kMaxMidiPorts = 16;

// represents some kind of
// stateful midi client with associated input and output ports that
// can be connected
class PortableMidiClient {
    // TODO: implement cross platform client
public:
#ifdef SAPF_COREMIDI
    PortableMidiClient(int numIn, int numOut, MIDIReadProc midiReadProc, MIDINotifyProc midiNotifyProc);
#else
    PortableMidiClient(int numIn, int numOut, RtMidiIn::RtMidiCallback midiCallback);
#endif
    // TODO: teardown
    ~PortableMidiClient();

    [[nodiscard]] int numMidiInPorts() const;
    [[nodiscard]] int numMidiOutPorts() const;

    // print the list of midi endpoints to stdout
    static void prListMIDIEndpoints();
private:
    #ifdef SAPF_COREMIDI
        int mNumMidiInPorts;
        int mNumMidiOutPorts;
        MIDIClientRef mMIDIClient;
        MIDIPortRef mMIDIInPgort[kMaxMidiPorts], mMIDIOutPort[kMaxMidiPorts];
    #else
        std::vector<std::unique_ptr<RtMidiIn>> mMIDIInPorts;
        std::vector<std::unique_ptr<RtMidiOut>> mMIDIOutPorts;
    #endif

};