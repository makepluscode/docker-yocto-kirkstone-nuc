#include <errno.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <linux/hdreg.h>

#include "mbr.h"
#include "update_handler.h"
#include "utils.h"

/* partition entry in MBR partition table, the system boots from */
#define BOOT_PARTITION_ENTRY		0
#define MBR_NUMBER_OF_PARTITIONS	4
#define MBR_MAGIC_NUMBER_L		0x55
#define MBR_MAGIC_NUMBER_H		0xAA

#pragma pack(push,1)
struct mbr_chs_entry {
	guint8 head;
	guint8 sector;
	guint8 cylinder;
};
G_STATIC_ASSERT(sizeof(struct mbr_chs_entry) == 3);

struct mbr_tbl_entry {
	guint8 boot_indicator;
	struct mbr_chs_entry chs_start;
	guint8 type;
	struct mbr_chs_entry chs_end;
	guint32 partition_start_le;
	guint32 partition_size_le;
};
G_STATIC_ASSERT(sizeof(struct mbr_tbl_entry) == 16);

struct mbr {
	guint8 bootstrap_code[440];
	guint32 disk_signature_le;
	guint8 unused[2];
	struct mbr_tbl_entry partition_table[MBR_NUMBER_OF_PARTITIONS];
	guint8 magic_number[2];
};
G_STATIC_ASSERT(sizeof(struct mbr) == 512);
#pragma pack(pop)

static gboolean get_number_of_sectors(gint fd, guint *sectors,
		GError **error)
{
	g_return_val_if_fail(sectors, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	if (ioctl(fd, BLKGETSIZE, sectors) != 0) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"ioctl command 0x%04x failed: %s",
				BLKGETSIZE, g_strerror(errno));
		return FALSE;
	}

	return TRUE;
}

static void get_hd_geometry(gint fd, guint8 *heads, guint8 *sectors)
{
	struct hd_geometry geometry;

	g_return_if_fail(heads);
	g_return_if_fail(sectors);

	if (ioctl(fd, HDIO_GETGEO, &geometry) == 0) {
		*heads = geometry.heads;
		*sectors = geometry.sectors;
	} else {
		g_message("Failed to get disk geometry, using LBA addressing: %s",
				g_strerror(errno));
		*heads = 255;
		*sectors = 63;
	}
}

static gboolean validate_region(gint fd, guint64 start, guint64 size,
		guint sector_size, GError **error)
{
	gboolean res = FALSE;
	guint number_of_sectors;
	GError *ierror = NULL;

	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	if (start < sizeof(struct mbr) || size == 0) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"no valid configuration for region");
		goto out;
	}

	if ((start % sector_size) != 0) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"Region start %"G_GINT64_MODIFIER "d is not aligned to the sector-size %d",
				start, sector_size);
		goto out;
	}

	if ((size % (2 * sector_size)) != 0) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"Region size %"G_GINT64_MODIFIER "d is not aligned to the double sector-size %d",
				size, 2 * sector_size);
		goto out;
	}

	res = get_number_of_sectors(fd, &number_of_sectors, &ierror);
	if (!res) {
		g_propagate_error(error, ierror);
		goto out;
	}

	if ((start + size) >= (guint64)number_of_sectors * sector_size) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"Region configuration is bigger than device");
		res = FALSE;
		goto out;
	}

out:
	return res;
}

static gboolean read_mbr(gint fd, struct mbr *mbr, GError **error)
{
	g_return_val_if_fail(mbr, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	if (read(fd, mbr, sizeof(*mbr)) != sizeof(*mbr)) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"Read: %s", g_strerror(errno));
		return FALSE;
	}

	if (mbr->magic_number[0] != MBR_MAGIC_NUMBER_L ||
	    mbr->magic_number[1] != MBR_MAGIC_NUMBER_H) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"No valid master boot record found");
		return FALSE;
	}

	return TRUE;
}

static gboolean is_region_free(guint64 region_start, guint64 region_size,
		const struct mbr_tbl_entry *partition_tbl, guint sector_size,
		GError **error)
{
	guint64 p_start, p_end;
	guint i;

	g_return_val_if_fail(partition_tbl, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	for (i = 0; i < MBR_NUMBER_OF_PARTITIONS; i++) {
		if (i == BOOT_PARTITION_ENTRY)
			continue;

		/* skip empty partitions */
		if (partition_tbl[i].partition_size_le == 0)
			continue;

		p_start = (guint64)GUINT32_FROM_LE(partition_tbl[i].partition_start_le) * sector_size;
		p_end = (guint64)GUINT32_FROM_LE(partition_tbl[i].partition_size_le) * sector_size +
		        p_start - 1;

		if (region_start >= p_start && region_start <= p_end) {
			g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
					"Region start address 0x%"G_GINT64_MODIFIER "x is in area of "
					"partition %d (0x%"G_GINT64_MODIFIER "x - 0x%"G_GINT64_MODIFIER "x)",
					region_start, i+1, p_start, p_end);
			break;
		}

		if (p_start >= region_start &&
		    p_start <= region_start + region_size - 1) {
			g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
					"Region end address 0x%"G_GINT64_MODIFIER "x is in area of "
					"partition %d (0x%"G_GINT64_MODIFIER "x - 0x%"G_GINT64_MODIFIER "x)",
					region_start + region_size - 1, i+1, p_start,
					p_end);
			break;
		}
	}

	if (i < MBR_NUMBER_OF_PARTITIONS)
		return FALSE;

	return TRUE;
}

