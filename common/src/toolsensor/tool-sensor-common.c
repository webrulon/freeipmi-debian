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
#include <stdint.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#include <assert.h>
#include <errno.h>

#include <freeipmi/freeipmi.h>

#include "tool-sensor-common.h"
#include "tool-sdr-cache-common.h"

#include "freeipmi-portability.h"
#include "pstdout.h"

#define RECORD_ID_BUFLEN       64

#define SENSOR_TYPE_BUFLEN     1024

#define SENSOR_UNITS_BUFLEN    1024

#define SENSOR_FMT_BUFLEN      1024

#define SENSOR_MODIFIER_BUFLEN 1024

#define SENSOR_CHARS_IN_ALPHA  26

static void
_str_replace_char (char *str, char chr, char with)
{
  char *p = NULL;
  char *s = NULL;
  
  assert (str);
  
  for (s = str;
       (p = strchr (s, chr));
       s = p + 1)
    *p = with;
}

const char *
get_sensor_type_output_string (unsigned int sensor_type)
{
  const char *sensor_type_str;

  if ((sensor_type_str = ipmi_get_sensor_type_string (sensor_type)))
    return (sensor_type_str);

  return (UNRECOGNIZED_SENSOR_TYPE);
}

void
get_sensor_type_cmdline_string (char *sensor_type)
{
  assert (sensor_type);

  _str_replace_char (sensor_type, ' ', '_');
  _str_replace_char (sensor_type, '/', '_');
}

