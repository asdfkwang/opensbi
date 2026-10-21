// SPDX-License-Identifier: BSD-2-Clause

#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/serial/bl808-uart.h>

#define UART_TX_CONFIG		0x00
#define UART_RX_CONFIG		0x04
#define UART_BIT_PERIOD		0x08
#define UART_DATA_CONFIG	0x0c
#define UART_SW_MODE		0x1c
#define UART_INT_MASK		0x24
#define UART_INT_CLEAR		0x28
#define UART_INT_ENABLE		0x2c
#define UART_FIFO_CONFIG0	0x80
#define UART_FIFO_CONFIG1	0x84
#define UART_FIFO_WDATA		0x88
#define UART_FIFO_RDATA		0x8c

#define UART_TX_ENABLE		BIT(0)
#define UART_RX_ENABLE		BIT(0)
#define UART_TX_FREE_RUN		BIT(2)
#define UART_TX_DATA_BITS	(7 << 8)
#define UART_TX_STOP_BITS	BIT(11)
#define UART_RX_DATA_BITS	(7 << 8)
#define UART_INT_MASK_ALL	0xfff
#define UART_INT_CLEAR_ALL	0xf33
#define UART_TX_FIFO_CLEAR	BIT(2)
#define UART_RX_FIFO_CLEAR	BIT(3)
#define UART_TX_FIFO_COUNT	0x3f
#define UART_RX_FIFO_COUNT	(0x3f << 8)

static volatile void *uart_base;

static void bl808_uart_putc(char ch)
{
	while (!(readl(uart_base + UART_FIFO_CONFIG1) & UART_TX_FIFO_COUNT))
		;

	writeb(ch, uart_base + UART_FIFO_WDATA);
}

static int bl808_uart_getc(void)
{
	if (!(readl(uart_base + UART_FIFO_CONFIG1) & UART_RX_FIFO_COUNT))
		return -1;

	return readb(uart_base + UART_FIFO_RDATA);
}

static struct sbi_console_device bl808_console = {
	.name = "bl808_uart",
	.console_putc = bl808_uart_putc,
	.console_getc = bl808_uart_getc,
};

int bl808_uart_init(unsigned long base, u32 in_freq, u32 baudrate)
{
	u64 divisor;
	u32 period;
	int rc;

	if (!base || (base & 3) || base > -1UL - PAGE_SIZE ||
	    !in_freq || !baudrate)
		return SBI_EINVAL;

	divisor = ((u64)in_freq + baudrate / 2) / baudrate;
	if (!divisor || divisor > 0x10000)
		return SBI_EINVAL;

	rc = sbi_domain_root_add_memrange(base, PAGE_SIZE, PAGE_SIZE,
					  (SBI_DOMAIN_MEMREGION_MMIO |
					  SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW));
	if (rc)
		return rc;

	uart_base = (void *)base;
	writel(0, uart_base + UART_TX_CONFIG);
	writel(0, uart_base + UART_RX_CONFIG);

	/* The TX and RX period fields both encode divisor minus one. */
	period = divisor - 1;
	writel(period | (period << 16), uart_base + UART_BIT_PERIOD);
	writel(UART_TX_FREE_RUN | UART_TX_DATA_BITS | UART_TX_STOP_BITS,
	       uart_base + UART_TX_CONFIG);
	writel(UART_RX_DATA_BITS, uart_base + UART_RX_CONFIG);
	writel(0, uart_base + UART_DATA_CONFIG);
	writel(0, uart_base + UART_SW_MODE);

	/* Polling only: mask interrupts, disable DMA, and clear both FIFOs. */
	writel(UART_INT_MASK_ALL, uart_base + UART_INT_MASK);
	writel(0, uart_base + UART_INT_ENABLE);
	writel(UART_INT_CLEAR_ALL, uart_base + UART_INT_CLEAR);
	writel(UART_TX_FIFO_CLEAR | UART_RX_FIFO_CLEAR,
	       uart_base + UART_FIFO_CONFIG0);
	writel(0, uart_base + UART_FIFO_CONFIG1);

	writel(UART_TX_FREE_RUN | UART_TX_DATA_BITS | UART_TX_STOP_BITS |
	       UART_TX_ENABLE, uart_base + UART_TX_CONFIG);
	writel(UART_RX_DATA_BITS | UART_RX_ENABLE, uart_base + UART_RX_CONFIG);

	sbi_console_set_device(&bl808_console);

	return 0;
}
