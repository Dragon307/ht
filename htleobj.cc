/* 
 *	HT Editor
 *	htleobj.cc
 *
 *	Copyright (C) 1999, 2000, 2001 Stefan Weyergraf (stefan@weyergraf.de)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "htatom.h"
#include "htnewexe.h"
#include "htle.h"
#include "htleobj.h"
#include "httag.h"
#include "htstring.h"
#include "formats.h"

#include "lestruct.h"

#include <stdlib.h>
#include <string.h>

ht_mask_ptable leobj[]=
{
	{"virtual size",			STATICTAG_EDIT_DWORD_LE("00000000")},
	{"relocation base address",	STATICTAG_EDIT_DWORD_LE("00000004")},
	{"flags",					STATICTAG_EDIT_DWORD_LE("00000008")" "STATICTAG_FLAGS("00000008", ATOM_LE_OBJFLAGS_STR)},
	{"page map index",			STATICTAG_EDIT_DWORD_LE("0000000c")},
	{"page map count",			STATICTAG_EDIT_DWORD_LE("00000010")},
	{"name",					STATICTAG_EDIT_CHAR("00000014")""STATICTAG_EDIT_CHAR("00000015")""STATICTAG_EDIT_CHAR("00000016")""STATICTAG_EDIT_CHAR("00000017")},
	{0, 0}
};

ht_tag_flags_s le_objflags[] =
{
	{0,  "[00] readable"},
	{1,  "[01] writable"},
	{2,  "[02] executable"},
	{3,  "[03] resource"},
	{4,  "[04] discardable"},
	{5,  "[05] shared"},
	{6,  "[06] preloaded"},
	{7,  "[07] * reserved"},
	{8,  "[08] * reserved"},
	{9,  "[09] * reserved"},
	{10, "[10] resident"},
	{11, "[11] * reserved"},
	{12, "[12] 16:16 alias"},
	{13, "[13] use32"},
	{14, "[14] conforming"},
	{15, "[15] object i/o privilege level"},
	{0, 0}
};

ht_view *htleobjects_init(bounds *b, ht_streamfile *file, ht_format_group *group)
{
	ht_le_shared_data *le_shared=(ht_le_shared_data *)group->get_shared_data();

	dword h=le_shared->hdr_ofs;
	ht_uformat_viewer *v=new ht_uformat_viewer();
	v->init(b, DESC_LE_OBJECTS, VC_EDIT, file, group);
	ht_mask_sub *m=new ht_mask_sub();
	m->init(file, 0);

	register_atom(ATOM_LE_OBJFLAGS, le_objflags);

/* FIXME: */
	bool le_bigendian = false;

	char t[64];
	sprintf(t, "* LE object headers at offset %08x", h+le_shared->hdr.objtab);
	m->add_mask(t);
	le_shared->objmap.count=le_shared->hdr.objcnt;
	le_shared->objmap.header=(IMAGE_LE_OBJECT_HEADER*)malloc(le_shared->objmap.count*sizeof *le_shared->objmap.header);
	le_shared->objmap.vsize=(dword*)malloc(le_shared->objmap.count*sizeof *le_shared->objmap.vsize);
	le_shared->objmap.psize=(dword*)malloc(le_shared->objmap.count*sizeof *le_shared->objmap.psize);

	v->insertsub(m);
	
	for (dword i=0; i<le_shared->hdr.objcnt; i++) {
		m=new ht_mask_sub();
		m->init(file, i);
		
		char n[5];
		file->seek(h+le_shared->hdr.objtab+i*24);
		file->read(&le_shared->objmap.header[i], sizeof *le_shared->objmap.header);

/* sum up page sizes to find object's physical size */
		dword psize=0;
		for (dword j=0; j<le_shared->objmap.header[i].page_map_count; j++) {
			psize+=le_shared->pagemap.psize[j+le_shared->objmap.header[i].page_map_index-1];
/* FIXME: security hole: array-index uncontrolled */
			if (j==le_shared->objmap.header[i].page_map_count-1)
				le_shared->pagemap.vsize[j+le_shared->objmap.header[i].page_map_index-1]=le_shared->objmap.header[i].virtual_size % le_shared->hdr.pagesize;
			else
				le_shared->pagemap.vsize[j+le_shared->objmap.header[i].page_map_index-1]=le_shared->hdr.pagesize;
		}
		le_shared->objmap.psize[i]=psize;

		le_shared->objmap.vsize[i]=le_shared->objmap.header[i].virtual_size;

		m->add_staticmask_ptable(leobj, h+le_shared->hdr.objtab+i*24, le_bigendian);
		
		memmove(&n, le_shared->objmap.header[i].name, 4);
		n[4]=0;

		bool use32=le_shared->objmap.header[i].flags & IMAGE_LE_OBJECT_FLAG_USE32;

		sprintf(t, "--- object %d USE%d: %s ---", i+1, use32 ? 32 : 16, (char*)&n);
		
		ht_collapsable_sub *cs=new ht_collapsable_sub();
		cs->init(file, m, 1, t, 1);
		v->insertsub(cs);
	}

	le_shared->v_objects=v;
	return v;
}

format_viewer_if htleobjects_if = {
	htleobjects_init,
	0
};
