/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * oggz_read.c
 *
 * Conrad Parker <conrad@annodex.net>
 */

#include "config.h"

#if OGGZ_CONFIG_READ

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <ogg/ogg.h>

#include "oggz_private.h"

/*#define DEBUG*/

/*#define DEBUG_BY_READING_PAGES*/

#define CHUNKSIZE 8500

OGGZ *
oggz_read_init (OGGZ * oggz)
{
  OggzReader * reader = &oggz->x.reader;

  ogg_sync_init (&reader->ogg_sync);
  ogg_stream_init (&reader->ogg_stream, (int)-1);
  reader->current_serialno = -1;

  reader->read_packet = NULL;
  reader->read_user_data = NULL;

  reader->current_unit = 0;

  return oggz;
}

OGGZ *
oggz_read_close (OGGZ * oggz)
{
  OggzReader * reader = &oggz->x.reader;

  ogg_sync_clear (&reader->ogg_sync);

  return oggz;
}

int
oggz_set_read_callback (OGGZ * oggz, long serialno,
			OggzReadPacket read_packet, void * user_data)
{
  OggzReader * reader;
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  reader =  &oggz->x.reader;

  if (oggz->flags & OGGZ_WRITE) {
    return OGGZ_ERR_INVALID;
  }

  if (serialno == -1) {
    reader->read_packet = read_packet;
    reader->read_user_data = user_data;
  } else {
    stream = oggz_get_stream (oggz, serialno);
    if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

    stream->read_packet = read_packet;
    stream->read_user_data = user_data;
  }

  return 0;
}

#if 1

static off_t
oggz_tell_raw_7 (OGGZ * oggz)
{
  off_t offset_at;

  offset_at = lseek (oggz->fd, 0, SEEK_CUR);
  if (offset_at == -1) {
#if 0
    if (errno == ESPIPE) {
      oggz_set_error (oggz, OGGZ_ENOSEEK);
    } else {
      oggz_set_error (oggz, OGGZ_ESYSTEM);
    }
#endif
    return -1;
  }

  return offset_at;
}

/*
 * oggz_get_next_page_7 (oggz, og, do_read)
 *
 * MODIFIED COPY OF CODE FROM BELOW SEEKING STUFF
 *
 * retrieves the next page.
 * returns >= 0 if found; return value is offset of page start
 * returns -1 on error
 * returns -2 if EOF was encountered
 */
static off_t
oggz_get_next_page_7 (OGGZ * oggz, ogg_page * og)
{
  OggzReader * reader = &oggz->x.reader;
#if _UNMODIFIED
  char * buffer;
#endif
  long bytes = 0, more;
  off_t page_offset = 0, ret;
  int found = 0;

  do {
    more = ogg_sync_pageseek (&reader->ogg_sync, og);

    if (more == 0) {
      page_offset = 0;
#if _UMMODIFIED_
      buffer = ogg_sync_buffer (&reader->ogg_sync, CHUNKSIZE);
      if ((bytes = read (oggz->fd, buffer, CHUNKSIZE)) < 0) {
	/*oggz_set_error (oggz, OGGZ_ESYSTEM);*/
	return -1;
      }

      if (bytes == 0) {
	return -2;
      }

      ogg_sync_wrote(&reader->ogg_sync, bytes);
#else
      return -2;
#endif
    } else if (more < 0) {
#ifdef DEBUG
      printf ("get_next_page: skipped %ld bytes\n", -more);
#endif
      page_offset -= more;
    } else {
#ifdef DEBUG
      printf ("get_next_page: page has %ld bytes\n", more);
#endif
      found = 1;
    }

  } while (!found);

  /* Calculate the byte offset of the page which was found */
  if (bytes > 0) {
    oggz->offset = oggz_tell_raw_7 (oggz) - bytes + page_offset;
    ret = oggz->offset;
  } else {
    /* didn't need to do any reading -- accumulate the page_offset */
    ret = oggz->offset + page_offset;
    oggz->offset += page_offset + more;
  }

  return ret;
}


