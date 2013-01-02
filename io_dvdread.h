// input module for FFMPEG using libdvdread
// Author: Max Schwarz <Max@x-quadraht.de>

#ifndef IO_DVDREAD_H
#define IO_DVDREAD_H

#include <stdint.h>

extern "C"
{
#include <libavformat/avio.h>
#include <dvdread/ifo_types.h>
}

/**
 * @brief Create io_dvdread IO context
 *
 * @param path Path template, needs to contain a %d for the file number
 *   (evaluated with snprintf)
 * @param split_size Maximum file size
 * */
AVIOContext* io_dvdread_create(dvd_file_t* file);

void io_dvdread_close(AVIOContext* ctx);

#endif // IO_SPLIT_H