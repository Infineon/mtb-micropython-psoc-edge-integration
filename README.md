# MTB Libraries for PSOC™ Edge MicroPython Integration

ModusToolbox assets required for PSOC™ Edge port and integrated boards.


## (WIP) VA model Integration

### Pre-Condition
Refer to [README](../../ports/psoc-edge/README.md) in ports.
Besides the MTB PSOC Edge Tools mentioned above, you need to also download llvm: [LLVM compiler](https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/releases/tag/release-19.1.5), and extract to /opt/Tools:

    sudo tar -xJf ./LLVM-ET-Arm-19.1.5-Linux-x86_64.tar.xz -C /opt/Tools

Then do not forget run the script set up the environment.

### Code example
```
import ipc
ipc.sendcmd_led_on() # Start VA model
ipc.sendcmd_led_off() # Stop VA model
```

### Note

#### Full demo
if you have license and want to run full demo (no time limit), you need to:
* set CONFIG_VOICE_CORE_MODE=FULL in ./common.mk

* After `make BOARD=KIT_PSE84_AI`

Copy your dowloaded licensed `/audio-voice-core/` to `../mtb_shared/audio-voice-core/release-v2.0.0.` Replace the file.
Then 

    make deploy

#### Deploy your own model

https://deepcraft-voice-assistant.infineon.com/DVA_User_Guide_new_branding.pdf