static long
oggz_read_sync (OGGZ * oggz)
{
  OggzReader * reader = &oggz->x.reader;
  long nread = 0;

  oggz_stream_t * stream;
  ogg_stream_state * os;
  ogg_sync_state * oy;
  ogg_packet * op;
  long serialno;
  ogg_int64_t granulepos = -1;
  int ret;

  ogg_packet packet;
  ogg_page og;

  /*os = &reader->ogg_stream;*/
  op = &packet;

  /* handle one packet.  Try to fetch it from current stream state */
  /* extract packets from page */
  while(1){

    if (reader->current_serialno != -1) {
    /* process a packet if we can.  If the machine isn't loaded,
       neither is a page */
    while(1) {
      ogg_int64_t granulepos;
      int result;

      serialno = reader->current_serialno;

      stream = oggz_get_stream (oggz, serialno);
      
      if (stream == NULL) {
	/* new stream ... check bos etc. */
	if ((stream = oggz_add_stream (oggz, serialno)) == NULL) {
	  /* error -- could not add stream */
	  return -1;
	}
      }
      os = &stream->ogg_stream;
      
      result = ogg_stream_packetout(os, op);

      if(result == -1) {
	/* hole in the data. */
	return -1;
      }

      if(result > 0){
	/* got a packet.  process it */
	granulepos = op->granulepos;

	if ((oggz->metric || stream->metric) && granulepos != -1) {
	  reader->current_unit = oggz_get_unit (oggz, serialno, granulepos);
	}
#ifndef DEBUG_BY_READING_PAGES
	if (stream->read_packet) {
	  stream->read_packet (oggz, op, serialno, stream->read_user_data);
	} else if (reader->read_packet) {
	  reader->read_packet (oggz, op, serialno, reader->read_user_data);
	}
#endif /* DEBUG_BY_READING_PAGES */
      }
      else
	break;
    }
    }

    if(oggz_get_next_page_7 (oggz, &og) < 0)
      return -404; /* eof. leave unitialized */

    serialno = ogg_page_serialno (&og);
    reader->current_serialno = serialno; /* XXX: maybe not necessary */

    stream = oggz_get_stream (oggz, serialno);
      
    if (stream == NULL) {
      /* new stream ... check bos etc. */
      if ((stream = oggz_add_stream (oggz, serialno)) == NULL) {
	/* error -- could not add stream */
	return -1;
      }
    }
    os = &stream->ogg_stream;

#ifdef DEBUG_BY_READING_PAGES
    {
      ogg_packet op_debug;
      op_debug.packet = og.body;
      op_debug.bytes = og.body_len;
      op_debug.b_o_s = ogg_page_bos (&og);
      op_debug.e_o_s = ogg_page_eos (&og);
      op_debug.granulepos = ogg_page_granulepos (&og);
      op_debug.packetno = ogg_page_packets (&og);

      if (stream->read_packet) {
	stream->read_packet (oggz, &op_debug, serialno,
			     stream->read_user_data);
      } else if (reader->read_packet) {
	reader->read_packet (oggz, &op_debug, serialno,
			     reader->read_user_data);
      }
    }
#endif

#if 0
    /* bitrate tracking; add the header's bytes here, the body bytes
       are done by packet above */
    vf->bittrack+=og.header_len*8;
#endif

    ogg_stream_pagein(os, &og);
  }

  return nread; /* XXX: nread isn't even looked at!!! */
}

#else /* 0 */

static long
oggz_read_sync (OGGZ * oggz)
{
  OggzReader * reader = &oggz->x.reader;

  oggz_stream_t * stream;
  ogg_stream_state * os;
  ogg_sync_state * oy;
  ogg_packet * op;
  ogg_page * og;
  long serialno;
  ogg_int64_t granulepos = -1;
  int result;
  long nread = 0;

  if (oggz == NULL) return -1;

  oy = &reader->ogg_sync;
  op = &oggz->current_packet;
  og = &oggz->current_page;

  while ((result = ogg_sync_pageout (oy, og)) == 1) {

    serialno = ogg_page_serialno (og);

    stream = oggz_get_stream (oggz, serialno);

    if (stream == NULL) {
      /* new stream ... check bos etc. */
      if ((stream = oggz_add_stream (oggz, serialno)) == NULL) {
	/* error -- could not add stream */
	break;
      }
    }
    os = &stream->ogg_stream;

    nread += og->body_len;

    if (!ogg_page_continued (og)) {
      granulepos = ogg_page_granulepos (og);
    }

    ogg_stream_pagein (os, og);

    while (ogg_stream_packetout (os, op) == 1) {

      if (op->e_o_s) stream->e_o_s = 1;

      if ((oggz->metric || stream->metric) && granulepos != -1) {
	reader->current_unit = oggz_get_unit (oggz, serialno, granulepos);
      }

      if (stream->read_packet) {
	stream->read_packet (oggz, op, serialno, stream->read_user_data);
      } else if (reader->read_packet) {
	reader->read_packet (oggz, op, serialno, reader->read_user_data);
      }
    }

    oggz->offset += og->header_len + og->body_len;
  }

  return nread;
}
#endif /* 1 */

