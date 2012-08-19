/*
 * extract.c
 *
 * Support for extracting WIM files.
 */

/*
 * Copyright (C) 2010 Carl Thijssen
 * Copyright (C) 2012 Eric Biggers
 *
 * This file is part of wimlib, a library for working with WIM files.
 *
 * wimlib is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * wimlib is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with wimlib; if not, see http://www.gnu.org/licenses/.
 */

#include "wimlib_internal.h"
#include "dentry.h"
#include "lookup_table.h"
#include "xml.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#ifdef WITH_NTFS_3G
#include <ntfs-3g/volume.h>
#include <ntfs-3g/security.h>
#endif

/* Internal */
#define WIMLIB_EXTRACT_FLAG_MULTI_IMAGE 0x80000000

/* Sets and creates the directory to which files are to be extracted when
 * extracting files from the WIM. */
static int make_output_dir(const char *dir)
{
	char *p;
	DEBUG("Setting output directory to `%s'", dir);

	if (mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
		if (errno == EEXIST) {
			DEBUG("`%s' already exists", dir);
			return 0;
		}
		ERROR_WITH_ERRNO("Cannot create directory `%s'", dir);
		return WIMLIB_ERR_MKDIR;
	} else {
		DEBUG("Created directory `%s'", dir);
	}
	return 0;
}

static int extract_regular_file_linked(const struct dentry *dentry, 
				       const char *output_dir,
				       const char *output_path,
				       int extract_flags,
				       const struct lookup_table_entry *lte)
{
	/* This mode overrides the normal hard-link extraction and
	 * instead either symlinks or hardlinks *all* identical files in
	 * the WIM, even if they are in a different image (in the case
	 * of a multi-image extraction) */
	wimlib_assert(lte->file_on_disk);

	if (extract_flags & WIMLIB_EXTRACT_FLAG_HARDLINK) {
		if (link(lte->file_on_disk, output_path) != 0) {
			ERROR_WITH_ERRNO("Failed to hard link "
					 "`%s' to `%s'",
					 output_path, lte->file_on_disk);
			return WIMLIB_ERR_LINK;
		}
	} else {
		int num_path_components;
		int num_output_dir_path_components;
		size_t file_on_disk_len;
		char *p;
		const char *p2;
		size_t i;

		num_path_components = 
			get_num_path_components(dentry->full_path_utf8) - 1;
		num_output_dir_path_components =
			get_num_path_components(output_dir);

		if (extract_flags & WIMLIB_EXTRACT_FLAG_MULTI_IMAGE) {
			num_path_components++;
			num_output_dir_path_components--;
		}
		file_on_disk_len = strlen(lte->file_on_disk);

		char buf[file_on_disk_len + 3 * num_path_components + 1];
		p = &buf[0];

		for (i = 0; i < num_path_components; i++) {
			*p++ = '.';
			*p++ = '.';
			*p++ = '/';
		}
		p2 = lte->file_on_disk;
		while (*p2 == '/')
			p2++;
		while (num_output_dir_path_components--)
			p2 = path_next_part(p2, NULL);
		strcpy(p, p2);
		if (symlink(buf, output_path) != 0) {
			ERROR_WITH_ERRNO("Failed to symlink `%s' to "
					 "`%s'",
					 buf, lte->file_on_disk);
			return WIMLIB_ERR_LINK;
		}

	}
	return 0;
}

static int extract_regular_file_unlinked(WIMStruct *w,
					 const struct dentry *dentry, 
				         const char *output_path,
				         int extract_flags,
				         struct lookup_table_entry *lte)
{
	int out_fd;
	const struct resource_entry *res_entry;
	int ret;
	/* Otherwise, we must actually extract the file contents. */

	out_fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (out_fd == -1) {
		ERROR_WITH_ERRNO("Failed to open the file `%s' for writing",
				 output_path);
		return WIMLIB_ERR_OPEN;
	}

	/* Extract empty file, with no lookup table entry... */
	if (!lte) {
		DEBUG("Empty file `%s'.", output_path);
		ret = 0;
		goto done;
	}

	res_entry = &lte->resource_entry;

	ret = extract_resource_to_fd(w, res_entry, out_fd, 
				     res_entry->original_size);

