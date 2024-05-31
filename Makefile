################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Top-level application make file.
#
################################################################################
# \copyright
# Copyright 2020-2024, Cypress Semiconductor Corporation (an Infineon company)
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


################################################################################
# Basic Configuration
################################################################################

#Type of MTB Makefile Options include:
#
#COMBINED    -- Top Level Makefile usually for single standalone application
#APPLICATION -- Top Level Makefile usually for multi project application
#PROJECT     -- Project Makefile under Application
#
MTB_TYPE=COMBINED

# Target board/hardware
TARGET=APP_CYW955913EVK-01

# Name of application (used to derive name of final linked file).
APPNAME=mtb-tester-threadx-wifi-bluetooth

# Name of toolchain to use. Options include:
#
# GCC_ARM -- GCC 11.3.1, provided with ModusToolbox IDE
# ARM     -- ARM Compiler (must be installed separately)
# IAR     -- IAR Compiler (must be installed separately)
#
# See also: CY_COMPILER_PATH below
TOOLCHAIN=GCC_ARM

# Default build configuration. Options include:
#
# Debug   -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
CONFIG=Debug

# If set to "true" or "1", display full command-lines when building.
VERBOSE=0

################################################################################
# Advanced Configuration
##############################################################################	##

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
COMPONENTS+=WCM WICED_BLE

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.
SOURCES= 

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
INCLUDES=

HEAP_SIZE=0x11800
# With XIP=0, the application runs from RAM. 
# Hence due to memory constraint, we can run only wifi+iperf or bluetooth at a time.
# With XIP=1, the application runs from flash.
# Hence we can run wifi+iperf and bluetooth together.
XIP=0

DEFINES+=WHD_PRINT_DISABLE
DEFINES+=TX_PACKET_POOL_SIZE=20
DEFINES+=RX_PACKET_POOL_SIZE=16
DEFINES+=WCM_WORKER_THREAD_STACK_SIZE=5120
DEFINES+=SECURE_SOCKETS_THREAD_STACKSIZE=1024
# The tcp window size is calculated by number of Rx buffers available
# total rx buffer available is RX_PACKET_POOL_SIZE(16) - MAX_EVENTBUF_POST(2)
# therefore tcp window size is ((14 - 1) * 1460)
DEFINES+=DEFAULT_TCP_WINDOW_SIZE=18980
DEFINES+=DEFAULT_IPERF_SERVER_TIMEOUT_SEC=10

# Add additional defines to the build process (without a leading -D).
DEFINES+=COMPONENT_WIFI_INTERFACE_OCI CYBSP_WIFI_CAPABLE HAVE_SNPRINTF CY_RTOS_AWARE

# Set CM33 clock frequency
DEFINES += H1CP_CLOCK_FREQ=96000000

DEFINES += DISABLE_COMMAND_CONSOLE_BT
ifeq ($(XIP),0)
ifeq ($(filter DISABLE_COMMAND_CONSOLE_BT, $(DEFINES)),)
ifeq ($(filter DISABLE_COMMAND_CONSOLE_WIFI, $(DEFINES)),)
$(error With XIP=0, either Wi-Fi or Bluetooth commands can be executed due to memory constraint. Add DISABLE_COMMAND_CONSOLE_BT or DISABLE_COMMAND_CONSOLE_WIFI to DEFINES to continue the build. To run both Wi-Fi and Bluetooth commands, set XIP=1.)
endif
ifeq ($(filter DISABLE_COMMAND_CONSOLE_IPERF, $(DEFINES)),)
$(error When Wi-Fi is disabled, iperf should also be disabled as network traffic cannot be run. Add DISABLE_COMMAND_CONSOLE_IPERF to DEFINES in Makefile.)
endif
endif
endif

# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT=

# Additional / custom C compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CFLAGS=

# Additional / custom C++ compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CXXFLAGS=

# Additional / custom assembler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ASFLAGS=

# Additional / custom linker flags.
LDFLAGS=

# Additional / custom libraries to link in to the application.
LDLIBS=

# Path to the linker script to use (if empty, use the default linker script).
LINKER_SCRIPT=

# Custom pre-build commands to run.
PREBUILD=

# Custom post-build commands to run.
POSTBUILD=

################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
#
# This controls where automatic source code discovery looks for code.
CY_APP_PATH=

# Relative path to the shared repo location.
#
# All .mtb files have the format, <URI><COMMIT><LOCATION>. If the <LOCATION> field
# begins with $$ASSET_REPO$$, then the repo is deposited in the path specified by
# the CY_GETLIBS_SHARED_PATH variable. The default location is one directory level
# above the current app directory.
# This is used with CY_GETLIBS_SHARED_NAME variable, which specifies the directory name.
CY_GETLIBS_SHARED_PATH=../

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared

# Absolute path to the compiler's "bin" directory.
#
# The default depends on the selected TOOLCHAIN (GCC_ARM uses the ModusToolbox
# IDE provided compiler by default).
CY_COMPILER_PATH=


# Locate ModusToolbox IDE helper tools folders in default installation
# locations for Windows, Linux, and macOS.
CY_WIN_HOME=$(subst \,/,$(USERPROFILE))
CY_TOOLS_PATHS ?= $(wildcard \
    $(CY_WIN_HOME)/ModusToolbox/tools_* \
    $(HOME)/ModusToolbox/tools_* \
    /Applications/ModusToolbox/tools_*)

# If you install ModusToolbox IDE in a custom location, add the path to its
# "tools_X.Y" folder (where X and Y are the version number of the tools
# folder).
CY_TOOLS_PATHS+=

# Default to the newest installed tools folder, or the users override (if it's
# found).
CY_TOOLS_DIR=$(lastword $(sort $(wildcard $(CY_TOOLS_PATHS))))

ifeq ($(CY_TOOLS_DIR),)
$(error Unable to find any of the available CY_TOOLS_PATHS -- $(CY_TOOLS_PATHS))
endif

$(info Tools Directory: $(CY_TOOLS_DIR))

include $(CY_TOOLS_DIR)/make/start.mk
