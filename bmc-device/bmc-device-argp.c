/*
 * Copyright (C) 2008-2012 FreeIPMI Core Team
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#if HAVE_ARGP_H
#include <argp.h>
#else /* !HAVE_ARGP_H */
#include "freeipmi-argp.h"
#endif /* !HAVE_ARGP_H */

#include "bmc-device.h"
#include "bmc-device-argp.h"

#include "freeipmi-portability.h"
#include "tool-cmdline-common.h"
#include "tool-config-file-common.h"

const char *argp_program_version =
  "bmc-device - " PACKAGE_VERSION "\n"
  "Copyright (C) 2008-2012 FreeIPMI Core Team\n"
  "This program is free software; you may redistribute it under the terms of\n"
  "the GNU General Public License.  This program has absolutely no warranty.";

const char *argp_program_bug_address =
  "<" PACKAGE_BUGREPORT ">";

static char cmdline_doc[] =
  "bmc-device - perform advanced BMC commands";

static char cmdline_args_doc[] = "";

static struct argp_option cmdline_options[] =
  {
    ARGP_COMMON_OPTIONS_DRIVER,
    ARGP_COMMON_OPTIONS_INBAND,
    ARGP_COMMON_OPTIONS_OUTOFBAND_HOSTRANGED,
    ARGP_COMMON_OPTIONS_AUTHENTICATION_TYPE,
    ARGP_COMMON_OPTIONS_CIPHER_SUITE_ID,
    ARGP_COMMON_OPTIONS_PRIVILEGE_LEVEL,
    ARGP_COMMON_OPTIONS_CONFIG_FILE,
    ARGP_COMMON_OPTIONS_WORKAROUND_FLAGS,
    ARGP_COMMON_SDR_OPTIONS,
    ARGP_COMMON_HOSTRANGED_OPTIONS,
    ARGP_COMMON_OPTIONS_DEBUG,
    { "cold-reset", COLD_RESET_KEY, NULL, 0,
      "Perform a cold reset.", 30},
    { "warm-reset", WARM_RESET_KEY, NULL, 0,
      "Perform a warm reset.", 31},
    { "get-self-test-results", GET_SELF_TEST_RESULTS_KEY, NULL, 0,
      "Output BMC self test results.", 32},
    { "get-acpi-power-state", GET_ACPI_POWER_STATE_KEY, NULL, 0,
      "Get ACPI system and device power state.", 33},
    { "set-acpi-power-state", SET_ACPI_POWER_STATE_KEY, NULL, 0,
      "Set ACPI power state.", 34},
    { "set-acpi-system-power-state", SET_ACPI_SYSTEM_POWER_STATE_KEY, "SYSTEM_POWER_STATE", 0,
      "Set ACPI system power state.", 35},
    { "set-acpi-device-power-state", SET_ACPI_DEVICE_POWER_STATE_KEY, "DEVICE_POWER_STATE", 0,
      "Set ACPI device power state.", 36},
    { "get-lan-statistics", GET_LAN_STATISTICS_KEY, NULL, 0,
      "Get IP, UDP, and RMCP statistics.", 37},
    { "clear-lan-statistics", CLEAR_LAN_STATISTICS_KEY, NULL, 0,
      "Clear IP, UDP, and RMCP statistics.", 38},
    { "rearm-sensor", REARM_SENSOR_KEY, "<record_id> [<assertion_bitmask> <deassertion_bitmask>]", 0,
      "Re-arm a sensor.", 39},
    { "get-sdr-repository-time",   GET_SDR_REPOSITORY_TIME_KEY,  0, 0,
      "Get SDR repository time.", 40},
    { "set-sdr-repository-time",   SET_SDR_REPOSITORY_TIME_KEY,  "TIME", 0,
      "Set SDR repository time.  Input format = \"MM/DD/YYYY - HH:MM:SS\" or \"now\".", 41},
    { "get-sel-time", GET_SEL_TIME_KEY,  0, 0,
      "Get SEL time.", 42},
    { "set-sel-time", SET_SEL_TIME_KEY,  "TIME", 0,
      "Set SEL time.  Input format = \"MM/DD/YYYY - HH:MM:SS\" or \"now\".", 43},
    { "platform-event", PLATFORM_EVENT_KEY, "[generator_id] <event_message_format_version> <sensor_type> <sensor_number> <event_type> <event_direction> <event_data1> <event_data2> <event_data3>", 0,
      "Instruct the BMC to process the specified event data.", 44},
    { "get-mca-auxiliary-log-status", GET_MCA_AUXILIARY_LOG_STATUS_KEY, NULL, 0,
      "Get machine check architecture (MCA) auxiliary log status information.", 45},
    { "get-ssif-interface-capabilities", GET_SSIF_INTERFACE_CAPABILITIES_KEY, NULL, 0,
      "Get SSIF interface capabilities.", 46},
    { "get-kcs-interface-capabilities", GET_KCS_INTERFACE_CAPABILITIES_KEY, NULL, 0,
      "Get KCS interface capabilities.", 47},
    { "get-bt-interface-capabilities", GET_BT_INTERFACE_CAPABILITIES_KEY, NULL, 0,
      "Get BT interface capabilities.", 48},
    { "get-bmc-global-enables", GET_BMC_GLOBAL_ENABLES_KEY, NULL, 0,
      "Get BMC Global Enables.", 49},
    { "set-system-firmware-version", SET_SYSTEM_FIRMWARE_VERSION_KEY, "STRING", 0,
      "Set System Firmware Version.", 50},
    { "set-system-name", SET_SYSTEM_NAME_KEY, "STRING", 0,
      "Set System Name.", 51},
    { "set-primary-operating-system-name", SET_PRIMARY_OPERATING_SYSTEM_NAME_KEY, "STRING", 0,
      "Set Primary Operating System Name.", 52},
    { "set-operating-system-name", SET_OPERATING_SYSTEM_NAME_KEY, "STRING", 0,
      "Set Operating System Name.", 53},
    { "verbose", VERBOSE_KEY, 0, 0,
      "Increase verbosity in output.", 54},
    { NULL, 0, NULL, 0, NULL, 0}
  };

