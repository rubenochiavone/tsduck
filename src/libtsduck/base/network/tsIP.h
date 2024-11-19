//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup net
//!  Include the multiple and messy system headers for IP networking.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

#if defined(TS_WINDOWS)
    #include "tsBeforeStandardHeaders.h"
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <mswsock.h>
    #include "tsAfterStandardHeaders.h"
    #if defined(TS_MSC)
        #pragma comment(lib, "ws2_32.lib")
    #endif
#else
    #include "tsBeforeStandardHeaders.h"
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <net/if.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <netdb.h>
    #include <ifaddrs.h>
    #include "tsAfterStandardHeaders.h"
#endif

namespace ts {
    //!
    //! Generation of IP networks as an enum class.
    //! @ingroup net
    //!
    enum class IP : uint8_t {
        Any = 0,  //!< Any generation of IP networks.
        v4  = 4,  //!< IPv4.
        v6  = 6,  //!< IPv6.
    };
}
