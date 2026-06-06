/*
 * transport.c -- Transport Layer via Sockets
 *
 * Copyright (c) 2009-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: $
 */

#include <Winsock2.h>
#include <Ws2tcpip.h>

#include "navajoPort.h"
#include "navajo.h"
#include "navajoProtocol.h"
#include "transport.h"
#include "phone.h"
#include "queue.h"
#include "puke.h"
#include "tinCan.h"
#include "resource.h"

#if defined(__SYMBIAN32__)
#define TEST_DATABASE_PATH				"E:\\"
#elif defined(_WIN32_WCE)
#define TEST_DATABASE_PATH				"\\Storage Card\\"
#else
#define TEST_DATABASE_PATH				"X:\\"
#endif

static ACK_RESULT KickOffSendReceive();
static DWORD WINAPI XmitThread(LPVOID pVoid);
static DWORD WINAPI RcveThread(LPVOID pVoid);

struct sockaddr_in addressRemote;
SOCKET theSocket = 0;

QUEUE queueAvailable;
QUEUE queueEncrypt;

HANDLE hXmitThread = 0;
/*
 * NOTE: bXmitThreadRunning and bRcveThreadRunning are read by the
 *       worker threads and written by the main thread on shutdown;
 *       'volatile' prevents the compiler from caching the read in a
 *       register across iterations of the while-loop bodies.  XmitEvent
 *       is published from the main thread; the volatile qualifier here
 *       guards a similar publish/consume pattern.
 *
 * NOTE: theSocket is published exactly once from InitializeTransport
 *       before either worker thread is created and is invalidated
 *       only after both workers have stopped, so the TOCTOU pattern
 *       at DialRemote / AnswerRemote entry is single-threaded in
 *       practice.  Marking it volatile here is defense-in-depth.
 */
volatile BOOL bXmitThreadRunning = FALSE;
volatile HANDLE XmitEvent = NULL;
HANDLE hRcveThread = 0;
volatile BOOL bRcveThreadRunning = FALSE;
CRITICAL_SECTION CritSect;
ACKP_PROVIDER hProvider = NULL;

unsigned char HeartbeatPkt[]	= { 0x01 };
unsigned char ConnectPkt[]		= { 0x80, 0x00, 0x00, 0x00, 0x00 };
unsigned char AcceptPkt[]		= { 0x40, 0x00, 0x00, 0x00, 0x00 };
unsigned char DataPkt[]			= { 0x20 };
unsigned char TerminatePkt[]	= { 0x10 };

