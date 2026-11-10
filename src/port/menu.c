/*
 * menu.c - the BMLisa menu.
 *
 * F12, a gamepad's Home button or Select+Start open it: the Lisa's disks,
 * its power button, speed and model, and BMLisa's settings (video,
 * network, power). Settings are saved in bmlisa.cfg and used again at the
 * next boot.
 *
 * From BMApple2's menu.c.
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../host.h"
#include "bmlisa.h"
#include "lisa.h"
#include "osd.h"

#ifndef BMLISA_VERSION
#define BMLISA_VERSION "dev"
#endif

#define WPA_CONFIG "wpa_supplicant.conf"

/* Something changed that only takes effect after a reboot. */
static int reboot_needed;

/* ---- Settings ---------------------------------------------------------- */

static void save_setting(const char *name, const char *value)
{
	if (!host_option_set(name, value) || !host_options_save())
		osd_message("Cannot save bmlisa.cfg", TRUE);
}

static void save_setting_int(const char *name, int value)
{
	char text[16];
	snprintf(text, sizeof(text), "%d", value);
	save_setting(name, text);
}

static int setting_on(const char *name, int default_value)
{
	const char *value = host_option(name, NULL);
	if (value == NULL || *value == '\0')
		return default_value;
	return strcmp(value, "1") == 0 || strcmp(value, "yes") == 0
	    || strcmp(value, "true") == 0 || strcmp(value, "on") == 0;
}

static unsigned check(int on)
{
	return OSD_ITEM_CHECK | (on ? OSD_ITEM_CHECKED : 0);
}

static void power(int off)
{
	bmlisa_request_power(off);
	bmlisa_idle();
}

/* ---- Disks ---------------------------------------------------------------- */

/* Where the file browser starts: the disks folder, then where the last
   disk was chosen. */
static char browse_dir[256];

static void start_browsing(void)
{
	const char *root = bmlisa_sd_root();
	size_t len = strlen(root);
	if (browse_dir[0] == '\0')
		snprintf(browse_dir, sizeof browse_dir, "%s%sdisks", root,
		         len > 0 && root[len - 1] == '/' ? "" : "/");
}

/* The file's name, without its folder. */
static const char *base_name(const char *path)
{
	const char *slash = path != NULL ? strrchr(path, '/') : NULL;
	return slash != NULL ? slash + 1 : path;
}

/* With a disk in: insert another, or eject it. Returns 0 to choose
   another, 1 if it was ejected, -1 if nothing was chosen. */
static int insert_or_eject(const char *title, const char *eject)
{
	const char *choices[] = {"Insert Another...", eject, NULL};
	int i = osd_choose(title, choices, 0);
	return i < 0 ? -1 : i;
}

static int choose_floppy(int drive)
{
	static const char *const exts[] = {"dc42", "image", "img", "dsk", "dc", NULL};
	char path[512];
	if (bmlisa_floppy(drive) != NULL) {
		int i = insert_or_eject(drive == 1 ? "Lower Floppy" : "Floppy", "Eject");
		if (i < 0)
			return FALSE;
		if (i == 1) {
			bmlisa_eject_floppy(drive);
			return TRUE;
		}
	}
	start_browsing();
	if (!osd_browse("Floppy Disk Image", browse_dir, sizeof browse_dir, exts, path, sizeof path))
		return FALSE;
	if (!bmlisa_insert_floppy(path, drive)) {
		osd_message("That isn't a floppy image the Lisa can read (Disk Copy 4.2).", TRUE);
		return FALSE;
	}
	return TRUE;
}

static int choose_hard_disk(void)
{
	char path[512];
	if (bmlisa_hard_disk()[0] != '\0') {
		int i = insert_or_eject("Hard Disk", "Eject (unplug the ProFile)");
		if (i < 0)
			return FALSE;
		if (i == 1) {
			if (osd_confirm("Eject the Hard Disk",
			                "Shut the Lisa down first, or what isn't saved is lost. Eject it?"))
				bmlisa_eject_hard_disk();
			return TRUE;
		}
	}
	start_browsing();
	if (!osd_browse("Hard Disk Image", browse_dir, sizeof browse_dir, NULL, path, sizeof path))
		return FALSE;
	if (!bmlisa_set_hard_disk(path)) {
		osd_message("That isn't a ProFile hard disk image (raw, or LisaEm's .dc42).", TRUE);
		return FALSE;
	}
	return TRUE;
}

