/*
 *	HT Editor
 *	htobj.cc
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
#include "cmds.h"
#include "htapp.h"
#include "htctrl.h"
#include "htdebug.h"
#include "htkeyb.h"
#include "htmenu.h"
#include "htobj.h"
#include "htpal.h"
#include "htreg.h"
#include "htstring.h"
#include "snprintf.h"
#include "store.h"
#include "tools.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define ATOM_HT_VIEW		MAGICD("OBJ\0")
#define ATOM_HT_GROUP		MAGICD("OBJ\1")
#define ATOM_HT_XGROUP		MAGICD("OBJ\2")
#define ATOM_HT_WINDOW		MAGICD("OBJ\3")
#define ATOM_HT_FRAME		MAGICD("OBJ\4")
#define ATOM_HT_SCROLLBAR	MAGICD("OBJ\5")

void bounds_and(bounds *a, bounds *b)
{
	if (b->x>a->x) {
		a->w-=b->x-a->x;
		a->x=b->x;
	}
	if (b->y>a->y) {
		a->h-=b->y-a->y;
		a->y=b->y;
	}
	if (a->x+a->w>b->x+b->w) a->w-=a->x+a->w-b->x-b->w;
	if (a->y+a->h>b->y+b->h) a->h-=a->y+a->h-b->y-b->h;
	if (a->w<0) a->w=0;
	if (a->h<0) a->h=0;
}

void put_bounds(ht_object_stream *s, bounds *b)
{
	s->putIntDec(b->x, 4, NULL);
	s->putIntDec(b->y, 4, NULL);
	s->putIntDec(b->w, 4, NULL);
	s->putIntDec(b->h, 4, NULL);
}

void clearmsg(htmsg *msg)
{
	msg->msg=msg_empty;
	msg->type=mt_empty;
}

/*
 *	CLASS ht_text
 */

void ht_text::settext(char *text)
{
}

/*
 *	CLASS ht_view
 */

void ht_view::init(bounds *b, int _options, char *_desc)
{
	Object::init();
	VIEW_DEBUG_NAME("ht_view");
	desc=ht_strdup(_desc);
	group=0;
	focused=0;
	browse_idx=0;
	view_is_dirty=1;
	size.x=0;
	size.y=0;
	size.w=0;
	size.h=0;
	prev=0;
	next=0;
	setoptions(_options);
	buf=0;
	enabled=1;

	growmode = GM_TOP |  GM_LEFT;
	
	if (options & VO_OWNBUFFER) {
		buf=new drawbuf(&size);
		enable_buffering();
	} else {
		buf=screen;
		disable_buffering();
	}

	g_hdist=0;
	g_vdist=0;

	setbounds(b);

	pal.data=0;
	pal.size=0;

	pal_class=defaultpaletteclass();
	pal_name=defaultpalette();

	reloadpalette();
}

void ht_view::done()
{
	if (desc) free(desc);
	if (pal.data) free(pal.data);
	if (options & VO_OWNBUFFER) delete buf;
	Object::done();
}

int ht_view::alone()
{
	return (group && group->isalone(this));
}

int ht_view::buf_lprint(int x, int y, int c, int l, char *text)
{
	if ((size.y+y>=vsize.y) && (size.y+y<vsize.y+vsize.h)) {
		if (size.x+x+l>vsize.x+vsize.w) l=vsize.x+vsize.w-size.x-x;
		if (size.x+x-vsize.x<0) {
			int kqx=-size.x-x+vsize.x;
			for (int i=0; i<kqx; i++) {
				if (!*text) return 0;
				text++;
				x++;
				l--;
			}
		}
		return (l>0) ? buf->b_lprint(size.x+x, size.y+y, c, l, text) : 0;
	}
	return 0;
}

int ht_view::buf_lprintw(int x, int y, int c, int l, int *text)
{
	if ((size.y+y>=vsize.y) && (size.y+y<vsize.y+vsize.h)) {
		if (size.x+x+l>vsize.x+vsize.w) l=vsize.x+vsize.w-size.x-x;
		if (size.x+x-vsize.x<0) {
			int kqx=-size.x-x+vsize.x;
			for (int i=0; i<kqx; i++) {
				if (!*text) return 0;
				text++;
				x++;
				l--;
			}
		}
		return (l>0) ? buf->b_lprintw(size.x+x, size.y+y, c, l, text) : 0;
	}
	return 0;
}

int ht_view::buf_print(int x, int y, int c, char *text)
{
	if ((size.y+y>=vsize.y) && (size.y+y<vsize.y+vsize.h)) {
		int l=vsize.x+vsize.w-x-size.x;
		if (size.x+x-vsize.x<0) {
			int kqx=-size.x-x+vsize.x;
			for (int i=0; i<kqx; i++) {
				if (!*text) return 0;
				text++;
				x++;
				l--;
			}
		}
		return (l>0) ? buf->b_lprint(size.x+x, size.y+y, c, l, text) : 0;
	}
	return 0;
}

void ht_view::buf_printchar(int x, int y, int c, int ch)
{
	if (pointvisible(size.x+x, size.y+y)) buf->b_printchar(size.x+x, size.y+y, c, ch);
}

int ht_view::buf_printf(int x, int y, int c, char *format, ...)
{
	char buf[256];	/* secure */
	va_list arg;
	va_start(arg, format);
	ht_vsnprintf(buf, sizeof buf, format, arg);
	va_end(arg);
	return buf_print(x, y, c, buf);
}

int ht_view::buf_printw(int x, int y, int c, int *text)
{
	if ((size.y+y>=vsize.y) && (size.y+y<vsize.y+vsize.h)) {
		int l=vsize.x+vsize.w-x-size.x;
		if (size.x+x-vsize.x<0) {
			int kqx=-size.x-x+vsize.x;
			for (int i=0; i<kqx; i++) {
				if (!*text) return 0;
				text++;
				x++;
				l--;
			}
		}
		return (l>0) ? buf->b_lprintw(size.x+x, size.y+y, c, l, text) : 0;
	}
	return 0;
}

