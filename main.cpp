#include "openal/include/al.h"
#include "openal/include/alc.h"
#include <thread>
#include <chrono>
#include <map>
#include <string>
#include <iostream>
#include <array>
#include <cmath>
#include <cassert>
#include <vector>

ALuint sources[2];
ALuint buffers[2];
const int samplingFrequency = 8000;
bool doLog = true;
std::string generator = "squareWave";


void printError(const ALenum &error, const std::string &context = "default") {
	if (!doLog) {
		return;
	}
	
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

ALubyte computeSampleValueSquareWave(const int &sample, const int &samplingFrequency,
										const float &signalFrequency) {
	constexpr ALubyte hi = 192;
	constexpr ALubyte lo = 64;
	
	const int stride = samplingFrequency / signalFrequency;
	
	return (sample / stride) % 2 ? lo : hi;
}

ALubyte computeSampleValueSawtoothWave(const int &sample, const int &samplingFrequency,
										const float &signalFrequency) {
	const float t = (float)sample / (float)samplingFrequency;
	const float a = 1.0 / signalFrequency;
	
	return (ALubyte) ((2.0f * (t/a - std::floor(0.5f + t/a)) + 1.0f) / 2.0f * 255.0f);
}

ALubyte computeSampleValueSineWave(const int &sample, const int &samplingFrequency,
										const float &signalFrequency) {
	const float t = (float)sample / (float)samplingFrequency;
	return (ALubyte) ((std::sin(2.0f*M_PI*(float)signalFrequency*t) + 1.0f) / 2.0f * 255.0f);
}

ALubyte computeSampleValue(const int &sample, const int &sampleFrequency, 
							const float &signalFrequency, const std::string &method="squareWave") {
	if (method == "squareWave") {
		return computeSampleValueSquareWave(sample, sampleFrequency, signalFrequency);
	} else if (method == "sawtoothWave") {
		return computeSampleValueSawtoothWave(sample, sampleFrequency, signalFrequency);
	} else {
		return computeSampleValueSineWave(sample, sampleFrequency, signalFrequency);
	}
}

void playNote(const float &frequency, const int &durationDivisor) {
	std::array<ALubyte, samplingFrequency> data;
	auto signalFrequency = frequency;
	for (int sample = 0; sample < samplingFrequency; ++sample) {
		data[sample] = computeSampleValue(sample, samplingFrequency, signalFrequency, generator);
	}
	
	alSourcei(sources[0], AL_BUFFER, 0);
	printError(alGetError(), "PlayNote_DetachBuffers");
	
	alBufferData(buffers[0], 
					AL_FORMAT_MONO8, 
					(void*)&data, 
					samplingFrequency,
					samplingFrequency);
	
	printError(alGetError(), "PlayNote_BufferData");
	
	alSourcei(sources[0], AL_BUFFER, buffers[0]);
	printError(alGetError(), "PlayNote_BindBuffer");
	
	alSourcePlay(sources[0]);
	
	std::this_thread::sleep_for(std::chrono::milliseconds(1000 / durationDivisor));
	alSourceStop(sources[0]);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

// TODO(mja): Why is alBufferdata sometimes generating AL_INVALID_OPERATION after recompile on OSX? 
void playNote(char note, int duration) {
	const float C = 261.626;
	const float D = 293.665;
	const float E = 329.628;
	const float F = 349.228;
	const float G = 391.995;
	const float A = 440.000;

	std::map<char, float> frequencyForNote{{'c',C}, {'d',D}, {'e',E}, {'f',F}, {'g', G}, {'a',A}};

	std::array<ALubyte, samplingFrequency> data;
	auto signalFrequency = frequencyForNote[note];
	for (int sample = 0; sample < samplingFrequency; ++sample) {
		data[sample] = computeSampleValue(sample, samplingFrequency, signalFrequency, generator);
		//std::cout << (int)data[sample] << std::endl;
		
	}
	
	alSourcei(sources[0], AL_BUFFER, 0);
	printError(alGetError(), "PlayNote_DetachBuffers");
	// std::this_thread::sleep_for(std::chrono::milliseconds(2));
	
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

struct Note {
	Note(const char &name, const int &durationDivisor) : 
		name(name),
		durationDivisor(durationDivisor),
		frequency() 
	{
		switch (name) {
			case 'c':
				frequency = 261.626f;
				break;
			case 'd':
				frequency = 293.665f;
				break;
			case 'e':
				frequency = 329.628f;
				break;
			case 'f':
				frequency = 349.228f;
				break;
			case 'g':
				frequency = 391.995f;
				break;
			case 'a':
				frequency = 440.000f;
				break;
			default:
				// TODO(mja): throw
				frequency = 440.000f;
		}
	}

	char name;
	int durationDivisor;
	float frequency;
};

void playNotes(const std::vector<Note> &notes) {
	for (auto &note : notes) {
		playNote(note.frequency, note.durationDivisor);
	}
}

int main(int argc, char* argv[]) {
	if (argc == 2) {
		generator = argv[1];
	}
	
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
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
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
	
	std::vector<Note> alleMeineEntchen = {
		Note('c', 8), Note('d', 8), Note('e', 8), Note('f', 8), Note('g', 4), Note('g', 4),
		Note('a', 8), Note('a', 8), Note('a', 8), Note('a', 8), Note('g', 2),
		Note('a', 8), Note('a', 8), Note('a', 8), Note('a', 8), Note('g', 2),
		Note('f', 8), Note('f', 8), Note('f', 8), Note('f', 8), Note('e', 4), Note('e', 4),
		Note('g', 8), Note('g', 8), Note('g', 8), Note('g', 8), Note('c', 2)
	};
	playNotes(alleMeineEntchen);
	
	alcCloseDevice(device);
	return 0;
}