/* ---- Video -------------------------------------------------------------- */

static const char *vsync_mode(void)
{
	const char *mode = host_option("vsync", "auto");
	if (strcmp(mode, "on") == 0 || strcmp(mode, "1") == 0)
		return "On";
	if (strcmp(mode, "off") == 0 || strcmp(mode, "0") == 0)
		return "Off";
	return bmlisa_vsync_locked() ? "Auto (locked)" : "Auto (timer)";
}

/* The picture's height as a percentage of the screen's (the scale option):
   less for a TV that cuts off the edges (overscan). From BMCoco. */
static int screen_size(void)
{
	double scale = host_option_double("scale", 1.0);
	if (scale <= 0.1 || scale > 1.0)
		scale = 1.0;
	return (int) (scale * 100.0 + 0.5);
}

static void choose_screen_size(void)
{
	static const char *const choices[] = {
		"100%: fill the screen", "95%", "90%", "85%", "80%", NULL
	};
	static const int sizes[] = {100, 95, 90, 85, 80};
	int size = screen_size(), current = 0, k, i;
	char text[16];
	for (k = 1; k < 5; k++) {
		int d = sizes[k] - size, best = sizes[current] - size;
		if ((d < 0 ? -d : d) < (best < 0 ? -best : best))
			current = k;
	}
	i = osd_choose("Screen Size", choices, current);
	if (i < 0)
		return;
	snprintf(text, sizeof text, "%d.%02d", sizes[i] / 100, sizes[i] % 100);
	save_setting("scale", text);
	bmlisa_video_settings_changed();
}

static void video_menu(void)
{
	enum { V_MONITOR, V_SIZE, V_VSYNC, V_RESOLUTION, V_SHADER, V_CURVATURE, V_SMOOTH };
	/* The Pi 5 only: its firmware ignores hdmi_mode (from BMA800XL). */
	static const char *const resolutions[] = {"auto", "1920x1080", "1280x720", NULL};
	static const char *const resolution_names[] = {"Auto", "1920x1080", "1280x720", NULL};
	int sel = 0;
	for (;;) {
		const char *res = host_option("resolution", "auto");
		int res_index = 0, n_items, k;
		char size_text[8];
		snprintf(size_text, sizeof size_text, "%d%%", screen_size());
		for (k = 0; resolutions[k] != NULL; k++)
			if (strcmp(res, resolutions[k]) == 0)
				res_index = k;
		struct osd_item items[] = {
			{"Monitor", bmlisa_monitor_name(bmlisa_monitor()), OSD_ITEM_SUBMENU, V_MONITOR},
			{"Screen Size", size_text, OSD_ITEM_SUBMENU, V_SIZE},
			{"Frame Lock", vsync_mode(), OSD_ITEM_SUBMENU, V_VSYNC},
			{"Resolution", resolution_names[res_index], OSD_ITEM_SUBMENU, V_RESOLUTION},
			{"CRT Effect (reboot)", NULL, check(setting_on("crt_shader", FALSE)), V_SHADER},
			{"CRT Curvature (reboot)", NULL, check(setting_on("crt_curvature", FALSE)), V_CURVATURE},
			{"Smooth Scaling (reboot)", NULL, check(setting_on("smooth", FALSE)), V_SMOOTH},
			{"", NULL, OSD_ITEM_LABEL, -1},
			{"If your TV cuts off the edges, make", NULL, OSD_ITEM_LABEL, -1},
			{"the Screen Size smaller.", NULL, OSD_ITEM_LABEL, -1},
		};
		int chosen, i;
		n_items = sizeof(items) / sizeof(items[0]);
		if (!host_display_resolution_choice()) {
			/* Resolution is only for the Pi 5 (with video_driver=kms). */
			for (k = 3; k < n_items - 1; k++)
				items[k] = items[k + 1];
			n_items--;
		}
		chosen = osd_menu("Video", items, n_items, &sel);
		switch (chosen) {
		case V_SIZE:
			choose_screen_size();
			break;
		case V_RESOLUTION:
			i = osd_choose("Resolution", resolution_names, res_index);
			if (i >= 0 && i != res_index) {
				save_setting("resolution", resolutions[i]);
				bmlisa_display_mode_changed();
			}
			break;
		case V_CURVATURE:
			save_setting("crt_curvature", setting_on("crt_curvature", FALSE) ? "0" : "1");
			reboot_needed = TRUE;
			break;
		case V_MONITOR: {
			const char *names[BMLISA_MONITORS + 1];
			int m;
			for (m = 0; m < BMLISA_MONITORS; m++)
				names[m] = bmlisa_monitor_name(m);
			names[BMLISA_MONITORS] = NULL;
			m = osd_choose("Monitor", names, bmlisa_monitor());
			if (m >= 0)
				bmlisa_set_monitor(m);
			break;
		}
		case V_VSYNC: {
			static const char *const choices[] = {
				"Automatic", "On: follow the display", "Off: keep exact time", NULL
			};
			static const char *const values[] = {"auto", "on", "off"};
			const char *mode = host_option("vsync", "auto");
			int current = strcmp(mode, "on") == 0 ? 1 : strcmp(mode, "off") == 0 ? 2 : 0;
			i = osd_choose("Frame Lock", choices, current);
			if (i >= 0) {
				save_setting("vsync", values[i]);
				bmlisa_video_settings_changed();
			}
			break;
		}
		case V_SHADER:
			save_setting("crt_shader", setting_on("crt_shader", FALSE) ? "0" : "1");
			reboot_needed = TRUE;
			break;
		case V_SMOOTH:
			save_setting("smooth", setting_on("smooth", FALSE) ? "0" : "1");
			reboot_needed = TRUE;
			break;
		default:
			return;
		}
	}
}

