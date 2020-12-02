/******************************************************************************
* File Name:   ext_flash_map.c
*
* Description:
* This file defines the custom flash map required for Rollback using external
* flash memory. This flash  map is used when both Factory application and the
* secondary slots are placed in external memory.
*
*******************************************************************************
* (c) 2020, Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* ("Software"), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries ("Cypress") and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress's integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer of such
* system or application assumes all risk of such use and in doing so agrees to
* indemnify Cypress against all liability.
*******************************************************************************/

/* header file for flash configuration */
#include "flash_map_backend/flash_map_backend.h"
#include "sysflash.h"

/*******************************************************************************
* Macros
********************************************************************************/
/*
 * For now, we only support one flash device.
 *
 * Pick a random device ID for it that's unlikely to collide with
 * anything "real".
 */
#define FLASH_DEVICE_ID                         111
#define FLASH_MAP_ENTRY_MAGIC                   0xd00dbeef

#define FLASH_AREA_IMAGE_SECTOR_SIZE FLASH_AREA_IMAGE_SCRATCH_SIZE

#ifndef CY_BOOTLOADER_START_ADDRESS
#define CY_BOOTLOADER_START_ADDRESS             (0x10000000)
#endif

#ifndef CY_BOOT_INTERNAL_FLASH_ERASE_VALUE
/* This is the value of internal flash bytes after an erase */
#define CY_BOOT_INTERNAL_FLASH_ERASE_VALUE      (0x00)
#endif

#ifndef CY_BOOT_EXTERNAL_FLASH_ERASE_VALUE
/* This is the value of external flash bytes after an erase */
#define CY_BOOT_EXTERNAL_FLASH_ERASE_VALUE      (0xff)
#endif

/* All the macros defined here are originated from cy_flash_map.c in MCUBoot.
 * Please refer that file more details.
 */
#if defined(CY_FLASH_MAP_EXT_DESC)

static struct flash_area bootloader =
{
    .fa_id = FLASH_AREA_BOOTLOADER,
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
    .fa_off = CY_BOOTLOADER_START_ADDRESS,
    .fa_size = CY_BOOT_BOOTLOADER_SIZE
};

static struct flash_area primary_1 =
{
    .fa_id = FLASH_AREA_IMAGE_PRIMARY(0),
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
    .fa_off = CY_FLASH_BASE + CY_BOOT_BOOTLOADER_SIZE,
    .fa_size = CY_BOOT_PRIMARY_1_SIZE
};

#ifndef CY_BOOT_USE_EXTERNAL_FLASH
static struct flash_area secondary_1 =
{
    .fa_id = FLASH_AREA_IMAGE_SECONDARY(0),
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
    .fa_off = CY_FLASH_BASE +\
                CY_BOOT_BOOTLOADER_SIZE +\
                CY_BOOT_PRIMARY_1_SIZE,
    .fa_size = CY_BOOT_SECONDARY_1_SIZE
};
#else
static struct flash_area secondary_1 =
{
    .fa_id = FLASH_AREA_IMAGE_SECONDARY(0),
    .fa_device_id = FLASH_DEVICE_EXTERNAL_FLASH(CY_BOOT_EXTERNAL_DEVICE_INDEX),
    .fa_off = CY_SMIF_BASE_MEM_OFFSET,
    .fa_size = CY_BOOT_SECONDARY_1_SIZE
};
#endif /* CY_BOOT_USE_EXTERNAL_FLASH */
#if (MCUBOOT_IMAGE_NUMBER == 2) /* if dual-image */
static struct flash_area primary_2 =
{
    .fa_id = FLASH_AREA_IMAGE_PRIMARY(1),
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
    .fa_off = CY_FLASH_BASE +\
                CY_BOOT_BOOTLOADER_SIZE +\
                CY_BOOT_PRIMARY_1_SIZE +\
                CY_BOOT_SECONDARY_1_SIZE,
    .fa_size = CY_BOOT_PRIMARY_2_SIZE
};

static struct flash_area secondary_2 =
{
    .fa_id = FLASH_AREA_IMAGE_SECONDARY(1),
    /* it is for external flash memory
    .fa_device_id = FLASH_DEVICE_EXTERNAL_FLASH(CY_BOOT_EXTERNAL_DEVICE_INDEX), */
#ifndef CY_BOOT_USE_EXTERNAL_FLASH
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
    .fa_off = CY_FLASH_BASE +\
                CY_BOOT_BOOTLOADER_SIZE +\
                CY_BOOT_PRIMARY_1_SIZE +\
                CY_BOOT_SECONDARY_1_SIZE +\
                CY_BOOT_PRIMARY_2_SIZE,
#else
    .fa_device_id = FLASH_DEVICE_EXTERNAL_FLASH(CY_BOOT_EXTERNAL_DEVICE_INDEX),
    .fa_off = CY_SMIF_BASE_MEM_OFFSET + 0x40000,
#endif /* CY_BOOT_USE_EXTERNAL_FLASH */
    .fa_size = CY_BOOT_SECONDARY_2_SIZE
};
#endif /* MCUBOOT_IMAGE_NUMBER */

#ifdef MCUBOOT_SWAP_USING_SCRATCH
static struct flash_area scratch =
{
    .fa_id = FLASH_AREA_IMAGE_SCRATCH,
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
#if (MCUBOOT_IMAGE_NUMBER == 1) /* if single-image */

#ifndef CY_BOOT_USE_EXTERNAL_FLASH
    .fa_off = CY_FLASH_BASE +\
              CY_BOOT_BOOTLOADER_SIZE +\
              CY_BOOT_PRIMARY_1_SIZE +\
              CY_BOOT_SECONDARY_1_SIZE,
#else
    .fa_off = CY_FLASH_BASE +\
              CY_BOOT_BOOTLOADER_SIZE +\
              CY_BOOT_PRIMARY_1_SIZE,
#endif /* CY_BOOT_USE_EXTERNAL_FLASH */
#elif (MCUBOOT_IMAGE_NUMBER == 2) /* if dual-image */
#ifndef CY_BOOT_USE_EXTERNAL_FLASH
    .fa_off = CY_FLASH_BASE +\
              CY_BOOT_BOOTLOADER_SIZE +\
              CY_BOOT_PRIMARY_1_SIZE +\
              CY_BOOT_SECONDARY_1_SIZE +\
              CY_BOOT_PRIMARY_2_SIZE +\
              CY_BOOT_SECONDARY_2_SIZE,
#else
    .fa_off = CY_FLASH_BASE +\
              CY_BOOT_BOOTLOADER_SIZE +\
              CY_BOOT_PRIMARY_1_SIZE +\
              CY_BOOT_PRIMARY_2_SIZE,
#endif /* CY_BOOT_USE_EXTERNAL_FLASH */
#endif /* MCUBOOT_IMAGE_NUMBER */
    .fa_size = CY_BOOT_SCRATCH_SIZE
};
#endif /* MCUBOOT_SWAP_USING_SCRATCH */


struct flash_area *boot_area_descs[] =
{
    &bootloader,
    &primary_1,
    &secondary_1,
#if (MCUBOOT_IMAGE_NUMBER == 2) /* if dual-image */
    &primary_2,
    &secondary_2,
#endif /* MCUBOOT_IMAGE_NUMBER */
#ifdef MCUBOOT_SWAP_USING_SCRATCH
    &scratch,
#endif /* MCUBOOT_SWAP_USING_SCRATCH */
    NULL
};

#endif /* CY_FLASH_MAP_EXT_DESC */