static error_t cmdline_parse (int key, char *arg, struct argp_state *state);

static struct argp cmdline_argp = { cmdline_options,
                                    cmdline_parse,
                                    cmdline_args_doc,
                                    cmdline_doc };

static struct argp cmdline_config_file_argp = { cmdline_options,
                                                cmdline_config_file_parse,
                                                cmdline_args_doc,
                                                cmdline_doc };

static error_t
cmdline_parse (int key, char *arg, struct argp_state *state)
{
  struct bmc_device_arguments *cmd_args = state->input;
  error_t ret;

  switch (key)
    {
    case COLD_RESET_KEY:
      cmd_args->cold_reset++;
      break;
    case WARM_RESET_KEY:
      cmd_args->warm_reset++;
      break;
    case GET_SELF_TEST_RESULTS_KEY:
      cmd_args->get_self_test_results++;
      break;
    case GET_ACPI_POWER_STATE_KEY:
      cmd_args->get_acpi_power_state++;
      break;
    case SET_ACPI_POWER_STATE_KEY:
      cmd_args->set_acpi_power_state++;
      break;
    case SET_ACPI_SYSTEM_POWER_STATE_KEY:
      if (!strcasecmp (arg, "S0") /* acceptable here */
          || !strcasecmp (arg, "G0") /* acceptable here */
          || !strcasecmp (arg, "S0_G0")
          || !strcasecmp (arg, "S0/G0")) /* just in case */
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_S0_G0;
      else if (!strcasecmp (arg, "S1"))
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_S1;
      else if (!strcasecmp (arg, "S2"))
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_S2;
      else if (!strcasecmp (arg, "S3"))
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_S3;
      else if (!strcasecmp (arg, "S4"))
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_S4;
      else if (!strcasecmp (arg, "G2") /* acceptable here */
               || !strcasecmp (arg, "S5_G2")
               || !strcasecmp (arg, "S5/G2")) /* just in case */
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_S5_G2;
      else if (!strcasecmp (arg, "S4_S5")
               || !strcasecmp (arg, "S4/S5")) /* just in case */
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_S4_S5;
      else if (!strcasecmp (arg, "G3"))
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_G3;
      else if (!strcasecmp (arg, "SLEEPING"))
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_SLEEPING;
      else if (!strcasecmp (arg, "G1_SLEEPING"))
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_G1_SLEEPING;
      else if (!strcasecmp (arg, "OVERRIDE"))
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_OVERRIDE;
      else if (!strcasecmp (arg, "LEGACY_ON"))
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_LEGACY_ON;
      else if (!strcasecmp (arg, "LEGACY_OFF"))
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_LEGACY_OFF;
      else if (!strcasecmp (arg, "UNKNOWN"))
        cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_UNKNOWN;
      else
        {
          fprintf (stderr, "invalid value for system power state\n");
          exit (1);
        }
      break;
    case SET_ACPI_DEVICE_POWER_STATE_KEY:
      if (!strcasecmp (arg, "D0"))
        cmd_args->set_acpi_power_state_args.device_power_state = IPMI_ACPI_DEVICE_POWER_STATE_D0;
      else if (!strcasecmp (arg, "D1"))
        cmd_args->set_acpi_power_state_args.device_power_state = IPMI_ACPI_DEVICE_POWER_STATE_D1;
      else if (!strcasecmp (arg, "D2"))
        cmd_args->set_acpi_power_state_args.device_power_state = IPMI_ACPI_DEVICE_POWER_STATE_D2;
      else if (!strcasecmp (arg, "D3"))
        cmd_args->set_acpi_power_state_args.device_power_state = IPMI_ACPI_DEVICE_POWER_STATE_D3;
      else if (!strcasecmp (arg, "UNKNOWN"))
        cmd_args->set_acpi_power_state_args.device_power_state = IPMI_ACPI_DEVICE_POWER_STATE_UNKNOWN;
      else
        {
          fprintf (stderr, "invalid value for device power state\n");
          exit (1);
        }
      break;
    case GET_LAN_STATISTICS_KEY:
      cmd_args->get_lan_statistics++;
      break;
    case CLEAR_LAN_STATISTICS_KEY:
      cmd_args->clear_lan_statistics++;
      break;
    case REARM_SENSOR_KEY:
      cmd_args->rearm_sensor = 1;
      cmd_args->rearm_sensor_arg = arg;
      break;
    case GET_SDR_REPOSITORY_TIME_KEY:
      cmd_args->get_sdr_repository_time = 1;
      break;
    case SET_SDR_REPOSITORY_TIME_KEY:
      cmd_args->set_sdr_repository_time = 1;
      cmd_args->set_sdr_repository_time_arg = arg;
      break;
    case GET_SEL_TIME_KEY:
      cmd_args->get_sel_time = 1;
      break;
    case SET_SEL_TIME_KEY:
      cmd_args->set_sel_time = 1;
      cmd_args->set_sel_time_arg = arg;
      break;
    case PLATFORM_EVENT_KEY:
      cmd_args->platform_event = 1;
      cmd_args->platform_event_arg = arg;
      break;
    case GET_MCA_AUXILIARY_LOG_STATUS_KEY:
      cmd_args->get_mca_auxiliary_log_status = 1;
      break;
    case GET_SSIF_INTERFACE_CAPABILITIES_KEY:
      cmd_args->get_ssif_interface_capabilities = 1;
      break;
    case GET_KCS_INTERFACE_CAPABILITIES_KEY:
      cmd_args->get_kcs_interface_capabilities = 1;
      break;
    case GET_BT_INTERFACE_CAPABILITIES_KEY:
      cmd_args->get_bt_interface_capabilities = 1;
      break;
    case GET_BMC_GLOBAL_ENABLES_KEY:
      cmd_args->get_bmc_global_enables = 1;
      break;
    case SET_SYSTEM_FIRMWARE_VERSION_KEY:
      cmd_args->set_system_firmware_version = 1;
      cmd_args->set_system_firmware_version_arg = arg;
      break;
    case SET_SYSTEM_NAME_KEY:
      cmd_args->set_system_name = 1;
      cmd_args->set_system_name_arg = arg;
      break;
    case SET_PRIMARY_OPERATING_SYSTEM_NAME_KEY:
      cmd_args->set_primary_operating_system_name = 1;
      cmd_args->set_primary_operating_system_name_arg = arg;
      break;
    case SET_OPERATING_SYSTEM_NAME_KEY:
      cmd_args->set_operating_system_name = 1;
      cmd_args->set_operating_system_name_arg = arg;
      break;
    case VERBOSE_KEY:
      cmd_args->verbose++;
      break;
    case ARGP_KEY_ARG:
      /* Too many arguments. */
      argp_usage (state);
      break;
    case ARGP_KEY_END:
      break;
    default:
      ret = common_parse_opt (key, arg, &(cmd_args->common));
      if (ret == ARGP_ERR_UNKNOWN)
	ret = sdr_parse_opt (key, arg, &(cmd_args->sdr));
      if (ret == ARGP_ERR_UNKNOWN)
        ret = hostrange_parse_opt (key, arg, &(cmd_args->hostrange));
      return (ret);
    }

  return (0);
}

