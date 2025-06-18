/*
 * SPDX-License-Identifier: GPL-2.0
 * virt_tty.c - A virtual null-modem TTY driver for Linux.
 *
 * Creates a pair of connected virtual TTY devices. Data written to
 * one device can be read from the other, and vice-versa.
 *
 * Author: Pluto Yang
 * Based on the modern tty_port infrastructure.
 * Target Kernel: 6.6+
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>

#define DRIVER_NAME         "virt_tty"
#define DEVICE_NAME_PREFIX  "ttyVIRT"
#define NUM_DEVICES         2

static struct tty_port port_array[NUM_DEVICES];
static struct tty_driver *virt_tty_driver;

static const struct tty_port_operations virt_tty_port_ops = {};

static int virt_tty_install(struct tty_driver *driver, struct tty_struct *tty)
{
	return tty_port_install(&port_array[tty->index], driver, tty);
}

static int virt_tty_open(struct tty_struct *tty, struct file *filp)
{
	int idx = tty->index;
	struct tty_port *port = &port_array[idx];

	tty_port_tty_set(port, tty);
	return tty_port_open(&port_array[tty->index], tty, filp);
}

static void virt_tty_close(struct tty_struct *tty, struct file *filp)
{
	tty_port_close(&port_array[tty->index], tty, filp);
}

static ssize_t virt_tty_write(struct tty_struct *tty, const unsigned char *buf, size_t count)
{
	int index = tty->index;
	int peer_index = !index;
	struct tty_port *peer_port = &port_array[peer_index];
	struct tty_struct *peer_tty;
	int written = 0;

	peer_tty = tty_port_tty_get(peer_port);
	if (!peer_tty)
		return -EIO;

	written = tty_insert_flip_string(peer_port, buf, count);
	if (written > 0)
		tty_flip_buffer_push(peer_port);

	tty_kref_put(peer_tty);
	return written;
}

static unsigned int virt_tty_write_room(struct tty_struct *tty)
{
	return 4096;
}

/*
 * The TTY core updates tty->termios before calling this. We don't need
 * to do anything, but it's good practice to acknowledge the baud rate
 * so that a subsequent TCGETS ioctl will see the new speed.
 */
static void virt_tty_set_termios(struct tty_struct *tty, const struct ktermios *old)
{
	speed_t baud = tty_termios_baud_rate(&tty->termios);

	/* For a virtual device, there is no hardware to update,
	 * but we can update the tty_struct's speed fields. */
	tty_encode_baud_rate(tty, baud, baud);
}

static const struct tty_operations virt_tty_ops = {
	.install       = virt_tty_install,
	.open          = virt_tty_open,
	.close         = virt_tty_close,
	.write         = virt_tty_write,
	.write_room    = virt_tty_write_room,
	.set_termios   = virt_tty_set_termios,
};

static int __init virt_tty_init(void)
{
	int ret;
	int i;
	pr_info("%s: loading driver with set_termios\n", DRIVER_NAME);
	virt_tty_driver = tty_alloc_driver(NUM_DEVICES,
					   TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV);
	if (IS_ERR(virt_tty_driver))
		return PTR_ERR(virt_tty_driver);

	virt_tty_driver->driver_name = DRIVER_NAME;
	virt_tty_driver->name = DEVICE_NAME_PREFIX;
	virt_tty_driver->owner = THIS_MODULE;
	/* It's good practice to define the driver type */
	virt_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	virt_tty_driver->subtype = SERIAL_TYPE_NORMAL;
	tty_set_operations(virt_tty_driver, &virt_tty_ops);

	for (i = 0; i < NUM_DEVICES; i++) {
		tty_port_init(&port_array[i]);
		port_array[i].ops = &virt_tty_port_ops;
	}

	ret = tty_register_driver(virt_tty_driver);
	if (ret) {
		pr_err("%s: failed to register tty driver\n", DRIVER_NAME);
		for (i = 0; i < NUM_DEVICES; i++)
			tty_port_destroy(&port_array[i]);
		tty_driver_kref_put(virt_tty_driver);
		return ret;
	}

	for (i = 0; i < NUM_DEVICES; i++)
		tty_register_device(virt_tty_driver, i, NULL);

	return 0;
}

static void __exit virt_tty_exit(void)
{
	int i;
	pr_info("%s: unloading driver with set_termios\n", DRIVER_NAME);
	for (i = 0; i < NUM_DEVICES; i++)
		tty_unregister_device(virt_tty_driver, i);
	tty_unregister_driver(virt_tty_driver);
	for (i = 0; i < NUM_DEVICES; i++)
		tty_port_destroy(&port_array[i]);
	tty_driver_kref_put(virt_tty_driver);
}

module_init(virt_tty_init);
module_exit(virt_tty_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Pluto Yang");
