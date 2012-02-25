/*****************************************************************************\
 *  $Id: ipmipower_util.c,v 1.37 2010-02-08 22:02:31 chu11 Exp $
 *****************************************************************************
 *  Copyright (C) 2007-2012 Lawrence Livermore National Security, LLC.
 *  Copyright (C) 2003-2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Albert Chu <chu11@llnl.gov>
 *  UCRL-CODE-155698
 *
 *  This file is part of Ipmipower, a remote power control utility.
 *  For details, see http://www.llnl.gov/linux/.
 *
 *  Ipmipower is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  Ipmipower is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Ipmipower.  If not, see <http://www.gnu.org/licenses/>.
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#if STDC_HEADERS
#include <string.h>
#include <stdarg.h>
#endif /* STDC_HEADERS */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else  /* !TIME_WITH_SYS_TIME */
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else /* !HAVE_SYS_TIME_H */
#include <time.h>
#endif  /* !HAVE_SYS_TIME_H */
#endif /* !TIME_WITH_SYS_TIME */
#include <sys/poll.h>
#include <assert.h>
#include <errno.h>

#include "ipmipower_util.h"
#include "ipmipower_error.h"

#include "freeipmi-portability.h"
#include "cbuf.h"

extern struct ipmipower_arguments cmd_args;

int
ipmipower_poll (struct pollfd *ufds, unsigned int nfds, int timeout)
{
  int n;
  struct timeval tv, tv_orig;
  struct timeval start, end, delta;

  /* prep for EINTR handling */
  if (timeout >= 0)
    {
      /* poll uses timeout in milliseconds */
      tv_orig.tv_sec = (long)timeout/1000;
      tv_orig.tv_usec = (timeout % 1000) * 1000;

      if (gettimeofday(&start, NULL) < 0)
        {
          IPMIPOWER_ERROR (("gettimeofday: %s", strerror (errno)));
          exit (1);
        }
    }
  else
    {
      tv_orig.tv_sec = 0;
      tv_orig.tv_usec = 0;
    }

  /* repeat poll if interrupted */
  do
    {
      n = poll(ufds, nfds, timeout);

      /* unrecov error */
      if (n < 0 && errno != EINTR)
        {
          IPMIPOWER_ERROR (("poll: %s", strerror (errno)));
          exit (1);
        }

      if (n < 0 && timeout >= 0)      /* EINTR - adjust timeout */
        {
          if (gettimeofday(&end, NULL) < 0)
            {
              IPMIPOWER_ERROR (("gettimeofday: %s", strerror (errno)));
              exit (1);
            }

          timersub(&end, &start, &delta);     /* delta = end-start */
          timersub(&tv_orig, &delta, &tv);    /* tv = tvsave-delta */
          timeout = (tv.tv_sec * 1000) + (tv.tv_usec/1000);
        }
    } while (n < 0);

  return n;
}

void
ipmipower_cbuf_printf(cbuf_t cbuf, const char *fmt, ...)
{
  char buf[IPMIPOWER_OUTPUT_BUFLEN];
  va_list ap;
  int written, dropped;
  int len;

  va_start(ap, fmt);

  /* overflow ignored */
  len = vsnprintf (buf, IPMIPOWER_OUTPUT_BUFLEN, fmt, ap);
  
  written = cbuf_write (cbuf, buf, len, &dropped);
  if (written < 0)
    {
      IPMIPOWER_ERROR (("cbuf_write: %s", strerror (errno)));
      exit (1);
    }
  
  va_end(ap);
}

int
ipmipower_cbuf_peek_and_drop (cbuf_t buf, void *buffer, int len)
{
  int rv, r_len, dropped = 0;

  assert (buf);
  assert (buffer);
  assert (len > 0);

  if ((r_len = cbuf_peek (buf, buffer, len)) < 0)
    {
      IPMIPOWER_ERROR (("cbuf_peek: %s", strerror (errno)));
      exit (1);
    }

  /* Nothing there */
  if (!r_len)
    return (0);

  if ((dropped = cbuf_drop (buf, len)) < 0)
    {
      IPMIPOWER_ERROR (("cbuf_drop: %s", strerror (errno)));
      exit (1);
    }

  if (dropped != r_len)
    {
      IPMIPOWER_ERROR (("cbuf_drop: dropped incorrect bytes: %d", dropped));
      exit (1);
    }

  if ((rv = cbuf_drop (buf, -1)) < 0)
    {
      IPMIPOWER_ERROR (("cbuf_drop: %s", strerror (errno)));
      exit (1);
    }

  if (rv > 0)
    IPMIPOWER_DEBUG (("cbuf_drop dropped data: %d", rv));
  
  return (r_len);
}