unsigned int iTransmited = 0;
unsigned int iTransmitDropped = 1;	//Prevent division by zero.
unsigned int iReceived = 0;
unsigned int iReceivedDropped = 1;	//Prevent division by zero.
static DWORD WINAPI XmitThread(LPVOID pVoid)
{
	USES_RTN;
	WSADATA WSAData;
	ACP_SESSION hEncryptSession;
	int index;
	int count;
	ACK_LPKEYINFO* pKeyInfo = NULL;
	CHAR szEncKeyId[ACK_KEYINFO_MAXID] = {0};

	RTN_IF_FAILED(ACKP_GetAllKeys(hProvider,&count,&pKeyInfo));
	for (index = 0; index < count; index++)
		if (pKeyInfo[index]->isEncrypt)
		{
			memcpy(&szEncKeyId[0],pKeyInfo[index]->keyId,ACK_KEYINFO_MAXID);
			break;
		}
	ACK_free(pKeyInfo);
	pKeyInfo = NULL;

	//Create streaming session.
	RTN_IF_FAILED(ACP_CreateSession(&hEncryptSession,(ACK_SESSIONTYPE)(ACKST_Stream | ACKST_Encrypt),hProvider));
	//Set encryption key to use.
	RTN_IF_FAILED(ACP_SetSessionKey(hEncryptSession,szEncKeyId));

	//Initialize WinSock for this thread.
	RTN_IF_FAILED(HRESULT_FROM_WIN32(WSAStartup(MAKEWORD(1,1),&WSAData)));

	while (bXmitThreadRunning)
	{
		switch (WaitForSingleObject(XmitEvent,1000))
		{
			case WAIT_TIMEOUT:
				//Timed out - send a heartbeat packet if we are connected.
				//send(theSocket,(const char*) &(HeartbeatPkt[0]),sizeof(HeartbeatPkt),0);
				//NOTE:This is disabled because we are not filtering empty packets
				//out, so we always have something to send.
				break;
			case WAIT_OBJECT_0:
			{
				ACK_RESULT res = S_OK;
				LPBYTE* ppBuffer;
				SIZE_T sizeBuffer;

				do
				{
					if (!bXmitThreadRunning)
						break;

					//Get the next buffer.
					res = QueuePopBuffer(queueEncrypt,&ppBuffer,&sizeBuffer);
					if (SUCCEEDED(res))
					{
						LPBYTE* ppEncryptedBuffer;
						SIZE_T sizeEncryptedBuffer;
						SIZE_T sizeSave;

						//Encrypt it.
						RTN_IF_FAILED(QueuePopBuffer(queueAvailable,&ppEncryptedBuffer,&sizeEncryptedBuffer));
						sizeSave = sizeEncryptedBuffer;
						EnterCriticalSection(&CritSect);
						if (SUCCEEDED(ACP_Encrypt(hEncryptSession,0,*ppBuffer,sizeBuffer,ppEncryptedBuffer,&sizeEncryptedBuffer)))
						{
							if (sizeEncryptedBuffer >= sizeSave)
								ACK_realloc(ppEncryptedBuffer,sizeEncryptedBuffer + 1,FALSE);
							(*ppEncryptedBuffer)[sizeEncryptedBuffer] = (*ppEncryptedBuffer)[0];
							sizeEncryptedBuffer = sizeEncryptedBuffer + 1;
							(*ppEncryptedBuffer)[0] = DataPkt[0];

							//Send it.  This may block...
							RTN_IF_SOCKET_ERROR(sendto(theSocket,(const char*)*ppEncryptedBuffer,sizeEncryptedBuffer,
								0,(struct sockaddr*) &(addressRemote),sizeof(addressRemote)));
							iTransmited++;
						}
						else
							iTransmitDropped++;
						LeaveCriticalSection(&CritSect);

						//Push it back into the available queue.
						RTN_IF_FAILED(QueuePushBuffer(queueAvailable,ppBuffer,-1));
						RTN_IF_FAILED(QueuePushBuffer(queueAvailable,ppEncryptedBuffer,-1));
					}
					else
						if (res != E_OUTOFQUEUEBUFFERS)
							RTN_IF_FAILED(res);
				} while (res == S_OK);
				break;
			}
			default:
				break;
		}
	}

	//Flood the socket with terminate packets.
	for (index = 0; index < 100; index++)
	{
		RTN_IF_SOCKET_ERROR(sendto(theSocket,(const char*) &(TerminatePkt[0]),sizeof(TerminatePkt),
			0,(struct sockaddr*) &(addressRemote),sizeof(addressRemote)));
	}

	BEGIN_RTN_CLEAN_UP
//		SetStatus(ghWnd, TEXT("Transmit thread terminated."));

		ACP_CloseSession(hEncryptSession);
		//Terminate WinSock for this thread.
		WSACleanup();
	END_RTN_CLEAN_UP;
}