int ht_view::childcount()
{
	return 1;
}

void ht_view::cleanview()
{
	view_is_dirty=0;
}

void ht_view::clear(int c)
{
	buf->b_fill(vsize.x, vsize.y, vsize.w, vsize.h, c, ' ');
}

void ht_view::clipbounds(bounds *b)
{
	bounds c;
	getbounds(&c);
	bounds_and(b, &c);
	bounds_and(b, &vsize);
}

void ht_view::config_changed()
{
	reloadpalette();
	dirtyview();
}

int ht_view::countselectables()
{
	return (options & VO_SELECTABLE) ? 1 : 0;
}

int ht_view::datasize()
{
	return 0;
}

char *ht_view::defaultpalette()
{
	return palkey_generic_window_default;
}

char *ht_view::defaultpaletteclass()
{
	return palclasskey_generic;
}

void ht_view::dirtyview()
{
	view_is_dirty=1;
}

void ht_view::disable()
{
	enabled=0;
}

void ht_view::disable_buffering()
{
	if (options & VO_OWNBUFFER) {
		if (buf) delete buf;
		buf=screen;
		setoptions(options&(~VO_OWNBUFFER));
	}
}

void ht_view::draw()
{
}

void ht_view::enable()
{
	enabled=1;
}

void ht_view::enable_buffering()
{
	if (!(options & VO_OWNBUFFER)) {
		buf=new drawbuf(&size);
		setoptions(options | VO_OWNBUFFER);
	}
}

bool view_line_exposed(ht_view *v, int y, int x1, int x2)
{
	ht_group *g=v->group;
	while (g) {
		if ((y>=g->size.y) && (y<g->size.y+g->size.h)) {
			if (x1<g->size.x) x1=g->size.x;
			if (x2>g->size.x+g->size.w) x2=g->size.x+g->size.w;
			ht_view *n=g->first;
			while (n && n!=v) n=n->next;
			if (n) {
				n=n->next;
				if (n)
				while (n) {
					if (!(n->options & VO_TRANSPARENT_CHARS)) {
						if ((y>=n->size.y) && (y<n->size.y+n->size.h)) {
							if (n->size.x<=x1) {
								if (n->size.x+n->size.w>=x2) {
									return 0;
								} else if (n->size.x+n->size.w>x1) {
									x1=n->size.x+n->size.w;
								}
							} else if (n->size.x<=x2) {
								if (n->size.x+n->size.w<x2) {
									if (!view_line_exposed(n, y, x1, n->size.x)) return 0;
									x1=n->size.x+n->size.w;
								} else {
									x2=n->size.x;
								}
							}
						}
					}
					n=n->next;
				}
			}
		} else break;
		v=g;
		g=g->group;
	}
	return 1;
}

int ht_view::enum_start()
{
	return 0;
}

ht_view *ht_view::enum_next(int *handle)
{
	return 0;
}

bool ht_view::exposed()
{
#if 1
	for (int y=0; y<size.h; y++) {
		if (view_line_exposed(this, size.y+y, size.x, size.x+size.w)) return 1;
	}
	return 0;
#else
	return 1;
#endif
}

void ht_view::fill(int x, int y, int w, int h, int c, int chr)
{
	bounds b;
	b.x=size.x+x;
	b.y=size.y+y;
	b.w=w;
	b.h=h;
	bounds_and(&b, &vsize);
	buf->b_fill(b.x, b.y, b.w, b.h, c, chr);
}

int ht_view::focus(ht_view *view)
{
	if (view==this) {
		if (!focused) receivefocus();
		return 1;
	}
	return 0;
}

void ht_view::getbounds(bounds *b)
{
	*b=size;
}

vcp ht_view::getcolor(UINT index)
{
	return getcolorv(&pal, index);
}

struct databufdup_s {
	ht_memmap_file *f;
	ht_object_stream_memmap *s;
};

void ht_view::databuf_freedup(void *handle)
{
	databufdup_s *s=(databufdup_s*)handle;
	
	s->s->done();
	delete s->s;
	
	s->f->done();
	delete s->f;

	free(s);
}

void ht_view::databuf_get(void *buf)
{
	ht_memmap_file *f=new ht_memmap_file();
	f->init((byte*)buf);
	
	ht_object_stream_memmap *s=new ht_object_stream_memmap();
	s->init(f, false);
	
	getdata(s);

	s->done();
	delete s;

	f->done();
	delete f;
}

void *ht_view::databuf_getdup(void *buf)
{
	ht_memmap_file *f=new ht_memmap_file();
	f->init((byte*)buf);
	
	ht_object_stream_memmap *s=new ht_object_stream_memmap();
	s->init(f, true);
	
	getdata(s);

	databufdup_s *q=(databufdup_s*)malloc(sizeof (databufdup_s));
	q->f=f;
	q->s=s;
	return q;
}

void ht_view::databuf_set(void *buf)
{
	ht_memmap_file *f=new ht_memmap_file();
	f->init((byte*)buf);
	
	ht_object_stream_memmap *s=new ht_object_stream_memmap();
	s->init(f, false);
	
	setdata(s);
	
	s->done();
	delete s;
	
	f->done();
	delete f;
}

void ht_view::getdata(ht_object_stream *s)
{
}

ht_view *ht_view::getfirstchild()
{
	return 0;
}

UINT ht_view::getnumber()
{
	return 0;
}

char *ht_view::getpalette()
{
	return pal_name;
}

ht_view *ht_view::getselected()
{
	return this;
}

void ht_view::handlemsg(htmsg *msg)
{
	switch (msg->msg) {
		case msg_draw:
			redraw();
			return;
		case msg_dirtyview:
			dirtyview();
			if (msg->type & mt_broadcast==0) clearmsg(msg);
			return;
		case msg_config_changed:
			config_changed();
//          	clearmsg(msg);
			return;
	}
}

void ht_view::hidecursor()
{
	screen->hidecursor();
}

