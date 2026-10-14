#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

FreqMapProcessor::FreqMapProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      fifoData ((size_t) fifoSize, 0.0f)
{
}

bool FreqMapProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    return in == layouts.getMainOutputChannelSet()
        && (in == AudioChannelSet::mono() || in == AudioChannelSet::stereo());
}

void FreqMapProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int numCh = getTotalNumInputChannels();
    const int n = buffer.getNumSamples();
    if (numCh == 0 || n == 0) return;

    int s1, z1, s2, z2;
    fifo.prepareToWrite (n, s1, z1, s2, z2);

    auto mono = [&] (int i)
    {
        float sum = 0.0f;
        for (int ch = 0; ch < numCh; ++ch) sum += buffer.getReadPointer (ch)[i];
        return sum / (float) numCh;
    };

    for (int i = 0; i < z1; ++i) fifoData[(size_t) (s1 + i)] = mono (i);
    for (int i = 0; i < z2; ++i) fifoData[(size_t) (s2 + i)] = mono (z1 + i);
    fifo.finishedWrite (z1 + z2);
    // audio przechodzi bez zmian
}

int FreqMapProcessor::pullSamples (float* dest, int maxNum)
{
    int s1, z1, s2, z2;
    fifo.prepareToRead (maxNum, s1, z1, s2, z2);
    std::copy (fifoData.begin() + s1, fifoData.begin() + s1 + z1, dest);
    std::copy (fifoData.begin() + s2, fifoData.begin() + s2 + z2, dest + z1);
    fifo.finishedRead (z1 + z2);
    return z1 + z2;
}

AudioProcessorEditor* FreqMapProcessor::createEditor() { return new FreqMapEditor (*this); }

void FreqMapProcessor::getStateInformation (MemoryBlock& dest)
{
    XmlElement xml ("FreqMapState");
    for (auto& path : customGuideFiles)
        xml.createNewChildElement ("Guide")->setAttribute ("path", path);
    copyXmlToBinary (xml, dest);
}

void FreqMapProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        customGuideFiles.clear();
        for (auto* e : xml->getChildIterator())
            customGuideFiles.addIfNotAlreadyThere (e->getStringAttribute ("path"));
    }
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FreqMapProcessor(); }
