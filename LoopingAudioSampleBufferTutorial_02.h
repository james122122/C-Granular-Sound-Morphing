/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2017 - ROLI Ltd.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             LoopingAudioSampleBufferTutorial
 version:          2.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Explores audio sample buffer looping.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2017, linux_make

 type:             Component
 mainClass:        MainContentComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
class MainContentComponent   : public AudioAppComponent
{
public:
    MainContentComponent()
    {
        addAndMakeVisible (openButton);
        openButton.setButtonText ("Open...");
        openButton.onClick = [this] { openButtonClicked(); };

        addAndMakeVisible (clearButton);
        clearButton.setButtonText ("Clear");
        clearButton.onClick = [this] { clearButtonClicked(); };

        addAndMakeVisible (levelSlider);
        levelSlider.setRange (0.0, 1.0);
        levelSlider.onValueChange = [this] { currentLevel = (float) levelSlider.getValue(); };
        
        addAndMakeVisible (randomSlider);
        randomSlider.setRange (0.0, 1.0);
        randomSlider.onValueChange = [this] { currentRandom = (float) randomSlider.getValue(); };

        addAndMakeVisible (rampLenSlider);
        rampLenSlider.setRange (0, 256);

        setSize (300, 200);

        formatManager.registerBasicFormats();
        
        //Seed the random object
        random.setSeed(Time::currentTimeMillis());
    }

    ~MainContentComponent()
    {
        shutdownAudio();
    }

    void prepareToPlay (int, double) override {}

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        auto level = currentLevel;
        auto startLevel = level == previousLevel ? level : previousLevel;

        auto numInputChannels = fileBuffer.getNumChannels();
        auto numOutputChannels = bufferToFill.buffer->getNumChannels();

        auto outputSamplesRemaining = bufferToFill.numSamples;
        auto outputSamplesOffset = bufferToFill.startSample;

        //A crass looping mechanism to remove the complexity of the loop below.
        //If the file buffer wants more samples than are available in the file buffer,
        //just loop back to zero right away instead of reading samples to the end then
        //looping to the start. This means we'll lose a small bunch of samples at the very
        //end of the file each loop, but worth it to make the loop below a bit easier to follow
        if((position + bufferToFill.numSamples) >= fileBuffer.getNumSamples()) {
            position = 0;
        }
        
        auto bufferSamplesRemaining = fileBuffer.getNumSamples() - position;
        auto samplesThisTime = jmin (outputSamplesRemaining, bufferSamplesRemaining);

        //Create a read position based on the correct read position, plus an
        //offset depending on value of the random slider
        int readPosition = position;
        if(currentRandom != 0.f) {
            int randomRange = (int)((float)fileBuffer.getNumSamples() * currentRandom);
            readPosition = position + random.nextInt(randomRange);
            //Wrap it around to make sure it doesn't exceed the file buffer size
            if(readPosition >= fileBuffer.getNumSamples() - samplesThisTime) {
                readPosition = (readPosition + samplesThisTime) % fileBuffer.getNumSamples();
            }
        }
        
        for (auto channel = 0; channel < numOutputChannels; ++channel)
        {
            bufferToFill.buffer->copyFrom (channel,
                                           outputSamplesOffset,
                                           fileBuffer,
                                           channel % numInputChannels,
                                           readPosition,
                                           samplesThisTime);

            //Apply the gain from the volume slider (do it as a ramp from the previous
            //slider value to avoid 'zipping' or sudden change in ampltitude)
            bufferToFill.buffer->applyGainRamp (channel, outputSamplesOffset, samplesThisTime, startLevel, level);
            
        }

        outputSamplesRemaining -= samplesThisTime;
        outputSamplesOffset += samplesThisTime;
        position += samplesThisTime;

        if (position == fileBuffer.getNumSamples()) {
                position = 0;
        }
        
        //Also apply a window to the sample to smooth out square amplitude window
        //Get the ramp length from the slider
        int rampLen = (int)rampLenSlider.getValue();
        
        int totalSamples = bufferToFill.numSamples;
        for (auto channel = 0; channel < numOutputChannels; ++channel) {
            //Fade in
            bufferToFill.buffer->applyGainRamp(channel, 0, rampLen, 0.f, 1.f);
            //Fade out
            bufferToFill.buffer->applyGainRamp
            (channel, totalSamples - rampLen, rampLen, 1.f, 0.f);
        }


        previousLevel = level;
    }

    void releaseResources() override
    {
        fileBuffer.setSize (0, 0);
    }

    void resized() override
    {
        openButton .setBounds (10, 10, getWidth() - 20, 20);
        clearButton.setBounds (10, 40, getWidth() - 20, 20);
        levelSlider.setBounds (10, 70, getWidth() - 20, 20);
        randomSlider.setBounds (10, 100, getWidth() - 20, 20);
        rampLenSlider.setBounds (10, 130, getWidth() - 20, 20);
    }

private:
    void openButtonClicked()
    {
        shutdownAudio();                                                                      // [1]

        FileChooser chooser ("Select a Wave file shorter than 5 seconds to play...",
                             File::nonexistent,
                             "*.wav");

        if (chooser.browseForFileToOpen())
        {
            auto file = chooser.getResult();
            std::unique_ptr<AudioFormatReader> reader (formatManager.createReaderFor (file)); // [2]

            if (reader.get() != nullptr)
            {
                auto duration = reader->lengthInSamples / reader->sampleRate;                 // [3]

                if (duration < 5)
                {
                    fileBuffer.setSize (reader->numChannels, (int) reader->lengthInSamples);  // [4]
                    reader->read (&fileBuffer,                                                // [5]
                                  0,                                                          //  [5.1]
                                  (int) reader->lengthInSamples,                              //  [5.2]
                                  0,                                                          //  [5.3]
                                  true,                                                       //  [5.4]
                                  true);                                                      //  [5.5]
                    position = 0;                                                             // [6]
                    setAudioChannels (0, reader->numChannels);                                // [7]
                }
                else
                {
                    // handle the error that the file is 2 seconds or longer..
                }
            }
        }
    }

    void clearButtonClicked()
    {
        shutdownAudio();
    }

    //==========================================================================
    TextButton openButton;
    TextButton clearButton;
    Slider levelSlider;
    Slider randomSlider;
    Slider rampLenSlider;

    AudioFormatManager formatManager;
    AudioSampleBuffer fileBuffer;
    int position = 0;

    float currentLevel = 0.0f, previousLevel = 0.0f;
    float currentRandom = 0.f;
    
    Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
