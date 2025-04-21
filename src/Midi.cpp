//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "Midi.hpp"
#include "VM.hpp"
#include "UGen.hpp"
#include "ErrorCodes.hpp"
#include <vector>
#include "PortableMidiPacket.hpp"
#include "PortableMidiClient.hpp"
#ifdef SAPF_COREMIDI
	#include <CoreMidi/CoreMidi.h>
	#include <mach/mach_time.h>
#else
	#include <algorithm>
	#include <string>
	#include <RtMidi.h>
#endif

struct MidiChanState
{
	uint8_t control[128];
	uint8_t polytouch[128];
	uint8_t keyvel[128];
	uint32_t numKeysDown;
	uint16_t bend;
	uint8_t touch;
	uint8_t program;
	uint8_t lastkey;
	uint8_t lastvel;
};

MidiChanState gMidiState[kMaxMidiPorts][16];
bool gMidiDebug = false;
std::vector<uint8_t> gSysexData;
static uint8_t gRunningStatus;
static bool gSysexFlag = false;

static std::unique_ptr<PortableMidiClient> gMidiClient;

static void sysexBegin() {
	gRunningStatus = 0; // clear running status
	//gSysexData.clear();
	gSysexFlag = true;
}

static void sysexEnd(int lastUID) {
	gSysexFlag = false;
}

static void sysexEndInvalid() {
	gSysexFlag = false;
}

static int midiProcessSystemPacket(const PortableMidiPacket& pkt, const int chan) {
	int index, data;
	switch (chan) {
		case 7: // added cp: Sysex EOX must be taken into account if first on data packet
		case 0:
		{
			int last_uid = 0;
			auto m = pkt.length();
			auto p_pkt = pkt.bytes();
			uint8_t pktval;

			while(m--) {
				pktval = *p_pkt++;
				if(pktval & 0x80) { // status byte
					if(pktval == 0xF7) { // end packet
						gSysexData.push_back(pktval); // add EOX
						if(gSysexFlag)
							sysexEnd(last_uid); // if last_uid != 0 rebuild the VM.
						else
							sysexEndInvalid(); // invalid 1 byte with only EOX can happen
						break;
					}
					else if(pktval == 0xF0) { // new packet
						if(gSysexFlag) {// invalid new one/should not happen -- but handle in case
							// store the last uid value previous to invalid data to rebuild VM after sysexEndInvalid call
							// since it may call sysexEnd() just after it !
							sysexEndInvalid();
						}
						sysexBegin(); // new sysex in
						//gSysexData.push_back(pktval); // add SOX
					}
					else {// abnormal data in middle of sysex packet
						//gSysexData.push_back(pktval); // add it as an abort message
						sysexEndInvalid(); // flush invalid
						m = 0; // discard all packet
						break;
					}
				}
				else if(gSysexFlag) {
					//gSysexData.push_back(pktval); // add Byte
				} else { // garbage - handle in case - discard it
					break;
				}
			}
			return (pkt.length()-m);
		}
			break;

		case 1 :
			index = pkt.bytes()[1] >> 4;
			data  = pkt.bytes()[1] & 0xf;
			switch (index) { case 1: case 3: case 5: case 7: { data = data << 4; } }
			return 2;

		case 2 : 	//songptr
			return 3;

		case 3 :	// song select
			return 2;

		case 8 :	//clock
		case 10:	//start
		case 11:	//continue
		case 12: 	//stop
		case 15:	//reset
			gRunningStatus = 0; // clear running status
			return 1;

		default:
			break;
	}

	return (1);
}

static void midiProcessPacket(const PortableMidiPacket& pkt, const int srcIndex)
{
	int i = 0;
	while (i < pkt.length()) {
		uint8_t status = pkt.bytes()[i] & 0xF0;
		uint8_t chan = pkt.bytes()[i] & 0x0F;
		uint8_t a, b;

		if(status & 0x80) // set the running status for voice messages
			gRunningStatus = ((status >> 4) == 0xF) ? 0 : pkt.bytes()[i]; // keep also additional info
	L:
		switch (status) {
		case 0x80 : //noteOff
			a = pkt.bytes()[i+1];
			b = pkt.bytes()[i+2];
			if (gMidiDebug) printf("midi note off %d %d %d %d\n", srcIndex, chan+1, a, b);
			gMidiState[srcIndex][chan].keyvel[a] = 0;
			--gMidiState[srcIndex][chan].numKeysDown;
			i += 3;
			break;
		case 0x90 : //noteOn
			a = pkt.bytes()[i+1];
			b = pkt.bytes()[i+2];
			if (gMidiDebug) printf("midi note on %d %d %d %d\n", srcIndex, chan+1, a, b);
			if (b) {
				gMidiState[srcIndex][chan].lastkey = a;
				gMidiState[srcIndex][chan].lastvel = b;
				++gMidiState[srcIndex][chan].numKeysDown;
			} else {
				--gMidiState[srcIndex][chan].numKeysDown;
			}
			gMidiState[srcIndex][chan].keyvel[a] = b;
			i += 3;
			break;
		case 0xA0 : //polytouch
			a = pkt.bytes()[i+1];
			b = pkt.bytes()[i+2];
			if (gMidiDebug) printf("midi poly %d %d %d %d\n", srcIndex, chan+1, a, b);
			gMidiState[srcIndex][chan].polytouch[a] = b;
			i += 3;
			break;
		case 0xB0 : //control
			a = pkt.bytes()[i+1];
			b = pkt.bytes()[i+2];
			if (gMidiDebug) printf("midi control %d %d %d %d\n", srcIndex, chan+1, a, b);
			gMidiState[srcIndex][chan].control[a] = b;
			if (a == 120 || (a >= 123 && a <= 127)) {
				// all notes off
				memset(gMidiState[srcIndex][chan].keyvel, 0, 128);
				gMidiState[srcIndex][chan].numKeysDown = 0;
			} else if (a == 121) {
				// reset ALL controls to zero, don't follow MMA recommended practices.
				memset(gMidiState[srcIndex][chan].control, 0, 128);
				gMidiState[srcIndex][chan].bend = 0x4000;
			}
			i += 3;
			break;
		case 0xC0 : //program
			a = pkt.bytes()[i+1];
			gMidiState[srcIndex][chan].program = a;
			if (gMidiDebug) printf("midi program %d %d %d\n", srcIndex, chan+1, a);
			i += 2;
			break;
		case 0xD0 : //touch
			a = pkt.bytes()[i+1];
			printf("midi touch %d %d\n", chan+1, a);
			gMidiState[srcIndex][chan].touch = a;
			i += 2;
			break;
		case 0xE0 : //bend
			a = pkt.bytes()[i+1];
			b = pkt.bytes()[i+2];
			if (gMidiDebug) printf("midi bend %d %d %d %d\n", srcIndex, chan+1, a, b);
			gMidiState[srcIndex][chan].bend = ((b << 7) | a) - 8192;
			i += 3;
			break;
		case 0xF0 :
			i += midiProcessSystemPacket(pkt, chan);
			break;
		default :	// data byte => continuing sysex message
			if(gRunningStatus && !gSysexFlag) { // modified cp: handling running status. may be we should here
				status = gRunningStatus & 0xF0; // accept running status only inside a packet beginning
				chan = gRunningStatus & 0x0F;	// with a valid status byte ?
				--i;
				goto L; // parse again with running status set
			}
			chan = 0;
			i += midiProcessSystemPacket(pkt, chan);
			break;
		}
	}
}

