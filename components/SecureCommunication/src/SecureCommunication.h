#include "interfaces/if_OS_Socket.h"
#include <camkes.h>


static OS_Error_t waitForNetworkStackInit(const if_OS_Socket_t* const ctx);

static OS_Error_t waitForConnectionEstablished(const int handleId);

static OS_Error_t exchange_keys(void);