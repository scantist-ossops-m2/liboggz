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

#ifndef __OGGZ_SEEK_H__
#define __OGGZ_SEEK_H__


/** \defgroup auto OGGZ_AUTO
 *
 * \section auto Automatic Metrics
 *
 * Oggz can automatically provide \link metric metrics \endlink for the
 * common media codecs
 * <a href="http://www.speex.org/">Speex</a>,
 * <a href="http://www.vorbis.com/">Vorbis</a>,
 * and <a href="http://www.theora.org/">Theora</a>,
 * as well as all <a href="http://www.annodex.net/">Annodex</a> streams.
 * You need do nothing more than open the file with the OGGZ_AUTO flag set.
 *
 * - Create an OGGZ handle for reading with \a flags = OGGZ_READ | OGGZ_AUTO
 * - Read data, ensuring that you have received all b_o_s pages before
 *   attempting to seek.
 *
 * Oggz will silently parse known codec headers and associate metrics
 * appropriately; if you attempt to seek before you have received all
 * b_o_s pages, Oggz will not have had a chance to parse the codec headers
 * and associate metrics.
 * It is safe to seek once you have received a packet with \a b_o_s == 0;
 * see the \link basics Ogg basics \endlink section for more details.
 *
 * \note This functionality is provided for convenience. Oggz parses
 * these codec headers internally, and so liboggz is \b not linked to
 * libspeex, libvorbis, libtheora or libannodex.
 */

/** \defgroup metric Using OggzMetric
 *
 * \section metric_intro Introduction
 *
 * An OggzMetric is a helper function for Oggz's seeking mechanism.
 *
 * If every position in an Ogg stream can be described by a metric such as
 * time, then it is possible to define a function that, given a serialno and
 * granulepos, returns a measurement in units such as milliseconds. Oggz
 * will use this function repeatedly while seeking in order to navigate
 * through the Ogg stream.
 *
 * The meaning of the units is arbitrary, but must be consistent across all
 * logical bitstreams. This allows Oggz to seek accurately through Ogg
 * bitstreams containing multiple logical bitstreams such as tracks of media.
 *
 * \section setting How to set metrics
 *
 * You don't need to set metrics for Vorbis, Speex, Theora or Annodex.
 * These can be handled \link auto automatically \endlink by Oggz.
 *
 * For most others it is simply a matter of providing a linear multiplication
 * factor (such as a sampling rate, if each packet's granulepos represents a
 * sample number). For non-linear data streams, you will need to explicitly
 * provide your own OggzMetric function.
 *
 * \subsection linear Linear Metrics
 *
 * - Set the \a granule_rate_numerator and \a granule_rate_denominator
 *   appropriately using oggz_set_metric_linear()
 *
 * \subsection custom Custom Metrics
 *
 * For streams with non-linear granulepos, you need to set a custom metric:
 *
 * - Implement an OggzMetric callback
 * - Set the OggzMetric callback using oggz_set_metric()
 *
 * \section using Seeking with OggzMetrics
 *
 * To seek, use oggz_seek_units(). Oggz will perform a ratio search
 * through the Ogg bitstream, using the OggzMetric callback to determine
 * its position relative to the desired unit.
 *
 * \note
 *
 * Many data streams begin with headers describing such things as codec
 * setup parameters. One of the assumptions Monty makes is:
 *
 * - Given pre-cached decode headers, a player may seek into a stream at
 *   any point and begin decode.
 *
 * Thus, the first action taken by applications dealing with such data is
 * to read in and cache the decode headers; thereafter the application can
 * safely seek to arbitrary points in the data.
 *
 * This impacts seeking because the portion of the bitstream containing
 * decode headers should not be considered part of the metric space. To
 * inform Oggz not to seek earlier than the end of the decode headers,
 * use oggz_set_data_start().
 *
 */

