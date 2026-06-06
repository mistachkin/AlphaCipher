/*
 * phone.cpp --
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

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <windows.h>
#include <windowsx.h>

#if 0
#include <commctrl.h>
#endif

#include <aygshell.h>
#include <sms.h>
#include <winsock.h>

#if defined(FEATURE_RAS)
#include <ras.h>
#endif /* FEATURE_RAS */

#include "navajoPort.h"
#include "navajo.h"

#include "phone.h"

#if defined(FEATURE_PHONE_NUMBER)
#include <tapi.h>
#include <tsp.h>

#include "phoneNumber.h"
#endif /* FEATURE_PHONE_NUMBER */

#include "tinCan.h"
#include "handshake.h"

#include "buildnum.h"
#include "resource.h"
#include "transport.h"

#if defined(FEATURE_TOOLHELP)

#include "tlhelp32.h"

#define TMAIL_EXE_NAME                     "tmail.exe"

#ifndef TH32CS_SNAPNOHEAPS
#define TH32CS_SNAPNOHEAPS  0x40000000
#endif

#endif

#if defined(FEATURE_RIL)

#include "ril.h"

#define RADIO_INTERFACE_LIBRARY        "ril.dll"

#define RIL_INITIALIZE_NAME           "RIL_Initialize"
#define RIL_SETAUDIODEVICES_NAME "RIL_SetAudioDevices"
#define RIL_DEINITIALIZE_NAME       "RIL_Deinitialize"

typedef HRESULT (WINAPI *PRIL_Initialize)(
    DWORD dwIndex,
    RILRESULTCALLBACK pfnResult,
    RILNOTIFYCALLBACK pfnNotify,
    DWORD dwNotificationClasses,
    DWORD dwParam,
    HRIL* lphRil
);

typedef HRESULT (WINAPI *PRIL_SetAudioDevices)(
    HRIL hRil,
    const RILAUDIODEVICEINFO* lpAudioDeviceInfo
);

typedef HRESULT (WINAPI *PRIL_Deinitialize)(
    HRIL hRil
);

static HMODULE ghRilModule = NULL;
static PRIL_Initialize gpRilInitialize = NULL;
static PRIL_SetAudioDevices gpRilSetAudioDevices = NULL;
static PRIL_Deinitialize gpRilDeinitialize = NULL;
static HRIL ghRil = NULL;

#endif /* FEATURE_RIL */

#if defined(FEATURE_PHONE_NUMBER)
static TCHAR gPhoneNumber[MAX_PATH + 1] = {0};
#endif /* FEATURE_PHONE_NUMBER */

#if defined(FEATURE_RAS)
static HRASCONN ghRasConn = NULL;
#endif /* FEATURE_RAS */

#if defined(__cplusplus)
extern "C" {
HWND ghWnd = NULL;
};
#endif

static HINSTANCE ghInstance = NULL;
static HWND ghWndChild = NULL;
static LPCTSTR gCellHost = NULL;
static LPCTSTR gCellPort = NULL;
static LPCTSTR gWiFiHost = NULL;
static LPCTSTR gWiFiPort = NULL;
static unsigned long gWiFiBroadcast = NULL;
static ACK_MUTEX gSection = {0};

#if defined(FEATURE_TOOLHELP)
static DWORD FindProcess(
    LPTSTR name
    )
{
    HANDLE hSnapshot = NULL;

    hSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS | TH32CS_SNAPNOHEAPS, 0);

    DBG_LOC(CreateToolhelp32Snapshot);

    if ((hSnapshot != NULL) &&
        (hSnapshot != INVALID_HANDLE_VALUE)) {
        /*
         * NOTE: Process list snapshot has been created.
         */

        PROCESSENTRY32 processEntry = {0};

        processEntry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &processEntry)) {
            DBG_LOC(Process32First);

            do {
                if ((processEntry.szExeFile != NULL) &&
                    (_tcsicmp(processEntry.szExeFile, name) == 0)) {
                    /*
                     * NOTE: We found the process with the matching executable
                     *       name.  Return the corresponding process Id now.
                     */

                    CloseToolhelp32Snapshot(hSnapshot);
                    DBG_LOC(CloseToolhelp32Snapshot);

                    return processEntry.th32ProcessID;
                }
            } while (Process32Next(hSnapshot, &processEntry));
        }
    }

    CloseToolhelp32Snapshot(hSnapshot);
    DBG_LOC(CloseToolhelp32Snapshot);

    return 0;
}