int
get_entity_sensor_name_string (pstdout_state_t pstate,
                               ipmi_sdr_parse_ctx_t sdr_parse_ctx,
                               const void *sdr_record,
                               unsigned int sdr_record_len,
                               struct sensor_entity_id_counts *entity_id_counts,
                               uint8_t *sensor_number,
                               char *sensor_name_buf,
                               unsigned int sensor_name_buf_len)
{
  char id_string[IPMI_SDR_CACHE_MAX_ID_STRING + 1];
  char device_id_string[IPMI_SDR_CACHE_MAX_DEVICE_ID_STRING + 1];
  char *id_string_ptr = NULL;
  uint8_t entity_id, entity_instance, entity_instance_type;
  char *entity_id_str;
  uint16_t record_id;
  uint8_t record_type;

  assert (sdr_parse_ctx);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (entity_id_counts);
  assert (sensor_name_buf);
  assert (sensor_name_buf_len);

  memset (sensor_name_buf, '\0', sensor_name_buf_len);

  if (ipmi_sdr_parse_record_id_and_type (sdr_parse_ctx,
                                         sdr_record,
                                         sdr_record_len,
                                         &record_id,
                                         &record_type) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_parse_record_id_and_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
      return (-1);
    }
  
  if (record_type == IPMI_SDR_FORMAT_FULL_SENSOR_RECORD
      || record_type == IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD
      || record_type == IPMI_SDR_FORMAT_EVENT_ONLY_RECORD)
    {
      memset (id_string, '\0', IPMI_SDR_CACHE_MAX_ID_STRING + 1);
      if (ipmi_sdr_parse_id_string (sdr_parse_ctx,
                                    sdr_record,
                                    sdr_record_len,
                                    id_string,
                                    IPMI_SDR_CACHE_MAX_ID_STRING) < 0)
        {
          PSTDOUT_FPRINTF (pstate,
                           stderr,
                           "ipmi_sdr_parse_id_string: %s\n",
                           ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
          return (-1);
        }

      id_string_ptr = id_string;
    }
  else if (record_type == IPMI_SDR_FORMAT_GENERIC_DEVICE_LOCATOR_RECORD
           || record_type == IPMI_SDR_FORMAT_MANAGEMENT_CONTROLLER_DEVICE_LOCATOR_RECORD)
    {
      memset (device_id_string, '\0', IPMI_SDR_CACHE_MAX_DEVICE_ID_STRING + 1);
      if (ipmi_sdr_parse_device_id_string (sdr_parse_ctx,
                                           sdr_record,
                                           sdr_record_len,
                                           device_id_string,
                                           IPMI_SDR_CACHE_MAX_DEVICE_ID_STRING) < 0)
        {
          PSTDOUT_FPRINTF (pstate,
                           stderr,
                           "ipmi_sdr_parse_device_id_string: %s\n",
                           ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
          return (-1);
        }
      
      id_string_ptr = device_id_string;
    }
  else
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "Internal Error, cannot handle record type: %Xh\n",
                       record_type);
      return (-1);
    }

  if (ipmi_sdr_parse_entity_id_instance_type (sdr_parse_ctx,
                                              sdr_record,
                                              sdr_record_len,
                                              &entity_id,
                                              &entity_instance,
                                              &entity_instance_type) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_parse_entity_id_instance_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
      return (-1);
    }

  if (IPMI_ENTITY_ID_VALID (entity_id))
    entity_id_str = (char *)ipmi_entity_ids_pretty[entity_id];
  else if (IPMI_ENTITY_ID_IS_CHASSIS_SPECIFIC (entity_id))
    entity_id_str = "Chassis Specific";
  else if (IPMI_ENTITY_ID_IS_BOARD_SET_SPECIFIC (entity_id))
    entity_id_str = "Board-Set Specific";
  else if (IPMI_ENTITY_ID_IS_OEM_SYSTEM_INTEGRATOR_DEFINED (entity_id))
    entity_id_str = "OEM System Integrator";
  else
    entity_id_str = "OEM Entity"; /* vendor screwed up, assume it's an OEM entity id */

  /* a few special cases, for entity_ids are special, the vendor has
   * specifically stated there is no "entity" associated with this sdr
   * record
   */
  if (entity_id == IPMI_ENTITY_ID_UNSPECIFIED
      || entity_id == IPMI_ENTITY_ID_OTHER
      || entity_id == IPMI_ENTITY_ID_UNKNOWN)
    {
      snprintf (sensor_name_buf,
                sensor_name_buf_len,
                "%s",
                id_string_ptr);
    }
  else
    {
      if (entity_id_counts->count[entity_id] > 1)
        {
          /* special case if sensor sharing is involved */
          if ((record_type == IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD
               || record_type == IPMI_SDR_FORMAT_EVENT_ONLY_RECORD)
              && sensor_number)
            {
              uint8_t share_count;
              uint8_t id_string_instance_modifier_type;
              uint8_t id_string_instance_modifier_offset;
              uint8_t entity_instance_sharing;

              if (ipmi_sdr_parse_sensor_record_sharing (sdr_parse_ctx,
                                                        sdr_record,
                                                        sdr_record_len,
                                                        &share_count,
                                                        &id_string_instance_modifier_type,
                                                        &id_string_instance_modifier_offset,
                                                        &entity_instance_sharing) < 0)
                {
                  PSTDOUT_FPRINTF (pstate,
                                   stderr,
                                   "ipmi_sdr_parse_sensor_record_sharing: %s\n",
                                   ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
                  return (-1);
                }

              if (share_count > 1
                  && entity_instance_sharing == IPMI_SDR_ENTITY_INSTANCE_INCREMENTS_FOR_EACH_SHARED_RECORD)
                {
                  uint8_t sensor_number_base;
                  uint8_t sensor_number_offset;

                  if (ipmi_sdr_parse_sensor_number (sdr_parse_ctx,
                                                    sdr_record,
                                                    sdr_record_len,
                                                    &sensor_number_base) < 0)
                    {
                      PSTDOUT_FPRINTF (pstate,
                                       stderr,
                                       "ipmi_sdr_parse_sensor_number: %s\n",
                                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
                      return (-1);
                    }

                  /* I guess it's a bug if the sensor number passed in is bad */
                  if ((*sensor_number) >= sensor_number_base)
                    sensor_number_offset = (*sensor_number) - sensor_number_base;
                  else
                    goto fallthrough;

                  if (id_string_instance_modifier_type == IPMI_SDR_ID_STRING_INSTANCE_MODIFIER_TYPE_ALPHA)
                    {
                      char modifierbuf[SENSOR_MODIFIER_BUFLEN];

                      memset (modifierbuf, '\0', SENSOR_MODIFIER_BUFLEN);
                      
                      /* IPMI spec example is:
                       *
                       * "If the modifier = alpha, offset=0
                       * corresponds to 'A', offset=25 corresponses to
                       * 'Z', and offset = 26 corresponds to 'AA', for
                       * offset=26 the sensors could be identified as:
                       * Temp AA, Temp AB, Temp AC."
                       *
                       * achu note: id_string_instance_modifier_type
                       * is a 7 bit field, so we cannot reach a
                       * situation of 'AAA' or 'AAB'.  The max is
                       * 'EX':
                       *
                       * 'A' + (127/26) = 4 => 'E'
                       * 'A' + (127 % 26) = 23 => 'X'
                       */

                      if ((id_string_instance_modifier_type + sensor_number_offset) < SENSOR_CHARS_IN_ALPHA)
                        snprintf (sensor_name_buf,
                                  sensor_name_buf_len,
                                  "%s %s %c",
                                  entity_id_str,
                                  id_string_ptr,
                                  'A' + ((id_string_instance_modifier_type + sensor_number_offset)/SENSOR_CHARS_IN_ALPHA));
                      else
                        snprintf (sensor_name_buf,
                                  sensor_name_buf_len,
                                  "%s %s %c%c",
                                  entity_id_str,
                                  id_string_ptr,
                                  'A' + ((id_string_instance_modifier_type + sensor_number_offset)/SENSOR_CHARS_IN_ALPHA),
                                  'A' + (id_string_instance_modifier_type % SENSOR_CHARS_IN_ALPHA));
                    }
                  else
                    {
                      /* IPMI spec example is:
                       *
                       * "Suppose snsor ID is 'Temp' for 'Temperature
                       * Sensor', share count = 3, ID string instance
                       * modifier = numeric, instance modifier offset
                       * = 5 - then the sensors oculd be identified
                       * as: Temp 5, Temp 6, Temp 7"
                       */
                      snprintf (sensor_name_buf,
                                sensor_name_buf_len,
                                "%s %s %u",
                                entity_id_str,
                                id_string_ptr,
                                id_string_instance_modifier_offset + sensor_number_offset);
                    }

                  goto out;
                }
            }

        fallthrough:
          snprintf (sensor_name_buf,
                    sensor_name_buf_len,
                    "%s %u %s",
                    entity_id_str,
                    entity_instance,
                    id_string_ptr);
        }
      else
        snprintf (sensor_name_buf,
                  sensor_name_buf_len,
                  "%s %s",
                  entity_id_str,
                  id_string_ptr);
    }

 out:
  return (0);
}

int
get_entity_sensor_name_string_by_record_id (pstdout_state_t pstate,
                                            ipmi_sdr_parse_ctx_t sdr_parse_ctx,
                                            ipmi_sdr_cache_ctx_t sdr_cache_ctx,
                                            uint16_t record_id,
                                            struct sensor_entity_id_counts *entity_id_counts,
                                            uint8_t *sensor_number,
                                            char *sensor_name_buf,
                                            unsigned int sensor_name_buf_len)
{
  uint8_t sdr_record[IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH];
  int sdr_record_len = 0;

  assert (sdr_parse_ctx);
  assert (sdr_cache_ctx);
  assert (entity_id_counts);
  assert (sensor_name_buf);
  assert (sensor_name_buf_len);

  if (ipmi_sdr_cache_search_record_id (sdr_cache_ctx, record_id) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_cache_search_record_id: %s\n",
                       ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
      return (-1);
    }

  if ((sdr_record_len = ipmi_sdr_cache_record_read (sdr_cache_ctx,
                                                    sdr_record,
                                                    IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH)) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_cache_record_read: %s\n",
                       ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
      return (-1);
    }

  return get_entity_sensor_name_string (pstate,
                                        sdr_parse_ctx,
                                        sdr_record,
                                        sdr_record_len,
                                        entity_id_counts,
                                        sensor_number,
                                        sensor_name_buf,
                                        sensor_name_buf_len);
}

int
display_sensor_type_cmdline (pstdout_state_t pstate, 
                             unsigned int sensor_type)
{
  const char *sensor_type_str;
  char *tmpstr = NULL;

  sensor_type_str = get_sensor_type_output_string (sensor_type);

  if (!(tmpstr = strdupa (sensor_type_str)))
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "strdupa: %s\n",
                       strerror (errno));
      return (-1);
    }

  get_sensor_type_cmdline_string (tmpstr);

  PSTDOUT_PRINTF (pstate, "%s\n", tmpstr);
  return (0);
}

