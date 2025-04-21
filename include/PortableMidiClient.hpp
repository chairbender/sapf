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

    // connect the specified MIDI input endpoint to this client's input port
    void connectInputPort(int uid, int inputIndex) const;
    void disconnectInputPort(int uid, int inputIndex);

    // print the list of midi endpoints to stdout
    static void printMIDIEndpoints();
private:
    // TODO: are the outputs even ever actually used? Let's remove them if not
    // TODO: Can these actually be declared const?
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