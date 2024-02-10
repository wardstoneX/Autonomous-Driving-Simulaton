
# SecureCommunication component

This component implements a key exchange algorithm based on [Microsoft Azure's IoT TPM attestation protocol](https://learn.microsoft.com/en-us/azure/iot-dps/concepts-tpm-attestation). Furthermore, data sent and received with this component through calls to the Socket API will we encrypted/decrypted with an AES256 algorithm is GCM mode before being forwarded to the corresponding network stack provider.

## Connecting the component
### CAmkES
In the main CAmkES file of the project, include the SecureCommunication component.
```camkes
#include "components/SecureCommunication/SecureCommunication.camkes"
```

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
    <client>, secureCommunication,
    1
)
```

