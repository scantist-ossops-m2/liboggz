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

#ifndef __OGGZ_VECTOR_H__
#define __OGGZ_VECTOR_H__

typedef struct _OggzVector OggzVector;

typedef int (*OggzFunc) (void * data);
typedef int (*OggzFindFunc) (void * data, long serialno);
typedef int (*OggzCmpFunc) (void * a, void * b, void * user_data);

struct _OggzVector {
  int max_elements;
  int nr_elements;
  void ** data;
  size_t sizeof_element;
  OggzCmpFunc compare;
  void * compare_user_data;
};

OggzVector *
oggz_vector_init (OggzVector * vector, size_t sizeof_element);

void
oggz_vector_clear (OggzVector * vector);

void *
oggz_vector_find (OggzVector * vector, OggzFindFunc func, long serialno);

int
oggz_vector_foreach (OggzVector * vector, OggzFunc func);

/**
 * Add an element to a vector. If the vector has a comparison function,
 * the new element is inserted in sorted order, otherwise it is appended
 * to the tail.
 * \param vector An OggzVector
 * \param data The new element to add
 * \retval data If the element was successfully added
 * \retval NULL If adding the element failed due to a realloc() error
 */
void *
oggz_vector_add_element (OggzVector * vector, void * data);

int
oggz_vector_set_cmp (OggzVector * vector, OggzCmpFunc compare,
		     void * user_data);

void *
oggz_vector_pop (OggzVector * vector);

#endif /* __OGGZ_VECTOR_H__ */