	if (ret != 0) {
		ERROR("Failed to extract resource to `%s'", output_path);
		goto done;
	}

	/* Mark the lookup table entry to indicate this file has been extracted. */
	lte->out_refcnt++;
	FREE(lte->file_on_disk);
	lte->file_on_disk = STRDUP(output_path);
	if (!lte->file_on_disk)
		ret = WIMLIB_ERR_NOMEM;
done:
	close(out_fd);
	return ret;
}

/* 
 * Extracts a regular file from the WIM archive. 
 */
static int extract_regular_file(WIMStruct *w, 
				const struct dentry *dentry, 
				const char *output_dir,
				const char *output_path,
				int extract_flags)
{
	struct lookup_table_entry *lte;

	lte = __lookup_resource(w->lookup_table, dentry_hash(dentry));

	/* If we already extracted the same file or a hard link copy of it, we
	 * may be able to simply create a link.  The exact action is specified
	 * by the current @link_type. */
	if ((extract_flags & (WIMLIB_EXTRACT_FLAG_SYMLINK | WIMLIB_EXTRACT_FLAG_HARDLINK)) &&
	      lte && lte->out_refcnt != 0)
		return extract_regular_file_linked(dentry, output_dir,
						   output_path, extract_flags,
						   lte);
	else
		return extract_regular_file_unlinked(w, dentry, output_path,
						     extract_flags, lte);

}

static int extract_symlink(const struct dentry *dentry, const char *output_path,
			   const WIMStruct *w)
{
	char target[4096];
	ssize_t ret = dentry_readlink(dentry, target, sizeof(target), w);
	if (ret <= 0) {
		ERROR("Could not read the symbolic link from dentry `%s'",
		      dentry->full_path_utf8);
		return WIMLIB_ERR_INVALID_DENTRY;
	}
	ret = symlink(target, output_path);
	if (ret != 0) {
		ERROR_WITH_ERRNO("Failed to symlink `%s' to `%s'",
				 output_path, target);
		return WIMLIB_ERR_LINK;
	}
	return 0;
}

/* 
 * Extracts a directory from the WIM archive. 
 *
 * @dentry:		The directory entry for the directory.
 * @output_path:   	The path to which the directory is to be extracted to.
 * @return: 		True on success, false on failure. 
 */