static DWORD WINAPI RcveThread(LPVOID pVoid)
{
	USES_RTN;
	WSADATA WSAData;
	ACP_SESSION hDecryptSession;
	int iFrameCount = 0;
	DWORD dwFrameStartTime = 0;

	//Initialize WinSock for this thread.
	RTN_IF_FAILED(HRESULT_FROM_WIN32(WSAStartup(MAKEWORD(1,1),&WSAData)));

	//Create streaming session.
	RTN_IF_FAILED(ACP_CreateSession(&hDecryptSession,(ACK_SESSIONTYPE)(ACKST_Stream | ACKST_Decrypt),hProvider));

	while (bRcveThreadRunning)
	{
		int iReceivedBytes;
		LPBYTE* ppEncryptedBuffer;
		SIZE_T sizeEncryptedBuffer;
		fd_set fds;
		int status;
		struct timeval timeout = {0};

		timeout.tv_usec = 100000;
		FD_ZERO(&fds);
		FD_SET(theSocket,&fds);
		//Check the link to see if there is a packet available for read.
		status = select(1,&fds,NULL,NULL,&timeout);
		RTN_IF_SOCKET_ERROR(status);
		if ((status==0) || (!(FD_ISSET(theSocket,&fds))))
			continue;

		//Get next transmission.
		RTN_IF_FAILED(QueuePopBuffer(queueAvailable,&ppEncryptedBuffer,&sizeEncryptedBuffer));
		//Recieve should not block since we just did a select.
		iReceivedBytes = recv(theSocket,*ppEncryptedBuffer,sizeEncryptedBuffer,0);
		RTN_IF_SOCKET_ERROR(iReceivedBytes);

		if (*ppEncryptedBuffer[0] == DataPkt[0])
		{
			//Currently we have a voice packet every 25 ms.  Every quarter of a second,
			//we should be able to process ten packets.  If we drifted off of that mark
			//make up the difference by dropping extra packets
			if ((GetTickCount() - dwFrameStartTime) < 250)
			{
				LPBYTE* ppBuffer;
				SIZE_T sizeBuffer;

				(*ppEncryptedBuffer)[0] = (*ppEncryptedBuffer)[iReceivedBytes - 1];

				//Decrypt it.
				RTN_IF_FAILED(QueuePopBuffer(queueAvailable,&ppBuffer,&sizeBuffer));
				EnterCriticalSection(&CritSect);
				if (SUCCEEDED(ACP_Decrypt(hDecryptSession,0,*ppEncryptedBuffer,iReceivedBytes - 1,ppBuffer,&sizeBuffer)))
				{
					//Post it to sound thread.
					LPBYTE ptr = ACK_malloc(sizeBuffer);
					memcpy(ptr,*ppBuffer,sizeBuffer);
					PostToWaveOut(ptr, sizeBuffer);
				}
				LeaveCriticalSection(&CritSect);

				//Push it back into the available queue.
				RTN_IF_FAILED(QueuePushBuffer(queueAvailable,ppBuffer,-1));
				iReceived++;
			}
			else
				iReceivedDropped++;
			if (++iFrameCount > 10)
			{
				iFrameCount = 0;
				dwFrameStartTime = GetTickCount();

				//Display some stats regarding the connection (about every quarter of a second.
				SetStatus(ghWnd, TEXT("Xmit:%d(%d%%) Rcvd:%d(%d%%)"),iTransmited,iTransmitDropped * 100 / (iTransmited + iTransmitDropped),
																		iReceived,iReceivedDropped * 100 / (iReceived + iReceivedDropped));
				//UpdateWindow(ghWnd);
			}
		}
		else if (*ppEncryptedBuffer[0] == TerminatePkt[0])
		{
			PostMessage(ghWnd,WM_COMMAND,ID_HANGUP,0);
		}

		//Push it back into the available queue.
		RTN_IF_FAILED(QueuePushBuffer(queueAvailable,ppEncryptedBuffer,-1));
	}

	BEGIN_RTN_CLEAN_UP
//		SetStatus(ghWnd, TEXT("Receive thread terminated."));

		ACP_CloseSession(hDecryptSession);
		//Terminate WinSock for this thread.
		WSACleanup();
	END_RTN_CLEAN_UP;
}

