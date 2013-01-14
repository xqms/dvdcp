// input module for FFMPEG using libdvdread
// Author: Max Schwarz <Max@x-quadraht.de>

#include "io_dvdread.h"

#define LOG_PREFIX "[io_dvdread]"
#define DEBUG 0
#include "log.h"

extern "C" {
#include <dvdread/nav_types.h>
#include <dvdread/nav_read.h>
}

#include <vector>

struct CellDesc
{
	int idx;
	int64_t offset;
	int64_t size;
	cell_playback_t* pb;
};

struct DVDReadContext
{
	dvd_file_t* file;
	pgc_t* pgc;
	int start_cell;
	std::vector<CellDesc> cells;
	int cur_cell_idx;

	uint8_t* buffer;
	uint64_t pos;
	uint64_t size;
};

const CellDesc* get_cell(DVDReadContext* d, int64_t offset)
{
	if(d->cur_cell_idx != -1)
	{
		const CellDesc& desc = d->cells[d->cur_cell_idx];
		if(offset >= desc.offset && offset < desc.offset + desc.size)
			return &desc;
	}

	for(size_t i = 0; i < d->cells.size(); ++i)
	{
		const CellDesc& desc = d->cells[i];
		if(offset >= desc.offset && offset < desc.offset + desc.size)
			return &desc;
	}

	return NULL;
}

int64_t get_title_size(DVDReadContext* d)
{
	int cell = d->start_cell;
	int64_t size = 0;

	for(cell = d->start_cell; cell < d->pgc->nr_of_cells; ++cell)
	{
		CellDesc desc;
		desc.idx = cell;
		desc.offset = size;
		desc.pb = &d->pgc->cell_playback[cell];

		int64_t sectors = desc.pb->last_sector - desc.pb->first_sector + 1;

		desc.size = DVD_VIDEO_LB_LEN * sectors;
		d->cells.push_back(desc);

		size += desc.size;
	}

	return size;
}

int io_dvdread_read_packet(void* opaque, uint8_t* buf, int size)
{
	DVDReadContext* d = (DVDReadContext*)opaque;

	uint64_t offset = d->pos;
	uint64_t count = 0;
	uint64_t dsize = size;

	if(d->pos >= d->size)
		dsize = 0;
	else if(d->pos + dsize > d->size)
		dsize = d->size - d->pos;

	while(dsize > 0)
	{
		const CellDesc* cell = get_cell(d, offset);
		if(!cell)
		{
			log_warning("Could not find cell for offset %20" PRId64, offset);
			return -1;
		}

		int64_t real_sector = (offset - cell->offset) / DVD_VIDEO_LB_LEN + cell->pb->first_sector;
		int64_t ret = DVDReadBlocks(d->file, real_sector, 1, d->buffer);
		if(ret < 0)
			return ret;
		if(ret == 0)
			break;

		uint64_t bytes = ret * DVD_VIDEO_LB_LEN;

		log_debug("read %10" PRId64 " from block %10lu for size %10d", ret, offset / DVD_VIDEO_LB_LEN, size);

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
			log_debug("seek set %" PRId64, offset);
			d->pos = offset;
			return offset;
			break;
		case SEEK_CUR:
			log_debug("seek cur => %" PRId64, d->pos);
			return d->pos;
		case SEEK_END:
			log_debug("seek end %" PRId64, offset);
			d->pos = offset = d->size + offset;
			return offset;
		case AVSEEK_SIZE:
			return d->size;
		default:
			log_debug("seek unsupported whence=%d", whence);
			return -1;
	}
}

AVIOContext* io_dvdread_create(dvd_file_t* file, int ttn, ifo_handle_t* vts_file)
{
	DVDReadContext* d = new DVDReadContext;
	AVIOContext* ctx = 0;

	log_debug("Got DVD title %d", ttn);
	vts_ptt_srpt_t* vts_ptt_srpt = vts_file->vts_ptt_srpt;
	int pgc_id = vts_ptt_srpt->title[ ttn - 1 ].ptt[0].pgcn;
	int pgn = vts_ptt_srpt->title[ ttn - 1 ].ptt[0].pgn;
	d->pgc = vts_file->vts_pgcit->pgci_srp[ pgc_id - 1 ].pgc;
	d->start_cell = d->pgc->program_map[ pgn - 1 ] - 1;

	uint8_t* buffer = (uint8_t*)av_malloc(DVD_VIDEO_LB_LEN);
	if(!d || !buffer)
		goto error;

	d->file = file;
	d->pos = 0;
	d->buffer = (uint8_t*)av_malloc(DVD_VIDEO_LB_LEN);
	d->size = get_title_size(d);
	d->cur_cell_idx = -1;

	log_debug("file size: %10" PRId64 " MiB", d->size / 1024 / 1024);

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