int
get_sensor_units_output_string (pstdout_state_t pstate,
                                ipmi_sdr_parse_ctx_t sdr_parse_ctx,
                                const void *sdr_record,
                                unsigned int sdr_record_len,
                                char *sensor_units_buf,
                                unsigned int sensor_units_buflen,
                                unsigned int abbreviated_units_flag)
{
  uint8_t sensor_units_percentage;
  uint8_t sensor_units_modifier;
  uint8_t sensor_units_rate;
  uint8_t sensor_base_unit_type;
  uint8_t sensor_modifier_unit_type;
  int sensor_units_ret = 0;
  int rv = -1;

  assert (sdr_parse_ctx);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (sensor_units_buf);
  assert (sensor_units_buflen);

  if (ipmi_sdr_parse_sensor_units (sdr_parse_ctx,
                                   sdr_record,
                                   sdr_record_len,
                                   &sensor_units_percentage,
                                   &sensor_units_modifier,
                                   &sensor_units_rate,
                                   &sensor_base_unit_type,
                                   &sensor_modifier_unit_type) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_units: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
      goto cleanup;
    }
  
  memset (sensor_units_buf, '\0', sensor_units_buflen);
  sensor_units_ret = ipmi_sensor_units_string (sensor_units_percentage,
                                               sensor_units_modifier,
                                               sensor_units_rate,
                                               sensor_base_unit_type,
                                               sensor_modifier_unit_type,
                                               sensor_units_buf,
                                               sensor_units_buflen,
                                               abbreviated_units_flag);
  
  if (sensor_units_ret <= 0)
    snprintf (sensor_units_buf,
              sensor_units_buflen,
              "%s",
              ipmi_sensor_units[IPMI_SENSOR_UNIT_UNSPECIFIED]);
  
  rv = 0;
 cleanup:
  return rv;
}

int
display_string_cmdline (pstdout_state_t pstate, 
                        const char *str)
{
  char *tmpstr;

  assert (str);

  if (!(tmpstr = strdupa (str)))
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "strdupa: %s\n",
                       strerror (errno));
      return (-1);
    } 

  get_sensor_type_cmdline_string (tmpstr);

  PSTDOUT_PRINTF (pstate, "%s\n", tmpstr);
  return (0);
}

int
sensor_type_strcmp (pstdout_state_t pstate,
                    const char *sensor_type_str_input,
                    unsigned int sensor_type)
{
  const char *sensor_type_str;
  char *tmpstr;

  assert (sensor_type_str_input);

  /* Don't use get_sensor_type_output_string() - want NULL if invalid */
  sensor_type_str = ipmi_get_sensor_type_string (sensor_type);

  if (!sensor_type_str)
    return (0);

  if (!(tmpstr = strdupa (sensor_type_str)))
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "strdupa: %s\n",
                       strerror (errno));
      return (-1);
    }

  get_sensor_type_cmdline_string (tmpstr);

  if (!strcasecmp (sensor_type_str_input, sensor_type_str)
      || !strcasecmp (sensor_type_str_input, tmpstr))
    return (1);
  
  return (0);
}

void
output_sensor_headers (pstdout_state_t pstate,
                       int quiet_readings,
                       int output_sensor_state,
                       int comma_separated_output,
                       int no_sensor_type_output,
                       struct sensor_column_width *column_width)
{
  char fmt[SENSOR_FMT_BUFLEN + 1];

  assert (column_width);

  memset (fmt, '\0', SENSOR_FMT_BUFLEN + 1);
  if (no_sensor_type_output)
    {
      if (comma_separated_output)
        snprintf (fmt,
                  SENSOR_FMT_BUFLEN,
                  "%%s,%%s");
      else
        snprintf (fmt,
                  SENSOR_FMT_BUFLEN,
                  "%%-%ds | %%-%ds",
                  column_width->record_id,
                  column_width->sensor_name);
      
      PSTDOUT_PRINTF (pstate,
                      fmt,
                      SENSORS_HEADER_RECORD_ID_STR,
                      SENSORS_HEADER_NAME_STR);
    }
  else
    {
      if (comma_separated_output)
        snprintf (fmt,
                  SENSOR_FMT_BUFLEN,
                  "%%s,%%s,%%s");
      else
        snprintf (fmt,
                  SENSOR_FMT_BUFLEN,
                  "%%-%ds | %%-%ds | %%-%ds",
                  column_width->record_id,
                  column_width->sensor_name,
                  column_width->sensor_type);
      
      PSTDOUT_PRINTF (pstate,
                      fmt,
                      SENSORS_HEADER_RECORD_ID_STR,
                      SENSORS_HEADER_NAME_STR,
                      SENSORS_HEADER_TYPE_STR);
    }

  if (output_sensor_state)
    {
      if (comma_separated_output)
        PSTDOUT_PRINTF (pstate,
                        ",%s",
                        SENSORS_HEADER_STATE_STR);
      else
        PSTDOUT_PRINTF (pstate,
                        " | %s   ",
                        SENSORS_HEADER_STATE_STR);
    }
  
  if (!quiet_readings)
    {
      if (comma_separated_output)
        PSTDOUT_PRINTF (pstate,
                        ",%s",
                        SENSORS_HEADER_READING_STR);
      else
        PSTDOUT_PRINTF (pstate,
                        " | %s   ",
                        SENSORS_HEADER_READING_STR);
      
      memset (fmt, '\0', SENSOR_FMT_BUFLEN + 1);
      if (comma_separated_output)
        snprintf (fmt,
                  SENSOR_FMT_BUFLEN,
                  ",%%s");
      else
        snprintf (fmt,
                  SENSOR_FMT_BUFLEN,
                  " | %%-%ds",
                  column_width->sensor_units);
      
      PSTDOUT_PRINTF (pstate,
                      fmt,
                      SENSORS_HEADER_UNITS_STR);
    }

