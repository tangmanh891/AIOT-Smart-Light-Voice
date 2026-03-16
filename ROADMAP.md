# Roadmap

## Q2 2026
- Stabilize module boundaries (`audio`, `inference`, `iot`, `button`, `light`) and document coding conventions.
- Add integration test plan for wake-word and command transitions.
- Add power-on self-check logs for microphone and Sinric connectivity.

## Q3 2026
- Add OTA firmware update flow with rollback safety.
- Extend voice commands (brightness, scenes) with confidence/timeout tuning.
- Add local fallback mode when cloud control is unavailable.

## Q4 2026
- Add telemetry hooks for command accuracy and latency.
- Add optional web dashboard for device status.
- Support multiple devices and room-level naming.

## Backlog
- Add Home Assistant bridge.
- Add energy-saving automations.
- Add hardware abstraction for relay and PWM dimmer outputs.
