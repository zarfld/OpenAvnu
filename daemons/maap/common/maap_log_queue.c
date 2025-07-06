/*************************************************************************************
Copyright (c) 2016-2017, Harman International Industries, Incorporated
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS LISTED "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS LISTED BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Use standard C99 integer types - Visual Studio 2013+ supports these */
#include <stdint.h>
#include <inttypes.h>

#include "maap_log_queue.h"

#define MAAP_LOG_COMPONENT "Queue"
#include "maap_log.h"

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

struct maap_log_queue_elem {
	int setFlg;
	void *data;
};

struct maap_log_queue {
	// Size of each element
	uint32_t elemSize;

	// Number of queue element slots
	uint32_t queueSize;

	// Next element to be filled
	uint32_t head;

	// Next element to be pulled
	uint32_t tail;

	maap_log_queue_elem_t elemArray;
};

maap_log_queue_t maapLogQueueNewQueue(uint32_t elemSize, uint32_t queueSize)
{
	maap_log_queue_t retQueue;

	if (elemSize < 1 || queueSize < 1)
		return NULL;

	retQueue = calloc(1, sizeof(struct maap_log_queue));
	if (retQueue) {
		retQueue->elemArray = calloc(queueSize, sizeof(struct maap_log_queue_elem));
		if (retQueue->elemArray) {
			uint32_t i1;
			for (i1 = 0; i1 < queueSize; i1++) {
				retQueue->elemArray[i1].data = calloc(1, elemSize);
				if (!retQueue->elemArray[i1].data) {
					maapLogQueueDeleteQueue(retQueue);
					return NULL;
				}
			}
		}
		else {
			maapLogQueueDeleteQueue(retQueue);
			return NULL;
		}

		retQueue->elemSize = elemSize;
		retQueue->queueSize = queueSize;
		retQueue->head = 0;
		retQueue->tail = 0;
	}

	return retQueue;
}

void maapLogQueueDeleteQueue(maap_log_queue_t queue)
{
	if (queue) {
		uint32_t i1;
		for (i1 = 0; i1 < queue->queueSize; i1++) {
			free(queue->elemArray[i1].data);
			queue->elemArray[i1].data = NULL;
		}
		free(queue->elemArray);
		queue->elemArray = NULL;
		free(queue);
	}
}

uint32_t maapLogQueueGetQueueSize(maap_log_queue_t queue)
{
	if (queue) {
		return queue->queueSize;
	}
	return 0;
}

uint32_t maapLogQueueGetElemCount(maap_log_queue_t queue)
{
	uint32_t cnt = 0;
	if (queue) {
		if (queue->head > queue->tail) {
			cnt += queue->head - queue->tail - 1;
		}
		else if (queue->head < queue->tail) {
			cnt += queue->head + ((queue->queueSize - 1) - queue->tail);
		}

		if (queue->elemArray[queue->tail].setFlg) {
			cnt++;
		}
	}
	return cnt;
}

uint32_t maapLogQueueGetElemSize(maap_log_queue_t queue)
{
	if (queue) {
		return queue->elemSize;
	}
	return 0;
}

void *maapLogQueueData(maap_log_queue_elem_t elem)
{
	if (elem) {
		return elem->data;
	}
	return NULL;
}

maap_log_queue_elem_t maapLogQueueHeadLock(maap_log_queue_t queue)
{
	if (queue) {
		if (!queue->elemArray[queue->head].setFlg) {
			return &queue->elemArray[queue->head];
		}
	}
	return NULL;
}

void maapLogQueueHeadUnlock(maap_log_queue_t queue)
{
}

void maapLogQueueHeadPush(maap_log_queue_t queue)
{
	if (queue) {
		queue->elemArray[queue->head++].setFlg = TRUE;
		if (queue->head >= queue->queueSize) {
			queue->head = 0;
		}
	}
}

maap_log_queue_elem_t maapLogQueueTailLock(maap_log_queue_t queue)
{
	if (queue) {
		if (queue->elemArray[queue->tail].setFlg) {
			return &queue->elemArray[queue->tail];
		}
	}
	return NULL;
}

void maapLogQueueTailUnlock(maap_log_queue_t queue)
{
}

void maapLogQueueTailPull(maap_log_queue_t queue)
{
	if (queue) {
		queue->elemArray[queue->tail++].setFlg = FALSE;
		if (queue->tail >= queue->queueSize) {
			queue->tail = 0;
		}
	}
}
