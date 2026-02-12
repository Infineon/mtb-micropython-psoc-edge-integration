# MTB Libraries for PSOC™ Edge MicroPython Integration

ModusToolbox assets required for PSOC™ Edge port and integrated boards.


## (WIP) VA model Integration

### Pre-Condition
Refer to [README](../../ports/psoc-edge/README.md) in ports.
Besides the MTB PSOC Edge Tools mentioned above, you need to also download llvm: [LLVM compiler](https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/releases/tag/release-19.1.5), and extract to /opt/Tools:

    sudo tar -xJf ./LLVM-ET-Arm-19.1.5-Linux-x86_64.tar.xz -C /opt/Tools

Then do not forget run the script set up the environment.

### Full demo
if you have license and want to run full demo (no time limit), you need to:
* set `CONFIG_VOICE_CORE_MODE=FULL` in ./common.mk

* After `make BOARD=KIT_PSE84_AI`

Copy your dowloaded licensed `/audio-voice-core/` to `../mtb_shared/audio-voice-core/release-v2.0.0.` Replace the file.
Then 

    make deploy

### Code example
```
import ipc

try:
    ipc.start_cm55_va_model()
    print("[App] Voice Assistant started successfully")
except Exception as e:
    print("[App] Error starting Voice Assistant: {}".format(e))

try:
    while True:
        # Check for wake-word detection
        if ipc.has_wakeword():
            print("\n[App] >>> Wake-word detected! <<<")
            ipc.clear_wakeword()
        
        # Check for command detection
        if ipc.has_command():
            # Get the command
            command = ipc.e()
            
            if command:
                print("\n[App] >>> Command received: {}".format(command))
                
                # Forward command via I2C
                if send_command_via_i2c(command):
                    print("[App] Command forwarded successfully")
                else:
                    print("[App] Failed to forward command")
                
                # Clear the command flag
                ipc.clear_command()
            
        # Small delay to avoid busy-waiting
        time.sleep(1)


```

#### Deploy your own model

https://deepcraft-voice-assistant.infineon.com/DVA_User_Guide_new_branding.pdf