  if (comma_separated_output)
    PSTDOUT_PRINTF (pstate,
                    ",%s\n",
                    SENSORS_HEADER_EVENT_STR);
  else
    PSTDOUT_PRINTF (pstate,
                    " | %s\n",
                    SENSORS_HEADER_EVENT_STR);
}

static int
_is_sdr_sensor_type_listed (pstdout_state_t pstate,
                            ipmi_sdr_parse_ctx_t sdr_parse_ctx,
                            const void *sdr_record,
                            unsigned int sdr_record_len,
                            char sensor_types[][MAX_SENSOR_TYPES_STRING_LENGTH+1],
                            unsigned int sensor_types_length)
{
  uint16_t record_id;
  uint8_t record_type;
  uint8_t sensor_type;
  int i;

  assert (sdr_parse_ctx);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (sensor_types);
  assert (sensor_types_length);

  if (ipmi_sdr_parse_record_id_and_type (sdr_parse_ctx,
                                         sdr_record,
                                         sdr_record_len,
                                         &record_id,
                                         &record_type) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_parse_record_id_and_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
      return (-1);
    }

  if (record_type != IPMI_SDR_FORMAT_FULL_SENSOR_RECORD
      && record_type != IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD
      && record_type != IPMI_SDR_FORMAT_EVENT_ONLY_RECORD)
    return (0);

  if (ipmi_sdr_parse_sensor_type (sdr_parse_ctx,
                                  sdr_record,
                                  sdr_record_len,
                                  &sensor_type) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
      return (-1);
    }

  for (i = 0; i < sensor_types_length; i++)
    {
      int ret;

      if ((ret = sensor_type_strcmp (pstate,
                                     sensor_types[i],
                                     sensor_type)) < 0)
        return (-1);

      if (ret)
        return (1);
    }

  return (0);
}
static void
_sensor_entity_id_counts_init (struct sensor_entity_id_counts *entity_id_counts)
{
  assert (entity_id_counts);

  memset (entity_id_counts, '\0', sizeof (struct sensor_entity_id_counts));
}

static int
_store_entity_id_count (pstdout_state_t pstate,
                        ipmi_sdr_parse_ctx_t sdr_parse_ctx,
                        const void *sdr_record,
                        unsigned int sdr_record_len,
                        struct sensor_entity_id_counts *entity_id_counts)
{
  uint16_t record_id;
  uint8_t record_type;
  uint8_t entity_id, entity_instance, entity_instance_type;

  assert (sdr_parse_ctx);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (entity_id_counts);

  if (ipmi_sdr_parse_record_id_and_type (sdr_parse_ctx,
                                         sdr_record,
                                         sdr_record_len,
                                         &record_id,
                                         &record_type) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_parse_record_id_and_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
      return (-1);
    }

  if (record_type != IPMI_SDR_FORMAT_FULL_SENSOR_RECORD
      && record_type != IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD
      && record_type != IPMI_SDR_FORMAT_EVENT_ONLY_RECORD
      && record_type != IPMI_SDR_FORMAT_GENERIC_DEVICE_LOCATOR_RECORD
      && record_type != IPMI_SDR_FORMAT_MANAGEMENT_CONTROLLER_DEVICE_LOCATOR_RECORD)
    return (0);

  if (ipmi_sdr_parse_entity_id_instance_type (sdr_parse_ctx,
                                              sdr_record,
                                              sdr_record_len,
                                              &entity_id,
                                              &entity_instance,
                                              &entity_instance_type) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_parse_entity_id_instance_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
      return (-1);
    }

  /* there's now atleast one of this entity id */
  if (!entity_id_counts->count[entity_id])
    entity_id_counts->count[entity_id] = 1;

  if (entity_instance > entity_id_counts->count[entity_id])
    entity_id_counts->count[entity_id] = entity_instance;

  /* special case if sensor sharing is involved */
  if (record_type == IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD
      || record_type == IPMI_SDR_FORMAT_EVENT_ONLY_RECORD)
    {
      uint8_t share_count;
      uint8_t entity_instance_sharing;

      if (ipmi_sdr_parse_sensor_record_sharing (sdr_parse_ctx,
                                                sdr_record,
                                                sdr_record_len,
                                                &share_count,
                                                NULL,
                                                NULL,
                                                &entity_instance_sharing) < 0)
        {
          PSTDOUT_FPRINTF (pstate,
                           stderr,
                           "ipmi_sdr_parse_sensor_record_sharing: %s\n",
                           ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
          return (-1);
        }
      
      if (share_count > 1
          && entity_instance_sharing == IPMI_SDR_ENTITY_INSTANCE_INCREMENTS_FOR_EACH_SHARED_RECORD)
        {
          int i;

          for (i = 0; i < share_count; i++)
            {
              entity_instance += (i + 1);
              
              if (entity_instance > entity_id_counts->count[entity_id])
                entity_id_counts->count[entity_id] = entity_instance;
            }
        }
    }
 
  return (0);
}

int
calculate_entity_id_counts (pstdout_state_t pstate,
                            ipmi_sdr_cache_ctx_t sdr_cache_ctx,
                            ipmi_sdr_parse_ctx_t sdr_parse_ctx,
                            struct sensor_entity_id_counts *entity_id_counts)
{
  uint8_t sdr_record[IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH];
  int sdr_record_len = 0;
  uint16_t record_count;
  int rv = -1;
  int i;

  assert (sdr_cache_ctx);
  assert (sdr_parse_ctx);
  assert (entity_id_counts);

  _sensor_entity_id_counts_init (entity_id_counts);

  if (ipmi_sdr_cache_record_count (sdr_cache_ctx, &record_count) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_cache_record_count: %s\n",
                       ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
      goto cleanup;
    }
  