static void
_bmc_device_config_file_parse (struct bmc_device_arguments *cmd_args)
{
  if (config_file_parse (cmd_args->common.config_file,
                         0,
                         &(cmd_args->common),
                         &(cmd_args->sdr),
                         &(cmd_args->hostrange),
                         CONFIG_FILE_INBAND | CONFIG_FILE_OUTOFBAND | CONFIG_FILE_SDR | CONFIG_FILE_HOSTRANGE,
                         CONFIG_FILE_TOOL_BMC_DEVICE,
                         NULL) < 0)
    {
      fprintf (stderr, "config_file_parse: %s\n", strerror (errno));
      exit (1);
    }
}

static void
_bmc_device_args_validate (struct bmc_device_arguments *cmd_args)
{
  if (!cmd_args->sdr.flush_cache
      && !cmd_args->cold_reset
      && !cmd_args->warm_reset
      && !cmd_args->get_self_test_results
      && !cmd_args->get_acpi_power_state
      && !cmd_args->set_acpi_power_state
      && !cmd_args->get_lan_statistics
      && !cmd_args->clear_lan_statistics
      && !cmd_args->rearm_sensor
      && !cmd_args->get_sdr_repository_time
      && !cmd_args->set_sdr_repository_time
      && !cmd_args->get_sel_time
      && !cmd_args->set_sel_time
      && !cmd_args->platform_event
      && !cmd_args->get_mca_auxiliary_log_status
      && !cmd_args->get_ssif_interface_capabilities
      && !cmd_args->get_kcs_interface_capabilities
      && !cmd_args->get_bt_interface_capabilities
      && !cmd_args->get_bmc_global_enables
      && !cmd_args->set_system_firmware_version
      && !cmd_args->set_system_name
      && !cmd_args->set_primary_operating_system_name
      && !cmd_args->set_operating_system_name)
    {
      fprintf (stderr,
               "No command specified.\n");
      exit (1);
    }

  if ((cmd_args->sdr.flush_cache
       + cmd_args->cold_reset
       + cmd_args->warm_reset
       + cmd_args->get_self_test_results
       + cmd_args->get_acpi_power_state
       + cmd_args->set_acpi_power_state
       + cmd_args->get_lan_statistics
       + cmd_args->clear_lan_statistics
       + cmd_args->rearm_sensor
       + cmd_args->get_sdr_repository_time
       + cmd_args->set_sdr_repository_time
       + cmd_args->get_sel_time
       + cmd_args->set_sel_time
       + cmd_args->platform_event
       + cmd_args->get_mca_auxiliary_log_status
       + cmd_args->get_ssif_interface_capabilities
       + cmd_args->get_kcs_interface_capabilities
       + cmd_args->get_bt_interface_capabilities
       + cmd_args->get_bmc_global_enables
       + cmd_args->set_system_firmware_version
       + cmd_args->set_system_name
       + cmd_args->set_primary_operating_system_name
       + cmd_args->set_operating_system_name) > 1)
    
    {
      fprintf (stderr,
               "Multiple commands specified.\n");
      exit (1);
    }
  
  if (cmd_args->set_acpi_power_state
      && (cmd_args->set_acpi_power_state_args.system_power_state == IPMI_ACPI_SYSTEM_POWER_STATE_NO_CHANGE
          && cmd_args->set_acpi_power_state_args.device_power_state == IPMI_ACPI_DEVICE_POWER_STATE_NO_CHANGE))
    {
      fprintf (stderr,
               "No acpi power state configuration changes specified\n");
      exit (1);
    }

  if (cmd_args->set_system_firmware_version
      && strlen (cmd_args->set_system_firmware_version_arg) > IPMI_SYSTEM_INFO_STRING_LEN_MAX)
    {
      fprintf (stderr,
	       "system firmware version string too long\n");
      exit (1);
    }
  
  if (cmd_args->set_system_name
      && strlen (cmd_args->set_system_name_arg) > IPMI_SYSTEM_INFO_STRING_LEN_MAX)
    {
      fprintf (stderr,
	       "system name string too long\n");
      exit (1);
    }
  
  if (cmd_args->set_primary_operating_system_name
      && strlen (cmd_args->set_primary_operating_system_name_arg) > IPMI_SYSTEM_INFO_STRING_LEN_MAX)
    {
      fprintf (stderr,
	       "primary operating system name string too long\n");
      exit (1);
    }
  
  if (cmd_args->set_operating_system_name
      && strlen (cmd_args->set_operating_system_name_arg) > IPMI_SYSTEM_INFO_STRING_LEN_MAX)
    {
      fprintf (stderr,
	       "operating system name string too long\n");
      exit (1);
    }
}

