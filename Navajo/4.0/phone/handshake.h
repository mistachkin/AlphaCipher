/*
 * handshake.h --
 *
 * Copyright (c) 2008-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * written by: Joe Mistachkin
 *
 * RCS: @(#) $Id: $
 */

#if defined(FEATURE_RAS) && !defined(_RAS_H_)
#error "The header file <ras.h> must be included prior to this file."
#endif

#if !defined(_NAVAJO_PORT_H_)
#error "The header file \"navajoPort.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_H_)
#error "The header file \"navajo.h\" must be included prior to this file."
#endif

#ifndef _HANDSHAKE_H_
#define _HANDSHAKE_H_

typedef HRESULT (HANDSHAKEPROC)(
    INT addressType,          /* in */
    LPCTSTR address,          /* in */
    LPCTSTR protocol,         /* in */
    LPCTSTR version,          /* in */
    LPCTSTR host,             /* in */
    LPCTSTR port,             /* in */
    ACK_CLIENTDATA clientData /* in */
);

typedef HANDSHAKEPROC *LPHANDSHAKEPROC;

typedef struct HANDSHAKESENDINFO_tag {
    LPCTSTR address;
    LPCTSTR protocol;
    LPCTSTR version;
    LPCTSTR host;
    LPCTSTR port;
    unsigned long broadcast;
} HANDSHAKESENDINFO, *LPHANDSHAKESENDINFO;

typedef struct HANDSHAKEREADINFO_tag {
    LPHANDSHAKEPROC proc;
    ACK_CLIENTDATA clientData;
} HANDSHAKEREADINFO, *LPHANDSHAKEREADINFO;

#if defined(FEATURE_RAS)
ACK_API HRESULT AcquireOrUpdateIpAddress(
    LPCTSTR entryName,
    LPHRASCONN phRasConn
);
#endif /* FEATURE_RAS */

ACK_API HRESULT QueryLocalIpAddressAndPort(
    BOOL bCellularLine,
    LPCWSTR *ppIpAddress,
    LPCWSTR *ppPort,
    unsigned long *ppIpBrodcast
);

#if defined(FEATURE_SMS)

ACK_API HRESULT StartupSmsSendThread(
    ACK_LPMUTEX pMutex,                    /* in */
    LPHANDSHAKESENDINFO pHandshakeSendInfo /* in */
);

ACK_API HRESULT StartupSmsReadThread(
    ACK_LPMUTEX pMutex,                    /* in */
    LPHANDSHAKEREADINFO pHandshakeReadInfo /* in */
);

ACK_API HRESULT ShutdownSmsThreads(
    ACK_LPMUTEX pMutex /* in */
);

#endif /* FEATURE_SMS */

#if defined(FEATURE_WIFI)

ACK_API HRESULT StartupWiFiBroadcastThread(
    ACK_LPMUTEX pMutex,                    /* in */
    LPHANDSHAKESENDINFO pHandshakeSendInfo /* in */
);

ACK_API HRESULT StartupWiFiSendThread(
    ACK_LPMUTEX pMutex,                    /* in */
    LPHANDSHAKESENDINFO pHandshakeSendInfo /* in */
);

ACK_API HRESULT StartupWiFiReadThread(
    ACK_LPMUTEX pMutex,                    /* in */
    LPHANDSHAKEREADINFO pHandshakeReadInfo /* in */
);

ACK_API HRESULT ShutdownWiFiThreads(
    ACK_LPMUTEX pMutex /* in */
);

#endif /* FEATURE_WIFI */
/*
 * NOTE: AlphaCipher SMS Handshake Protocol
 */

#ifndef HANDSHAKE_SMS_PROTOCOL_NAME
#define HANDSHAKE_SMS_PROTOCOL_NAME        "ACSHP"
#endif

#ifndef HANDSHAKE_WIFI_PROTOCOL_NAME
#define HANDSHAKE_WIFI_PROTOCOL_NAME       "ACWiFi"
#endif

#ifndef HANDSHAKE_PROTOCOL_VERSION
#define HANDSHAKE_PROTOCOL_VERSION         "1.0"
#endif

#ifndef HANDSHAKE_TEST_HOST
#define HANDSHAKE_TEST_HOST                "127.0.0.1"
#endif

#ifndef HANDSHAKE_TEST_PORT
#define HANDSHAKE_TEST_PORT                "12345"
#endif

#ifndef HANDSHAKE_FIELD_SEPARATOR
#define HANDSHAKE_FIELD_SEPARATOR            ","
#endif

#ifndef WAIT_OBJECT_1
#define WAIT_OBJECT_1        (WAIT_OBJECT_0 + 1)
#endif

#ifndef LOOPBACK_IP_ADDR
#define LOOPBACK_IP_ADDR              (16777343)
#endif

#endif /* _HANDSHAKE_H_ */

/* end of file */
