// input module for FFMPEG using libdvdread
// Author: Max Schwarz <Max@x-quadraht.de>

#include "io_dvdread.h"

#define LOG_PREFIX "[io_dvdread]"
#define DEBUG 0
#include "log.h"

struct DVDReadContext
{
	dvd_file_t* file;
	uint8_t* buffer;
	uint64_t pos;
	uint64_t size;
};

int io_dvdread_read_packet(void* opaque, uint8_t* buf, int size)
{
	DVDReadContext* d = (DVDReadContext*)opaque;

	log_debug("[io_dvdread] read_packet pos %lu", d->pos);

	uint64_t offset = d->pos;
	uint64_t count = 0;
	uint64_t dsize = size;

	if(d->pos >= d->size)
		dsize = 0;
	else if(d->pos + dsize > d->size)
		dsize = d->size - d->pos;

	while(dsize > 0)
	{
		int64_t ret = DVDReadBlocks(d->file, offset / DVD_VIDEO_LB_LEN, 1, d->buffer);
		if(ret < 0)
			return ret;
		if(ret == 0)
			break;

		uint64_t bytes = ret * DVD_VIDEO_LB_LEN;

		log_debug("read %10d from block %10lu for size %10d", ret, offset / DVD_VIDEO_LB_LEN, size);

		uint64_t cnt = (dsize < bytes) ? dsize : bytes;

		memcpy(buf, d->buffer, cnt);

		offset += cnt;
		d->pos += cnt;
		buf += cnt;
		count += cnt;
		dsize -= cnt;
	}

	return count;
}

int64_t io_dvdread_seek(void* opaque, int64_t offset, int whence)
{
	DVDReadContext* d = (DVDReadContext*)opaque;

	switch(whence)
	{
		case SEEK_SET:
			log_debug("seek set %"PRId64, offset);
			d->pos = offset;
			return offset;
			break;
		case SEEK_CUR:
			log_debug("seek cur => %"PRId64, d->pos);
			return d->pos;
		case SEEK_END:
			log_debug("seek end %"PRId64, offset);
			d->pos = offset = d->size + offset;
			return offset;
		case AVSEEK_SIZE:
			return d->size;
		default:
			log_debug("seek unsupported whence=%d", whence);
			return -1;
	}
}

AVIOContext* io_dvdread_create(dvd_file_t* file)
{
	DVDReadContext* d = new DVDReadContext;
	AVIOContext* ctx = 0;
	uint8_t* buffer = (uint8_t*)av_malloc(DVD_VIDEO_LB_LEN);

	if(!d || !buffer)
		goto error;

	d->file = file;
	d->pos = 0;
	d->buffer = (uint8_t*)av_malloc(DVD_VIDEO_LB_LEN);
	d->size = ((uint64_t)DVDFileSize(file)) * ((uint64_t)DVD_VIDEO_LB_LEN);

	log_debug("file size: %10"PRId64" MiB", d->size / 1024 / 1024);

	ctx = avio_alloc_context(
		buffer, DVD_VIDEO_LB_LEN,
		0, /* write_flag */
		(void*)d,
		&io_dvdread_read_packet, /* read_packet */
		NULL,
		&io_dvdread_seek /* seek */
	);

	ctx->seekable = AVIO_SEEKABLE_NORMAL;

	if(ctx)
		return ctx;

error:
	av_free(buffer);
	delete d;
	return NULL;
}
