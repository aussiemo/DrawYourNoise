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

// TODO(moritz): This ALubyte business is annoying. Just move all computation to float?
//  Does OpenAL support float buffers?


ALCdevice *g_device = nullptr;
ALCcontext *g_context = nullptr;
ALuint g_sources[2];
ALuint g_buffers[2];
const int g_samplingFrequency = 8000;
bool g_doLog = true;
std::string g_generator = "squareWave";


void printAlError(const ALenum &error, const std::string &context = "default") {
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

void printAlcError(const ALCenum &error, const std::string &context = "default") {
    if (!g_doLog) {
        return;
    }
    
    std::string errorText;
    switch (error) {
        case ALC_NO_ERROR:
            errorText = "ALC_NO_ERROR";
            break;
        case ALC_INVALID_DEVICE:
            errorText = "ALC_INVALID_DEVICE";
            break;
        case ALC_INVALID_CONTEXT:
            errorText = "ALC_INVALID_CONTEXT";
            break;
        case ALC_INVALID_ENUM:
            errorText = "ALC_INVALID_ENUM";
            break;
        case ALC_INVALID_VALUE:
            errorText = "ALC_INVALID_VALUE";
            break;
        case ALC_OUT_OF_MEMORY:
            errorText = "ALC_OUT_OF_MEMORY";
            break;
        default:
            errorText = "Unknown error";
    }
    std::cout << "[" << context << "] Error: " << errorText << "(" << error << ")" << std::endl; 
}

ALubyte computeSampleValueSquareWave(const int &sample, const int &samplingFrequency,
                                     const float &amplitude, const float &signalFrequency,
                                     const float &phase, const float &offset) {
    const float t = (float)sample / (float)samplingFrequency;
    const float sinval = std::sin(2.f*M_PI*signalFrequency*t + phase) + offset;
    const float sign = std::copysign(1.f, sinval);
    float sampleValue = sign < 0 ? 0.f : 255.f;
    return sampleValue * amplitude;
}

ALubyte computeSampleValueSawtoothWave(const int &sample, const int &samplingFrequency,
                                       const float &amplitude, const float &signalFrequency,
                                       const float &phase, const float &offset) {
    const float t = (float)sample / (float)samplingFrequency + phase;
    const float period = 1.0 / signalFrequency;
    
    return (ALubyte) amplitude * ((2.0f * (t/period - std::floor(0.5f + t/period)) + 1.0f) / 2.0f * 255.0f) + offset;
}

ALubyte computeSampleValueSineWave(const int &sample, const int &samplingFrequency,
                                   const float &amplitude, const float &signalFrequency,
                                   const float &phase, const float &offset) {
    const float t = (float)sample / (float)samplingFrequency;

    // I'm kind of expecting:
    //   0.0f <= amplitude <= 1.0f
    //   -1.0f <= offset <= 1.0f
    //   offset +/- amplitude <= +/-1.0f
    // To clip or not to clip?

    return (ALubyte) ((amplitude * std::sin(2.0f*M_PI*signalFrequency*t + phase) + offset + 1.0f) / 2.0f * 255.0f);
}

ALubyte computeSampleValue(const int &sample, const int &samplingFrequency, 
                           const float &amplitude, const float &signalFrequency,
                           const float &phase, const float offset, 
                           const std::string &method="squareWave") {
    if (method == "squareWave") {
        return computeSampleValueSquareWave(sample, samplingFrequency, amplitude, signalFrequency, phase, offset);
    } else if (method == "sawtoothWave") {
        return computeSampleValueSawtoothWave(sample, samplingFrequency, amplitude, signalFrequency, phase, offset);
    } else {
        return computeSampleValueSineWave(sample, samplingFrequency, amplitude, signalFrequency, phase, offset);
    }
}

void playBuffer(void* buffer, int bufferSize, int milliseconds) {
    alSourcei(g_sources[0], AL_BUFFER, 0);
    printAlError(alGetError(), "PlayNote_DetachBuffers");
    
    alBufferData(g_buffers[0], 
                    AL_FORMAT_MONO8, 
                    buffer, 
                    bufferSize,
                    g_samplingFrequency);
        
    printAlError(alGetError(), "PlayNote_BufferData");
        
    alSourcei(g_sources[0], AL_BUFFER, g_buffers[0]);
    printAlError(alGetError(), "PlayNote_BindBuffer");
        
    alSourcePlay(g_sources[0]);
        
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    alSourceStop(g_sources[0]);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void playNote(const float &frequency, const int &durationDivisor) {
    std::array<ALubyte, g_samplingFrequency> data;
    auto signalFrequency = frequency;
    for (int sample = 0; sample < g_samplingFrequency; ++sample) {
        data[sample] = computeSampleValue(sample, g_samplingFrequency, 1.0f, signalFrequency, 0.0f, 0.0f, g_generator);
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
    alcGetError(g_device);
    
    g_device = alcOpenDevice(0);
    g_context = alcCreateContext(g_device, 0);
    alcMakeContextCurrent(g_context);
    printAlcError(alcGetError(g_device), "alcOpenDevice");

    alGenBuffers(1, g_buffers);
    printAlError(alGetError());
    
    alGenSources(1, g_sources);
    printAlError(alGetError());
    
    alSourcei(g_sources[0], AL_LOOPING, AL_TRUE);
    printAlError(alGetError());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void tearDownOpenAl() {
    printAlcError(alcGetError(g_device), "alcCloseDevice_pre");
    
    ALCboolean closeSucceeded = alcCloseDevice(g_device);
    if (!closeSucceeded) {
        std::cout << "closing device failed";
    }
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
        
        if (height < 0) height = -height;
        
        
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
        
        // debug file
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
        
        // Generate pcmData
        std::cout << "Generate pcmData" << std::endl;
        std::vector<ALubyte> pcmData;
        const int sizeFactor = 2;
        for (int sample{}; sample < g_samplingFrequency*sizeFactor; ++sample) {
            bool doDebug = sample % 100 == 0;
            if (doDebug) { std::cout << "Debug sample loop: " << sample << std::endl; }
            
            std::vector<ALubyte> sampleValuesForCurrentSample;
            for (int y{}; y < height; ++y) {
                //std::cout << "Debug y loop: " << y << std::endl;
                for (int x{}; x < width; ++x) {
                    // std::cout << "Debug x loop: " << x << std::endl;
                    RGB &rgb = pixels[y*width+x];
                    if (rgb.r == 255 && rgb.g == 255 && rgb.b == 255) {
                        continue;
                    }
                    float signalFrequency = x;
                    if (signalFrequency == 0) {
                        continue;
                    }
                    float amplitude = rgb.r / 255.0f;
                    float phase = (y/height) * 1.f/signalFrequency; // TODO(moritz): think about phase handling that makes more sense
                    float offset = 0.0f;
                    if (rgb.g || rgb.b) {
                        offset = (1.0f - amplitude) * (1.0f / (rgb.g + rgb.b));
                    }
                    // std::cout << "Computing sampleValue " << sample << ", " << x << ", " << y << std::endl;
                    // TODO(moritz): Optimize loop: 
                    //                 - currently the relevant pixels never change, 
                    //                   collect them first before sampling
                    //                 - presize vectors move up scope, they will be the same size for all samples
                    ALubyte sampleValue = computeSampleValue(sample, g_samplingFrequency, amplitude, 
                                                             signalFrequency, phase, offset, g_generator);
                    sampleValuesForCurrentSample.emplace_back(sampleValue);
                }
            }
            std::uint32_t sum{};
            for (const auto &sv : sampleValuesForCurrentSample) {
                sum += sv;
            }
            sum /= sampleValuesForCurrentSample.size();
            assert(sum < 256);
            pcmData.emplace_back((ALubyte) sum);
        }
        
        // play pcmData
        std::cout << "Play pcmData from Bitmap" << std::endl;
        playBuffer((void*) pcmData.data(), g_samplingFrequency*sizeFactor, 4000);
        
        // TODO(moritz): Save generated Data as poor mans .pcm so that
        //  a generate result can be replayed quickly.
        
        
        
        
    }
    
    tearDownOpenAl();
    return 0;
}