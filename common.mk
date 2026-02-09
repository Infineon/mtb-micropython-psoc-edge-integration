################################################################################
# \file common.mk
# \version 1.0
#
# \brief
# Settings shared across all projects.
#
################################################################################
# \copyright
# Copyright 2023-2025, Cypress Semiconductor Corporation (an Infineon company)
# SPDX-License-Identifier: Apache-2.0
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################

MTB_TYPE=PROJECT

# Target board/hardware (BSP).
# To change the target, it is recommended to use the Library manager
# ('make library-manager' from command line), which will also update 
# Eclipse IDE launch configurations.
TARGET=APP_$(BOARD)

# Name of toolchain to use. Options include:
#
# GCC_ARM 	-- GCC is available as part of ModusToolbox Setup program
# ARM     	-- ARM Compiler (must be installed separately)
# IAR     	-- IAR Compiler (must be installed separately)
# LLVM_ARM	-- LLVM Embedded Toolchain (must be installed separately)
#
# See also: CY_COMPILER_PATH below
TOOLCHAIN=GCC_ARM
# But for build the CM55 project, we need to use the LLVM toolchain. See lib/mtb-psoc-edge-libs/mtb-makefile.mk - mtb_build_m55

# Default build configuration. Options include:
#
# Debug -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
# Custom -- build with custom configuration, set the optimization flag in CFLAGS
# 
# If CONFIG is manually edited, ensure to update or regenerate 
# launch configurations for your IDE.
CONFIG=Debug

# Config file for postbuild sign and merge operations.
# NOTE: Check the JSON file for the command parameters
COMBINE_SIGN_JSON?=configs/boot_with_extended_boot.json


# Option to enable the DEEPCRAFT Audio Enhancement.
#
# ENABLED   - use the audio-enhancement to filter the audio input stream before
#             executing the voice-assistant process function.
# DISABLED  - use raw audio input stream from microphone to the feed the 
#             voice assistant process function.
USE_AUDIO_ENHANCEMENT=ENABLED

# Option to use FULL or LIMITED version of the Audio Voice Core library
#
# LIMITED - Limited time functionality (Default)
# FULL    - Full functionality, no time limit
#
CONFIG_VOICE_CORE_MODE=LIMITED

# Set the name of the project created in DEEPCRAFT Voice Assistant cloud tool 
# and placed in the va_models/ folder.
# Alternatively, use one of the projects below that comes with this code example
#
# Smart_Lights_Demo -- use the smart lights demo model (default)
# Chess             -- use the chess game voice control model
DEEPCRAFT_PROJECT_NAME=Chess

include ../common_app.mk
