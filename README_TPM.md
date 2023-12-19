# TPM

## Entropy source

### Connecting TestApp and TPM component

Before writing the code for the TPM itself, I needed to set up the program structure in a way that the TestApp component can read entropy bytes from the TPM component. Like in the homework tasks, TestApp uses an `entropy_rpc` and has `entropy_dp`:

```camkes
component TestApp {
  control;

  /* For connecting TPM RNG */
  uses		if_OS_Entropy	entropy_rpc;
  dataport	Buf		entropy_dp;
};
```

The TPM component *provides* the `entropy_rpc`. I had to rename `entropy_dp` to `entropy_port` because that's the name expected by the `EntropySource_INSTANCE_CONNECT_CLIENT` macro. I assume that I can't just use `EntropySource_COMPONENT_DEFINE`, because I'll need to expand the TPM component with more interface later.

```camkes
#include <if_OS_Entropy.camkes>

component TPM {
  provides	if_OS_Entropy	entropy_rpc;
  dataport	Buf		entropy_port;
}
```

In the `demo_tpm.camkes` file, nothing interesting happens, just the `#include`s of relevant CAmkES files, declaration of the TPM and TestApp components, and their connection via the `EntropySource_INSTANCE_CONNECT_CLIENT` macro.
