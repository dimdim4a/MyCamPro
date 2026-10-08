# MyCam Pro architecture

## Core runtime
Android Camera → Encoder → Transport → Decoder → Processing → Virtual Camera

Audio: Android/PC microphone → Audio Engine → MyCam Pro Microphone

Side services: Device Manager, Sync, Fallback, Recording, Stats, UI.

## Hard rule
The main video path must not depend on UI, AI, effects, recording or diagnostics. Secondary features may degrade or stop without taking the camera down.

## Performance rules
1. Prefer hardware encoding/decoding.
2. Minimize CPU↔GPU copies.
3. Keep buffers small and adaptive.
4. Never add heavy background work while the camera is live unless enabled.
5. User can disable automatic quality adaptation.