// TODO: more of below can be refactored into an abstraction to reduce duplication
//  between macOS / not
#ifdef SAPF_COREMIDI
// TODO: still needed?
static int midiProcessSystemPacket(MIDIPacket *pkt, int chan) {
	return midiProcessSystemPacket(PortableMidiPacket{pkt}, chan)
}

static void midiReadProc(const MIDIPacketList *pktlist, void* readProcRefCon, void* srcConnRefCon)
{
	MIDIPacket *pkt = (MIDIPacket*)pktlist->packet;
	int srcIndex = (int)(size_t) srcConnRefCon;
	for (uint32_t i=0; i<pktlist->numPackets; ++i) {
		midiProcessPacket(PortableMidiPacket{pkt}, srcIndex);
		pkt = MIDIPacketNext(pkt);
	}
}

static void midiNotifyProc(const MIDINotification *message, void *refCon)
{
	printf("midi notification %d %d\n", (int)message->messageID, (int)message->messageSize);
}

static struct mach_timebase_info machTimebaseInfo() {
    struct mach_timebase_info info;
    mach_timebase_info(&info);
    return info;
}

static MIDITimeStamp midiTime(float latencySeconds)
{
    // add the latency expressed in seconds, to the current host time base.
    static struct mach_timebase_info info = machTimebaseInfo(); // cache the timebase info.
    Float64 latencyNanos = 1000000000 * latencySeconds;
    MIDITimeStamp latencyMIDI = (latencyNanos / (Float64)info.numer) * (Float64)info.denom;
    return (MIDITimeStamp)mach_absolute_time() + latencyMIDI;
}

static int prListMIDIEndpoints()
{
	OSStatus error;
	int numSrc = (int)MIDIGetNumberOfSources();
	int numDst = (int)MIDIGetNumberOfDestinations();

	printf("midi sources %d destinations %d\n", (int)numSrc, (int)numDst);

	for (int i=0; i<numSrc; ++i) {
		MIDIEndpointRef src = MIDIGetSource(i);
		SInt32 uid = 0;
		MIDIObjectGetIntegerProperty(src, kMIDIPropertyUniqueID, &uid);

		MIDIEntityRef ent;
		error = MIDIEndpointGetEntity(src, &ent);

		CFStringRef devname, endname;
		char cendname[1024], cdevname[1024];

		// Virtual sources don't have entities
		if(error)
		{
			MIDIObjectGetStringProperty(src, kMIDIPropertyName, &devname);
			MIDIObjectGetStringProperty(src, kMIDIPropertyName, &endname);
			CFStringGetCString(devname, cdevname, 1024, kCFStringEncodingUTF8);
			CFStringGetCString(endname, cendname, 1024, kCFStringEncodingUTF8);
		}
		else
		{
			MIDIDeviceRef dev;

			MIDIEntityGetDevice(ent, &dev);
			MIDIObjectGetStringProperty(dev, kMIDIPropertyName, &devname);
			MIDIObjectGetStringProperty(src, kMIDIPropertyName, &endname);
			CFStringGetCString(devname, cdevname, 1024, kCFStringEncodingUTF8);
			CFStringGetCString(endname, cendname, 1024, kCFStringEncodingUTF8);
		}
		
		printf("MIDI Source %2d '%s', '%s' UID: %d\n", i, cdevname, cendname, uid);
	}



	for (int i=0; i<numDst; ++i) {
		MIDIEndpointRef dst = MIDIGetDestination(i);
		SInt32 uid = 0;
		MIDIObjectGetIntegerProperty(dst, kMIDIPropertyUniqueID, &uid);

		MIDIEntityRef ent;
		error = MIDIEndpointGetEntity(dst, &ent);

		CFStringRef devname, endname;
		char cendname[1024], cdevname[1024];

		// Virtual destinations don't have entities either
		if(error)
		{
			MIDIObjectGetStringProperty(dst, kMIDIPropertyName, &devname);
			MIDIObjectGetStringProperty(dst, kMIDIPropertyName, &endname);
			CFStringGetCString(devname, cdevname, 1024, kCFStringEncodingUTF8);
			CFStringGetCString(endname, cendname, 1024, kCFStringEncodingUTF8);

		}
		else
		{
			MIDIDeviceRef dev;

			MIDIEntityGetDevice(ent, &dev);
			MIDIObjectGetStringProperty(dev, kMIDIPropertyName, &devname);
			MIDIObjectGetStringProperty(dst, kMIDIPropertyName, &endname);
			CFStringGetCString(devname, cdevname, 1024, kCFStringEncodingUTF8);
			CFStringGetCString(endname, cendname, 1024, kCFStringEncodingUTF8);
		}
		printf("MIDI Destination %2d '%s', '%s' UID: %d\n", i, cdevname, cendname, uid);
	}
	return errNone;
}



static int prConnectMIDIIn(int uid, int inputIndex)
{
	if (inputIndex < 0 || inputIndex >= client->mNumMIDIInPorts) return errOutOfRange;

	MIDIEndpointRef src=0;
	MIDIObjectType mtype;
	MIDIObjectFindByUniqueID(uid, (MIDIObjectRef*)&src, &mtype);
	if (mtype != kMIDIObjectType_Source) return errFailed;

	//pass the uid to the midiReadProc to identify the src
	void* p = (void*)(uintptr_t)inputIndex;
	MIDIPortConnectSource(gMIDIInPort[inputIndex], src, p);

	return errNone;
}


