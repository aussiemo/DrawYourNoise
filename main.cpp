#include "openal/include/al.h"
#include "openal/include/alc.h"

ALubyte pcmData[1024];

void fillPcmData() {
	for (int i = 0; i < 1024; ++i) {
		pcmData[i] = 128 + ((i%128) - 64);
	}
}

int main(int argc, char* argv[]) {
	auto device = alcOpenDevice(0);
	auto context = alcCreateContext(device, 0);
	alcMakeContextCurrent(context);
	
	ALuint *buffers = nullptr;
	alGenBuffers(1, buffers);
	
	alBufferData(buffers[0], 
					AL_FORMAT_MONO8, 
					data, 
					size, 
					freq);
	
	alcCloseDevice(device);
	
	if (ALC_TRUE == closeWasSuccessful) {
		return 0;
	} else {
		return 1;
	}
}