/* ---- Network --------------------------------------------------------------- */

static char wifi_ssid[33], wifi_psk[64], wifi_country[3];

static void wpa_path(char *path, size_t size)
{
	const char *root = bmlisa_sd_root();
	size_t len = strlen(root);
	snprintf(path, size, "%s%s%s", root, len > 0 && root[len - 1] == '/' ? "" : "/", WPA_CONFIG);
}

static void copy_quoted(char *dst, size_t size, const char *src)
{
	const char *end;
	if (*src == '"')
		src++;
	end = strchr(src, '"');
	if (end == NULL)
		end = src + strcspn(src, "\r\n");
	if ((size_t) (end - src) >= size)
		end = src + size - 1;
	memcpy(dst, src, end - src);
	dst[end - src] = '\0';
}

static void read_wifi_config(void)
{
	char path[256], line[160];
	FILE *f;
	wifi_ssid[0] = wifi_psk[0] = '\0';
	strcpy(wifi_country, "US");
	wpa_path(path, sizeof(path));
	f = fopen(path, "r");
	if (f == NULL)
		return;
	while (fgets(line, sizeof(line), f) != NULL) {
		char *p = line + strspn(line, " \t");
		if (strncmp(p, "country=", 8) == 0)
			copy_quoted(wifi_country, sizeof(wifi_country), p + 8);
		else if (strncmp(p, "ssid=", 5) == 0 && wifi_ssid[0] == '\0')
			copy_quoted(wifi_ssid, sizeof(wifi_ssid), p + 5);
		else if (strncmp(p, "psk=", 4) == 0 && wifi_psk[0] == '\0')
			copy_quoted(wifi_psk, sizeof(wifi_psk), p + 4);
	}
	fclose(f);
}

static int write_wifi_config(void)
{
	char path[256];
	FILE *f;
	wpa_path(path, sizeof(path));
	f = fopen(path, "w");
	if (f == NULL)
		return FALSE;
	fprintf(f, "country=%s\n\nnetwork={\n\tssid=\"%s\"\n", wifi_country, wifi_ssid);
	if (wifi_psk[0] != '\0')
		fprintf(f, "\tpsk=\"%s\"\n\tkey_mgmt=WPA-PSK\n", wifi_psk);
	else
		fprintf(f, "\tkey_mgmt=NONE\n");
	fprintf(f, "}\n");
	return fclose(f) == 0;
}

static void edit_wifi(const char *title, char *value, size_t size, int secret)
{
	char text[64];
	snprintf(text, sizeof(text), "%s", value);
	if (osd_edit(title, text, size < sizeof(text) ? size : sizeof(text), secret)) {
		snprintf(value, size, "%s", text);
		if (write_wifi_config())
			reboot_needed = TRUE;
		else
			osd_message("Cannot write " WPA_CONFIG, TRUE);
	}
}