static ACK_RESULT KickOffSendReceive()
{
	USES_RTN;

	//Kick off transmit thread.
	XmitEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	RTN_IF_BADPTR(XmitEvent);
	bXmitThreadRunning = TRUE;						//Let the thread run.
	hXmitThread = CreateThread(NULL,				//Ptr to Security Attributes (must be NULL in CE)
								0,					//Use default stack size
								&XmitThread,		//Ptr to the ThreadProc entry point
								NULL,				//Parameter to the ThreadProc
								0,					//Flags => CREATE_READY, USE_DEFAULT_STACK
								NULL);				//<receives the ThreadId on creation>
	if (hXmitThread)
	{
		DWORD dwExitCode;
		if (GetExitCodeThread(hXmitThread,&dwExitCode))
		{
			bXmitThreadRunning = (dwExitCode == STILL_ACTIVE) ? TRUE : FALSE;
			//XXXXXX:CeSetThreadPriority(hXmitThread, THREAD_PRI_VOICE_XMIT);
		}
	}
	else
		bXmitThreadRunning = FALSE;

	//Kick off receive thread.
	bRcveThreadRunning = TRUE;						//Let the thread run.
	hRcveThread = CreateThread(NULL,				//Ptr to Security Attributes (must be NULL in CE)
								0,					//Use default stack size
								&RcveThread,		//Ptr to the ThreadProc entry point
								NULL,				//Parameter to the ThreadProc
								0,					//Flags => CREATE_READY, USE_DEFAULT_STACK
								NULL);				//<receives the ThreadId on creation>
	if (hRcveThread)
	{
		DWORD dwExitCode;
		if (GetExitCodeThread(hRcveThread,&dwExitCode))
		{
			bRcveThreadRunning = (dwExitCode == STILL_ACTIVE) ? TRUE : FALSE;
			//XXXXXX:CeSetThreadPriority(hRcveThread, THREAD_PRI_VOICE_RCVE);
		}
	}
	else
		bRcveThreadRunning = FALSE;

	BEGIN_RTN_CLEAN_UP
	END_RTN_CLEAN_UP;
}

ACK_RESULT InitializeTransport()
{
	USES_RTN;

	if (hXmitThread)
		return ACT_E_IN_USE;

	//Initialize queues.
	RTN_IF_FAILED(CreateBufferQueue(&queueAvailable,2,10,400));
	RTN_IF_FAILED(CreateBufferQueue(&queueEncrypt,0,0,400));

	InitializeCriticalSection(&CritSect);
	RTN_IF_FAILED(ACKP_CreateProvider(&hProvider));

	//Scan the specified path for keys.
	RTN_IF_FAILED(ACKP_ScanPathForKeys(hProvider,TEST_DATABASE_PATH));

	BEGIN_RTN_CLEAN_UP
	END_RTN_CLEAN_UP;
}

ACK_RESULT TerminateTransport()
{
	USES_RTN;

	if (hXmitThread)
	{
		//Signal the thread to stop, then wait for the thread to terminate.
		bXmitThreadRunning = FALSE;
		SetEvent(XmitEvent);

		if (WaitForSingleObject(hXmitThread, THREAD_TIMEOUT) != WAIT_OBJECT_0)
			TerminateThread(hXmitThread, EXITCODE_TERMINATE);

		if (XmitEvent)
		{
			CloseHandle(XmitEvent);
			XmitEvent = NULL;
		}
		CloseHandle(hXmitThread);
		hXmitThread = NULL;
	}

	if (hRcveThread)
	{
		//Signal the thread to stop, then wait for the thread to terminate.
		bRcveThreadRunning = FALSE;

		if (WaitForSingleObject(hRcveThread, THREAD_TIMEOUT) != WAIT_OBJECT_0)
			TerminateThread(hRcveThread, EXITCODE_TERMINATE);

		CloseHandle(hRcveThread);
		hRcveThread = NULL;
	}

	if (theSocket)
	{
		RTN_IF_SOCKET_ERROR(closesocket(theSocket));
		theSocket = 0;
	}

	ACKP_CloseProvider(hProvider);
	hProvider = NULL;
	DeleteCriticalSection(&CritSect);

	//Free up memory allocated by queue.
	RTN_IF_FAILED(DestroyBufferQueue(queueAvailable));
	RTN_IF_FAILED(DestroyBufferQueue(queueEncrypt));

	BEGIN_RTN_CLEAN_UP
	END_RTN_CLEAN_UP;
}

