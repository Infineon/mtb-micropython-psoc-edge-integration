# MTB Libraries for PSOC™ Edge MicroPython Integration

ModusToolbox assets required for PSOC™ Edge port and integrated boards.


## (WIP) VA model Integration

Refer to [README](ports/psoc-edge/README.md) in ports.

```
import ipc
ipc.sendcmd_led_on() # Start VA model
ipc.sendcmd_led_off() # Stop VA model
```

if you have license and want to run full demo, you need to:
1. set CONFIG_VOICE_CORE_MODE=FULL in ./common.mk
2. After
    make BOARD=KIT_PSE84_AI
Copy your dowloaded licensed /audio-voice-core/ to ../mtb_shared/audio-voice-core/release-v2.0.0. Replace the file.
Then 
    make deploy