int ht_view::isalone(ht_view *view)
{
	return (view==this) && (countselectables()==1);
}

int ht_view::isviewdirty()
{
	return view_is_dirty;
}

int ht_view::load(ht_object_stream *s)
{
/*     s->get_bool(enabled, NULL);
	s->get_bool(focused, NULL);
	s->get_int_dec(options, 4, NULL);
	s->get_int_dec(browse_idx, 4, NULL);
	s->get_string(desc, NULL);
	get_bounds(s, &size);
	get_bounds(s, &vsize);
	s->get_string(pal_class, NULL);
	s->get_string(pal_name, NULL);
	s->get_int_dec(growmode, 4, NULL);*/
	return 1;
}

void ht_view::move(int rx, int ry)
{
	size.x+=rx;
	size.y+=ry;
	buf->b_rmove(rx, ry);
	vsize=size;
	if (group) group->clipbounds(&vsize);
	app->clipbounds(&vsize);
}

OBJECT_ID ht_view::object_id()
{
	return ATOM_HT_VIEW;
}

int ht_view::pointvisible(int x, int y)
{
	return ((x>=vsize.x) && (y>=vsize.y) && (x<vsize.x+vsize.w) && (y<vsize.y+vsize.h));
}

void ht_view::receivefocus()
{
	dirtyview();
	focused=1;
}

void ht_view::redraw()
{
	if (exposed()) {
		if (options & VO_OWNBUFFER) {
			if (isviewdirty()) {
				draw();
				cleanview();
			}
			screen->drawbuffer((drawbuf*)buf, size.x, size.y, &vsize);
		} else {
			draw();
			cleanview();
		}
	}
}

void ht_view::resize(int rw, int rh)
{
	if (options & VO_RESIZE) {
		if (size.w+rw-1<0) rw=-size.w+1;
		if (size.h+rh-1<0) rh=-size.h+1;
		size.w+=rw;
		size.h+=rh;
		buf->b_resize(rw, rh);
	}
	vsize=size;
	if (group) group->clipbounds(&vsize);
	app->clipbounds(&vsize);
}

void ht_view::resize_group(int rx, int ry)
{
	int px=0, py=0;
	int sx=0, sy=0;
	if (growmode & GM_HDEFORM) {
		sx=rx;
	} else if (growmode & GM_RIGHT) {
		px=rx;
	} else /* GM_LEFT */ {
	}
	if (growmode & GM_VDEFORM) {
		sy=ry;
	} else if (growmode & GM_BOTTOM) {
		py=ry;
	} else /* GM_TOP */ {
	}
	move(px, py);
	resize(sx, sy);
}

void ht_view::releasefocus()
{
	dirtyview();
	hidecursor();
	focused=0;
}

void ht_view::reloadpalette()
{
	if (pal.data) {
		free(pal.data);
		pal.data=0;
	}	    
	load_pal(pal_class, pal_name, &pal);
}

void ht_view::relocate_to(ht_view *view)
{
	bounds b;
	view->getbounds(&b);
	move(b.x, b.y);
}

int ht_view::select(ht_view *view)
{
	return (view==this);
}

void ht_view::selectfirst()
{
}

void ht_view::selectlast()
{
}

void ht_view::sendmsg(htmsg *msg)
{
	if (enabled) handlemsg(msg);
}

void ht_view::sendmsg(int msg, void *data1, void *data2)
{
	htmsg m;
	m.msg=msg;
	m.type=mt_empty;
	m.data1.ptr=data1;
	m.data2.ptr=data2;
	sendmsg(&m);
}

void ht_view::sendmsg(int msg, int data1, int data2)
{
	htmsg m;
	switch (msg) {
		case msg_empty:
			return;
		case msg_draw:
		case msg_dirtyview:
			m.msg=msg;
			m.type=mt_broadcast;
			m.data1.integer=data1;
			m.data2.integer=data2;
			break;
		default:
			m.msg=msg;
			m.type=mt_empty;
			m.data1.integer=data1;
			m.data2.integer=data2;
			break;
	}
	sendmsg(&m);
}

void ht_view::setbounds(bounds *b)
{
	size=*b;
	setvisualbounds(&size);
}

void ht_view::setvisualbounds(bounds *b)
{
	vsize=*b;
	if (options & VO_OWNBUFFER) {
		buf->b_setbounds(b);
	}
}

void ht_view::setcursor(int x, int y, cursor_mode c)
{
	if (pointvisible(size.x+x, size.y+y)) {
		screen->setcursor(size.x+x, size.y+y);
		switch (c) {
			case cm_normal:
				screen->setcursormode(0);
				break;
			case cm_overwrite:
				screen->setcursormode(1);
				break;
		}
	} else {
		screen->hidecursor();
	}
}

void ht_view::setdata(ht_object_stream *s)
{
}

void ht_view::setgroup(ht_group *_group)
{
	group=_group;
}

void ht_view::setnumber(UINT number)
{
}

void ht_view::setoptions(int Options)
{
	options = Options;
}

void ht_view::setpalette(char *Pal_name)
{
	pal_name = Pal_name;
	reloadpalette();
}

void ht_view::setpalettefull(char *_pal_name, char *_pal_class)
{
	pal_class=_pal_class;
	setpalette(pal_name);
}

void	ht_view::store(ht_object_stream *s)
{
	s->putBool(enabled, NULL);
	s->putBool(focused, NULL);
	s->putIntDec(options, 4, NULL);
	s->putIntDec(browse_idx, 4, NULL);
	s->putString(desc, NULL);
	put_bounds(s, &size);
	put_bounds(s, &vsize);
	s->putString(pal_class, NULL);
	s->putString(pal_name, NULL);
	s->putIntDec(growmode, 4, NULL);
}

void ht_view::unrelocate_to(ht_view *view)
{
	bounds b;
	view->getbounds(&b);
	b.x=-b.x;
	b.y=-b.y;
	move(b.x, b.y);
}

