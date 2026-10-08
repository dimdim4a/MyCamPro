# MyCam Pro — Prototype P0

## Goal
Validate the first end-to-end path:
Android camera → hardware H.264 → transport → Windows decode → MyCam Pro Camera → OBS.

## Acceptance criteria
- Manual device connection only.
- 720p30, 720p60, 1080p30 and 1080p60 when the phone supports them.
- H.264 hardware encoder when available.
- Windows hardware decoder when available; CPU fallback otherwise.
- One virtual camera: MyCam Pro Camera.
- Camera stays present while the phone is temporarily unavailable and shows fallback.
- Reconnect without restarting OBS.
- Report actual FPS, bitrate, latency and dropped frames.
- No cloud account or cloud media path.
- Core video path must remain independent from UI, AI and recording.

## Prototype-only note
The current bootstrap still consumes the Nexora engine at a pinned commit. This is temporary for P0 validation. It is not the final clean-room MyCam Engine and must be replaced/isolated before commercial release.


CI packaging validation updated.