static int prDisconnectMIDIIn(int uid, int inputIndex)
{
	if (inputIndex < 0 || inputIndex >= client->mNumMIDIInPorts) return errOutOfRange;

	MIDIEndpointRef src=0;
	MIDIObjectType mtype;
	MIDIObjectFindByUniqueID(uid, (MIDIObjectRef*)&src, &mtype);
	if (mtype != kMIDIObjectType_Source) return errFailed;

	MIDIPortDisconnectSource(gMIDIInPort[inputIndex], src);

	return errNone;
}

static void midiConnectInput_(Thread& th, Prim* prim)
{
	int index = (int)th.popInt("midiConnectInput : port");
	int uid = (int)th.popInt("midiConnectInput : sourceUID");
	prConnectMIDIIn(uid, index);
}

static void midiDisconnectInput_(Thread& th, Prim* prim)
{
	int index = (int)th.popInt("midiDisconnectInput : port");
	int uid = (int)th.popInt("midiDisconnectInput : sourceUID");
	prDisconnectMIDIIn(uid, index);
}

static void midiDebug_(Thread& th, Prim* prim)
{
    gMidiDebug = th.popFloat("midiDebug : onoff") != 0.;
}
#else
// RtMidi-based implementation
// TODO: dedupe this stuff - lots of dupe with coremidi

std::vector<RtMidiIn*> gMIDIInPorts;
std::vector<RtMidiOut*> gMIDIOutPorts;

static int midiProcessSystemPacket(const std::vector<unsigned char>& message, int startIdx, int chan)
{
    if (startIdx >= message.size()) return 0;
	return midiProcessSystemPacket(PortableMidiPacket{message}, chan);
}

static void midiProcessPacket(const std::vector<unsigned char>& message, int srcIndex)
{
    if (message.empty()) return;
    
    int i = 0;
    while (i < message.size()) {
        uint8_t status = message[i] & 0xF0;
        uint8_t chan = message[i] & 0x0F;
        uint8_t a, b;
        
        if(status & 0x80) // set the running status for voice messages
            gRunningStatus = ((status >> 4) == 0xF) ? 0 : message[i]; // keep also additional info
    L:
        switch (status) {
        case 0x80 : //noteOff
            if (i + 2 >= message.size()) return; // Ensure we have enough data
            a = message[i+1];
            b = message[i+2];
            if (gMidiDebug) printf("midi note off %d %d %d %d\n", srcIndex, chan+1, a, b);
            gMidiState[srcIndex][chan].keyvel[a] = 0;
            --gMidiState[srcIndex][chan].numKeysDown;
            i += 3;
            break;
        case 0x90 : //noteOn
            if (i + 2 >= message.size()) return; // Ensure we have enough data
            a = message[i+1];
            b = message[i+2];
            if (gMidiDebug) printf("midi note on %d %d %d %d\n", srcIndex, chan+1, a, b);
            if (b) {
                gMidiState[srcIndex][chan].lastkey = a;
                gMidiState[srcIndex][chan].lastvel = b;
                ++gMidiState[srcIndex][chan].numKeysDown;
            } else {
                --gMidiState[srcIndex][chan].numKeysDown;
            }
            gMidiState[srcIndex][chan].keyvel[a] = b;
            i += 3;
            break;
        case 0xA0 : //polytouch
            if (i + 2 >= message.size()) return; // Ensure we have enough data
            a = message[i+1];
            b = message[i+2];
            if (gMidiDebug) printf("midi poly %d %d %d %d\n", srcIndex, chan+1, a, b);
            gMidiState[srcIndex][chan].polytouch[a] = b;
            i += 3;
            break;
        case 0xB0 : //control
            if (i + 2 >= message.size()) return; // Ensure we have enough data
            a = message[i+1];
            b = message[i+2];
            if (gMidiDebug) printf("midi control %d %d %d %d\n", srcIndex, chan+1, a, b);
            gMidiState[srcIndex][chan].control[a] = b;
            if (a == 120 || (a >= 123 && a <= 127)) {
                // all notes off
                memset(gMidiState[srcIndex][chan].keyvel, 0, 128);
                gMidiState[srcIndex][chan].numKeysDown = 0;
            } else if (a == 121) {
                // reset ALL controls to zero, don't follow MMA recommended practices.
                memset(gMidiState[srcIndex][chan].control, 0, 128);
                gMidiState[srcIndex][chan].bend = 0x4000;
            }
            i += 3;
            break;
        case 0xC0 : //program
            if (i + 1 >= message.size()) return; // Ensure we have enough data
            a = message[i+1];
            gMidiState[srcIndex][chan].program = a;
            if (gMidiDebug) printf("midi program %d %d %d\n", srcIndex, chan+1, a);
            i += 2;
            break;
        case 0xD0 : //touch
            if (i + 1 >= message.size()) return; // Ensure we have enough data
            a = message[i+1];
            printf("midi touch %d %d\n", chan+1, a);
            gMidiState[srcIndex][chan].touch = a;
            i += 2;
            break;
        case 0xE0 : //bend
            if (i + 2 >= message.size()) return; // Ensure we have enough data
            a = message[i+1];
            b = message[i+2];
            if (gMidiDebug) printf("midi bend %d %d %d %d\n", srcIndex, chan+1, a, b);
            gMidiState[srcIndex][chan].bend = ((b << 7) | a) - 8192;
            i += 3;
            break;
        case 0xF0 : // System messages
            i += midiProcessSystemPacket(message, i, chan);
            break;
        default :  // Data byte or unrecognized status
            if(gRunningStatus && !gSysexFlag) {
                status = gRunningStatus & 0xF0;
                chan = gRunningStatus & 0x0F;
                --i;
                goto L; // parse again with running status set
            }
            // If we reach here with a data byte during sysex collection
            if (gSysexFlag && !(message[i] & 0x80)) {
                // Add data byte to sysex collection
                // gSysexData.push_back(message[i]);
                i++;
            } else {
                // Handle data byte outside of a valid context by treating it as system common message
                chan = 0;
                i += midiProcessSystemPacket(message, i, chan);
            }
            break;
        }
    }
}

