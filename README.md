# virt_tty - Virtual Null-Modem TTY Driver for Linux

`virt_tty` is a Linux kernel module that creates a pair of connected virtual TTY devices (like a null-modem cable). Data written to one device can be read from the other, and vice-versa.

This is useful for:
- Testing serial communication code without physical hardware.
- Creating virtual loops for debugging serial tools.
- Interfacing between software expecting serial devices.

A matching `udev` rule can be installed to assign permissions and group access. For example:

```udev
# /etc/udev/rules.d/99-virt_tty.rules
KERNEL=="ttyVIRT*", SUBSYSTEM=="tty", MODE="0660", GROUP="dialout"
```

---

## Features

- Creates two connected TTY devices: `/dev/ttyVIRT0` and `/dev/ttyVIRT1`.
- Full-duplex communication (bi-directional).
- Integrates with Linux TTY layer via modern `tty_port` and `tty_driver` APIs.
- Compatible with modern Linux kernels (6.6+).

---

## Requirements

- Linux kernel 6.6 or later.
- Kernel headers installed (`linux-headers-$(uname -r)`).

---

## Building

```bash
make
```
