/*
 * queue.h -- Queue Data Structure
 *
 * Copyright (c) 2008-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: $
 */

#ifndef _QUEUE_DEFINED
#define _QUEUE_DEFINED
typedef struct _QUEUE _QUEUE;
typedef _QUEUE *QUEUE;
#endif

#ifndef _LPQUEUE_DEFINED
#define _LPQUEUE_DEFINED
typedef QUEUE *LPQUEUE;
#endif



#ifndef FACILITY_QUEUE
#define FACILITY_QUEUE                       99
#endif

#ifndef E_OUTOFQUEUEBUFFERS
#define E_OUTOFQUEUEBUFFERS \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_QUEUE, 0x1)
#endif

#ifndef E_INVALIDQUEUEBUFFER
#define E_INVALIDQUEUEBUFFER \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_QUEUE, 0x2)
#endif

#ifndef E_ALREADYINSERTEDINTOQUEUE
#define E_ALREADYINSERTEDINTOQUEUE \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_QUEUE, 0x3)
#endif



ACK_RESULT CreateBufferQueue(LPQUEUE pQueue,int iInitialCount,int iGrowBy,int iBufferSize);
ACK_RESULT DestroyBufferQueue(QUEUE queue);
ACK_RESULT QueuePopBuffer(QUEUE queue,LPBYTE** pppBuffer,SIZE_T* pSizeData);
ACK_RESULT QueuePushBuffer(QUEUE queue,LPBYTE* ppBuffer,SIZE_T sizeData);
//ACK_RESULT QueueFreeBuffer(LPBYTE pBuffer);
