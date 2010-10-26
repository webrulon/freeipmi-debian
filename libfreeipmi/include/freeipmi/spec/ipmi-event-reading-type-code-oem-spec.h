/*
   Copyright (C) 2003-2010 FreeIPMI Core Team

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#ifndef _IPMI_EVENT_READING_TYPE_CODE_OEM_SPEC_H
#define _IPMI_EVENT_READING_TYPE_CODE_OEM_SPEC_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************
 * Dell                                    *
 *******************************************/

/*
 * Dell Poweredge R610
 * Dell Poweredge R710
 */

/* achu: not official names, named based on use context */

/* achu: the strings are all over the place, "status" is the best name
 * I can think of
 */
#define IPMI_EVENT_READING_TYPE_CODE_OEM_DELL_STATUS                    0x70

/* achu: names taken from code, are correct names? */
#define IPMI_EVENT_READING_TYPE_CODE_OEM_DELL_OEM_DIAGNOSTIC_EVENT_DATA 0x7E

/*******************************************
 * Intel                                   *
 *******************************************/

/*
 * Intel S5500WB/Penguin Computing Relion 700
 */

#define IPMI_EVENT_READING_TYPE_CODE_OEM_INTEL_PCIE_FATAL_SENSOR       0x70
#define IPMI_EVENT_READING_TYPE_CODE_OEM_INTEL_PCIE_CORRECTABLE_SENSOR 0x71
#define IPMI_EVENT_READING_TYPE_CODE_OEM_INTEL_QPI_CORRECTABLE_SENSOR  0x72
#define IPMI_EVENT_READING_TYPE_CODE_OEM_INTEL_QPI_NON_FATAL_SENSOR    0x73
#define IPMI_EVENT_READING_TYPE_CODE_OEM_INTEL_QPI_FATAL_SENSOR        0x74

/*
 * Intel Node Manager
 *
 * http://download.intel.com/support/motherboards/server/s5500wb/sb/s5500wb_tps_1_0.pdf
 * 
 * For Intel Chips, not just Intel Motherboards.  Confirmed for:
 *
 * Intel S5500WB/Penguin Computing Relion 700
 * Inventec 5441/Dell Xanadu II
 * Inventec 5442/Dell Xanadu III
 * Quanta S99Q/Dell FS12-TY
 */

#define IPMI_EVENT_READING_TYPE_CODE_OEM_INTEL_NODE_MANAGER_EXCEPTION_EVENT                       0x72
#define IPMI_EVENT_READING_TYPE_CODE_OEM_INTEL_NODE_MANAGER_HEALTH_EVENT                          0x73
#define IPMI_EVENT_READING_TYPE_CODE_OEM_INTEL_NODE_MANAGER_OPERATIONAL_CAPABILITIES_CHANGE_EVENT 0x74
#define IPMI_EVENT_READING_TYPE_CODE_OEM_INTEL_NODE_MANAGER_ALERT_THRESHOLD_EXCEEDED              0x72
#define IPMI_EVENT_READING_TYPE_CODE_OEM_INTEL_SERVER_PLATFORM_SERVICES_FIRMWARE_HEALTH           0x75

/*******************************************
 * Inventec                                *
 *******************************************/

/*
 * Inventec 5441/Dell Xanadu II
 * Inventec 5442/Dell Xanadu III
 */
/* achu: not official names, named based on use context */
#define IPMI_EVENT_READING_TYPE_CODE_OEM_INVENTEC_BIOS 0x70

/*******************************************
 * Supermicro                              *
 *******************************************/

/*
 * Supermicro X8DTH
 */

/* achu: not official names, named based on use context */
#define IPMI_EVENT_READING_TYPE_CODE_OEM_SUPERMICRO_GENERIC 0x70

#ifdef __cplusplus
}
#endif

#endif /* ipmi-event-reading-type-code-oem-spec.h */