ACK_RESULT DialRemote(
    LPCSTR host,
    LPCSTR port
    )
{
	USES_RTN;
	BYTE buffer[10];
	struct sockaddr_in localAddress;
	int iAddressLength;
	int iTriesLeft;
	int iStrayLeft;
	BOOL bDialCompleted = FALSE;

	if (theSocket != 0)
		RTN_IF_FAILED(E_STARTED);

	SetStatus(ghWnd, TEXT("DIAL: Starting..."));

	//Create the socket.
	theSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	RTN_IF_INVALID_SOCKET(theSocket);

	//Bind to the local VOIP port we will be receiving packets on.
	ZeroMemory(&localAddress,sizeof(localAddress));
	localAddress.sin_family = AF_INET;
	localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddress.sin_port = htons(atoi(port));

	SetStatus(ghWnd, TEXT("DIAL: Binding socket..."));
	RTN_IF_SOCKET_ERROR(bind(theSocket,(struct sockaddr*) &localAddress,sizeof(localAddress)));
	SetStatus(ghWnd, TEXT("DIAL: Socket bound."));

	//Increase the send and receive buffers to 128K.
	//ZEHBRA>>>setsockopt()

	//Try receiving a few times in case we get stray packets.  Bound both
	//the real-retry counter and a separate stray-packet counter so a flood
	//of non-Connect packets cannot extend the dial phase indefinitely.
	iTriesLeft = 1000;
	iStrayLeft = 1000;
	while (iTriesLeft--)
	{
		int iReceivedBytes;
		unsigned long ipHost = inet_addr(host);
		fd_set fds;
		int status;
		struct timeval timeout = {0};

		timeout.tv_usec = 100000;
		FD_ZERO(&fds);
		FD_SET(theSocket,&fds);
		//Check the link to see if there is a packet available for read.
		SetStatus(ghWnd, TEXT("DIAL: Checking for packet... %d"), iTriesLeft);
		status = select(1,&fds,NULL,NULL,&timeout);
		RTN_IF_SOCKET_ERROR(status);
		if ((status==0) || (!(FD_ISSET(theSocket,&fds))))
			continue;

		iAddressLength = sizeof(addressRemote);
		ZeroMemory(&addressRemote,sizeof(addressRemote));
		addressRemote.sin_family = AF_INET;

		SetStatus(ghWnd, TEXT("DIAL: Receiving packet... %d"), iTriesLeft);
		iReceivedBytes = recvfrom(theSocket,buffer,sizeof(buffer),0,(struct sockaddr*) &addressRemote,&iAddressLength);
		SetStatus(ghWnd, TEXT("DIAL: Received packet.  %d"), iTriesLeft);
		RTN_IF_SOCKET_ERROR(iReceivedBytes);

		//Check the received packet to see if it is the Connect packet, which includes my IP address for verification.
		if ((iReceivedBytes == sizeof(ConnectPkt)) && 
			(buffer[0] == ConnectPkt[0]) &&
			(memcmp(&buffer[1],(LPBYTE) &ipHost,sizeof(ipHost)) == 0))
		{
			int index;

			//Send an accept packet with their IP address for verification.
			AcceptPkt[1] = (addressRemote.sin_addr.S_un.S_addr) & 0xFF;
			AcceptPkt[2] = (addressRemote.sin_addr.S_un.S_addr >> 8) & 0xFF;
			AcceptPkt[3] = (addressRemote.sin_addr.S_un.S_addr >> 16) & 0xFF;
			AcceptPkt[4] = (addressRemote.sin_addr.S_un.S_addr >> 24) & 0xFF;

			for (index = 0;index < 100;index++)
			{
				SetStatus(ghWnd, TEXT("DIAL: Sending response... %d -> %d"), iTriesLeft, index);
				RTN_IF_SOCKET_ERROR(sendto(theSocket,(const char*)&(AcceptPkt[0]),sizeof(AcceptPkt),
					0,(struct sockaddr*) &(addressRemote),sizeof(addressRemote)));
				SetStatus(ghWnd, TEXT("DIAL: Sent response. %d -> %d"), iTriesLeft, index);
			}

			RTN_IF_FAILED(KickOffSendReceive())
			//Dialing is complete.
			bDialCompleted = TRUE;
			break;
		}
		else
		{
			//Not a ConnectPkt so don't count it as part of the try, but
			//do consume a stray-packet credit so a peer cannot keep us
			//here indefinitely with non-Connect traffic.
			if (--iStrayLeft <= 0)
				break;
			iTriesLeft++;
		}
	}

	BEGIN_RTN_CLEAN_UP
		if (SUCCEEDED(hrRes))
			SetStatus(ghWnd, TEXT("DIAL: Done."));
		else
			SetStatus(ghWnd, TEXT("DIAL: Failed %08X."),hrRes);

		if (!bDialCompleted)
		{
			closesocket(theSocket);
			theSocket = 0;
		}
	END_RTN_CLEAN_UP;
}

