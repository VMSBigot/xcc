// XCCFileView.cpp : implementation file
//

#include "stdafx.h"
#include "MainFrm.h"
#include "XCCFileView.h"
#include "XCC Mixer.h"

#include <cmath>
#include "aud_file.h"
#include "cps_file.h"
#include "fname.h"
#include "fnt_file.h"
#include "hva_file.h"
#include <id_log.h>
#include "map_td_ini_reader.h"
#include "map_ra_ini_reader.h"
#include "map_ts_ini_reader.h"
#include "mp3_file.h"
#include "pal_file.h"
#include "pcx_decode.h"
#include "pcx_file.h"
#include "shp_decode.h"
#include "shp_images.h"
#include "shp_dune2_file.h"
#include "shp_file.h"
#include "shp_ts_file.h"
#include "sound_ts_ini_reader.h"
#include "st_file.h"
#include "string_conversion.h"
#include "theme_ts_ini_reader.h"
#include "tmp_file.h"
#include "tmp_ra_file.h"
#include "tmp_ts_file.h"
#include "virtual_tfile.h"
#include "voc_file.h"
#include "vqa_file.h"
#include "vxl_file.h"
#include "wav_file.h"
#include "wsa_dune2_file.h"
#include "wsa_file.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CXCCFileView

IMPLEMENT_DYNCREATE(CXCCFileView, CScrollView)

CXCCFileView::CXCCFileView()
{
	m_is_open = false;
}

CXCCFileView::~CXCCFileView()
{
	if (m_is_open)
		close_f();
}

BEGIN_MESSAGE_MAP(CXCCFileView, CScrollView)
	//{{AFX_MSG_MAP(CXCCFileView)
	ON_UPDATE_COMMAND_UI(ID_FILE_NEW, OnDisable)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN, OnDisable)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSE, OnDisable)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXCCFileView drawing

void CXCCFileView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	SetScrollSizes(MM_TEXT, CSize(0, 0));
	m_font.CreateFont(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, "Courier New");
}

void CXCCFileView::draw_image8(const byte* s, int cx_s, int cy_s, CDC* pDC, int x_d, int y_d)
{
	if (!CRect().IntersectRect(m_clip_rect, CRect(CPoint(x_d, y_d), CSize(cx_s, cy_s))))
		return;
	CDC mem_dc;
	mem_dc.CreateCompatibleDC(pDC);
	void* old_bitmap;
	{	
		BITMAPINFO bmi;
		ZeroMemory(&bmi, sizeof(BITMAPINFO));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = cx_s;
		bmi.bmiHeader.biHeight = -cy_s;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = bmi.bmiHeader.biWidth * -bmi.bmiHeader.biHeight * (bmi.bmiHeader.biBitCount >> 3);
		mh_dib = CreateDIBSection(*pDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&mp_dib), 0, 0);
	}
	old_bitmap = mem_dc.SelectObject(mh_dib);
	for (int y = 0; y < cy_s; y++)
	{
		for (int x = 0; x < cx_s; x++)
			mp_dib[x + cx_s * y] = m_color_table[s[x + cx_s * y]];
	}
	pDC->BitBlt(x_d, y_d, cx_s, cy_s, &mem_dc, 0, 0, SRCCOPY);
	mem_dc.SelectObject(old_bitmap);
	DeleteObject(mh_dib);
}

void CXCCFileView::draw_image24(const byte* s, int cx_s, int cy_s, CDC* pDC, int x_d, int y_d)
{
	if (!CRect().IntersectRect(m_clip_rect, CRect(CPoint(x_d, y_d), CSize(cx_s, cy_s))))
		return;
	CDC mem_dc;
	mem_dc.CreateCompatibleDC(pDC);
	void* old_bitmap;
	{	
		BITMAPINFO bmi;
		ZeroMemory(&bmi, sizeof(BITMAPINFO));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = cx_s;
		bmi.bmiHeader.biHeight = -cy_s;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = bmi.bmiHeader.biWidth * -bmi.bmiHeader.biHeight * (bmi.bmiHeader.biBitCount >> 3);
		mh_dib = CreateDIBSection(*pDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&mp_dib), 0, 0);
	}
	old_bitmap = mem_dc.SelectObject(mh_dib);
	const byte* r = s;
	for (int y = 0; y < cy_s; y++)
	{
		for (int x = 0; x < cx_s; x++)
		{
			t_palet32bgr_entry v;
			v.r = *s++;
			v.g = *s++;
			v.b = *s++;
			mp_dib[x + cx_s * y] = v.v;
		}
	}
	pDC->BitBlt(x_d, y_d, cx_s, cy_s, &mem_dc, 0, 0, SRCCOPY);
	mem_dc.SelectObject(old_bitmap);
	DeleteObject(mh_dib);
}

