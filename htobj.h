/*
 *	HT Editor
 *	htobj.h
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

#ifndef __HTOBJ_H__
#define __HTOBJ_H__

class ht_view;
class ht_group;

#include "common.h"
#include "htcurses.h"

struct palette {
	UINT size;
	vcp *data;
};

/* messages */
#define msg_message				0x00000000
#define HT_MESSAGE(m)			(msg_message+(m))

#define msg_empty 				HT_MESSAGE(0)
#define msg_retval				HT_MESSAGE(1)
#define msg_draw 				HT_MESSAGE(2)
#define msg_resize 				HT_MESSAGE(3)
#define msg_keypressed 			HT_MESSAGE(4)
#define msg_kill				HT_MESSAGE(5)
#define msg_complete_init		HT_MESSAGE(6)
#define msg_funcexec			HT_MESSAGE(7)
#define msg_funcquery			HT_MESSAGE(8)
#define msg_menucapquery			HT_MESSAGE(9)
#define msg_menuquery			HT_MESSAGE(10)
#define msg_button_pressed		HT_MESSAGE(11)
#define msg_dirtyview			HT_MESSAGE(12)
#define msg_config_changed		HT_MESSAGE(13)
#define msg_accept_close			HT_MESSAGE(14)
#define msg_file_changed			HT_MESSAGE(15)
#define msg_get_scrollinfo		HT_MESSAGE(16)
#define msg_get_analyser			HT_MESSAGE(17)
#define msg_set_analyser			HT_MESSAGE(18)	// (Analyser *)
#define msg_postinit			HT_MESSAGE(19)
#define msg_contextmenuquery		HT_MESSAGE(20)
#define msg_project_changed		HT_MESSAGE(21)
#define msg_vstate_save			HT_MESSAGE(22)	// (Object *data, ht_view *)
#define msg_vstate_restore		HT_MESSAGE(23) // (Object *data)
#define msg_goto_offset			HT_MESSAGE(24) // (FILEOFS ofs)

#define msg_filesize_changed		HT_MESSAGE(100)

#define gsi_pindicator			1
#define gsi_hscrollbar			2
#define gsi_vscrollbar			3

struct gsi_scrollbar_t {
	int pstart;
	int psize;
};

/* message types */
#define mt_empty			0
#define mt_broadcast		1
#define mt_preprocess		2
#define mt_postprocess		3

/*
 *	CLASS ht_view
 */

/* options */
#define VO_OWNBUFFER		1
#define VO_BROWSABLE		2
#define VO_SELECTABLE		4
#define VO_SELBOUND			8
#define VO_PREPROCESS		16
#define VO_POSTPROCESS		32
#define VO_MOVE			64
#define VO_RESIZE			128
#define VO_FORMAT_VIEW		256
#define VO_TRANSPARENT_CHARS	512

/* grow modes */

class ht_group;

#define VIEW_DEBUG_NAME(name)	ht_view::view_debug_name=name;

#define	GM_TOP		0
#define	GM_BOTTOM		1
#define	GM_VDEFORM	2

#define	GM_LEFT		0
#define	GM_RIGHT		4
#define	GM_HDEFORM	8

void clearmsg(htmsg *msg);

enum cursor_mode {cm_normal, cm_overwrite};

class ht_view: public Object {
protected:
			bool view_is_dirty;
			
			void cleanview();
	virtual	char *defaultpalette();
	virtual	char *defaultpaletteclass();
	virtual	void reloadpalette();
public:
	bool focused;
	bool enabled;
	ht_group *group;
	int options;
	char *desc;
	int browse_idx;
	genericdrawbuf *buf;
	ht_view *prev, *next;

	bounds size;
	bounds vsize;	/* visual bounds */
	UINT growmode;
	UINT g_hdist, g_vdist;

	palette pal;
	char *pal_class;
	char *pal_name;

/*debug:*/char *view_debug_name;