static void midiCallback(double timeStamp, std::vector<unsigned char>* message, void* userData)
{
    if (!message || message->empty()) return;
    // userData contains the port index
    size_t srcIndex = reinterpret_cast<size_t>(userData);
    midiProcessPacket(PortableMidiPacket{*message}, srcIndex);
}

static int prConnectMIDIIn(int uid, int inputIndex)
{
    if (inputIndex < 0 || inputIndex >= gMidiClient->numMidiInPorts()) return errOutOfRange;
    
    try {
        RtMidiIn* midiIn = gMIDIInPorts[inputIndex];
        if (!midiIn) return errFailed;
        
        // Close any existing connection
        if (midiIn->isPortOpen()) {
            midiIn->closePort();
        }
        
        // Connect to the specified port by UID (which is port number in RtMidi)
        if (uid >= 0 && uid < midiIn->getPortCount()) {
            midiIn->openPort(uid);
            return errNone;
        } else {
            return errOutOfRange;
        }
    } catch (RtMidiError &error) {
        fprintf(stderr, "Error connecting MIDI input: %s\n", error.getMessage().c_str());
        return errFailed;
    }
}

static int prDisconnectMIDIIn(int uid, int inputIndex)
{
    if (inputIndex < 0 || inputIndex >= gMidiClient->numMidiInPorts()) return errOutOfRange;
    
    try {
        RtMidiIn* midiIn = gMIDIInPorts[inputIndex];
        if (!midiIn) return errFailed;
        
        // Close the port
        if (midiIn->isPortOpen()) {
            midiIn->closePort();
        }
        return errNone;
    } catch (RtMidiError &error) {
        fprintf(stderr, "Error disconnecting MIDI input: %s\n", error.getMessage().c_str());
        return errFailed;
    }
}

static void midiConnectInput_(Thread& th, Prim* prim)
{
    int index = (int)th.popInt("midiConnectInput : port");
    int uid = (int)th.popInt("midiConnectInput : sourceUID");
    prConnectMIDIIn(uid, index);
}

static void midiDisconnectInput_(Thread& th, Prim* prim)
{
    int index = (int)th.popInt("midiDisconnectInput : port");
    int uid = (int)th.popInt("midiDisconnectInput : sourceUID");
    prDisconnectMIDIIn(uid, index);
}

static void midiDebug_(Thread& th, Prim* prim)
{
    gMidiDebug = th.popFloat("midiDebug : onoff") != 0.;
}

#endif // SAPF_COREMIDI


static int midiInit(int numIn, int numOut)
{
	memset(gMidiState, 0, sizeof(gMidiState));
	
	numIn = std::clamp(numIn, 1, kMaxMidiPorts);
	numOut = std::clamp(numOut, 1, kMaxMidiPorts);

	#ifdef SAPF_COREMIDI
		client = std::make_unique<PortableMidiClient>(numIn, numOut, midiReadProc, midiNotifyProc);
	#else
		gMidiClient = std::make_unique<PortableMidiClient>(numIn, numOut, &midiCallback);
	#endif

	PortableMidiClient::prListMIDIEndpoints();
	
	return errNone;
}

static int midiCleanUp()
{
	gMidiClient = nullptr;
	return errNone;
}

static void midiStart_(Thread& th, Prim* prim)
{
	midiInit(16, 19);
}

static void midiStop_(Thread& th, Prim* prim)
{
	midiCleanUp();
}

static void midiList_(Thread& th, Prim* prim)
{
	PortableMidiClient::prListMIDIEndpoints();
}


#ifdef SAPF_COREMIDI
static void midiRestart_(Thread& th, Prim* prim)
{
	MIDIRestart();
}
#else
// TODO: not quite sure what this should do on RtMidi
static void midiRestart_(Thread& th, Prim* prim)
{
	midiStart_(th, prim);
}
#endif

const Z kOneOver127 = 1./127.;
const Z kOneOver8191 = 1./8191.;

static void mctl1_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("mctl1 : hi");
	Z lo   = th.popFloat("mctl1 : lo");

	int cnum = th.popInt("mctl1 : ctlNum") & 127;
	int chan = (th.popInt("mctl1 : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl1 : srcIndex") & 15;
	Z z = kOneOver127 * gMidiState[srcIndex][chan].control[cnum];
	th.push(lo + z * (hi - lo));
}

static void xmctl1_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("xmctl1 : hi");
	Z lo   = th.popFloat("xmctl1 : lo");

	int cnum = th.popInt("xmctl1 : ctlNum") & 127;
	int chan = (th.popInt("xmctl1 : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl1 : srcIndex") & 15;
	Z z = kOneOver127 * gMidiState[srcIndex][chan].control[cnum];
	th.push(lo * pow(hi / lo, z));
}

static void mpoly1_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("mpoly1 : hi");
	Z lo   = th.popFloat("mpoly1 : lo");

	int key = th.popInt("mpoly1 : key") & 127;
	int chan = (th.popInt("mpoly1 : chan") - 1) & 15;
	int srcIndex = th.popInt("mpoly1 : srcIndex") & 15;

	Z z = kOneOver127 * gMidiState[srcIndex][chan].polytouch[key];
	th.push(lo + z * (hi - lo));
}

static void xmpoly1_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("xmpoly1 : hi");
	Z lo   = th.popFloat("xmpoly1 : lo");

	int key = th.popInt("xmpoly1 : key") & 127;
	int chan = (th.popInt("xmpoly1 : chan") - 1) & 15;
	int srcIndex = th.popInt("xmpoly1 : srcIndex") & 15;

	Z z = kOneOver127 * gMidiState[srcIndex][chan].polytouch[key];
	th.push(lo * pow(hi / lo, z));
}

static void mgate1_(Thread& th, Prim* prim)
{
	int key = th.popInt("mgate1 : key") & 127;
	int chan = (th.popInt("mgate1 : chan") - 1) & 15;
	int srcIndex = th.popInt("mgate1 : srcIndex") & 15;

	th.pushBool(gMidiState[srcIndex][chan].keyvel[key] > 0);
}

static void mtouch1_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("mtouch1 : hi");
	Z lo   = th.popFloat("mtouch1 : lo");

	int chan = (th.popInt("mtouch1 : chan") - 1) & 15;
	int srcIndex = th.popInt("mtouch1 : srcIndex") & 15;

	Z z = kOneOver127 * gMidiState[srcIndex][chan].touch;
	th.push(lo + z * (hi - lo));
}