static CMainFrame* GetMainFrame()
{
	return reinterpret_cast<CMainFrame*>(AfxGetMainWnd());
}

const t_paletentry* CXCCFileView::get_default_palet()
{
	const t_palet_entry* p = GetMainFrame()->get_pal_data();
	if (p)
		return p;
	if (m_palet)
		return m_palet;
	return GetMainFrame()->get_game_palet(m_game);
}

void CXCCFileView::load_color_table(const t_palet palet, bool convert_palet)
{
	t_palet p;
	if (!palet)
	{
		convert_palet = true;
		palet = get_default_palet();
	}
	memcpy(p, palet, sizeof(t_palet));
	if (convert_palet)
		convert_palet_18_to_24(p);
	t_palet32bgr_entry* color_table = reinterpret_cast<t_palet32bgr_entry*>(m_color_table);
	for (long i = 0; i < 256; i++)
	{
		color_table[i].r = p[i].r;
		color_table[i].g = p[i].g;
		color_table[i].b = p[i].b;
	}
}

void CXCCFileView::draw_info(const string& n, const string& d)
{
	if (!m_text_cache_valid)
	{
		t_text_cache_entry e;
		e.text_extent = CRect(CPoint(0, m_y), m_dc->GetTextExtent(n.c_str()));
		e.t = n;
		m_text_cache.push_back(e);
		if (!d.empty())
		{
			e.text_extent = CRect(CPoint(128, m_y), m_dc->GetTextExtent(n.c_str()));
			e.t = d;
			m_text_cache.push_back(e);
		}
	}
	m_y += m_y_inc;
}


void CXCCFileView::OnDisable(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(false);
}

void fnt_decode(const byte* r, byte* w, int cx, int cy)
{
	while (cy--)
	{
		for (int x = 1; x < cx; x += 2)
		{
			*w++ = *r & 0xf;
			*w++ = *r++ >> 4;
		}
		if (cx & 1)
			*w++ = *r++ & 0xf;
	}
}

void fnt_adjust(byte* d, int size)
{
	while (size--)
	{
		byte& v = *d++;
		v = v ? 0 : 0xff;
	}
}

struct t_vector
{
	double x;
	double y;
	double z;
};

const double pi = 3.141592654;

t_vector rotate_x(t_vector v, double a)
{
	double l = sqrt(v.y * v.y + v.z * v.z);
	double d_a = atan2(v.y, v.z) + a;
	t_vector r;
	r.x = v.x;
	r.y = l * sin(d_a);
	r.z = l * cos(d_a);
	return r;
}

t_vector rotate_y(t_vector v, double a)
{
	double l = sqrt(v.x * v.x + v.z * v.z);
	double d_a = atan2(v.x, v.z) + a;
	t_vector r;
	r.x = l * sin(d_a);
	r.y = v.y;
	r.z = l * cos(d_a);
	return r;
}