			void	init(bounds *b, int options, char *desc);
	virtual	void done();
/* new */
			void *allocdatabuf(void *handle);
	virtual	int alone();
			int buf_lprint(int x, int y, int c, int l, char *text);
			int buf_lprintw(int x, int y, int c, int l, int *text);
			int buf_print(int x, int y, int c, char *text);
			void buf_printchar(int x, int y, int c, int ch);
			int buf_printf(int x, int y, int c, char *format, ...);
			int buf_printw(int x, int y, int c, int *text);
	virtual	int childcount();
			void clear(int color);
	virtual	void clipbounds(bounds *b);
	virtual	void config_changed();
	virtual	int countselectables();
			void databuf_freedup(void *handle);
			void databuf_get(void *buf);
			void *databuf_getdup(void *buf);
			void databuf_set(void *buf);
	virtual	int datasize();
			void dirtyview();
	virtual	void disable();
			void disable_buffering();
	virtual 	void draw();
	virtual	void enable();
			void enable_buffering();
	virtual	int enum_start();
	virtual	ht_view *enum_next(int *handle);
			bool exposed();
			void fill(int x, int y, int w, int h, int c, int chr);
	virtual	int focus(ht_view *view);
			void getbounds(bounds *b);
			vcp getcolor(UINT index);
	virtual	void getdata(ht_object_stream *s);
	virtual 	ht_view *getfirstchild();
	virtual	UINT getnumber();
			char *getpalette();
	virtual	ht_view *getselected();
	virtual 	void handlemsg(htmsg *msg);
			void hidecursor();
			int isviewdirty();
	virtual	int isalone(ht_view *view);
	virtual   int	load(ht_object_stream *s);
	virtual	void move(int rx, int ry);
	virtual   OBJECT_ID object_id();
			int pointvisible(int x, int y);
	virtual	void receivefocus();
	virtual	void redraw();
			void relocate_to(ht_view *view);
	virtual	void resize(int rw, int rh);
	virtual	void resize_group(int rx, int ry);
	virtual 	void releasefocus();
	virtual	int select(ht_view *view);
	virtual	void selectfirst();
	virtual	void selectlast();
			void sendmsg(htmsg *msg);
			void sendmsg(int msg, int data1=0, int data2=0);
			void sendmsg(int msg, void *data1, void *data2=0);
			void setbounds(bounds *b);
			void setvisualbounds(bounds *b);
			void setcursor(int x, int y, cursor_mode c=cm_normal);
	virtual	void setdata(ht_object_stream *s);
	virtual	void setgroup(ht_group *group);
	virtual	void setnumber(UINT number);
			void setoptions(int options);
	virtual	void setpalette(char *pal_name);
			void setpalettefull(char *pal_name, char *pal_class);
	virtual   void	store(ht_object_stream *s);
			void unrelocate_to(ht_view *view);
};

/*
 *	CLASS ht_group
 */

class ht_group: public ht_view {
protected:
	int 		view_count;

public:
	ht_view 	*first, *current, *last;
	void		*shared_data;

			void	init(bounds *b, int options, char *desc);
	virtual	void done();
/* overwritten */
	virtual	int childcount();
	virtual	int countselectables();
	virtual	int datasize();
	virtual	int enum_start();
	virtual	ht_view *enum_next(int *handle);
	virtual	int focus(ht_view *view);
	virtual	void getdata(ht_object_stream *s);
	virtual	ht_view *getselected();
	virtual 	ht_view *getfirstchild();
	virtual	void handlemsg(htmsg *msg);
	virtual	int isalone(ht_view *view);
			int isviewdirty();
	virtual	int load(ht_object_stream *s);
	virtual	void move(int x, int y);
	virtual   OBJECT_ID object_id();
			void putontop(ht_view *view);
	virtual 	void receivefocus();
	virtual	void resize(int rw, int rh);
	virtual 	void releasefocus();
	virtual	int select(ht_view *view);
	virtual	void selectfirst();
	virtual	void selectlast();
	virtual	void setdata(ht_object_stream *s);
	virtual	void setpalette(char *pal_name);
	virtual	void store(ht_object_stream *s);
/* new */
			void remove(ht_view *view);
	virtual	void insert(ht_view *view);
			int focusnext();
			int focusprev();
			ht_view *get_by_browse_idx(int i);
};

/*
 *	CLASS ht_xgroup
 */

