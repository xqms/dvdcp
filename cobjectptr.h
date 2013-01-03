// Pointer to c object with free function / functor
// Author: Max Schwarz <xqms@online.de>

#ifndef COBJECTPTR_H
#define COBJECTPTR_H

extern "C"
{
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/avfiltergraph.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
}

template<class T, void (*Release)(T*)>
class CObjectPtr
{
public:
	CObjectPtr()
	 : m_ptr(0)
	{
	}

	CObjectPtr(T* ptr)
	 : m_ptr(ptr)
	{
	}

	~CObjectPtr()
	{
		if(m_ptr)
			Release(m_ptr);
	}

	T* operator->()
	{
		return m_ptr;
	}

	const T* operator->() const
	{
		return m_ptr;
	}

	void free()
	{
		if(!m_ptr)
			return;
		Release(m_ptr);
		m_ptr = 0;
	}

	void operator=(T* ptr)
	{
		free();
		m_ptr = ptr;
	}

	T* release()
	{
		T* ptr = m_ptr;
		m_ptr = 0;
		return m_ptr;
	}

	operator bool() const
	{ return m_ptr; }

	T* operator()()
	{ return m_ptr; }

	operator T*()
	{ return m_ptr; }
private:
	T* m_ptr;

	CObjectPtr(const CObjectPtr& other);
};

inline void really_free_avfilter_graph(AVFilterGraph* graph)
{
	avfilter_graph_free(&graph);
}

typedef CObjectPtr<AVFormatContext, avformat_free_context> AVFormatContextPtr;
typedef CObjectPtr<AVFilterGraph, really_free_avfilter_graph> AVFilterGraphPtr;
typedef CObjectPtr<dvd_reader_t, DVDClose> DVDReaderPtr;
typedef CObjectPtr<ifo_handle_t, ifoClose> IFOHandlePtr;
typedef CObjectPtr<dvd_file_t, DVDCloseFile> DVDFilePtr;

#endif
