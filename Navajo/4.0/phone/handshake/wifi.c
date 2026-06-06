/*
 * wifi.c --
 *
 * Copyright (c) 2009-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#if !defined(_WIN32_WCE)
#error "This file can only be compiled on Windows CE."
#endif

#if !defined(STRICT)
#define STRICT
#endif

#include <windows.h>
#include <winsock.h>

#if defined(FEATURE_RAS)
#include <ras.h>
//#include <raserror.h>
#endif /* FEATURE_RAS */

#include "hresult.h"

#include "navajoPort.h"
#include "navajo.h"
#include "navajoProtocol.h"

#include "phone.h"
#include "puke.h"
#include "tinCan.h"
#include "handshake.h"

#define HANDSHAKE_WIFI_PORT				"54321"

#if defined(FEATURE_WIFI)
static DWORD WINAPI WiFiBroadcastThreadProc(LPVOID pParameter);
static DWORD WINAPI WiFiSendThreadProc(LPVOID pParameter);
static DWORD WINAPI WiFiReadThreadProc(LPVOID pParameter);

static HANDLE hBroadcastThread = NULL;
static HANDLE hSendThread = NULL;
static HANDLE hReadThread = NULL;

static DWORD broadcastThreadId = 0;
static DWORD sendThreadId = 0;
static DWORD readThreadId = 0;

static HANDLE hShutdownBroadcast = NULL;
static HANDLE hShutdownRead = NULL;

SOCKET theBrodcastSocket = 0;

static HANDSHAKESENDINFO gHandshakeBroadcastInfo = {0};
HRESULT StartupWiFiBroadcastThread(ACK_LPMUTEX pMutex,LPHANDSHAKESENDINFO pHandshakeSendInfo)
{
	USES_RTN;
	struct sockaddr_in localAddress;
	int iEnalbeBroadcast = TRUE;

	if ((pMutex == NULL) || (pHandshakeSendInfo == NULL))
		return E_POINTER;

	if (theBrodcastSocket != 0)
		RTN_IF_FAILED(E_STARTED);

	gHandshakeBroadcastInfo = *pHandshakeSendInfo;

	EnterCriticalSection(pMutex);

	//Create the socket.
	theBrodcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	RTN_IF_INVALID_SOCKET(theBrodcastSocket);

	//Bind to the local handshake port we will be receiving packets on.
	ZeroMemory(&localAddress,sizeof(localAddress));
	localAddress.sin_family = AF_INET;
	localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddress.sin_port = htons(atoi(HANDSHAKE_WIFI_PORT));

	SetStatus(ghWnd, TEXT("WIFI: Binding socket..."));
	RTN_IF_SOCKET_ERROR(bind(theBrodcastSocket,(struct sockaddr*) &localAddress,sizeof(localAddress)));
	RTN_IF_SOCKET_ERROR(setsockopt(theBrodcastSocket,SOL_SOCKET,SO_BROADCAST,(LPCSTR) &iEnalbeBroadcast,sizeof(iEnalbeBroadcast)));
	SetStatus(ghWnd, TEXT("WIFI: Socket bound."));

	if (hBroadcastThread != NULL) {
		RTN_IF_FAILED(E_STARTED);
	}

	if (hShutdownBroadcast == NULL) {
		hShutdownBroadcast = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (hShutdownBroadcast == NULL) {
			RTN_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
		}
	}

	hBroadcastThread = CreateThread(NULL, 0, WiFiBroadcastThreadProc, &gHandshakeBroadcastInfo, 0, &broadcastThreadId);

	if (hBroadcastThread == NULL) {
		RTN_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
	}

	BEGIN_RTN_CLEAN_UP
		LeaveCriticalSection(pMutex);
	END_RTN_CLEAN_UP;
}

static HANDSHAKESENDINFO gHandshakeSendInfo = {0};
HRESULT StartupWiFiSendThread(ACK_LPMUTEX pMutex,LPHANDSHAKESENDINFO pHandshakeSendInfo)
{
	HRESULT hResult = S_OK;

	if ((pMutex == NULL) || (pHandshakeSendInfo == NULL))
		return E_POINTER;

	if (theBrodcastSocket == 0)
		return E_POINTER;

	gHandshakeSendInfo = *pHandshakeSendInfo;

	EnterCriticalSection(pMutex);

	if (hSendThread != NULL) {
		hResult = E_STARTED;
		goto done;
	}

	hSendThread = CreateThread(NULL, 0, WiFiSendThreadProc, &gHandshakeSendInfo, 0, &sendThreadId);

	if (hSendThread == NULL) {
		hResult = HRESULT_FROM_WIN32(GetLastError());
		goto done;
	}

done:
	LeaveCriticalSection(pMutex);

	return hResult;
}

