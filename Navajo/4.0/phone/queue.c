/*
 * queue.c -- Queue Data Structure
 *
 * Copyright (c) 2009-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: $
 */

#include "navajoport.h"
#include "navajo.h"
#include "hresult.h"
#include "puke.h"
#include "queue.h"

#define QUEUE_MAGIC			0xEE240B41
#define QUEUE_COOKIE		0x318A20B7

typedef struct ELEMENT_tag {
	struct ELEMENT_tag* pNextElement;		//Link to the next element in the list.
	int iCookie;
	SIZE_T sizeData;
	LPBYTE pBuffer[0];
} ELEMENT, *LPELEMENT;

typedef struct QUEUEINFO_tag {
	int magic;
	int iGrowBy;							//Grow queue length by this amount.
	int iBufferSize;						//Size of each buffer in a queue.
	CRITICAL_SECTION CritSect;				//Critical section to protect a queue.
	LPELEMENT pHeadElement;					//Pointer to the head of a queue.
	LPELEMENT pTailElement;					//Pointer to the tail of a queue.
} QUEUEINFO, *LPQUEUEINFO;

/******************************************************************************
** Grow a queue of buffers by the requested size.
******************************************************************************/
ACK_RESULT GrowQueue(LPQUEUEINFO pQueueInfo,int iGrowBy)
{
	USES_RTN;
	int index;
	LPELEMENT pElement;

	for (index = 0;index < iGrowBy;index++)
	{
		//Create new element.  Note that *pElement->pBuffer (the stored
		//pointer) must be tested, not pElement->pBuffer (the address of
		//the flexible array, which is always non-NULL).  If the inner
		//allocation fails the outer element must be freed before exit.
		pElement = ACK_malloc(sizeof(ELEMENT) + sizeof(LPBYTE));
		RTN_IF_BADNEW(pElement);
		pElement->pNextElement = NULL;
		pElement->iCookie = QUEUE_COOKIE;
		*pElement->pBuffer = ACK_malloc(pQueueInfo->iBufferSize);
		if (*pElement->pBuffer == NULL) {
			ACK_free(pElement);
			pElement = NULL;
			hrRes = E_OUTOFMEMORY;
			goto cleanup;
		}
		pElement->sizeData = pQueueInfo->iBufferSize;

		if (pQueueInfo->pHeadElement == NULL)
			pQueueInfo->pHeadElement = pElement;				//Create the head element.
		else
			pQueueInfo->pTailElement->pNextElement = pElement;	//Add it to the tail.
		pQueueInfo->pTailElement = pElement;
	}

	BEGIN_RTN_CLEAN_UP
	END_RTN_CLEAN_UP;
}

/******************************************************************************
** Create a queue of buffers with at least iInitialCount buffers of size
** iBufferSize.  And optionaly increase the size of a queue with iGrowBy.
******************************************************************************/
ACK_RESULT CreateBufferQueue(LPQUEUE pQueue,int iInitialCount,int iGrowBy,int iBufferSize)
{
	USES_RTN;
	QUEUEINFO* pQueueInfo = NULL;

	RTN_IF_BADPTR(pQueue);
	*pQueue = NULL;

	//Create and initialize a queue.
	pQueueInfo = ACK_zalloc(sizeof(QUEUEINFO));
	RTN_IF_BADNEW(pQueueInfo);
	pQueueInfo->magic = QUEUE_MAGIC;
	pQueueInfo->iGrowBy = iGrowBy;
	pQueueInfo->iBufferSize = iBufferSize;
	InitializeCriticalSection(&pQueueInfo->CritSect);

	//Create the initial element count.
	RTN_IF_FAILED(GrowQueue(pQueueInfo,iInitialCount));

	*pQueue = (QUEUE) pQueueInfo;
	pQueueInfo = NULL;

	BEGIN_RTN_CLEAN_UP
		ACK_free(pQueueInfo);
	END_RTN_CLEAN_UP;
}

/******************************************************************************
** Free a queue of buffers.
******************************************************************************/
ACK_RESULT DestroyBufferQueue(QUEUE queue)
{
	USES_RTN;
	QUEUEINFO* pQueueInfo = (LPQUEUEINFO) queue;

	RTN_IF_BADHANDLE(pQueueInfo);
	RTN_IF_BADMAGIC(pQueueInfo->magic,QUEUE_MAGIC);

	while (pQueueInfo->pHeadElement)
	{
		LPELEMENT pElement = pQueueInfo->pHeadElement;
		pQueueInfo->pHeadElement = pQueueInfo->pHeadElement->pNextElement;
		ACK_free(*pElement->pBuffer);
		ACK_free(pElement);
	}
	pQueueInfo->pTailElement = NULL;
	DeleteCriticalSection(&pQueueInfo->CritSect);
	ACK_free(pQueueInfo);

	BEGIN_RTN_CLEAN_UP
	END_RTN_CLEAN_UP;
}

