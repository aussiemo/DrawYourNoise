#include "openal/include/al.h"
#include "openal/include/alc.h"



int main(int argc, char* argv[]) {
	auto defaultDevice = alcOpenDevice(0);
	auto closeWasSuccessful = alcCloseDevice(defaultDevice); 
	// for fun: alcCloseDevice((ALCdevice *)1); this causes crash instead of returning ALC_FALSE
	
	if (ALC_TRUE == closeWasSuccessful) {
		return 0;
	} else {
		return 1;
	}
}