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
#include <fstream>
#include <cstdint>

ALCdevice *g_device = nullptr;
ALCcontext *g_context = nullptr;
ALuint g_sources[2];
ALuint g_buffers[2];
const int g_samplingFrequency = 8000;
bool g_doLog = true;
std::string g_generator = "squareWave";


void printError(const ALenum &error, const std::string &context = "default") {
	if (!g_doLog) {
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

void playBuffer(void* buffer, int bufferSize, int milliseconds) {
	alSourcei(g_sources[0], AL_BUFFER, 0);
	printError(alGetError(), "PlayNote_DetachBuffers");
	
	alBufferData(g_buffers[0], 
					AL_FORMAT_MONO8, 
					buffer, 
					bufferSize,
					g_samplingFrequency);
		
	printError(alGetError(), "PlayNote_BufferData");
		
	alSourcei(g_sources[0], AL_BUFFER, g_buffers[0]);
	printError(alGetError(), "PlayNote_BindBuffer");
		
	alSourcePlay(g_sources[0]);
		
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	alSourceStop(g_sources[0]);
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void playNote(const float &frequency, const int &durationDivisor) {
	std::array<ALubyte, g_samplingFrequency> data;
	auto signalFrequency = frequency;
	for (int sample = 0; sample < g_samplingFrequency; ++sample) {
		data[sample] = computeSampleValue(sample, g_samplingFrequency, signalFrequency, g_generator);
	}
	
	playBuffer((void*)data.data(), g_samplingFrequency, 1000/durationDivisor);
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

// TODO(mja): Why is alBufferdata sometimes generating AL_INVALID_OPERATION after 
//			  recompile on OSX?
//			  Maybe this is connected to alc-errors which are not handled properly yet.
// NOTE(mja): Not used, but here so that clients do not have to deal with Note if they
// 			  don't want to.
void playNote(char noteName, int durationDivisor) {
	Note note(noteName, durationDivisor);
	playNote(note.frequency, note.durationDivisor);
}

void playNotes(const std::vector<Note> &notes) {
	for (auto &note : notes) {
		playNote(note.frequency, note.durationDivisor);
	}
}

void setupOpenAlDeviceWithOneSourceAndOneBuffer() {
	alGetError();
	
	g_device = alcOpenDevice(0);
	g_context = alcCreateContext(g_device, 0);
	alcMakeContextCurrent(g_context);
	printError(alGetError());

	alGenBuffers(1, g_buffers);
	printError(alGetError());
	
	alGenSources(1, g_sources);
	printError(alGetError());
	
	alSourcei(g_sources[0], AL_LOOPING, AL_TRUE);
	printError(alGetError());
	
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void tearDownOpenAl() {
	alcGetError(g_device);
	
	alcCloseDevice(g_device);
	
	// TODO(mja): printError is based on al-errorCodes, not on alc-errorCodes.
	//printError(alcGetError(g_device), "alcCloseDevice");
	
}

void playAlleMeineEntchen() {
	std::vector<Note> alleMeineEntchen = {
		Note('c', 8), Note('d', 8), Note('e', 8), Note('f', 8), Note('g', 4), Note('g', 4),
		Note('a', 8), Note('a', 8), Note('a', 8), Note('a', 8), Note('g', 2),
		Note('a', 8), Note('a', 8), Note('a', 8), Note('a', 8), Note('g', 2),
		Note('f', 8), Note('f', 8), Note('f', 8), Note('f', 8), Note('e', 4), Note('e', 4),
		Note('g', 8), Note('g', 8), Note('g', 8), Note('g', 8), Note('c', 2)
	};
	playNotes(alleMeineEntchen);
}

struct RGB {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

int main(int argc, char* argv[]) {
	// STUDY(mja): replace this epicness with proper command line parser
	std::map<std::string, std::string> commandLineOptions;
	std::string currentKey;
	for (int i = 0; i < argc; ++i) {
		std::string value = argv[i];
		if (value.size() > 1 && value[0] == '-') {
			currentKey = value;
			commandLineOptions[currentKey] = "";
		} else {
			commandLineOptions[currentKey] = value;
		}
	}
	
	if (commandLineOptions.find("-generator") != commandLineOptions.end()) {
		g_generator = commandLineOptions["-generator"];
	}
	
	setupOpenAlDeviceWithOneSourceAndOneBuffer();
	
	if (commandLineOptions.find("-alleMeineEntchen") != commandLineOptions.end()) {
		playAlleMeineEntchen();
	}
	
	if (commandLineOptions.find("-playFile") != commandLineOptions.end()) {
		auto fileName = commandLineOptions["-playFile"];
		std::ifstream testFile(fileName, std::ios::binary|std::ios::ate);
		auto bytesCount = testFile.tellg();
		auto fileBytes = new ALubyte[bytesCount];
		testFile.seekg(0, std::ios::beg);
		testFile.read((char *)fileBytes, bytesCount);
		
		playBuffer((void*)fileBytes, bytesCount, 4000);
		
		delete[] fileBytes;
	}
	
	if (commandLineOptions.find("-playBitmap") != commandLineOptions.end()) {
		auto fileName = commandLineOptions["-playBitmap"];
		std::ifstream bmpFile(fileName, std::ios::binary);
		std::cout << "File is open: " << std::boolalpha << bmpFile.is_open() << std::endl;
		
        // compute file size in bytes
        bmpFile.seekg(0, bmpFile.end);
        int length = bmpFile.tellg();
        bmpFile.seekg(0, bmpFile.beg);
        std::cout << "File size: " << length << std::endl;
		
		// Header
		std::uint16_t type = 0;
		std::uint32_t sizeInBytes = 0;
		std::uint32_t reserved_1 = 0;
		std::uint32_t dataOffset = 0;
        
        bmpFile.read((char*)&type, sizeof(type)); // should be 16973
        bmpFile.read((char*)&sizeInBytes, sizeof(sizeInBytes));
        bmpFile.read((char*)&reserved_1, sizeof(reserved_1));
        bmpFile.read((char*)&dataOffset, sizeof(dataOffset));
        
        // Info header
        std::uint32_t infoHeaderSizeInBytes = 0;
        std::int32_t width = 0;
        std::int32_t height = 0;
        
        bmpFile.read((char*)&infoHeaderSizeInBytes, sizeof(infoHeaderSizeInBytes));
        bmpFile.read((char*)&width, sizeof(width));
        bmpFile.read((char*)&height, sizeof(height));
		
        // read pixels
        // line by line, each line is 4 byte aligned with zero bytes
        bmpFile.seekg(dataOffset, bmpFile.beg);
        std::vector<RGB> pixels;
        const int pad = width % 4;
        for (int y{}; y < height; ++y) {
            for (int x{}; x < width; ++x) {
                RGB rgb;
                rgb.b = bmpFile.get();
                rgb.g = bmpFile.get();
                rgb.r = bmpFile.get();
                pixels.emplace_back(rgb);
            }
            bmpFile.ignore(pad);
        }

		
		std::cout << "       Type: " << type << " " << ((char*)&type)[0] << ((char*)&type)[1] << std::endl;
		std::cout << "SizeInBytes: " << sizeInBytes << std::endl;
		std::cout << " reserved_1: " << reserved_1 << std::endl;
		std::cout << " DataOffset: " << dataOffset << std::endl;
        std::cout << "   infoSize: " << infoHeaderSizeInBytes << std::endl;
        std::cout << "      width: " << width << std::endl;
        std::cout << "     height: " << height << std::endl;
        std::cout << std::endl;
        std::cout << "pixel count: " << pixels.size() << std::endl;
        std::cout << "pixel 0 = rgb(" << +pixels[0].r << "," << +pixels[0].g << "," << +pixels[0].b << ")" << std::endl;
        
        
	}
	
	tearDownOpenAl();
	return 0;
}