static HANDSHAKEREADINFO gHandshakeReadInfo = {0};
HRESULT StartupWiFiReadThread(ACK_LPMUTEX pMutex,LPHANDSHAKEREADINFO pHandshakeReadInfo)
{
	HRESULT hResult = S_OK;

	if ((pMutex == NULL) || (pHandshakeReadInfo == NULL))
		return E_POINTER;

	if (theBrodcastSocket == 0)
		return E_POINTER;

	gHandshakeReadInfo = *pHandshakeReadInfo;

	EnterCriticalSection(pMutex);

	if (hReadThread != NULL) {
		hResult = E_STARTED;
		goto done;
	}

	if (hShutdownRead == NULL) {
		hShutdownRead = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (hShutdownRead == NULL) {
			hResult = HRESULT_FROM_WIN32(GetLastError());
			goto done;
		}
	}

	hReadThread = CreateThread(NULL, 0, WiFiReadThreadProc, &gHandshakeReadInfo, 0, &readThreadId);

	if (hReadThread == NULL) {
		hResult = HRESULT_FROM_WIN32(GetLastError());
		goto done;
	}

done:
	LeaveCriticalSection(pMutex);

	return hResult;
}

HRESULT ShutdownWiFiThreads(ACK_LPMUTEX pMutex)
{
	HRESULT hResult = S_OK;

	if (pMutex == NULL)
		return E_POINTER;

	EnterCriticalSection(pMutex);

	if (hBroadcastThread != NULL) {
		if (hShutdownBroadcast != NULL) {
			SetEvent(hShutdownBroadcast);

			if (WaitForSingleObject(hBroadcastThread, THREAD_TIMEOUT) != WAIT_OBJECT_0)
				TerminateThread(hBroadcastThread, EXITCODE_TERMINATE);

			CloseHandle(hShutdownBroadcast);
			hShutdownBroadcast = NULL;
		}

		CloseHandle(hBroadcastThread);
		hBroadcastThread = NULL;
	}

	if (hSendThread != NULL) {
		if (WaitForSingleObject(hSendThread, THREAD_TIMEOUT) != WAIT_OBJECT_0)
			TerminateThread(hSendThread, EXITCODE_TERMINATE);

		CloseHandle(hSendThread);
		hSendThread = NULL;
	}

	if (hReadThread != NULL) {
		if (hShutdownRead != NULL) {
			SetEvent(hShutdownRead);

			if (WaitForSingleObject(hReadThread, THREAD_TIMEOUT) != WAIT_OBJECT_0)
				TerminateThread(hReadThread, EXITCODE_TERMINATE);

			CloseHandle(hShutdownRead);
			hShutdownRead = NULL;
		}

		CloseHandle(hReadThread);
		hReadThread = NULL;
	}

	closesocket(theBrodcastSocket);
	theBrodcastSocket = 0;

	LeaveCriticalSection(pMutex);

	return hResult;
}

LPTSTR AssembleMessage(LPHANDSHAKESENDINFO pHandshakeSendInfo,LPCTSTR lpszEncKeyName)
{
	SIZE_T messageSize = 0;
	LPTSTR pMessage = NULL;

	if (pHandshakeSendInfo->protocol != NULL)
		messageSize += _tcslen(pHandshakeSendInfo->protocol);

	if (pHandshakeSendInfo->version != NULL)
		messageSize += _tcslen(pHandshakeSendInfo->version);

	if (pHandshakeSendInfo->host != NULL)
		messageSize += _tcslen(pHandshakeSendInfo->host);

	if (pHandshakeSendInfo->port != NULL)
		messageSize += _tcslen(pHandshakeSendInfo->port);

	if (lpszEncKeyName != NULL)
		messageSize += _tcslen(lpszEncKeyName);

	messageSize += 4 * _tcslen(TEXT(HANDSHAKE_FIELD_SEPARATOR));
	messageSize++; /* NOTE: Null terminated. */

	pMessage = (LPTSTR)ACK_zalloc(sizeof(TCHAR) * messageSize);
	if (pMessage)
	{
		if (pHandshakeSendInfo->protocol != NULL)
			_tcscat(pMessage, pHandshakeSendInfo->protocol);

		_tcscat(pMessage, TEXT(HANDSHAKE_FIELD_SEPARATOR));

		if (pHandshakeSendInfo->version != NULL)
			_tcscat(pMessage, pHandshakeSendInfo->version);

		_tcscat(pMessage, TEXT(HANDSHAKE_FIELD_SEPARATOR));

		if (pHandshakeSendInfo->host != NULL)
			_tcscat(pMessage, pHandshakeSendInfo->host);

		_tcscat(pMessage, TEXT(HANDSHAKE_FIELD_SEPARATOR));

		if (pHandshakeSendInfo->port != NULL)
			_tcscat(pMessage, pHandshakeSendInfo->port);

		_tcscat(pMessage, TEXT(HANDSHAKE_FIELD_SEPARATOR));

		if (lpszEncKeyName != NULL)
		{
			_tcscat(pMessage, lpszEncKeyName);
		}
	}

	return pMessage;
}