long
oggz_read (OGGZ * oggz, long n)
{
  OggzReader * reader;
  char * buffer;
  long bytes, bytes_read = 1, remaining = n, nread = 0;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (oggz->flags & OGGZ_WRITE) {
    return OGGZ_ERR_INVALID;
  }

  reader = &oggz->x.reader;

  oggz_read_sync (oggz);

  while (bytes_read > 0 && remaining > 0) {
    bytes = MIN (remaining, 4096);
    buffer = ogg_sync_buffer (&reader->ogg_sync, bytes);
    if ((bytes_read = read (oggz->fd, buffer, bytes)) == -1) {
      return OGGZ_ERR_SYSTEM;
    }

    ogg_sync_wrote (&reader->ogg_sync, bytes_read);
    
    remaining -= bytes_read;
    nread += bytes_read;

    oggz_read_sync (oggz);
  }

  return nread;
}

/* generic */
long
oggz_read_input (OGGZ * oggz, unsigned char * buf, long n)
{
  OggzReader * reader;
  char * buffer;
  long bytes, remaining = n, nread = 0;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (oggz->flags & OGGZ_WRITE) {
    return OGGZ_ERR_INVALID;
  }

  reader = &oggz->x.reader;

  oggz_read_sync (oggz);

  while (/* !oggz->eos && */ remaining > 0) {
    bytes = MIN (remaining, 4096);
    buffer = ogg_sync_buffer (&reader->ogg_sync, bytes);
    memcpy (buffer, buf, bytes);
    ogg_sync_wrote (&reader->ogg_sync, bytes);

    buf += bytes;
    remaining -= bytes;
    nread += bytes;

    oggz_read_sync (oggz);    
  }

  return nread;
}

/* oggz_seek() related functions from here down */

#if 0
/*
 * XXX: how about this instead of data_begins_here ?
 *
 * Then the typical usage is:
 *
 *   oggz_set_data_start (oggz, oggz_tell (oggz));
 *
 * and more general usage is possible, plus the
 * overall usage is more obvious.
 */
int
oggz_set_data_start (OGGZ * oggz, off_t offset)
{
  if (oggz == NULL) return -1;

  if (offset < 0) return -1;

  oggz->offset_data_begin = offset;

  return 0;
}
#endif

int
oggz_data_begins_here (OGGZ * oggz)
{
  if (oggz == NULL) return -1;

  if (oggz->offset < 0) return -1;

  oggz->offset_data_begin = oggz->offset;

  return 0;
}

static off_t
oggz_tell_raw (OGGZ * oggz)
{
  off_t offset_at;

  offset_at = lseek (oggz->fd, 0, SEEK_CUR);
  if (offset_at == -1) {
#if 0
    if (errno == ESPIPE) {
      oggz_set_error (oggz, OGGZ_ENOSEEK);
    } else {
      oggz_set_error (oggz, OGGZ_ESYSTEM);
    }
#endif
    return -1;
  }

  return offset_at;
}

/*
 * seeks and syncs
 */
static off_t
oggz_seek_raw (OGGZ * oggz, off_t offset, int whence)
{
  OggzReader * reader = &oggz->x.reader;
  off_t offset_at;

  offset_at = lseek (oggz->fd, offset, whence);
  if (offset_at == -1) {
#if 0
    if (errno == ESPIPE) {
      oggz_set_error (oggz, OGGZ_ENOSEEK);
    } else {
      oggz_set_error (oggz, OGGZ_ESYSTEM);
    }
#endif
    return -1;
  }

  oggz->offset = offset_at;

  ogg_sync_reset (&reader->ogg_sync);

  return offset_at;
}

