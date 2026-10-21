// SPDX-License-Identifier: BSD-2-Clause

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/serial/bl808-uart.h>
#include <sbi_utils/serial/fdt_serial.h>

static int serial_bl808_init(const void *fdt, int nodeoff,
			     const struct fdt_match *match)
{
	const fdt32_t *val;
	u64 addr, size;
	u32 freq, baud;
	int len, rc;

	rc = fdt_get_node_addr_size(fdt, nodeoff, 0, &addr, &size);
	if (rc)
		return rc;
	if (size < 0x90 || addr != (unsigned long)addr)
		return SBI_EINVAL;

	val = fdt_getprop(fdt, nodeoff, "clock-frequency", &len);
	if (!val || len != sizeof(*val))
		return SBI_EINVAL;
	freq = fdt32_to_cpu(*val);

	val = fdt_getprop(fdt, nodeoff, "current-speed", &len);
	if (!val || len != sizeof(*val))
		return SBI_EINVAL;
	baud = fdt32_to_cpu(*val);

	return bl808_uart_init(addr, freq, baud);
}

static const struct fdt_match serial_bl808_match[] = {
	{ .compatible = "bflb,bl808-uart" },
	{ },
};

const struct fdt_driver fdt_serial_bl808 = {
	.match_table = serial_bl808_match,
	.init = serial_bl808_init,
};
