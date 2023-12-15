##############################################################################
#				General					     #
##############################################################################


ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS	= -I$(top_srcdir)/include $(PLATFORM_CPPFLAGS)	\
		  -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE

# Note: -std=gnu99 instead of -std=c99 is needed for unnamed structs and unions,
# which are in C11 but not C99.  But we can't yet actually use -std=c11 because
# we want to support older versions of gcc.
AM_CFLAGS	= -std=gnu99 $(PLATFORM_CFLAGS) -fno-common	\
		  -Wmissing-prototypes -Wstrict-prototypes	\
		  -Wundef -Wno-pointer-sign

AM_LDFLAGS	= $(PLATFORM_LDFLAGS)

EXTRA_DIST	= README.md README.WINDOWS.md NEWS.md		\
		  COPYING COPYING.GPLv3	COPYING.LGPLv3

##############################################################################
#				Library					     #
##############################################################################

lib_LTLIBRARIES = libwim.la
include_HEADERS = include/wimlib.h

pkgconfigdir	= @pkgconfigdir@
pkgconfig_DATA	= wimlib.pc
wimlib.pc: config.status

libwim_la_SOURCES =		\
	src/add_image.c		\
	src/avl_tree.c		\
	src/blob_table.c	\
	src/compress.c		\
	src/compress_common.c	\
	src/compress_parallel.c	\
	src/compress_serial.c	\
	src/cpu_features.c	\
	src/decompress.c	\
	src/decompress_common.c	\
	src/delete_image.c	\
	src/dentry.c		\
	src/divsufsort.c	\
	src/encoding.c		\
	src/error.c		\
	src/export_image.c	\
	src/extract.c		\
	src/file_io.c		\
	src/header.c		\
	src/inode.c		\
	src/inode_fixup.c	\
	src/inode_table.c	\
	src/integrity.c		\
	src/iterate_dir.c	\
	src/join.c		\
	src/lcpit_matchfinder.c	\
	src/lzms_common.c	\
	src/lzms_compress.c	\
	src/lzms_decompress.c	\
	src/lzx_common.c	\
	src/lzx_compress.c	\
	src/lzx_decompress.c	\
	src/metadata_resource.c	\
	src/mount_image.c	\
	src/pathlist.c		\
	src/paths.c		\
	src/pattern.c		\
	src/progress.c		\
	src/reference.c		\
	src/registry.c		\
	src/reparse.c		\
	src/resource.c		\
	src/scan.c		\
	src/security.c		\
	src/sha1.c		\
	src/solid.c		\
	src/split.c		\
	src/tagged_items.c	\
	src/template.c		\
	src/textfile.c		\
	src/threads.c		\
	src/timestamp.c		\
	src/update_image.c	\
	src/util.c		\
	src/verify.c		\
	src/wim.c		\
	src/write.c		\
	src/xml.c		\
	src/xml_windows.c	\
	src/xmlproc.c		\
	src/xpress_compress.c	\
	src/xpress_decompress.c \
	include/wimlib/alloca.h		\
	include/wimlib/apply.h		\
	include/wimlib/assert.h		\
	include/wimlib/avl_tree.h	\
	include/wimlib/bitops.h		\
	include/wimlib/blob_table.h	\
	include/wimlib/bt_matchfinder.h	\
	include/wimlib/case.h		\
	include/wimlib/compiler.h	\
	include/wimlib/compressor_ops.h	\
	include/wimlib/compress_common.h	\
	include/wimlib/chunk_compressor.h	\
	include/wimlib/cpu_features.h	\
	include/wimlib/decompressor_ops.h	\
	include/wimlib/decompress_common.h	\
	include/wimlib/dentry.h		\
	include/wimlib/divsufsort.h	\
	include/wimlib/encoding.h	\
	include/wimlib/endianness.h	\
	include/wimlib/error.h		\
	include/wimlib/file_io.h	\
	include/wimlib/glob.h		\
	include/wimlib/guid.h		\
	include/wimlib/hc_matchfinder.h	\
	include/wimlib/header.h		\
	include/wimlib/inode.h		\
	include/wimlib/inode_table.h	\
	include/wimlib/integrity.h	\
	include/wimlib/lcpit_matchfinder.h	\
	include/wimlib/list.h		\
	include/wimlib/lzms_common.h	\
	include/wimlib/lzms_constants.h	\
	include/wimlib/lzx_common.h	\
	include/wimlib/lzx_constants.h	\
	include/wimlib/matchfinder_common.h	\
	include/wimlib/metadata.h	\
	include/wimlib/object_id.h	\
	include/wimlib/pathlist.h	\
	include/wimlib/paths.h		\
	include/wimlib/pattern.h	\
	include/wimlib/progress.h	\
	include/wimlib/registry.h	\
	include/wimlib/reparse.h	\
	include/wimlib/resource.h	\
	include/wimlib/scan.h		\
	include/wimlib/security.h	\
	include/wimlib/security_descriptor.h	\
	include/wimlib/sha1.h		\
	include/wimlib/solid.h		\
	include/wimlib/tagged_items.h	\
	include/wimlib/textfile.h	\
	include/wimlib/threads.h	\
	include/wimlib/timestamp.h	\
	include/wimlib/types.h		\
	include/wimlib/unaligned.h	\
	include/wimlib/unix_data.h	\
	include/wimlib/util.h		\
	include/wimlib/wim.h		\
	include/wimlib/write.h		\
	include/wimlib/xattr.h		\
	include/wimlib/xml.h		\
	include/wimlib/xml_windows.h	\
	include/wimlib/xmlproc.h	\
	include/wimlib/xpress_constants.h

if !WINDOWS_NATIVE_BUILD
libwim_la_SOURCES += src/unix_apply.c		\
		     src/unix_capture.c
PLATFORM_LIBS =
endif

if ENABLE_TEST_SUPPORT
libwim_la_SOURCES += src/test_support.c		\
		     include/wimlib/test_support.h
if !WINDOWS_NATIVE_BUILD
PLATFORM_LIBS += -lm
endif
endif

libwim_la_CFLAGS =		\
	-DBUILDING_WIMLIB	\
	$(AM_CFLAGS)		\
	$(PTHREAD_CFLAGS)

libwim_la_LDFLAGS = $(AM_LDFLAGS) -version-info 40:0:25

libwim_la_LIBADD =		\
	$(PTHREAD_LIBS)		\
	$(LIBRT_LIBS)		\
	$(PLATFORM_LIBS)
