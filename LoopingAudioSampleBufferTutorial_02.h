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
        
        addAndMakeVisible (openButton2);
        openButton2.setButtonText ("Open2...");
        openButton2.onClick = [this] { openButton2Clicked(); };

        addAndMakeVisible (clearButton);
        clearButton.setButtonText ("Clear");
        clearButton.onClick = [this] { clearButtonClicked(); };

        addAndMakeVisible (levelSlider);
        levelSlider.setRange (0.0, 1.0);
        levelSlider.onValueChange = [this] { currentLevel = (float) levelSlider.getValue(); };
        
        addAndMakeVisible (levelSlider2);
        levelSlider2.setRange (0.0, 1.0);
        levelSlider2.onValueChange = [this] { currentLevel = (float) levelSlider2.getValue(); };

        setSize (300, 200);

        formatManager.registerBasicFormats();
    }

    ~MainContentComponent()
    {
        shutdownAudio();
    }

    void prepareToPlay (int, double) override {
        
        
        
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        auto level = currentLevel;
        auto startLevel = level == previousLevel ? level : previousLevel;

        auto numInputChannels = fileBuffer.getNumChannels();
        auto numOutputChannels = bufferToFill.buffer->getNumChannels();

        auto outputSamplesRemaining = bufferToFill.numSamples;
        auto outputSamplesOffset = bufferToFill.startSample;

        while (outputSamplesRemaining > 0)
        {
            auto bufferSamplesRemaining = fileBuffer.getNumSamples() - position;
            auto samplesThisTime = jmin (outputSamplesRemaining, bufferSamplesRemaining);

            for (auto channel = 0; channel < numOutputChannels; ++channel)
            {
                bufferToFill.buffer->copyFrom (channel,
                                               outputSamplesOffset,
                                               fileBuffer,
                                               channel % numInputChannels,
                                               position,
                                               samplesThisTime);

                bufferToFill.buffer->applyGainRamp (channel, outputSamplesOffset, samplesThisTime, startLevel, level);
            }

            outputSamplesRemaining -= samplesThisTime;
            outputSamplesOffset += samplesThisTime;
            position += samplesThisTime;

            if (position == fileBuffer.getNumSamples())
                position = 0;
        }

        previousLevel = level;
        
        
    }

    void releaseResources() override
    {
        fileBuffer.setSize (0, 0);
        fileBuffer2.setSize (0, 0);
    }

    void resized() override
    {
        openButton .setBounds (10, 10, getWidth() - 20, 20);
        clearButton.setBounds (10, 40, getWidth() - 20, 20);
        levelSlider.setBounds (10, 70, getWidth() - 20, 20);
        openButton2 .setBounds (10, 100, getWidth() - 20, 20);
        levelSlider2.setBounds (10, 160, getWidth() - 20, 20);
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
                    // handle the error that the file is 5 seconds or longer..
                }
            }
        }
        
    }
    void openButton2Clicked()
    {
        shutdownAudio();
    FileChooser chooser2 ("Select a Wave file shorter than 2 seconds to play...",
                          File::nonexistent,
                          "*.wav");
    
    if (chooser2.browseForFileToOpen())
    {
        auto file2 = chooser2.getResult();
        std::unique_ptr<AudioFormatReader> reader2 (formatManager.createReaderFor (file2)); // [2]
        
        if (reader2.get() != nullptr)
        {
            auto duration2 = reader2->lengthInSamples / reader2->sampleRate;                 // [3]
            
            if (duration2 < 5)
            {
                fileBuffer2.setSize (reader2->numChannels, (int) reader2->lengthInSamples);  // [4]
                reader2->read (&fileBuffer2,                                                // [5]
                               0,                                                          //  [5.1]
                               (int) reader2->lengthInSamples,                              //  [5.2]
                               0,                                                          //  [5.3]
                               true,                                                       //  [5.4]
                               true);                                                      //  [5.5]
                position2 = 0;                                                             // [6]
                //setAudioChannels (0, reader2->numChannels); //this line causes runtime error                                // [7]
            }
            else
            {
                // handle the error that the file is 5 seconds or longer..
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
    TextButton openButton2;
    TextButton clearButton;
    Slider levelSlider;
    Slider levelSlider2;

    AudioFormatManager formatManager;
    AudioSampleBuffer fileBuffer, fileBuffer2;
    int position = 0, position2 = 0;

    float currentLevel = 0.0f, previousLevel = 0.0f, currentLevel2 = 0.0f, previousLevel2 = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
