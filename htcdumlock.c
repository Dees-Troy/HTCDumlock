/*
 * This binary is a workaround for HTC's unlock method that doesn't let
 * you flash boot while booted to recovery.  It is designed to dump
 * recovery and boot to the sdcard then flash recovery to boot. When
 * used with a supported recovery, you can reflash the dumped copy of
 * boot once you enter the recovery.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * The code was written from scratch by Dees_Troy dees_troy at
 * yahoo
 *
 * Copyright (c) 2012
 *
 * Note that this all could probably be done as a shell script, but
 * I am much better at C than I am at scripting. :)
 */
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int verbose = 0, java = 0;

void reboot_device() {
	// Reboot
	printf("Rebooting!\n");
	system("reboot system");
}

void flash_recovery_to_boot(int no_flash, int no_reboot) {
	char device_path[255], recovery_path[255], boot_path[255],
		exec[255], md5recovery[255], md5boot[255],
		recoveryimg[255], bootimg[255], tempimg[255];
	int ret_val = 0;
	FILE *fp;
	char* token;

	// Create folders
	if (verbose)
		printf("Making folder '/sdcard/htcdumlock'\n");
	mkdir("/sdcard/htcdumlock", 0777);
	strcpy(device_path, "/sdcard/htcdumlock/");
	// Make folder for recovery
	strcpy(recovery_path, device_path);
	strcat(recovery_path, "recovery/");
	if (verbose)
		printf("Making folder '%s'\n", recovery_path);
	mkdir(recovery_path, 0777);
	// Make folder for boot
	strcpy(boot_path, device_path);
	strcat(boot_path, "boot/");
	if (verbose)
		printf("Making folder '%s'\n", boot_path);
	mkdir(boot_path, 0777);

	// Set up file locations
	strcpy(recoveryimg, recovery_path);
	strcat(recoveryimg, "recovery.img");
	strcpy(bootimg, boot_path);
	strcat(bootimg, "boot.img");
	strcpy(tempimg, device_path);
	strcat(tempimg, "temp.img");

	// Dump recovery
	strcpy(exec, "dump_image recovery ");
	strcat(exec, recoveryimg);
	if (verbose)
		printf("Running command: '%s'\n", exec);
	ret_val = system(exec);
	if (ret_val != 0) {
		printf("Unable to dump recovery.\nFailed\n");
		return;
	}

	// Dump boot (kernel)
	strcpy(exec, "dump_image boot ");
	strcat(exec, tempimg);
	if (verbose)
		printf("Running command: '%s'\n", exec);
	ret_val = system(exec);
	if (ret_val != 0) {
		printf("Unable to dump recovery.\nFailed\n");
		return;
	}

	// Get md5sum of recovery
	strcpy(exec, "md5sum ");
	strcat(exec, recoveryimg);
	fp = popen(exec, "r");
	if (fgets(md5recovery, sizeof(md5recovery), fp) == NULL)
    {
		printf("Unable to get md5sum of recovery.img\nFailed\n");
		return;
	}
	if (verbose)
		printf("md5sum of recovery.img: '%s'\n", md5recovery);
	fclose(fp);

	// Get md5sum of boot stored in temp location
	strcpy(exec, "md5sum ");
	strcat(exec, tempimg);
	fp = popen(exec, "r");
	if (fgets(md5boot, sizeof(md5boot), fp) == NULL)
    {
		printf("Unable to get md5sum of boot.img\nFailed\n");
		return;
	}
	if (verbose)
		printf("md5sum of boot.img: '%s'\n", md5boot);
	fclose(fp);

	// Compare md5sums
	if (strncmp(md5recovery, md5boot, 32) != 0) {
		if (verbose) {
			printf("md5sums of boot and recovery do not match so recovery is not flashed to boot yet...\n");
			printf("Note that on some devices, boot and recovery partitions are not the same size so md5sums never match!\n");
		}
		strcpy(exec, "mv ");
		strcat(exec, tempimg);
		strcat(exec, " ");
		strcat(exec, bootimg);
		if (verbose)
			printf("Moving temporary boot.img: '%s'\n", exec);
		ret_val = system(exec);
		if (ret_val != 0) {
			printf("Unable to move temporary boot image.\nFailed\n");
			return;
		}
	} else {
		if (!java)
			printf("md5sum of recovery and boot matches! Recovery is already flashed to boot!\n");
		if (!no_reboot)
			reboot_device();
		return;
	}

	// Move boot.img into /data
	strcpy(exec, "cp ");
	strcat(exec, bootimg);
	strcat(exec, " ");
	strcat(exec, "/data/boot.img");
	if (verbose)
		printf("Copying boot.img to /data: '%s'\n", exec);
	ret_val = system(exec);
	if (ret_val != 0) {
		printf("Unable to copy boot to /data: '%s'\nFailed\n");
		return;
	}

	// Flash recovery to boot
	strcpy(exec, "flash_image boot ");
	strcat(exec, recoveryimg);
	if (no_flash) {
		if (verbose)
			printf("NOT flashing recovery to boot due to argument 'noflash', command is '%s'\n", exec);
	} else {
		if (verbose)
			printf("Running command: '%s'\n", exec);
		ret_val = system(exec);
		if (ret_val != 0) {
			printf("Unable to flash recovery to boot.\nFailed\n");
			return;
		}
	}

	if (!no_reboot && !ret_val)
		reboot_device();
}