static VOID KillProcess(
    LPTSTR name
    )
{
    DWORD dwProcessId = FindProcess(name);

    if (dwProcessId != 0) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);

        if (hProcess != NULL) {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
    }
}
#endif /* FEATURE_TOOLHELP */

#if defined(FEATURE_RIL)
static HRESULT InitializeRadioInterfaceLibrary(
    VOID
    )
{
    HRESULT hResult = S_OK;

    ghRilModule = LoadLibrary(TEXT(RADIO_INTERFACE_LIBRARY));

    if (ghRilModule == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    DBG_LOC(LoadLibrary);

    gpRilInitialize = (PRIL_Initialize)GetProcAddress(
        ghRilModule, TEXT(RIL_INITIALIZE_NAME));

    if (gpRilInitialize == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    DBG_LOC(GetProcAddress);

    gpRilSetAudioDevices = (PRIL_SetAudioDevices)GetProcAddress(
        ghRilModule, TEXT(RIL_SETAUDIODEVICES_NAME));

    if (gpRilSetAudioDevices == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    DBG_LOC(GetProcAddress);

    gpRilDeinitialize = (PRIL_Deinitialize)GetProcAddress(
        ghRilModule, TEXT(RIL_DEINITIALIZE_NAME));

    if (gpRilDeinitialize == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    DBG_LOC(GetProcAddress);

    hResult = gpRilInitialize(1, DummyRadioLibraryResultCallback, NULL, 0, 0,
        &ghRil);

    DBG_LOC(gpRilInitialize);

    return hResult;
}

static HRESULT UseEarpiece(
    VOID
    )
{
    RILAUDIODEVICEINFO audioDeviceInfo = {0};

    if (gpRilSetAudioDevices == NULL)
        return E_POINTER;

    if (ghRil == NULL)
        return E_HANDLE;

    audioDeviceInfo.cbSize = sizeof(RILAUDIODEVICEINFO);
    audioDeviceInfo.dwParams = RIL_PARAM_ADI_ALL;
    audioDeviceInfo.dwRxDevice = RIL_AUDIO_HANDSET;
    audioDeviceInfo.dwTxDevice = RIL_AUDIO_HANDSET;

    return gpRilSetAudioDevices(ghRil, &audioDeviceInfo);
}

static HRESULT FinalizeRadioInterfaceLibrary(
    VOID
    )
{
    HRESULT hResult = S_OK;

    if ((gpRilDeinitialize != NULL) && (ghRil != NULL)) {
        hResult = gpRilDeinitialize(ghRil);

        if (SUCCEEDED(hResult))
            ghRil = NULL;
        else
            return hResult;
    }

    gpRilDeinitialize = NULL;
    gpRilSetAudioDevices = NULL;
    gpRilInitialize = NULL;

    if (ghRilModule != NULL) {
        if (FreeLibrary(ghRilModule))
            ghRilModule = NULL;
        else
            return HRESULT_FROM_WIN32(GetLastError());
    }

    return hResult;
}

static VOID CALLBACK DummyRadioLibraryResultCallback(
    DWORD dwCode,
    HRESULT hrCmdID,
    const void* lpData,
    DWORD cbData,
    DWORD dwParam
    )
{
    return;
}
#endif /* FEATURE_RIL */

VOID FatalMessage(
    LPCTSTR message
    )
{
    _ftprintf(stdout, message);

    MessageBox(NULL, message, TEXT(CAPTION),
        MB_OK | MB_SETFOREGROUND | MB_TOPMOST);

    return;
}

VOID FatalError(
    LPCTSTR function,
    HRESULT hResult
    )
{
    TCHAR buffer[MAX_PATH + 1];

    _sntprintf(buffer, MAX_PATH,
        TEXT("FATAL: Function %s failed with HRESULT 0x%08lX."),
        function, hResult);

    /*
     * NOTE: _sntprintf does not guarantee NUL termination on truncation,
     *       so force the final slot explicitly.
     */

    buffer[MAX_PATH] = TEXT('\0');

    _ftprintf(stdout, TEXT("%s\n"), buffer);

    MessageBox(NULL, buffer, TEXT(CAPTION),
        MB_OK | MB_SETFOREGROUND | MB_TOPMOST);

    return;
}

static LPTSTR GetSelectedListText(
    HWND hWnd, BOOL* pbWiFi
    )
{
    LRESULT index = 0;
    LRESULT size = 0;
    LRESULT lResult = 0;
    LPTSTR text = NULL;

    if ((hWnd == NULL) || (pbWiFi == NULL))
        return NULL;

    index = SendMessage(hWnd, LB_GETCURSEL, 0, 0);

    if (index == LB_ERR)
        return NULL;

    size = SendMessage(hWnd, LB_GETTEXTLEN, index, 0);

    if (size == LB_ERR)
        return NULL;

    /*
     * NOTE: LB_GETTEXTLEN returns the character count excluding the NUL
     *       terminator that LB_GETTEXT will write.  Allocate one extra
     *       TCHAR so the NUL does not land one past the end.
     */

    text = (LPWSTR)ACK_zalloc(sizeof(TCHAR) * (size + 1));

    if (text == NULL)
        return NULL;

    lResult = SendMessage(hWnd, LB_GETTEXT, index, (LPARAM)text);

    if (lResult == LB_ERR) {
        ACK_free(text);
        return NULL;
    }

    *pbWiFi = SendDlgItemMessage(ghWnd, IDC_CONTACT, LB_GETITEMDATA, index, 0);

    return text;
}

INT csnprintf(
    FILE *pFile,
    LPTSTR buffer,
    SIZE_T count,
    LPCTSTR format,
    ...
    )
{
    INT result = 0;
    va_list argPtr = NULL;

    va_start(argPtr, format);
    result = _vsntprintf(buffer, count, format, argPtr);
    va_end(argPtr);

    if (pFile != NULL)
        _ftprintf(pFile, TEXT("%s"), buffer);

    return result;
}

HRESULT SetStatus(
    HWND hWnd,
    LPCTSTR format,
    ...
    )
{
    HWND hWndStatus = NULL;
    va_list argPtr = NULL;
    TCHAR buffer[MAX_PATH + 1] = {0};

    if (hWnd == NULL)
        return E_HANDLE;

    if (format == NULL)
        return E_POINTER;

    hWndStatus = GetDlgItem(hWnd, IDC_STATUS);

    if (hWndStatus == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    va_start(argPtr, format);
    _vsntprintf(buffer, MAX_PATH, format, argPtr);
    va_end(argPtr);

    /* IGNORED */
    UpdateWindow(hWnd);

    if (SendMessage(hWndStatus, WM_SETTEXT, 0, (LPARAM)buffer))
        return S_OK;
    else
        return E_FAIL;
}

static VOID OnSize(
    HWND hWnd,
    UINT state,
    int cx,
    int cy
    )
{
    if (ghWndChild != NULL)
        MoveWindow(ghWndChild, 0, 0, cx, cy, TRUE);
}

static BOOL OnCreate(
    HWND hWnd,
    LPCREATESTRUCT pCreate
    )
{
    return TRUE;
}

static BOOL OnInitDialog(
    HWND hWnd,
    HWND hwndFocus,
    LPARAM lInitParam
    )
{
#if defined(FEATURE_PHONE_NUMBER)
    TCHAR phoneNumber[MAX_PATH + 1] = {0};
    LPTSTR pPhoneNumber = phoneNumber;
    UINT index = 0;

    for (index = 0; index < min(MAX_PATH, _tcslen(gPhoneNumber)); index++) {
        if (isdigit(gPhoneNumber[index])) {
            *pPhoneNumber++ = gPhoneNumber[index];
        }
    }

    *pPhoneNumber = TEXT('\0');
#endif /* FEATURE_PHONE_NUMBER */

#if defined(FEATURE_PHONE_NUMBER)
    if (_tcscmp(gPhoneNumber, TEXT(PHONE_NUMBER_1)) != 0) {
#endif /* FEATURE_PHONE_NUMBER */
        SendDlgItemMessage(hWnd, IDC_CONTACT, LB_ADDSTRING, 0,
            (LPARAM)CONTACT_ALICE);
#if defined(FEATURE_PHONE_NUMBER)
    }
#endif /* FEATURE_PHONE_NUMBER */

#if defined(FEATURE_PHONE_NUMBER)
    if (_tcscmp(gPhoneNumber, TEXT(PHONE_NUMBER_2)) != 0) {
#endif /* FEATURE_PHONE_NUMBER */
        SendDlgItemMessage(hWnd, IDC_CONTACT, LB_ADDSTRING, 0,
            (LPARAM)CONTACT_BOB);
#if defined(FEATURE_PHONE_NUMBER)
    }
#endif /* FEATURE_PHONE_NUMBER */

    return TRUE;
}

static VOID OnDestroy(
    HWND hWnd
    )
{
    PostQuitMessage(0);
}

static VOID PaintContent(
    HWND hWnd,
    PAINTSTRUCT *pPaint
    )
{
    return;
}

static VOID OnPaint(
    HWND hWnd
    )
{
    PAINTSTRUCT paint = {0};

    BeginPaint(hWnd, &paint);
    PaintContent(hWnd, &paint);
    EndPaint(hWnd, &paint);
}

#if !defined(_WIN32_WCE)
static VOID OnPrintClient(
    HWND hWnd,
    HDC hDC
    )
{
    PAINTSTRUCT paint = {0};

    paint.hdc = hDC;

    GetClientRect(hWnd, &paint.rcPaint);
    PaintContent(hWnd, &paint);
}
#endif

#if 0
static LRESULT CALLBACK WindowProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMsg) {
        HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hWnd, WM_SIZE, OnSize);
        HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hWnd, WM_PAINT, OnPaint);

#if !defined(_WIN32_WCE)
        case WM_PRINTCLIENT: {
            OnPrintClient(hWnd, (HDC)wParam);
            return 0;
        }
#endif
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
#endif

static INT_PTR CALLBACK DialogProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMsg) {
        HANDLE_MSG(hWnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);

        case WM_COMMAND: {
            WORD wNotifyCode = HIWORD(wParam);
            WORD wId = LOWORD(wParam);

            switch (wId) {
                case ID_ABOUT:
                case ID_MAIN_ABOUT: {
                    CHAR buffer[MAX_PATH + 1];
                    WCHAR wBuffer[MAX_PATH + 1];

                    _snprintf(buffer, MAX_PATH,
                        "%s v%s (%s) with %s%c",
                        FILE_DESCRIPTION, FILE_VERSION,
                        FILE_CONFIGURATION, ACK_GetVersion(),
                        /* NUL */ '\0');

                    buffer[MAX_PATH] = '\0';

                    MultiByteToWideChar(CP_ACP, 0, buffer,
                        MAX_PATH, wBuffer, MAX_PATH);

                    MessageBoxW(hWnd, wBuffer, TEXT(CAPTION),
                        MB_OK | MB_SETFOREGROUND | MB_TOPMOST);

                    SetWindowLong(hWnd, DWL_MSGRESULT, TRUE);
                    return TRUE;
                }
                case ID_HOST:
                case ID_MAIN_HOST: {
                    TCHAR buffer[MAX_PATH + 1];

                    _sntprintf(buffer, MAX_PATH,
                        TEXT("%s:%s%c"), gCellHost, gCellPort,
                        /* NUL */ '\0');

                    buffer[MAX_PATH] = TEXT('\0');

                    MessageBox(hWnd, buffer, TEXT(CAPTION),
                        MB_OK | MB_SETFOREGROUND | MB_TOPMOST);

                    SetWindowLong(hWnd, DWL_MSGRESULT, TRUE);
                    return TRUE;
                }
                case ID_EXIT:
                case ID_MAIN_EXIT: {
                    DestroyWindow(hWnd);

                    return TRUE;
                }
                case ID_DIAL:
                case ID_MAIN_DIAL: {
                    HWND hWndContact = NULL;
                    LPTSTR text = NULL;
                    BOOL bWiFi = FALSE;
                    LPTSTR phoneNumber = NULL;
                    CHAR hostBuffer[MAX_PATH + 1] = {0};
                    CHAR portBuffer[MAX_PATH + 1] = {0};
                    HRESULT hResult = S_OK;
                    HANDSHAKESENDINFO handshakeSendInfo = {0};

                    hWndContact = GetDlgItem(hWnd, IDC_CONTACT);

                    if (hWndContact == NULL) {
                        SetStatus(hWnd, TEXT("No contacts listed."));
                        goto dialDone;
                    }

                    text = GetSelectedListText(hWndContact,&bWiFi);

                    if (text == NULL) {
                        SetStatus(hWnd, TEXT("Please select a contact."));
                        goto dialDone;
                    }

                    phoneNumber = _tcstok(text, TEXT(CONTACT_SEPARATOR));

                    if (phoneNumber == NULL) {
                        SetStatus(hWnd, TEXT("Malformed contact entry."));
                        goto dialDone;
                    }

                    phoneNumber = _tcstok(NULL, TEXT(CONTACT_SEPARATOR));

                    if (phoneNumber == NULL) {
                        SetStatus(hWnd, TEXT("Malformed contact entry."));
                        goto dialDone;
                    }

                    phoneNumber++;

                    if (!isdigit(*phoneNumber)) {
                        SetStatus(hWnd, TEXT("Invalid phone number."));
                        goto dialDone;
                    }

                    SetStatus(hWnd, TEXT("Dialing %s%s"),
                        phoneNumber, TEXT("..."));

                    if (bWiFi) {
#if defined(FEATURE_WIFI)
                        handshakeSendInfo.address = phoneNumber;
                        handshakeSendInfo.protocol = TEXT(HANDSHAKE_WIFI_PROTOCOL_NAME);
                        handshakeSendInfo.version = TEXT(HANDSHAKE_PROTOCOL_VERSION);
                        handshakeSendInfo.host = gWiFiHost;
                        handshakeSendInfo.port = gWiFiPort;

                        hResult = StartupWiFiSendThread(&gSection, &handshakeSendInfo);

                        if (FAILED(hResult)) {
                            SetStatus(hWnd, TEXT("Failed to initiate "),
                                phoneNumber, TEXT("."));
                            goto dialDone;
                        }
#endif /* FEATURE_WIFI */
                        WideCharToMultiByte(CP_ACP, 0, gWiFiHost,
                            wcslen(gWiFiHost), hostBuffer, MAX_PATH,
                            NULL, NULL);

                        WideCharToMultiByte(CP_ACP, 0, gWiFiPort,
                            wcslen(gWiFiPort), portBuffer, MAX_PATH,
                            NULL, NULL);
                    } else {
#if defined(FEATURE_SMS)
                        handshakeSendInfo.address = phoneNumber;
                        handshakeSendInfo.protocol = TEXT(HANDSHAKE_SMS_PROTOCOL_NAME);
                        handshakeSendInfo.version = TEXT(HANDSHAKE_PROTOCOL_VERSION);
                        handshakeSendInfo.host = gCellHost;
                        handshakeSendInfo.port = gCellPort;

                        hResult = StartupSmsSendThread(&gSection, &handshakeSendInfo);

                        if (FAILED(hResult)) {
                            SetStatus(hWnd, TEXT("Failed to initiate "),
                                phoneNumber, TEXT("."));
                            goto dialDone;
                        }
#endif /* FEATURE_SMS */

                        WideCharToMultiByte(CP_ACP, 0, gCellHost,
                            wcslen(gCellHost), hostBuffer, MAX_PATH,
                            NULL, NULL);

                        WideCharToMultiByte(CP_ACP, 0, gCellPort,
                            wcslen(gCellPort), portBuffer, MAX_PATH,
                            NULL, NULL);
                    }

                    hResult = DialRemote(hostBuffer, portBuffer);

                    if (FAILED(hResult)) {
                        SetStatus(hWnd, TEXT("Failed to dial "),
                            phoneNumber, TEXT("."));
                        goto dialDone;
                    }

dialDone:
                    if (text != NULL)
                        ACK_free(text);

                    SetWindowLong(hWnd, DWL_MSGRESULT, TRUE);
                    return TRUE;
                }
                case ID_HANGUP:
                case ID_MAIN_HANGUP: {
                    HRESULT hResult = S_OK;

                    SetStatus(hWnd, TEXT("Hanging up..."));

                    hResult = TerminateTransport();

                    if (FAILED(hResult)) {
                        SetStatus(hWnd, TEXT("Failed to hangup, phase 1."));
                        goto hangupDone;
                    }

                    hResult = InitializeTransport();

                    if (FAILED(hResult)) {
                        SetStatus(hWnd, TEXT("Failed to hangup, phase 2."));
                        goto hangupDone;
                    }

                    SetStatus(hWnd, TEXT("Hangup complete."));

hangupDone:
                    SetWindowLong(hWnd, DWL_MSGRESULT, TRUE);
                    return TRUE;
                }
                case ID_CONTACT: {
                    HWND hWndContact = GetDlgItem(hWnd, IDC_CONTACT);

                    if (hWndContact != NULL) {
                        SetFocus(hWndContact);

                        SetWindowLong(hWnd, DWL_MSGRESULT, TRUE);
                        return TRUE;
                    }
                }
            }
        }
    }

    return FALSE;
}

static BOOL InitApp(
    HINSTANCE hInstance
    )
{
#if 0
    WNDCLASS wndClass = {0};

    wndClass.style = 0;
    wndClass.lpfnWndProc = WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = NULL;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = TEXT("Scratch");

    if (!RegisterClass(&wndClass))
        return FALSE;

    InitCommonControls(); /* In case we use a common control */
#endif

    return TRUE;
}

static HRESULT AnswerCellHandshakeProc(
    INT addressType,
    LPCWSTR address,
    LPCWSTR protocol,
    LPCWSTR version,
    LPCWSTR host,
    LPCWSTR port,
    ACK_CLIENTDATA clientData
    )
{
    CHAR localHost[MAX_PATH + 1] = {0};
    CHAR remoteHost[MAX_PATH + 1] = {0};
    CHAR remotePort[MAX_PATH + 1] = {0};

    WideCharToMultiByte(CP_ACP, 0, gCellHost, wcslen(gCellHost), localHost, MAX_PATH,
        NULL, NULL);

    WideCharToMultiByte(CP_ACP, 0, host, wcslen(host), remoteHost, MAX_PATH,
        NULL, NULL);

    WideCharToMultiByte(CP_ACP, 0, port, wcslen(port), remotePort, MAX_PATH,
        NULL, NULL);

    return AnswerRemote(localHost, remoteHost, remotePort);
}

static HRESULT AnswerWifiHandshakeProc
(
    INT addressType,
    LPCWSTR address,
    LPCWSTR protocol,
    LPCWSTR version,
    LPCWSTR host,
    LPCWSTR port,
    ACK_CLIENTDATA clientData
    )
{
    CHAR localHost[MAX_PATH + 1] = {0};
    CHAR remoteHost[MAX_PATH + 1] = {0};
    CHAR remotePort[MAX_PATH + 1] = {0};

    if (_tcscmp(port,_T(" ")) == 0) {
        //Encountered a broadcast message.
        if (_tcscmp(gWiFiHost,host) != 0) {
            //Encountered a broadcast message from a peer; add the peer to the list box.
            TCHAR szBuffer[MAX_PATH] = {0};
            int index;

            /*
             * NOTE: 'address' and 'host' may flow from network-controlled
             *       input on the WiFi handshake.  Use _sntprintf with an
             *       explicit MAX_PATH bound so an oversized peer name
             *       cannot overflow the stack buffer.
             */

            _sntprintf(&szBuffer[0], MAX_PATH - 1,
                _T("%s ") _T(CONTACT_SEPARATOR) _T(" %s"), address, host);
            szBuffer[MAX_PATH - 1] = _T('\0');

            index = SendDlgItemMessage(ghWnd, IDC_CONTACT, LB_FINDSTRINGEXACT, -1, (LPARAM) szBuffer);
            if (index == LB_ERR)
                index = SendDlgItemMessage(ghWnd, IDC_CONTACT, LB_ADDSTRING, 0, (LPARAM) szBuffer);
            SendDlgItemMessage(ghWnd, IDC_CONTACT, LB_SETITEMDATA, index, (LPARAM) GetTickCount());
        } else {
            //Encountered a broadcast message from ourselves;  This gives us the
            //opportunity to remove stale WiFi entries from the list box.
            //A stale entry is one that has not recieved a heartbeat (there should
            //have been four) in 60 seconds.
            int count = SendDlgItemMessage(ghWnd, IDC_CONTACT, LB_GETCOUNT, 0, 0);
            int index = 0;
            while (index < count) {
                DWORD ticks = SendDlgItemMessage(ghWnd, IDC_CONTACT, LB_GETITEMDATA, index, 0);
                if ((ticks != 0) && ((GetTickCount() - ticks) > 60000)) {
                    SendDlgItemMessage(ghWnd, IDC_CONTACT, LB_DELETESTRING, index, 0);
                    count--;
                } else {
                    index++;
                }
            }

        }
        return S_FALSE;
    } else {
        WideCharToMultiByte(CP_ACP, 0, gWiFiHost, wcslen(gWiFiHost), localHost, MAX_PATH,
            NULL, NULL);

        WideCharToMultiByte(CP_ACP, 0, host, wcslen(host), remoteHost, MAX_PATH,
            NULL, NULL);

        WideCharToMultiByte(CP_ACP, 0, port, wcslen(port), remotePort, MAX_PATH,
            NULL, NULL);

        return AnswerRemote(localHost, remoteHost, remotePort);
    }
}

static HRESULT DefaultHandshakeProc(
    INT addressType,
    LPCWSTR address,
    LPCWSTR protocol,
    LPCWSTR version,
    LPCWSTR host,
    LPCWSTR port,
    ACK_CLIENTDATA clientData
    )
{
    return S_OK;
}

INT WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    INT nShowCmd
    )
{
    HANDLE hMutex = NULL;
    DWORD dwExitCode = EXITCODE_SUCCESS;
    HRESULT hResult = S_OK;
    BOOL comInitialized = FALSE;
    BOOL winSockInitialized = FALSE;
    WSADATA wsaData = {0};
    MSG msg = {0};
    HACCEL hAccelerators = NULL;
    HANDSHAKESENDINFO handshakeSendInfo = {0};
    HANDSHAKEREADINFO handshakeReadInfo = {0};

    hMutex = CreateMutex(NULL, TRUE, TEXT(MUTEX_NAME));

    if ((hMutex == NULL) || (GetLastError() == ERROR_ALREADY_EXISTS)) {
        HWND hWnd = FindWindow(NULL, TEXT(PRODUCT_NAME));

        if (hWnd != NULL) {
            hWnd = (HWND)((ULONG_PTR)hWnd | 0x1);
            SetForegroundWindow(hWnd);

            goto done;
        } else {
            MAIN_FATAL_MESSAGE("Only one instance supported.", EXITCODE_FAILURE);
        }
    }

    InitializeCriticalSection(&gSection);
    DBG_LOC(InitializeCriticalSection);

    if (!InitApp(hInstance))
        MAIN_FATAL_EXIT(InitApp, E_FAIL);

    DBG_LOC(InitApp);

    comInitialized = SUCCEEDED(hResult = CoInitializeEx(
        NULL, COINIT_MULTITHREADED));

    if (comInitialized) {
        DBG_LOC(CoInitializeEx);

        dwExitCode = WSAStartup(MAKEWORD(1, 0), &wsaData);

        if (dwExitCode == 0)
            dwExitCode = EXITCODE_SUCCESS;
        else
            MAIN_FATAL_EXIT(WSAStartup, HRESULT_FROM_WIN32(dwExitCode));

        DBG_LOC(WSAStartup);

        winSockInitialized = TRUE;

        hResult = ACK_Initialize();

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(ACK_Initialize, hResult);

        DBG_LOC(ACK_Initialize);

#if defined(FEATURE_PHONE_NUMBER)
        hResult = SHGetPhoneNumber(gPhoneNumber, MAX_PATH, 1);

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(SHGetPhoneNumber, hResult);

        DBG_LOC(SHGetPhoneNumber);
#endif /* FEATURE_PHONE_NUMBER */

#if defined(FEATURE_RIL)
        hResult = InitializeRadioInterfaceLibrary();

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(InitializeRadioInterfaceLibrary, hResult);

        DBG_LOC(InitializeRadioInterfaceLibrary);

        hResult = UseEarpiece();

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(UseEarpiece, hResult);

        DBG_LOC(UseEarpiece);
#endif /* FEATURE_RIL */

#if defined(FEATURE_RAS)
        hResult = AcquireOrUpdateIpAddress(TEXT(RAS_ENTRY_NAME), &ghRasConn);

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(AcquireOrUpdateIpAddress, hResult);

        DBG_LOC(AcquireOrUpdateIpAddress);
#endif /* FEATURE_RAS */

#if defined(FEATURE_SMS)
        hResult = QueryLocalIpAddressAndPort(TRUE, &gCellHost, &gCellPort, NULL);

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(QueryLocalIpAddressAndPort, hResult);

        DBG_LOC(QueryLocalIpAddressAndPort);
#endif /* FEATURE_SMS */

#if defined(FEATURE_WIFI)
        hResult = QueryLocalIpAddressAndPort(FALSE, &gWiFiHost, &gWiFiPort, &gWiFiBroadcast);

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(QueryLocalIpAddressAndPort, hResult);

        DBG_LOC(QueryLocalIpAddressAndPort);
#endif /* FEATURE_WIFI */

        hResult = InitializeTransport();

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(InitializeTransport, hResult);

        DBG_LOC(InitializeTransport);

        hResult = InitializeWaveModule(TransmitBuffer);

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(InitializeWaveModule, hResult);

        DBG_LOC(InitializeWaveModule);

#if defined(FEATURE_TOOLHELP)
        KillProcess(TEXT(TMAIL_EXE_NAME));

        DBG_LOC(KillProcess);
#endif /* FEATURE_TOOLHELP */

#if defined(FEATURE_SMS)
        handshakeReadInfo.proc = AnswerCellHandshakeProc;

        hResult = StartupSmsReadThread(&gSection, &handshakeReadInfo);

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(StartupSmsReadThread, hResult);

        DBG_LOC(StartupSmsReadThread);
#endif /* FEATURE_SMS */

#if defined(FEATURE_WIFI)
        handshakeSendInfo.protocol = TEXT(HANDSHAKE_WIFI_PROTOCOL_NAME);
        handshakeSendInfo.version = TEXT(HANDSHAKE_PROTOCOL_VERSION);
        handshakeSendInfo.host = gWiFiHost;
        handshakeSendInfo.port = gWiFiPort;
        handshakeSendInfo.broadcast = gWiFiBroadcast;

        hResult = StartupWiFiBroadcastThread(&gSection, &handshakeSendInfo);

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(StartupWiFiBroadcastThread, hResult);

        DBG_LOC(StartupWiFiBroadcastThread);

        handshakeReadInfo.proc = AnswerWifiHandshakeProc;

        hResult = StartupWiFiReadThread(&gSection, &handshakeReadInfo);

        if (FAILED(hResult))
            MAIN_FATAL_EXIT(StartupWiFiReadThread, hResult);

        DBG_LOC(StartupWiFiReadThread);
#endif /* FEATURE_WIFI */

#if 1
        ghWnd = CreateDialog(
            hInstance,
            MAKEINTRESOURCE(IDD_PHONE),
            NULL,
            DialogProc);
#else
        ghWnd = CreateWindowEx(
            0,                              /* Extended Style */
            TEXT("Scratch"),                /* Class Name */
            TEXT("Scratch"),                /* Title */
#if !defined(_WIN32_WCE)
            WS_OVERLAPPEDWINDOW,            /* Style */
#else
            WS_POPUP,                       /* Style */
#endif
            CW_USEDEFAULT, CW_USEDEFAULT,   /* Position */
            CW_USEDEFAULT, CW_USEDEFAULT,   /* Size */
            NULL,                           /* Parent */
            NULL,                           /* No menu */
            hInstance,                      /* Instance */
            0);                             /* No special parameters */
#endif

        if (ghWnd != NULL) {
#if 1
            DBG_LOC(CreateDialog);
#else
            DBG_LOC(CreateWindowEx);
#endif

            ghInstance = hInstance;

            ShowWindow(ghWnd, nShowCmd);

            hAccelerators = LoadAccelerators(hInstance,
                MAKEINTRESOURCE(IDR_PHONE));

            SHMENUBARINFO menuBarInfo = {0};

            menuBarInfo.cbSize = sizeof(SHMENUBARINFO);
            menuBarInfo.hwndParent = ghWnd;
            menuBarInfo.dwFlags = SHCMBF_HMENU;
            menuBarInfo.nToolBarId = IDR_MAIN_MENU;
            menuBarInfo.hInstRes = hInstance;

            SHCreateMenuBar(&menuBarInfo);

            SetStatus(ghWnd, TEXT("Ready."));

            while (GetMessage(&msg, NULL, 0, 0)) {
                if (!IsDialogMessage(msg.hwnd, &msg)) {
                    if (!TranslateAccelerator(msg.hwnd, hAccelerators, &msg)) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
        } else {
            /*
             * NOTE: We failed to create the main window?
             */

#if 1
            MAIN_FATAL_EXIT(CreateDialog, HRESULT_FROM_WIN32(GetLastError()));
#else
            MAIN_FATAL_EXIT(CreateWindowEx, HRESULT_FROM_WIN32(GetLastError()));
#endif
        }
    } else {
        /*
         * NOTE: We failed to initialize COM.
         */

        MAIN_FATAL_EXIT(CoInitializeEx, hResult);
    }

done:
#if defined(FEATURE_WIFI)
    ShutdownWiFiThreads(&gSection);
    DBG_LOC(ShutdownWiFiThreads);
#endif /* FEATURE_SMS */

#if defined(FEATURE_SMS)
    ShutdownSmsThreads(&gSection);
    DBG_LOC(ShutdownSmsThreads);
#endif /* FEATURE_SMS */

    TerminateTransport();
    DBG_LOC(TerminateTransport);

    FinalizeWaveModule();
    DBG_LOC(FinalizeWaveModule);

#if defined(FEATURE_RIL)
    FinalizeRadioInterfaceLibrary();
    DBG_LOC(FinalizeRadioInterfaceLibrary);
#endif /* FEATURE_RIL */

    ACK_Finalize();
    DBG_LOC(ACK_Finalize);

    if (winSockInitialized) {
        WSACleanup();
        winSockInitialized = FALSE;

        DBG_LOC(WSACleanup);
    }

    if (comInitialized) {
        CoUninitialize();
        comInitialized = FALSE;

        DBG_LOC(CoUninitialize);
    }

    DeleteCriticalSection(&gSection);
    DBG_LOC(DeleteCriticalSection);

    CloseHandle(hMutex); hMutex = NULL;
    DBG_LOC(CloseHandle);

    DBG_LOC(END_THIS_PROCESS_PLEASE);
    END_THIS_PROCESS_PLEASE(dwExitCode);
}

/* end of file*/
