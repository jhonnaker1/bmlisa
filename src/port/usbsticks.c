/*
 * usbsticks.c - USB sticks for BMLisa.
 *
 * The kernel mounts up to three FAT USB sticks as the FatFs volumes "USB:",
 * "USB2:" and "USB3:" (src/kernel.cpp, from BMA800XL). While a stick is in,
 * it shows as a folder of that name in the card's top folder ("/USB:"), in
 * the menu's file browser and the web UI, and a path in it
 * ("/USB:/lisa/los.dc42") stands for the stick's ("USB:/lisa/los.dc42").
 * On the Pi, fswrap.c puts the folders in the listings and the paths
 * through bmlisa_usb_path() for every file call (IDLE's too).
 *
 * Pulling a stick out ejects the floppy and unplugs the hard disk loaded
 * from it (once a second, from platform.c); bmlisa.cfg keeps the hard
 * disk, so it is used again at the next boot with the stick in.
 *
 * From BMApple2's usbsticks.c (for KEGS).
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "../host.h"
#include "bmlisa.h"

/* The folder names the sticks have in the card's top folder. */
static const char *const kVolume[HOST_USB_STICKS] = {"USB:", "USB2:", "USB3:"};

/* PATH without the card's root folder ("/" on the Pi, the test's folder in
   the simulator) or the slashes at its start. */
static const char *after_root(const char *path)
{
	const char *root = bmlisa_sd_root();
	size_t len = strlen(root);
	while (len > 0 && root[len - 1] == '/')
		len--;
	if (len > 0 && strncmp(path, root, len) == 0 && path[len] == '/')
		path += len;
	while (*path == '/')
		path++;
	return path;
}

int bmlisa_usb_path(const char *path, char *out, size_t size)
{
	const char *p;
	int n;
	if (path == NULL)
		return -1;
	p = after_root(path);
	for (n = 0; n < HOST_USB_STICKS; n++) {
		size_t len = strlen(kVolume[n]);
		const char *root = host_usb_root(n);
		if (root == NULL || strncasecmp(p, kVolume[n], len) != 0 ||
		    (p[len] != '/' && p[len] != '\0'))
			continue;
		p += len;
		while (*p == '/')
			p++;
		if ((size_t)snprintf(out, size, "%s%s", root, p) >= size)
			return -1;
		return n;
	}
	return -1;
}

int bmlisa_usb_stick_of(const char *path)
{
	char real[1024];
	int n = bmlisa_usb_path(path, real, sizeof(real));
	if (n >= 0 || path == NULL)
		return n;
	/* A stick's own path ("USB:/x.dc42", or "usb/x.dc42" in the simulator) */
	while (path[0] == '.' && path[1] == '/')
		path += 2;
	for (n = 0; n < HOST_USB_STICKS; n++) {
		const char *root = host_usb_root(n);
		if (root != NULL && strncasecmp(path, root, strlen(root)) == 0)
			return n;
	}
	return -1;
}

int bmlisa_usb_is_card_root(const char *path)
{
	const char *p;
	if (path == NULL)
		return 0;
	p = after_root(path);
	if (strncasecmp(p, "SD:", 3) == 0)
		p += 3;
	while (*p == '/')
		p++;
	return *p == '\0' || strcmp(p, ".") == 0 || strcmp(p, "./") == 0;
}

const char *bmlisa_usb_folder(int i)
{
	int n;
	for (n = 0; n < HOST_USB_STICKS; n++) {
		if (host_usb_stick(n) != 0 && i-- == 0)
			return kVolume[n];
	}
	return NULL;
}

/* Ejects the floppies and unplugs the hard disk loaded from stick N. */
static void eject_stick(int n)
{
	int ejected = 0, drive;
	const char *hd = bmlisa_hard_disk();
	for (drive = 0; drive < 2; drive++) {
		const char *f = bmlisa_floppy(drive);
		if (f != NULL && bmlisa_usb_stick_of(f) == n) {
			bmlisa_eject_floppy(drive);
			ejected++;
		}
	}
	if (hd[0] != '\0' && bmlisa_usb_stick_of(hd) == n) {
		bmlisa_unplug_hard_disk();
		ejected++;
	}
	printf("BMLisa: USB stick %s removed: %d ejected\n", kVolume[n], ejected);
}

void bmlisa_check_usb_sticks(void)
{
	static unsigned last[HOST_USB_STICKS];
	int n;
	for (n = 0; n < HOST_USB_STICKS; n++) {
		unsigned now = host_usb_stick(n);
		if (last[n] != 0 && now != last[n])
			eject_stick(n);
		last[n] = now;
	}
}