/*
 *	CLASS ht_group
 */

void ht_group::init(bounds *b, int options, char *desc)
{
	first=0;
	current=0;
	last=0;
	ht_view::init(b, options, desc);
	VIEW_DEBUG_NAME("ht_group");
	view_count=0;
	shared_data=0;

	growmode = GM_HDEFORM | GM_VDEFORM;

/* FIXME: debug */
	if (options & VO_OWNBUFFER) {
		HT_WARN("class 'ht_group' should not have own buffer !");
	}
/**/
}

void ht_group::done()
{
	ht_view *a, *b;
	a=first;
	while (a) {
		b=a->next;
		a->done();
		delete a;
		a=b;
	}
	ht_view::done();
}

int ht_group::childcount()
{
	return view_count;
}

int ht_group::countselectables()
{
	int c=0;
	ht_view *v=first;
	while (v) {
		c+=v->countselectables();
		v=v->next;
	}
	return c;
}

int ht_group::datasize()
{
	UINT size=0;
	ht_view *v=first;
	while (v) {
		size+=v->datasize();
		v=v->next;
	}
	return size;
}

int ht_group::enum_start()
{
	return -1;
}

ht_view *ht_group::enum_next(int *handle)
{
	int lowest=0x7fffffff;
	ht_view *view=0;

	ht_view *v=first;
	while (v) {
		if ((v->browse_idx > *handle) && (v->browse_idx < lowest)) {
			lowest=v->browse_idx;
			view=v;
		}
		v=v->next;
	}
	*handle=lowest;
	return view;
}

int ht_group::focus(ht_view *view)
{
	ht_view *v=first;
	while (v) {
		if (v->focus(view)) {
			releasefocus();
			current=v;
			putontop(v);
			receivefocus();
			return 1;
		}
		v=v->next;
	}
	return ht_view::focus(view);
}

int ht_group::focusnext()
{
	int i=current->browse_idx;
	int r=(options & VO_SELBOUND);
	ht_view *x=NULL;
	while (1) {
		i++;
		if (i>view_count-1) i=0;
		if (i==current->browse_idx) break;
		ht_view *v=get_by_browse_idx(i);
		if (v && (v->options & VO_SELECTABLE)) {
			x=v;
			break;
		}
	}
	if ((i < current->browse_idx) && !alone() && !r) {
		return 0;
	}
	if (x) {
		x->selectfirst();
		focus(x);
		return 1;
	}
	return r;
}

int ht_group::focusprev()
{
	int i=current->browse_idx;
	int r=(options & VO_SELBOUND);
	if (!i && !alone() && !r) {
		return 0;
	}
	while (1) {
		i--;
		if (i<0) i=view_count-1;
		if (i==current->browse_idx) break;
		ht_view *v=get_by_browse_idx(i);
		if (v && (v->options & VO_SELECTABLE)) {
			v->selectlast();
			focus(v);
			return 1;
		}
	}
	return r;
}

ht_view *ht_group::get_by_browse_idx(int i)
{
	ht_view *v=first;
	while (v) {
		if (v->browse_idx==i) return v;
		v=v->next;
	}
	return 0;
}

void ht_group::getdata(ht_object_stream *s)
{
	ht_view *v;
	int h=enum_start();
	while ((v=enum_next(&h))) {
		v->getdata(s);
	}
}

ht_view *ht_group::getselected()
{
	if (current) return current->getselected(); else return 0;
}

ht_view *ht_group::getfirstchild()
{
	return first;
}

void ht_group::handlemsg(htmsg *msg)
{
	if (!enabled) return;
	if (msg->type==mt_broadcast) {
		ht_view::handlemsg(msg);
		ht_view *v=first;
		while (v) {
			v->handlemsg(msg);
			v=v->next;
		}
	} else if (msg->type==mt_empty) {
		int msgtype=msg->type;
		ht_view *v;

		msg->type=mt_preprocess;
		v=first;
		while (v) {
			if (v->options & VO_PREPROCESS) {
				v->handlemsg(msg);
			}
			v=v->next;
		}

		msg->type=mt_empty;
		if (current) current->handlemsg(msg);

		msg->type=mt_postprocess;
		v=first;
		while (v) {
			if (v->options & VO_POSTPROCESS) {
				v->handlemsg(msg);
			}
			v=v->next;
		}

		msg->type=msgtype;
		ht_view::handlemsg(msg);
	} else if (msg->type==mt_preprocess) {
		ht_view *v;

		v=first;
		while (v) {
			if (v->options & VO_PREPROCESS) {
				v->handlemsg(msg);
			}
			v=v->next;
		}
	} else if (msg->type==mt_postprocess) {
		ht_view *v;

		v=first;
		while (v) {
			if (v->options & VO_POSTPROCESS) {
				v->handlemsg(msg);
			}
			v=v->next;
		}
	}

	if (((msg->type==mt_empty) || (msg->type==mt_broadcast)) && (msg->msg==msg_keypressed)) {
		switch (msg->data1.integer) {
			case K_Left:
			case K_BackTab: {
				if (focusprev()) {
					clearmsg(msg);
					dirtyview();
					return;
				}
				break;
			}
			case K_Right:
			case K_Tab: {
				if (focusnext()) {
					clearmsg(msg);
					dirtyview();
					return;
				}
				break;
			}
		}
	}
}

void ht_group::insert(ht_view *view)
{
	if (current) current->releasefocus();
	if (view->options & VO_PREPROCESS) setoptions(options | VO_PREPROCESS);
	if (view->options & VO_POSTPROCESS) setoptions(options | VO_POSTPROCESS);
	if (view->pal_class && pal_class && strcmp(view->pal_class, pal_class)==0) view->setpalette(pal_name);

	view->g_hdist=size.w - (view->size.x+view->size.w);
	view->g_vdist=size.h - (view->size.y+view->size.h);

	bounds c;
	getbounds(&c);
	view->move(c.x, c.y);

	if (last) last->next=view;
	view->prev=last;
	view->next=0;
	last=view;
	if (!first) first=view;
	view->setgroup(this);
	view->browse_idx=view_count++;
	if ((!current) || ((current) && (!(current->options & VO_SELECTABLE)) && (view->options & VO_SELECTABLE))) {
		current=view;
	}
	if ((current) && (current->options & VO_SELECTABLE)) {
		if (focused) {
			focus(current);
		} else {
			select(current);
		}
	}
}