static DWORD WINAPI WiFiBroadcastThreadProc(LPVOID pParameter)
{
	USES_RTN;
	LPHANDSHAKESENDINFO pHandshakeSendInfo = (LPHANDSHAKESENDINFO) pParameter;
	struct sockaddr_in addressRemote;
	ACK_RESULT hrResult = S_OK;
	LPTSTR pMessage = NULL;

	ACKP_PROVIDER hProvider = NULL;
	int index;
	int count;
	ACK_LPKEYINFO* pKeyInfo = NULL;
	TCHAR szEncKeyName[ACK_KEYINFO_MAXNAME] = {0};

	if (pHandshakeSendInfo == NULL)
		END_THIS_THREAD_PLEASE(E_POINTER);

	RTN_IF_FAILED(ACKP_CreateProvider(&hProvider));
	RTN_IF_FAILED(ACKP_GetAllKeys(hProvider,&count,&pKeyInfo));
	for (index = 0; index < count; index++)
		if (pKeyInfo[index]->isEncrypt)
		{
			size_t length = strlen(pKeyInfo[index]->aKeyName);

			MultiByteToWideChar(CP_ACP, 0, pKeyInfo[index]->aKeyName, length, &szEncKeyName[0], length);
			break;
		}
	ACK_free(pKeyInfo);
	pKeyInfo = NULL;
	ACKP_CloseProvider(hProvider);
	hProvider = NULL;

	//A blank port field is our signal that this is a broadcast message.
	pHandshakeSendInfo->port = _T(" ");
	pMessage = AssembleMessage(pHandshakeSendInfo,&szEncKeyName[0]);
	RTN_IF_BADNEW(pMessage);

	//Set up the remote address for the handshake port.
	ZeroMemory(&addressRemote,sizeof(addressRemote));
	addressRemote.sin_family = AF_INET;
	addressRemote.sin_addr.S_un.S_addr = pHandshakeSendInfo->broadcast;
	addressRemote.sin_port = htons(atoi(HANDSHAKE_WIFI_PORT));

	do
	{
		//Broadcast our IP address.
		RTN_IF_SOCKET_ERROR(sendto(theBrodcastSocket,(LPBYTE)pMessage, sizeof(TCHAR) * (_tcslen(pMessage) + 1),
			0,(struct sockaddr*) &(addressRemote),sizeof(addressRemote)));

		//Pause for 15 seconds before re-broadcasting (heartbeats) our IP address.
		switch (WaitForSingleObject(hShutdownBroadcast,15000))
		{
			case WAIT_TIMEOUT:
				hrResult = S_OK;
				break;
			case WAIT_OBJECT_0:
				hrResult = S_FALSE;
				break;
			case WAIT_FAILED:
				hrResult = HRESULT_FROM_WIN32(GetLastError());
				break;
		}
	} while (hrResult == S_OK);

	BEGIN_RTN_CLEAN_UP
		if (pMessage != NULL)
			ACK_free(pMessage);

		if (hBroadcastThread != NULL) {
			CloseHandle(hBroadcastThread);
			hBroadcastThread = NULL;
		}

		broadcastThreadId = 0;

		END_THIS_THREAD_PLEASE(hrResult);
	END_RTN_CLEAN_UP;
}

