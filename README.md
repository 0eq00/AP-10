# AP-10 / GT913 Digital Piano Plugin

A C++ VST3 Digital Piano plugin built with JUCE, emulating the Casio AP-10 / GT913 sound engine and 24-voice ADPCM architecture.  
This is a port of gt913.cpp from https://github.com/mamedev/mame as a JUCE plugin, and was primarily written using Google AI Studio & Claude. It is low-latency and real-time playable. Envelopes and EP vibrato written in machine code within the ROM are reproduced based on debug logs.

https://github.com/user-attachments/assets/ef54d114-83fa-49b5-b781-d9eefbd80e67

## Requirements

- AP-10 ROM file (`ap10.lsi303`)  
  It needs to be placed in the same directory as the VST3 plugin. For example, like this:
  ```
  C:\Program Files\Common Files\VST3\AP-10.vst3
  C:\Program Files\Common Files\VST3\ap10.lsi303
  ```
  