void restore_original_boot(int no_flash) {
	char boot_path[255], exec[255];
	int ret_val;

	// Restore original boot partition
	strcpy(exec, "flash_image boot /sdcard/htcdumlock/boot/boot.img");
	if (no_flash) {
		if (verbose)
			printf("NOT restoring boot due to argument 'noflash', command is '%s'\n", exec);
	} else {
		if (verbose)
			printf("Running command: '%s'\n", exec);
		ret_val = system(exec);
		if (ret_val != 0) {
			printf("Unable to restore boot.\nFailed\n");
		}
	}
}

int main(int argc, char** argv)
{
	int recovery = 0, no_flash = 0, restore_boot = 0, arg_error = 0,
		no_reboot = 0, i;

	// Parse the arguments
	if (argc < 2)
		arg_error = 1;
	else {
		for (i=1; i<argc; i++) {
			if (strcmp(argv[i], "recovery") == 0)
				recovery = 1;
			else if (strcmp(argv[i], "restore") == 0) {
				// Check to see if recovery option was set
				// Do not allow user to do recovery and restore at the same time
				if (recovery)
					arg_error = 1;
				restore_boot = 1;
			} else if (strcmp(argv[i], "noflash") == 0)
				no_flash = 1;
			else if (strcmp(argv[i], "noreboot") == 0)
				no_reboot = 1;
			else if (strcmp(argv[i], "verbose") == 0)
				verbose = 1;
			else if (strcmp(argv[i], "java") == 0)
				java = 1;
			else
				arg_error = 1;
		}
	}
	if (arg_error) {
		printf("Invalid argument given.\n");
		printf("Valid arguments are:\n");
		printf("  recovery -- backs up boot and recovery and flashes recovery to boot\n");
		printf("  restore  -- restores the most recent backup of boot made by this utility\n");
		printf("  noflash  -- same as 'recovery' but does not flash boot or reboot at the end\n");
		printf("  noreboot -- does not reboot after flashing boot during 'recovery'\n");
		printf("  verbose  -- show extra debug information\n");
		printf("\nNOTE: You cannot do 'recovery' and 'restore' in the same operation.\nFailed\n");
		return 0;
	}

	if (recovery) {
		if (!java)
			printf("Flashing recovery to boot, this may take a few minutes . . .\n");
		flash_recovery_to_boot(no_flash, no_reboot);
	}
	if (restore_boot) {
		printf("Restoring boot, this may take a few minutes . . .\n");
		restore_original_boot(no_flash);
	}

	return 0;
}