static int extract_directory(struct dentry *dentry, const char *output_path)
{
	/* Compute the output path directory to the directory. */
	if (mkdir(output_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) 
	{
		switch (errno) {
		case EEXIST: /* Already existing directory is OK */
		case EACCES: /* We may have permissions to extract files inside
				 the directory, but not for the directory
				 itself. */
			return 0;
		default:
			ERROR_WITH_ERRNO("Cannot create directory `%s'",
					 output_path);
			return WIMLIB_ERR_MKDIR;
		}
	}
	return 0;
}

struct extract_args {
	WIMStruct *w;
	int extract_flags;
	const char *output_dir;
#ifdef WITH_NTFS_3G
	struct SECURITY_API *scapi;
#endif
};

/* 
 * Extracts a file or directory from the WIM archive.  For use in
 * for_dentry_in_tree().
 *
 * @dentry:	The dentry to extract.
 * @arg:	A pointer to the WIMStruct for the WIM file.
 */
static int extract_dentry(struct dentry *dentry, void *arg)
{
	struct extract_args *args = arg;
	WIMStruct *w = args->w;
	int extract_flags = args->extract_flags;
	size_t len = strlen(args->output_dir);
	char output_path[len + dentry->full_path_utf8_len + 1];
	int ret = 0;

	if (extract_flags & WIMLIB_EXTRACT_FLAG_VERBOSE)
		puts(dentry->full_path_utf8);

	memcpy(output_path, args->output_dir, len);
	memcpy(output_path + len, dentry->full_path_utf8, dentry->full_path_utf8_len);
	output_path[len + dentry->full_path_utf8_len] = '\0';

	if (dentry_is_symlink(dentry)) {
		ret = extract_symlink(dentry, output_path, w);
	} else if (dentry_is_directory(dentry)) {
		if (!dentry_is_root(dentry)) /* Root doesn't need to be extracted. */
			ret = extract_directory(dentry, output_path);
	} else {
		ret = extract_regular_file(w, dentry, args->output_dir,
					   output_path, extract_flags);
	}
	return ret;
}


static int extract_single_image(WIMStruct *w, int image,
				const char *output_dir, int extract_flags)
{
	DEBUG("Extracting image %d", image);

	int ret;
	ret = wimlib_select_image(w, image);
	if (ret != 0)
		return ret;

	struct extract_args args = {
		.w = w,
		.extract_flags = extract_flags,
		.output_dir = output_dir,
	#ifdef WITH_NTFS_3G
		.scapi = NULL
	#endif
	};

	return for_dentry_in_tree(wim_root_dentry(w), extract_dentry, &args);
}


/* Extracts all images from the WIM to @output_dir, with the images placed in
 * subdirectories named by their image names. */
static int extract_all_images(WIMStruct *w, const char *output_dir,
			      int extract_flags)
{
	size_t image_name_max_len = max(xml_get_max_image_name_len(w), 20);
	size_t output_path_len = strlen(output_dir);
	char buf[output_path_len + 1 + image_name_max_len + 1];
	int ret;
	int image;
	const char *image_name;

	DEBUG("Attempting to extract all images from `%s'", w->filename);

	memcpy(buf, output_dir, output_path_len);
	buf[output_path_len] = '/';
	for (image = 1; image <= w->hdr.image_count; image++) {
		
		image_name = wimlib_get_image_name(w, image);
		if (*image_name) {
			strcpy(buf + output_path_len + 1, image_name);
		} else {
			/* Image name is empty. Use image number instead */
			sprintf(buf + output_path_len + 1, "%d", image);
		}
		ret = make_output_dir(buf);
		if (ret != 0)
			goto done;
		ret = extract_single_image(w, image, buf, extract_flags);
		if (ret != 0)
			goto done;
	}
done:
	/* Restore original output directory */
	buf[output_path_len + 1] = '\0';
	return 0;
}

/* Extracts a single image or all images from a WIM file. */
WIMLIBAPI int wimlib_extract_image(WIMStruct *w, int image,
				   const char *output_dir, int flags)
{
	int ret;
	if (!output_dir)
		return WIMLIB_ERR_INVALID_PARAM;
	if ((flags & (WIMLIB_EXTRACT_FLAG_SYMLINK | WIMLIB_EXTRACT_FLAG_HARDLINK))
			== (WIMLIB_EXTRACT_FLAG_SYMLINK | WIMLIB_EXTRACT_FLAG_HARDLINK))
		return WIMLIB_ERR_INVALID_PARAM;

	if (image == WIM_ALL_IMAGES)
		flags |= WIMLIB_EXTRACT_FLAG_MULTI_IMAGE;
	else
		flags &= ~WIMLIB_EXTRACT_FLAG_MULTI_IMAGE;
	
	ret = make_output_dir(output_dir);
	if (ret != 0)
		return ret;

	if ((flags & WIMLIB_EXTRACT_FLAG_NTFS)) {
	#ifdef WITH_NTFS_3G
		unsigned long mnt_flags;
		ret = ntfs_check_if_mounted(output_dir, &mnt_flags);
		if (ret != 0) {
			ERROR_WITH_ERRNO("NTFS-3g: Cannot determine if `%s' "
					 "is mounted", output_dir);
			return WIMLIB_ERR_NTFS_3G;
		}
		if (!(mnt_flags & NTFS_MF_MOUNTED)) {
			ERROR("NTFS-3g: Filesystem on `%s' is not mounted ",
			      output_dir);
			return WIMLIB_ERR_NTFS_3G;
		}
		if (mnt_flags & NTFS_MF_READONLY) {
			ERROR("NTFS-3g: Filesystem on `%s' is mounted "
			      "read-only", output_dir);
			return WIMLIB_ERR_NTFS_3G;
		}
	#else
		ERROR("wimlib was compiled without support for NTFS-3g, so");
		ERROR("we cannot extract a WIM image while preserving NTFS-");
		ERROR("specific information");
		return WIMLIB_ERR_UNSUPPORTED;
	#endif
	}
	if (image == WIM_ALL_IMAGES)
		ret = extract_all_images(w, output_dir, flags);
	else
		ret = extract_single_image(w, image, output_dir, flags);
	return ret;

}
