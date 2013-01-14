// libdvdread interface
// Author: Max Schwarz <xqms@online.de>

#ifndef DVDREADER_H
#define DVDREADER_H

#include <string>
#include <vector>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>

class DVDReader
{
public:
	DVDReader();
	virtual ~DVDReader();

	bool open(const std::string& path);
	bool readTitleIFOs();
	void close();

	std::string volumeID();

	inline dvd_reader_t* reader()
	{ return m_reader; }

	inline size_t ifoCount() const
	{ return m_ifo.size(); }
	ifo_handle_t* ifo(int ttn);
private:
	dvd_reader_t* m_reader;
	std::vector<ifo_handle_t*> m_ifo;
};

#endif
