// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2002
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 */

/*
 * SPI Read/Write Utilities
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <spi.h>

/*-----------------------------------------------------------------------
 * Definitions
 */

#ifndef MAX_SPI_BYTES
#   define MAX_SPI_BYTES 32	/* Maximum number of bytes we can handle */
#endif

#ifndef CONFIG_DEFAULT_SPI_BUS
#   define CONFIG_DEFAULT_SPI_BUS	0
#endif
#ifndef CONFIG_DEFAULT_SPI_MODE
#   define CONFIG_DEFAULT_SPI_MODE	SPI_MODE_0
#endif

/*
 * Values from last command.
 */
static unsigned int	bus;
static unsigned int	cs;
static unsigned int	mode;
static int		speed = 1000000;
static int		wordlen = 8;
static int   		bitlen;
static unsigned int	xfer_flags;
struct spi_slave	*g_slave;
#define XFERS_MAX	8
static uchar		* buf_list[XFERS_MAX];
static int		buf_cnt;
static uchar		*dout;
static uchar		*din;

static int do_spi_xfer(int bus, int cs)
{
	struct spi_slave *slave;
	int ret = 0;

	if (g_slave && !(xfer_flags & SPI_XFER_BEGIN)) {
		slave = g_slave;
		goto next_xfer;
	}
#ifdef CONFIG_DM_SPI
	char name[30], *str;
	struct udevice *dev;

	snprintf(name, sizeof(name), "generic_%d:%d", bus, cs);
	str = strdup(name);
	if (!str)
		return -ENOMEM;
	ret = spi_get_bus_and_cs(bus, cs, speed, mode, "spi_generic_drv",
				 str, &dev, &slave);
	if (ret)
		return ret;
#else
	slave = spi_setup_slave(bus, cs, speed, mode);
	if (!slave) {
		printf("Invalid device %d:%d\n", bus, cs);
		return -EINVAL;
	}
#endif
	g_slave = slave;
	slave->wordlen = wordlen;
	ret = spi_claim_bus(slave);
	if (ret)
		goto done;
next_xfer:
	ret = spi_xfer(slave, bitlen, dout, din, xfer_flags);
	if (ret > 0)
		return 0;
#ifndef CONFIG_DM_SPI
	/* We don't get an error code in this case */
	if (ret)
		ret = -EIO;
#endif
	if (ret) {
		printf("Error %d during SPI transaction\n", ret);
	} else {
		int j;

		for (j = 0; j < ((bitlen + 7) / 8); j++)
			printf("%02X", din[j]);
		printf("\n");
	}
done:
	g_slave = 0;
	for (int i = 0; i < buf_cnt; i++)
		kfree(buf_list[i]);
	buf_cnt = 0;
	spi_release_bus(slave);
#ifndef CONFIG_DM_SPI
	spi_free_slave(slave);
#endif

	return ret;
}

/*
 * SPI read/write
 *
 * Syntax:
 *   spi {dev} {num_bits} {dout}
 *     {dev} is the device number for controlling chip select (see TBD)
 *     {num_bits} is the number of bits to send & receive (base 10)
 *     {dout} is a hexadecimal string of data to send
 * The command prints out the hexadecimal string received via SPI.
 */

int do_spi (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char  *cp = 0;
	uchar tmp;
	int   j;

	/*
	 * We use the last specified parameters, unless new ones are
	 * entered.
	 */

	if ((flag & CMD_FLAG_REPEAT) == 0)
	{
		if (argc >= 2) {
			mode = CONFIG_DEFAULT_SPI_MODE;
			bus = simple_strtoul(argv[1], &cp, 10);
			if (*cp == ':') {
				cs = simple_strtoul(cp+1, &cp, 10);
			} else {
				cs = bus;
				bus = CONFIG_DEFAULT_SPI_BUS;
			}
			if (*cp == '.')
				mode = simple_strtoul(cp+1, &cp, 10);
			if (*cp == '.')
				speed = simple_strtoul(cp+1, &cp, 10);
			if (*cp == '.')
				wordlen = simple_strtoul(cp+1, &cp, 10);
			if (*cp == '.')
				xfer_flags = simple_strtoul(cp + 1, &cp, 16);
			else
				xfer_flags = SPI_XFER_BEGIN | SPI_XFER_END;
		}
		if (argc >= 3) {
			bitlen = simple_strtoul(argv[2], NULL, 10);
			dout = kcalloc(ALIGN(bitlen / 8, 8), 2, GFP_KERNEL);
			if (!dout)
				return -ENOMEM;
			din = dout + ALIGN(bitlen / 8, 8);
			buf_list[buf_cnt] = dout;
			if (++buf_cnt >= XFERS_MAX)
				xfer_flags |= SPI_XFER_END;
		}
		if (argc >= 4) {
			cp = argv[3];
			for(j = 0; *cp; j++, cp++) {
				tmp = *cp - '0';
				if(tmp > 9)
					tmp -= ('A' - '0') - 10;
				if(tmp > 15)
					tmp -= ('a' - 'A');
				if(tmp > 15) {
					printf("Hex conversion error on %c\n", *cp);
					return 1;
				}
				if((j % 2) == 0)
					dout[j / 2] = (tmp << 4);
				else
					dout[j / 2] |= tmp;
			}
		}
	}

	if ((bitlen < 0) || (bitlen >  (MAX_SPI_BYTES * 8))) {
		printf("Invalid bitlen %d\n", bitlen);
		return 1;
	}

	if (do_spi_xfer(bus, cs))
		return 1;

	return 0;
}

/***************************************************/

U_BOOT_CMD(
	sspi,	5,	1,	do_spi,
	"SPI utility command",
	"[<bus>:]<cs>[.<mode>] <bit_len> <dout> - Send and receive bits\n"
	"<bus>     - Identifies the SPI bus\n"
	"<cs>      - Identifies the chip select\n"
	"<mode>    - Identifies the SPI mode to use\n"
	"<bit_len> - Number of bits to send (base 10)\n"
	"<dout>    - Hexadecimal string that gets sent"
);