static int compare_signal(const void *a, const void *b)
{
	return ((const struct host_wifi_ap *) b)->signal - ((const struct host_wifi_ap *) a)->signal;
}

/* Returns 1 if a network was chosen, 2 if it also needs a password. */
static int scan_wifi(void)
{
	enum { MAX_APS = 20 };
	static struct host_wifi_ap aps[MAX_APS];
	static char signal[MAX_APS][16];
	struct osd_item items[MAX_APS];
	int n, i, sel = 0, chosen;

	osd_message("Scanning for Wi-Fi networks...", FALSE);
	n = host_wifi_scan(aps, MAX_APS);
	if (n < 0) {
		if (osd_confirm("Wi-Fi is Off", "Switch to Wi-Fi and reboot?")) {
			save_setting("network", "wifi");
			power(0);
		}
		return 0;
	}
	if (n == 0) {
		osd_message("No Wi-Fi networks found.", TRUE);
		return 0;
	}
	qsort(aps, n, sizeof(aps[0]), compare_signal);
	for (i = 0; i < n; i++) {
		snprintf(signal[i], sizeof(signal[i]), "%s%d", aps[i].secure ? "* " : "", aps[i].signal);
		items[i] = (struct osd_item){aps[i].ssid, signal[i], 0, i};
	}
	chosen = osd_menu("Choose a Wi-Fi Network", items, n, &sel);
	if (chosen < 0)
		return 0;
	if (!aps[chosen].secure || strcmp(wifi_ssid, aps[chosen].ssid) != 0)
		wifi_psk[0] = '\0';
	snprintf(wifi_ssid, sizeof(wifi_ssid), "%s", aps[chosen].ssid);
	if (!write_wifi_config()) {
		osd_message("Cannot write " WPA_CONFIG, TRUE);
		return 0;
	}
	save_setting("network", "wifi");
	reboot_needed = TRUE;
	return aps[chosen].secure && wifi_psk[0] == '\0' ? 2 : 1;
}