int ht_group::isalone(ht_view *view)
{
	ht_view *v=first;
	while (v) {
		if ((v!=view) && (v->countselectables())) return 0;
		v=v->next;
	}
	return 1;
}

int ht_group::isviewdirty()
{
	ht_view *v=first;
	while (v) {
		if (v->isviewdirty()) return 1;
		v=v->next;
	}
	return 0;
}

int ht_group::load(ht_object_stream *f)
{
	return 1;
}

void ht_group::move(int rx, int ry)
{
	ht_view::move(rx, ry);
	ht_view *v=first;
	while (v) {
		v->move(rx, ry);
		v=v->next;
	}
}

OBJECT_ID ht_group::object_id()
{
	return ATOM_HT_GROUP;
}

void ht_group::putontop(ht_view *view)
{
	if (view->next) {
		if (view->prev) view->prev->next=view->next; else first=view->next;
		view->next->prev=view->prev;
		view->prev=last;
		view->next=0;
		last->next=view;
		last=view;
	}
}

void ht_group::receivefocus()
{
	ht_view::receivefocus();
	if (current) current->receivefocus();
}

void ht_group::releasefocus()
{
	ht_view::releasefocus();
	if (current)
		current->releasefocus();
}

void ht_group::remove(ht_view *view)
{
	ht_view *n=view->next ? view->next : view->prev;
	if (n) focus(n); else {
		releasefocus();
		current=0;
	}
	
	bounds c;
	getbounds(&c);
	view->move(-c.x, -c.y);
	
	if (view->prev) view->prev->next=view->next;
	if (view->next) view->next->prev=view->prev;
	if (first==view) first=first->next;
	if (last==view) last=last->prev;
}

void ht_group::resize(int rx, int ry)
{
	ht_view::resize(rx, ry);
	
	ht_view *v=first;
	while (v) {
		v->resize_group(rx, ry);
		v=v->next;
	}
}

int ht_group::select(ht_view *view)
{
	ht_view *v=first;
	while (v) {
		if (v->select(view)) {
			current=v;
			putontop(v);
			return 1;
		}
		v=v->next;
	}
	return ht_view::select(view);
}

void ht_group::selectfirst()
{
	for (int i=0; i<view_count; i++) {
		ht_view *v=first;
		while (v) {
			if ((v->browse_idx==i) && (v->options & VO_SELECTABLE)) {
				select(v);
				return;
			}
			v=v->next;
		}
	}
}

void ht_group::selectlast()
{
	for (int i=view_count-1; i>=0; i--) {
		ht_view *v=first;
		while (v) {
			if ((v->browse_idx==i) && (v->options & VO_SELECTABLE)) {
				select(v);
				return;
			}
			v=v->next;
		}
	}
}

void ht_group::setdata(ht_object_stream *s)
{
	ht_view *v;
	int h=enum_start();
	while ((v=enum_next(&h))) {
		v->setdata(s);
	}
}

void ht_group::setpalette(char *pal_name)
{
	ht_view *v=first;
	while (v) {
		if (strcmp(pal_class, v->pal_class)==0) v->setpalette(pal_name);
		v=v->next;
	}
	ht_view::setpalette(pal_name);
}

void ht_group::store(ht_object_stream *s)
{
	ht_view::store(s);
	s->putIntDec(childcount(), 4, NULL);
	ht_view *v=first;
	while (v) {
		s->putObject(v, NULL);
		v=v->next;
	}
}

/*
 *	CLASS ht_xgroup
 */

void ht_xgroup::init(bounds *b, int options, char *desc)
{
	ht_group::init(b, options, desc);
	VIEW_DEBUG_NAME("ht_xgroup");
	first=0;
	current=0;
	last=0;
}

void ht_xgroup::done()
{
	ht_group::done();
}

int ht_xgroup::countselectables()
{
	return current->countselectables();
}

void ht_xgroup::handlemsg(htmsg *msg)
{
	if ((msg->msg!=msg_draw) && (msg->type==mt_broadcast)) {
		ht_group::handlemsg(msg);
	} else {
		if (msg->msg==msg_complete_init) return;
		if (current) current->handlemsg(msg);
		ht_view::handlemsg(msg);
	}
}

int ht_xgroup::isalone(ht_view *view)
{
	if (group) return group->isalone(this);
	return 0;
}

int ht_xgroup::load(ht_object_stream *s)
{
	return ht_group::load(s);
}

OBJECT_ID ht_xgroup::object_id()
{
	return ATOM_HT_XGROUP;
}

void ht_xgroup::redraw()
{
	ht_view::redraw();
/* no broadcasts. */
	if (current) current->redraw();
}

void ht_xgroup::selectfirst()
{
	current->selectfirst();
}

void ht_xgroup::selectlast()
{
	current->selectlast();
}

void	ht_xgroup::store(ht_object_stream *s)
{
	ht_group::store(s);
}

/*
 *	CLASS ht_scrollbar
 */

bool scrollbar_pos(int start, int size, int all, int *pstart, int *psize)
{
	if (!all) return false;
	if (start+size>=all) {
		if (size>=all) return false;
		*psize=size*100/all;
		*pstart=100-*psize;
	} else {
		*psize=size*100/all;
		*pstart=start*100/all;
	}
	return true;
}

void	ht_scrollbar::init(bounds *b, palette *p, bool isv)
{
	ht_view::init(b, VO_RESIZE, 0);
	VIEW_DEBUG_NAME("ht_scrollbar");
	
	pstart=0;
	psize=0;

	gpal=p;

	isvertical=isv;

	if (isvertical) {
		growmode=GM_BOTTOM | GM_HDEFORM;
	} else {
		growmode=GM_RIGHT | GM_VDEFORM;
	}

	enable();	// enabled by default
}

