// ima_adpcm_wav_decode.cpp: implementation of the Cima_adpcm_wav_decode class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "aud_decode.h"
#include "ima_adpcm_wav_decode.h"
#include "riff_structures.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Cima_adpcm_wav_decode::Cima_adpcm_wav_decode()
{
	m_data = NULL;
}

Cima_adpcm_wav_decode::~Cima_adpcm_wav_decode()
{
	delete[] m_data;
}

void Cima_adpcm_wav_decode::load(byte* r, int cb_s, int c_channels, int chunk_size)
{
	int c_chunks = (cb_s + chunk_size - 1) / chunk_size ;
	mc_samples = (cb_s - sizeof(t_ima_adpcm_chunk_header) * c_channels * c_chunks << 1) + c_chunks * c_channels;
	delete[] m_data;
	m_data = new short[mc_samples];
	short* w = m_data;
	int cs_remaining = mc_samples;
	while (cs_remaining)
	{
		if (c_channels == 1)
		{
			const t_ima_adpcm_chunk_header& chunk_header = *reinterpret_cast<const t_ima_adpcm_chunk_header*>(r);
			r += sizeof(t_ima_adpcm_chunk_header);
			*w++ = chunk_header.sample;
			cs_remaining--;
			int cs_chunk = min(cs_remaining, chunk_size - sizeof(t_ima_adpcm_chunk_header) << 1);
			aud_decode d;
			d.init(chunk_header.index, chunk_header.sample);
			d.decode_chunk(r, w, cs_chunk);
			r += cs_chunk >> 1;
			w += cs_chunk;
			cs_remaining -= cs_chunk;
		}
		else
		{
			assert(c_channels == 2);
			const t_ima_adpcm_chunk_header& left_chunk_header = *reinterpret_cast<const t_ima_adpcm_chunk_header*>(r);
			r += sizeof(t_ima_adpcm_chunk_header);
			*w++ = left_chunk_header.sample;
			cs_remaining--;
			const t_ima_adpcm_chunk_header& right_chunk_header = *reinterpret_cast<const t_ima_adpcm_chunk_header*>(r);
			r += sizeof(t_ima_adpcm_chunk_header);
			*w++ = right_chunk_header.sample;
			cs_remaining--;
			int cs_chunk = min(cs_remaining, chunk_size - (sizeof(t_ima_adpcm_chunk_header) << 1) << 1);
			aud_decode left_d, right_d;
			left_d.init(left_chunk_header.index, left_chunk_header.sample);
			right_d.init(right_chunk_header.index, right_chunk_header.sample);
			assert(!(cs_chunk & 0xf));
			while (cs_chunk)
			{
				short left_t[8], right_t[8];
				left_d.decode_chunk(r, left_t, 8);
				r += 4;
				right_d.decode_chunk(r, right_t, 8);
				r += 4;
				for (int i = 0; i < 8; i++)
				{
					*w++ = left_t[i];
					*w++ = right_t[i];
				}
				cs_chunk -= 16;
				cs_remaining -= 16;
			}
		}
	}
}