static void network_menu(void)
{
	enum { N_DEVICE, N_SCAN, N_SSID, N_PASSWORD, N_COUNTRY, N_WEBUI, N_PIN, N_TIMEZONE, N_REBOOT };
	static const char *const devices[] = {"off", "ethernet", "wifi", NULL};
	static const char *const device_names[] = {"Off", "Ethernet", "Wi-Fi", NULL};
	int sel = 2;
	for (;;) {
		char status[64], tz[16];
		const char *net = host_option("network", "off");
		const char *pin = host_option("webui_pin", "");
		int device = strcmp(net, "ethernet") == 0 ? 1 : strcmp(net, "wifi") == 0 ? 2 : 0;
		int minutes = host_option_int("timezone", 0);
		int chosen, i;

		read_wifi_config();
		host_network_status(status, sizeof(status));
		if (minutes == 0)
			strcpy(tz, "UTC");
		else
			snprintf(tz, sizeof(tz), "UTC%+d", minutes / 60);
		{
			struct osd_item items[] = {
				{status, NULL, OSD_ITEM_LABEL, -1},
				{"", NULL, OSD_ITEM_LABEL, -1},
				{"Network", device_names[device], OSD_ITEM_SUBMENU, N_DEVICE},
				{"Scan for Wi-Fi Networks", NULL, OSD_ITEM_SUBMENU, N_SCAN},
				{"Wi-Fi Network", wifi_ssid[0] ? wifi_ssid : "(none)", OSD_ITEM_SUBMENU, N_SSID},
				{"Wi-Fi Password", wifi_psk[0] ? "********" : "(none)", OSD_ITEM_SUBMENU, N_PASSWORD},
				{"Wi-Fi Country", wifi_country, OSD_ITEM_SUBMENU, N_COUNTRY},
				{"Web UI", NULL, check(setting_on("webui", TRUE)), N_WEBUI},
				{"Web UI PIN", pin[0] ? pin : "(none)", OSD_ITEM_SUBMENU, N_PIN},
				{"Time Zone", tz, OSD_ITEM_SUBMENU, N_TIMEZONE},
				{"", NULL, OSD_ITEM_LABEL, -1},
				{"Changes apply after a reboot.", NULL, OSD_ITEM_LABEL, -1},
				{"Reboot Now", NULL, 0, N_REBOOT},
			};
			chosen = osd_menu("Network", items, sizeof(items) / sizeof(items[0]), &sel);
		}
		switch (chosen) {
		case N_DEVICE:
			i = osd_choose("Network", device_names, device);
			if (i >= 0 && i != device) {
				save_setting("network", devices[i]);
				reboot_needed = TRUE;
			}
			break;
		case N_SCAN:
			if (scan_wifi() == 2) {
				wifi_psk[0] = '\0';
				edit_wifi("Wi-Fi Password", wifi_psk, sizeof(wifi_psk), TRUE);
			}
			break;
		case N_SSID:
			edit_wifi("Wi-Fi Network Name", wifi_ssid, sizeof(wifi_ssid), FALSE);
			break;
		case N_PASSWORD:
			wifi_psk[0] = '\0';
			edit_wifi("Wi-Fi Password (blank: none)", wifi_psk, sizeof(wifi_psk), TRUE);
			break;
		case N_COUNTRY:
			edit_wifi("Two Letter Country Code", wifi_country, sizeof(wifi_country), FALSE);
			break;
		case N_WEBUI:
			save_setting("webui", setting_on("webui", TRUE) ? "0" : "1");
			reboot_needed = TRUE;
			break;
		case N_PIN: {
			char text[9];
			snprintf(text, sizeof(text), "%s", pin);
			if (osd_edit("Web UI PIN (blank: none)", text, sizeof(text), FALSE)) {
				save_setting("webui_pin", text);
				reboot_needed = TRUE;
			}
			break;
		}
		case N_TIMEZONE: {
			static char labels[27][8];
			static const char *zones[28];
			int h, current = 12;
			for (h = -12; h <= 14; h++) {
				if (h == 0)
					strcpy(labels[h + 12], "UTC");
				else
					snprintf(labels[h + 12], sizeof(labels[0]), "UTC%+d", h);
				zones[h + 12] = labels[h + 12];
				if (h * 60 == minutes)
					current = h + 12;
			}
			zones[27] = NULL;
			h = osd_choose("Time Zone", zones, current);
			if (h >= 0 && h != current) {
				save_setting_int("timezone", (h - 12) * 60);
				reboot_needed = TRUE;
			}
			break;
		}
		case N_REBOOT:
			power(0);
			return;
		default:
			return;
		}
	}
}

/* ---- Information ----------------------------------------------------------- */

static void about(void)
{
	char text[640], status[64];
	int w, h;
	host_display_size(&w, &h);
	host_network_status(status, sizeof(status));
	snprintf(text, sizeof(text),
	         "BMLisa " BMLISA_VERSION "\n"
	         "Bare metal Apple Lisa emulator\n"
	         "for the Raspberry Pi.\n\n"
	         "Emulation: IDLE 0.10\n"
	         "by Gilles Fetis (BSD licence).\n"
	         "Based on BMApple2, BMCoco,\n"
	         "BMA800XL and BMC64.\n\n"
	         "Machine: %s\n"
	         "Hard disk: %s\n"
	         "Display: %dx%d %.2f Hz\n"
	         "Frames: %.2f Hz, %s\n"
	         "Network: %s",
	         bmlisa_machine_name(bmlisa_machine()), base_name(bmlisa_hard_disk()),
	         w, h, bmlisa_refresh_rate(), LISA_FRAME_RATE,
	         bmlisa_vsync_locked() ? "locked to display" : "timer paced", status);
	osd_info("About BMLisa", text);
}

static void offer_reboot(void)
{
	if (osd_confirm("Reboot?", "Settings changed that need a reboot. Reboot now?"))
		power(0);
}

/* ---- Machine ------------------------------------------------------------------ */

/* The models there are ROMs for; choosing one reboots into it. */
static void machine_menu(void)
{
	char names[BMLISA_MACHINES][48];
	const char *choices[BMLISA_MACHINES + 1];
	int m;
	for (m = 0; m < BMLISA_MACHINES; m++) {
		snprintf(names[m], sizeof names[m], "%s%s", bmlisa_machine_name(m),
		         bmlisa_machine_available(m) ? "" : " (no ROMs)");
		choices[m] = names[m];
	}
	choices[BMLISA_MACHINES] = NULL;
	m = osd_choose("Machine (reboots)", choices, bmlisa_machine() < 0 ? 0 : bmlisa_machine());
	if (m < 0 || m == bmlisa_machine())
		return;
	if (!bmlisa_machine_available(m)) {
		osd_message("Its ROMs aren't in the roms folder (see roms/ROMS.txt).", TRUE);
		return;
	}
	bmlisa_set_machine(m);
}