static DWORD WINAPI WiFiSendThreadProc(LPVOID pParameter)
{
	USES_RTN;
	LPHANDSHAKESENDINFO pHandshakeSendInfo = (LPHANDSHAKESENDINFO) pParameter;
	struct sockaddr_in addressRemote;
	ACK_RESULT hrResult = S_OK;
	LPTSTR pMessage = NULL;
	CHAR remoteBuffer[MAX_PATH + 1] = {0};

	if (pHandshakeSendInfo == NULL)
		END_THIS_THREAD_PLEASE(E_POINTER);

	pMessage = AssembleMessage(pHandshakeSendInfo,NULL);
	RTN_IF_BADNEW(pMessage);

	//Set up the remote address for the handshake port.
	WideCharToMultiByte(CP_ACP, 0, pHandshakeSendInfo->address,
		wcslen(pHandshakeSendInfo->address), remoteBuffer, MAX_PATH, NULL, NULL);

	ZeroMemory(&addressRemote,sizeof(addressRemote));
	addressRemote.sin_family = AF_INET;
	addressRemote.sin_addr.S_un.S_addr = inet_addr(remoteBuffer);
	addressRemote.sin_port = htons(atoi(HANDSHAKE_WIFI_PORT));

	//Send the initiation packet.
	RTN_IF_SOCKET_ERROR(sendto(theBrodcastSocket,(LPBYTE)pMessage, sizeof(TCHAR) * (_tcslen(pMessage) + 1),
		0,(struct sockaddr*) &(addressRemote),sizeof(addressRemote)));

	BEGIN_RTN_CLEAN_UP
		if (pMessage != NULL)
			ACK_free(pMessage);

		if (hSendThread != NULL) {
			CloseHandle(hSendThread);
			hSendThread = NULL;
		}

		sendThreadId = 0;

		END_THIS_THREAD_PLEASE(hrResult);
	END_RTN_CLEAN_UP;
}

static DWORD WINAPI WiFiReadThreadProc(LPVOID pParameter)
{
	USES_RTN;
	LPHANDSHAKEREADINFO pHandshakeReadInfo = (LPHANDSHAKEREADINFO) pParameter;
	ACK_RESULT hrResult = S_OK;

	if (pHandshakeReadInfo == NULL)
		END_THIS_THREAD_PLEASE(E_POINTER);

	do
	{
		int iReceivedBytes;
		fd_set fds;
		int status;
		struct timeval timeout = {0};
		int iAddressLength;
		struct sockaddr_in addressRemote;
		BYTE buffer[512];	//Broadcast messages should not exceed 512 bytes to avoid fragmantation.

		//Wait for messages for upto one second.
		timeout.tv_sec = 1;
		FD_ZERO(&fds);
		FD_SET(theBrodcastSocket,&fds);
		//Check the link to see if there is a packet available for read.
		status = select(1,&fds,NULL,NULL,&timeout);
		RTN_IF_SOCKET_ERROR(status);
		if ((status > 0) && (FD_ISSET(theBrodcastSocket,&fds)))
		{
			iAddressLength = sizeof(addressRemote);
			ZeroMemory(&addressRemote,sizeof(addressRemote));
			addressRemote.sin_family = AF_INET;

			iReceivedBytes = recvfrom(theBrodcastSocket,buffer,sizeof(buffer),0,(struct sockaddr*) &addressRemote,&iAddressLength);
			RTN_IF_SOCKET_ERROR(iReceivedBytes);

			if (pHandshakeReadInfo->proc != NULL)
			{
				LPTSTR protocol = NULL;
				LPTSTR version = NULL;
				LPTSTR host = NULL;
				LPTSTR port = NULL;
				LPTSTR encKeyName = NULL;

				protocol = _tcstok((LPTSTR) buffer,TEXT(HANDSHAKE_FIELD_SEPARATOR));

				if (protocol == NULL)
					continue;

				version = _tcstok(NULL, TEXT(HANDSHAKE_FIELD_SEPARATOR));

				if (version == NULL)
					continue;

				host = _tcstok(NULL, TEXT(HANDSHAKE_FIELD_SEPARATOR));

				if (host == NULL)
					continue;

				port = _tcstok(NULL, TEXT(HANDSHAKE_FIELD_SEPARATOR));

				if (port == NULL)
					continue;

				encKeyName = _tcstok(NULL, TEXT(HANDSHAKE_FIELD_SEPARATOR));

//				if (encKeyName == NULL)
//					continue;

				RTN_IF_FAILED(pHandshakeReadInfo->proc(
					0,	//SMSAT_UNKNOWN
					encKeyName,
					protocol, version, host, port,
					pHandshakeReadInfo->clientData));
			}
		}

		//Check to see if thread should shut down.
		switch (WaitForSingleObject(hShutdownRead,0))
		{
			case WAIT_TIMEOUT:
				hrResult = S_OK;
				break;
			case WAIT_OBJECT_0:
				hrResult = S_FALSE;
				break;
			case WAIT_FAILED:
				hrResult = HRESULT_FROM_WIN32(GetLastError());
				break;
		}
	} while (hrResult == S_OK);

	BEGIN_RTN_CLEAN_UP
		END_THIS_THREAD_PLEASE(hrResult);
	END_RTN_CLEAN_UP;
}
#endif /* FEATURE_WIFI */
