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

#ifndef __OGGZ_PRIVATE_H__
#define __OGGZ_PRIVATE_H__

#include <ogg/ogg.h>
#include <oggz/oggz_constants.h>

#include "oggz_vector.h"

/* Use the malloc and free used by ogg; defaults are those from stdlib */
#define oggz_malloc _ogg_malloc
#define oggz_free _ogg_free

#undef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))

#undef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))

#define OGGZ_READ 00
#define OGGZ_WRITE 01

typedef struct _OGGZ OGGZ;
typedef struct _OggzReader OggzReader;
typedef struct _OggzWriter OggzWriter;

typedef int (*OggzReadPacket) (OGGZ * oggz, ogg_packet * op, long serialno,
			       void * user_data);

typedef ogg_int64_t (*OggzMetric) (OGGZ * oggz, long serialno,
				   ogg_int64_t granulepos,
				   void * user_data);

typedef int (*OggzOrder) (OGGZ * oggz, ogg_packet * op, void * target,
			  void * user_data);

typedef int (*OggzWriteHungry) (OGGZ * oggz, int empty, void * user_data);

typedef struct {
  ogg_stream_state ogg_stream;

  /* non b_o_s packet has been written (not just queued) */
  int delivered_non_b_o_s;

  int b_o_s; /* beginning of stream */
  int e_o_s; /* end of stream */
  ogg_int64_t granulepos;
  ogg_int64_t packetno;

  OggzMetric metric;
  void * metric_user_data;
  int metric_internal;

  OggzOrder order;
  void * order_user_data;

  OggzReadPacket read_packet;
  void * read_user_data;
} oggz_stream_t;

struct _OggzReader {
  ogg_sync_state ogg_sync;

  /* XXX: these two can prolly be removed again :) */
  ogg_stream_state ogg_stream;
  long current_serialno;

  OggzReadPacket read_packet;
  void * read_user_data;

  ogg_int64_t current_unit;

#if 0
  off_t offset_page_end; /* offset of end of current page */
#endif
};

/**
 * Bundle a packet with the stream it is being queued for; used in
 * the packet_queue vector
 */
typedef struct {
  ogg_packet op;
  oggz_stream_t * stream;
  int flush;
  int * guard;
} oggz_writer_packet_t;

enum oggz_writer_state {
  OGGZ_MAKING_PACKETS = 0,
  OGGZ_WRITING_PAGES = 1
};

struct _OggzWriter {
  oggz_writer_packet_t * next_zpacket; /* stashed in case of FLUSH_BEFORE */
  OggzVector packet_queue;

  OggzWriteHungry hungry;
  void * hungry_user_data;
  int hungry_only_when_empty;

  int writing; /* already mid-write; check for recursive writes */
  int state; /* OGGZ_MAKING_PACKETS or OGGZ_WRITING_PAGES */

  int flushing; /* whether current packet is being flushed or just paged out */

  int eog; /* end of page */
#if 0
  int eop; /* end of packet */
#endif
  int eos; /* end of stream */

  oggz_writer_packet_t * current_zpacket;

  int packet_offset; /* n bytes already copied out of current packet */
  int page_offset; /* n bytes already copied out of current page */

  ogg_stream_state * current_stream;
};

struct _OGGZ {
  int flags;
  int fd;

  ogg_packet current_packet;
  ogg_page current_page;

  off_t offset; /* offset of current page start */
  off_t offset_data_begin; /* offset of unit 0 page start */

  OggzVector streams;
  int all_at_eos; /* all streams are at eos */

  OggzMetric metric;
  void * metric_user_data;
  int metric_internal;

  OggzOrder order;
  void * order_user_data;

  union {
    OggzReader reader;
    OggzWriter writer;
  } x;
};

OGGZ * oggz_read_init (OGGZ * oggz);
OGGZ * oggz_read_close (OGGZ * oggz);

OGGZ * oggz_write_init (OGGZ * oggz);
OGGZ * oggz_write_close (OGGZ * oggz);

oggz_stream_t * oggz_get_stream (OGGZ * oggz, long serialno);
oggz_stream_t * oggz_add_stream (OGGZ * oggz, long serialno);

int oggz_get_bos (OGGZ * oggz, long serialno);
ogg_int64_t oggz_get_unit (OGGZ * oggz, long serialno, ogg_int64_t granulepos);

#endif /* __OGGZ_PRIVATE_H__ */