void
bmc_device_argp_parse (int argc, char **argv, struct bmc_device_arguments *cmd_args)
{
  init_common_cmd_args_admin (&(cmd_args->common));
  init_sdr_cmd_args (&(cmd_args->sdr));
  init_hostrange_cmd_args (&(cmd_args->hostrange));

  cmd_args->cold_reset = 0;
  cmd_args->warm_reset = 0;
  cmd_args->get_self_test_results = 0;
  cmd_args->get_acpi_power_state = 0;
  cmd_args->set_acpi_power_state = 0;
  cmd_args->set_acpi_power_state_args.system_power_state = IPMI_ACPI_SYSTEM_POWER_STATE_NO_CHANGE;
  cmd_args->set_acpi_power_state_args.device_power_state = IPMI_ACPI_DEVICE_POWER_STATE_NO_CHANGE;
  cmd_args->get_lan_statistics = 0;
  cmd_args->clear_lan_statistics = 0;
  cmd_args->rearm_sensor = 0;
  cmd_args->rearm_sensor_arg = NULL;
  cmd_args->get_sdr_repository_time = 0;
  cmd_args->set_sdr_repository_time = 0;
  cmd_args->set_sdr_repository_time_arg = NULL;
  cmd_args->get_sel_time = 0;
  cmd_args->set_sel_time = 0;
  cmd_args->set_sel_time_arg = NULL;
  cmd_args->platform_event = 0;
  cmd_args->platform_event_arg = NULL;
  cmd_args->get_mca_auxiliary_log_status = 0;
  cmd_args->get_ssif_interface_capabilities = 0;
  cmd_args->get_kcs_interface_capabilities = 0;
  cmd_args->get_bt_interface_capabilities = 0;
  cmd_args->get_bmc_global_enables = 0;
  cmd_args->set_system_firmware_version = 0;
  cmd_args->set_system_firmware_version_arg = NULL;
  cmd_args->set_system_name = 0;
  cmd_args->set_system_name_arg = NULL;
  cmd_args->set_primary_operating_system_name = 0;
  cmd_args->set_primary_operating_system_name_arg = NULL;
  cmd_args->set_operating_system_name = 0;
  cmd_args->set_operating_system_name_arg = NULL;
  cmd_args->verbose = 0;

  argp_parse (&cmdline_config_file_argp,
              argc,
              argv,
              ARGP_IN_ORDER,
              NULL,
              &(cmd_args->common));

  _bmc_device_config_file_parse (cmd_args);

  argp_parse (&cmdline_argp,
              argc,
              argv,
              ARGP_IN_ORDER,
              NULL,
              cmd_args);

  verify_common_cmd_args (&(cmd_args->common));
  verify_hostrange_cmd_args (&(cmd_args->hostrange));
  _bmc_device_args_validate (cmd_args);
}