ACK_RESULT AnswerRemote(
    LPCSTR localHost,
    LPCSTR remoteHost,
    LPCSTR port
    )
{
	USES_RTN;
	BYTE buffer[10];
	struct sockaddr_in localAddress;
	ULONG remoteIPAddress = 0;
	int iTriesLeft;
	int iStrayLeft;
	BOOL bAnswerCompleted = FALSE;

	if (theSocket != 0)
		RTN_IF_FAILED(E_STARTED);

	SetStatus(ghWnd, TEXT("ANSWER: Starting..."));

	//Create the socket.
	theSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	RTN_IF_INVALID_SOCKET(theSocket);

	//Bind to the local VOIP port we will be receiving packets on.
	ZeroMemory(&localAddress,sizeof(localAddress));
	localAddress.sin_family = AF_INET;
	localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddress.sin_port = htons(atoi(port));

	SetStatus(ghWnd, TEXT("ANSWER: Binding socket..."));
	RTN_IF_SOCKET_ERROR(bind(theSocket,(struct sockaddr*) &localAddress,sizeof(localAddress)));
	SetStatus(ghWnd, TEXT("ANSWER: Socket bound."));

	//Increase the send and receive buffers to 128K.
	//ZEHBRA>>>setsockopt()

	//Set up the remote address using the VOIP port.
	remoteIPAddress = inet_addr(remoteHost);
	ZeroMemory(&addressRemote,sizeof(addressRemote));
	addressRemote.sin_family = AF_INET;
	addressRemote.sin_addr.S_un.S_addr = remoteIPAddress;
	addressRemote.sin_port = htons(atoi(port));

	//Send connection request using the party's address as verification.
	ConnectPkt[1] = (remoteIPAddress) & 0xFF;
	ConnectPkt[2] = (remoteIPAddress >> 8) & 0xFF;
	ConnectPkt[3] = (remoteIPAddress >> 16) & 0xFF;
	ConnectPkt[4] = (remoteIPAddress >> 24) & 0xFF;

	//Try receiving a few times in case we get stray packets.  Bound both
	//the real-retry counter and a separate stray-packet counter so a flood
	//of non-Accept packets cannot extend the answer phase indefinitely.
	iTriesLeft = 1000;
	iStrayLeft = 1000;
	while (iTriesLeft--)
	{
		int iReceivedBytes;
		unsigned long ipHost = inet_addr(localHost);
		fd_set fds;
		int status;
		struct timeval timeout = {0};

		SetStatus(ghWnd, TEXT("ANSWER: Sending request... %d"), iTriesLeft);
		RTN_IF_SOCKET_ERROR(sendto(theSocket,(const char*) &(ConnectPkt[0]),sizeof(ConnectPkt),
			0,(struct sockaddr*) &(addressRemote),sizeof(addressRemote)));
		SetStatus(ghWnd, TEXT("ANSWER: Sent request. %d"), iTriesLeft);

		timeout.tv_usec = 100000;
		FD_ZERO(&fds);
		FD_SET(theSocket,&fds);
		//Check the link to see if there is a packet available for read.
		SetStatus(ghWnd, TEXT("ANSWER: Checking for packet... %d"),iTriesLeft);
		status = select(1,&fds,NULL,NULL,&timeout);
		RTN_IF_SOCKET_ERROR(status);
		if ((status==0) || (!(FD_ISSET(theSocket,&fds))))
			continue;

		SetStatus(ghWnd, TEXT("ANSWER: Receiving response... %d"), iTriesLeft);
		iReceivedBytes = recv(theSocket,buffer,sizeof(buffer),0);
		SetStatus(ghWnd, TEXT("ANSWER: Received response. %d"), iTriesLeft);
		RTN_IF_SOCKET_ERROR(iReceivedBytes);

		//Check the received packet to see if it is the Accept packet, which includes my IP address for verification.
		if ((iReceivedBytes == sizeof(AcceptPkt)) && 
			(buffer[0] == AcceptPkt[0]) &&
			(memcmp(&buffer[1],(LPBYTE) &ipHost,sizeof(ipHost)) == 0))
		{
			RTN_IF_FAILED(KickOffSendReceive())
			//Answering is complete.
			bAnswerCompleted = TRUE;
			break;
		}
		else
		{
			//Not an AcceptPkt so don't count it as part of the try, but
			//do consume a stray-packet credit so a peer cannot keep us
			//here indefinitely with non-Accept traffic.
			if (--iStrayLeft <= 0)
				break;
			iTriesLeft++;
		}
	}

	BEGIN_RTN_CLEAN_UP
		if (SUCCEEDED(hrRes))
			SetStatus(ghWnd, TEXT("ANSWER: Done."));
		else
			SetStatus(ghWnd, TEXT("ANSWER: Failed %08X."),hrRes);

		if (!bAnswerCompleted)
		{
			closesocket(theSocket);
			theSocket = 0;
		}
	END_RTN_CLEAN_UP;
}