static int
oggz_stream_reset (void * data)
{
  oggz_stream_t * stream = (oggz_stream_t *) data;

  if (stream->ogg_stream.serialno != -1) {
    ogg_stream_reset (&stream->ogg_stream);
  }

  return 0;
}

static long
oggz_reset (OGGZ * oggz, off_t offset, ogg_int64_t unit, int whence)
{
  OggzReader * reader = &oggz->x.reader;

  off_t offset_at;

  offset_at = oggz_seek_raw (oggz, offset, whence);
  if (offset_at == -1) return -1;

  oggz->offset = offset_at;

#ifdef DEBUG
  printf ("reset to %ld\n", offset_at);
#endif

  oggz_vector_foreach (&oggz->streams, oggz_stream_reset);

#if 0
  ogg_stream_reset (&oggz->ogg_stream);
  /*  ogg_stream_reset_serialno (&oggz->ogg_stream, oggz->anno_serialno);*/
#endif

  if (unit != -1) reader->current_unit = unit;

  return offset_at;
}

/*
 * oggz_get_next_page (oggz, og, do_read)
 *
 * retrieves the next page.
 * returns >= 0 if found; return value is offset of page start
 * returns -1 on error
 * returns -2 if EOF was encountered
 */
static off_t
oggz_get_next_page (OGGZ * oggz, ogg_page * og)
{
  OggzReader * reader = &oggz->x.reader;
  char * buffer;
  long bytes = 0, more;
  off_t page_offset = 0, ret;
  int found = 0;

  do {
    more = ogg_sync_pageseek (&reader->ogg_sync, og);

    if (more == 0) {
      page_offset = 0;

      buffer = ogg_sync_buffer (&reader->ogg_sync, CHUNKSIZE);
      if ((bytes = read (oggz->fd, buffer, CHUNKSIZE)) < 0) {
	/*oggz_set_error (oggz, OGGZ_ESYSTEM);*/
	return -1;
      }

      if (bytes == 0) {
	return -2;
      }

      ogg_sync_wrote(&reader->ogg_sync, bytes);

    } else if (more < 0) {
#ifdef DEBUG
      printf ("get_next_page: skipped %ld bytes\n", -more);
#endif
      page_offset -= more;
    } else {
#ifdef DEBUG
      printf ("get_next_page: page has %ld bytes\n", more);
#endif
      found = 1;
    }

  } while (!found);

  /* Calculate the byte offset of the page which was found */
  if (bytes > 0) {
    oggz->offset = oggz_tell_raw (oggz) - bytes + page_offset;
    ret = oggz->offset;
  } else {
    /* didn't need to do any reading -- accumulate the page_offset */
    ret = oggz->offset + page_offset;
    oggz->offset += page_offset + more;
  }

  return ret;
}

static off_t
oggz_get_next_start_page (OGGZ * oggz, ogg_page * og)
{
  off_t page_offset;
  int found = 0;

  while (!found) {
    page_offset = oggz_get_next_page (oggz, og);

    /* Return this value if one of the following conditions is met:
     *
     *   page_offset < 0     : error or EOF
     *   page_offset == 0    : start of stream
     *   !ogg_page_continued : start of page
     */
    if (page_offset <= 0 || !ogg_page_continued (og))
      found = 1;
  }

  return page_offset;
}