void ht_scrollbar::done()
{
	ht_view::done();
}

void ht_scrollbar::enable()
{
	enable_buffering();
	ht_view::enable();
	dirtyview();
}

void ht_scrollbar::disable()
{
	disable_buffering();
	ht_view::disable();
	dirtyview();
}

void ht_scrollbar::draw()
{
	if (enabled) {
		vcp color = getcolorv(gpal, palidx_generic_scrollbar);
//          vcp color2 = VCP(VCP_BACKGROUND(color), VCP_FOREGROUND(color));
		if (isvertical) {
		} else {
			buf_printchar(0, 0, color, CHAR_ARROWBIG_UP);
			fill(0, 1, size.w, size.h-2, color, ' ');
			buf_printchar(0, size.h-1, color, CHAR_ARROWBIG_DOWN);
			int e, s;
			e=((size.h-2)*psize)/100;
			if (pstart+psize>=100) {
				s=size.h-2-e;
			} else {
				s=((size.h-2)*pstart)/100;
			}
			if (!e) {
				if (s==size.h-2) s--;
				e=1;
			}
			fill(0, s+1, 1, e, color, CHAR_FILLED_M);
		}
	}
}

int ht_scrollbar::load(ht_object_stream *s)
{
	return 1;
}

OBJECT_ID ht_scrollbar::object_id()
{
	return ATOM_HT_SCROLLBAR;
}

void ht_scrollbar::setpos(int ps, int pz)
{
	pstart=ps;
	psize=pz;
	dirtyview();
}

void	ht_scrollbar::store(ht_object_stream *s)
{
}

/*
 *	CLASS ht_frame
 */

void ht_frame::init(bounds *b, char *desc, UINT _style, UINT _number)
{
	ht_view::init(b, VO_RESIZE, desc);
	VIEW_DEBUG_NAME("ht_frame");

	number=_number;
	style=_style;
	framestate=FST_UNFOCUSED;
	
	growmode=GM_HDEFORM | GM_VDEFORM;
}

void ht_frame::done()
{
	ht_view::done();
}

void ht_frame::draw()
{
	int cornerul, cornerur, cornerll, cornerlr;
	int lineh, linev;
	ht_window *w=(ht_window*)group;
	if ((framestate!=FST_MOVE) && (framestate!=FST_RESIZE)) {
		if (w->focused) setframestate(FST_FOCUSED); else
			setframestate(FST_UNFOCUSED);
	}
	if (style & FS_THICK) {
		cornerul=CHAR_CORNERUL_DBL;
		cornerur=CHAR_CORNERUR_DBL;
		cornerll=CHAR_CORNERLL_DBL;
		cornerlr=CHAR_CORNERLR_DBL;
		lineh=CHAR_LINEH_DBL;
		linev=CHAR_LINEV_DBL;
	} else {
		cornerul=CHAR_CORNERUL;
		cornerur=CHAR_CORNERUR;
		cornerll=CHAR_CORNERLL;
		cornerlr=CHAR_CORNERLR;
		lineh=CHAR_LINEH;
		linev=CHAR_LINEV;
	}
	
	vcp c=getcurcol_normal();
/* "���...�Ŀ" */
	buf_printchar(0, 0, c, cornerul);
	for (int i=1; i<size.w-1; i++) buf_printchar(i, 0, c, lineh);
	buf_printchar(0+size.w-1, 0, c, cornerur);
/* "���...���" */
	buf_printchar(0, size.h-1, c, cornerll);
	for (int i=1; i<size.w-1; i++) buf_printchar(i, size.h-1, c, lineh);
/*	if (style & FS_RESIZE) {
		buf_printchar(size.w-1, size.h-1, getcurcol_killer(), CHAR_CORNERLR);
	} else {*/
		buf_printchar(size.w-1, size.h-1, c, cornerlr);
//     }
/* "�", "�" */
	for (int i=1; i<size.h-1; i++) {
		buf_printchar(0, i, c, linev);
		buf_printchar(size.w-1, i, c, linev);
	}
/* "[x]" */
	if (style & FS_KILLER) {
		buf_print(2, 0, c, "[ ]");
		buf_printchar(3, 0, getcurcol_killer(), CHAR_QUAD_SMALL);
	}
/* e.g. "1" */
	int ns=0;
	if (style & FS_NUMBER) {
		int l=number;
		do {
			l=l/10;
			ns++;
		} while (l);
		buf_printf(size.w-4-ns, 0, c, "%d", number);
		ns+=4;
	}
/* <title> */
	char *d;
	switch (framestate) {
		case FST_MOVE:
			d = "cursor keys move, space for resize mode";
			break;
		case FST_RESIZE:
			d = "cursor keys resize, space for move mode";
			break;
		default:
			d = desc;
	}
	int ks = (style & FS_KILLER) ? 4 : 0;
	ns++;
	if ((d) && (style & FS_TITLE)) {
		int l = strlen(d), k = 0;
		if (l > size.w-(5+ks+ns)) {
			k = l-(size.w-(6+ks+ns+2));
			if (size.w > 6+ks+ns+2) {
				d+=k;
			} else d="";
			buf_printf(2+ks, 0, c, " ...%s ",  d);
		} else {
			buf_printf((size.w-l-2)/2, 0, c, " %s ", d);
		}
	}
}

vcp ht_frame::getcurcol_normal()
{
	switch (framestate) {
		case FST_FOCUSED:
			return getcolor(palidx_generic_frame_focused);
		case FST_UNFOCUSED:
			return getcolor(palidx_generic_frame_unfocused);
		case FST_MOVE:
		case FST_RESIZE:
			return getcolor(palidx_generic_frame_move_resize);
	}
	return 0;
}

vcp ht_frame::getcurcol_killer()
{
	return getcolor(palidx_generic_frame_killer);
}