  for (i = 0; i < record_count; i++, ipmi_sdr_cache_next (sdr_cache_ctx))
    {
      if ((sdr_record_len = ipmi_sdr_cache_record_read (sdr_cache_ctx,
                                                        sdr_record,
                                                        IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH)) < 0)
        {
          PSTDOUT_FPRINTF (pstate,
                           stderr,
                           "ipmi_sdr_cache_record_read: %s\n",
                           ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
          goto cleanup;
        }
      
      /* Shouldn't be possible */
      if (!sdr_record_len)
        continue;
      
      if (_store_entity_id_count (pstate,
                                  sdr_parse_ctx,
                                  sdr_record,
                                  sdr_record_len,
                                  entity_id_counts) < 0)
        goto cleanup;
    }

  rv = 0;
 cleanup:
  ipmi_sdr_cache_first (sdr_cache_ctx);
  return (rv);
}

static void
_sensor_column_width_init (struct sensor_column_width *column_width)
{
  assert (column_width);

  column_width->record_id = 0;
  column_width->sensor_name = 0;
  column_width->sensor_type = 0;
  column_width->sensor_units = 0;
}

static void
_sensor_column_width_finish (struct sensor_column_width *column_width)
{
  assert (column_width);

  if (column_width->record_id < strlen(SENSORS_HEADER_RECORD_ID_STR))
    column_width->record_id = strlen(SENSORS_HEADER_RECORD_ID_STR);
  if (column_width->sensor_name < strlen(SENSORS_HEADER_NAME_STR))
    column_width->sensor_name = strlen(SENSORS_HEADER_NAME_STR);
  if (column_width->sensor_type < strlen(SENSORS_HEADER_TYPE_STR))
    column_width->sensor_type = strlen(SENSORS_HEADER_TYPE_STR);
  if (column_width->sensor_units < strlen(SENSORS_HEADER_UNITS_STR))
    column_width->sensor_units = strlen(SENSORS_HEADER_UNITS_STR);
}

static int
_store_column_widths (pstdout_state_t pstate,
                      ipmi_sdr_parse_ctx_t sdr_parse_ctx,
                      const void *sdr_record,
                      unsigned int sdr_record_len,
                      uint8_t *sensor_number,
                      unsigned int abbreviated_units,
                      unsigned int count_event_only_records,
                      unsigned int count_device_locator_records,
                      unsigned int count_oem_records,
                      struct sensor_entity_id_counts *entity_id_counts,
                      struct sensor_column_width *column_width)
{
  char record_id_buf[RECORD_ID_BUFLEN + 1];
  uint16_t record_id;
  uint8_t record_type;
  uint8_t sensor_type;
  uint8_t event_reading_type_code;
  int len;

  assert (sdr_parse_ctx);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (column_width);

  if (ipmi_sdr_parse_record_id_and_type (sdr_parse_ctx,
                                         sdr_record,
                                         sdr_record_len,
                                         &record_id,
                                         &record_type) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_parse_record_id_and_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
      return (-1);
    }

  if (record_type != IPMI_SDR_FORMAT_FULL_SENSOR_RECORD
      && record_type != IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD
      && (!count_event_only_records
          || record_type != IPMI_SDR_FORMAT_EVENT_ONLY_RECORD)
      && (!count_device_locator_records
          || (record_type != IPMI_SDR_FORMAT_GENERIC_DEVICE_LOCATOR_RECORD
              && record_type != IPMI_SDR_FORMAT_MANAGEMENT_CONTROLLER_DEVICE_LOCATOR_RECORD))
      && (!count_oem_records
          || record_type != IPMI_SDR_FORMAT_OEM_RECORD))
    return (0);

  memset (record_id_buf, '\0', RECORD_ID_BUFLEN + 1);
  snprintf (record_id_buf, RECORD_ID_BUFLEN, "%u", record_id);

  len = strlen (record_id_buf);
  if (len > column_width->record_id)
    column_width->record_id = len;

  /* Done, only need to calculate record id column for OEM records */
  if (record_type == IPMI_SDR_FORMAT_OEM_RECORD)
    return (0);

