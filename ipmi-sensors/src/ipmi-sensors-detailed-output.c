/*
  Copyright (C) 2003-2010 FreeIPMI Core Team

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#include <assert.h>
#include <errno.h>

#include <freeipmi/freeipmi.h>

#include "ipmi-sensors.h"
#include "ipmi-sensors-output-common.h"
#include "ipmi-sensors-util.h"

#include "freeipmi-portability.h"
#include "pstdout.h"
#include "tool-sensor-common.h"

#define ALL_EVENT_MESSAGES_DISABLED "All Event Messages Disabled"
#define SENSOR_SCANNING_DISABLED    "Sensor Scanning Disabled"

#define IPMI_SENSORS_DEVICE_TYPE_BUFLEN  1024

#define IPMI_SENSORS_IANA_LEN            1024
#define IPMI_SENSORS_OEM_DATA_LEN        1024

static char *
_get_record_type_string (ipmi_sensors_state_data_t *state_data,
                         uint8_t record_type)
{
  assert (state_data);
  assert (state_data->prog_data->args->verbose_count >= 1);

  switch (record_type)
    {
    case IPMI_SDR_FORMAT_FULL_SENSOR_RECORD:
      return (IPMI_SDR_FORMAT_FULL_SENSOR_RECORD_NAME);
    case IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD:
      return (IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD_NAME);
    case IPMI_SDR_FORMAT_EVENT_ONLY_RECORD:
      return (IPMI_SDR_FORMAT_EVENT_ONLY_RECORD_NAME);
    case IPMI_SDR_FORMAT_ENTITY_ASSOCIATION_RECORD:
      return (IPMI_SDR_FORMAT_ENTITY_ASSOCIATION_RECORD_NAME);
    case IPMI_SDR_FORMAT_DEVICE_RELATIVE_ENTITY_ASSOCIATION_RECORD:
      return (IPMI_SDR_FORMAT_DEVICE_RELATIVE_ENTITY_ASSOCIATION_RECORD_NAME);
    case IPMI_SDR_FORMAT_GENERIC_DEVICE_LOCATOR_RECORD:
      return (IPMI_SDR_FORMAT_GENERIC_DEVICE_LOCATOR_RECORD_NAME);
    case IPMI_SDR_FORMAT_FRU_DEVICE_LOCATOR_RECORD:
      return (IPMI_SDR_FORMAT_FRU_DEVICE_LOCATOR_RECORD_NAME);
    case IPMI_SDR_FORMAT_MANAGEMENT_CONTROLLER_DEVICE_LOCATOR_RECORD:
      return (IPMI_SDR_FORMAT_MANAGEMENT_CONTROLLER_DEVICE_LOCATOR_RECORD_NAME);
    case IPMI_SDR_FORMAT_MANAGEMENT_CONTROLLER_CONFIRMATION_RECORD:
      return (IPMI_SDR_FORMAT_MANAGEMENT_CONTROLLER_CONFIRMATION_RECORD_NAME);
    case IPMI_SDR_FORMAT_BMC_MESSAGE_CHANNEL_INFO_RECORD:
      return (IPMI_SDR_FORMAT_BMC_MESSAGE_CHANNEL_INFO_RECORD_NAME);
    case IPMI_SDR_FORMAT_OEM_RECORD:
      return (IPMI_SDR_FORMAT_OEM_RECORD_NAME);
    default:
      break;
    }

  return "Unknown Record";
}

static int
_abbreviated_units_flag (ipmi_sensors_state_data_t *state_data)
{
  assert (state_data);

  if (state_data->prog_data->args->legacy_output)
    return (0);

  if (state_data->prog_data->args->non_abbreviated_units)
    return (0);

  return (1);
}

static int
_detailed_output_record_type_and_id (ipmi_sensors_state_data_t *state_data,
                                     uint8_t record_type,
                                     uint16_t record_id)
{
  assert (state_data);
  assert (state_data->prog_data->args->verbose_count >= 1);

  pstdout_printf (state_data->pstate,
                  "Record ID: %u\n",
                  record_id);

  if (state_data->prog_data->args->verbose_count >= 2)
    {
      char *record_type_str = _get_record_type_string (state_data, record_type);

      pstdout_printf (state_data->pstate,
                      "Record Type: %s (%Xh)\n",
                      record_type_str,
                      record_type);
    }

  return (0);
}

static int
_detailed_output_entity_id_and_instance (ipmi_sensors_state_data_t *state_data,
                                         char *entity_id_prefix,
                                         uint8_t entity_id,
                                         uint8_t entity_instance)
{
  assert (state_data);
  assert (state_data->prog_data->args->verbose_count >= 1);

  if (IPMI_ENTITY_ID_VALID (entity_id))
    pstdout_printf (state_data->pstate,
                    "%s%sEntity ID: %s (%u)\n",
                    (entity_id_prefix) ? entity_id_prefix : "",
                    (entity_id_prefix) ? " " : "",
                    ipmi_entity_ids[entity_id],
                    entity_id);
  else if (IPMI_ENTITY_ID_IS_CHASSIS_SPECIFIC (entity_id))
    pstdout_printf (state_data->pstate,
                    "%s%sEntity ID: Chassis Specific (%u)\n",
                    (entity_id_prefix) ? entity_id_prefix : "",
                    (entity_id_prefix) ? " " : "",
                    entity_id);
  else if (IPMI_ENTITY_ID_IS_BOARD_SET_SPECIFIC (entity_id))
    pstdout_printf (state_data->pstate,
                    "%s%sEntity ID: Board-Set Specific (%u)\n",
                    (entity_id_prefix) ? entity_id_prefix : "",
                    (entity_id_prefix) ? " " : "",
                    entity_id);
  else if (IPMI_ENTITY_ID_IS_OEM_SYSTEM_INTEGRATOR_DEFINED (entity_id))
    pstdout_printf (state_data->pstate,
                    "%s%sEntity ID: OEM System Integrator (%u)\n",
                    (entity_id_prefix) ? entity_id_prefix : "",
                    (entity_id_prefix) ? " " : "",
                    entity_id);
  else
    pstdout_printf (state_data->pstate,
                    "%s%sEntity ID: %u\n",
                    (entity_id_prefix) ? entity_id_prefix : "",
                    (entity_id_prefix) ? " " : "",
                    entity_id);

  pstdout_printf (state_data->pstate,
                  "%s%sEntity Instance: %u\n",
                  (entity_id_prefix) ? entity_id_prefix : "",
                  (entity_id_prefix) ? " " : "",
                  entity_instance);

  return (0);
}

static int
_detailed_output_header (ipmi_sensors_state_data_t *state_data,
                         const void *sdr_record,
                         unsigned int sdr_record_len,
			 uint8_t sensor_number,
                         uint8_t record_type,
                         uint16_t record_id)
{
  char id_string[IPMI_SDR_CACHE_MAX_ID_STRING + 1];
  uint8_t sensor_type;
  uint8_t event_reading_type_code;
  uint8_t sensor_owner_id_type, sensor_owner_id;
  uint8_t sensor_owner_lun, channel_number;
  uint8_t entity_id, entity_instance, entity_instance_type;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 1);

  if (ipmi_sdr_parse_sensor_type (state_data->sdr_parse_ctx,
                                  sdr_record,
                                  sdr_record_len,
                                  &sensor_type) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  if (ipmi_sdr_parse_event_reading_type_code (state_data->sdr_parse_ctx,
                                              sdr_record,
                                              sdr_record_len,
                                              &event_reading_type_code) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_event_reading_type_code: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  memset (id_string, '\0', IPMI_SDR_CACHE_MAX_ID_STRING + 1);

  if (ipmi_sdr_parse_id_string (state_data->sdr_parse_ctx,
                                sdr_record,
                                sdr_record_len,
                                id_string,
                                IPMI_SDR_CACHE_MAX_ID_STRING) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_id_string: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  if (ipmi_sdr_parse_sensor_owner_id (state_data->sdr_parse_ctx,
                                      sdr_record,
                                      sdr_record_len,
                                      &sensor_owner_id_type,
                                      &sensor_owner_id) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_owner_id: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  if (ipmi_sdr_parse_sensor_owner_lun (state_data->sdr_parse_ctx,
                                       sdr_record,
                                       sdr_record_len,
                                       &sensor_owner_lun,
                                       &channel_number) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_owner_lun: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  if (ipmi_sdr_parse_entity_id_instance_type (state_data->sdr_parse_ctx,
                                              sdr_record,
                                              sdr_record_len,
                                              &entity_id,
                                              &entity_instance,
                                              &entity_instance_type) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_entity_id_instance_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  if (_detailed_output_record_type_and_id (state_data,
                                           record_type,
                                           record_id) < 0)
    return (-1);

  pstdout_printf (state_data->pstate,
                  "ID String: %s\n",
                  id_string);
  pstdout_printf (state_data->pstate,
                  "Sensor Type: %s (%Xh)\n",
                  get_sensor_type_output_string (sensor_type),
                  sensor_type);
  pstdout_printf (state_data->pstate,
                  "Sensor Number: %u\n",
                  sensor_number);
  if (sensor_owner_id_type)
    pstdout_printf (state_data->pstate,
                    "System Software ID: %Xh\n",
                    sensor_owner_id);
  else
    pstdout_printf (state_data->pstate,
                    "IPMB Slave Address: %Xh\n",
                    sensor_owner_id);
  pstdout_printf (state_data->pstate,
                  "Sensor Owner ID: %Xh\n",
                  (sensor_owner_id << 1) | sensor_owner_id_type);
  pstdout_printf (state_data->pstate,
                  "Sensor Owner LUN: %Xh\n",
                  sensor_owner_lun);
  pstdout_printf (state_data->pstate,
                  "Channel Number: %Xh\n",
                  channel_number);

  if (_detailed_output_entity_id_and_instance (state_data,
                                               NULL,
                                               entity_id,
                                               entity_instance) < 0)
    return (-1);

  pstdout_printf (state_data->pstate,
                  "Entity Instance Type: %s\n",
                  (entity_instance_type == IPMI_SDR_PHYSICAL_ENTITY) ? "Physical Entity" : "Logical Container Entity");
  pstdout_printf (state_data->pstate,
                  "Event/Reading Type Code: %Xh\n",
                  event_reading_type_code);


  return (0);
}

static int
_detailed_output_thresholds (ipmi_sensors_state_data_t *state_data,
                             const void *sdr_record,
                             unsigned int sdr_record_len,
                             const char *sensor_units_str)
{
  double *lower_non_critical_threshold = NULL;
  double *lower_critical_threshold = NULL;
  double *lower_non_recoverable_threshold = NULL;
  double *upper_non_critical_threshold = NULL;
  double *upper_critical_threshold = NULL;
  double *upper_non_recoverable_threshold = NULL;
  int rv = -1;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (sensor_units_str);
  assert (state_data->prog_data->args->verbose_count >= 1);

  if (ipmi_sensors_get_thresholds (state_data,
                                   sdr_record,
                                   sdr_record_len,
                                   &lower_non_critical_threshold,
                                   &lower_critical_threshold,
                                   &lower_non_recoverable_threshold,
                                   &upper_non_critical_threshold,
                                   &upper_critical_threshold,
                                   &upper_non_recoverable_threshold) < 0)
    goto cleanup;

  /* don't output at all if there isn't atleast 1 threshold to output */
  if (!lower_critical_threshold
      && !upper_critical_threshold
      && !lower_non_critical_threshold
      && !upper_non_critical_threshold
      && !lower_non_recoverable_threshold
      && !upper_non_recoverable_threshold)
    {
      rv = 0;
      goto cleanup;
    }

  if (lower_critical_threshold)
    pstdout_printf (state_data->pstate,
                    "Lower Critical Threshold: %f %s\n",
                    *lower_critical_threshold,
                    sensor_units_str);
  else
    pstdout_printf (state_data->pstate,
                    "Lower Critical Threshold: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  if (upper_critical_threshold)
    pstdout_printf (state_data->pstate,
                    "Upper Critical Threshold: %f %s\n",
                    *upper_critical_threshold,
                    sensor_units_str);
  else
    pstdout_printf (state_data->pstate,
                    "Upper Critical Threshold: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  if (lower_non_critical_threshold)
    pstdout_printf (state_data->pstate,
                    "Lower Non-Critical Threshold: %f %s\n",
                    *lower_non_critical_threshold,
                    sensor_units_str);
  else
    pstdout_printf (state_data->pstate,
                    "Lower Non-Critical Threshold: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  if (upper_non_critical_threshold)
    pstdout_printf (state_data->pstate,
                    "Upper Non-Critical Threshold: %f %s\n",
                    *upper_non_critical_threshold,
                    sensor_units_str);
  else
    pstdout_printf (state_data->pstate,
                    "Upper Non-Critical Threshold: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  if (lower_non_recoverable_threshold)
    pstdout_printf (state_data->pstate,
                    "Lower Non-Recoverable Threshold: %f %s\n",
                    *lower_non_recoverable_threshold,
                    sensor_units_str);
  else
    pstdout_printf (state_data->pstate,
                    "Lower Non-Recoverable Threshold: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  if (upper_non_recoverable_threshold)
    pstdout_printf (state_data->pstate,
                    "Upper Non-Recoverable Threshold: %f %s\n",
                    *upper_non_recoverable_threshold,
                    sensor_units_str);
  else
    pstdout_printf (state_data->pstate,
                    "Upper Non-Recoverable Threshold: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  rv = 0;
 cleanup:
  if (lower_non_critical_threshold)
    free (lower_non_critical_threshold);
  if (lower_critical_threshold)
    free (lower_critical_threshold);
  if (lower_non_recoverable_threshold)
    free (lower_non_recoverable_threshold);
  if (upper_non_critical_threshold)
    free (upper_non_critical_threshold);
  if (upper_critical_threshold)
    free (upper_critical_threshold);
  if (upper_non_recoverable_threshold)
    free (upper_non_recoverable_threshold);
  return (rv);
}

static int
_detailed_output_sensor_reading_ranges (ipmi_sensors_state_data_t *state_data,
                                        const void *sdr_record,
                                        unsigned int sdr_record_len,
                                        const char *sensor_units_str)
{
  uint8_t nominal_reading_specified = 0;
  uint8_t normal_maximum_specified = 0;
  uint8_t normal_minimum_specified = 0;
  double *nominal_reading = NULL;
  double *normal_maximum = NULL;
  double *normal_minimum = NULL;
  double *sensor_maximum_reading = NULL;
  double *sensor_minimum_reading = NULL;
  int rv = -1;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (sensor_units_str);
  assert (state_data->prog_data->args->verbose_count >= 1);

  if (ipmi_sdr_parse_sensor_reading_ranges_specified (state_data->sdr_parse_ctx,
                                                      sdr_record,
                                                      sdr_record_len,
                                                      &nominal_reading_specified,
                                                      &normal_maximum_specified,
                                                      &normal_minimum_specified) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_reading_ranges_specified: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      goto cleanup;
    }

  if (ipmi_sdr_parse_sensor_reading_ranges (state_data->sdr_parse_ctx,
                                            sdr_record,
                                            sdr_record_len,
                                            (nominal_reading_specified) ? &nominal_reading : NULL,
                                            (normal_maximum_specified) ? &normal_maximum : NULL,
                                            (normal_minimum_specified) ? &normal_minimum : NULL,
                                            &sensor_maximum_reading,
                                            &sensor_minimum_reading) < 0)
    {
      if (ipmi_sdr_parse_ctx_errnum (state_data->sdr_parse_ctx) == IPMI_SDR_PARSE_ERR_CANNOT_PARSE_OR_CALCULATE)
        {
          rv = 0;
          goto cleanup;
        }

      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_reading_ranges: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      goto cleanup;
    }

  /* don't output at all if there isn't atleast 1 threshold to output */
  if (!sensor_minimum_reading
      && !sensor_maximum_reading
      && !normal_minimum
      && !normal_maximum
      && !nominal_reading)
    {
      rv = 0;
      goto cleanup;
    }

  if (sensor_minimum_reading)
    pstdout_printf (state_data->pstate,
                    "Sensor Min. Reading: %f %s\n",
                    *sensor_minimum_reading,
                    sensor_units_str);
  else
    pstdout_printf (state_data->pstate,
                    "Sensor Min. Reading: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  if (sensor_maximum_reading)
    pstdout_printf (state_data->pstate,
                    "Sensor Max. Reading: %f %s\n",
                    *sensor_maximum_reading,
                    sensor_units_str);
  else
    pstdout_printf (state_data->pstate,
                    "Sensor Max. Reading: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  if (normal_minimum)
    pstdout_printf (state_data->pstate,
                    "Normal Min.: %f %s\n",
                    *normal_minimum,
                    sensor_units_str);
  else
    pstdout_printf (state_data->pstate,
                    "Normal Min.: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  if (normal_maximum)
    pstdout_printf (state_data->pstate,
                    "Normal Max.: %f %s\n",
                    *normal_maximum,
                    sensor_units_str);
  else
    pstdout_printf (state_data->pstate,
                    "Normal Max.: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  if (nominal_reading)
    pstdout_printf (state_data->pstate,
                    "Nominal Reading: %f %s\n",
                    *nominal_reading,
                    sensor_units_str);
  else
    pstdout_printf (state_data->pstate,
                    "Nominal Reading: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  rv = 0;
 cleanup:
  if (nominal_reading)
    free (nominal_reading);
  if (normal_maximum)
    free (normal_maximum);
  if (normal_minimum)
    free (normal_minimum);
  if (sensor_maximum_reading)
    free (sensor_maximum_reading);
  if (sensor_minimum_reading)
    free (sensor_minimum_reading);
  return (rv);
}

static int
_detailed_output_sensor_direction (ipmi_sensors_state_data_t *state_data,
                                   const void *sdr_record,
                                   unsigned int sdr_record_len)
{
  uint8_t sensor_direction = 0;
  int rv = -1;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (ipmi_sdr_parse_sensor_direction (state_data->sdr_parse_ctx,
                                       sdr_record,
                                       sdr_record_len,
                                       &sensor_direction) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_direction: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      goto cleanup;
    }

  if (sensor_direction == IPMI_SDR_SENSOR_DIRECTION_INPUT)
    pstdout_printf (state_data->pstate,
                    "Sensor Direction: Input\n");
  else if (sensor_direction == IPMI_SDR_SENSOR_DIRECTION_OUTPUT)
    pstdout_printf (state_data->pstate,
                    "Sensor Direction: Output\n");
  else
    pstdout_printf (state_data->pstate,
                    "Sensor Direction: Unspecified\n");

  rv = 0;
 cleanup:
  return (rv);
}

static int
_detailed_output_tolerance (ipmi_sensors_state_data_t *state_data,
                            const void *sdr_record,
                            unsigned int sdr_record_len,
                            const char *sensor_units_str)
{
  double *tolerance = NULL;
  int rv = -1;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (sensor_units_str);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (ipmi_sdr_parse_tolerance (state_data->sdr_parse_ctx,
                                sdr_record,
                                sdr_record_len,
                                &tolerance) < 0)
    {
      if (ipmi_sdr_parse_ctx_errnum (state_data->sdr_parse_ctx) == IPMI_SDR_PARSE_ERR_CANNOT_PARSE_OR_CALCULATE)
        {
          rv = 0;
          goto cleanup;
        }

      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_tolerance: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      goto cleanup;
    }

  if (tolerance)
    {
      pstdout_printf (state_data->pstate,
                      "Tolerance: %f %s\n",
                      *tolerance,
                      sensor_units_str);
    }
  else
    pstdout_printf (state_data->pstate,
                    "Tolerance: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);

  rv = 0;
 cleanup:
  if (tolerance)
    free (tolerance);
  return (rv);
}

static int
_detailed_output_resolution (ipmi_sensors_state_data_t *state_data,
                             const void *sdr_record,
                             unsigned int sdr_record_len,
                             const char *sensor_units_str)
{
  int8_t r_exponent;
  int16_t m;
  double resolution = 0.0;
  int rv = -1;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (sensor_units_str);
  assert (state_data->prog_data->args->verbose_count >= 2);

  /* achu: resolution is calculated using the decoding data, nothing
   * else in the SDR is read/required.  See section 36.4.2 in the
   * spec.
   */
  
  if (ipmi_sdr_parse_sensor_decoding_data (state_data->sdr_parse_ctx,
                                           sdr_record,
                                           sdr_record_len,
                                           &r_exponent,
                                           NULL,
                                           &m,
                                           NULL,
                                           NULL,
                                           NULL) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_decoding_data: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      goto cleanup;
    }
  
  
  if (ipmi_sensor_decode_resolution (r_exponent, m, &resolution) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sensor_decode_resolution: %s\n",
                       strerror (errno));
      goto cleanup;
    }
  
  pstdout_printf (state_data->pstate,
                  "Resolution: %f %s\n",
                  resolution,
                  sensor_units_str);

  rv = 0;
 cleanup:
  return (rv);
}

static int
_detailed_output_accuracy (ipmi_sensors_state_data_t *state_data,
                           const void *sdr_record,
                           unsigned int sdr_record_len)
{
  double *accuracy = NULL;
  int rv = -1;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (ipmi_sdr_parse_accuracy (state_data->sdr_parse_ctx,
                               sdr_record,
                               sdr_record_len,
                               &accuracy) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_accuracy: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      goto cleanup;
    }
  
  if (accuracy)
    pstdout_printf (state_data->pstate,
                    "Accuracy: %f%\n",
                    *accuracy);
  else
    pstdout_printf (state_data->pstate,
                    "Accuracy: %s\n",
                    IPMI_SENSORS_NA_STRING_OUTPUT);
  
  rv = 0;
 cleanup:
  if (accuracy)
    free (accuracy);
  return (rv);
}

static int
_detailed_output_hysteresis (ipmi_sensors_state_data_t *state_data,
                             const void *sdr_record,
                             unsigned int sdr_record_len,
			     uint8_t sensor_number,
                             uint8_t record_type)
{
  fiid_obj_t obj_cmd_rs = NULL;
  uint8_t positive_going_threshold_hysteresis_raw = 0;
  uint8_t negative_going_threshold_hysteresis_raw = 0;
  char sensor_units_buf[IPMI_SENSORS_UNITS_BUFLEN+1];
  uint8_t hysteresis_support;
  uint64_t val;
  int rv = -1;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  /* achu: first lets check if we have anything to output */
  if (ipmi_sdr_parse_sensor_capabilities (state_data->sdr_parse_ctx,
                                          sdr_record,
                                          sdr_record_len,
                                          NULL,
                                          NULL,
                                          &hysteresis_support,
                                          NULL,
                                          NULL) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_reading_ranges: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      goto cleanup;
    }

  if (hysteresis_support == IPMI_SDR_NO_HYSTERESIS_SUPPORT
      || hysteresis_support == IPMI_SDR_FIXED_UNREADABLE_HYSTERESIS_SUPPORT)
    {
      rv = 0;
      goto cleanup;
    }

  /* achu:
   *
   * I will admit I'm not entirely sure what the best way is to get
   * hysteresis.  It seems the information is stored/retrievable in
   * the SDR and through an IPMI command.
   *
   * We will try to read it via IPMI like we do with thresholds, since
   * a change to the hysteresis may not be written to the SDR.
   */

  memset (sensor_units_buf, '\0', IPMI_SENSORS_UNITS_BUFLEN+1);
  if (get_sensor_units_output_string (state_data->pstate,
                                      state_data->sdr_parse_ctx,
                                      sdr_record,
                                      sdr_record_len,
                                      sensor_units_buf,
                                      IPMI_SENSORS_UNITS_BUFLEN,
                                      _abbreviated_units_flag (state_data)) < 0)
    goto cleanup;

  if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_get_sensor_hysteresis_rs)))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_create: %s\n",
                       strerror (errno));
      goto cleanup;
    }

  if (ipmi_cmd_get_sensor_hysteresis (state_data->ipmi_ctx,
                                      sensor_number,
                                      IPMI_SENSOR_HYSTERESIS_MASK,
                                      obj_cmd_rs) < 0)
    {
      if (state_data->prog_data->args->common.debug)
        pstdout_fprintf (state_data->pstate,
                         stderr,
                         "ipmi_cmd_get_sensor_hysteresis: %s\n",
                         ipmi_ctx_errormsg (state_data->ipmi_ctx));

      if (ipmi_ctx_errnum (state_data->ipmi_ctx) == IPMI_ERR_BAD_COMPLETION_CODE
          && (ipmi_check_completion_code (obj_cmd_rs,
                                          IPMI_COMP_CODE_COMMAND_ILLEGAL_FOR_SENSOR_OR_RECORD_TYPE) == 1
              || ipmi_check_completion_code (obj_cmd_rs,
                                             IPMI_COMP_CODE_REQUEST_SENSOR_DATA_OR_RECORD_NOT_PRESENT) == 1))
        {
          /* The hysteresis cannot be gathered for one reason or
           * another, maybe b/c its a OEM sensor or something.  Output
           * "NA" stuff in output_raw.
           */
          goto output_raw;
        }

      goto cleanup;
    }

  if (FIID_OBJ_GET (obj_cmd_rs,
                    "positive_going_threshold_hysteresis_value",
                    &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'positive_going_threshold_hysteresis_value': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }
  positive_going_threshold_hysteresis_raw = val;

  if (FIID_OBJ_GET (obj_cmd_rs,
                    "negative_going_threshold_hysteresis_value",
                    &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'negative_going_threshold_hysteresis_value': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }
  negative_going_threshold_hysteresis_raw = val;

  /* achu: Well, compact records don't have the values to compute a
   * hysteresis value.  Perhaps that's a typo in the spec?  We just
   * output the integer values?  That's the best guess I can make.
   */

  if (record_type == IPMI_SDR_FORMAT_FULL_SENSOR_RECORD)
    {
      double positive_going_threshold_hysteresis_real;
      double negative_going_threshold_hysteresis_real;

      if (positive_going_threshold_hysteresis_raw
          || negative_going_threshold_hysteresis_raw)
        {
          int8_t r_exponent, b_exponent;
          int16_t m, b;
          uint8_t linearization, analog_data_format;

          if (ipmi_sdr_parse_sensor_decoding_data (state_data->sdr_parse_ctx,
                                                   sdr_record,
                                                   sdr_record_len,
                                                   &r_exponent,
                                                   &b_exponent,
                                                   &m,
                                                   &b,
                                                   &linearization,
                                                   &analog_data_format) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sdr_parse_sensor_decoding_data: %s\n",
                               ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
              goto cleanup;
            }

          /* if the sensor is not analog, this is most likely a bug in the
           * SDR, since we shouldn't be decoding a non-threshold sensor.
           *
           * Don't return an error.  Output integer value.
           */
          if (!IPMI_SDR_ANALOG_DATA_FORMAT_VALID (analog_data_format))
            goto output_raw;

          /* if the sensor is non-linear, I just don't know what to do
           *
           * Don't return an error.  Output integer value.
           */
          if (!IPMI_SDR_LINEARIZATION_IS_LINEAR (linearization))
            goto output_raw;

          if (ipmi_sensor_decode_value (r_exponent,
                                        b_exponent,
                                        m,
                                        b,
                                        linearization,
                                        analog_data_format,
                                        positive_going_threshold_hysteresis_raw,
                                        &positive_going_threshold_hysteresis_real) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sensor_decode_value: %s\n",
                               strerror (errno));
              goto cleanup;
            }

          if (ipmi_sensor_decode_value (r_exponent,
                                        b_exponent,
                                        m,
                                        b,
                                        linearization,
                                        analog_data_format,
                                        negative_going_threshold_hysteresis_raw,
                                        &negative_going_threshold_hysteresis_real) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sensor_decode_value: %s\n",
                               strerror (errno));
              goto cleanup;
            }
        }

      if (positive_going_threshold_hysteresis_raw)
        pstdout_printf (state_data->pstate,
                        "Positive Hysteresis: %f %s\n",
                        positive_going_threshold_hysteresis_real,
                        sensor_units_buf);
      else
        pstdout_printf (state_data->pstate,
                        "Positive Hysteresis: %s\n",
                        IPMI_SENSORS_NA_STRING_OUTPUT);

      if (negative_going_threshold_hysteresis_raw)
        pstdout_printf (state_data->pstate,
                        "Negative Hysteresis: %f %s\n",
                        negative_going_threshold_hysteresis_real,
                        sensor_units_buf);
      else
        pstdout_printf (state_data->pstate,
                        "Negative Hysteresis: %s\n",
                        IPMI_SENSORS_NA_STRING_OUTPUT);
    }
  else
    {
    output_raw:
      if (positive_going_threshold_hysteresis_raw)
        pstdout_printf (state_data->pstate,
                        "Positive Hysteresis: %u %s\n",
                        positive_going_threshold_hysteresis_raw,
                        sensor_units_buf);
      else
        pstdout_printf (state_data->pstate,
                        "Positive Hysteresis: %s\n",
                        IPMI_SENSORS_NA_STRING_OUTPUT);

      if (negative_going_threshold_hysteresis_raw)
        pstdout_printf (state_data->pstate,
                        "Negative Hysteresis: %u %s\n",
                        negative_going_threshold_hysteresis_raw,
                        sensor_units_buf);
      else
        pstdout_printf (state_data->pstate,
                        "Negative Hysteresis: %s\n",
                        IPMI_SENSORS_NA_STRING_OUTPUT);
    }

  rv = 0;
 cleanup:
  fiid_obj_destroy (obj_cmd_rs);
  return (rv);
}

static int
_detailed_output_event_enable (ipmi_sensors_state_data_t *state_data,
                               const void *sdr_record,
                               unsigned int sdr_record_len,
			       uint8_t sensor_number,
                               uint8_t record_type)
{
  fiid_obj_t obj_cmd_rs = NULL;
  int event_reading_type_code_class;
  uint64_t val;
  uint8_t event_reading_type_code;
  uint8_t sensor_type;
  uint8_t all_event_messages;
  uint8_t scanning_on_this_sensor;
  uint16_t event_bitmask;
  char **assertion_event_message_list = NULL;
  unsigned int assertion_event_message_list_len = 0;
  char **deassertion_event_message_list = NULL;
  unsigned int deassertion_event_message_list_len = 0;
  unsigned int i;
  int field_flag;
  int rv = -1;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  /* achu:
   *
   * I will admit I'm not entirely sure what the best way is to get
   * event enables.  It seems the information is stored/retrievable in
   * the SDR and through an IPMI command.
   *
   * We will try to read it via IPMI like we do with thresholds, since
   * a change to the event enables may not be written to the SDR.
   */

  if (ipmi_sdr_parse_event_reading_type_code (state_data->sdr_parse_ctx,
                                              sdr_record,
                                              sdr_record_len,
                                              &event_reading_type_code) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_event_reading_type_code: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      goto cleanup;
    }

  event_reading_type_code_class = ipmi_event_reading_type_code_class (event_reading_type_code);

  if (event_reading_type_code_class != IPMI_EVENT_READING_TYPE_CODE_CLASS_THRESHOLD
      && event_reading_type_code_class != IPMI_EVENT_READING_TYPE_CODE_CLASS_GENERIC_DISCRETE
      && event_reading_type_code_class !=  IPMI_EVENT_READING_TYPE_CODE_CLASS_SENSOR_SPECIFIC_DISCRETE)
    {
      if (state_data->prog_data->args->common.debug)
        pstdout_fprintf (state_data->pstate,
                         stderr,
                         "Cannot handle event enables for event type reading code: 0x%X\n",
                         event_reading_type_code);
      rv = 0;
      goto cleanup;
    }

  if (event_reading_type_code_class == IPMI_EVENT_READING_TYPE_CODE_CLASS_SENSOR_SPECIFIC_DISCRETE)
    {
      if (ipmi_sdr_parse_sensor_type (state_data->sdr_parse_ctx,
                                      sdr_record,
                                      sdr_record_len,
                                      &sensor_type) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sdr_parse_sensor_type: %s\n",
                           ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
          goto cleanup;
        }
    }

  if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_get_sensor_event_enable_rs)))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_create: %s\n",
                       strerror (errno));
      goto cleanup;
    }

  if (ipmi_cmd_get_sensor_event_enable (state_data->ipmi_ctx,
                                        sensor_number,
                                        obj_cmd_rs) < 0)
    {
      if (state_data->prog_data->args->common.debug)
        pstdout_fprintf (state_data->pstate,
                         stderr,
                         "ipmi_cmd_get_sensor_hysteresis: %s\n",
                         ipmi_ctx_errormsg (state_data->ipmi_ctx));

      if (ipmi_ctx_errnum (state_data->ipmi_ctx) == IPMI_ERR_BAD_COMPLETION_CODE
          && (ipmi_check_completion_code (obj_cmd_rs,
                                          IPMI_COMP_CODE_COMMAND_ILLEGAL_FOR_SENSOR_OR_RECORD_TYPE) == 1
              || ipmi_check_completion_code (obj_cmd_rs,
                                             IPMI_COMP_CODE_REQUEST_SENSOR_DATA_OR_RECORD_NOT_PRESENT) == 1
	      || ipmi_check_completion_code (obj_cmd_rs,
					     IPMI_COMP_CODE_COMMAND_RESPONSE_COULD_NOT_BE_PROVIDED) == 1))
        {
          /* The event enables cannot be gathered for one reason or
           * another, maybe b/c its a OEM sensor or something.  Just
           * don't output this info.
           */
          rv = 0;
          goto cleanup;
        }

      goto cleanup;
    }

  if (FIID_OBJ_GET (obj_cmd_rs,
                    "all_event_messages",
                    &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'all_event_messages': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }
  all_event_messages = val;

  if (all_event_messages == IPMI_SENSOR_ALL_EVENT_MESSAGES_DISABLE)
    {
      if (state_data->prog_data->args->legacy_output)
        {
          pstdout_printf (state_data->pstate,
                          "%s[%s]\n",
                          IPMI_SENSORS_ASSERTION_EVENT_PREFIX_LEGACY,
                          ALL_EVENT_MESSAGES_DISABLED);
          pstdout_printf (state_data->pstate,
                          "%s[%s]\n",
                          IPMI_SENSORS_DEASSERTION_EVENT_PREFIX_LEGACY,
                          ALL_EVENT_MESSAGES_DISABLED);
        }
      else
        {
          pstdout_printf (state_data->pstate,
                          "%s%s\n",
                          IPMI_SENSORS_ASSERTION_EVENT_PREFIX,
                          ALL_EVENT_MESSAGES_DISABLED);
          pstdout_printf (state_data->pstate,
                          "%s%s\n",
                          IPMI_SENSORS_DEASSERTION_EVENT_PREFIX,
                          ALL_EVENT_MESSAGES_DISABLED);
        }
      rv = 0;
      goto cleanup;
    }

  if (FIID_OBJ_GET (obj_cmd_rs,
                    "scanning_on_this_sensor",
                    &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'scanning_on_this_sensor': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }
  scanning_on_this_sensor = val;

  if (scanning_on_this_sensor == IPMI_SENSOR_SCANNING_ON_THIS_SENSOR_DISABLE)
    {
      if (state_data->prog_data->args->legacy_output)
        {
          pstdout_printf (state_data->pstate,
                          "%s[%s]\n",
                          IPMI_SENSORS_ASSERTION_EVENT_PREFIX_LEGACY,
                          SENSOR_SCANNING_DISABLED);
          pstdout_printf (state_data->pstate,
                          "%s[%s]\n",
                          IPMI_SENSORS_DEASSERTION_EVENT_PREFIX_LEGACY,
                          SENSOR_SCANNING_DISABLED);
        }
      rv = 0;
      goto cleanup;
    }

  /* achu: According to the spec, bytes 3-6 of the packet should exist
   * if all event messages are not disabled and sensor scanning is not
   * disabled.
   */

  if ((field_flag = fiid_obj_get (obj_cmd_rs,
                                  "assertion_event_bitmask",
                                  &val)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'assertion_event_bitmask': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }
  event_bitmask = val;

  if (field_flag)
    {
      if (event_reading_type_code_class == IPMI_EVENT_READING_TYPE_CODE_CLASS_THRESHOLD
          || event_reading_type_code_class == IPMI_EVENT_READING_TYPE_CODE_CLASS_GENERIC_DISCRETE)
        {
          if (get_generic_event_message_list (state_data,
                                              &assertion_event_message_list,
                                              &assertion_event_message_list_len,
                                              event_reading_type_code,
                                              event_bitmask,
                                              IPMI_SENSORS_NONE_STRING) < 0)
            goto cleanup;
        }
      else
        {
          if (get_sensor_specific_event_message_list (state_data,
                                                      &assertion_event_message_list,
                                                      &assertion_event_message_list_len,
                                                      sensor_type,
                                                      event_bitmask,
                                                      IPMI_SENSORS_NONE_STRING) < 0)
            goto cleanup;
        }

      if (ipmi_sensors_output_event_message_list (state_data,
                                                  assertion_event_message_list,
                                                  assertion_event_message_list_len,
                                                  IPMI_SENSORS_ASSERTION_EVENT_PREFIX_OUTPUT,
                                                  1) < 0)
        goto cleanup;
    }

  if ((field_flag = fiid_obj_get (obj_cmd_rs,
                                  "deassertion_event_bitmask",
                                  &val)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'deassertion_event_bitmask': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }
  event_bitmask = val;

  if (field_flag)
    {
      if (event_reading_type_code_class == IPMI_EVENT_READING_TYPE_CODE_CLASS_THRESHOLD
          || event_reading_type_code_class == IPMI_EVENT_READING_TYPE_CODE_CLASS_GENERIC_DISCRETE)
        {
          if (get_generic_event_message_list (state_data,
                                              &deassertion_event_message_list,
                                              &deassertion_event_message_list_len,
                                              event_reading_type_code,
                                              event_bitmask,
                                              IPMI_SENSORS_NONE_STRING) < 0)
            goto cleanup;
        }
      else
        {
          if (get_sensor_specific_event_message_list (state_data,
                                                      &deassertion_event_message_list,
                                                      &deassertion_event_message_list_len,
                                                      sensor_type,
                                                      event_bitmask,
                                                      IPMI_SENSORS_NONE_STRING) < 0)
            goto cleanup;
        }

      if (ipmi_sensors_output_event_message_list (state_data,
                                                  deassertion_event_message_list,
                                                  deassertion_event_message_list_len,
                                                  IPMI_SENSORS_DEASSERTION_EVENT_PREFIX_OUTPUT,
                                                  1) < 0)
        goto cleanup;
    }

  rv = 0;
 cleanup:
  if (assertion_event_message_list)
    {
      for (i = 0; i < assertion_event_message_list_len; i++)
        free (assertion_event_message_list[i]);
      free (assertion_event_message_list);
    }
  if (deassertion_event_message_list)
    {
      for (i = 0; i < deassertion_event_message_list_len; i++)
        free (deassertion_event_message_list[i]);
      free (deassertion_event_message_list);
    }
  fiid_obj_destroy (obj_cmd_rs);
  return (rv);
}

static int
_detailed_output_event_message_list (ipmi_sensors_state_data_t *state_data,
                                     char **event_message_list,
                                     unsigned int event_message_list_len)
{
  assert (state_data);
  assert (state_data->prog_data->args->verbose_count >= 1);

  if (ipmi_sensors_output_event_message_list (state_data,
                                              event_message_list,
                                              event_message_list_len,
                                              IPMI_SENSORS_SENSOR_EVENT_PREFIX_OUTPUT,
                                              1) < 0)
    return (-1);

  return (0);
}

static char *
_linearization_string (ipmi_sensors_state_data_t *state_data, uint8_t linearization)
{
  switch (linearization)
    {
    case IPMI_SDR_LINEARIZATION_LINEAR:
      return (IPMI_SDR_LINEARIZATION_LINEAR_STRING);
    case IPMI_SDR_LINEARIZATION_LN:
      return (IPMI_SDR_LINEARIZATION_LN_STRING);
    case IPMI_SDR_LINEARIZATION_LOG10:
      return (IPMI_SDR_LINEARIZATION_LOG10_STRING);
    case IPMI_SDR_LINEARIZATION_LOG2:
      return (IPMI_SDR_LINEARIZATION_LOG2_STRING);
    case IPMI_SDR_LINEARIZATION_E:
      return (IPMI_SDR_LINEARIZATION_E_STRING);
    case IPMI_SDR_LINEARIZATION_EXP10:
      return (IPMI_SDR_LINEARIZATION_EXP10_STRING);
    case IPMI_SDR_LINEARIZATION_EXP2:
      return (IPMI_SDR_LINEARIZATION_EXP2_STRING);
    case IPMI_SDR_LINEARIZATION_INVERSE:
      return (IPMI_SDR_LINEARIZATION_INVERSE_STRING);
    case IPMI_SDR_LINEARIZATION_SQR:
      return (IPMI_SDR_LINEARIZATION_SQR_STRING);
    case IPMI_SDR_LINEARIZATION_CUBE:
      return (IPMI_SDR_LINEARIZATION_CUBE_STRING);
    case IPMI_SDR_LINEARIZATION_SQRT:
      return (IPMI_SDR_LINEARIZATION_SQRT_STRING);
    case IPMI_SDR_LINEARIZATION_CUBERT:
      return (IPMI_SDR_LINEARIZATION_CUBERT_STRING);
    default:
      break;
    }

  return (IPMI_SDR_LINEARIZATION_NON_LINEAR_STRING);
}

static char *
_analog_data_format_string (ipmi_sensors_state_data_t *state_data, uint8_t analog_data_format)
{
  switch (analog_data_format)
    {
    case IPMI_SDR_ANALOG_DATA_FORMAT_UNSIGNED:
      return (IPMI_SDR_ANALOG_DATA_FORMAT_UNSIGNED_STRING);
    case IPMI_SDR_ANALOG_DATA_FORMAT_1S_COMPLEMENT:
      return (IPMI_SDR_ANALOG_DATA_FORMAT_1S_COMPLEMENT_STRING);
    case IPMI_SDR_ANALOG_DATA_FORMAT_2S_COMPLEMENT:
      return (IPMI_SDR_ANALOG_DATA_FORMAT_2S_COMPLEMENT_STRING);
    default:
      break;
    }

  return (IPMI_SDR_ANALOG_DATA_FORMAT_NOT_ANALOG_STRING);
}

static int
_detailed_output_full_record (ipmi_sensors_state_data_t *state_data,
                              const void *sdr_record,
                              unsigned int sdr_record_len,
			      uint8_t sensor_number,
                              uint8_t record_type,
                              uint16_t record_id,
                              double *reading,
                              char **event_message_list,
                              unsigned int event_message_list_len)
{
  uint8_t event_reading_type_code;
  int event_reading_type_code_class;
  char sensor_units_buf[IPMI_SENSORS_UNITS_BUFLEN+1];

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 1);

  if (_detailed_output_header (state_data,
                               sdr_record,
                               sdr_record_len,
			       sensor_number,
                               record_type,
                               record_id) < 0)
    return (-1);

  if (ipmi_sdr_parse_event_reading_type_code (state_data->sdr_parse_ctx,
                                              sdr_record,
                                              sdr_record_len,
                                              &event_reading_type_code) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_event_reading_type_code: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  memset (sensor_units_buf, '\0', IPMI_SENSORS_UNITS_BUFLEN+1);
  if (get_sensor_units_output_string (state_data->pstate,
                                      state_data->sdr_parse_ctx,
                                      sdr_record,
                                      sdr_record_len,
                                      sensor_units_buf,
                                      IPMI_SENSORS_UNITS_BUFLEN,
                                      _abbreviated_units_flag (state_data)) < 0)
    return (-1);

  event_reading_type_code_class = ipmi_event_reading_type_code_class (event_reading_type_code);

  if (event_reading_type_code_class == IPMI_EVENT_READING_TYPE_CODE_CLASS_THRESHOLD)
    {
      if (state_data->prog_data->args->verbose_count >= 2)
        {
          int8_t r_exponent, b_exponent;
          int16_t m, b;
          uint8_t linearization, analog_data_format;

          if (ipmi_sdr_parse_sensor_decoding_data (state_data->sdr_parse_ctx,
                                                   sdr_record,
                                                   sdr_record_len,
                                                   &r_exponent,
                                                   &b_exponent,
                                                   &m,
                                                   &b,
                                                   &linearization,
                                                   &analog_data_format) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sdr_parse_sensor_decoding_data: %s\n",
                               ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
              return (-1);
            }

          pstdout_printf (state_data->pstate,
                          "B: %d\n",
                          b);
          pstdout_printf (state_data->pstate,
                          "M: %d\n",
                          m);
          pstdout_printf (state_data->pstate,
                          "R Exponent: %d\n",
                          r_exponent);
          pstdout_printf (state_data->pstate,
                          "B Exponent: %d\n",
                          b_exponent);
          pstdout_printf (state_data->pstate,
                          "Linearization: %s (%Xh)\n",
                          _linearization_string (state_data, linearization),
                          linearization);
          pstdout_printf (state_data->pstate,
                          "Analog Data Format: %s (%Xh)\n",
                          _analog_data_format_string (state_data, analog_data_format),
                          analog_data_format);

          if (_detailed_output_tolerance (state_data,
                                          sdr_record,
                                          sdr_record_len,
                                          sensor_units_buf) < 0)
            return (-1);

          if (_detailed_output_resolution (state_data,
                                           sdr_record,
                                           sdr_record_len,
                                           sensor_units_buf) < 0)
            return (-1);
        }

      if (_detailed_output_thresholds (state_data,
                                       sdr_record,
                                       sdr_record_len,
                                       sensor_units_buf) < 0)
        return (-1);

      if (_detailed_output_sensor_reading_ranges (state_data,
                                                  sdr_record,
                                                  sdr_record_len,
                                                  sensor_units_buf) < 0)
        return (-1);
    }

  if (state_data->prog_data->args->verbose_count >= 2)
    {
      if (_detailed_output_accuracy (state_data,
                                     sdr_record,
                                     sdr_record_len) < 0)
        return (-1);

      if (_detailed_output_sensor_direction (state_data,
                                             sdr_record,
                                             sdr_record_len) < 0)
        return (-1);

      if (_detailed_output_hysteresis (state_data,
                                       sdr_record,
                                       sdr_record_len,
				       sensor_number,
                                       record_type) < 0)
        return (-1);

      if (_detailed_output_event_enable (state_data,
                                         sdr_record,
                                         sdr_record_len,
					 sensor_number,
                                         record_type) < 0)
        return (-1);
    }

  if (state_data->prog_data->args->legacy_output)
    {
      if (reading)
        pstdout_printf (state_data->pstate,
                        "Sensor Reading: %f %s\n",
                        *reading,
                        sensor_units_buf);
      else
        pstdout_printf (state_data->pstate,
                        "Sensor Reading: %s\n",
                        IPMI_SENSORS_NA_STRING_OUTPUT);
    }
  else
    {
      /* no need to output "N/A" for discrete sensors */

      if (reading)
        pstdout_printf (state_data->pstate,
                        "Sensor Reading: %f %s\n",
                        *reading,
                        sensor_units_buf);
      else if (event_reading_type_code_class == IPMI_EVENT_READING_TYPE_CODE_CLASS_THRESHOLD)
        pstdout_printf (state_data->pstate,
                        "Sensor Reading: %s\n",
                        IPMI_SENSORS_NA_STRING_OUTPUT);
    }

  if (_detailed_output_event_message_list (state_data,
                                           event_message_list,
                                           event_message_list_len) < 0)
    return (-1);

  pstdout_printf (state_data->pstate, "\n");

  return (0);
}

static int
_detailed_output_compact_record (ipmi_sensors_state_data_t *state_data,
                                 const void *sdr_record,
                                 unsigned int sdr_record_len,
				 uint8_t sensor_number,
                                 uint8_t record_type,
                                 uint16_t record_id,
                                 char **event_message_list,
                                 unsigned int event_message_list_len)
{
  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 1);

  if (_detailed_output_header (state_data,
                               sdr_record,
                               sdr_record_len,
			       sensor_number,
                               record_type,
                               record_id) < 0)
    return (-1);

  if (state_data->prog_data->args->verbose_count >= 2)
    {
      uint8_t share_count;
      uint8_t id_string_instance_modifier_type;
      uint8_t id_string_instance_modifier_offset;
      uint8_t entity_instance_sharing;

      if (_detailed_output_sensor_direction (state_data,
                                             sdr_record,
                                             sdr_record_len) < 0)
        return (-1);

      if (_detailed_output_hysteresis (state_data,
                                       sdr_record,
                                       sdr_record_len,
				       sensor_number,
                                       record_type) < 0)
        return (-1);

      if (_detailed_output_event_enable (state_data,
                                         sdr_record,
                                         sdr_record_len,
					 sensor_number,
                                         record_type) < 0)
        return (-1);

      if (ipmi_sdr_parse_sensor_record_sharing (state_data->sdr_parse_ctx,
                                                sdr_record,
                                                sdr_record_len,
                                                &share_count,
                                                &id_string_instance_modifier_type,
                                                &id_string_instance_modifier_offset,
                                                &entity_instance_sharing) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sdr_parse_sensor_record_sharing: %s\n",
                           ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
          return (-1);
        }

      pstdout_printf (state_data->pstate,
                      "Share Count: %u\n",
                      share_count);

      if (id_string_instance_modifier_type == IPMI_SDR_ID_STRING_INSTANCE_MODIFIER_TYPE_ALPHA)
        pstdout_printf (state_data->pstate,
                        "ID String Instance Modifier Type: Alpha\n");
      else
        pstdout_printf (state_data->pstate,
                        "ID String Instance Modifier Type: Numeric\n");
      
      pstdout_printf (state_data->pstate,
                      "ID String Instance Modifier Offset: %u\n",
                      id_string_instance_modifier_offset);

      if (entity_instance_sharing == IPMI_SDR_ENTITY_INSTANCE_INCREMENTS_FOR_EACH_SHARED_RECORD)
        pstdout_printf (state_data->pstate,
                        "Entity Instance Sharing: Increments for reach shared record\n");
      else
        pstdout_printf (state_data->pstate,
                        "Entity Instance Sharing: Same for all records\n");
    }

  if (_detailed_output_event_message_list (state_data,
                                           event_message_list,
                                           event_message_list_len) < 0)
    return (-1);

  pstdout_printf (state_data->pstate, "\n");

  return (0);
}

static int
_detailed_output_event_only_record (ipmi_sensors_state_data_t *state_data,
                                    const void *sdr_record,
                                    unsigned int sdr_record_len,
				    uint8_t sensor_number,
                                    uint8_t record_type,
                                    uint16_t record_id)
{
  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 1);

  if (_detailed_output_header (state_data,
                               sdr_record,
                               sdr_record_len,
			       sensor_number,
                               record_type,
                               record_id) < 0)
    return (-1);

  pstdout_printf (state_data->pstate, "\n");

  return (0);
}

static int
_detailed_output_entity_association_record (ipmi_sensors_state_data_t *state_data,
                                            const void *sdr_record,
                                            unsigned int sdr_record_len,
                                            uint8_t record_type,
                                            uint16_t record_id)
{
  uint8_t container_entity_id;
  uint8_t container_entity_instance;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (ipmi_sdr_parse_container_entity (state_data->sdr_parse_ctx,
                                       sdr_record,
                                       sdr_record_len,
                                       &container_entity_id,
                                       &container_entity_instance) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_container_entity: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  if (_detailed_output_record_type_and_id (state_data,
                                           record_type,
                                           record_id) < 0)
    return (-1);

  if (_detailed_output_entity_id_and_instance (state_data,
                                               "Container",
                                               container_entity_id,
                                               container_entity_instance) < 0)
    return (-1);

  pstdout_printf (state_data->pstate, "\n");

  return (0);
}

static int
_detailed_output_device_relative_entity_association_record (ipmi_sensors_state_data_t *state_data,
                                                            const void *sdr_record,
                                                            unsigned int sdr_record_len,
                                                            uint8_t record_type,
                                                            uint16_t record_id)
{
  uint8_t container_entity_id;
  uint8_t container_entity_instance;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (ipmi_sdr_parse_container_entity (state_data->sdr_parse_ctx,
                                       sdr_record,
                                       sdr_record_len,
                                       &container_entity_id,
                                       &container_entity_instance) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_container_entity: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  if (_detailed_output_record_type_and_id (state_data,
                                           record_type,
                                           record_id) < 0)
    return (-1);

  if (_detailed_output_entity_id_and_instance (state_data,
                                               "Container",
                                               container_entity_id,
                                               container_entity_instance) < 0)
    return (-1);

  pstdout_printf (state_data->pstate, "\n");

  return (0);
}

static int
_detailed_output_header2 (ipmi_sensors_state_data_t *state_data,
                          const void *sdr_record,
                          unsigned int sdr_record_len,
                          uint8_t record_type,
                          uint16_t record_id)
{
  char device_id_string[IPMI_SDR_CACHE_MAX_DEVICE_ID_STRING + 1];

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  memset (device_id_string, '\0', IPMI_SDR_CACHE_MAX_DEVICE_ID_STRING + 1);

  if (ipmi_sdr_parse_device_id_string (state_data->sdr_parse_ctx,
                                       sdr_record,
                                       sdr_record_len,
                                       device_id_string,
                                       IPMI_SDR_CACHE_MAX_DEVICE_ID_STRING) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_device_id_string: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  if (_detailed_output_record_type_and_id (state_data,
                                           record_type,
                                           record_id) < 0)
    return (-1);

  pstdout_printf (state_data->pstate,
                  "Device ID String: %s\n",
                  device_id_string);

  return (0);
}

static int
_output_device_type_and_modifier (ipmi_sensors_state_data_t *state_data,
                                  const void *sdr_record,
                                  unsigned int sdr_record_len)
{
  uint8_t device_type;
  uint8_t device_type_modifier;
  char device_type_modifier_buf[IPMI_SENSORS_DEVICE_TYPE_BUFLEN + 1];
  int len;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (ipmi_sdr_parse_device_type (state_data->sdr_parse_ctx,
                                  sdr_record,
                                  sdr_record_len,
                                  &device_type,
                                  &device_type_modifier) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_device_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));

      return (-1);
    }

  if (IPMI_DEVICE_TYPE_VALID (device_type))
    pstdout_printf (state_data->pstate,
                    "Device Type: %s (%Xh)\n",
                    ipmi_device_types[device_type],
                    device_type);
  else if (IPMI_DEVICE_TYPE_IS_OEM (device_type))
    pstdout_printf (state_data->pstate,
                    "Device Type: %s (%Xh)\n",
                    ipmi_oem_device_type,
                    device_type);
  else
    pstdout_printf (state_data->pstate,
                    "Device Type: %Xh\n",
                    device_type);

  memset (device_type_modifier_buf, '\0', IPMI_SENSORS_DEVICE_TYPE_BUFLEN + 1);
  
  if ((len = ipmi_device_type_modifer_message (device_type,
                                               device_type_modifier,
                                               device_type_modifier_buf,
                                               IPMI_SENSORS_DEVICE_TYPE_BUFLEN)) < 0)
    {
      /* assume invalid device type and/or modifier */
      pstdout_printf (state_data->pstate,
                      "Device Type Modifier: %Xh\n",
                      device_type_modifier);
      return (0);
    }

  if (len)
    pstdout_printf (state_data->pstate,
                    "Device Type Modifier: %s (%Xh)\n",
                    device_type_modifier_buf,
                    device_type_modifier);
  else
    pstdout_printf (state_data->pstate,
                    "Device Type Modifier: %Xh\n",
                    device_type_modifier);

  return (0);
}

static int
_output_entity_id_and_instance (ipmi_sensors_state_data_t *state_data,
                                const void *sdr_record,
                                unsigned int sdr_record_len)
{
  uint8_t entity_id;
  uint8_t entity_instance;
  uint8_t entity_instance_type;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);
  
  if (ipmi_sdr_parse_entity_id_instance_type (state_data->sdr_parse_ctx,
                                              sdr_record,
                                              sdr_record_len,
                                              &entity_id,
                                              &entity_instance,
                                              &entity_instance_type) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_entity_id_and_instance: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  if (_detailed_output_entity_id_and_instance (state_data,
                                               NULL,
                                               entity_id,
                                               entity_instance) < 0)
    return (-1);

  pstdout_printf (state_data->pstate,
                  "Entity Instance Type: %s\n",
                  (entity_instance_type == IPMI_SDR_PHYSICAL_ENTITY) ? "Physical Entity" : "Logical Container Entity");

  return (0);
}

static int
_detailed_output_generic_device_locator_record (ipmi_sensors_state_data_t *state_data,
                                                const void *sdr_record,
                                                unsigned int sdr_record_len,
                                                uint8_t record_type,
                                                uint16_t record_id)
{
  uint8_t device_access_address;
  uint8_t channel_number;
  uint8_t device_slave_address;
  uint8_t private_bus_id;
  uint8_t lun_for_master_write_read_command;
  uint8_t address_span;
  uint8_t oem;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (_detailed_output_header2 (state_data,
                                sdr_record,
                                sdr_record_len,
                                record_type,
                                record_id) < 0)
    return (-1);

  if (ipmi_sdr_parse_generic_device_locator_parameters (state_data->sdr_parse_ctx,
                                                        sdr_record,
                                                        sdr_record_len,
                                                        &device_access_address,
                                                        &channel_number,
                                                        &device_slave_address,
                                                        &private_bus_id,
                                                        &lun_for_master_write_read_command,
                                                        &address_span,
                                                        &oem) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_generic_device_locator_parameters: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  pstdout_printf (state_data->pstate,
                  "Device Access Address: %Xh\n",
                  device_access_address);
  pstdout_printf (state_data->pstate,
                  "Channel Number: %Xh\n",
                  channel_number);
  pstdout_printf (state_data->pstate,
                  "Direct Slave Address: %Xh\n",
                  device_slave_address);
  pstdout_printf (state_data->pstate,
                  "Private Bus ID: %Xh\n",
                  private_bus_id);
  pstdout_printf (state_data->pstate,
                  "LUN for Master Write-Read Command: %Xh\n",
                  lun_for_master_write_read_command);
  pstdout_printf (state_data->pstate,
                  "Address Span: %u\n",
                  address_span);
  pstdout_printf (state_data->pstate,
                  "OEM: %Xh\n",
                  oem);

  if (_output_device_type_and_modifier (state_data,
                                        sdr_record,
                                        sdr_record_len) < 0)
    return (-1);

  if (_output_entity_id_and_instance (state_data,
                                      sdr_record,
                                      sdr_record_len) < 0)
    return (-1);

  pstdout_printf (state_data->pstate, "\n");

  return (0);
}

static int
_detailed_output_fru_device_locator_record (ipmi_sensors_state_data_t *state_data,
                                            const void *sdr_record,
                                            unsigned int sdr_record_len,
                                            uint8_t record_type,
                                            uint16_t record_id)
{
  uint8_t device_access_address;
  uint8_t logical_fru_device_device_slave_address;
  uint8_t private_bus_id;
  uint8_t lun_for_master_write_read_fru_command;
  uint8_t logical_physical_fru_device;
  uint8_t channel_number;
  uint8_t fru_entity_id;
  uint8_t fru_entity_instance;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (_detailed_output_header2 (state_data,
                                sdr_record,
                                sdr_record_len,
                                record_type,
                                record_id) < 0)
    return (-1);

  if (ipmi_sdr_parse_fru_device_locator_parameters (state_data->sdr_parse_ctx,
                                                    sdr_record,
                                                    sdr_record_len,
                                                    &device_access_address,
                                                    &logical_fru_device_device_slave_address,
                                                    &private_bus_id,
                                                    &lun_for_master_write_read_fru_command,
                                                    &logical_physical_fru_device,
                                                    &channel_number) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_generic_device_locator_parameters: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));

      return (-1);
    }

  pstdout_printf (state_data->pstate,
                  "Device Access Address: %Xh\n",
                  device_access_address);

  if (logical_physical_fru_device)
    pstdout_printf (state_data->pstate,
                    "FRU Device ID: %Xh\n",
                    logical_fru_device_device_slave_address);
  else
    pstdout_printf (state_data->pstate,
                    "Device Slave Address: %Xh\n",
                    logical_fru_device_device_slave_address);

  pstdout_printf (state_data->pstate,
                  "Private Bus ID: %Xh\n",
                  private_bus_id);

  pstdout_printf (state_data->pstate,
                  "LUN for Master Write-Read or FRU Command: %Xh\n",
                  lun_for_master_write_read_fru_command);

  pstdout_printf (state_data->pstate,
                  "Channel Number: %Xh\n",
                  channel_number);

  if (_output_device_type_and_modifier (state_data,
                                        sdr_record,
                                        sdr_record_len) < 0)
    return (-1);

  if (ipmi_sdr_parse_fru_entity_id_and_instance (state_data->sdr_parse_ctx,
                                                 sdr_record,
                                                 sdr_record_len,
                                                 &fru_entity_id,
                                                 &fru_entity_instance) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_fru_entity_id_and_instance: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  if (_detailed_output_entity_id_and_instance (state_data,
                                               "FRU",
                                               fru_entity_id,
                                               fru_entity_instance) < 0)
    return (-1);

  pstdout_printf (state_data->pstate, "\n");

  return (0);
}

static int
_detailed_output_management_controller_device_locator_record (ipmi_sensors_state_data_t *state_data,
                                                              const void *sdr_record,
                                                              unsigned int sdr_record_len,
                                                              uint8_t record_type,
                                                              uint16_t record_id)
{
  uint8_t device_slave_address;
  uint8_t channel_number;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (_detailed_output_header2 (state_data,
                                sdr_record,
                                sdr_record_len,
                                record_type,
                                record_id) < 0)
    return (-1);

  if (ipmi_sdr_parse_management_controller_device_locator_parameters (state_data->sdr_parse_ctx,
                                                                      sdr_record,
                                                                      sdr_record_len,
                                                                      &device_slave_address,
                                                                      &channel_number) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_management_controller_device_locator_parameters: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  pstdout_printf (state_data->pstate,
                  "Device Slave Address: %Xh\n",
                  device_slave_address);

  pstdout_printf (state_data->pstate,
                  "Channel Number: %Xh\n",
                  channel_number);

  if (_output_entity_id_and_instance (state_data,
                                      sdr_record,
                                      sdr_record_len) < 0)
    return (-1);

  pstdout_printf (state_data->pstate, "\n");

  return (0);
}

static int
_output_manufacturer_id (ipmi_sensors_state_data_t *state_data,
                         const void *sdr_record,
                         unsigned int sdr_record_len)
{
  uint32_t manufacturer_id;
  char iana_buf[IPMI_SENSORS_IANA_LEN + 1];
  int ret;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (ipmi_sdr_parse_manufacturer_id (state_data->sdr_parse_ctx,
                                      sdr_record,
                                      sdr_record_len,
                                      &manufacturer_id) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_manufacturer_id: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  memset (iana_buf, '\0', IPMI_SENSORS_IANA_LEN + 1);

  /* if ret == 0 means no string, < 0 means bad manufacturer id
   * either way, output just the number
   */
  ret = ipmi_iana_enterprise_numbers_string (manufacturer_id,
                                             iana_buf,
                                             IPMI_SENSORS_IANA_LEN);
  if (ret > 0)
    pstdout_printf (state_data->pstate,
                    "Manufacturer ID: %s (%Xh)\n",
                    iana_buf,
                    manufacturer_id);

  else
    pstdout_printf (state_data->pstate,
                    "Manufacturer ID: %Xh\n",
                    manufacturer_id);
  
  return (0);
}

static int
_detailed_output_management_controller_confirmation_record (ipmi_sensors_state_data_t *state_data,
                                                            const void *sdr_record,
                                                            unsigned int sdr_record_len,
                                                            uint8_t record_type,
                                                            uint16_t record_id)
{
  uint16_t product_id;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (_detailed_output_record_type_and_id (state_data,
                                           record_type,
                                           record_id) < 0)
    return (-1);

  if (_output_manufacturer_id (state_data,
                               sdr_record,
                               sdr_record_len) < 0)
    return (-1);

  if (ipmi_sdr_parse_product_id (state_data->sdr_parse_ctx,
                                 sdr_record,
                                 sdr_record_len,
                                 &product_id) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_product_id: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  pstdout_printf (state_data->pstate,
                  "Product ID: %Xh\n",
                  product_id);

  pstdout_printf (state_data->pstate, "\n");

  return (0);
}

static int
_detailed_output_bmc_message_channel_info_record (ipmi_sensors_state_data_t *state_data,
                                                  const void *sdr_record,
                                                  unsigned int sdr_record_len,
                                                  uint8_t record_type,
                                                  uint16_t record_id)
{
  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (_detailed_output_record_type_and_id (state_data,
                                           record_type,
                                           record_id) < 0)
    return (-1);

  pstdout_printf (state_data->pstate, "\n");

  return (0);
}

static int
_detailed_output_oem_record (ipmi_sensors_state_data_t *state_data,
                             const void *sdr_record,
                             unsigned int sdr_record_len,
                             uint8_t record_type,
                             uint16_t record_id)
{
  uint8_t oem_data[IPMI_SENSORS_OEM_DATA_LEN];
  int len = 0;
  int i;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 2);

  if (_detailed_output_record_type_and_id (state_data,
                                           record_type,
                                           record_id) < 0)
    return (-1);


  if (_output_manufacturer_id (state_data,
                               sdr_record,
                               sdr_record_len) < 0)
    return (-1);

  if ((len = ipmi_sdr_parse_oem_data (state_data->sdr_parse_ctx,
                                      sdr_record,
                                      sdr_record_len,
                                      oem_data,
                                      IPMI_SENSORS_OEM_DATA_LEN)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_oem_data: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  pstdout_printf (state_data->pstate,
                  "OEM Data: ");

  for (i = 0; i < len; i++)
    pstdout_printf (state_data->pstate,
                    "%02X ",
                    oem_data[i]);
  pstdout_printf (state_data->pstate, "\n");

  pstdout_printf (state_data->pstate, "\n");

  return (0);
}

int
ipmi_sensors_detailed_output (ipmi_sensors_state_data_t *state_data,
                              const void *sdr_record,
                              unsigned int sdr_record_len,
			      uint8_t sensor_number,
                              double *reading,
                              char **event_message_list,
                              unsigned int event_message_list_len)
{
  uint16_t record_id;
  uint8_t record_type;

  assert (state_data);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (state_data->prog_data->args->verbose_count >= 1);

  if (ipmi_sdr_parse_record_id_and_type (state_data->sdr_parse_ctx,
                                         sdr_record,
                                         sdr_record_len,
                                         &record_id,
                                         &record_type) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sdr_parse_record_id_and_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
      return (-1);
    }

  if (state_data->prog_data->args->verbose_count >= 2)
    {
      switch (record_type)
        {
        case IPMI_SDR_FORMAT_FULL_SENSOR_RECORD:
          return (_detailed_output_full_record (state_data,
                                                sdr_record,
                                                sdr_record_len,
						sensor_number,
                                                record_type,
                                                record_id,
                                                reading,
                                                event_message_list,
                                                event_message_list_len));
        case IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD:
          return (_detailed_output_compact_record (state_data,
                                                   sdr_record,
                                                   sdr_record_len,
						   sensor_number,
                                                   record_type,
                                                   record_id,
                                                   event_message_list,
                                                   event_message_list_len));
        case IPMI_SDR_FORMAT_EVENT_ONLY_RECORD:
          return (_detailed_output_event_only_record (state_data,
                                                      sdr_record,
                                                      sdr_record_len,
						      sensor_number,
                                                      record_type,
                                                      record_id));
        case IPMI_SDR_FORMAT_ENTITY_ASSOCIATION_RECORD:
          return (_detailed_output_entity_association_record (state_data,
                                                              sdr_record,
                                                              sdr_record_len,
                                                              record_type,
                                                              record_id));

        case IPMI_SDR_FORMAT_DEVICE_RELATIVE_ENTITY_ASSOCIATION_RECORD:
          return (_detailed_output_device_relative_entity_association_record (state_data,
                                                                              sdr_record,
                                                                              sdr_record_len,
                                                                              record_type,
                                                                              record_id));
        case IPMI_SDR_FORMAT_GENERIC_DEVICE_LOCATOR_RECORD:
          return (_detailed_output_generic_device_locator_record (state_data,
                                                                  sdr_record,
                                                                  sdr_record_len,
                                                                  record_type,
                                                                  record_id));
        case IPMI_SDR_FORMAT_FRU_DEVICE_LOCATOR_RECORD:
          return (_detailed_output_fru_device_locator_record (state_data,
                                                              sdr_record,
                                                              sdr_record_len,
                                                              record_type,
                                                              record_id));
        case IPMI_SDR_FORMAT_MANAGEMENT_CONTROLLER_DEVICE_LOCATOR_RECORD:
          return (_detailed_output_management_controller_device_locator_record (state_data,
                                                                                sdr_record,
                                                                                sdr_record_len,
                                                                                record_type,
                                                                                record_id));
        case IPMI_SDR_FORMAT_MANAGEMENT_CONTROLLER_CONFIRMATION_RECORD:
          return (_detailed_output_management_controller_confirmation_record (state_data,
                                                                              sdr_record,
                                                                              sdr_record_len,
                                                                              record_type,
                                                                              record_id));
          break;
        case IPMI_SDR_FORMAT_BMC_MESSAGE_CHANNEL_INFO_RECORD:
          return (_detailed_output_bmc_message_channel_info_record (state_data,
                                                                    sdr_record,
                                                                    sdr_record_len,
                                                                    record_type,
                                                                    record_id));
          break;
        case IPMI_SDR_FORMAT_OEM_RECORD:
          return (_detailed_output_oem_record (state_data,
                                               sdr_record,
                                               sdr_record_len,
                                               record_type,
                                               record_id));
        default:
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "Unknown Record Type: %X\n",
                           record_type);
          break;
        }
    }
  else
    {
      switch (record_type)
        {
        case IPMI_SDR_FORMAT_FULL_SENSOR_RECORD:
          return (_detailed_output_full_record (state_data,
                                                sdr_record,
                                                sdr_record_len,
						sensor_number,
                                                record_type,
                                                record_id,
                                                reading,
                                                event_message_list,
                                                event_message_list_len));
        case IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD:
          return (_detailed_output_compact_record (state_data,
                                                   sdr_record,
                                                   sdr_record_len,
						   sensor_number,
                                                   record_type,
                                                   record_id,
                                                   event_message_list,
                                                   event_message_list_len));
        case IPMI_SDR_FORMAT_EVENT_ONLY_RECORD:
          /* only in legacy output, I dond't know why this was output
           * under verbose before
           */
          if (state_data->prog_data->args->legacy_output)
            return (_detailed_output_event_only_record (state_data,
                                                        sdr_record,
                                                        sdr_record_len,
							sensor_number,
                                                        record_type,
                                                        record_id));
        default:
          /* don't output any other types in verbose mode */
          break;
        }
    }

  return (0);
}