static off_t
oggz_get_prev_start_page (OGGZ * oggz, ogg_page * og,
			 ogg_int64_t * granule, long * serialno)
{
  off_t offset_at, offset_start;
  off_t page_offset, prev_offset = 0;
  long granule_at = -1;

#if 0
  offset_at = oggz_tell_raw (oggz);
  if (offset_at == -1) return -1;
#else
  offset_at = oggz->offset;
#endif

  offset_start = offset_at;

  do {

    offset_start = offset_at - CHUNKSIZE;
    if (offset_start < 0) offset_start = 0;

    offset_start = oggz_seek_raw (oggz, offset_start, SEEK_SET);
    if (offset_start == -1) return -1;

#ifdef DEBUG

    printf ("[A] offset_at: @%ld\toffset_start: @%ld\n",
	    offset_at, offset_start);

    printf ("*** get_prev_start_page: seeked to %ld\n", offset_start);
#endif

    page_offset = 0;

    do {
      prev_offset = page_offset;

      page_offset = oggz_get_next_start_page (oggz, og);
      if (page_offset == -1) return -1;
      if (page_offset == -2) break;

      granule_at = (long)ogg_page_granulepos (og);

#ifdef DEBUG
      printf ("\tGOT page (%ld) @%ld\tat @%ld\n", granule_at,
	      page_offset, offset_at);
#endif

      /* Need to stash the granule and serialno of this page because og
       * will be overwritten by the time we realise this was the desired
       * prev page */
      if (page_offset >= 0 && page_offset < offset_at) {
	*granule = granule_at;
	*serialno = ogg_page_serialno (og);
      }

    } while (page_offset >= 0 && page_offset < offset_at);

#ifdef DEBUG
    printf ("[B] offset_at: @%ld\toffset_start: @%ld\n"
	    "prev_offset: @%ld\tpage_offset: @%ld\n",
	    offset_at, offset_start, prev_offset, page_offset);
#endif
    /* reset the file offset */
    offset_at = offset_start;

  } while (offset_at > 0 && prev_offset == 0);

  if (offset_at > 0)
    return prev_offset;
  else
    return -1;
}

static off_t
oggz_scan_for_page (OGGZ * oggz, ogg_page * og, ogg_int64_t unit_target,
		   off_t offset_begin, off_t offset_end)
{
  off_t offset_at, offset_next;
  off_t offset_prev = -1;
  ogg_int64_t granule_at;
  ogg_int64_t unit_at;
  long serialno;

#ifdef DEBUG
  printf (" SCANNING from %ld...", offset_begin);
#endif

  while (1) {
    offset_at = oggz_seek_raw (oggz, offset_begin, SEEK_SET);
    if (offset_at == -1) return -1;

#ifdef DEBUG
    printf (" scan @%ld\n", offset_at);
#endif

    offset_next = oggz_get_next_start_page (oggz, og);

    if (offset_next < 0) {
      return offset_next;
    }

    if (offset_next == 0 && offset_begin != 0) {
#ifdef DEBUG
      printf (" ... scanned past EOF\n");
#endif
      return -1;
    }
    if (offset_next > offset_end) {
#ifdef DEBUG
      printf (" ... scanned to page %ld\n", (long)ogg_page_granulepos (og));
#endif
      if (offset_prev != -1) {
	offset_at = oggz_seek_raw (oggz, offset_prev, SEEK_SET);
	if (offset_at == -1) return -1;

	offset_next = oggz_get_next_start_page (oggz, og);
	if (offset_next < 0) return offset_next;

	serialno = ogg_page_serialno (og);
	granule_at = ogg_page_granulepos (og);
	unit_at = oggz_get_unit (oggz, serialno, granule_at);

	return offset_at;
      } else {
	return -1;
      }
    }

    offset_at = offset_next;

    serialno = ogg_page_serialno (og);
    granule_at = ogg_page_granulepos (og);
    unit_at = oggz_get_unit (oggz, serialno, granule_at);

    if (unit_at < unit_target) {
#ifdef DEBUG
      printf (" scan: (%ld) < (%ld)\n", unit_at, unit_target);
#endif
      offset_prev = offset_next;
      offset_begin = offset_next+1;
    } else if (unit_at > unit_target) {
#ifdef DEBUG
      printf (" scan: (%ld) > (%ld)\n", unit_at, unit_target);
#endif
#if 0
      /* hole ? */
      offset_at = oggz_seek_raw (oggz, offset_begin, SEEK_SET);
      if (offset_at == -1) return -1;

      offset_next = oggz_get_next_start_page (oggz, og);
      if (offset_next < 0) return offset_next;

      serialno = ogg_page_serialno (og);
      granule_at = ogg_page_granulepos (og);
      unit_at = oggz_get_unit (oggz, serialno, granule_at);

      break;
#else
      return offset_at;
#endif
    } else if (unit_at == unit_target) {
#ifdef DEBUG
      printf (" scan: (%ld) == (%ld)\n", unit_at, unit_target);
#endif
      break;
    }
  }

  return offset_at;
}