static void xmtouch1_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("xmtouch1 : hi");
	Z lo   = th.popFloat("xmtouch1 : lo");

	int chan = (th.popInt("xmtouch1 : chan") - 1) & 15;
	int srcIndex = th.popInt("xmtouch1 : srcIndex") & 15;

	Z z = kOneOver127 * gMidiState[srcIndex][chan].touch;
	th.push(lo * pow(hi / lo, z));
}

static void mprog1_(Thread& th, Prim* prim)
{
	int chan = (th.popInt("mprog1 : chan") - 1) & 15;
	int srcIndex = th.popInt("mprog1 : srcIndex") & 15;

	th.push(gMidiState[srcIndex][chan].touch);
}

static void mlastkey1_(Thread& th, Prim* prim)
{
	int chan = (th.popInt("mlastkey1 : chan") - 1) & 15;
	int srcIndex = th.popInt("mlastkey1 : srcIndex") & 15;

	th.push(gMidiState[srcIndex][chan].lastkey);
}

static void mlastvel1_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("mlastvel1 : hi");
	Z lo   = th.popFloat("mlastvel1 : lo");
 
	int chan = (th.popInt("mlastvel1 : chan") - 1) & 15;
	int srcIndex = th.popInt("mlastvel1 : srcIndex") & 15;

	Z z = kOneOver127 * gMidiState[srcIndex][chan].lastvel;
	th.push(lo + z * (hi - lo));
}

static void xmlastvel1_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("xmlastvel1 : hi");
	Z lo   = th.popFloat("xmlastvel1 : lo");
 
	int chan = (th.popInt("xmlastvel1 : chan") - 1) & 15;
	int srcIndex = th.popInt("xmlastvel1 : srcIndex") & 15;

	Z z = kOneOver127 * gMidiState[srcIndex][chan].lastvel;
	th.push(lo * pow(hi / lo, z));	
}


static void mbend1_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("mbend1 : hi");
	Z lo   = th.popFloat("mbend1 : lo");

	int chan = (th.popInt("mbend1 : chan") - 1) & 15;
	int srcIndex = th.popInt("mbend1 : srcIndex") & 15;

	Z z = kOneOver8191 * gMidiState[srcIndex][chan].bend;
	th.push(lo + z * (hi - lo));
}
static void xmbend1_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("xmbend1 : hi");
	Z lo   = th.popFloat("xmbend1 : lo");

	int chan = (th.popInt("mbend1 : chan") - 1) & 15;
	int srcIndex = th.popInt("xmbend1 : srcIndex") & 15;

	Z z = kOneOver8191 * gMidiState[srcIndex][chan].bend;
	th.push(lo * pow(hi / lo, z));	
}


Z gMidiLagTime = .1;
Z gMidiLagMul = log001 / gMidiLagTime;

struct MCtl : public TwoInputUGen<MCtl>
{
	Z _b1;
	Z _y1;
	Z _lagmul;
    int _srcIndex;
    int _chan;
    int _cnum;
	
	MCtl(Thread& th, int srcIndex, int chan, int cnum, Arg lo, Arg hi)
        : TwoInputUGen<MCtl>(th, lo, hi), _b1(1. + gMidiLagMul * th.rate.invSampleRate),
        _srcIndex(srcIndex), _chan(chan), _cnum(cnum)
	{
	}
	
	virtual const char* TypeName() const override { return "MCtl"; }
	
	void calc(int n, Z* out, Z* lo, Z* hi, int loStride, int hiStride) 
	{
		Z y1 = _y1;
		Z b1 = _b1;
        uint8_t& ctl = gMidiState[_srcIndex][_chan].control[_cnum];
		for (int i = 0; i < n; ++i) {
            Z z = kOneOver127 * ctl;
			Z y0 = *lo + z * (*hi - *lo);
			out[i] = y1 = y0 + b1 * (y1 - y0);
			lo += loStride;
			hi += hiStride;
		}
		_y1 = y1;
	}
};


struct XMCtl : public TwoInputUGen<XMCtl>
{
	Z _b1;
	Z _y1;
	Z _lagmul;
    int _srcIndex;
    int _chan;
    int _cnum;
	
	XMCtl(Thread& th, int srcIndex, int chan, int cnum, Arg lo, Arg hi)
        : TwoInputUGen<XMCtl>(th, lo, hi), _b1(1. + gMidiLagMul * th.rate.invSampleRate),
        _srcIndex(srcIndex), _chan(chan), _cnum(cnum)
	{
	}
	
	virtual const char* TypeName() const override { return "XMCtl"; }
	
	void calc(int n, Z* out, Z* lo, Z* hi, int loStride, int hiStride) 
	{
		Z y1 = _y1;
		Z b1 = _b1;
        uint8_t& ctl = gMidiState[_srcIndex][_chan].control[_cnum];
		for (int i = 0; i < n; ++i) {
            Z z = kOneOver127 * ctl;
			Z y0 = *lo * pow(*hi / *lo, z);
			out[i] = y1 = y0 + b1 * (y1 - y0);
			lo += loStride;
			hi += hiStride;
		}
		_y1 = y1;
	}
};


struct MPoly : public TwoInputUGen<MPoly>
{
	Z _b1;
	Z _y1;
	Z _lagmul;
    int _srcIndex;
    int _chan;
    int _cnum;
	
	MPoly(Thread& th, int srcIndex, int chan, int cnum, Arg lo, Arg hi)
        : TwoInputUGen<MPoly>(th, lo, hi), _b1(1. + gMidiLagMul * th.rate.invSampleRate),
        _srcIndex(srcIndex), _chan(chan), _cnum(cnum)
	{
	}
	
	virtual const char* TypeName() const override { return "MPoly"; }
	
	void calc(int n, Z* out, Z* lo, Z* hi, int loStride, int hiStride) 
	{
		Z y1 = _y1;
		Z b1 = _b1;
        uint8_t& ctl = gMidiState[_srcIndex][_chan].polytouch[_cnum];
		for (int i = 0; i < n; ++i) {
            Z z = kOneOver127 * ctl;
			Z y0 = *lo + z * (*hi - *lo);
			out[i] = y1 = y0 + b1 * (y1 - y0);
			lo += loStride;
			hi += hiStride;
		}
		_y1 = y1;
	}
};