/** \defgroup seek_semantics Semantics of seeking in Ogg bitstreams
 *
 * \section seek_semantics_intro Introduction
 *
 * The seeking semantics of the Ogg file format were outlined by Monty in
 * <a href="http://www.xiph.org/archives/theora-dev/200209/0040.html">a
 * post to theora-dev</a> in September 2002. Quoting from that post, we
 * have the following assumptions:
 *
 * - Ogg is not a non-linear format. ... It is a media transport format
 *   designed to do nothing more than deliver content, in a stream, and
 *   have all the pieces arrive on time and in sync.
 * - The Ogg layer does not know the specifics of the codec data it's
 *   multiplexing into a stream. It knows nothing beyond 'Oooo, packets!',
 *   that the packets belong to different buckets, that the packets go in
 *   order, and that packets have position markers. Ogg does not even have
 *   a concept of 'time'; it only knows about the sequentially increasing,
 *   unitless position markers. It is up to higher layers which have
 *   access to the codec APIs to assign and convert units of framing or
 *   time.
 *
 * (For more details on the structure of Ogg streams, see the
 * \link basics Ogg Basics \endlink section).
 *
 * For data such as media, for which it is possible to provide a mapping
 * such as 'time', OGGZ can efficiently navigate through an Ogg stream
 * by use of an OggzMetric callback, thus allowing automatic seeking to
 * points in 'time'.
 *
 * For common codecs you can ask Oggz to set this for you automatically by
 * instantiating the OGGZ handle with the OGGZ_AUTO flag set. For others
 * you can specify a multiplier with oggz_set_metric_linear(), or a generic
 * non-linear metric with oggz_set_metric().
 *
 */

/** \defgroup seek_api OGGZ Seek API
 *
 * Oggz can seek on multitrack, multicodec bitstreams.
 *
 * - If you are expecting to only handle
 *   <a href="http://www.annodex.net/">Annodex</a> bitstreams,
 *   or any combination of <a href="http://www.vorbis.com/">Vorbis</a>,
 *   <a href="http://www.speex.org/">Speex</a> and
 *   <a href="http://www.theora.org/">Theora</a> logical bitstreams, seeking
 *   is built-in to Oggz. See \link auto OGGZ_AUTO \endlink for details.
 *
 * - For other data streams, you will need to provide a metric function;
 *   see the section on \link metric Using OggzMetrics \endlink for details
 *   of setting up and seeking with metrics.
 *
 * - It is always possible to seek to exact byte positions using oggz_seek().
 *
 * - A mechanism to aid seeking across non-metric spaces for which a partial
 *   order exists (ie. data that is not synchronised by a measure such as time,
 *   but is nevertheless somehow seekably structured), is also planned.
 *
 * For a full description of the seeking methods possible in Ogg, see
 * \link seek_semantics Semantics of seeking in Ogg bitstreams \endlink.
 *
 * \{
 */

/**
 * Specify that a logical bitstream has a linear metric
 * \param oggz An OGGZ handle
 * \param serialno Identify the logical bitstream in \a oggz to attach
 * this linear metric to. A value of -1 indicates that the metric should
 * be attached to all unattached logical bitstreams in \a oggz.
 * \param granule_rate_numerator The numerator of the granule rate
 * \param granule_rate_denominator The denominator of the granule rate
 * \returns 0 Success
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 * logical bitstream in \a oggz.
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 */
int oggz_set_metric_linear (OGGZ * oggz, long serialno,
			    ogg_int64_t granule_rate_numerator,
			    ogg_int64_t granule_rate_denominator);

/**
 * This is the signature of a function to correlate Ogg streams.
 * If every position in an Ogg stream can be described by a metric (eg. time)
 * then define this function that returns some arbitrary unit value.
 * This is the normal use of OGGZ for media streams. The meaning of units is
 * arbitrary, but must be consistent across all logical bitstreams; for
 * example a conversion of the time offset of a given packet into nanoseconds
 * or a similar stream-specific subdivision may be appropriate.
 *
 * \param oggz An OGGZ handle
 * \param serialno Identifies a logical bitstream within \a oggz
 * \param granulepos A granulepos within the logical bitstream identified
 *                   by \a serialno
 * \param user_data Arbitrary data you wish to pass to your callback
 * \returns A conversion of the (serialno, granulepos) pair into a measure
 * in units which is consistent across all logical bitstreams within \a oggz
 */
typedef ogg_int64_t (*OggzMetric) (OGGZ * oggz, long serialno,
				   ogg_int64_t granulepos, void * user_data);