static long
oggz_seek_set (OGGZ * oggz, ogg_int64_t unit_target)
{
  OggzReader * reader = &oggz->x.reader;
  struct stat statbuf;
  off_t offset_orig, offset_at, offset_guess;
  off_t offset_begin, offset_end, offset_next;
  ogg_int64_t granule_at;
  ogg_int64_t unit_at, unit_begin = 0, unit_end = -1;
  long serialno;
  double guess_ratio;
  ogg_page * og;

  if (oggz == NULL) {
    return -1;
  }

  if (unit_target > 0 && oggz->metric == NULL) {
    /* No metric defined */
    return -1;
  }

  if (oggz->fd == -1) {
    /*oggz_set_error (oggz, OGGZ_ENOSEEK);*/
    return -1;
  }

  if (fstat (oggz->fd, &statbuf) == -1) {
    /*oggz_set_error (oggz, OGGZ_ESYSTEM);*/
    return -1;
  }

  if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
    offset_end = statbuf.st_size;
  } else {
    /*oggz_set_error (oggz, OGGZ_ENOSEEK);*/
    return -1;
  }

  if (unit_target == reader->current_unit) {
    return reader->current_unit;
  }

  if (unit_target == 0) {
    offset_at = oggz_reset (oggz, oggz->offset_data_begin, 0, SEEK_SET);
    if (offset_at == -1) return -1;
    return 0;
  }

  offset_at = oggz_tell_raw (oggz);
  if (offset_at == -1) return -1;

#if 0
  offset_orig = offset_at;
#else
  offset_orig = oggz->offset;
#endif

  offset_begin = 0;

  unit_at = reader->current_unit;
  unit_begin = 0;
  unit_end = -1;

  og = &oggz->current_page;

  while (1) {

#ifdef DEBUG
    printf ("oggz_read_seek (%ld): (%ld - %ld) [%ld - %ld]\t",
	    unit_target, unit_begin, unit_end, offset_begin, offset_end);
#endif

    if (unit_end == -1) {
      if (unit_at == unit_begin) {
#ifdef DEBUG
	printf ("*G1*");
#endif
	offset_guess = offset_begin + (offset_end - offset_begin)/2;
      } else {
#ifdef DEBUG
	printf ("*G2*");
#endif
	guess_ratio =
	  (double)(unit_target - unit_begin) /
	  (double)(unit_at - unit_begin);

#ifdef DEBUG
	printf ("\nguess_ration %f = (%ld - %ld) / (%ld - %ld)\n",
		guess_ratio, unit_target, unit_begin, unit_at, unit_begin);
#endif

	offset_guess = offset_begin +
	  (off_t)((offset_at - offset_begin) * guess_ratio);
      }
    } else if (unit_end <= unit_begin) {
#ifdef DEBUG
      printf ("unit_end <= unit_begin\n");
#endif
      break;
    } else {
#if 1
      guess_ratio =
	(double)(unit_target - unit_begin) /
	(double)(unit_end - unit_begin);

      offset_guess = offset_begin +
	(off_t)((offset_end - offset_begin) * guess_ratio);

      /*
      if (offset_guess <= offset_begin) {
	offset_guess = offset_begin + 1;
      }
      */
#else
      offset_guess = offset_begin + (offset_end - offset_begin)/2;
#endif
    }

#ifdef DEBUG
    printf ("%ld ->", offset_guess);
#endif

    offset_at = oggz_seek_raw (oggz, offset_guess, SEEK_SET);
    if (offset_at == -1) {
      goto notfound;
    }

    offset_next = oggz_get_next_start_page (oggz, og);

#ifdef DEBUG
    printf ("\n");
#endif

    if (offset_next < 0) {
      goto notfound;
    }

    if (offset_next > offset_end) {
      offset_next = oggz_scan_for_page (oggz, og, unit_target,
					offset_begin, offset_end);

      if (offset_next < 0) {
	goto notfound;
      }

      offset_at = offset_next;
      serialno = ogg_page_serialno (og);
      granule_at = ogg_page_granulepos (og);

      unit_at = oggz_get_unit (oggz, serialno, granule_at);

      goto found;
    }

    offset_at = offset_next;
    serialno = ogg_page_serialno (og);
    granule_at = ogg_page_granulepos (og);

    unit_at = oggz_get_unit (oggz, serialno, granule_at);

#ifdef DEBUG
    printf ("oggz_read_seek (%ld): got page (%ld) @%ld\n", unit_target,
	    unit_at, offset_at);
#endif

    if (unit_at < unit_target) {
      offset_begin = offset_at;
      unit_begin = unit_at;
    } else if (unit_at > unit_target) {
      offset_end = offset_at-1;
      unit_end = unit_at;
    } else {
      break;
    }


  }

 found:
#ifdef DEBUG
  printf ("FOUND (%ld)\n", unit_at);
#endif

  offset_at = oggz_reset (oggz, offset_at, unit_at, SEEK_SET);
  if (offset_at == -1) return -1;

  return reader->current_unit;

 notfound:
#ifdef DEBUG
  printf ("NOT FOUND\n");
#endif

  oggz_reset (oggz, offset_orig, -1, SEEK_SET);

  return -1;
}

static long
oggz_seek_end (OGGZ * oggz, ogg_int64_t unit_offset)
{
  off_t offset_orig, offset_at, offset_end;
  ogg_int64_t granulepos;
  ogg_int64_t unit_end;
  long serialno;
  ogg_page * og;

  og = &oggz->current_page;

  offset_orig = oggz->offset;

  offset_at = oggz_seek_raw (oggz, 0, SEEK_END);
  if (offset_at == -1) return -1;

  offset_end = oggz_get_prev_start_page (oggz, og, &granulepos, &serialno);

  unit_end = oggz_get_unit (oggz, serialno, granulepos);

  if (offset_end < 0) {
    oggz_reset (oggz, offset_orig, -1, SEEK_SET);
    return -1;
  }
  
#ifdef DEBUG
  printf ("*** oggz_seek_end: found packet (%ld) at @%ld\n",
	  unit_end, offset_end);
#endif

  return oggz_seek_set (oggz, unit_end + unit_offset);
}

off_t
oggz_seek (OGGZ * oggz, off_t offset, int whence)
{
  ogg_int64_t units = -1;

  if (oggz == NULL) return -1;

  if (oggz->flags & OGGZ_WRITE) {
    return -1;
  }

  if (offset == 0) units = 0;

  return oggz_reset (oggz, offset, units, whence);
}

long
oggz_seek_units (OGGZ * oggz, ogg_int64_t units, int whence)
{
  OggzReader * reader = &oggz->x.reader;

  if (oggz == NULL) return -1;

  if (oggz->flags & OGGZ_WRITE) {
    return -1;
  }

  if (oggz->metric == NULL) {
    return -1;
  }

  switch (whence) {
  case SEEK_SET:
    return oggz_seek_set (oggz, units);
    break;
  case SEEK_CUR: 
    units += reader->current_unit;
    return oggz_seek_set (oggz, units);
    break;
  case SEEK_END:
    return oggz_seek_end (oggz, units);
    break;
  default:
    /*oggz_set_error (oggz, OGGZ_EINVALID);*/
    return -1;
    break;
  }
}

long
oggz_seek_byorder (OGGZ * oggz, void * target)
{
  return -1;
}

long
oggz_seek_packets (OGGZ * oggz, long serialno, long packets, int whence)
{
  return -1;
}

#else /* OGGZ_CONFIG_READ */

#include <ogg/ogg.h>
#include "oggz_private.h"

int
oggz_set_read_callback (OGGZ * oggz, long serialno,
			OggzReadPacket read_packet, void * user_data)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_read (OGGZ * oggz, long n)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_read_input (OGGZ * oggz, unsigned char * buf, long n)
{
  return OGGZ_ERR_DISABLED;
}

off_t
oggz_seek (OGGZ * oggz, off_t offset, int whence)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_seek_units (OGGZ * oggz, ogg_int64_t units, int whence)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_seek_byorder (OGGZ * oggz, void * target)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_seek_packets (OGGZ * oggz, long serialno, long packets, int whence)
{
  return OGGZ_ERR_DISABLED;
}

#endif