struct XMPoly : public TwoInputUGen<XMPoly>
{
	Z _b1;
	Z _y1;
	Z _lagmul;
    int _srcIndex;
    int _chan;
    int _cnum;
	
	XMPoly(Thread& th, int srcIndex, int chan, int cnum, Arg lo, Arg hi)
        : TwoInputUGen<XMPoly>(th, lo, hi), _b1(1. + gMidiLagMul * th.rate.invSampleRate),
        _srcIndex(srcIndex), _chan(chan), _cnum(cnum)
	{
	}
	
	virtual const char* TypeName() const override { return "XMPoly"; }
	
	void calc(int n, Z* out, Z* lo, Z* hi, int loStride, int hiStride) 
	{
		Z y1 = _y1;
		Z b1 = _b1;
        uint8_t& ctl = gMidiState[_srcIndex][_chan].polytouch[_cnum];
		for (int i = 0; i < n; ++i) {
            Z z = kOneOver127 * ctl;
			Z y0 = *lo * pow(*hi / *lo, z);
			out[i] = y1 = y0 + b1 * (y1 - y0);
			lo += loStride;
			hi += hiStride;
		}
		_y1 = y1;
	}
};


struct MTouch : public TwoInputUGen<MTouch>
{
	Z _b1;
	Z _y1;
	Z _lagmul;
    int _srcIndex;
    int _chan;

	MTouch(Thread& th, int srcIndex, int chan, Arg lo, Arg hi)
        : TwoInputUGen<MTouch>(th, lo, hi), _b1(1. + gMidiLagMul * th.rate.invSampleRate),
        _srcIndex(srcIndex), _chan(chan)
	{
	}
	
	virtual const char* TypeName() const override { return "MTouch"; }
	
	void calc(int n, Z* out, Z* lo, Z* hi, int loStride, int hiStride) 
	{
		Z y1 = _y1;
		Z b1 = _b1;
        uint8_t& ctl = gMidiState[_srcIndex][_chan].touch;
		for (int i = 0; i < n; ++i) {
            Z z = kOneOver127 * ctl;
			Z y0 = *lo + z * (*hi - *lo);
			out[i] = y1 = y0 + b1 * (y1 - y0);
			lo += loStride;
			hi += hiStride;
		}
		_y1 = y1;
	}
};


struct XMTouch : public TwoInputUGen<XMTouch>
{
	Z _b1;
	Z _y1;
	Z _lagmul;
    int _srcIndex;
    int _chan;

	XMTouch(Thread& th, int srcIndex, int chan, Arg lo, Arg hi)
        : TwoInputUGen<XMTouch>(th, lo, hi), _b1(1. + gMidiLagMul * th.rate.invSampleRate),
        _srcIndex(srcIndex), _chan(chan)
	{
	}
	
	virtual const char* TypeName() const override { return "XMTouch"; }
	
	void calc(int n, Z* out, Z* lo, Z* hi, int loStride, int hiStride) 
	{
		Z y1 = _y1;
		Z b1 = _b1;
        uint8_t& ctl = gMidiState[_srcIndex][_chan].touch;
		for (int i = 0; i < n; ++i) {
            Z z = kOneOver127 * ctl;
			Z y0 = *lo * pow(*hi / *lo, z);
			out[i] = y1 = y0 + b1 * (y1 - y0);
			lo += loStride;
			hi += hiStride;
		}
		_y1 = y1;
	}
};


struct MBend : public TwoInputUGen<MBend>
{
	Z _b1;
	Z _y1;
	Z _lagmul;
    int _srcIndex;
    int _chan;

	MBend(Thread& th, int srcIndex, int chan, Arg lo, Arg hi)
        : TwoInputUGen<MBend>(th, lo, hi), _b1(1. + gMidiLagMul * th.rate.invSampleRate),
        _srcIndex(srcIndex), _chan(chan)
	{
	}
	
	virtual const char* TypeName() const override { return "MBend"; }
	
	void calc(int n, Z* out, Z* lo, Z* hi, int loStride, int hiStride) 
	{
		Z y1 = _y1;
		Z b1 = _b1;
        uint16_t& ctl = gMidiState[_srcIndex][_chan].bend;
		for (int i = 0; i < n; ++i) {
            Z z = kOneOver8191 * ctl;
			Z y0 = *lo + z * (*hi - *lo);
			out[i] = y1 = y0 + b1 * (y1 - y0);
			lo += loStride;
			hi += hiStride;
		}
		_y1 = y1;
	}
};


struct XMBend : public TwoInputUGen<XMBend>
{
	Z _b1;
	Z _y1;
	Z _lagmul;
    int _srcIndex;
    int _chan;

	XMBend(Thread& th, int srcIndex, int chan, Arg lo, Arg hi)
        : TwoInputUGen<XMBend>(th, lo, hi), _b1(1. + gMidiLagMul * th.rate.invSampleRate),
        _srcIndex(srcIndex), _chan(chan)
	{
	}
	
	virtual const char* TypeName() const override { return "XMBend"; }
	
	void calc(int n, Z* out, Z* lo, Z* hi, int loStride, int hiStride) 
	{
		Z y1 = _y1;
		Z b1 = _b1;
        uint16_t& ctl = gMidiState[_srcIndex][_chan].bend;
		for (int i = 0; i < n; ++i) {
            Z z = kOneOver8191 * ctl;
			Z y0 = *lo * pow(*hi / *lo, z);
			out[i] = y1 = y0 + b1 * (y1 - y0);
			lo += loStride;
			hi += hiStride;
		}
		_y1 = y1;
	}
};


struct MLastVel : public TwoInputUGen<MLastVel>
{
	Z _b1;
	Z _y1;
	Z _lagmul;
    int _srcIndex;
    int _chan;

	MLastVel(Thread& th, int srcIndex, int chan, Arg lo, Arg hi)
        : TwoInputUGen<MLastVel>(th, lo, hi), _b1(1. + gMidiLagMul * th.rate.invSampleRate),
        _srcIndex(srcIndex), _chan(chan)
	{
	}
	
	virtual const char* TypeName() const override { return "MLastVel"; }
	
