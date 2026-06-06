/*
 * sms.cpp --
 *
 * Copyright (c) 2009-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * written by: Joe Mistachkin
 *
 * RCS: @(#) $Id: $
 */

#if !defined(_WIN32_WCE)
#error "This file can only be compiled on Windows CE."
#endif

#if !defined(STRICT)
#define STRICT
#endif

#include <windows.h>
#include <winsock.h>
#include <iphlpapi.h>

#if defined(FEATURE_RAS)
#include <ras.h>
#include <raserror.h>
#endif /* FEATURE_RAS */

#if defined(FEATURE_SMS)
#include <sms.h>
#endif /* FEATURE_SMS */

#include "hresult.h"

#include "navajoPort.h"
#include "navajo.h"

#include "phone.h"
#include "tinCan.h"
#include "handshake.h"

#if defined(FEATURE_SMS)
static DWORD WINAPI SmsSendThreadProc(LPVOID pParameter);
static DWORD WINAPI SmsReadThreadProc(LPVOID pParameter);

static HANDLE hSendThread = NULL;
static HANDLE hReadThread = NULL;

static DWORD sendThreadId = 0;
static DWORD readThreadId = 0;

static HANDLE hShutdownRead = NULL;
#endif /* FEATURE_SMS */

#if defined(FEATURE_RAS)
HRESULT AcquireOrUpdateIpAddress(
    LPCTSTR entryName,
    LPHRASCONN phRasConn
    )
{
    DWORD dwResult = 0;
    RASDIALPARAMS rasDialParams = {0};
    BOOL bPassword = FALSE;

    if ((entryName == NULL) || (phRasConn == NULL))
        return E_POINTER;

    rasDialParams.dwSize = sizeof(RASDIALPARAMS);
    _tcsnccpy(rasDialParams.szEntryName, entryName, RAS_MaxEntryName);

    dwResult = RasGetEntryDialParams(NULL, &rasDialParams, &bPassword);

    if (dwResult != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(dwResult);

    dwResult = RasDial(NULL, NULL, &rasDialParams, 0, NULL, phRasConn);

    /*
     * HACK: Assume that ERROR_UNKNOWN means "you are already connected".
     */

    if ((dwResult != ERROR_SUCCESS) && (dwResult != ERROR_UNKNOWN))
        return HRESULT_FROM_WIN32(dwResult);

    return S_OK;
}
#endif /* FEATURE_RAS */

HRESULT QueryLocalIpAddressAndPort(
    BOOL bCellularLine,
    LPCWSTR *ppIpAddress,
    LPCWSTR *ppPort,
    unsigned long *ppIpBrodcast
    )
{
    HRESULT hResult = E_NOTIMPL;
    DWORD dwResult = 0;
    PIP_INTERFACE_INFO infoInterface;
    ULONG infoSize = 0;
    ULONG indexCellularLine = 0;
    PMIB_IPADDRTABLE pOldIpAddrTable = NULL;
    PMIB_IPADDRTABLE pIpAddrTable = NULL;
    ULONG size = 0;
    LPTSTR pIpAddress = NULL;

    if ((ppIpAddress == NULL) || (ppPort == NULL))
        return E_POINTER;

    if ((*ppIpAddress != NULL) || (*ppPort != NULL))
        return E_MUSTBENULL;

    //Determine the interface index of "Cellular Line".
    GetInterfaceInfo(NULL,&infoSize);
    infoInterface = (PIP_INTERFACE_INFO) ACK_zalloc(infoSize);
    GetInterfaceInfo(infoInterface,&infoSize);
    for (LONG index = 0;index < infoInterface->NumAdapters;index++) {
        //The name is in the form of "Cellular Line*".
        if (wcsncmp(infoInterface->Adapter[index].Name,L"Cellular Line",13) == 0)
            indexCellularLine = infoInterface->Adapter[index].Index;
    }

    dwResult = GetIpAddrTable(pIpAddrTable, &size, TRUE);

    if (dwResult != ERROR_INSUFFICIENT_BUFFER) {
        hResult = HRESULT_FROM_WIN32(dwResult);
        goto done;
    }

    pOldIpAddrTable = pIpAddrTable;
    pIpAddrTable = (PMIB_IPADDRTABLE)ACK_realloc(pOldIpAddrTable, size, TRUE);

    if (pIpAddrTable != NULL) {
        pOldIpAddrTable = NULL;
    } else {
        hResult = E_OUTOFMEMORY;
        goto done;
    }

    dwResult = GetIpAddrTable(pIpAddrTable, &size, TRUE);

    if (dwResult != NO_ERROR) {
        hResult = HRESULT_FROM_WIN32(dwResult);
        goto done;
    }

    for (DWORD dwIndex = 0; dwIndex < pIpAddrTable->dwNumEntries; dwIndex++) {
        PMIB_IPADDRROW pIpAddrRow = &pIpAddrTable->table[dwIndex];
        struct in_addr inetAddr = {0};
        char *addr = NULL;
        size_t length = 0;

        if (!(pIpAddrRow->wType & MIB_IPADDR_PRIMARY))
            continue;

        if (pIpAddrRow->dwAddr == 0)
            continue;

        if (bCellularLine) {
            //We are looking for the cellular line.
            if (pIpAddrRow->dwIndex != indexCellularLine)
                continue;
        } else {
            //We are looking for the first non-cellular line.
            if (pIpAddrRow->dwIndex == indexCellularLine)
                continue;
        }

        if (pIpAddrRow->dwAddr == LOOPBACK_IP_ADDR)
            continue;

        inetAddr.S_un.S_addr = pIpAddrRow->dwAddr;
        addr = inet_ntoa(inetAddr);

        if (addr == NULL)
            continue;

        length = strlen(addr);

        pIpAddress = (LPTSTR)ACK_zalloc(sizeof(TCHAR) * (length + 1));

        if (pIpAddress == NULL) {
            hResult = E_OUTOFMEMORY;
            break;
        }

        MultiByteToWideChar(CP_ACP, 0, addr, length, pIpAddress, length);

        *ppIpAddress = pIpAddress;
        pIpAddress = NULL;
        *ppPort = TEXT(HANDSHAKE_TEST_PORT);
        if (ppIpBrodcast)
            *ppIpBrodcast = pIpAddrRow->dwAddr | (~pIpAddrRow->dwMask);

        hResult = S_OK;
        break;
    }
	hResult = S_FALSE;

done:
    if (pIpAddress != NULL)
        ACK_free(pIpAddress);

    if (pIpAddrTable != NULL)
        ACK_free(pIpAddrTable);

    if (pOldIpAddrTable != NULL)
        ACK_free(pOldIpAddrTable);

    return hResult;
}

#if defined(FEATURE_SMS)
static HANDSHAKESENDINFO gHandshakeSendInfo = {0};
HRESULT StartupSmsSendThread(
    ACK_LPMUTEX pMutex,
    LPHANDSHAKESENDINFO pHandshakeSendInfo
    )
{
    HRESULT hResult = S_OK;

    if ((pMutex == NULL) || (pHandshakeSendInfo == NULL))
        return E_POINTER;

    gHandshakeSendInfo = *pHandshakeSendInfo;

    EnterCriticalSection(pMutex);

    if (hSendThread != NULL) {
        hResult = E_STARTED;
        goto done;
    }

    hSendThread = CreateThread(NULL, 0, SmsSendThreadProc, &gHandshakeSendInfo,
        0, &sendThreadId);

    if (hSendThread == NULL) {
        hResult = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

done:
    LeaveCriticalSection(pMutex);

    return hResult;
}

static HANDSHAKEREADINFO gHandshakeReadInfo = {0};
HRESULT StartupSmsReadThread(
    ACK_LPMUTEX pMutex,
    LPHANDSHAKEREADINFO pHandshakeReadInfo
    )
{
    HRESULT hResult = S_OK;

    if ((pMutex == NULL) || (pHandshakeReadInfo == NULL))
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

    hReadThread = CreateThread(NULL, 0, SmsReadThreadProc, &gHandshakeReadInfo,
        0, &readThreadId);

    if (hReadThread == NULL) {
        hResult = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

done:
    LeaveCriticalSection(pMutex);

    return hResult;
}

HRESULT ShutdownSmsThreads(
    ACK_LPMUTEX pMutex
    )
{
    HRESULT hResult = S_OK;

    if (pMutex == NULL)
        return E_POINTER;

    EnterCriticalSection(pMutex);

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

    if (hSendThread != NULL) {
        if (WaitForSingleObject(hSendThread, THREAD_TIMEOUT) != WAIT_OBJECT_0)
            TerminateThread(hSendThread, EXITCODE_TERMINATE);

        CloseHandle(hSendThread);
        hSendThread = NULL;
    }

    LeaveCriticalSection(pMutex);

    return hResult;
}

static DWORD WINAPI SmsSendThreadProc(
    LPVOID pParameter
    )
{
    HRESULT hResult = S_OK;
    LPHANDSHAKESENDINFO pHandshakeSendInfo = (LPHANDSHAKESENDINFO)pParameter;
    DWORD dwResult = 0;
    SIZE_T messageSize = 0;
    LPTSTR pMessage = NULL;
    SMS_HANDLE hSms = NULL;
    SMS_ADDRESS destinationAddress;
    TEXT_PROVIDER_SPECIFIC_DATA textProviderData = {0};

    if (pHandshakeSendInfo == NULL)
        END_THIS_THREAD_PLEASE(E_POINTER);

    hResult = SmsOpen(SMS_MSGTYPE_TEXT, SMS_MODE_SEND, &hSms, NULL);

    if (FAILED(hResult)) {
        dwResult = hResult;
        goto done;
    }

    if (pHandshakeSendInfo->protocol != NULL)
        messageSize += _tcslen(pHandshakeSendInfo->protocol);

    if (pHandshakeSendInfo->version != NULL)
        messageSize += _tcslen(pHandshakeSendInfo->version);

    if (pHandshakeSendInfo->host != NULL)
        messageSize += _tcslen(pHandshakeSendInfo->host);

    if (pHandshakeSendInfo->port != NULL)
        messageSize += _tcslen(pHandshakeSendInfo->port);

    messageSize += 3 * _tcslen(TEXT(HANDSHAKE_FIELD_SEPARATOR));
    messageSize++; /* NOTE: Null terminated. */

    pMessage = (LPTSTR)ACK_zalloc(sizeof(TCHAR) * messageSize);

    if (pMessage == NULL) {
        dwResult = E_OUTOFMEMORY;
        goto done;
    }

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

    destinationAddress.smsatAddressType = SMSAT_UNKNOWN;

    _tcsncpy(destinationAddress.ptsAddress, pHandshakeSendInfo->address,
        SMS_MAX_ADDRESS_LENGTH);

    destinationAddress.ptsAddress[SMS_MAX_ADDRESS_LENGTH - 1] = TEXT('\0');

    hResult = SmsSendMessage(hSms, NULL, &destinationAddress, NULL,
        (LPBYTE)pMessage, sizeof(TCHAR) * _tcslen(pMessage),
        (LPBYTE)&textProviderData, sizeof(TEXT_PROVIDER_SPECIFIC_DATA),
        SMSDE_OPTIMAL, SMS_OPTION_DELIVERY_NONE, NULL);

    if (pMessage == NULL) {
        dwResult = hResult;
        goto done;
    }

    dwResult = EXITCODE_SUCCESS;

done:
    if (pMessage != NULL)
        ACK_free(pMessage);

    if (hSms != NULL)
        SmsClose(hSms);

    if (hSendThread != NULL) {
        CloseHandle(hSendThread);
        hSendThread = NULL;
    }

    sendThreadId = 0;

    END_THIS_THREAD_PLEASE(dwResult);
}

static DWORD WINAPI SmsReadThreadProc(
    LPVOID pParameter
    )
{
    HRESULT hResult = S_OK;
    LPHANDSHAKEREADINFO pHandshakeReadInfo = (LPHANDSHAKEREADINFO)pParameter;
    DWORD dwResult = 0;
    HANDLE hMessageAvailable = NULL;
    HANDLE waitHandles[2] = {NULL, NULL};
    LPBYTE pOldMessage = NULL;
    LPBYTE pMessage = NULL;
    SMS_HANDLE hSms = NULL;
    SMS_ADDRESS sourceAddress;
    TEXT_PROVIDER_SPECIFIC_DATA textProviderData = {0};
    LPTSTR address = NULL;
    LPTSTR protocol = NULL;
    LPTSTR version = NULL;
    LPTSTR host = NULL;
    LPTSTR port = NULL;

    if (pHandshakeReadInfo == NULL)
        END_THIS_THREAD_PLEASE(E_POINTER);

    hResult = SmsOpen(SMS_MSGTYPE_TEXT, SMS_MODE_RECEIVE, &hSms,
        &hMessageAvailable);

    if (FAILED(hResult)) {
        FATAL_EXIT(SmsOpen, hResult);

        /*
        dwResult = hResult;
        goto done;
        */
    }

    waitHandles[0] = hShutdownRead;
    waitHandles[1] = hMessageAvailable;

    do {
        dwResult = WaitForMultipleObjects(
            sizeof(waitHandles) / sizeof(HANDLE),
            waitHandles, FALSE, INFINITE);

        if (dwResult == WAIT_FAILED) {
            dwResult = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }

        if (dwResult == WAIT_OBJECT_1) {
            /*
             * NOTE: We are being notified of an incoming SMS message.
             */

            #if 1
            SetStatus(ghWnd, TEXT("SMS received."));
            #endif

            hResult = SmsGetMessageSize(hSms, &dwResult);

            if (FAILED(hResult)) {
                dwResult = hResult;
                goto done;
            }

            pOldMessage = pMessage;
            pMessage = (LPBYTE)ACK_realloc(pOldMessage, dwResult, TRUE);

            if (pMessage != NULL) {
                pOldMessage = NULL;
            } else {
                dwResult = E_OUTOFMEMORY;
                goto done;
            }

            hResult = SmsReadMessage(hSms, NULL, &sourceAddress, NULL,
                pMessage, dwResult, (LPBYTE)&textProviderData,
                sizeof(TEXT_PROVIDER_SPECIFIC_DATA), &dwResult);

            if (FAILED(hResult)) {
                dwResult = hResult;
                goto done;
            }

            if (pHandshakeReadInfo->proc != NULL) {
                protocol = _tcstok((LPTSTR)pMessage,
                    TEXT(HANDSHAKE_FIELD_SEPARATOR));

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

                hResult = pHandshakeReadInfo->proc(
                    sourceAddress.smsatAddressType,
                    sourceAddress.ptsAddress,
                    protocol, version, host, port,
                    pHandshakeReadInfo->clientData);

                if (FAILED(hResult)) {
                    dwResult = hResult;
                    goto done;
                }
            }
        }
    } while (dwResult != WAIT_OBJECT_0);

    dwResult = EXITCODE_SUCCESS;

done:
    if (pMessage != NULL)
        ACK_free(pMessage);

    if (pOldMessage != NULL)
        ACK_free(pOldMessage);

    if (hSms != NULL)
        SmsClose(hSms);

    readThreadId = 0;

    END_THIS_THREAD_PLEASE(dwResult);
}
#endif /* FEATURE_SMS */

/* end of file*/