/******************************************************************************
** Pop off a buffer from a queue.  When finished with the buffer, push the
** buffer back into a queue via PushBuffer or free the buffer via
** QueueFreeBuffer.
******************************************************************************/
ACK_RESULT QueuePopBuffer(QUEUE queue,LPBYTE** pppBuffer,SIZE_T* pSizeData)
{
	USES_RTN;
	QUEUEINFO* pQueueInfo = (LPQUEUEINFO) queue;
	LPELEMENT pElement;

	RTN_IF_BADHANDLE(pQueueInfo);
	RTN_IF_BADMAGIC(pQueueInfo->magic,QUEUE_MAGIC);
	RTN_IF_BADPTR(pppBuffer);
	RTN_IF_BADPTR(pSizeData);
	*pppBuffer = NULL;
	*pSizeData = 0;

	EnterCriticalSection(&pQueueInfo->CritSect);
	//If there are no elements left, increase a queue's size.
	if (pQueueInfo->pHeadElement == NULL)
	{
		if (pQueueInfo->iGrowBy)
			RTN_IF_FAILED(GrowQueue(pQueueInfo,pQueueInfo->iGrowBy))
		else
			RTN_IF_FAILED(E_OUTOFQUEUEBUFFERS);
	}

	//Remove the element that is to be returned from a queue.
	pElement = pQueueInfo->pHeadElement;
	pQueueInfo->pHeadElement = pQueueInfo->pHeadElement->pNextElement;
	if (pQueueInfo->pHeadElement == NULL)
		pQueueInfo->pTailElement = NULL;

	//Make sure element does not point to any "next" elements.
	//The next element will be itself, so we can sanity check during push.
	pElement->pNextElement = pElement;
	//Return the actual size of the data.
	*pSizeData = pElement->sizeData;
	//Adjust the pointer to point to the data area and return that.
	pElement++;
	*pppBuffer = (LPBYTE*) pElement;

	BEGIN_RTN_CLEAN_UP
		LeaveCriticalSection(&pQueueInfo->CritSect);
	END_RTN_CLEAN_UP;
}

/******************************************************************************
** Add back into a queue a buffer that was previously poped off.
******************************************************************************/
ACK_RESULT QueuePushBuffer(QUEUE queue,LPBYTE* ppBuffer,SIZE_T sizeData)
{
	USES_RTN;
	QUEUEINFO* pQueueInfo = (LPQUEUEINFO) queue;
	LPELEMENT pElement = (LPELEMENT) ppBuffer;

	RTN_IF_BADHANDLE(pQueueInfo);
	RTN_IF_BADMAGIC(pQueueInfo->magic,QUEUE_MAGIC);
	RTN_IF_BADPTR(pElement);

	//Adjust the pointer to point to the header area.
	pElement--;
	//Make sure it is a valid queue buffer that a queue created previously.
	//If you bomb on the next statement, you are trying to push in a buffer
	//that was not created by a queue.
	if (pElement->iCookie != QUEUE_COOKIE)
		return E_INVALIDQUEUEBUFFER;
	//Make sure we haven't already inserted this queue buffer.
	//If we haven't done so, the next element should point to itself.
	if (pElement->pNextElement != pElement)
		return E_ALREADYINSERTEDINTOQUEUE;

	//Reset next pointer to NULL (since we are going to be the tail).
	pElement->pNextElement = NULL;
	//Actual size of the data.
	if (sizeData == -1)
		pElement->sizeData = pQueueInfo->iBufferSize;
	else
		pElement->sizeData = sizeData;

	//Insert the element back into queue at the tail end.
	EnterCriticalSection(&pQueueInfo->CritSect);
	if (pQueueInfo->pHeadElement == NULL)
		pQueueInfo->pHeadElement = pElement;				//Create the head element.
	else
		pQueueInfo->pTailElement->pNextElement = pElement;	//Add it to the tail.
	pQueueInfo->pTailElement = pElement;
	LeaveCriticalSection(&pQueueInfo->CritSect);

	BEGIN_RTN_CLEAN_UP
	END_RTN_CLEAN_UP;
}

/******************************************************************************
** Free a queue buffer that you don't want to put back into a queue.
******************************************************************************/
ACK_RESULT QueueFreeBuffer(LPBYTE* *ppBuffer)
{
	USES_RTN;
	LPELEMENT pElement = (LPELEMENT) ppBuffer;

	RTN_IF_BADPTR(pElement);

	//Adjust the pointer to point to the header area.
	pElement--;
	//Make sure it is a valid queue buffer that a queue created previously.
	//If you bomb on the next statement, you are trying to push in a buffer
	//that was not created by a queue.
	if (pElement->iCookie != QUEUE_COOKIE)
		return E_INVALIDQUEUEBUFFER;
	//Make sure we haven't already inserted this queue buffer.
	//If we haven't done so, the next element should point to itself.
	if (pElement->pNextElement != pElement)
		return E_ALREADYINSERTEDINTOQUEUE;

	//Free the buffer.
	ACK_free(pElement);

	BEGIN_RTN_CLEAN_UP
	END_RTN_CLEAN_UP;
}