class ht_xgroup: public ht_group {
public:
			void	init(bounds *b, int options, char *desc);
	virtual	void done();
/* overwritten */
	virtual	int countselectables();
	virtual	void handlemsg(htmsg *msg);
	virtual	int isalone(ht_view *view);
	virtual	int load(ht_object_stream *s);
	virtual   OBJECT_ID object_id();
	virtual	void redraw();
	virtual	void selectfirst();
	virtual	void selectlast();
	virtual	void store(ht_object_stream *s);
};

/*
 *	CLASS ht_scrollbar
 */

class ht_scrollbar: public ht_view {
protected:
	int pstart, psize;
	palette *gpal;
	bool isvertical;
public:
			void	init(bounds *b, palette *gpal, bool isvertical);
	virtual	void done();
/* overwritten */
	virtual	void enable();
	virtual	void disable();
	virtual 	void draw();
	virtual	int load(ht_object_stream *s);
	virtual   OBJECT_ID object_id();
	virtual	void store(ht_object_stream *s);
/* new */
	virtual	void setpos(int pstart, int psize);
};

/*
 *	CLASS ht_text
 */

class ht_text: public ht_view {
public:
/* new */
	virtual	void settext(char *text);
};

/*
 *	CLASS ht_frame
 */

#define FS_KILLER		1
#define FS_TITLE 		2
#define FS_NUMBER		4
#define FS_RESIZE		8
#define FS_MOVE		16
#define FS_THICK		32

#define FST_FOCUSED		0
#define FST_UNFOCUSED	1
#define FST_MOVE      	2
#define FST_RESIZE      	3

class ht_frame: public ht_view {
protected:
	UINT number;
	UINT style;
	UINT framestate;

/* new */
	virtual	vcp getcurcol_normal();
	virtual	vcp getcurcol_killer();
public:
			void	init(bounds *b, char *desc, UINT style, UINT number=0);
	virtual	void done();
/* overwritten */
	virtual	void draw();
	virtual	UINT getnumber();
	virtual	int load(ht_object_stream *s);
	virtual	OBJECT_ID object_id();
	virtual	void setnumber(UINT number);
	virtual	void store(ht_object_stream *s);
/* new */
			UINT getstyle();
			void setframestate(UINT framestate);
			void setstyle(UINT style);
			void settitle(char *title);
};

/*
 *	CLASS ht_window
 */

#define WAC_NORMAL	0
#define WAC_MOVE	1
#define WAC_RESIZE	2

class ht_window: public ht_group {
protected:
	ht_frame *frame;
	ht_scrollbar *hscrollbar;
	ht_scrollbar *vscrollbar;
	ht_text *pindicator;
	UINT number;

	int action_state;
	
			bool next_action_state();
public:
			void	init(bounds *b, char *desc, UINT framestyle, UINT number=0);
	virtual	void done();
/* overwritten */
	virtual	void draw();
	virtual	UINT getnumber();
	virtual	void handlemsg(htmsg *msg);
	virtual	void insert(ht_view *view);
	virtual	int load(ht_object_stream *s);
	virtual	OBJECT_ID object_id();
	virtual	void receivefocus();
	virtual	void releasefocus();
	virtual	void redraw();
	virtual	void resize(int rw, int rh);
	virtual	void setnumber(UINT number);
	virtual	void store(ht_object_stream *s);
/* new */
			void getclientarea(bounds *b);
			void setframe(ht_frame *frame);
			void sethscrollbar(ht_scrollbar *scrollbar);
			void setpindicator(ht_text *pindicator);
			void settitle(char *title);
			void setvscrollbar(ht_scrollbar *scrollbar);
};

bool scrollbar_pos(int start, int size, int all, int *pstart, int *psize);

/*
 *	CLASS ht_hbar
 */

class ht_hbar: public ht_view {
public:
/* overwritten */
	virtual	 void draw();
};

/*
 *	CLASS ht_vbar
 */

class ht_vbar: public ht_view {
public:
/* overwritten */
	virtual	 void draw();
};

/*
 *	INIT
 */

bool init_obj();

/*
 *	DONE
 */

void done_obj();

#endif /* !__HTOBJ_H__ */