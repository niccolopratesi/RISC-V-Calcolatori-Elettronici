#include "internal.h"

// make_CAP now directly returns the memory area mapped
// to the sepcified PCI device.
natl* make_CAP(natb bus, natb dev, natb fun, natb off)
{
	// According to the previous work from Edoardo Geraci,
	// qemu -machine virt puts PCIe config space at 0x30000000.
	natl* ecam = (natl*)0x30000000L;
	natl offset = (bus << 16) | (dev << 11) | (fun << 8) | (off & 0xFC);
	return ecam + offset;
}