UINT ht_frame::getnumber()
{
	return number;
}

UINT ht_frame::getstyle()
{
	return style;
}

int ht_frame::load(ht_object_stream *s)
{
	return ht_view::load(s);
}

OBJECT_ID ht_frame::object_id()
{
	return ATOM_HT_FRAME;
}

void ht_frame::setframestate(UINT _framestate)
{
	framestate=_framestate;
	dirtyview();
}

void ht_frame::setnumber(UINT _number)
{
	number=_number;
	dirtyview();
}

void ht_frame::setstyle(UINT s)
{
	style=s;
}

void ht_frame::settitle(char *title)
{
	if (desc) free(desc);
	desc=ht_strdup(title);
}

void ht_frame::store(ht_object_stream *s)
{
	ht_view::store(s);
}

/*
 *	CLASS ht_window
 */

void	ht_window::init(bounds *b, char *desc, UINT framestyle, UINT num)
{
	ht_group::init(b, VO_SELECTABLE | VO_SELBOUND | VO_BROWSABLE, desc);
	VIEW_DEBUG_NAME("ht_window");
	number=num;
	hscrollbar=NULL;
	vscrollbar=NULL;
	pindicator=NULL;
	bounds c=*b;
	c.x=0;
	c.y=0;
	frame=0;
	action_state=WAC_NORMAL;
	ht_frame *f=new ht_frame();
	f->init(&c, desc, framestyle, number);
	setframe(f);
}

void ht_window::done()
{
	pindicator=NULL;
	hscrollbar=NULL;
	vscrollbar=NULL;
	ht_group::done();
}

void ht_window::draw()
{
	vcp c=getcolor(palidx_generic_body);
	clear(c);
}

void ht_window::getclientarea(bounds *b)
{
	getbounds(b);
	if (frame) {
		b->x++;
		b->y++;
		b->w-=2;
		b->h-=2;
	}
}

UINT ht_window::getnumber()
{
	return number;
}

void ht_window::handlemsg(htmsg *msg)
{
	switch (msg->msg) {
		case msg_keypressed:
			if (action_state==WAC_MOVE) {
				if (options & VO_MOVE) {
					switch (msg->data1.integer) {
						case K_Up:
							if (size.y>group->size.y) move(0, -1);
							break;
						case K_Down:
							if (size.y<group->size.y+group->size.h-1) move(0, 1);
							break;
						case K_Left:
							if (size.x+size.w>group->size.x+1) move(-1, 0);
							break;
						case K_Right:
							if (size.x<group->size.x+group->size.w-1) move(1, 0);
							break;
						case K_Control_Up:
							if (size.y>group->size.y+5-1) move(0, -5); else
								move(0, group->size.y-size.y);
							break;
						case K_Control_Down:
							if (size.y<group->size.y+group->size.h-5) move(0, 5); else
								move(0, group->size.y+group->size.h-size.y-1);
							break;
						case K_Control_Left:
							if (size.x+size.w>group->size.x+5) move(-5, 0); else
								move(-(size.x+size.w)+group->size.x+1, 0);
							break;
						case K_Control_Right:
							if (size.x<group->size.x+group->size.w-5) move(5, 0); else
								move(group->size.x+group->size.w-size.x-1, 0);
							break;
					}
				}
			} else if (action_state==WAC_RESIZE) {
				if (options & VO_RESIZE) {
					switch (msg->data1.integer) {
						case K_Up:
							if (size.h>/*MIN(*/7/*, isize.h)*/) resize(0, -1);
							break;
						case K_Down:
							if (size.h<group->size.h) resize(0, 1);
							break;
						case K_Left:
							if ((size.x+size.w>1) && (size.w>/*MIN(*/25/*, isize.w)*/)) resize(-1, 0);
							break;
						case K_Right:
							if (size.w<group->size.w) resize(1, 0);
							break;
					}
				}
			} else {
				if (msg->data1.integer == K_Control_F5) {
					sendmsg(cmd_window_resizemove);
				}
				break;
			}
			switch (msg->data1.integer) {
				case K_Escape:
				case K_Return:
					sendmsg(cmd_window_resizemove);
					break;
				case K_Space:
				case K_Control_F5:
					sendmsg(cmd_window_switch_resizemove);
					break;
			}
			app->sendmsg(msg_dirtyview, 0);
			clearmsg(msg);
			return;
		case cmd_window_resizemove: {
			bool b = (action_state == WAC_NORMAL);
			do {
				if (!next_action_state()) break;
			} while (b == (action_state == WAC_NORMAL));
			dirtyview();
			clearmsg(msg);
			return;
		}
		case cmd_window_switch_resizemove:
			do {
				if (!next_action_state()) break;
			} while (action_state == WAC_NORMAL);
			dirtyview();
			clearmsg(msg);
			return;
	}
	ht_group::handlemsg(msg);
}

void ht_window::insert(ht_view *view)
{
	if (frame) view->move(1, 1);
	ht_group::insert(view);
}

int ht_window::load(ht_object_stream *s)
{
	if (ht_group::load(s)!=0) return 1;
	return s->get_error();
}

bool ht_window::next_action_state()
{
#define wstate_count 3
	int ass[wstate_count] = { WAC_NORMAL, WAC_MOVE, WAC_RESIZE };
	int fss[wstate_count] = { FST_FOCUSED, FST_MOVE, FST_RESIZE };
	for (int i=0; i < wstate_count; i++) {
		if (action_state == ass[i]) {
			int p = i;
			while (++p != i) {
				if (p > wstate_count-1) p = 0;
				bool allowed = true;
				switch (ass[p]) {
					case WAC_MOVE: allowed = ((options & VO_MOVE) != 0); break;
					case WAC_RESIZE: allowed = ((options & VO_RESIZE) != 0); break;
				}
				if (allowed) {
					action_state = ass[p];
					if (frame) frame->setframestate(fss[p]);
					return (p != i);
				}
			}
			return false;
		}
	}
	return false;
}