void CXCCFileView::OnDraw(CDC* pDC)
{
	const char* b2a[] = {"no", "yes"};
	pDC->SelectObject(&m_font);
	if (m_is_open)
	{
		pDC->GetClipBox(&m_clip_rect);
		TEXTMETRIC tm;
		pDC->GetTextMetrics(&tm);
		m_dc = pDC;
		m_y = 0;
		m_y_inc = tm.tmHeight;
		draw_info("ID:", nh(8, m_id));
		draw_info("Size:", n(m_size));
		draw_info("Type:", ft_name[m_ft]);
		switch (m_ft)
		{
		case ft_aud:
			{
				Caud_file f;
				f.load(m_data, m_cb_data);
				draw_info("Audio:", n(f.get_samplerate()) + " hz, " + n(f.get_cb_sample() << 3) + " bit, " + (f.get_c_channels() == 1 ? "mono" : "stereo"));
				draw_info("Count samples:", n(f.get_c_samples()));
				draw_info("Compression:", nh(2, f.get_header()->compression));
				break;
			}
		case ft_cps:
			{
				Ccps_file f;
				f.load(m_data, m_cb_data);
				draw_info("Palet:", f.has_palet() ? "yes" : "no");
				m_y += m_y_inc;
				byte* image = new byte[320 * 200];
				decode80(f.get_image(), image);
				load_color_table(f.has_palet() ? f.get_palet() : get_default_palet(), true);
				draw_image8(image, 320, 200, pDC, 0, m_y);
				m_y += 200 + m_y_inc;
				delete[] image;
				break;
			}
		case ft_fnt:
			{
				Cfnt_file f;
				f.load(m_data, m_cb_data);
				const int c_chars = f.get_c_chars();
				const t_fnt_header& header = *f.get_header();
				draw_info("Count chars:", n(c_chars));
				draw_info("Size:", n(f.get_cmax_x()) + " x " + n(f.get_cy()));
				m_y += m_y_inc;
				byte* d = new byte[f.get_cmax_x() * f.get_cy()];
				load_color_table(get_default_palet(), true);
				for (int i = 0; i < c_chars; i++)
				{
					const int cx = f.get_cx(i);
					const int cy = f.get_cy(i);
					if (!cx || !cy)
						continue;
					fnt_decode(f.get_image(i), d, cx, cy);
					fnt_adjust(d, cx * cy);
					draw_image8(d, cx, cy, pDC, 400, m_y);
					draw_info(nwzl(3, i), n(cx) + " x " + n(cy) + " at " + n(f.get_image(i) - f.get_data()));
				}
				delete[] d;
				break;
			}
		case ft_hva:
			{
				Chva_file f;
				f.load(m_data, m_size);
				draw_info("Count frames:", n(f.get_c_frames()));
				draw_info("Count sections:", n(f.get_c_sections()));
				break;
			}
		case ft_map_td:
			{
				Cvirtual_tfile tf;
				tf.load_data(m_data, m_size);
				Cmap_td_ini_reader ir;
				while (!tf.eof())
				{
					ir.process_line(tf.read_line());
				}
				const Cmap_td_ini_reader::t_map_data& md = ir.get_map_data();
				draw_info("Name:", ir.get_basic_data().name);
				draw_info("Size:", n(md.cx) + " x " + n(md.cy));
				draw_info("Theater:", ir.get_map_data().theater);
				break;
			}
		case ft_map_ra:
			{
				Cvirtual_tfile tf;
				tf.load_data(m_data, m_size);
				Cmap_ra_ini_reader ir;
				while (!tf.eof())
				{
					ir.process_line(tf.read_line());
				}
				const Cmap_ra_ini_reader::t_map_data& md = ir.get_map_data();
				draw_info("Name:", ir.get_basic_data().name);
				draw_info("Size:", n(md.cx) + " x " + n(md.cy));
				draw_info("Theater:", ir.get_map_data().theater);
				break;
			}
		case ft_map_ts:
			{
				Cvirtual_tfile tf;
				tf.load_data(m_data, m_size);
				Cmap_ts_ini_reader ir;
				while (!tf.eof())
				{
					ir.process_line(tf.read_line());
				}
				const Cmap_ts_ini_reader::t_map_data& md = ir.get_map_data();
				const Cmap_ts_ini_reader::t_preview_data& pd = ir.get_preview_data();
				const string& pdp = ir.get_preview_pack_data();
				draw_info("Name:", ir.get_basic_data().name);
				draw_info("Size:", n(md.size_right) + " x " + n(md.size_bottom));
				draw_info("Theater:", ir.get_map_data().theater);
				if (pd.cx && pd.cy)
				{
					m_y += m_y_inc;
					byte* s = new byte[pdp.length()];
					int cb_s = decode64(reinterpret_cast<const byte*>(pdp.c_str()), s);
					byte* image = new byte[pd.cx * pd.cy * 3];
					decode5(s, image, cb_s);
					draw_image24(image, pd.cx, pd.cy, pDC, 0, m_y);
					delete[] image;
					delete[] s;
				}
				break;
			}
		case ft_mix:
		case ft_pak:
			{
				Cmix_file f;
				f.load(m_data, m_size);
				const int c_files = f.get_c_files();
				const t_game game = f.get_game();
				draw_info("Count files:", n(c_files));
				draw_info("Checksum:", n(f.has_checksum()));
				draw_info("Encrypted:", n(f.is_encrypted()));
				draw_info("Game:", game_name[game]);
				m_y += m_y_inc;
				for (int i = 0; i < c_files; i++)
				{
					int id = f.get_id(i);
					draw_info(nwzl(4, i) + " - " + nh(8, id) + nwsl(11, f.get_size(id)) + ' ' + mix_database::get_name(game, id), "");
				}
				break;
			}
		case ft_mp3:
			{
				Cmp3_file f;
				f.load(m_data, m_size);
				const Cmp3_frame_header& header = f.header();
				draw_info("Bitrate:", n(header.bitrate()));
				draw_info("Channel mode:", mpcm_name[header.channel_mode()]);
				draw_info("Copyright:", header.copyright() ? "yes" : "no");
				draw_info("CRC:", header.crc() ? "yes" : "no");
				draw_info("Emphasis:", n(header.emphasis()));
				draw_info("Layer:", n(header.layer()));
				draw_info("Mode extension:", n(header.mode_extension()));
				draw_info("Original:", header.original() ? "yes" : "no");
				draw_info("Padding:", header.padding() ? "yes" : "no");
				draw_info("Samplerate:", n(header.samplerate()));
				draw_info("Version:", mpv_name[header.version()]);
				break;
			}
		case ft_pal:
			{
				Cpal_file f;
				f.load(m_data, m_cb_data);
				m_y += m_y_inc;
				const t_palet& palet = *f.get_palet();
				for (int i = 0; i < 256; i++)
				{
					draw_info((nh(2, i) + " - " + nwzl(2, palet[i].r) + ' '+ nwzl(2, palet[i].g) + ' '+ nwzl(2, palet[i].b)), "");
				}
				break;
			}
		case ft_pcx:
			{
				Cpcx_file f;
				f.load(m_data, m_cb_data);
				const int c_planes = f.get_c_planes();;
				const int cx = f.get_cx();
				const int cy = f.get_cy();
				draw_info("Bits/pixel:", n(8 * c_planes));
				draw_info("Size:", n(cx) + " x " + n(cy));
				m_y += m_y_inc;
				byte* image = new byte[c_planes * cx * cy];
				pcx_decode(f.get_image(), image, *f.get_header());
				if (c_planes == 1)
				{
					load_color_table(*f.get_palet(), false);
					draw_image8(image, cx, cy, pDC, 0, m_y);
				}
				else
					draw_image24(image, cx, cy, pDC, 0, m_y);
				m_y += cy + m_y_inc;
				delete[] image;
				break;
			}
		case ft_shp_dune2:
			{
				Cshp_dune2_file f;
				f.load(m_data, m_cb_data);
				const int c_images = f.get_c_images();
				draw_info("Count images:", n(c_images));
				draw_info("Offset size:", n(f.get_cb_ofs()));
				m_y += m_y_inc;
				load_color_table(get_default_palet(), true);
				for (int i = 0; i < c_images; i++)
				{
					const int cx = f.get_cx(i);
					const int cy = f.get_cy(i);
					if (f.get_image_header(i)->compression & 1)
						draw_info("Unknown block",  "yes");
					byte* image = new byte[cx * cy];
					if (f.is_compressed(i))
					{
						byte* d = new byte[f.get_image_header(i)->size_out];
						decode2(d, image, decode80(f.get_image(i), d));
						delete[] d;
					}
					else
						decode2(f.get_image(i), image, f.get_image_header(i)->size_out);
					draw_image8(image, cx, cy, pDC, 0, m_y);
					delete[] image;
					m_y += cy + m_y_inc;
				}
				break;
			}
		case ft_shp:
			{
				Cshp_file f;
				f.load(m_data, m_cb_data);
				const int c_images = f.get_c_images();
				int cx = f.get_cx();
				int cy = f.get_cy();
				draw_info("Count images:", n(c_images));
				draw_info("Size:", n(cx) + " x " + n(cy));
				draw_info("Unknown1:", n(f.get_header()->unknown1));
				draw_info("Unknown2:", n(f.get_header()->unknown2));
				draw_info("Unknown3:", n(f.get_header()->unknown3));
				m_y += m_y_inc;
				void* p;
				if (cx && cy && !shp_images::load_shp(f, p))
				{
					load_color_table(get_default_palet(), true);
					for (int i = 0; i < c_images; i++)
					{
						draw_image8(shp_images::get_shp(p, i), cx, cy, pDC, 0, m_y);
						m_y += cy + m_y_inc;
					}
					shp_images::destroy_shp(p);
				}
				break;
			}
		case ft_shp_ts:
			{
				Cshp_ts_file f;
				f.load(m_data, m_cb_data);
				const int c_images = f.get_c_images();
				const int cx = f.get_cx();
				const int cy = f.get_cy();
				draw_info("Count images:", n(c_images));
				draw_info("Size:", n(cx) + " x " + n(cy));
				m_y += m_y_inc;
				load_color_table(get_default_palet(), true);
				for (int i = 0; i < c_images; i++)
				{
					const int cx = f.get_cx(i);
					const int cy = f.get_cy(i);
					if (cx && cy)
					{
						if (f.is_compressed(i))
						{
							byte* image = new byte[cx * cy];
							decode3(f.get_image(i), image, cx, cy);
							draw_image8(image, cx, cy, pDC, 0, m_y);
							delete[] image;
						}
						else
							draw_image8(f.get_image(i), cx, cy, pDC, 0, m_y);
						m_y += cy + m_y_inc;
					}
				}
				break;
			}
		case ft_sound_ini_ts:
			{
				Csound_ts_ini_reader ir;
				ir.process(m_data, m_size);
				const Csound_ts_ini_reader::t_sound_list& sl = ir.get_sound_list();
				draw_info("Count sounds:", n(sl.size()));
				m_y += m_y_inc;
				/*
				int column_size[] = {0, 6, 3, 0};
				for (Csound_ts_ini_reader::t_sound_list::const_iterator i = sl.begin(); i != sl.end(); i++)
				{
					const Csound_data& td = i->second;
					column_size[0] = max(column_size[0], td.name().length());
					column_size[3] = max(column_size[3], td.side().length());
				}
				*/
				for (Csound_ts_ini_reader::t_sound_list::const_iterator i = sl.begin(); i != sl.end(); i++)
				{
					draw_info(i->first, "");
				}
				break;
			}
		case ft_st:
			if (!m_text_cache_valid)
			{
				Cst_file f;
				f.load(m_data, m_cb_data);
				const int c_strings = f.get_c_strings();
				draw_info("Count strings", n(c_strings));
				m_y += m_y_inc;
				for (int i = 0; i < c_strings; i++)
					draw_info(nwzl(5, i) + ' ' + f.get_string(i), "");
			}
			break;
		case ft_art_ini_ts:
		case ft_ini:
		case ft_rules_ini_ts:
		case ft_text:
			if (!m_text_cache_valid)
			{
				m_y += m_y_inc;
				Cvirtual_tfile tf;
				tf.load_data(m_data, m_cb_data);
				while (!tf.eof())
					draw_info(tf.read_line(), "");
			}
			break;
		case ft_theme_ini_ts:
			{
				Cvirtual_tfile tf;
				tf.load_data(m_data, m_size);
				Ctheme_ts_ini_reader ir;
				while (!tf.eof())
				{
					ir.process_line(tf.read_line());
				}
				const Ctheme_ts_ini_reader::t_theme_list& tl = ir.get_theme_list();
				draw_info("Count themes:", n(tl.size()));
				m_y += m_y_inc;
				int column_size[] = {0, 6, 3, 0};
				for (Ctheme_ts_ini_reader::t_theme_list::const_iterator i = tl.begin(); i != tl.end(); i++)
				{
					const Ctheme_data& td = i->second;
					column_size[0] = max(column_size[0], td.name().length());
					column_size[3] = max(column_size[3], td.side().length());
				}
				for (i = tl.begin(); i != tl.end(); i++)
				{
					const Ctheme_data& td = i->second;
					float length = td.length();
					string line = swsr(column_size[0], td.name()) + nwsl(3, length) + ':' + nwzl(2, static_cast<int>(length * 60) % 60)
						+ nwsl(3, td.scenario())
						+ ' ' + swsr(column_size[3], td.side());
					if (td.normal())
						line += " normal";
					if (td.repeat())
						line += " repeat";
					draw_info(line, "");
				}
				break;
			}
		case ft_tmp:
			{
				Ctmp_file f;
				f.load(m_data, m_cb_data);
				const int c_tiles = f.get_c_tiles();
				draw_info("Count tiles:", n(c_tiles));
				m_y += m_y_inc;
				load_color_table(get_default_palet(), true);
				for (int i = 0; i < c_tiles; i++)
				{
					if (f.get_index1()[i] != 0xff)
					{
						draw_image8(f.get_image(i), 24, 24, pDC, 0, m_y);
						m_y += 24 + m_y_inc;
					}
				}
				break;
			}
		case ft_tmp_ra:
			{
				Ctmp_ra_file f;
				f.load(m_data, m_cb_data);
				const int c_tiles = f.get_c_tiles();
				const int cx = f.get_cblocks_x();
				const int cy = f.get_cblocks_y();
				draw_info("Count tiles:", n(c_tiles));
				draw_info("Size:", n(cx) + " x " + n(cy));
				m_y += m_y_inc;
				load_color_table(get_default_palet(), true);
				if (cx == 1 && cy == 1)
				{
					for (int i = 0; i < c_tiles; i++)
					{
						if (f.get_index1()[i] != 0xff)
						{
							draw_image8(f.get_image(i), 24, 24, pDC, 0, m_y);
							m_y += 24 + m_y_inc;
						}
					}
				}
				else
				{
					int i = 0;
					for (int y = 0; y < cy; y++)
					{
						for (int x = 0; x < cx; x++)
						{
							if (f.get_index1()[i] != 0xff)
							{
								draw_image8(f.get_image(i), 24, 24, pDC, 24 * x, m_y);
							}
							i++;
						}
						m_y += 24;
					}
					m_y += m_y_inc;
				}
				break;
			}
		case ft_tmp_ts:
			{
				Ctmp_ts_file f;
				f.load(m_data, m_cb_data);
				const int c_tiles = f.get_c_tiles();
				draw_info("Count tiles:", n(c_tiles));
				draw_info("Size:", n(f.get_cblocks_x()) + " x " + n(f.get_cblocks_y()));
				m_y += m_y_inc;
				load_color_table(get_default_palet(), true);
				int x, y, cx, cy;
				f.get_rect(x, y, cx, cy);
				byte* d = new byte[cx * cy];
				f.draw(d);
				draw_image8(d, cx, cy, pDC, 0, m_y);
				m_y += cy + m_y_inc;
				for (int i = 0; i < f.get_c_tiles(); i++)
				{
					if (!f.get_index()[i])
						continue;
					const t_tmp_image_header& header = *f.get_image_header(i);
					draw_info("Tile:", n(i));
					/*
					draw_info("x:", n(header.x));
					draw_info("y:", n(header.y));
					for (int j = 0; j < 3; j++)
						draw_info("unknown1[" + n(j) +"]:", n(header.unknown1[j]));
					draw_info("x_extra:", n(header.x_extra));
					draw_info("y_extra:", n(header.y_extra));
					draw_info("cx_extra:", n(header.cx_extra));
					draw_info("cy_extra:", n(header.cy_extra));
					*/
					draw_info("flags:", n(header.flags & 7));
					draw_info("unknown2[0]:", nh(6, header.unknown2[0] & 0xffffff));
					/*
					for (j = 0; j < 3; j++)
						draw_info("unknown2[" + n(j) +"]:", nh(8, header.unknown2[j]));
					if (f.has_extra_graphics(i))
					{
						draw_image8(f.get_image(i) + 2 * 576, header.cx_extra, header.cy_extra, pDC, 0, m_y);
						m_y += header.cy_extra;
					}
					*/
					m_y += m_y_inc;
				}
				delete[] d;
				break;
			}
		case ft_voc:
			{
				Cvoc_file f;
				f.load(m_data, m_cb_data);
				draw_info("Audio:", n(f.get_samplerate()) + " hz, 8 bit, mono");
				draw_info("Count samples:", n(f.get_c_samples()));
				break;
			}
		case ft_vqa:
			{
				Cvqa_file f;
				f.load(m_data, m_cb_data);
				draw_info("Version:", n(f.get_header()->version));
				draw_info("Video flags:", nh(4, f.get_header()->video_flags));
				draw_info("Count frames:", n(f.get_c_frames()));
				draw_info("Size:", n(f.get_cx()) + " x " + n(f.get_cy()));
				draw_info("Block size:", n(f.get_cx_block()) + " x " + n(f.get_cy_block()));
				draw_info("Audio:", n(f.get_samplerate()) + " hz, " + n(f.get_cbits_sample()) + " bit, " + (f.get_c_channels() == 1 ? "mono" : "stereo"));
				break;
			}
		case ft_vxl:
			{
				m_y += m_y_inc;
				Cvxl_file f;
				f.load(m_data, m_cb_data);
				int vxl_mode = GetMainFrame()->get_vxl_mode();
				t_palet gray_palet;
				for (int i = 0; i < 256; i++)
					gray_palet[i].r = gray_palet[i].g = gray_palet[i].b = i * 255 / 35;
				load_color_table(get_default_palet()/*f.get_palet()*/, true);
				for (i = 0; i < f.get_c_section_headers(); i++)
				{
					const t_vxl_section_tailer& section_tailer = *f.get_section_tailer(i);
					const int cx = section_tailer.cx;
					const int cy = section_tailer.cy;
					const int cz = section_tailer.cz;
					const int l = ceil(sqrt((cx * cx + cy * cy + cz * cz) / 4.0));
					const int cl = 2 * l;
					const double center_x = cx / 2;
					const double center_y = cy / 2;
					const double center_z = cz / 2;
					const int c_pixels = cl * cl;
					draw_info("Section " + n(i) + ':', n(cx) + " x " + n(cy) + " x " + n(cz));
					char fb[32];
					for (int ty = 0; ty < 3; ty++)
					{
						string s;
						for (int tx = 0; tx < 4; tx++)
						{
							s += _gcvt(section_tailer.transform[ty][tx], 10, fb);
							s += ' ';
						}
						draw_info(n(ty), s);
					}
					draw_info("Scale:", _gcvt(section_tailer.scale, 10, fb));
					draw_info("Scale X min:", _gcvt(section_tailer.x_min_scale, 10, fb));
					draw_info("Scale Y min:", _gcvt(section_tailer.y_min_scale, 10, fb));
					draw_info("Scale Z min:", _gcvt(section_tailer.z_min_scale, 10, fb));
					draw_info("Scale X max:", _gcvt(section_tailer.x_max_scale, 10, fb));
					draw_info("Scale Y max:", _gcvt(section_tailer.y_max_scale, 10, fb));
					draw_info("Scale Z max:", _gcvt(section_tailer.z_max_scale, 10, fb));
					draw_info("Unknown:", n(section_tailer.unknown));
					byte* image = new byte[c_pixels];
					byte* image_s = new byte[c_pixels];
					char* image_z = new char[c_pixels];
					for (int yr = 0; yr < 8; yr++)
					{
						for (int xr = 0; xr < 8; xr++)
						{
							if (CRect().IntersectRect(m_clip_rect, CRect(CPoint(xr * (cl + m_y_inc), m_y), CSize(cl, cl))))
							{
								memset(image, 0, c_pixels);
								memset(image_s, 0, c_pixels);
								memset(image_z, CHAR_MIN, c_pixels);
								int j = 0;
								for (int y = 0; y < cy; y++)
								{
									for (int x = 0; x < cx; x++)
									{
										const byte* r = f.get_span_data(i, j);
										if (r)
										{
											int z = 0;
											while (z < cz)
											{
												z += *r++;
												int c = *r++;
												while (c--)
												{
													t_vector s_pixel;
													s_pixel.x = x - center_x;
													s_pixel.y = y - center_y;
													s_pixel.z = z - center_z;
													t_vector d_pixel = rotate_y(rotate_x(s_pixel, xr * pi / 4), yr * pi / 4);
													d_pixel.x += l;
													d_pixel.y += l;
													d_pixel.z += center_z;
													int ofs = static_cast<int>(d_pixel.x) + cl * static_cast<int>(d_pixel.y);
													if (d_pixel.z > image_z[ofs])
													{
														image[ofs] = *r++;
														image_s[ofs] = *r++;
														image_z[ofs] = d_pixel.z;
													}
													else
														r += 2;;
													z++;
												}
												r++;
											}
										}
										j++;
									}
								}
								switch (vxl_mode)
								{
								case 0:
									draw_image8(image, cl, cl, pDC, xr * (cl + m_y_inc), m_y);
									break;
								case 1:
									load_color_table(gray_palet, false);
									draw_image8(image_s, cl, cl, pDC, xr * (cl + m_y_inc), m_y);
									break;
								case 2:
									{
										int min_z = INT_MAX;
										int max_z = INT_MIN;
										for (int o = 1; o < c_pixels; o++)
										{
											int v = image_z[o];
											if (v == CHAR_MIN)
												continue;
											if (v < min_z)
												min_z = v;
											if (v > max_z)
												max_z = v;
										}
										for (o = 0; o < c_pixels; o++)
										{
											if (image_z[o] == CHAR_MIN)
												image_z[o] = -1;
											else
												image_z[o] -= min_z;
										}
										max_z -= min_z;
										for (int p = 0; p < max_z; p++)
											gray_palet[p].r = gray_palet[p].g = gray_palet[p].b = p * 255 / max_z;
										gray_palet[0xff].r = 0;
										gray_palet[0xff].g = 0;
										gray_palet[0xff].b = 0xff;
										load_color_table(gray_palet, false);
										draw_image8(reinterpret_cast<const byte*>(image_z), cl, cl, pDC, xr * (cl + m_y_inc), m_y);
										break;
									}
								}
							}
						}
						m_y += cl + m_y_inc;
					}					
					delete[] image_z;
					delete[] image_s;
					delete[] image;
				}
				break;
			}
		case ft_wav:
			{
				Cwav_file f;
				f.load(m_data, m_cb_data);
				if (!f.process())
				{
					const t_riff_wave_format_chunk& format_chunk = f.get_format_chunk();
					draw_info("Audio:", n(format_chunk.samplerate) + " hz, " + n(format_chunk.cbits_sample) + " bit, " + (format_chunk.c_channels == 1 ? "mono" : "stereo"));
					draw_info("Count samples:", n(f.get_data_header().size * 8 / (format_chunk.cbits_sample * format_chunk.c_channels)));
					draw_info("Format:", nh(4, format_chunk.tag));
				}
				break;
			}
		case ft_wsa_dune2:
			{
				Cwsa_dune2_file f;
				f.load(m_data, m_cb_data);
				const int c_frames = f.get_c_frames();
				const int cx = f.get_cx();
				const int cy = f.get_cy();
				draw_info("Count frames:", n(c_frames));
				draw_info("Size:", n(cx) + " x " + n(cy));
				m_y += m_y_inc;
				byte* frame = new byte[cx * cy];
				memset(frame, 0, cx * cy);
				byte* temp = new byte[64 << 10];
				load_color_table(get_default_palet(), true);
				for (int i = 0; i < c_frames; i++)
				{
					if (f.get_offset(i))
					{
						decode80(f.get_frame(i), temp);
						decode40(temp, frame);					
					}
					draw_info("Offset:", n(f.get_offset(i)));
					draw_image8(frame, cx, cy, pDC, 0, m_y);
					m_y += cy + m_y_inc;
				}
				delete[] temp;
				delete[] frame;
				break;
			}
		case ft_wsa:
			{
				Cwsa_file f;
				f.load(m_data, m_cb_data);
				const int c_frames = f.get_c_frames();
				const int cx = f.get_cx();
				const int cy = f.get_cy();
				draw_info("Count frames:", n(c_frames));
				draw_info("Palet:", f.has_palet() ? "yes" : "no");
				draw_info("Position:", n(f.get_x()) + "," + n(f.get_y()));
				draw_info("Size:", n(cx) + " x " + n(cy));
				m_y += m_y_inc;
				byte* frame = new byte[cx * cy];
				memset(frame, 0, cx * cy);
				byte* temp = new byte[64 << 10];
				load_color_table(f.has_palet() ? f.get_palet() : get_default_palet(), true);
				for (int i = 0; i < c_frames; i++)
				{
					if (f.get_offset(i))
					{
						decode80(f.get_frame(i), temp);
						decode40(temp, frame);
					}
					draw_image8(frame, cx, cy, pDC, 0, m_y);
					m_y += cy + m_y_inc;
				}
				delete[] temp;
				delete[] frame;
				break;
			}
		default:
			if (!m_text_cache_valid)
			{
				m_y += m_y_inc;
				if (m_cb_data > 32 * 1024)
					m_cb_data = 32 * 1024;
				for (int r = 0; r < m_cb_data; )
				{
					string line = nwzl(5, r) + ' ';
					int line_data[16];
					for (int c = 0; c < 16; c++)
					{
						line_data[c] = r < m_cb_data ? m_data[r] : -1;
						r++;
					}
					for (c = 0; c < 16; c++)
					{
						if (!(c & 7))
							line += "- ";
						line += line_data[c] == -1 ? "   " : nh(2, line_data[c]) + ' ';
					}
					line += "- ";
					for (c = 0; c < 16; c++)
						line += line_data[c] < 0x20 ? ' ' : line_data[c];
					draw_info(line, "");
				}
			}
		}
		if (!m_text_cache_valid)
		{
			SetScrollSizes(MM_TEXT, CSize(0, m_y));
			m_text_cache_valid = true;
		}
		for (t_text_cache::iterator i = m_text_cache.begin(); i != m_text_cache.end(); i++)
		{
			if (CRect().IntersectRect(m_clip_rect, i->text_extent))
				pDC->TextOut(i->text_extent.TopLeft().x, i->text_extent.TopLeft().y, i->t.c_str());
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CXCCFileView diagnostics

#ifdef _DEBUG
void CXCCFileView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CXCCFileView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CXCCFileView message handlers

void CXCCFileView::open_f(int id, Cmix_file& mix_f, t_game game, t_palet palet)
{
	if (m_is_open)
		close_f();
	Ccc_file f(false);
	if (!f.open(id, mix_f))
	{
		m_game = game;
		m_id = id;
		m_palet = palet;
	}
	post_open(f);
}

void CXCCFileView::open_f(const string& name)
{
	if (m_is_open)
		close_f();
	Ccc_file f(false);
	if (!f.open(name))
	{
		m_game = GetMainFrame()->get_game();
		m_id = Cmix_file::get_id(m_game, Cfname(name).get_ftitle());
		m_palet = NULL;
	}
	post_open(f);
}

void CXCCFileView::post_open(Ccc_file& f)
{
	if (f.is_open())
	{
		m_ft = f.get_file_type();
		m_size = f.get_size();
		int cb_max_data = (m_ft == ft_map_td || m_ft == ft_map_ra || m_ft == ft_map_ts || m_ft == ft_pcx || m_ft == ft_shp || m_ft == ft_shp_ts || m_ft == ft_vxl || m_ft == ft_wsa_dune2 || m_ft == ft_wsa) ? m_size : 256 << 10;
		m_cb_data = m_size > cb_max_data ? cb_max_data : m_size;	
		m_data = new byte[m_cb_data];
		f.read(m_data, m_cb_data);
		f.close();
		m_text_cache_valid = false;
		m_is_open = true;
	}
	ScrollToPosition(CPoint(0, 0));
	Invalidate();
}

void CXCCFileView::close_f()
{
	m_is_open = false;
	m_text_cache.clear();
	delete[] m_data;
}