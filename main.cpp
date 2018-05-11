#include "openal/include/al.h"
#include "openal/include/alc.h"
#include <thread>
#include <chrono>
#include <map>
#include <string>
#include <iostream>
#include <array>

ALuint sources[2];
ALuint buffers[2];
const int samplingFrequency = 8000;

void printError(const ALenum &error, const std::string &context = "default") {
	std::string errorText;
	switch (error) {
		case AL_NO_ERROR:
			errorText = "AL_NO_ERROR";
			break;
		case AL_INVALID_NAME:
			errorText = "AL_INVALID_NAME";
			break;
		case AL_INVALID_ENUM:
			errorText = "AL_INVALID_ENUM";
			break;
		case AL_INVALID_VALUE:
			errorText = "AL_INVALID_VALUE";
			break;
		case AL_INVALID_OPERATION:
			errorText = "AL_INVALID_OPERATION";
			break;
		case AL_OUT_OF_MEMORY:
			errorText = "AL_OUT_OF_MEMORY";
			break;
		default:
			errorText = "Unknown error";
	}
	std::cout << "[" << context << "] Error: " << errorText << "(" << error << ")" <<  std::endl;
}

void playNote(char note, int duration) {
	// c = 261,626
	// d = 293,665
	// e = 329,628
	// f = 349,228
	// g = 391,995
	// a = 440,000
	
	const int C = samplingFrequency / 261.626;
	const int D = samplingFrequency / 293.665;
	const int E = samplingFrequency / 329.628;
	const int F = samplingFrequency / 349.228;
	const int G = samplingFrequency / 391.995;
	const int A = samplingFrequency / 440.000;
	
	std::map<char, int> strideForNote{{'c',C}, {'d',D}, {'e',E}, {'f',F}, {'g', G}, {'a',A}};
	
	
	//ALubyte data[1000];
	std::array<ALubyte, samplingFrequency> data;
	const ALubyte hi = 192;
	const ALubyte lo = 64;
	ALubyte cu = lo; // current level 
	
	int stride = strideForNote[note];
	
	for (int i = 0; i < samplingFrequency; ++i) {
		data[i] = cu;
		--stride;
		//std::cout << cu << " | " << stride << std::endl;
		if (stride == 0) {
			stride = strideForNote[note];
			cu = cu == hi ? lo : hi;
		}
	}
	
	alSourcei(sources[0], AL_BUFFER, 0);
	printError(alGetError(), "PlayNote_DetachBuffers");
	
	//alGetError();
	alBufferData(buffers[0], 
					AL_FORMAT_MONO8, 
					(void*)&data, 
					samplingFrequency,
					samplingFrequency);
	
	auto error = alGetError();
	printError(error, "PlayNote_BufferData");
	
	alSourcei(sources[0], AL_BUFFER, buffers[0]);
	printError(alGetError(), "PlayNote_BindBuffer");
	
	alSourcePlay(sources[0]);
	
	std::this_thread::sleep_for(std::chrono::milliseconds(1000 / duration));
	alSourceStop(sources[0]);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

int main(int argc, char* argv[]) {
	alGetError();
	
	
	auto device = alcOpenDevice(0);
	auto context = alcCreateContext(device, 0);
	alcMakeContextCurrent(context);
	printError(alGetError());

	alGenBuffers(1, buffers);
	printError(alGetError());
	
	
	alGenSources(1, sources);
	printError(alGetError());
	
	alSourcei(sources[0], AL_LOOPING, AL_TRUE);
	printError(alGetError());
	
	// c d e f g g a a a a g a a a a g
	// 8 8 8 8 4 4 8 8 8 8 2 8 8 8 8 2

	// f f f f e e g g g g c
	// 8 8 8 8 4 4 8 8 8 8 2
	playNote('c', 8);
	playNote('d', 8);
	playNote('e', 8);
	playNote('f', 8);
	playNote('g', 4);
	playNote('g', 4);
	
	playNote('a', 8);
	playNote('a', 8);
	playNote('a', 8);
	playNote('a', 8);
	playNote('g', 2);
	
	playNote('a', 8);
	playNote('a', 8);
	playNote('a', 8);
	playNote('a', 8);
	playNote('g', 2);
	
	playNote('f', 8);
	playNote('f', 8);
	playNote('f', 8);
	playNote('f', 8);
	playNote('e', 4);
	playNote('e', 4);
	
	playNote('g', 8);
	playNote('g', 8);
	playNote('g', 8);
	playNote('g', 8);
	playNote('c', 2);

	
	alcCloseDevice(device);
	return 0;
}