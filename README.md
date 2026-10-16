# AP-10 / GT913 Digital Piano Plugin

A C++ VST3 Digital Piano plugin built with JUCE, emulating the Casio AP-10 / GT913 sound engine and 24-voice ADPCM architecture.  
This is a port of gt913.cpp from https://github.com/mamedev/mame as a JUCE plugin, and was primarily written using Google AI Studio. It is low-latency and real-time playable, but the vibrato for the E.Piano and other aspects are still incomplete.

https://github.com/user-attachments/assets/f16b9c0b-1f7b-42df-9ac5-ef7b28b373ae

## Requirements

- AP-10 ROM file (`ap10.lsi303`)  
  It needs to be placed in the same directory as the VST3 plugin. For example, like this:
  ```
  C:\Program Files\Common Files\VST3\AP-10.vst3
  C:\Program Files\Common Files\VST3\ap10.lsi303
  ```
  