	void calc(int n, Z* out, Z* lo, Z* hi, int loStride, int hiStride) 
	{
		Z y1 = _y1;
		Z b1 = _b1;
        uint8_t& ctl = gMidiState[_srcIndex][_chan].lastvel;
		for (int i = 0; i < n; ++i) {
            Z z = kOneOver127 * ctl;
			Z y0 = *lo + z * (*hi - *lo);
			out[i] = y1 = y0 + b1 * (y1 - y0);
			lo += loStride;
			hi += hiStride;
		}
		_y1 = y1;
	}
};

struct XMLastVel : public TwoInputUGen<XMLastVel>
{
	Z _b1;
	Z _y1;
	Z _lagmul;
    int _srcIndex;
    int _chan;

	XMLastVel(Thread& th, int srcIndex, int chan, Arg lo, Arg hi)
        : TwoInputUGen<XMLastVel>(th, lo, hi), _b1(1. + gMidiLagMul * th.rate.invSampleRate),
        _srcIndex(srcIndex), _chan(chan)
	{
	}
	
	virtual const char* TypeName() const override { return "XMLastVel"; }
	
	void calc(int n, Z* out, Z* lo, Z* hi, int loStride, int hiStride) 
	{
		Z y1 = _y1;
		Z b1 = _b1;
        uint8_t& ctl = gMidiState[_srcIndex][_chan].lastvel;
		for (int i = 0; i < n; ++i) {
            Z z = kOneOver127 * ctl;
			Z y0 = *lo * pow(*hi / *lo, z);
			out[i] = y1 = y0 + b1 * (y1 - y0);
			lo += loStride;
			hi += hiStride;
		}
		_y1 = y1;
	}
};


struct MLastKey : public ZeroInputUGen<MLastKey>
{
    int _srcIndex;
    int _chan;

	MLastKey(Thread& th, int srcIndex, int chan)
        : ZeroInputUGen<MLastKey>(th, false),
        _srcIndex(srcIndex), _chan(chan)
	{
	}
	
	virtual const char* TypeName() const override { return "MLastKey"; }
	
	void calc(int n, Z* out) 
	{
        uint8_t& ctl = gMidiState[_srcIndex][_chan].lastkey;
		for (int i = 0; i < n; ++i) {
			out[i] = ctl;
		}
	}
};

struct MProg : public ZeroInputUGen<MProg>
{
    int _srcIndex;
    int _chan;

	MProg(Thread& th, int srcIndex, int chan)
        : ZeroInputUGen<MProg>(th, false),
        _srcIndex(srcIndex), _chan(chan)
	{
	}
	
	virtual const char* TypeName() const override { return "MProg"; }
	
	void calc(int n, Z* out) 
	{
        uint8_t& ctl = gMidiState[_srcIndex][_chan].program;
		for (int i = 0; i < n; ++i) {
			out[i] = ctl;
		}
	}
};

struct MGate : public ZeroInputUGen<MGate>
{
    int _srcIndex;
    int _chan;
    int _key;
	
	MGate(Thread& th, int srcIndex, int chan, int key)
        : ZeroInputUGen<MGate>(th, false),
        _srcIndex(srcIndex), _chan(chan), _key(key)
	{
	}
	
	virtual const char* TypeName() const override { return "MGate"; }
	
	void calc(int n, Z* out) 
	{
        uint8_t& ctl = gMidiState[_srcIndex][_chan].keyvel[_key];
		for (int i = 0; i < n; ++i) {
			out[i] = ctl > 0 ? 1. : 0.;
		}
	}
};

struct ZCtl : public ZeroInputUGen<ZCtl>
{
	Z _b1;
	Z _y1;
	Z _lagmul;
    P<ZRef> zref;
	
	ZCtl(Thread& th, P<ZRef> const& inZRef)
        : ZeroInputUGen<ZCtl>(th, false), _b1(1. + gMidiLagMul * th.rate.invSampleRate),
        zref(inZRef)
	{
	}
	
	virtual const char* TypeName() const override { return "ZCtl"; }
	
	void calc(int n, Z* out) 
	{
		Z y1 = _y1;
		Z b1 = _b1;
        Z& ctl = zref->z;
		for (int i = 0; i < n; ++i) {
			Z y0 = ctl;
			out[i] = y1 = y0 + b1 * (y1 - y0);
		}
		_y1 = y1;
	}
};

static void zctl_(Thread& th, Prim* prim)
{
	P<ZRef> zref = th.popZRef("mctl : zref");

	th.push(new List(new ZCtl(th, zref)));
}


