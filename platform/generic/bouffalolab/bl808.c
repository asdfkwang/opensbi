// SPDX-License-Identifier: BSD-2-Clause

#include <platform_override.h>
#include <sbi/riscv_io.h>

#define MM_CLK_CTRL_CPU		((void *)0x30007000)
#define MM_CLK_CTRL_PERI	((void *)0x30007010)
#define MM_UART_CLK_SEL_MASK	(3 << 4)
#define MM_UART_CLK_SEL_XCLK	(2 << 4)
#define MM_XCLK_SEL_XTAL		BIT(10)
#define MM_UART_CLK_ENABLE	BIT(16)
#define MM_UART_CLK_DIV_MASK	(7 << 17)

#define GPIO16_CONFIG		((void *)0x20000904)
#define GPIO17_CONFIG		((void *)0x20000908)
#define GPIO_INPUT_ENABLE	BIT(0)
#define GPIO_SCHMITT_ENABLE	BIT(1)
#define GPIO_DRIVE_STRENGTH	BIT(2)
#define GPIO_PULL_UP		BIT(4)
#define GPIO_FUNC_MM_UART	(21 << 8)
#define GPIO_INT_MASK		BIT(22)

static int bl808_platform_init(const void *fdt, int nodeoff,
			       const struct fdt_match *match)
{
	u32 val;

	/* UART3 uses the 40 MHz crystal without a peripheral divider. */
	val = readl(MM_CLK_CTRL_PERI);
	writel(val & ~MM_UART_CLK_ENABLE, MM_CLK_CTRL_PERI);
	val = readl(MM_CLK_CTRL_CPU);
	val = (val & ~MM_UART_CLK_SEL_MASK) | MM_UART_CLK_SEL_XCLK |
	      MM_XCLK_SEL_XTAL;
	writel(val, MM_CLK_CTRL_CPU);
	val = readl(MM_CLK_CTRL_PERI);
	val = (val & ~MM_UART_CLK_DIV_MASK) | MM_UART_CLK_ENABLE;
	writel(val, MM_CLK_CTRL_PERI);

	/* Route UART3 TX/RX to the M1s Dock console on GPIO16/17. */
	val = GPIO_FUNC_MM_UART | GPIO_SCHMITT_ENABLE | GPIO_INT_MASK;
	writel(val, GPIO16_CONFIG);
	writel(val | GPIO_INPUT_ENABLE | GPIO_DRIVE_STRENGTH | GPIO_PULL_UP,
	       GPIO17_CONFIG);

	return 0;
}

static const struct fdt_match bl808_match[] = {
	{ .compatible = "sipeed,m1s-dock" },
	{ },
};

const struct fdt_driver bl808 = {
	.match_table = bl808_match,
	.init = bl808_platform_init,
};
