################################################################################
# \file config.mk
# \version 1.0
#
# \brief
# Holds configuration and error checking that are common to both the Bootloader
# and Blinky apps. Ensure that this file is included in the application's 
# Makefile after other application-specific variables such as TARGET, 
# USE_EXT_FLASH are defined. 
#
################################################################################
# \copyright
# Copyright 2020 Cypress Semiconductor Corporation
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

# Path to Amazon FreeRTOS root directory
CY_AFR_ROOT=../../../..

################################################################################
# MCUboot Specific Configuration
###############################################################################

# Set to 1 when MCUboot needs to support external flash for the secondary slot.
# Default location is external flash.
USE_EXT_FLASH ?= 1

# Number of images supported in case of multi-image bootloading. 
# This application supports only 1 image.
NUMBER_OF_IMAGES = 1

# NOTE: Following variables are passed as options to the linker. 
# Ensure that the values have no trailing white space. Linker will throw error
# otherwise. 

# The bootloader app is run by CM0+. Therefore, the scratch size and the
# bootloader size are used with the linker script for the bootloader app. 
# The slot sizes (primary_1, secondary_1, primary_2, and secondary_2) are used
# with the linker script for the OTA app run by CM4. 

# Flash and RAM size for MCUBoot Bootloader app run by CM0+
BOOTLOADER_APP_FLASH_SIZE=0x18000
BOOTLOADER_APP_RAM_SIZE=0x20000

# Flash size for the OTA App run by CM4
# Slot size includes the flash size of the OTA App run by CM4
# One slot = MCUboot Header + App + TLV + Trailer (Trailer is not present for BOOT image)
# Scratch memory refers to the section of memory which is used to swap the contents between primary and secondary slot.
# The RAM size of the OTA App is set in the linker script as given below. 
# CM4 RAM size = Total RAM available in the device - 2 KB reserved for system use - BOOTLOADER_APP_RAM_SIZE. 
ifeq ($(USE_EXT_FLASH), 1)
MCUBOOT_SLOT_SIZE=0x001C0000
MAX_IMG_SECTORS=3584 # 1.75 Mb max app size
else
MCUBOOT_SLOT_SIZE=0x000F3800 
MAX_IMG_SECTORS=2000 # 1 Mb max app size
endif
MCUBOOT_SCRATCH_SIZE=0x1000

# MCUBoot header size
# Header size is used in two places. 
# 1. The location of CM4 image is offset by the header size from the ORIGIN
# value specified in the linker script. 
# 2. Passed to the imgtool while signing the image. The imgtool fills the space
# of this size with zeroes and then adds the actual header starting from address
# zero. 
MCUBOOT_HEADER_SIZE=0x400

# Add define to pick the custom flash map defined in
# bootloader_cm0p/ext_flash_map.c.
DEFINES+=CY_FLASH_MAP_EXT_DESC