  if (entity_id_counts)
    {
      char sensor_name[MAX_ENTITY_ID_SENSOR_NAME_STRING + 1];

      memset (sensor_name, '\0', MAX_ENTITY_ID_SENSOR_NAME_STRING + 1);

      if (get_entity_sensor_name_string (pstate,
                                         sdr_parse_ctx,
                                         sdr_record,
                                         sdr_record_len,
                                         entity_id_counts,
                                         sensor_number,
                                         sensor_name,
                                         MAX_ENTITY_ID_SENSOR_NAME_STRING) < 0)
        return (-1);

      len = strlen (sensor_name);
      if (len > column_width->sensor_name)
        column_width->sensor_name = len;
    }
  else
    {
      char id_string[IPMI_SDR_CACHE_MAX_ID_STRING + 1];
      char device_id_string[IPMI_SDR_CACHE_MAX_DEVICE_ID_STRING + 1];
      char *id_string_ptr = NULL;

      if (record_type == IPMI_SDR_FORMAT_FULL_SENSOR_RECORD
          || record_type == IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD
          || record_type == IPMI_SDR_FORMAT_EVENT_ONLY_RECORD)
        {
          memset (id_string, '\0', IPMI_SDR_CACHE_MAX_ID_STRING + 1);
          if (ipmi_sdr_parse_id_string (sdr_parse_ctx,
                                        sdr_record,
                                        sdr_record_len,
                                        id_string,
                                        IPMI_SDR_CACHE_MAX_ID_STRING) < 0)
            {
              PSTDOUT_FPRINTF (pstate,
                               stderr,
                               "ipmi_sdr_parse_id_string: %s\n",
                               ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
              return (-1);
            }

          id_string_ptr = id_string;
        }
      else
        {
          memset (device_id_string, '\0', IPMI_SDR_CACHE_MAX_DEVICE_ID_STRING + 1);
          if (ipmi_sdr_parse_device_id_string (sdr_parse_ctx,
                                               sdr_record,
                                               sdr_record_len,
                                               device_id_string,
                                               IPMI_SDR_CACHE_MAX_DEVICE_ID_STRING) < 0)
            {
              PSTDOUT_FPRINTF (pstate,
                               stderr,
                               "ipmi_sdr_parse_device_id_string: %s\n",
                               ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
              return (-1);
            }

          id_string_ptr = device_id_string;
        }
      
      len = strlen (id_string_ptr);
      if (len > column_width->sensor_name)
        column_width->sensor_name = len;
    }

  if (record_type == IPMI_SDR_FORMAT_FULL_SENSOR_RECORD
      || record_type == IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD
      || record_type == IPMI_SDR_FORMAT_EVENT_ONLY_RECORD)
    {
      if (ipmi_sdr_parse_sensor_type (sdr_parse_ctx,
                                      sdr_record,
                                      sdr_record_len,
                                      &sensor_type) < 0)
        {
          PSTDOUT_FPRINTF (pstate,
                           stderr,
                           "ipmi_sdr_parse_sensor_type: %s\n",
                           ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
          return (-1);
        }

      len = strlen (get_sensor_type_output_string (sensor_type));
      if (len > column_width->sensor_type)
        column_width->sensor_type = len;

      if (ipmi_sdr_parse_event_reading_type_code (sdr_parse_ctx,
                                                  sdr_record,
                                                  sdr_record_len,
                                                  &event_reading_type_code) < 0)
        {
          PSTDOUT_FPRINTF (pstate,
                           stderr,
                           "ipmi_sdr_parse_event_reading_type_code: %s\n",
                           ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
          return (-1);
        }

      if (ipmi_event_reading_type_code_class (event_reading_type_code) == IPMI_EVENT_READING_TYPE_CODE_CLASS_THRESHOLD)
        {
          char sensor_units_buf[SENSOR_UNITS_BUFLEN + 1];
          
          memset (sensor_units_buf, '\0', SENSOR_UNITS_BUFLEN + 1);
          if (get_sensor_units_output_string (pstate,
                                              sdr_parse_ctx,
                                              sdr_record,
                                              sdr_record_len,
                                              sensor_units_buf,
                                              SENSOR_UNITS_BUFLEN,
                                              abbreviated_units) < 0)
            return (-1);
          
          len = strlen (sensor_units_buf);
          if (len > column_width->sensor_units)
            column_width->sensor_units = len;
        }
    }
  
  return (0);
}

static int
_store_column_widths_shared (pstdout_state_t pstate,
                             ipmi_sdr_parse_ctx_t sdr_parse_ctx,
                             const void *sdr_record,
                             unsigned int sdr_record_len,
                             unsigned int abbreviated_units,
                             unsigned int count_event_only_records,
                             unsigned int count_device_locator_records,
                             unsigned int count_oem_records,
                             struct sensor_entity_id_counts *entity_id_counts,
                             struct sensor_column_width *column_width)
{
  uint8_t record_type;
  uint8_t share_count;
  uint8_t sensor_number_base;
  int i;

  assert (sdr_parse_ctx);
  assert (sdr_record);
  assert (sdr_record_len);
  assert (column_width);

  if (ipmi_sdr_parse_record_id_and_type (sdr_parse_ctx,
                                         sdr_record,
                                         sdr_record_len,
                                         NULL,
                                         &record_type) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_parse_record_id_and_type: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
      return (-1);
    }

  if (record_type != IPMI_SDR_FORMAT_COMPACT_SENSOR_RECORD
      && record_type != IPMI_SDR_FORMAT_EVENT_ONLY_RECORD)
    {
      if (_store_column_widths (pstate,
                                sdr_parse_ctx,
                                sdr_record,
                                sdr_record_len,
                                NULL,
                                abbreviated_units,
                                count_event_only_records,
                                count_device_locator_records,
                                count_oem_records,
                                entity_id_counts,
                                column_width) < 0)
        return (-1);
      return (0);
    }
  
  if (ipmi_sdr_parse_sensor_record_sharing (sdr_parse_ctx,
                                            sdr_record,
                                            sdr_record_len,
                                            &share_count,
                                            NULL,
                                            NULL,
                                            NULL) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_record_sharing: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
      return (-1);
    }
  
  if (share_count <= 1)
    {
      if (_store_column_widths (pstate,
                                sdr_parse_ctx,
                                sdr_record,
                                sdr_record_len,
                                NULL,
                                abbreviated_units,
                                count_event_only_records,
                                count_device_locator_records,
                                count_oem_records,
                                entity_id_counts,
                                column_width) < 0)
        return (-1);
      return (0);
    }

  if (ipmi_sdr_parse_sensor_number (sdr_parse_ctx,
                                    sdr_record,
                                    sdr_record_len,
                                    &sensor_number_base) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_parse_sensor_number: %s\n",
                       ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
      return (-1);
    }

  
  /* IPMI spec gives the following example:
   *
   * "If the starting sensor number was 10, and the share
   * count was 3, then sensors 10, 11, and 12 would share
   * the record"
   */
  for (i = 0; i < share_count; i++)
    {
      uint8_t sensor_number;
      
      sensor_number = sensor_number_base + i;
      
      if (_store_column_widths (pstate,
                                sdr_parse_ctx,
                                sdr_record,
                                sdr_record_len,
                                &sensor_number,
                                abbreviated_units,
                                count_event_only_records,
                                count_device_locator_records,
                                count_oem_records,
                                entity_id_counts,
                                column_width) < 0)
        return (-1);
    }

  return (0);
}

int
calculate_column_widths (pstdout_state_t pstate,
                         ipmi_sdr_cache_ctx_t sdr_cache_ctx,
                         ipmi_sdr_parse_ctx_t sdr_parse_ctx,
                         char sensor_types[][MAX_SENSOR_TYPES_STRING_LENGTH+1],
                         unsigned int sensor_types_length,
                         unsigned int record_ids[],
                         unsigned int record_ids_length,
                         unsigned int abbreviated_units,
                         unsigned int shared_sensors,
                         unsigned int count_event_only_records,
                         unsigned int count_device_locator_records,
                         unsigned int count_oem_records,
                         struct sensor_entity_id_counts *entity_id_counts,
                         struct sensor_column_width *column_width)
{
  uint8_t sdr_record[IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH];
  int sdr_record_len = 0;
  int rv = -1;
  int i;

  assert (sdr_cache_ctx);
  assert (sdr_parse_ctx);
  assert (column_width);

  _sensor_column_width_init (column_width);

  if (record_ids && record_ids_length)
    {
      for (i = 0; i < record_ids_length; i++)
        {
          if (ipmi_sdr_cache_search_record_id (sdr_cache_ctx, record_ids[i]) < 0)
            {
              if (ipmi_sdr_cache_ctx_errnum (sdr_cache_ctx) == IPMI_SDR_CACHE_ERR_NOT_FOUND)
                continue;
              else
                {
                  PSTDOUT_FPRINTF (pstate,
                                   stderr,
                                   "ipmi_sdr_cache_search_record_id: %s\n",
                                   ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
                  goto cleanup;
                }
            }

          if ((sdr_record_len = ipmi_sdr_cache_record_read (sdr_cache_ctx,
                                                            sdr_record,
                                                            IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH)) < 0)
            {
              PSTDOUT_FPRINTF (pstate,
                               stderr,
                               "ipmi_sdr_cache_record_read: %s\n",
                               ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
              goto cleanup;
            }

          /* Shouldn't be possible */
          if (!sdr_record_len)
            continue;

          if (shared_sensors)
            {
              if (_store_column_widths_shared (pstate,
                                               sdr_parse_ctx,
                                               sdr_record,
                                               sdr_record_len,
                                               abbreviated_units,
                                               count_event_only_records,
                                               count_device_locator_records,
                                               count_oem_records,
                                               entity_id_counts,
                                               column_width) < 0)
                goto cleanup;
            }
          else
            {
              if (_store_column_widths (pstate,
                                        sdr_parse_ctx,
                                        sdr_record,
                                        sdr_record_len,
                                        NULL,
                                        abbreviated_units,
                                        count_event_only_records,
                                        count_device_locator_records,
                                        count_oem_records,
                                        entity_id_counts,
                                        column_width) < 0)
                goto cleanup;
            }
        }
    }
  else
    {
      uint16_t record_count;

      if (ipmi_sdr_cache_record_count (sdr_cache_ctx, &record_count) < 0)
        {
          PSTDOUT_FPRINTF (pstate,
                           stderr,
                           "ipmi_sdr_cache_record_count: %s\n",
                           ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
          goto cleanup;
        }

      for (i = 0; i < record_count; i++, ipmi_sdr_cache_next (sdr_cache_ctx))
        {
          int ret;

          if ((sdr_record_len = ipmi_sdr_cache_record_read (sdr_cache_ctx,
                                                            sdr_record,
                                                            IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH)) < 0)
            {
              PSTDOUT_FPRINTF (pstate,
                               stderr,
                               "ipmi_sdr_cache_record_read: %s\n",
                               ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
              goto cleanup;
            }

          /* Shouldn't be possible */
          if (!sdr_record_len)
            continue;

          if (sensor_types && sensor_types_length)
            {
              if ((ret = _is_sdr_sensor_type_listed (pstate,
                                                     sdr_parse_ctx,
                                                     sdr_record,
                                                     sdr_record_len,
                                                     sensor_types,
                                                     sensor_types_length)) < 0)
                goto cleanup;
            }
          else
            ret = 1;            /* accept all */

          if (ret)
            {
              if (shared_sensors)
                {
                  if (_store_column_widths_shared (pstate,
                                                   sdr_parse_ctx,
                                                   sdr_record,
                                                   sdr_record_len,
                                                   abbreviated_units,
                                                   count_event_only_records,
                                                   count_device_locator_records,
                                                   count_oem_records,
                                                   entity_id_counts,
                                                   column_width) < 0)
                    goto cleanup;
                }
              else
                {
                  if (_store_column_widths (pstate,
                                            sdr_parse_ctx,
                                            sdr_record,
                                            sdr_record_len,
                                            NULL,
                                            abbreviated_units,
                                            count_event_only_records,
                                            count_device_locator_records,
                                            count_oem_records,
                                            entity_id_counts,
                                            column_width) < 0)
                    goto cleanup;
                }
            }
        }
    }

  rv = 0;
  _sensor_column_width_finish (column_width);
 cleanup:
  ipmi_sdr_cache_first (sdr_cache_ctx);
  return (rv);
}

int
calculate_record_ids (pstdout_state_t pstate,
                      ipmi_sdr_cache_ctx_t sdr_cache_ctx,
                      ipmi_sdr_parse_ctx_t sdr_parse_ctx,
                      char sensor_types[][MAX_SENSOR_TYPES_STRING_LENGTH+1],
                      unsigned int sensor_types_length,
                      char exclude_sensor_types[][MAX_SENSOR_TYPES_STRING_LENGTH+1],
                      unsigned int exclude_sensor_types_length,
                      unsigned int record_ids[],
                      unsigned int record_ids_length,
                      unsigned int exclude_record_ids[],
                      unsigned int exclude_record_ids_length,
                      unsigned int output_record_ids[MAX_SENSOR_RECORD_IDS],
                      unsigned int *output_record_ids_length)
{
  uint8_t sdr_record[IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH];
  int sdr_record_len = 0;
  uint16_t record_count;
  uint16_t record_id;
  uint8_t record_type;
  unsigned int i;
  unsigned int j;

  assert (sdr_cache_ctx);
  assert (sdr_parse_ctx);
  assert (sensor_types);
  assert (exclude_sensor_types);
  assert (record_ids);
  assert (exclude_record_ids);
  assert (output_record_ids);
  assert (output_record_ids_length);

  memset (output_record_ids, '\0', sizeof (unsigned int) * MAX_SENSOR_RECORD_IDS);
  (*output_record_ids_length) = 0;

  if (ipmi_sdr_cache_record_count (sdr_cache_ctx,
                                   &record_count) < 0)
    {
      PSTDOUT_FPRINTF (pstate,
                       stderr,
                       "ipmi_sdr_cache_record_count: %s\n",
                       ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
      return (-1);
    }

  /* 
   * achu: This code could be done far more concisely, but it got
   * confusing and hard to understand.  We will divide this up into
   * simple chunks, if the user specified neither record_ids and
   * types, record_ids, or types.  record_ids take precedence over
   * types, and exclude_record_ids takes precedence over
   * exclude_types.
   */

  if (!record_ids_length && !sensor_types_length)
    {
      for (i = 0; i < record_count; i++, ipmi_sdr_cache_next (sdr_cache_ctx))
        {
          if ((sdr_record_len = ipmi_sdr_cache_record_read (sdr_cache_ctx,
                                                            sdr_record,
                                                            IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH)) < 0)
            {
              PSTDOUT_FPRINTF (pstate,
                               stderr,
                               "ipmi_sdr_cache_record_read: %s\n",
                               ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
              return (-1);
            }

          /* Shouldn't be possible */
          if (!sdr_record_len)
            continue;

          if (ipmi_sdr_parse_record_id_and_type (sdr_parse_ctx,
                                                 sdr_record,
                                                 sdr_record_len,
                                                 &record_id,
                                                 &record_type) < 0)
            {
              PSTDOUT_FPRINTF (pstate,
                               stderr,
                               "ipmi_sdr_parse_record_id_and_type: %s\n",
                               ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
              return (-1);
            }

          if (exclude_record_ids_length)
            {
              int found_exclude = 0;
              
              for (j = 0; j < exclude_record_ids_length; j++)
                {
                  if (record_id == exclude_record_ids[j])
                    {
                      found_exclude++;
                      break;
                    }
                }
              
              if (found_exclude)
                continue;
            }

          if (exclude_sensor_types_length)
            {
              int flag;

              if ((flag = _is_sdr_sensor_type_listed (pstate,
                                                      sdr_parse_ctx,
                                                      sdr_record,
                                                      sdr_record_len,
                                                      exclude_sensor_types,
                                                      exclude_sensor_types_length)) < 0)
                return (-1);

              if (flag)
                continue;
            }
          
          output_record_ids[(*output_record_ids_length)] = record_id;
          (*output_record_ids_length)++;
        }
    }
  else if (record_ids_length)
    {
      for (i = 0; i < record_ids_length; i++)
        {
          if (exclude_record_ids_length)
            {
              int found_exclude = 0;
              
              for (j = 0; j < exclude_record_ids_length; j++)
                {
                  if (record_ids[i] == exclude_record_ids[j])
                    {
                      found_exclude++;
                      break;
                    }
                }
              
              if (found_exclude)
                continue;
            }

          if (ipmi_sdr_cache_search_record_id (sdr_cache_ctx, record_ids[i]) < 0)
            {
              if (ipmi_sdr_cache_ctx_errnum (sdr_cache_ctx) == IPMI_SDR_CACHE_ERR_NOT_FOUND)
                {
                  PSTDOUT_PRINTF (pstate,
                                  "Sensor Record ID '%d' not found\n",
                                  record_ids[i]);
                  return (-1);
                }
              else
                {
                  PSTDOUT_FPRINTF (pstate,
                                   stderr,
                                   "ipmi_sdr_cache_search_record_id: %s\n",
                                   ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
                  return (-1);
                }
            }

          if ((sdr_record_len = ipmi_sdr_cache_record_read (sdr_cache_ctx,
                                                            sdr_record,
                                                            IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH)) < 0)
            {
              PSTDOUT_FPRINTF (pstate,
                               stderr,
                               "ipmi_sdr_cache_record_read: %s\n",
                               ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
              return (-1);
            }

          /* Shouldn't be possible */
          if (!sdr_record_len)
            continue;

          if (exclude_sensor_types_length)
            {
              int flag;
          
              if ((flag = _is_sdr_sensor_type_listed (pstate,
                                                      sdr_parse_ctx,
                                                      sdr_record,
                                                      sdr_record_len,
                                                      exclude_sensor_types,
                                                      exclude_sensor_types_length)) < 0)
                return (-1);

              if (flag)
                continue;
            }
          
          output_record_ids[(*output_record_ids_length)] = record_ids[i];
          (*output_record_ids_length)++;
        }
    }
  else /* sensor_types_length */
    {
      for (i = 0; i < record_count; i++, ipmi_sdr_cache_next (sdr_cache_ctx))
        {
          int flag;

          if ((sdr_record_len = ipmi_sdr_cache_record_read (sdr_cache_ctx,
                                                            sdr_record,
                                                            IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH)) < 0)
            {
              PSTDOUT_FPRINTF (pstate,
                               stderr,
                               "ipmi_sdr_cache_record_read: %s\n",
                               ipmi_sdr_cache_ctx_errormsg (sdr_cache_ctx));
              return (-1);
            }

          /* Shouldn't be possible */
          if (!sdr_record_len)
            continue;

          if (ipmi_sdr_parse_record_id_and_type (sdr_parse_ctx,
                                                 sdr_record,
                                                 sdr_record_len,
                                                 &record_id,
                                                 &record_type) < 0)
            {
              PSTDOUT_FPRINTF (pstate,
                               stderr,
                               "ipmi_sdr_parse_record_id_and_type: %s\n",
                               ipmi_sdr_parse_ctx_errormsg (sdr_parse_ctx));
              return (-1);
            }

          if ((flag = _is_sdr_sensor_type_listed (pstate,
                                                  sdr_parse_ctx,
                                                  sdr_record,
                                                  sdr_record_len,
                                                  sensor_types,
                                                  sensor_types_length)) < 0)
            return (-1);

          if (!flag)
            continue;

          if (exclude_record_ids)
            {
              int found_exclude = 0;
              
              for (j = 0; j < exclude_record_ids_length; j++)
                {
                  if (record_id == exclude_record_ids[j])
                    {
                      found_exclude++;
                      break;
                    }
                }
              
              if (found_exclude)
                continue;
            }

          if (exclude_sensor_types_length)
            {
              if ((flag = _is_sdr_sensor_type_listed (pstate,
                                                      sdr_parse_ctx,
                                                      sdr_record,
                                                      sdr_record_len,
                                                      exclude_sensor_types,
                                                      exclude_sensor_types_length)) < 0)
                return (-1);

              if (flag)
                continue;
            }

          output_record_ids[(*output_record_ids_length)] = record_id;
          (*output_record_ids_length)++;
        }
    }

  return (0);
}