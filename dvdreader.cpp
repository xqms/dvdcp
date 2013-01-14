// libdvdread interface
// Author: Max Schwarz <xqms@online.de>

#include "dvdreader.h"

#define LOG_PREFIX "[DVDReader]"
#include "log.h"

#include <assert.h>

#include <dvdread/dvd_udf.h>

DVDReader::DVDReader()
{
}

DVDReader::~DVDReader()
{
	close();
}

bool DVDReader::open(const std::string& path)
{
	m_reader = DVDOpen(path.c_str());
	if(!m_reader)
	{
		error("Could not open dvd");
		return false;
	}

	ifo_handle_t* ifoZero = ifoOpen(m_reader, 0);
	if(!ifoZero)
	{
		error("Could not open main IFO");
		return false;
	}

	m_ifo.resize(1, ifoZero);

	return true;
}

bool DVDReader::readTitleIFOs()
{
	assert(m_ifo.size() == 1 && m_ifo[0]);

	m_ifo.resize(m_ifo[0]->vts_atrt->nr_of_vtss+1, 0);
	for(size_t i = 1; i < m_ifo.size(); ++i)
	{
		m_ifo[i] = ifoOpen(m_reader, i);
		if(!m_ifo[i])
		{
			error("Could not open IFO for title %zu", i);
			return false;
		}
	}

	return true;
}

void DVDReader::close()
{
	for(size_t i = 0; i < m_ifo.size(); ++i)
	{
		if(!m_ifo[i])
			continue;

		ifoClose(m_ifo[i]);
	}
	m_ifo.clear();

	if(m_reader)
	{
		DVDClose(m_reader);
		m_reader = 0;
	}
}

ifo_handle_t* DVDReader::ifo(int ttn)
{
	if(ttn < 0 || ttn >= (int)m_ifo.size())
		return 0;

	return m_ifo[ttn];
}

std::string DVDReader::volumeID()
{
	char volid[64] = {0};
	UDFGetVolumeIdentifier(m_reader, volid, sizeof(volid));

	return std::string(volid);
}