OBJECT_ID ht_window::object_id()
{
	return ATOM_HT_WINDOW;
}

void ht_window::receivefocus()
{
	htmsg m;
	m.msg = msg_contextmenuquery;
	m.type = mt_empty;
	sendmsg(&m);
	ht_menu *q = (ht_menu*)((ht_app*)app)->menu;
	if (m.msg == msg_retval) {
		ht_context_menu *n = (ht_context_menu*)m.data1.ptr;
		if (q) {
			if (!q->set_local_menu(n)) {
				n->done();
				delete n;
			}
			q->sendmsg(msg_dirtyview);
		} else {
			n->done();
			delete n;
		}
	}

	ht_group::receivefocus();
	if (frame) frame->setstyle(frame->getstyle() | FS_THICK);
}

void ht_window::redraw()
{
	htmsg m;
	
	char buf[256];
	buf[0]=0;
	
	m.msg=msg_get_scrollinfo;
	m.type=mt_empty;
	m.data1.integer=gsi_pindicator;
	m.data2.ptr=buf;
	sendmsg(&m);

	if (pindicator) pindicator->settext(buf);

	gsi_scrollbar_t p;

	p.pstart=0;
	p.psize=200;
	m.msg=msg_get_scrollinfo;
	m.type=mt_empty;
	m.data1.integer=gsi_hscrollbar;
	m.data2.ptr=&p;
	sendmsg(&m);

	if (hscrollbar) {
		if (p.psize>=100) {
			hscrollbar->disable();
		} else {
			hscrollbar->enable();
			hscrollbar->setpos(p.pstart, p.psize);
		}
	}

	p.pstart=0;
	p.psize=200;
	m.msg=msg_get_scrollinfo;
	m.type=mt_empty;
	m.data1.integer=gsi_vscrollbar;
	m.data2.ptr=&p;
	sendmsg(&m);

	if (vscrollbar) {
		if (p.psize>=100) {
			vscrollbar->disable();
		} else {
			vscrollbar->enable();
			vscrollbar->setpos(p.pstart, p.psize);
		}
	}

	ht_group::redraw();
}

void ht_window::releasefocus()
{
	ht_menu *q = (ht_menu*)((ht_app*)app)->menu;
	if (q) {
		q->delete_local_menu();
		q->sendmsg(msg_dirtyview);
	}

	if (frame) frame->setstyle(frame->getstyle() & (~FS_THICK));
	ht_group::releasefocus();
}

void ht_window::resize(int rw, int rh)
{
	ht_group::resize(rw, rh);
}

void ht_window::setframe(ht_frame *newframe)
{
	if (frame) {
		ht_group::remove(frame);
		frame->done();
		delete frame;
		frame=NULL;
	}
	if (newframe) {
		UINT style=newframe->getstyle();
		if (style & FS_MOVE) options|=VO_MOVE; else options&=~VO_MOVE;
		if (style & FS_RESIZE) options|=VO_RESIZE; else options&=~VO_RESIZE;
		insert(newframe);
	} else {
		options&=~VO_MOVE;
		options&=~VO_RESIZE;
	}
	frame=newframe;
}

void ht_window::setnumber(UINT _number)
{
	if (frame) frame->setnumber(_number);
	number=_number;
	dirtyview();
}

void ht_window::sethscrollbar(ht_scrollbar *s)
{
	if (hscrollbar) remove(hscrollbar);
	hscrollbar=s;
	insert(hscrollbar);
	putontop(hscrollbar);
}

void ht_window::setpindicator(ht_text *p)
{
	if (pindicator) remove(pindicator);
	pindicator=p;
	insert(pindicator);
	putontop(pindicator);
}

void ht_window::settitle(char *title)
{
	if (desc) free(desc);
	desc=ht_strdup(title);
	if (frame) frame->settitle(title);
}

void ht_window::setvscrollbar(ht_scrollbar *s)
{
	if (vscrollbar) remove(vscrollbar);
	vscrollbar=s;
	insert(vscrollbar);
	putontop(vscrollbar);
}

void	ht_window::store(ht_object_stream *s)
{
	ht_group::store(s);
}

/*
 *	CLASS ht_vbar
 */

void ht_vbar::draw()
{
	fill(0, 0, 1, size.h, getcolor(palidx_generic_body), CHAR_LINEV);
}

/*
 *	CLASS ht_hbar
 */

void ht_hbar::draw()
{
	fill(0, 0, size.w, 1, getcolor(palidx_generic_body), CHAR_LINEH);
}

/***/
BUILDER(ATOM_HT_VIEW, ht_view);
BUILDER(ATOM_HT_GROUP, ht_group);
BUILDER(ATOM_HT_XGROUP, ht_xgroup);
BUILDER(ATOM_HT_WINDOW, ht_window);
BUILDER(ATOM_HT_FRAME, ht_frame);
BUILDER(ATOM_HT_SCROLLBAR, ht_scrollbar);

/*
 *	INIT
 */

bool init_obj()
{
	REGISTER(ATOM_HT_VIEW, ht_view);
	REGISTER(ATOM_HT_GROUP, ht_group);
	REGISTER(ATOM_HT_XGROUP, ht_xgroup);
	REGISTER(ATOM_HT_WINDOW, ht_window);
	REGISTER(ATOM_HT_FRAME, ht_frame);
	REGISTER(ATOM_HT_SCROLLBAR, ht_scrollbar);
	return true;
}

/*
 *	DONE
 */

void done_obj()
{
	UNREGISTER(ATOM_HT_VIEW, ht_view);
	UNREGISTER(ATOM_HT_GROUP, ht_group);
	UNREGISTER(ATOM_HT_XGROUP, ht_xgroup);
	UNREGISTER(ATOM_HT_WINDOW, ht_window);
	UNREGISTER(ATOM_HT_FRAME, ht_frame);
	UNREGISTER(ATOM_HT_SCROLLBAR, ht_scrollbar);
}
