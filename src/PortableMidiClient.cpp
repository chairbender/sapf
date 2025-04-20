#include "PortableMidiClient.hpp"

#ifdef SAPF_COREMIDI
PortableMidiClient::PortableMidiClient(const int numIn, const int numOut, const MIDIReadProc midiReadProc,
    const MIDINotifyProc midiNotifyProc) {
    
    int enc = kCFStringEncodingMacRoman;
    CFAllocatorRef alloc = CFAllocatorGetDefault();

    {
        CFStringRef clientName = CFStringCreateWithCString(alloc, "SAPF", enc);
        CFReleaser clientNameReleaser(clientName);

        __block OSStatus err2 = noErr;
        dispatch_sync(dispatch_get_main_queue(), ^{
            err2 = MIDIClientCreate(clientName, midiNotifyProc, nil, &mMIDIClient);
        });
        printf("mMIDIClient %d\n", (int)mMIDIClient);
        if (err2) {
            fprintf(stderr, "Could not create MIDI client. error %d\n", err);
            return;
        }
    }

    for (int i=0; i<numIn; ++i) {
        char str[32];
        snprintf(str, 32, "in%d\n", i);
        CFStringRef inputPortName = CFStringCreateWithCString(alloc, str, enc);
        CFReleaser inputPortNameReleaser(inputPortName);

        err = MIDIInputPortCreate(mMIDIClient, inputPortName, midiReadProc, &i, mMIDIInPort+i);
        if (err) {
            mNumMIDIInPorts = i;
            fprintf(stderr, "Could not create MIDI port %s. error %d\n", str, err);
            return errFailed;
        }
    }

    mNumMIDIInPorts = numIn;

    for (int i=0; i<numOut; ++i) {
        char str[32];
        snprintf(str, 32, "out%d\n", i);
        CFStringRef outputPortName = CFStringCreateWithCString(alloc, str, enc);
        CFReleaser outputPortNameReleaser(outputPortName);

        err = MIDIOutputPortCreate(mMIDIClient, outputPortName, mMIDIOutPort+i);
        if (err) {
            mNumMIDIOutPorts = i;
            fprintf(stderr, "Could not create MIDI out port. error %d\n", err);
            return errFailed;
        }
    }
    mNumMIDIOutPorts = numOut;
}

void PortableMidiClient::prListMIDIEndpoints() {
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
#else
PortableMidiClient::PortableMidiClient(const int numIn, const int numOut, const RtMidiIn::RtMidiCallback midiCallback) {
    try {
        // Create and configure MIDI input ports
        for (int i = 0; i < numIn; ++i) {
            try {
                auto midiIn = new RtMidiIn();
                midiIn->setCallback(midiCallback, reinterpret_cast<void*>(i));
                midiIn->ignoreTypes(false, false, false); // Don't ignore sysex, timing, or active sensing
                mMIDIInPorts.push_back(midiIn);
            } catch (RtMidiError &error) {
                fprintf(stderr, "Error creating MIDI input port %d: %s\n", i, error.getMessage().c_str());
            }
        }
        
        mNumMidiInPorts = mMIDIInPorts.size();
        
        // Create MIDI output ports
        for (int i = 0; i < numOut; ++i) {
            try {
                auto midiOut = new RtMidiOut();
                mMIDIOutPorts.push_back(midiOut);
            } catch (RtMidiError &error) {
                fprintf(stderr, "Error creating MIDI output port %d: %s\n", i, error.getMessage().c_str());
            }
        }
        
        mNumMidiOutPorts = mMIDIOutPorts.size();
    } catch (RtMidiError &error) {
        fprintf(stderr, "Error initializing MIDI: %s\n", error.getMessage().c_str());
    } 
}

void PortableMidiClient::prListMIDIEndpoints() {
	try {
		RtMidiIn* midiin = new RtMidiIn();
		RtMidiOut* midiout = new RtMidiOut();
        
		unsigned int numSrc = midiin->getPortCount();
		unsigned int numDst = midiout->getPortCount();
        
		printf("midi sources %d destinations %d\n", (int)numSrc, (int)numDst);
        
		for (unsigned int i = 0; i < numSrc; i++) {
			try {
				std::string portName = midiin->getPortName(i);
				printf("MIDI Source %2d '%s' UID: %d\n", i, portName.c_str(), i);
			} catch (RtMidiError &error) {
				fprintf(stderr, "Error: %s\n", error.getMessage().c_str());
			}
		}
        
		for (unsigned int i = 0; i < numDst; i++) {
			try {
				std::string portName = midiout->getPortName(i);
				printf("MIDI Destination %2d '%s' UID: %d\n", i, portName.c_str(), i);
			} catch (RtMidiError &error) {
				fprintf(stderr, "Error: %s\n", error.getMessage().c_str());
			}
		}
        
		delete midiin;
		delete midiout;
	} catch (RtMidiError &error) {
		fprintf(stderr, "Error: %s\n", error.getMessage().c_str());
	}
}

#endif
