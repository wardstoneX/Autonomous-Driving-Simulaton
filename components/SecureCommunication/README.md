# SecureCommunication component
This component provides encrypted socket communication between the client application and the network. It relies on an instance of the NetworkStack_PicoTcp component for network functionality. Communication is secured using symmetric AES256 encryption in GCM mode. The key for this encryption is exchanged with an algorithm based on [Microsoft Azure's IoT TPM attestation protocol](https://learn.microsoft.com/en-us/azure/iot-dps/concepts-tpm-attestation). RSA key generation and decryption is done by a WolfTPM component connected to this component. The WolfTPM component will also store the exchanged symmetric key, and provide random bytes needed for the encryption's IV.

## Connecting the component
Details about the declaration, instantiation, and configuration of the Network_PicoTcp and WolfTPM component can be read on the TRENTOS Handbook section 4.5 and the [WolfTPM documentation](../WolfTPM/README.md), respectively. The following sections focus only on the SecureCommunication component and its connections.

### CAmkES
In the main CAmkES file of the project, include the SecureCommunication component.
```camkes
#include "components/SecureCommunication/SecureCommunication.camkes"
```
#### Instantiation
Declare the component in the composition section of the file.
```camkes
assembly {
    composition {
        component	SecureCommunication	secureCommunication;
    }
}
```

Connect the SecureCommunication component as a client of the network stack
```camkes
NetworkStack_PicoTcp_INSTANCE_CONNECT_CLIENTS(
    nwStack,
    secureCommunication, networkStack
)
```
where the network stack was declared as:
```camkes
EXERCISE_DEMO_NIC_INSTANCE(nwDriver)
component	NetworkStack_PicoTcp	nwStack;
```

Furthermore, connect the SecureCommunication component's client using
```camkes
IF_OS_SOCKET_CONNECT(
    secureCommunication, secureCommunication,
    <client_instance>, secureCommunication,
    1
)
```

#### Configuration
The SecureCommunication component needs to be configured as a client of the NetworkStack component:
```camkes
NetworkStack_PicoTcp_CLIENT_ASSIGN_BADGES(
    secureCommunication, networkStack
)
```

#### Client side setup
In the CAmkES component definition of a SecureCommunication client, the macro IF_OS_SOCKET_USE(<if_OS_Socket_prefix>) will be used. Please use the prefix secureCommunication, as consistency across the prefixes is what allows the Socket API to function correctly. An example of how that would look is the following:
```camkes
#include <if_OS_Socket.camkes>

component Client {
    IF_OS_SOCKET_USE(secureCommunication)
}
```

### CMake
In the main CMakeLists.txt file of the project, add the following lines:
```cmake
include(components/SecureCommunication/CMakeLists.txt)

SecureCommunication_DeclareCAmkESComponent(SecureCommunication)

CAmkESAddCPPInclude(interfaces/camkes)
CAmkESAddImportPath(interfaces/camkes)
```
The last 2 lines are necessary to include some interfaces provided by the WolfTPM component used by the SecureCommunication component.

### C Source Files
The prefix used with IF_OS_SOCKET_USE will be used again in the C source file of this component's client in order to generate the ctx parameter needed for Socket API calls. This code will look like the following:
```c
#include "OS_Error.h"
#include "OS_Socket.h"
#include "interfaces/if_OS_Socket.h"
#include <camkes.h>

static const if_OS_Socket_t ctx = IF_OS_SOCKET_ASSIGN(secureCommunication);
```

## Socket API functionality with the SecureCommunication Component

### Not implemented
Functionality for the following Socket API calls is not implemented: OS_Socket_listen(), OS_Socket_accept(), OS_Socket_bind(), OS_Socket_getPendingEvents(), OS_Socket_wait(), OS_Socket_poll(), OS_Socket_regCallback().

### OS_Socket_create()
If the SecureCommunication component detects that a symmetric key has not been exchanged yet, this call will run the key exchange algorithm before proceeding to create a socket.

### OS_Socket_connect()
This call will connect to a socket, but contrary to NetworkStack_PicoTcp's implementation, the call will not return until an event confirming the successful connection has been received from the underlying network stack. This prevents the used from having to call OS_Socket_wait() after a call to this function.

### OS_Socket_write() and OS_Socket_sendto()
These functions encrypt the data to be sent using AES256 in GCM mode. For this, an arbitrary 12-byte long IV is generated and used in conjunction with the previously exchanged symmetric key. The generated IV is appended to the beginning of the message that is then forwarded to the underlying NetworkStack_PicoTcp component using the corresponding write/sendto call.

### OS_Socket_read() and OS_Socket_recvfrom()
These functions decrypt data received from the corresponding read/recvfrom call to the NetworkStack_PicoTcp commponent. The first 12 bytes of data are interpreted as the IV, which is used to decrypt the remaining bytes using AES256 in GCM mode with the previously exchanged symmetric key.

### OS_Socket_getStatus()
The return value of this function is the status of the underlying NetworkStack_PicoTcp component.

### OS_Socket_close()
This call's functionality remains essentially the same as when called directly from a NetworkStack_PicoTcp component.