static void mctl_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("mctl : hi");
	Z lo   = th.popFloat("mctl : lo");

	int cnum = th.popInt("mctl : ctlNum") & 127;
	int chan = (th.popInt("mctl : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new MCtl(th, srcIndex, chan, cnum, lo, hi)));
}

static void xmctl_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("xmctl : hi");
	Z lo   = th.popFloat("xmctl : lo");

	int cnum = th.popInt("xmctl : ctlNum") & 127;
	int chan = (th.popInt("xmctl : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new XMCtl(th, srcIndex, chan, cnum, lo, hi)));
}

static void mpoly_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("mpoly : hi");
	Z lo   = th.popFloat("mpoly : lo");

	int key = th.popInt("mpoly : key") & 127;
	int chan = (th.popInt("mpoly : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new MPoly(th, srcIndex, chan, key, lo, hi)));
}

static void xmpoly_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("xmpoly : hi");
	Z lo   = th.popFloat("xmpoly : lo");

	int key = th.popInt("xmpoly : key") & 127;
	int chan = (th.popInt("xmpoly : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new XMPoly(th, srcIndex, chan, key, lo, hi)));
}

static void mtouch_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("mtouch : hi");
	Z lo   = th.popFloat("mtouch : lo");

	int chan = (th.popInt("mtouch : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new MTouch(th, srcIndex, chan, lo, hi)));
}

static void xmtouch_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("xmtouch : hi");
	Z lo   = th.popFloat("xmtouch : lo");

	int chan = (th.popInt("xmtouch : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new XMTouch(th, srcIndex, chan, lo, hi)));
}

static void mbend_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("mbend : hi");
	Z lo   = th.popFloat("mbend : lo");

	int chan = (th.popInt("mbend : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new MBend(th, srcIndex, chan, lo, hi)));
}

static void xmbend_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("xmbend : hi");
	Z lo   = th.popFloat("xmbend : lo");

	int chan = (th.popInt("xmbend : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new XMBend(th, srcIndex, chan, lo, hi)));
}


static void mprog_(Thread& th, Prim* prim)
{
	int chan = (th.popInt("mprog : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new MProg(th, srcIndex, chan)));
}

static void mgate_(Thread& th, Prim* prim)
{
	int key = th.popInt("mgate : key") & 127;
	int chan = (th.popInt("mgate : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new MGate(th, srcIndex, chan, key)));
}


static void mlastvel_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("mlastvel : hi");
	Z lo   = th.popFloat("mlastvel : lo");

	int chan = (th.popInt("mlastvel : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new MLastVel(th, srcIndex, chan, lo, hi)));
}

static void mlastkey_(Thread& th, Prim* prim)
{
	int chan = (th.popInt("mlastkey : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new MLastKey(th, srcIndex, chan)));
}

static void xmlastvel_(Thread& th, Prim* prim)
{
	Z hi   = th.popFloat("xmlastvel : hi");
	Z lo   = th.popFloat("xmlastvel : lo");

	int chan = (th.popInt("xmlastvel : chan") - 1) & 15;
	int srcIndex = th.popInt("mctl : srcIndex") & 15;
	th.push(new List(new XMLastVel(th, srcIndex, chan, lo, hi)));
}

#define DEF(NAME, TAKES, LEAVES, HELP) 	vm.def(#NAME, TAKES, LEAVES, NAME##_, HELP);
#define DEFMCX(NAME, N, HELP) 	vm.defmcx(#NAME, N, NAME##_, HELP);
#define DEFAM(NAME, MASK, HELP) 	vm.defautomap(#NAME, #MASK, NAME##_, HELP);

void AddMidiOps()
{
	vm.addBifHelp("\n*** MIDI control ***");
	DEF(midiStart, 0, 0, "(-->) start up MIDI services");
	DEF(midiRestart, 0, 0, "(-->) rescan MIDI services");
	DEF(midiStop, 0, 0, "(-->) stop MIDI services");
	DEF(midiList, 0, 0, "(-->) list MIDI endpoints");
	DEF(midiConnectInput, 2, 0, "(sourceUID index -->) connect a MIDI source");
	DEF(midiDisconnectInput, 2, 0, "(sourceUID index -->) disconnect a MIDI source");
	DEF(midiDebug, 1, 0, "(onoff -->) turn on or off midi input monitoring");
	
	vm.addBifHelp("\n*** MIDI instantaneous value ***");
	DEFMCX(mctl1, 5, "(srcIndex chan ctlnum lo hi --> out) value of midi controller mapped to the linear range [lo,hi].");
	DEFMCX(mpoly1, 5, "(srcIndex chan key lo hi --> out) value of midi poly key pressure mapped to the linear range [lo,hi].");
	DEFMCX(mtouch1, 4, "(srcIndex chan lo hi --> out) value of midi channel pressure mapped to the linear range [lo,hi].");
	DEFMCX(mbend1, 4, "(srcIndex chan lo hi --> out) value of midi pitch bend mapped to the linear range [lo,hi].");
	DEFMCX(mprog1, 2, "(srcIndex chan --> out) value of midi channel program 0-127.");
	DEFMCX(mgate1, 3, "(srcIndex chan key --> out) value of midi key state. 1 if key is down, 0 if key is up.");
	DEFMCX(mlastkey1, 2, "(srcIndex chan --> out) value of key of most recent midi note on.");
	DEFMCX(mlastvel1, 4, "(srcIndex chan lo hi --> out) value of velocity of most recent midi note on mapped to the linear range [lo,hi].");

	DEFMCX(xmctl1, 5, "(srcIndex chan ctlnum lo hi --> out) value of midi controller mapped to the exponential range [lo,hi].");
	DEFMCX(xmpoly1, 5, "(srcIndex chan key lo hi --> out) value of midi poly key pressure mapped to the exponential range [lo,hi].");
	DEFMCX(xmtouch1, 4, "(srcIndex chan lo hi --> out) value of midi channel pressure mapped to the exponential range [lo,hi].");
	DEFMCX(xmbend1, 4, "(srcIndex chan lo hi --> out) value of midi pitch bend mapped to the exponential range [lo,hi].");
	DEFMCX(xmlastvel1, 4, "(srcIndex chan lo hi --> out) value of velocity of most recent midi note on mapped to the exponential range [lo,hi].");

	vm.addBifHelp("\n*** MIDI control signal ***");
	DEFMCX(mctl, 5, "(srcIndex chan ctlnum lo hi --> out) signal of midi controller mapped to the linear range [lo,hi].");
	DEFMCX(mpoly, 5, "(srcIndex chan key lo hi --> out) signal of midi poly key pressure mapped to the linear range [lo,hi].");
	DEFMCX(mtouch, 4, "(srcIndex chan lo hi --> out) signal of midi channel pressure mapped to the linear range [lo,hi].");
	DEFMCX(mbend, 4, "(srcIndex chan lo hi --> out) signal of midi pitch bend mapped to the linear range [lo,hi].");
	DEFMCX(mlastkey, 2, "(srcIndex chan --> out) signal of key of most recent midi note on.");
	DEFMCX(mlastvel, 4, "(srcIndex chan lo hi --> out) signal of velocity of most recent midi note on mapped to the linear range [lo,hi].");

	DEFMCX(mprog, 2, "(srcIndex chan --> out) signal of midi channel program 0-127.");
	DEFMCX(mgate, 3, "(srcIndex chan key --> out) signal of midi key state. 1 if key is down, 0 if key is up.");

	DEFMCX(xmctl, 5, "(srcIndex chan ctlnum lo hi --> out) signal of midi controller mapped to the exponential range [lo,hi].");
	DEFMCX(xmpoly, 5, "(srcIndex chan key lo hi --> out) signal of midi poly key pressure mapped to the exponential range [lo,hi].");
	DEFMCX(xmtouch, 4, "(srcIndex chan lo hi --> out) signal of midi channel pressure mapped to the exponential range [lo,hi].");
	DEFMCX(xmbend, 4, "(srcIndex chan lo hi --> out) signal of midi pitch bend mapped to the exponential range [lo,hi].");
	DEFMCX(xmlastvel, 4, "(srcIndex chan lo hi --> out) signal of velocity of most recent midi note on mapped to the exponential range [lo,hi].");

	vm.addBifHelp("\n*** ZRef control signal ***");
	DEFMCX(zctl, 1, "(zref --> out) makes a smoothed control signal from a zref.");
}