/**
 * Calculation of the CHS value from an LBA value
 *
 * The 3 CHS bytes in Partition are stored with following layout:
 *   - 8 bits for HEAD
 *   - upper 2 bits for CYLINDER
 *   - 6 bits for SECTOR
 *   - lower 8 bits for CYLINDER
 */
static void get_chs(struct mbr_chs_entry *chs, guint32 lba,
		guint8 heads, guint8 sectors)
{
	g_return_if_fail(chs);
	g_return_if_fail(heads);
	g_return_if_fail(sectors);

	chs->sector = lba % sectors + 1;

	lba /= sectors;
	chs->head = lba % heads;

	lba /= heads;
	chs->cylinder = lba & 0xFF;

	/* Move bit 8 & 9 of cylinder to bit 6 & 7 of sector */
	chs->sector |= (lba >> 2) & 0xC0;
}

static gboolean get_raw_partition_entry(gint fd,
		struct mbr_tbl_entry *raw_entry,
		const struct boot_switch_partition *partition, GError **error)
{
	gboolean res = FALSE;
	guint32 start, size;
	guint sector_size;
	guint8 heads, sectors;

	g_return_val_if_fail(raw_entry, FALSE);
	g_return_val_if_fail(partition, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	sector_size = get_sectorsize(fd);

	if (partition->start % sector_size || partition->size % sector_size) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"Partition start address or size is not a multiple"
				" of sector size %d", sector_size);
		goto out;
	}

	start = partition->start / sector_size;
	size = partition->size / sector_size;

	raw_entry->partition_start_le = GUINT32_TO_LE(start);
	raw_entry->partition_size_le = GUINT32_TO_LE(size);

	get_hd_geometry(fd, &heads, &sectors);

	get_chs(&raw_entry->chs_start, start, heads, sectors);

	get_chs(&raw_entry->chs_end, start + size - 1, heads, sectors);

	res = TRUE;
out:
	return res;
}

gboolean r_mbr_switch_get_inactive_partition(const gchar *device,
		struct boot_switch_partition *partition,
		guint64 region_start, guint64 region_size,
		GError **error)
{
	gboolean res = FALSE;
	struct mbr mbr;
	GError *ierror = NULL;
	struct mbr_tbl_entry *boot_part;
	guint sector_size;
	gint fd;

	g_return_val_if_fail(device, FALSE);
	g_return_val_if_fail(partition, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	fd = g_open(device, O_RDONLY);

	sector_size = get_sectorsize(fd);

	res = validate_region(fd, region_start, region_size, sector_size, &ierror);
	if (!res) {
		g_propagate_error(error, ierror);
		goto out;
	}

	res = read_mbr(fd, &mbr, &ierror);
	if (!res) {
		g_propagate_prefixed_error(error, ierror,
				"Failed to read MBR:");
		goto out;
	}

	/* check if region overlaps with any partition */
	res = is_region_free(region_start, region_size, mbr.partition_table,
			sector_size, &ierror);
	if (!res) {
		g_propagate_error(error, ierror);
		goto out;
	}
	res = FALSE;

	boot_part = &mbr.partition_table[BOOT_PARTITION_ENTRY];

	if (GUINT32_FROM_LE(boot_part->partition_start_le) == 0) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"No boot partition found in entry %d",
				BOOT_PARTITION_ENTRY);
		goto out;
	}

	if ((region_start / sector_size) ==
	    (guint64)GUINT32_FROM_LE(boot_part->partition_start_le)) {
		partition->start = region_start + region_size / 2;
	} else if (((region_start + region_size / 2) / sector_size) ==
	           (guint64)GUINT32_FROM_LE(boot_part->partition_start_le)) {
		partition->start = region_start;
	} else {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"Boot partition's start address does not match "
				"region configuration");
		goto out;
	}
	partition->size = region_size / 2;

	res = TRUE;
out:
	if (fd >= 0)
		g_close(fd, NULL);

	return res;
}

gboolean r_mbr_switch_set_boot_partition(const gchar *device,
		const struct boot_switch_partition *partition,
		GError **error)
{
	gboolean res = FALSE;
	struct mbr mbr;
	struct mbr_tbl_entry *boot_part;
	GError *ierror = NULL;
	gint fd;

	g_return_val_if_fail(device, FALSE);
	g_return_val_if_fail(partition, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	fd = g_open(device, O_RDWR);
	if (fd == -1) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"Opening device failed: %s",
				g_strerror(errno));
		goto out;
	}

	res = read_mbr(fd, &mbr, &ierror);
	if (!res) {
		g_propagate_prefixed_error(error, ierror,
				"Failed to read MBR:");
		goto out;
	}

	boot_part = &mbr.partition_table[BOOT_PARTITION_ENTRY];

	res = get_raw_partition_entry(fd, boot_part, partition, &ierror);
	if (!res) {
		g_propagate_prefixed_error(error, ierror,
				"Failed to create new partition entry:");
		goto out;
	}

	if (lseek(fd, 0, SEEK_SET) != 0) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"Failed to seek to position 0");
		res = FALSE;
		goto out;
	}

	if (write(fd, &mbr, sizeof(mbr)) != sizeof(mbr)) {
		g_set_error(error, R_UPDATE_ERROR, R_UPDATE_ERROR_FAILED,
				"Could not write new MBR: %s",
				g_strerror(errno));
		res = FALSE;
		goto out;
	}

	res = TRUE;
out:
	if (fd >= 0)
		g_close(fd, NULL);

	return res;
}
