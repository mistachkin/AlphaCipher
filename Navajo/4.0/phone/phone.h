/*
 * phone.h --
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

#if !defined(_WINDOWS_) && !defined(__WINDOWS__)
#error "The header file <windows.h> must be included prior to this file."
#endif

#if !defined(_NAVAJO_PORT_H_)
#error "The header file \"navajoPort.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_H_)
#error "The header file \"navajo.h\" must be included prior to this file."
#endif

#ifndef _PHONE_H_
#define _PHONE_H_

/*****************************************************************************/

#ifndef DBG_LOC
    #define DBG_LOC(a)  {                                               \
        _ftprintf(stdout, TEXT("%s --> %s, (file \"%s\", line #%d)\n"), \
            TEXT(__FUNCTION__), TEXT(#a), TEXT(__FILE__), __LINE__);    \
    }
#endif

/*****************************************************************************/

/*
 * NOTE: Certain fatal events have been known to occur.
 */

#ifndef FATAL_EXIT
    #define FATAL_EXIT(a,b) {      \
        FatalError(TEXT(#a), (b)); \
        ExitProcess((b));          \
    }
#endif

#ifndef FATAL_MESSAGE
    #define FATAL_MESSAGE(a,b) { \
        FatalMessage(TEXT(a));   \
        ExitProcess((b));        \
    }
#endif

#ifndef MAIN_FATAL_EXIT
    #define MAIN_FATAL_EXIT(a,b) { \
        FatalError(TEXT(#a), (b)); \
        dwExitCode = (b);          \
        goto done;                 \
    }
#endif

#ifndef MAIN_FATAL_MESSAGE
    #define MAIN_FATAL_MESSAGE(a,b) { \
        FatalMessage(TEXT(a));        \
        dwExitCode = (b);             \
        goto done;                    \
    }
#endif

/*****************************************************************************/

#define END_THIS_PROCESS_PLEASE(exitCode) { \
    ExitProcess(exitCode); return exitCode; \
}

/*****************************************************************************/

#ifndef TEXTJOIN2
#define TEXTJOIN2(a,b)  TEXT(a) TEXT(b)
#endif

#ifndef TEXTJOIN3
#define TEXTJOIN3(a,b,c) TEXT(a) TEXT(b) TEXT(c)
#endif

#ifndef TEXTJOIN4
#define TEXTJOIN4(a,b,c,d) TEXT(a) TEXT(b) TEXT(c) TEXT(d)
#endif

/*****************************************************************************/

#define CAPTION                    "AlphaCipher"
#define DATABASE    "\\Storage Card\\layout.pef"
#define CONTACT_SEPARATOR                    "-"

#define PHONE_NUMBER_1              "2533301614"
#define PHONE_NUMBER_2              "2533301081"

#define CONTACT_ALICE \
    TEXTJOIN4("Alice ", CONTACT_SEPARATOR, " ", PHONE_NUMBER_1)

#define CONTACT_BOB   \
    TEXTJOIN4("Bob ", CONTACT_SEPARATOR, " ", PHONE_NUMBER_2)

/*****************************************************************************/

#define MUTEX_NAME       "There_Can_Be_Only_One"

/*****************************************************************************/

#define RAS_ENTRY_NAME                  "Sprint"

/*****************************************************************************/

static LPTSTR GetSelectedListText(
    HWND hWnd /* in */
);

/*****************************************************************************/

static BOOL OnCreate(
    HWND hWnd,
    LPCREATESTRUCT pCreate /* in */
);

static BOOL OnInitDialog(
    HWND hWnd,        /* in */
    HWND hwndFocus,   /* in */
    LPARAM lInitParam /* in */
);

static VOID OnSize(
    HWND hWnd,  /* in */
    UINT state, /* in */
    int cx,     /* in */
    int cy      /* in */
);

static VOID OnPaint(
    HWND hWnd /* in */
);

static VOID OnDestroy(
    HWND hWnd /* in */
);

#if !defined(_WIN32_WCE)
static VOID OnPrintClient(
    HWND hWnd,  /* in */
    HDC hDC     /* in */
);
#endif

static VOID PaintContent(
    HWND hWnd,          /* in */
    PAINTSTRUCT *pPaint /* in */
);

static BOOL InitApp(
    HINSTANCE hInstance /* in */
);

static LRESULT CALLBACK WindowProc(
    HWND hWnd,     /* in */
    UINT uMsg,     /* in */
    WPARAM wParam, /* in */
    LPARAM lParam  /* in */
);

static INT_PTR CALLBACK DialogProc(
    HWND hWnd,     /* in */
    UINT uMsg,     /* in */
    WPARAM wParam, /* in */
    LPARAM lParam  /* in */
);

/*****************************************************************************/

#if defined(FEATURE_TOOLHELP)
static DWORD FindProcess(
    LPTSTR name
);

static VOID KillProcess(
    LPTSTR name
);
#endif /* FEATURE_TOOLHELP */

/*****************************************************************************/

#if defined(FEATURE_RIL)
static HRESULT InitializeRadioInterfaceLibrary(
    VOID
);

static HRESULT UseEarpiece(
    VOID
);

static HRESULT FinalizeRadioInterfaceLibrary(
    VOID
);

static VOID CALLBACK DummyRadioLibraryResultCallback(
    DWORD dwCode,
    HRESULT hrCmdID,
    const void* lpData,
    DWORD cbData,
    DWORD dwParam
);
#endif /* FEATURE_RIL */

/*****************************************************************************/

static HRESULT DefaultHandshakeProc(
    INT addressType,          /* in */
    LPCWSTR address,          /* in */
    LPCWSTR protocol,         /* in */
    LPCWSTR version,          /* in */
    LPCWSTR host,             /* in */
    LPCWSTR port,             /* in */
    ACK_CLIENTDATA clientData /* in */
);

/*****************************************************************************/

static HRESULT AnswerHandshakeProc(
    INT addressType,          /* in */
    LPCWSTR address,          /* in */
    LPCWSTR protocol,         /* in */
    LPCWSTR version,          /* in */
    LPCWSTR host,             /* in */
    LPCWSTR port,             /* in */
    ACK_CLIENTDATA clientData /* in */
);

/*****************************************************************************/

#if defined(__cplusplus)
    extern "C" {
#endif

extern HWND ghWnd;

INT csnprintf(
    FILE *pFile,
    LPTSTR buffer,
    SIZE_T count,
    LPCTSTR format,
    ...
);

HRESULT SetStatus(
    HWND hWnd,
    LPCTSTR format,
    ...
);

VOID FatalMessage(
    LPCTSTR message
);

VOID FatalError(
    LPCTSTR function,
    HRESULT hResult
);

#if defined(__cplusplus)
    }
#endif

/*****************************************************************************/

INT WINAPI WinMain(
    HINSTANCE hInstance,     /* in */
    HINSTANCE hPrevInstance, /* in */
    LPWSTR lpCmdLine,        /* in */
    INT nShowCmd             /* in */
);

/*****************************************************************************/

#endif /* _PHONE_H_ */

/* end of file*/