ACK_RESULT TransmitBuffer(LPBYTE pBuffer,SIZE_T length)
{
	USES_RTN;
	LPBYTE* ppBuffer = NULL;
	SIZE_T sizeBuffer;

	//The sound is being captured regardless of an existing connection; so if
	//the XmitEvent has not been created yet (no connection), skip processing
	//the input.
	if (XmitEvent)
	{
		//Get an available buffer.
		RTN_IF_FAILED(QueuePopBuffer(queueAvailable,&ppBuffer,&sizeBuffer));
		//Make sure it is big enough.
		if (sizeBuffer < length)
		{
			ACK_free(*ppBuffer);
			*ppBuffer = ACK_malloc(length);
			if (*ppBuffer)
			{
				//If memory allocation failed, don't put buffer back into available queue.
				ppBuffer = NULL;
				RTN_IF_BADNEW(*ppBuffer);
			}
		}
		//Copy input.
		memcpy(*ppBuffer,pBuffer,length);

		//Push it into the to-be-encrypted queue and signal its availability.
		RTN_IF_FAILED(QueuePushBuffer(queueEncrypt,ppBuffer,length));
		ppBuffer = NULL;
		SetEvent(XmitEvent);
	}
	
	BEGIN_RTN_CLEAN_UP
		//Return buffer into available pool.
		if (ppBuffer)
			RTN_IF_FAILED(QueuePushBuffer(queueAvailable,ppBuffer,-1));
	END_RTN_CLEAN_UP;
}