/**
 * Set the OggzMetric to use for an OGGZ handle
 *
 * \param oggz An OGGZ handle
 * \param serialno Identify the logical bitstream in \a oggz to attach
 *                 this metric to. A value of -1 indicates that this metric
 *                 should be attached to all unattached logical bitstreams
 *                 in \a oggz.
 * \param metric An OggzMetric callback
 * \param user_data arbitrary data to pass to the metric callback
 *
 * \returns 0 Success
 * \retval OGGZ_ERR_BAD_SERIALNO \a serialno does not identify an existing
 *                               logical bitstream in \a oggz, and is not -1
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 *
 * \note Specifying values of \a serialno other than -1 allows you to pass
 *       logical bitstream specific user_data to the same metric.
 * \note Alternatively, you may use a different \a metric for each
 *       \a serialno, but all metrics used must return mutually consistent
 *       unit measurements.
 */
int oggz_set_metric (OGGZ * oggz, long serialno, OggzMetric metric,
		     void * user_data);

/**
 * Query the current offset in units corresponding to the Metric function
 * \param oggz An OGGZ handle
 * \returns the offset in units
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
ogg_int64_t oggz_tell_units (OGGZ * oggz);

/**
 * Seek to a number of units corresponding to the Metric function
 * \param oggz An OGGZ handle
 * \param units A number of units
 * \param whence As defined in <stdio.h>: SEEK_SET, SEEK_CUR or SEEK_END
 * \returns the new file offset, or -1 on failure.
 */
ogg_int64_t oggz_seek_units (OGGZ * oggz, ogg_int64_t units, int whence);

#ifdef _UNIMPLEMENTED
/** \defgroup order OggzOrder
 *
 * \subsection OggzOrder
 *
 * Suppose there is a partial order < and a corresponding equivalence
 * relation = defined on the space of packets in the Ogg stream of 'OGGZ'.
 * An OggzOrder simply provides a comparison in terms of '<' and '=' for
 * ogg_packets against a target.
 *
 * To use OggzOrder:
 *
 * - Implement an OggzOrder callback
 * - Set the OggzOrder callback for an OGGZ handle with oggz_set_order()
 * - To seek, use oggz_seek_byorder(). Oggz will use a combination bisection
 *   search and scan of the Ogg bitstream, using the OggzOrder callback to
 *   match against the desired 'target'.
 *
 * Otherwise, for more general ogg streams for which a partial order can be
 * defined, define a function matching this specification.
 *
 * Parameters:
 *
 *     OGGZ: the OGGZ object
 *     op:  an ogg packet in the stream
 *     target: a user defined object
 *
 * Return values:
 *
 *    -1 , if 'op' would occur before the position represented by 'target'
 *     0 , if the position of 'op' is equivalent to that of 'target'
 *     1 , if 'op' would occur after the position represented by 'target'
 *     2 , if the relationship between 'op' and 'target' is undefined.
 *
 * Symbolically:
 *
 * Suppose there is a partial order < and a corresponding equivalence
 * relation = defined on the space of packets in the Ogg stream of 'OGGZ'.
 * Let p represent the position of the packet 'op', and t be the position
 * represented by 'target'.
 *
 * Then a function implementing OggzPacketOrder should return as follows:
 *
 *    -1 , p < t
 *     0 , p = t
 *     1 , t < p
 *     2 , otherwise
 *
 * Hacker's hint: if there are no circumstances in which you would return
 * a value of 2, there is a linear order; it may be possible to define a
 * Metric rather than an Order.
 *
 */
typedef int (*OggzOrder) (OGGZ * oggz, ogg_packet * op, void * target,
			 void * user_data);
/**
 * \retval 0 Success
 * \retval OGGZ_ERR_BAD_OGGZ \a oggz does not refer to an existing OGGZ
 * \retval OGGZ_ERR_INVALID Operation not suitable for this OGGZ
 */
int oggz_set_order (OGGZ * oggz, long serialno, OggzOrder order,
		    void * user_data);

long oggz_seek_byorder (OGGZ * oggz, void * target);

#endif /* _UNIMPLEMENTED */

#endif /* __OGGZ_SEEK_H__ */