/* ---- Main menu ---------------------------------------------------------------- */

void bmlisa_menu(void)
{
	enum { M_FLOPPY, M_FLOPPY2, M_HARD_DISK, M_POWER_BUTTON, M_RESTART, M_NMI, M_FAST,
	       M_MACHINE, M_VIDEO, M_NETWORK, M_ABOUT, M_REBOOT, M_OFF };
	static int sel = 0;
	char title[48];

	snprintf(title, sizeof(title), "BMLisa %s", BMLISA_VERSION);
	reboot_needed = FALSE;
	osd_open();
	for (;;) {
		int two = bmlisa_lisa_floppy_drives() == 2;
		const char *f1 = base_name(bmlisa_floppy(0)), *f2 = base_name(bmlisa_floppy(1));
		struct osd_item items[] = {
			{two ? "Upper Floppy" : "Floppy", f1 ? f1 : "(insert)", OSD_ITEM_SUBMENU, M_FLOPPY},
			{"Lower Floppy", f2 ? f2 : "(insert)", OSD_ITEM_SUBMENU, M_FLOPPY2},
			{"Hard Disk", bmlisa_hard_disk()[0] ? base_name(bmlisa_hard_disk()) : "(none)",
			 OSD_ITEM_SUBMENU, M_HARD_DISK},
			{"", NULL, OSD_ITEM_LABEL, -1},
			{"Lisa Power Button (F10)", NULL, 0, M_POWER_BUTTON},
			{"Restart the Lisa", NULL, 0, M_RESTART},
			{"Interrupt (NMI)", NULL, 0, M_NMI},
			{"Fast", NULL, check(bmlisa_fast()), M_FAST},
			{"Machine", bmlisa_machine_name(bmlisa_machine()), OSD_ITEM_SUBMENU, M_MACHINE},
			{"", NULL, OSD_ITEM_LABEL, -1},
			{"Video", NULL, OSD_ITEM_SUBMENU, M_VIDEO},
			{"Network", NULL, OSD_ITEM_SUBMENU, M_NETWORK},
			{"About", NULL, OSD_ITEM_SUBMENU, M_ABOUT},
			{"", NULL, OSD_ITEM_LABEL, -1},
			{"Reboot", NULL, 0, M_REBOOT},
			{"Shut Down", NULL, 0, M_OFF},
		};
		int n_items = sizeof(items) / sizeof(items[0]), k;
		int chosen, leave = FALSE;
		if (!two) {
			/* One drive: no Lower Floppy. */
			for (k = 1; k < n_items - 1; k++)
				items[k] = items[k + 1];
			n_items--;
		}
		chosen = osd_menu(title, items, n_items, &sel);

		switch (chosen) {
		case M_FLOPPY: leave = choose_floppy(0); break;
		case M_FLOPPY2: leave = choose_floppy(1); break;
		case M_HARD_DISK: leave = choose_hard_disk(); break;
		case M_POWER_BUTTON: bmlisa_power_button(); leave = TRUE; break;
		case M_RESTART:
			if (osd_confirm("Restart", "Restart the Lisa? What isn't saved is lost."))
				bmlisa_restart();
			leave = TRUE;
			break;
		case M_NMI: bmlisa_lisa_nmi(); leave = TRUE; break;
		case M_FAST: bmlisa_set_fast(!bmlisa_fast()); break;
		case M_MACHINE: machine_menu(); break;
		case M_VIDEO: video_menu(); break;
		case M_NETWORK: network_menu(); break;
		case M_ABOUT: about(); break;
		case M_REBOOT:
			if (osd_confirm("Reboot", "Reboot BMLisa?"))
				power(0);
			break;
		case M_OFF:
			if (osd_confirm("Shut Down", "Shut down? It's then safe to switch off."))
				power(1);
			break;
		default:
			leave = TRUE;
			break;
		}
		if (leave || osd_closing())
			break;
	}
	if (reboot_needed && !osd_closing())
		offer_reboot();
	osd_close();
}
