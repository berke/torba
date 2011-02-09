/* $torba: torba.c,v 1.308 2011/01/18 19:43:12 marco Exp $ */
/*
 * Copyright (c) 2009-2010 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009 Ryan McBride <mcbride@countersiege.com>
 * Copyright (c) 2009 Darrin Chandler <dwchandler@stilyagin.com>
 * Copyright (c) 2009 Pierre-Yves Ritschard <pyr@spootnik.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Much code and ideas taken from dwm under the following license:
 * MIT/X Consortium License
 *
 * 2006-2008 Anselm R Garbe <garbeam at gmail dot com>
 * 2006-2007 Sander van Dijk <a dot h dot vandijk at gmail dot com>
 * 2006-2007 Jukka Salmi <jukka at salmi dot ch>
 * 2007 Premysl Hruby <dfenze at gmail dot com>
 * 2007 Szabolcs Nagy <nszabolcs at gmail dot com>
 * 2007 Christof Musik <christof at sendfax dot de>
 * 2007-2008 Enno Gottox Boland <gottox at s01 dot de>
 * 2007-2008 Peter Hartlich <sgkkr at hartlich dot com>
 * 2008 Martin Hurton <martin dot hurton at gmail dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

static const char	*cvstag = "$torba: torba.c,v 1.308 2011/01/18 19:43:12 marco Exp $";

#define	TWM_VERSION	"0.9.27"

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <util.h>
#include <pwd.h>
#include <paths.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/select.h>

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#ifdef __OSX__
#include <osx.h>
#endif

#if RANDR_MAJOR < 1
#  error XRandR versions less than 1.0 are not supported
#endif

#if RANDR_MAJOR >= 1
#if RANDR_MINOR >= 2
#define TWM_XRR_HAS_CRTC
#endif
#endif

/* #define TWM_DEBUG */
#ifdef TWM_DEBUG
#define DPRINTF(x...)		do { if (twm_debug) fprintf(stderr, x); } while (0)
#define DNPRINTF(n,x...)	do { if (twm_debug & n) fprintf(stderr, x); } while (0)
#define	TWM_D_MISC		0x0001
#define	TWM_D_EVENT		0x0002
#define	TWM_D_WS		0x0004
#define	TWM_D_FOCUS		0x0008
#define	TWM_D_MOVE		0x0010
#define	TWM_D_STACK		0x0020
#define	TWM_D_MOUSE		0x0040
#define	TWM_D_PROP		0x0080
#define	TWM_D_CLASS		0x0100
#define TWM_D_KEY		0x0200
#define TWM_D_QUIRK		0x0400
#define TWM_D_SPAWN		0x0800
#define TWM_D_EVENTQ		0x1000
#define TWM_D_CONF		0x2000

u_int32_t		twm_debug = 0
			    | TWM_D_MISC
			    | TWM_D_EVENT
			    | TWM_D_WS
			    | TWM_D_FOCUS
			    | TWM_D_MOVE
			    | TWM_D_STACK
			    | TWM_D_MOUSE
			    | TWM_D_PROP
			    | TWM_D_CLASS
			    | TWM_D_KEY
			    | TWM_D_QUIRK
			    | TWM_D_SPAWN
			    | TWM_D_EVENTQ
			    | TWM_D_CONF
			    ;
#else
#define DPRINTF(x...)
#define DNPRINTF(n,x...)
#endif

#define LENGTH(x)		(sizeof x / sizeof x[0])
#define MODKEY			Mod1Mask
#define CLEANMASK(mask)		(mask & ~(numlockmask | LockMask))
#define BUTTONMASK		(ButtonPressMask|ButtonReleaseMask)
#define MOUSEMASK		(BUTTONMASK|PointerMotionMask)
#define TWM_PROPLEN		(16)
#define TWM_FUNCNAME_LEN	(32)
#define TWM_KEYS_LEN		(255)
#define TWM_QUIRK_LEN		(64)
#define X(r)			(r)->g.x
#define Y(r)			(r)->g.y
#define WIDTH(r)		(r)->g.w
#define HEIGHT(r)		(r)->g.h
#define TWM_MAX_FONT_STEPS	(3)
#define WINID(w)		(w ? w->id : 0)

#define TWM_FOCUS_DEFAULT	(0)
#define TWM_FOCUS_SYNERGY	(1)
#define TWM_FOCUS_FOLLOW	(2)

#ifndef TWM_LIB
#define TWM_LIB			"/usr/local/lib/libtwmhack.so"
#endif

char			**start_argv;
Atom			astate;
Atom			aprot;
Atom			adelete;
Atom			takefocus;
volatile sig_atomic_t   running = 1;
volatile sig_atomic_t   restart_wm = 0;
int			outputs = 0;
int			last_focus_event = FocusOut;
int			(*xerrorxlib)(Display *, XErrorEvent *);
int			other_wm;
int			ss_enabled = 0;
int			xrandr_support;
int			xrandr_eventbase;
unsigned int		numlockmask = 0;
Display			*display;

int			cycle_empty = 0;
int			cycle_visible = 0;
int			term_width = 0;
int			font_adjusted = 0;
unsigned int		mod_key = MODKEY;

/* dialog windows */
double			dialog_ratio = .6;
/* status bar */
#define TWM_BAR_MAX	(256)
char			*bar_argv[] = { NULL, NULL };
int			bar_pipe[2];
char			bar_ext[TWM_BAR_MAX];
char			bar_vertext[TWM_BAR_MAX];
int			bar_version = 0;
sig_atomic_t		bar_alarm = 0;
int			bar_delay = 30;
int			bar_enabled = 1;
int			bar_at_bottom = 0;
int			bar_extra = 1;
int			bar_extra_running = 0;
int			bar_verbose = 1;
int			bar_height = 0;
int			border_width = 2;
int			stack_enabled = 1;
int			clock_enabled = 1;
char			*clock_format = NULL;
int			title_name_enabled = 0;
int			title_class_enabled = 0;
int			window_name_enabled = 0;
int			focus_mode = TWM_FOCUS_DEFAULT;
int			disable_border = 0;
pid_t			bar_pid;
GC			bar_gc;
XGCValues		bar_gcv;
int			bar_fidx = 0;
XFontStruct		*bar_fs;
char			*bar_fonts[] = { NULL, NULL, NULL, NULL };/* XXX Make fully dynamic */
char			*spawn_term[] = { NULL, NULL };		/* XXX Make fully dynamic */

#define TWM_MENU_FN	(2)
#define TWM_MENU_NB	(4)
#define TWM_MENU_NF	(6)
#define TWM_MENU_SB	(8)
#define TWM_MENU_SF	(10)

/* layout manager data */
struct twm_geometry {
	int			x;
	int			y;
	int			w;
	int			h;
};

struct twm_screen;
struct workspace;

/* virtual "screens" */
struct twm_region {
	TAILQ_ENTRY(twm_region)	entry;
	struct twm_geometry	g;
	struct workspace	*ws;	/* current workspace on this region */
	struct workspace	*ws_prior; /* prior workspace on this region */
	struct twm_screen	*s;	/* screen idx */
	Window			bar_window;
};
TAILQ_HEAD(twm_region_list, twm_region);

struct ws_win {
	TAILQ_ENTRY(ws_win)	entry;
	Window			id;
	Window			transient;
	struct ws_win		*child_trans;	/* transient child window */
	struct twm_geometry	g;		/* current geometry */
	struct twm_geometry	g_float;	/* geometry when floating */
	struct twm_geometry	rg_float;	/* region geom when floating */
	int			g_floatvalid;	/* flag: geometry in g_float is valid */
	int			floatmaxed;	/* flag: floater was maxed in max_stack */
	int			floating;
	int			manual;
	unsigned int		ewmh_flags;
	int			font_size_boundary[TWM_MAX_FONT_STEPS];
	int			font_steps;
	int			last_inc;
	int			can_delete;
	int			take_focus;
	int			java;
	unsigned long		quirks;
	struct workspace	*ws;	/* always valid */
	struct twm_screen	*s;	/* always valid, never changes */
	XWindowAttributes	wa;
	XSizeHints		sh;
	XClassHint		ch;
};
TAILQ_HEAD(ws_win_list, ws_win);

/* layout handlers */
void	stack(void);
void	vertical_config(struct workspace *, int);
void	vertical_stack(struct workspace *, struct twm_geometry *);
void	horizontal_config(struct workspace *, int);
void	horizontal_stack(struct workspace *, struct twm_geometry *);
void	max_stack(struct workspace *, struct twm_geometry *);

struct ws_win *find_window(Window);

void	grabbuttons(struct ws_win *, int);
void	new_region(struct twm_screen *, int, int, int, int);
void	unmanage_window(struct ws_win *);
long	getstate(Window);

struct layout {
	void		(*l_stack)(struct workspace *, struct twm_geometry *);
	void		(*l_config)(struct workspace *, int);
	u_int32_t	flags;
#define TWM_L_FOCUSPREV		(1<<0)
#define TWM_L_MAPONFOCUS	(1<<1)
	char		*name;
} layouts[] =  {
	/* stack,		configure */
	{ vertical_stack,	vertical_config,	0,	"[|]" },
	{ horizontal_stack,	horizontal_config,	0,	"[-]" },
	{ max_stack,		NULL,
	  TWM_L_MAPONFOCUS | TWM_L_FOCUSPREV,			"[ ]"},
	{ NULL,			NULL,			0,	NULL },
};

/* position of max_stack mode in the layouts array */
#define TWM_MAX_STACK		2

#define TWM_H_SLICE		(32)
#define TWM_V_SLICE		(32)

/* define work spaces */
struct workspace {
	int			idx;		/* workspace index */
	struct layout		*cur_layout;	/* current layout handlers */
	struct ws_win		*focus;		/* may be NULL */
	struct ws_win		*focus_prev;	/* may be NULL */
	struct twm_region	*r;		/* may be NULL */
	struct twm_region	*old_r;		/* may be NULL */
	struct ws_win_list	winlist;	/* list of windows in ws */
	struct ws_win_list	unmanagedlist;	/* list of dead windows in ws */

	/* stacker state */
	struct {
				int horizontal_msize;
				int horizontal_mwin;
				int horizontal_stacks;
				int vertical_msize;
				int vertical_mwin;
				int vertical_stacks;
	} l_state;
};

enum	{ TWM_S_COLOR_BAR, TWM_S_COLOR_BAR_BORDER, TWM_S_COLOR_BAR_FONT,
	  TWM_S_COLOR_FOCUS, TWM_S_COLOR_UNFOCUS, TWM_S_COLOR_MAX };

/* physical screen mapping */
#define TWM_WS_MAX		(10)
struct twm_screen {
	int			idx;	/* screen index */
	struct twm_region_list	rl;	/* list of regions on this screen */
	struct twm_region_list	orl;	/* list of old regions */
	Window			root;
	struct workspace	ws[TWM_WS_MAX];

	/* colors */
	struct {
		unsigned long	color;
		char		*name;
	} c[TWM_S_COLOR_MAX];
};
struct twm_screen	*screens;
int			num_screens;

/* args to functions */
union arg {
	int			id;
#define TWM_ARG_ID_FOCUSNEXT	(0)
#define TWM_ARG_ID_FOCUSPREV	(1)
#define TWM_ARG_ID_FOCUSMAIN	(2)
#define TWM_ARG_ID_FOCUSCUR	(4)
#define TWM_ARG_ID_SWAPNEXT	(10)
#define TWM_ARG_ID_SWAPPREV	(11)
#define TWM_ARG_ID_SWAPMAIN	(12)
#define TWM_ARG_ID_MOVELAST	(13)
#define TWM_ARG_ID_MASTERSHRINK (20)
#define TWM_ARG_ID_MASTERGROW	(21)
#define TWM_ARG_ID_MASTERADD	(22)
#define TWM_ARG_ID_MASTERDEL	(23)
#define TWM_ARG_ID_STACKRESET	(30)
#define TWM_ARG_ID_STACKINIT	(31)
#define TWM_ARG_ID_CYCLEWS_UP	(40)
#define TWM_ARG_ID_CYCLEWS_DOWN	(41)
#define TWM_ARG_ID_CYCLESC_UP	(42)
#define TWM_ARG_ID_CYCLESC_DOWN	(43)
#define TWM_ARG_ID_STACKINC	(50)
#define TWM_ARG_ID_STACKDEC	(51)
#define TWM_ARG_ID_SS_ALL	(60)
#define TWM_ARG_ID_SS_WINDOW	(61)
#define TWM_ARG_ID_DONTCENTER	(70)
#define TWM_ARG_ID_CENTER	(71)
#define TWM_ARG_ID_KILLWINDOW	(80)
#define TWM_ARG_ID_DELETEWINDOW	(81)
	char			**argv;
};

void	focus(struct twm_region *, union arg *);
void	focus_magic(struct ws_win *, int);
#define TWM_F_GENERIC		(0)
#define TWM_F_TRANSIENT		(1)
/* quirks */
struct quirk {
	char			*class;
	char			*name;
	unsigned long		quirk;
#define TWM_Q_FLOAT		(1<<0)	/* float this window */
#define TWM_Q_TRANSSZ		(1<<1)	/* transiend window size too small */
#define TWM_Q_ANYWHERE		(1<<2)	/* don't position this window */
#define TWM_Q_XTERM_FONTADJ	(1<<3)	/* adjust xterm fonts when resizing */
#define TWM_Q_FULLSCREEN	(1<<4)	/* remove border */
};
int				quirks_size = 0, quirks_length = 0;
struct quirk			*quirks = NULL;

/*
 * Supported EWMH hints should be added to
 * both the enum and the ewmh array
 */
enum { _NET_ACTIVE_WINDOW, _NET_MOVERESIZE_WINDOW, _NET_CLOSE_WINDOW,
    _NET_WM_WINDOW_TYPE, _NET_WM_WINDOW_TYPE_DOCK,
    _NET_WM_WINDOW_TYPE_TOOLBAR, _NET_WM_WINDOW_TYPE_UTILITY,
    _NET_WM_WINDOW_TYPE_SPLASH, _NET_WM_WINDOW_TYPE_DIALOG,
    _NET_WM_WINDOW_TYPE_NORMAL, _NET_WM_STATE,
    _NET_WM_STATE_MAXIMIZED_HORZ, _NET_WM_STATE_MAXIMIZED_VERT,
    _NET_WM_STATE_SKIP_TASKBAR, _NET_WM_STATE_SKIP_PAGER,
    _NET_WM_STATE_HIDDEN, _NET_WM_STATE_ABOVE, _TWM_WM_STATE_MANUAL,
    _NET_WM_STATE_FULLSCREEN, _NET_WM_ALLOWED_ACTIONS, _NET_WM_ACTION_MOVE,
    _NET_WM_ACTION_RESIZE, _NET_WM_ACTION_FULLSCREEN, _NET_WM_ACTION_CLOSE,
    TWM_EWMH_HINT_MAX };

struct ewmh_hint {
	char	*name;
	Atom	 atom;
} ewmh[TWM_EWMH_HINT_MAX] =	{
    /* must be in same order as in the enum */
    {"_NET_ACTIVE_WINDOW", None},
    {"_NET_MOVERESIZE_WINDOW", None},
    {"_NET_CLOSE_WINDOW", None},
    {"_NET_WM_WINDOW_TYPE", None},
    {"_NET_WM_WINDOW_TYPE_DOCK", None},
    {"_NET_WM_WINDOW_TYPE_TOOLBAR", None},
    {"_NET_WM_WINDOW_TYPE_UTILITY", None},
    {"_NET_WM_WINDOW_TYPE_SPLASH", None},
    {"_NET_WM_WINDOW_TYPE_DIALOG", None},
    {"_NET_WM_WINDOW_TYPE_NORMAL", None},
    {"_NET_WM_STATE", None},
    {"_NET_WM_STATE_MAXIMIZED_HORZ", None},
    {"_NET_WM_STATE_MAXIMIZED_VERT", None},
    {"_NET_WM_STATE_SKIP_TASKBAR", None},
    {"_NET_WM_STATE_SKIP_PAGER", None},
    {"_NET_WM_STATE_HIDDEN", None},
    {"_NET_WM_STATE_ABOVE", None},
    {"_TWM_WM_STATE_MANUAL", None},
    {"_NET_WM_STATE_FULLSCREEN", None},
    {"_NET_WM_ALLOWED_ACTIONS", None},
    {"_NET_WM_ACTION_MOVE", None},
    {"_NET_WM_ACTION_RESIZE", None},
    {"_NET_WM_ACTION_FULLSCREEN", None},
    {"_NET_WM_ACTION_CLOSE", None},
};

void	store_float_geom(struct ws_win *win, struct twm_region *r);
int	floating_toggle_win(struct ws_win *win);

int
get_property(Window id, Atom atom, long count, Atom type,
    unsigned long *n, unsigned char **data)
{
	int			format, status;
	unsigned long		tmp, extra;
	unsigned long		*nitems;
	Atom			real;

	nitems = n != NULL ? n : &tmp;
	status = XGetWindowProperty(display, id, atom, 0L, count, False, type,
	    &real, &format, nitems, &extra, data);

	if (status != Success)
		return False;
	if (real != type)
		return False;

	return True;
}

void
setup_ewmh(void)
{
	int			i,j;
	Atom			sup_list;

	sup_list = XInternAtom(display, "_NET_SUPPORTED", False);

	for (i = 0; i < LENGTH(ewmh); i++)
		ewmh[i].atom = XInternAtom(display, ewmh[i].name, False);

	for (i = 0; i < ScreenCount(display); i++) {
		/* Support check window will be created by workaround(). */

		/* Report supported atoms */
		XDeleteProperty(display, screens[i].root, sup_list);
		for (j = 0; j < LENGTH(ewmh); j++)
			XChangeProperty(display, screens[i].root, sup_list, XA_ATOM, 32,
			    PropModeAppend, (unsigned char *)&ewmh[j].atom,1);
	}
}

void
teardown_ewmh(void)
{
	int			i, success;
	unsigned char		*data = NULL;
	unsigned long		n;
	Atom			sup_check, sup_list;
	Window			id;

	sup_check = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
	sup_list = XInternAtom(display, "_NET_SUPPORTED", False);

	for (i = 0; i < ScreenCount(display); i++) {
		/* Get the support check window and destroy it */
		success = get_property(screens[i].root, sup_check, 1, XA_WINDOW,
		    &n, &data);

		if (success) {
			id = data[0];
			XDestroyWindow(display, id);
			XDeleteProperty(display, screens[i].root, sup_check);
			XDeleteProperty(display, screens[i].root, sup_list);
		}

		XFree(data);
	}
}

void
ewmh_autoquirk(struct ws_win *win)
{
	int			success, i;
	unsigned long		*data = NULL;
	unsigned long		n;
	Atom			type;

	success = get_property(win->id, ewmh[_NET_WM_WINDOW_TYPE].atom, (~0L),
	    XA_ATOM, &n, (unsigned char **)&data);

	if (!success) {
		XFree(data);
		return;
	}

	for (i = 0; i < n; i++) {
		type = data[i];
		if (type == ewmh[_NET_WM_WINDOW_TYPE_NORMAL].atom)
			break;
		if (type == ewmh[_NET_WM_WINDOW_TYPE_DOCK].atom ||
		    type == ewmh[_NET_WM_WINDOW_TYPE_TOOLBAR].atom ||
		    type == ewmh[_NET_WM_WINDOW_TYPE_UTILITY].atom) {
			win->floating = 1;
			win->quirks = TWM_Q_FLOAT | TWM_Q_ANYWHERE;
			break;
		}
		if (type == ewmh[_NET_WM_WINDOW_TYPE_SPLASH].atom ||
		    type == ewmh[_NET_WM_WINDOW_TYPE_DIALOG].atom) {
			win->floating = 1;
			win->quirks = TWM_Q_FLOAT;
			break;
		}
	}

	XFree(data);
}

#define TWM_EWMH_ACTION_COUNT_MAX	(6)
#define EWMH_F_FULLSCREEN		(1<<0)
#define EWMH_F_ABOVE			(1<<1)
#define EWMH_F_HIDDEN			(1<<2)
#define EWMH_F_SKIP_PAGER		(1<<3)
#define EWMH_F_SKIP_TASKBAR		(1<<4)
#define TWM_F_MANUAL			(1<<5)

int
ewmh_set_win_fullscreen(struct ws_win *win, int fs)
{
	struct twm_geometry	rg;

	if (!win->ws->r)
		return 0;

	if (!win->floating)
		return 0;

	DNPRINTF(TWM_D_MISC, "ewmh_set_win_fullscreen: win 0x%lx fs: %d\n",
	    win->id, fs);

	rg = win->ws->r->g;

	if (fs) {
		store_float_geom(win, win->ws->r);

		win->g.x = rg.x;
		win->g.y = rg.y;
		win->g.w = rg.w;
		win->g.h = rg.h;
	}	else {
		if (win->g_floatvalid) {
			/* refloat at last floating relative position */
			win->g.x = win->g_float.x - win->rg_float.x + rg.x;
			win->g.y = win->g_float.y - win->rg_float.y + rg.y;
			win->g.w = win->g_float.w;
			win->g.h = win->g_float.h;
		}
	}

	return 1;
}

void
ewmh_update_actions(struct ws_win *win)
{
	Atom			actions[TWM_EWMH_ACTION_COUNT_MAX];
	int			n = 0;

	if (win == NULL)
		return;

	actions[n++] = ewmh[_NET_WM_ACTION_CLOSE].atom;

	if (win->floating) {
		actions[n++] = ewmh[_NET_WM_ACTION_MOVE].atom;
		actions[n++] = ewmh[_NET_WM_ACTION_RESIZE].atom;
	}

	XChangeProperty(display, win->id, ewmh[_NET_WM_ALLOWED_ACTIONS].atom,
	    XA_ATOM, 32, PropModeReplace, (unsigned char *)actions, n);
}

#define _NET_WM_STATE_REMOVE	0    /* remove/unset property */
#define _NET_WM_STATE_ADD	1    /* add/set property */
#define _NET_WM_STATE_TOGGLE	2    /* toggle property */

void
ewmh_update_win_state(struct ws_win *win, long state, long action)
{
	unsigned int		mask = 0;
	unsigned int		changed = 0;
	unsigned int		orig_flags;

	if (win == NULL)
		return;

	if (state == ewmh[_NET_WM_STATE_FULLSCREEN].atom)
		mask = EWMH_F_FULLSCREEN;
	if (state == ewmh[_NET_WM_STATE_ABOVE].atom)
		mask = EWMH_F_ABOVE;
	if (state == ewmh[_TWM_WM_STATE_MANUAL].atom)
		mask = TWM_F_MANUAL;
	if (state == ewmh[_NET_WM_STATE_SKIP_PAGER].atom)
		mask = EWMH_F_SKIP_PAGER;
	if (state == ewmh[_NET_WM_STATE_SKIP_TASKBAR].atom)
		mask = EWMH_F_SKIP_TASKBAR;


	orig_flags = win->ewmh_flags;

	switch (action) {
	case _NET_WM_STATE_REMOVE:
		win->ewmh_flags &= ~mask;
		break;
	case _NET_WM_STATE_ADD:
		win->ewmh_flags |= mask;
		break;
	case _NET_WM_STATE_TOGGLE:
		win->ewmh_flags ^= mask;
		break;
	}

	changed = (win->ewmh_flags & mask) ^ (orig_flags & mask) ? 1 : 0;

	if (state == ewmh[_NET_WM_STATE_ABOVE].atom)
		if (changed)
			if (!floating_toggle_win(win))
				win->ewmh_flags = orig_flags; /* revert */
	if (state == ewmh[_TWM_WM_STATE_MANUAL].atom)
		if (changed)
			win->manual = (win->ewmh_flags & TWM_F_MANUAL) != 0;
	if (state == ewmh[_NET_WM_STATE_FULLSCREEN].atom)
		if (changed)
			if (!ewmh_set_win_fullscreen(win, win->ewmh_flags & EWMH_F_FULLSCREEN))
				win->ewmh_flags = orig_flags; /* revert */

	XDeleteProperty(display, win->id, ewmh[_NET_WM_STATE].atom);

	if (win->ewmh_flags & EWMH_F_FULLSCREEN)
		XChangeProperty(display, win->id, ewmh[_NET_WM_STATE].atom,
		    XA_ATOM, 32, PropModeAppend,
		    (unsigned char *)&ewmh[_NET_WM_STATE_FULLSCREEN].atom, 1);
	if (win->ewmh_flags & EWMH_F_SKIP_PAGER)
		XChangeProperty(display, win->id, ewmh[_NET_WM_STATE].atom,
		    XA_ATOM, 32, PropModeAppend,
		    (unsigned char *)&ewmh[_NET_WM_STATE_SKIP_PAGER].atom, 1);
	if (win->ewmh_flags & EWMH_F_SKIP_TASKBAR)
		XChangeProperty(display, win->id, ewmh[_NET_WM_STATE].atom,
		    XA_ATOM, 32, PropModeAppend,
		    (unsigned char *)&ewmh[_NET_WM_STATE_SKIP_TASKBAR].atom, 1);
	if (win->ewmh_flags & EWMH_F_ABOVE)
		XChangeProperty(display, win->id, ewmh[_NET_WM_STATE].atom,
		    XA_ATOM, 32, PropModeAppend,
		    (unsigned char *)&ewmh[_NET_WM_STATE_ABOVE].atom, 1);
	if (win->ewmh_flags & TWM_F_MANUAL)
		XChangeProperty(display, win->id, ewmh[_NET_WM_STATE].atom,
		    XA_ATOM, 32, PropModeAppend,
		    (unsigned char *)&ewmh[_TWM_WM_STATE_MANUAL].atom, 1);
}

void
ewmh_get_win_state(struct ws_win *win)
{
	int			success, i;
	unsigned long		n;
	Atom			*states;

	if (win == NULL)
		return;

	win->ewmh_flags = 0;
	if (win->floating)
		win->ewmh_flags |= EWMH_F_ABOVE;
	if (win->manual)
		win->ewmh_flags |= TWM_F_MANUAL;

	success = get_property(win->id, ewmh[_NET_WM_STATE].atom, (~0L), XA_ATOM,
	    &n, (unsigned char **)&states);

	if (!success)
		return;

	for (i = 0; i < n; i++)
		ewmh_update_win_state(win, states[i], _NET_WM_STATE_ADD);

	XFree(states);
}

/* events */
#ifdef TWM_DEBUG
void
dumpevent(XEvent *e)
{
	char			*name = NULL;

	switch (e->type) {
	case KeyPress:
		name = "KeyPress";
		break;
	case KeyRelease:
		name = "KeyRelease";
		break;
	case ButtonPress:
		name = "ButtonPress";
		break;
	case ButtonRelease:
		name = "ButtonRelease";
		break;
	case MotionNotify:
		name = "MotionNotify";
		break;
	case EnterNotify:
		name = "EnterNotify";
		break;
	case LeaveNotify:
		name = "LeaveNotify";
		break;
	case FocusIn:
		name = "FocusIn";
		break;
	case FocusOut:
		name = "FocusOut";
		break;
	case KeymapNotify:
		name = "KeymapNotify";
		break;
	case Expose:
		name = "Expose";
		break;
	case GraphicsExpose:
		name = "GraphicsExpose";
		break;
	case NoExpose:
		name = "NoExpose";
		break;
	case VisibilityNotify:
		name = "VisibilityNotify";
		break;
	case CreateNotify:
		name = "CreateNotify";
		break;
	case DestroyNotify:
		name = "DestroyNotify";
		break;
	case UnmapNotify:
		name = "UnmapNotify";
		break;
	case MapNotify:
		name = "MapNotify";
		break;
	case MapRequest:
		name = "MapRequest";
		break;
	case ReparentNotify:
		name = "ReparentNotify";
		break;
	case ConfigureNotify:
		name = "ConfigureNotify";
		break;
	case ConfigureRequest:
		name = "ConfigureRequest";
		break;
	case GravityNotify:
		name = "GravityNotify";
		break;
	case ResizeRequest:
		name = "ResizeRequest";
		break;
	case CirculateNotify:
		name = "CirculateNotify";
		break;
	case CirculateRequest:
		name = "CirculateRequest";
		break;
	case PropertyNotify:
		name = "PropertyNotify";
		break;
	case SelectionClear:
		name = "SelectionClear";
		break;
	case SelectionRequest:
		name = "SelectionRequest";
		break;
	case SelectionNotify:
		name = "SelectionNotify";
		break;
	case ColormapNotify:
		name = "ColormapNotify";
		break;
	case ClientMessage:
		name = "ClientMessage";
		break;
	case MappingNotify:
		name = "MappingNotify";
		break;
	}

	if (name)
		DNPRINTF(TWM_D_EVENTQ ,"window: %lu event: %s (%d), %d "
		    "remaining\n",
		    e->xany.window, name, e->type, QLength(display));
	else
		DNPRINTF(TWM_D_EVENTQ, "window: %lu unknown event %d, %d "
		    "remaining\n",
		    e->xany.window, e->type, QLength(display));
}

void
dumpwins(struct twm_region *r, union arg *args)
{
	struct ws_win		*win;
	unsigned int		state;
	XWindowAttributes	wa;

	if (r->ws == NULL) {
		fprintf(stderr, "invalid workspace\n");
		return;
	}

	fprintf(stderr, "=== managed window list ws %02d ===\n", r->ws->idx);

	TAILQ_FOREACH(win, &r->ws->winlist, entry) {
		state = getstate(win->id);
		if (!XGetWindowAttributes(display, win->id, &wa))
			fprintf(stderr, "window: %lu failed "
			    "XGetWindowAttributes\n", win->id);
		fprintf(stderr, "window: %lu map_state: %d state: %d\n",
		    win->id, wa.map_state, state);
	}

	fprintf(stderr, "===== unmanaged window list =====\n");
	TAILQ_FOREACH(win, &r->ws->unmanagedlist, entry) {
		state = getstate(win->id);
		if (!XGetWindowAttributes(display, win->id, &wa))
			fprintf(stderr, "window: %lu failed "
			    "XGetWindowAttributes\n", win->id);
		fprintf(stderr, "window: %lu map_state: %d state: %d\n",
		    win->id, wa.map_state, state);
	}

	fprintf(stderr, "=================================\n");
}
#else
#define dumpevent(e)
void
dumpwins(struct twm_region *r, union arg *args)
{
}
#endif /* TWM_DEBUG */

void			expose(XEvent *);
void			keypress(XEvent *);
void			buttonpress(XEvent *);
void			configurerequest(XEvent *);
void			configurenotify(XEvent *);
void			destroynotify(XEvent *);
void			enternotify(XEvent *);
void			focusevent(XEvent *);
void			mapnotify(XEvent *);
void			mappingnotify(XEvent *);
void			maprequest(XEvent *);
void			propertynotify(XEvent *);
void			unmapnotify(XEvent *);
void			visibilitynotify(XEvent *);
void			clientmessage(XEvent *);

void			(*handler[LASTEvent])(XEvent *) = {
				[Expose] = expose,
				[KeyPress] = keypress,
				[ButtonPress] = buttonpress,
				[ConfigureRequest] = configurerequest,
				[ConfigureNotify] = configurenotify,
				[DestroyNotify] = destroynotify,
				[EnterNotify] = enternotify,
				[FocusIn] = focusevent,
				[FocusOut] = focusevent,
				[MapNotify] = mapnotify,
				[MappingNotify] = mappingnotify,
				[MapRequest] = maprequest,
				[PropertyNotify] = propertynotify,
				[UnmapNotify] = unmapnotify,
				[VisibilityNotify] = visibilitynotify,
				[ClientMessage] = clientmessage,
};

void
sighdlr(int sig)
{
	int			saved_errno, status;
	pid_t			pid;

	saved_errno = errno;

	switch (sig) {
	case SIGCHLD:
		while ((pid = waitpid(WAIT_ANY, &status, WNOHANG)) != 0) {
			if (pid == -1) {
				if (errno == EINTR)
					continue;
#ifdef TWM_DEBUG
				if (errno != ECHILD)
					warn("sighdlr: waitpid");
#endif /* TWM_DEBUG */
				break;
			}

#ifdef TWM_DEBUG
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) != 0)
					warnx("sighdlr: child exit status: %d",
					    WEXITSTATUS(status));
			} else
				warnx("sighdlr: child is terminated "
				    "abnormally");
#endif /* TWM_DEBUG */
		}
		break;

	case SIGHUP:
		restart_wm = 1;
		break;
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		running = 0;
		break;
	}

	errno = saved_errno;
}

unsigned long
name_to_color(char *colorname)
{
	Colormap		cmap;
	Status			status;
	XColor			screen_def, exact_def;
	unsigned long		result = 0;
	char			cname[32] = "#";

	cmap = DefaultColormap(display, screens[0].idx);
	status = XAllocNamedColor(display, cmap, colorname,
	    &screen_def, &exact_def);
	if (!status) {
		strlcat(cname, colorname + 2, sizeof cname - 1);
		status = XAllocNamedColor(display, cmap, cname, &screen_def,
		    &exact_def);
	}
	if (status)
		result = screen_def.pixel;
	else
		fprintf(stderr, "color '%s' not found.\n", colorname);

	return (result);
}

void
setscreencolor(char *val, int i, int c)
{
	if (i > 0 && i <= ScreenCount(display)) {
		screens[i - 1].c[c].color = name_to_color(val);
		free(screens[i - 1].c[c].name);
		if ((screens[i - 1].c[c].name = strdup(val)) == NULL)
			errx(1, "strdup");
	} else if (i == -1) {
		for (i = 0; i < ScreenCount(display); i++) {
			screens[i].c[c].color = name_to_color(val);
			free(screens[i].c[c].name);
			if ((screens[i].c[c].name = strdup(val)) == NULL)
				errx(1, "strdup");
		}
	} else
		errx(1, "invalid screen index: %d out of bounds (maximum %d)\n",
		    i, ScreenCount(display));
}

void
custom_region(char *val)
{
	unsigned int			sidx, x, y, w, h;

	if (sscanf(val, "screen[%u]:%ux%u+%u+%u", &sidx, &w, &h, &x, &y) != 5)
		errx(1, "invalid custom region, "
		    "should be 'screen[<n>]:<n>x<n>+<n>+<n>\n");
	if (sidx < 1 || sidx > ScreenCount(display))
		errx(1, "invalid screen index: %d out of bounds (maximum %d)\n",
		    sidx, ScreenCount(display));
	sidx--;

	if (w < 1 || h < 1)
		errx(1, "region %ux%u+%u+%u too small\n", w, h, x, y);

	if (x  < 0 || x > DisplayWidth(display, sidx) ||
	    y < 0 || y > DisplayHeight(display, sidx) ||
	    w + x > DisplayWidth(display, sidx) ||
	    h + y > DisplayHeight(display, sidx)) {
		fprintf(stderr, "ignoring region %ux%u+%u+%u - not within screen boundaries "
		    "(%ux%u)\n", w, h, x, y,
		    DisplayWidth(display, sidx), DisplayHeight(display, sidx));
		return;
	}

	new_region(&screens[sidx], x, y, w, h);
}

void
socket_setnonblock(int fd)
{
	int			flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		err(1, "fcntl F_GETFL");
	flags |= O_NONBLOCK;
	if ((flags = fcntl(fd, F_SETFL, flags)) == -1)
		err(1, "fcntl F_SETFL");
}

void
bar_print(struct twm_region *r, char *s)
{
	XClearWindow(display, r->bar_window);
	XSetForeground(display, bar_gc, r->s->c[TWM_S_COLOR_BAR_FONT].color);
	XDrawString(display, r->bar_window, bar_gc, 4, bar_fs->ascent, s,
	    strlen(s));
}

void
bar_extra_stop(void)
{
	if (bar_pipe[0]) {
		close(bar_pipe[0]);
		bzero(bar_pipe, sizeof bar_pipe);
	}
	if (bar_pid) {
		kill(bar_pid, SIGTERM);
		bar_pid = 0;
	}
	strlcpy(bar_ext, "", sizeof bar_ext);
	bar_extra = 0;
}

void
bar_class_name(char *s, ssize_t sz, struct ws_win *cur_focus)
{
	int			do_class, do_name;
	Status			status;
	XClassHint		*xch = NULL;

	if ((title_name_enabled == 1 || title_class_enabled == 1) &&
	    cur_focus != NULL) {
		if ((xch = XAllocClassHint()) == NULL)
			goto out;
		status = XGetClassHint(display, cur_focus->id, xch);
		if (status == BadWindow || status == BadAlloc)
			goto out;
		do_class = (title_class_enabled && xch->res_class != NULL);
		do_name = (title_name_enabled && xch->res_name != NULL);
		if (do_class)
			strlcat(s, xch->res_class, sz);
		if (do_class && do_name)
			strlcat(s, ":", sz);
		if (do_name)
			strlcat(s, xch->res_name, sz);
		strlcat(s, "    ", sz);
	}
out:
	if (xch)
		XFree(xch);
}

void
bar_window_name(char *s, ssize_t sz, struct ws_win *cur_focus)
{
	char			*title;

	if (window_name_enabled && cur_focus != NULL) {
		XFetchName(display, cur_focus->id, &title);
		if (title) {
			if (cur_focus->floating)
				strlcat(s, "(f) ", sz);
			strlcat(s, title, sz);
			strlcat(s, " ", sz);
			XFree(title);
		}
	}
}

void
bar_update(void)
{
	time_t			tmt;
	struct tm		tm;
	struct twm_region	*r;
	int			i, x;
	size_t			len;
	char			s[TWM_BAR_MAX];
	char			cn[TWM_BAR_MAX];
	char			loc[TWM_BAR_MAX];
	char			*b;
	char			*stack = "";

	if (bar_enabled == 0)
		return;
	if (bar_extra && bar_extra_running) {
		/* ignore short reads; it'll correct itself */
		while ((b = fgetln(stdin, &len)) != NULL)
			if (b && b[len - 1] == '\n') {
				b[len - 1] = '\0';
				strlcpy(bar_ext, b, sizeof bar_ext);
			}
		if (b == NULL && errno != EAGAIN) {
			fprintf(stderr, "bar_extra failed: errno: %d %s\n",
			    errno, strerror(errno));
			bar_extra_stop();
		}
	} else
		strlcpy(bar_ext, "", sizeof bar_ext);

	if (clock_enabled == 0)
		strlcpy(s, "", sizeof s);
	else {
		time(&tmt);
		localtime_r(&tmt, &tm);
		strftime(s, sizeof s, clock_format, &tm);
		strlcat(s, "    ", sizeof s);
	}

	for (i = 0; i < ScreenCount(display); i++) {
		x = 1;
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			strlcpy(cn, "", sizeof cn);
			if (r && r->ws) {
				bar_class_name(cn, sizeof cn, r->ws->focus);
				bar_window_name(cn, sizeof cn, r->ws->focus);
			}

			if (stack_enabled)
				stack = r->ws->cur_layout->name;

			snprintf(loc, sizeof loc, "%d:%d %s   %s%s    %s    %s",
			    x++, r->ws->idx + 1, stack, s, cn, bar_ext,
			    bar_vertext);
			bar_print(r, loc);
		}
	}
	alarm(bar_delay);
}

void
bar_signal(int sig)
{
	bar_alarm = 1;
}

void
bar_toggle(struct twm_region *r, union arg *args)
{
	struct twm_region	*tmpr;
	int			i, sc = ScreenCount(display);

	DNPRINTF(TWM_D_MISC, "bar_toggle\n");

	if (bar_enabled)
		for (i = 0; i < sc; i++)
			TAILQ_FOREACH(tmpr, &screens[i].rl, entry)
				XUnmapWindow(display, tmpr->bar_window);
	else
		for (i = 0; i < sc; i++)
			TAILQ_FOREACH(tmpr, &screens[i].rl, entry)
				XMapRaised(display, tmpr->bar_window);

	bar_enabled = !bar_enabled;

	stack();
	/* must be after stack */
	bar_update();
}

void
bar_refresh(void)
{
	XSetWindowAttributes	wa;
	struct twm_region	*r;
	int			i;

	/* do this here because the conf file is in memory */
	if (bar_extra && bar_extra_running == 0 && bar_argv[0]) {
		/* launch external status app */
		bar_extra_running = 1;
		if (pipe(bar_pipe) == -1)
			err(1, "pipe error");
		socket_setnonblock(bar_pipe[0]);
		socket_setnonblock(bar_pipe[1]); /* XXX hmmm, really? */
		if (dup2(bar_pipe[0], 0) == -1)
			errx(1, "dup2");
		if (dup2(bar_pipe[1], 1) == -1)
			errx(1, "dup2");
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			err(1, "could not disable SIGPIPE");
		switch (bar_pid = fork()) {
		case -1:
			err(1, "cannot fork");
			break;
		case 0: /* child */
			close(bar_pipe[0]);
			execvp(bar_argv[0], bar_argv);
			err(1, "%s external app failed", bar_argv[0]);
			break;
		default: /* parent */
			close(bar_pipe[1]);
			break;
		}
	}

	bzero(&wa, sizeof wa);
	for (i = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			wa.border_pixel =
			    screens[i].c[TWM_S_COLOR_BAR_BORDER].color;
			wa.background_pixel =
			    screens[i].c[TWM_S_COLOR_BAR].color;
			XChangeWindowAttributes(display, r->bar_window,
			    CWBackPixel | CWBorderPixel, &wa);
		}
	bar_update();
}

void
bar_setup(struct twm_region *r)
{
	int			i, x, y;

	if (bar_fs) {
		XFreeFont(display, bar_fs);
		bar_fs = NULL;
	}

	for (i = 0; bar_fonts[i] != NULL; i++) {
		bar_fs = XLoadQueryFont(display, bar_fonts[i]);
		if (bar_fs) {
			bar_fidx = i;
			break;
		}
	}
	if (bar_fonts[i] == NULL)
			errx(1, "couldn't load font");
	if (bar_fs == NULL)
		errx(1, "couldn't create font structure");

	bar_height = bar_fs->ascent + bar_fs->descent + 3;
	x = X(r);
	y = bar_at_bottom ? (Y(r) + HEIGHT(r) - bar_height) : Y(r);

	r->bar_window = XCreateSimpleWindow(display,
	    r->s->root, x, y, WIDTH(r) - 2, bar_height - 2,
	    1, r->s->c[TWM_S_COLOR_BAR_BORDER].color,
	    r->s->c[TWM_S_COLOR_BAR].color);
	bar_gc = XCreateGC(display, r->bar_window, 0, &bar_gcv);
	XSetFont(display, bar_gc, bar_fs->fid);
	XSelectInput(display, r->bar_window, VisibilityChangeMask);
	if (bar_enabled)
		XMapRaised(display, r->bar_window);
	DNPRINTF(TWM_D_MISC, "bar_setup: bar_window %lu\n", r->bar_window);

	if (signal(SIGALRM, bar_signal) == SIG_ERR)
		err(1, "could not install bar_signal");
	bar_refresh();
}

void
set_win_state(struct ws_win *win, long state)
{
	long			data[] = {state, None};

	DNPRINTF(TWM_D_EVENT, "set_win_state: window: %lu\n", win->id);

	if (win == NULL)
		return;

	XChangeProperty(display, win->id, astate, astate, 32, PropModeReplace,
	    (unsigned char *)data, 2);
}

long
getstate(Window w)
{
	long			result = -1;
	unsigned char		*p = NULL;
	unsigned long		n;

	if (!get_property(w, astate, 2L, astate, &n, &p))
		return (-1);
	if (n != 0)
		result = *((long *)p);
	XFree(p);
	return (result);
}

void
version(struct twm_region *r, union arg *args)
{
	bar_version = !bar_version;
	if (bar_version)
		snprintf(bar_vertext, sizeof bar_vertext, "Version: %s CVS: %s",
		    TWM_VERSION, cvstag);
	else
		strlcpy(bar_vertext, "", sizeof bar_vertext);
	bar_update();
}

void
client_msg(struct ws_win *win, Atom a)
{
	XClientMessageEvent	cm;

	if (win == NULL)
		return;

	bzero(&cm, sizeof cm);
	cm.type = ClientMessage;
	cm.window = win->id;
	cm.message_type = aprot;
	cm.format = 32;
	cm.data.l[0] = a;
	cm.data.l[1] = CurrentTime;
	XSendEvent(display, win->id, False, 0L, (XEvent *)&cm);
}

void
configreq_win(struct ws_win *win)
{
	XConfigureRequestEvent	cr;

	if (win == NULL)
		return;

	bzero(&cr, sizeof cr);
	cr.type = ConfigureRequest;
	cr.display = display;
	cr.parent = win->id;
	cr.window = win->id;
	cr.x = win->g.x;
	cr.y = win->g.y;
	cr.width = win->g.w;
	cr.height = win->g.h;
	cr.border_width = border_width;

	XSendEvent(display, win->id, False, StructureNotifyMask, (XEvent *)&cr);
}

void
config_win(struct ws_win *win)
{
	XConfigureEvent		ce;

	DNPRINTF(TWM_D_MISC, "config_win: win %lu x %d y %d w %d h %d\n",
	    win->id, win->g.x, win->g.y, win->g.w, win->g.h);

	if (win == NULL)
		return;

	ce.type = ConfigureNotify;
	ce.display = display;
	ce.event = win->id;
	ce.window = win->id;
	ce.x = win->g.x;
	ce.y = win->g.y;
	ce.width = win->g.w;
	ce.height = win->g.h;
	ce.border_width = border_width; /* XXX store this! */
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(display, win->id, False, StructureNotifyMask, (XEvent *)&ce);
}

int
count_win(struct workspace *ws, int count_transient)
{
	struct ws_win		*win;
	int			count = 0;

	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (count_transient == 0 && win->floating)
			continue;
		if (count_transient == 0 && win->transient)
			continue;
		count++;
	}
	DNPRINTF(TWM_D_MISC, "count_win: %d\n", count);

	return (count);
}

void
quit(struct twm_region *r, union arg *args)
{
	DNPRINTF(TWM_D_MISC, "quit\n");
	running = 0;
}

void
unmap_window(struct ws_win *win)
{
	if (win == NULL)
		return;

	/* don't unmap again */
	if (getstate(win->id) == IconicState)
		return;

	set_win_state(win, IconicState);

	XUnmapWindow(display, win->id);
	XSetWindowBorder(display, win->id,
	    win->s->c[TWM_S_COLOR_UNFOCUS].color);
}

void
unmap_all(void)
{
	struct ws_win		*win;
	int			i, j;

	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < TWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				unmap_window(win);
}

void
fake_keypress(struct ws_win *win, int keysym, int modifiers)
{
	XKeyEvent event;

	if (win == NULL)
		return;

	event.display = display;	/* Ignored, but what the hell */
	event.window = win->id;
	event.root = win->s->root;
	event.subwindow = None;
	event.time = CurrentTime;
	event.x = win->g.x;
	event.y = win->g.y;
	event.x_root = 1;
	event.y_root = 1;
	event.same_screen = True;
	event.keycode = XKeysymToKeycode(display, keysym);
	event.state = modifiers;

	event.type = KeyPress;
	XSendEvent(event.display, event.window, True,
	    KeyPressMask, (XEvent *)&event);

	event.type = KeyRelease;
	XSendEvent(event.display, event.window, True,
	    KeyPressMask, (XEvent *)&event);

}

void
restart(struct twm_region *r, union arg *args)
{
	DNPRINTF(TWM_D_MISC, "restart: %s\n", start_argv[0]);

	/* disable alarm because the following code may not be interrupted */
	alarm(0);
	if (signal(SIGALRM, SIG_IGN) == SIG_ERR)
		errx(1, "can't disable alarm");

	bar_extra_stop();
	bar_extra = 1;
	unmap_all();
	XCloseDisplay(display);
	execvp(start_argv[0], start_argv);
	fprintf(stderr, "execvp failed\n");
	perror(" failed");
	quit(NULL, NULL);
}

struct twm_region *
root_to_region(Window root)
{
	struct twm_region	*r = NULL;
	Window			rr, cr;
	int			i, x, y, wx, wy;
	unsigned int		mask;

	for (i = 0; i < ScreenCount(display); i++)
		if (screens[i].root == root)
			break;

	if (XQueryPointer(display, screens[i].root,
	    &rr, &cr, &x, &y, &wx, &wy, &mask) != False) {
		/* choose a region based on pointer location */
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			if (x >= X(r) && x <= X(r) + WIDTH(r) &&
			    y >= Y(r) && y <= Y(r) + HEIGHT(r))
				break;
	}

	if (r == NULL)
		r = TAILQ_FIRST(&screens[i].rl);

	return (r);
}

struct ws_win *
find_unmanaged_window(Window id)
{
	struct ws_win		*win;
	int			i, j;

	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < TWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].unmanagedlist,
			    entry)
				if (id == win->id)
					return (win);
	return (NULL);
}

struct ws_win *
find_window(Window id)
{
	struct ws_win		*win;
	Window			wrr, wpr, *wcr = NULL;
	int			i, j;
	unsigned int		nc;

	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < TWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (id == win->id)
					return (win);

	/* if we were looking for the parent return that window instead */
	if (XQueryTree(display, id, &wrr, &wpr, &wcr, &nc) == 0)
		return (NULL);
	if (wcr)
		XFree(wcr);

	/* ignore not found and root */
	if (wpr == 0 || wrr == wpr)
		return (NULL);

	/* look for parent */
	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < TWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (wpr == win->id)
					return (win);

	return (NULL);
}

void
spawn(struct twm_region *r, union arg *args)
{
	int			fd;
	char			*ret = NULL;

	DNPRINTF(TWM_D_MISC, "spawn: %s\n", args->argv[0]);

	if (fork() == 0) {
		if (display)
			close(ConnectionNumber(display));

#if 0
		setenv("LD_PRELOAD", TWM_LIB, 1);
#endif

		if (asprintf(&ret, "%d", r->ws->idx) == -1) {
			perror("_TWM_WS");
			_exit(1);
		}
		setenv("_TWM_WS", ret, 1);
		free(ret);
		ret = NULL;

		if (asprintf(&ret, "%d", getpid()) == -1) {
			perror("_TWM_PID");
			_exit(1);
		}
		setenv("_TWM_PID", ret, 1);
		free(ret);
		ret = NULL;

		if (setsid() == -1) {
			perror("setsid");
			_exit(1);
		}

		/*
		 * close stdin and stdout to prevent interaction between apps
		 * and the baraction script
		 * leave stderr open to record errors
		*/
		if ((fd = open(_PATH_DEVNULL, O_RDWR, 0)) == -1) {
			perror("open");
			_exit(1);
		}
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		if (fd > 2)
			close(fd);

		execvp(args->argv[0], args->argv);

		perror("execvp");
		_exit(1);
	}
}

void
spawnterm(struct twm_region *r, union arg *args)
{
	DNPRINTF(TWM_D_MISC, "spawnterm\n");

	if (term_width)
		setenv("_TWM_XTERM_FONTADJ", "", 1);
	spawn(r, args);
}

void
kill_refs(struct ws_win *win)
{
	int			i, x;
	struct twm_region	*r;
	struct workspace	*ws;

	if (win == NULL)
		return;

	for (i = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < TWM_WS_MAX; x++) {
				ws = &r->s->ws[x];
				if (win == ws->focus)
					ws->focus = NULL;
				if (win == ws->focus_prev)
					ws->focus_prev = NULL;
			}
}

int
validate_win(struct ws_win *testwin)
{
	struct ws_win		*win;
	struct workspace	*ws;
	struct twm_region	*r;
	int			i, x, foundit = 0;

	if (testwin == NULL)
		return(0);

	for (i = 0, foundit = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < TWM_WS_MAX; x++) {
				ws = &r->s->ws[x];
				TAILQ_FOREACH(win, &ws->winlist, entry)
					if (win == testwin)
						return (0);
			}
	return (1);
}

int
validate_ws(struct workspace *testws)
{
	struct twm_region	*r;
	struct workspace	*ws;
	int			foundit, i, x;

	/* validate all ws */
	for (i = 0, foundit = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < TWM_WS_MAX; x++) {
				ws = &r->s->ws[x];
				if (ws == testws)
					return (0);
			}
	return (1);
}

void
unfocus_win(struct ws_win *win)
{
	XEvent			cne;
	Window			none = None;

	DNPRINTF(TWM_D_FOCUS, "unfocus_win: id: %lu\n", WINID(win));

	if (win == NULL)
		return;
	if (win->ws == NULL)
		return;

	if (validate_ws(win->ws))
		return; /* XXX this gets hit with thunderbird, needs fixing */

	if (win->ws->r == NULL)
		return;

	if (validate_win(win)) {
		kill_refs(win);
		return;
	}

	if (win->ws->focus == win) {
		win->ws->focus = NULL;
		win->ws->focus_prev = win;
	}

	if (validate_win(win->ws->focus)) {
		kill_refs(win->ws->focus);
		win->ws->focus = NULL;
	}
	if (validate_win(win->ws->focus_prev)) {
		kill_refs(win->ws->focus_prev);
		win->ws->focus_prev = NULL;
	}

	/* drain all previous unfocus events */
	while (XCheckTypedEvent(display, FocusOut, &cne) == True)
		;

	grabbuttons(win, 0);
	XSetWindowBorder(display, win->id,
	    win->ws->r->s->c[TWM_S_COLOR_UNFOCUS].color);

	XChangeProperty(display, win->s->root,
	    ewmh[_NET_ACTIVE_WINDOW].atom, XA_WINDOW, 32,
	    PropModeReplace, (unsigned char *)&none,1);
}

void
unfocus_all(void)
{
	struct ws_win		*win;
	int			i, j;

	DNPRINTF(TWM_D_FOCUS, "unfocus_all\n");

	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < TWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				unfocus_win(win);
}

void
focus_win(struct ws_win *win)
{
	XEvent			cne;
	Window			cur_focus;
	int			rr;
	struct ws_win		*cfw = NULL;


	DNPRINTF(TWM_D_FOCUS, "focus_win: id: %lu\n", win ? win->id : 0);

	if (win == NULL)
		return;
	if (win->ws == NULL)
		return;

	if (validate_ws(win->ws))
		return; /* XXX this gets hit with thunderbird, needs fixing */

	if (validate_win(win)) {
		kill_refs(win);
		return;
	}

	if (validate_win(win)) {
		kill_refs(win);
		return;
	}

	XGetInputFocus(display, &cur_focus, &rr);
	if ((cfw = find_window(cur_focus)) != NULL)
		unfocus_win(cfw);

	win->ws->focus = win;

	if (win->ws->r != NULL) {
		/* drain all previous focus events */
		while (XCheckTypedEvent(display, FocusIn, &cne) == True)
			;

		if (win->java == 0)
			XSetInputFocus(display, win->id,
			    RevertToParent, CurrentTime);
		grabbuttons(win, 1);
		XSetWindowBorder(display, win->id,
		    win->ws->r->s->c[TWM_S_COLOR_FOCUS].color);
		if (win->ws->cur_layout->flags & TWM_L_MAPONFOCUS)
			XMapRaised(display, win->id);

		XChangeProperty(display, win->s->root,
		    ewmh[_NET_ACTIVE_WINDOW].atom, XA_WINDOW, 32,
		    PropModeReplace, (unsigned char *)&win->id,1);
	}

	if (window_name_enabled)
		bar_update();
}

void
switchws(struct twm_region *r, union arg *args)
{
	int			wsid = args->id, unmap_old = 0;
	struct twm_region	*this_r, *other_r;
	struct ws_win		*win;
	struct workspace	*new_ws, *old_ws;
	union arg		a;

	if (!(r && r->s))
		return;

	this_r = r;
	old_ws = this_r->ws;
	new_ws = &this_r->s->ws[wsid];

	DNPRINTF(TWM_D_WS, "switchws screen[%d]:%dx%d+%d+%d: "
	    "%d -> %d\n", r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r),
	    old_ws->idx, wsid);

	if (new_ws == NULL || old_ws == NULL)
		return;
	if (new_ws == old_ws)
		return;

	other_r = new_ws->r;
	if (other_r == NULL) {
		/* the other workspace is hidden, hide this one */
		old_ws->r = NULL;
		unmap_old = 1;
	} else {
		/* the other ws is visible in another region, exchange them */
		other_r->ws_prior = new_ws;
		other_r->ws = old_ws;
		old_ws->r = other_r;
	}
	this_r->ws_prior = old_ws;
	this_r->ws = new_ws;
	new_ws->r = this_r;

	stack();
	a.id = TWM_ARG_ID_FOCUSCUR;
	focus(new_ws->r, &a);
	bar_update();

	/* unmap old windows */
	if (unmap_old)
		TAILQ_FOREACH(win, &old_ws->winlist, entry)
			unmap_window(win);
}

void
cyclews(struct twm_region *r, union arg *args)
{
	union			arg a;
	struct twm_screen	*s = r->s;

	DNPRINTF(TWM_D_WS, "cyclews id %d "
	    "in screen[%d]:%dx%d+%d+%d ws %d\n", args->id,
	    r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	a.id = r->ws->idx;
	do {
		switch (args->id) {
		case TWM_ARG_ID_CYCLEWS_UP:
			if (a.id < TWM_WS_MAX - 1)
				a.id++;
			else
				a.id = 0;
			break;
		case TWM_ARG_ID_CYCLEWS_DOWN:
			if (a.id > 0)
				a.id--;
			else
				a.id = TWM_WS_MAX - 1;
			break;
		default:
			return;
		};

		if (cycle_empty == 0 && TAILQ_EMPTY(&s->ws[a.id].winlist))
			continue;
		if (cycle_visible == 0 && s->ws[a.id].r != NULL)
			continue;

		switchws(r, &a);
	} while (a.id != r->ws->idx);
}

void
priorws(struct twm_region *r, union arg *args)
{
	union arg		a;

	DNPRINTF(TWM_D_WS, "priorws id %d "
	    "in screen[%d]:%dx%d+%d+%d ws %d\n", args->id,
	    r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	if (r->ws_prior == NULL)
		return;

	a.id = r->ws_prior->idx;
	switchws(r, &a);
}

void
cyclescr(struct twm_region *r, union arg *args)
{
	struct twm_region	*rr = NULL;
	union arg		a;
	int			i, x, y;

	/* do nothing if we don't have more than one screen */
	if (!(ScreenCount(display) > 1 || outputs > 1))
		return;

	i = r->s->idx;
	switch (args->id) {
	case TWM_ARG_ID_CYCLESC_UP:
		rr = TAILQ_NEXT(r, entry);
		if (rr == NULL)
			rr = TAILQ_FIRST(&screens[i].rl);
		break;
	case TWM_ARG_ID_CYCLESC_DOWN:
		rr = TAILQ_PREV(r, twm_region_list, entry);
		if (rr == NULL)
			rr = TAILQ_LAST(&screens[i].rl, twm_region_list);
		break;
	default:
		return;
	};
	if (rr == NULL)
		return;

	/* move mouse to region */
	x = rr->g.x + 1;
	y = rr->g.y + 1 + (bar_enabled ? bar_height : 0);
	XWarpPointer(display, None, rr->s[i].root, 0, 0, 0, 0, x, y);

	a.id = TWM_ARG_ID_FOCUSCUR;
	focus(rr, &a);

	if (rr->ws->focus) {
		/* move to focus window */
		x = rr->ws->focus->g.x + 1;
		y = rr->ws->focus->g.y + 1;
		XWarpPointer(display, None, rr->s[i].root, 0, 0, 0, 0, x, y);
	}
}

void
swapwin(struct twm_region *r, union arg *args)
{
	struct ws_win		*target, *source;
	struct ws_win		*cur_focus;
	struct ws_win_list	*wl;


	DNPRINTF(TWM_D_WS, "swapwin id %d "
	    "in screen %d region %dx%d+%d+%d ws %d\n", args->id,
	    r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	cur_focus = r->ws->focus;
	if (cur_focus == NULL)
		return;

	source = cur_focus;
	wl = &source->ws->winlist;

	switch (args->id) {
	case TWM_ARG_ID_SWAPPREV:
		target = TAILQ_PREV(source, ws_win_list, entry);
		TAILQ_REMOVE(wl, cur_focus, entry);
		if (target == NULL)
			TAILQ_INSERT_TAIL(wl, source, entry);
		else
			TAILQ_INSERT_BEFORE(target, source, entry);
		break;
	case TWM_ARG_ID_SWAPNEXT:
		target = TAILQ_NEXT(source, entry);
		TAILQ_REMOVE(wl, source, entry);
		if (target == NULL)
			TAILQ_INSERT_HEAD(wl, source, entry);
		else
			TAILQ_INSERT_AFTER(wl, target, source, entry);
		break;
	case TWM_ARG_ID_SWAPMAIN:
		target = TAILQ_FIRST(wl);
		if (target == source) {
			if (source->ws->focus_prev != NULL &&
			    source->ws->focus_prev != target)

				source = source->ws->focus_prev;
			else
				return;
                }
		if (target == NULL || source == NULL)
			return;
		source->ws->focus_prev = target;
		TAILQ_REMOVE(wl, target, entry);
		TAILQ_INSERT_BEFORE(source, target, entry);
		TAILQ_REMOVE(wl, source, entry);
		TAILQ_INSERT_HEAD(wl, source, entry);
		break;
	case TWM_ARG_ID_MOVELAST:
		TAILQ_REMOVE(wl, source, entry);
		TAILQ_INSERT_TAIL(wl, source, entry);
		break;
	default:
		DNPRINTF(TWM_D_MOVE, "invalid id: %d\n", args->id);
		return;
	}

	stack();
}

void
focus_prev(struct ws_win *win)
{
	struct ws_win		*winfocus = NULL, *winlostfocus = NULL;
	struct ws_win		*cur_focus = NULL;
	struct ws_win_list	*wl = NULL;
	struct workspace	*ws = NULL;

	DNPRINTF(TWM_D_FOCUS, "focus_prev: id %lu\n", WINID(win));

	if (!(win && win->ws))
		return;

	ws = win->ws;
	wl = &ws->winlist;
	cur_focus = ws->focus;
	winlostfocus = cur_focus;

	/* pickle, just focus on whatever */
	if (cur_focus == NULL) {
		/* use prev_focus if valid */
		if (ws->focus_prev && ws->focus_prev != cur_focus &&
		    find_window(WINID(ws->focus_prev)))
			winfocus = ws->focus_prev;
		if (winfocus == NULL)
			winfocus = TAILQ_FIRST(wl);
		goto done;
	}

	/* if transient focus on parent */
	if (cur_focus->transient) {
		winfocus = find_window(cur_focus->transient);
		goto done;
	}

	/* if in max_stack try harder */
	if (ws->cur_layout->flags & TWM_L_FOCUSPREV) {
		if (cur_focus != ws->focus_prev)
			winfocus = ws->focus_prev;
		else if (cur_focus != ws->focus)
			winfocus = ws->focus;
		else
			winfocus = TAILQ_PREV(win, ws_win_list, entry);
		if (winfocus)
			goto done;
	}

	if (cur_focus == win)
		winfocus = TAILQ_PREV(win, ws_win_list, entry);
	if (winfocus == NULL)
		winfocus = TAILQ_LAST(wl, ws_win_list);
	if (winfocus == NULL || winfocus == win)
		winfocus = TAILQ_NEXT(cur_focus, entry);
done:
	if (winfocus == winlostfocus || winfocus == NULL)
		return;

	focus_magic(winfocus, TWM_F_GENERIC);
}

void
focus(struct twm_region *r, union arg *args)
{
	struct ws_win		*winfocus = NULL, *winlostfocus = NULL;
	struct ws_win		*cur_focus = NULL;
	struct ws_win_list	*wl = NULL;
	struct workspace	*ws = NULL;

	if (!(r && r->ws))
		return;

	DNPRINTF(TWM_D_FOCUS, "focus: id %d\n", args->id);

	/* treat FOCUS_CUR special */
	if (args->id == TWM_ARG_ID_FOCUSCUR) {
		if (r->ws->focus)
			winfocus = r->ws->focus;
		else if (r->ws->focus_prev)
			winfocus = r->ws->focus_prev;
		else
			winfocus = TAILQ_FIRST(&r->ws->winlist);

		focus_magic(winfocus, TWM_F_GENERIC);
		return;
	}

	if ((cur_focus = r->ws->focus) == NULL)
		return;
	ws = r->ws;
	wl = &ws->winlist;

	winlostfocus = cur_focus;

	switch (args->id) {
	case TWM_ARG_ID_FOCUSPREV:
		winfocus = TAILQ_PREV(cur_focus, ws_win_list, entry);
		if (winfocus == NULL)
			winfocus = TAILQ_LAST(wl, ws_win_list);
		break;

	case TWM_ARG_ID_FOCUSNEXT:
		winfocus = TAILQ_NEXT(cur_focus, entry);
		if (winfocus == NULL)
			winfocus = TAILQ_FIRST(wl);
		break;

	case TWM_ARG_ID_FOCUSMAIN:
		winfocus = TAILQ_FIRST(wl);
		if (winfocus == cur_focus)
			winfocus = cur_focus->ws->focus_prev;
		break;

	default:
		return;
	}

	if (winfocus == winlostfocus || winfocus == NULL)
		return;

	focus_magic(winfocus, TWM_F_GENERIC);
}

void
cycle_layout(struct twm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;
	struct ws_win		*winfocus;
	union arg		a;

	DNPRINTF(TWM_D_EVENT, "cycle_layout: workspace: %d\n", ws->idx);

	winfocus = ws->focus;

	ws->cur_layout++;
	if (ws->cur_layout->l_stack == NULL)
		ws->cur_layout = &layouts[0];

	stack();
	a.id = TWM_ARG_ID_FOCUSCUR;
	focus(r, &a);
	bar_update();
}

void
stack_config(struct twm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;

	DNPRINTF(TWM_D_STACK, "stack_config for workspace %d (id %d\n",
	    args->id, ws->idx);

	if (ws->cur_layout->l_config != NULL)
		ws->cur_layout->l_config(ws, args->id);

	if (args->id != TWM_ARG_ID_STACKINIT);
		stack();
}

void
stack(void) {
	struct twm_geometry	g;
	struct twm_region	*r;
	int			i, j;

	DNPRINTF(TWM_D_STACK, "stack\n");

	for (i = 0; i < ScreenCount(display); i++) {
		j = 0;
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			DNPRINTF(TWM_D_STACK, "stacking workspace %d "
			    "(screen %d, region %d)\n", r->ws->idx, i, j++);

			/* start with screen geometry, adjust for bar */
			g = r->g;
			g.w -= 2;
			g.h -= 2;
			if (bar_enabled) {
				if (!bar_at_bottom)
					g.y += bar_height;
				g.h -= bar_height;
			}
			r->ws->cur_layout->l_stack(r->ws, &g);
			/* save r so we can track region changes */
			r->ws->old_r = r;
		}
	}
	if (font_adjusted)
		font_adjusted--;
}

void
store_float_geom(struct ws_win *win, struct twm_region *r)
{
	/* retain window geom and region geom */
	win->g_float.x = win->g.x;
	win->g_float.y = win->g.y;
	win->g_float.w = win->g.w;
	win->g_float.h = win->g.h;
	win->rg_float.x = r->g.x;
	win->rg_float.y = r->g.y;
	win->rg_float.w = r->g.w;
	win->rg_float.h = r->g.h;
	win->g_floatvalid = 1;
}

void
stack_floater(struct ws_win *win, struct twm_region *r)
{
	unsigned int		mask;
	XWindowChanges		wc;

	if (win == NULL)
		return;

	bzero(&wc, sizeof wc);
	mask = CWX | CWY | CWBorderWidth | CWWidth | CWHeight;

	/*
	 * to allow windows to change their size (e.g. mplayer fs) only retrieve
	 * geom on ws switches or return from max mode
	 */

	if (win->floatmaxed || (r != r->ws->old_r && win->g_floatvalid
	    && !(win->ewmh_flags & EWMH_F_FULLSCREEN))) {
		/*
		 * use stored g and rg to set relative position and size
		 * as in old region or before max stack mode
		 */
		win->g.x = win->g_float.x - win->rg_float.x + r->g.x;
		win->g.y = win->g_float.y - win->rg_float.y + r->g.y;
		win->g.w = win->g_float.w;
		win->g.h = win->g_float.h;
		win->g_floatvalid = 0;
	}

	win->floatmaxed = 0;

	if ((win->quirks & TWM_Q_FULLSCREEN) && (win->g.w >= WIDTH(r)) &&
	    (win->g.h >= HEIGHT(r)))
		wc.border_width = 0;
	else
		wc.border_width = border_width;
	if (win->transient && (win->quirks & TWM_Q_TRANSSZ)) {
		win->g.w = (double)WIDTH(r) * dialog_ratio;
		win->g.h = (double)HEIGHT(r) * dialog_ratio;
	}

	if (!win->manual) {
		/*
		 * floaters and transients are auto-centred unless moved
		 * or resized
		 */
		win->g.x = r->g.x + (WIDTH(r) - win->g.w) / 2;
		win->g.y = r->g.y + (HEIGHT(r) - win->g.h) / 2;
	}

	/* win can be outside r if new r smaller than old r */
	/* Ensure top left corner inside r (move probs otherwise) */
	if (win->g.x < r->g.x )
		win->g.x = r->g.x;
	if (win->g.x > r->g.x + r->g.w - 1)
		win->g.x = (win->g.w > r->g.w) ? r->g.x :
		    (r->g.x + r->g.w - win->g.w - 2);
	if (win->g.y < r->g.y )
		win->g.y = r->g.y;
	if (win->g.y > r->g.y + r->g.h - 1)
		win->g.y = (win->g.h > r->g.h) ? r->g.y :
		    (r->g.y + r->g.h - win->g.h - 2);

	wc.x = win->g.x;
	wc.y = win->g.y;
	wc.width = win->g.w;
	wc.height = win->g.h;

	/*
	 * Retain floater and transient geometry for correct positioning
	 * when ws changes region
	 */
	if (!(win->ewmh_flags & EWMH_F_FULLSCREEN))
		store_float_geom(win, r);

	DNPRINTF(TWM_D_MISC, "stack_floater: win %lu x %d y %d w %d h %d\n",
	    win->id, wc.x, wc.y, wc.width, wc.height);

	XConfigureWindow(display, win->id, mask, &wc);
	configreq_win(win);
}

/*
 * Send keystrokes to terminal to decrease/increase the font size as the
 * window size changes.
 */
void
adjust_font(struct ws_win *win)
{
	if (!(win->quirks & TWM_Q_XTERM_FONTADJ) ||
	    win->floating || win->transient)
		return;

	if (win->sh.width_inc && win->last_inc != win->sh.width_inc &&
	    win->g.w / win->sh.width_inc < term_width &&
	    win->font_steps < TWM_MAX_FONT_STEPS) {
		win->font_size_boundary[win->font_steps] =
		    (win->sh.width_inc * term_width) + win->sh.base_width;
		win->font_steps++;
		font_adjusted++;
		win->last_inc = win->sh.width_inc;
		fake_keypress(win, XK_KP_Subtract, ShiftMask);
	} else if (win->font_steps && win->last_inc != win->sh.width_inc &&
	    win->g.w > win->font_size_boundary[win->font_steps - 1]) {
		win->font_steps--;
		font_adjusted++;
		win->last_inc = win->sh.width_inc;
		fake_keypress(win, XK_KP_Add, ShiftMask);
	}
}

#define SWAPXY(g)	do {				\
	int tmp;					\
	tmp = (g)->y; (g)->y = (g)->x; (g)->x = tmp;	\
	tmp = (g)->h; (g)->h = (g)->w; (g)->w = tmp;	\
} while (0)
void
stack_master(struct workspace *ws, struct twm_geometry *g, int rot, int flip)
{
	XWindowChanges		wc;
	XWindowAttributes	wa;
	struct twm_geometry	win_g, r_g = *g;
	struct ws_win		*win, *fs_win = 0;
	int			i, j, s, stacks;
	int			w_inc = 1, h_inc, w_base = 1, h_base;
	int			hrh, extra = 0, h_slice, last_h = 0;
	int			split, colno, winno, mwin, msize, mscale;
	int			remain, missing, v_slice, reconfigure;
	unsigned int		mask;

	DNPRINTF(TWM_D_STACK, "stack_master: workspace: %d\n rot=%s flip=%s",
	    ws->idx, rot ? "yes" : "no", flip ? "yes" : "no");

	winno = count_win(ws, 0);
	if (winno == 0 && count_win(ws, 1) == 0)
		return;

	TAILQ_FOREACH(win, &ws->winlist, entry)
		if (win->transient == 0 && win->floating == 0)
			break;

	if (win == NULL)
		goto notiles;

	if (rot) {
		w_inc = win->sh.width_inc;
		w_base = win->sh.base_width;
		mwin = ws->l_state.horizontal_mwin;
		mscale = ws->l_state.horizontal_msize;
		stacks = ws->l_state.horizontal_stacks;
		SWAPXY(&r_g);
	} else {
		w_inc = win->sh.height_inc;
		w_base = win->sh.base_height;
		mwin = ws->l_state.vertical_mwin;
		mscale = ws->l_state.vertical_msize;
		stacks = ws->l_state.vertical_stacks;
	}
	win_g = r_g;

	if (stacks > winno - mwin)
		stacks = winno - mwin;
	if (stacks < 1)
		stacks = 1;

	h_slice = r_g.h / TWM_H_SLICE;
	if (mwin && winno > mwin) {
		v_slice = r_g.w / TWM_V_SLICE;

		split = mwin;
		colno = split;
		win_g.w = v_slice * mscale;

		if (w_inc > 1 && w_inc < v_slice) {
			/* adjust for window's requested size increment */
			remain = (win_g.w - w_base) % w_inc;
			missing = w_inc - remain;
			win_g.w -= remain;
			extra += remain;
		}

		msize = win_g.w;
		if (flip)
			win_g.x += r_g.w - msize;
	} else {
		msize = -2;
		colno = split = winno / stacks;
		win_g.w = ((r_g.w - (stacks * 2) + 2) / stacks);
	}
	hrh = r_g.h / colno;
	extra = r_g.h - (colno * hrh);
	win_g.h = hrh - 2;

	/*  stack all the tiled windows */
	i = j = 0, s = stacks;
	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient != 0 || win->floating != 0)
			continue;

		if (win->ewmh_flags & EWMH_F_FULLSCREEN) {
			fs_win = win;
			continue;
		}

		if (split && i == split) {
			colno = (winno - mwin) / stacks;
			if (s <= (winno - mwin) % stacks)
				colno++;
			split = split + colno;
			hrh = (r_g.h / colno);
			extra = r_g.h - (colno * hrh);
			if (flip)
				win_g.x = r_g.x;
			else
				win_g.x += win_g.w + 2;
			win_g.w = (r_g.w - msize - (stacks * 2)) / stacks;
			if (s == 1)
				win_g.w += (r_g.w - msize - (stacks * 2)) %
				    stacks;
			s--;
			j = 0;
		}
		win_g.h = hrh - 2;
		if (rot) {
			h_inc = win->sh.width_inc;
			h_base = win->sh.base_width;
		} else {
			h_inc =	win->sh.height_inc;
			h_base = win->sh.base_height;
		}
		if (j == colno - 1) {
			win_g.h = hrh + extra;
		} else if (h_inc > 1 && h_inc < h_slice) {
			/* adjust for window's requested size increment */
			remain = (win_g.h - h_base) % h_inc;
			missing = h_inc - remain;

			if (missing <= extra || j == 0) {
				extra -= missing;
				win_g.h += missing;
			} else {
				win_g.h -= remain;
				extra += remain;
			}
		}

		if (j == 0)
			win_g.y = r_g.y;
		else
			win_g.y += last_h + 2;

		bzero(&wc, sizeof wc);
		if (disable_border && bar_enabled == 0 && winno == 1){
			wc.border_width = 0;
			win_g.w += 2 * border_width;
			win_g.h += 2 * border_width;
		} else
			wc.border_width = border_width;
		reconfigure = 0;
		if (rot) {
			if (win->g.x != win_g.y || win->g.y != win_g.x ||
			    win->g.w != win_g.h || win->g.h != win_g.w) {
				reconfigure = 1;
				win->g.x = wc.x = win_g.y;
				win->g.y = wc.y = win_g.x;
				win->g.w = wc.width = win_g.h;
				win->g.h = wc.height = win_g.w;
			}
		} else {
			if (win->g.x != win_g.x || win->g.y != win_g.y ||
			    win->g.w != win_g.w || win->g.h != win_g.h) {
				reconfigure = 1;
				win->g.x = wc.x = win_g.x;
				win->g.y = wc.y = win_g.y;
				win->g.w = wc.width = win_g.w;
				win->g.h = wc.height = win_g.h;
			}
		}
		if (reconfigure) {
			adjust_font(win);
			mask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
			XConfigureWindow(display, win->id, mask, &wc);
			configreq_win(win);
		}

		if (XGetWindowAttributes(display, win->id, &wa))
			if (wa.map_state == IsUnmapped)
				XMapRaised(display, win->id);

		last_h = win_g.h;
		i++;
		j++;
	}

 notiles:
	/* now, stack all the floaters and transients */
	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient == 0 && win->floating == 0)
			continue;

		if (win->ewmh_flags & EWMH_F_FULLSCREEN) {
			fs_win = win;
			continue;
		}

		stack_floater(win, ws->r);
		XMapRaised(display, win->id);
	}

	if (fs_win) {
		stack_floater(fs_win, ws->r);
		XMapRaised(display, fs_win->id);
	}
}

void
vertical_config(struct workspace *ws, int id)
{
	DNPRINTF(TWM_D_STACK, "vertical_resize: workspace: %d\n", ws->idx);

	switch (id) {
	case TWM_ARG_ID_STACKRESET:
	case TWM_ARG_ID_STACKINIT:
		ws->l_state.vertical_msize = TWM_V_SLICE / 2;
		ws->l_state.vertical_mwin = 1;
		ws->l_state.vertical_stacks = 1;
		break;
	case TWM_ARG_ID_MASTERSHRINK:
		if (ws->l_state.vertical_msize > 1)
			ws->l_state.vertical_msize--;
		break;
	case TWM_ARG_ID_MASTERGROW:
		if (ws->l_state.vertical_msize < TWM_V_SLICE - 1)
			ws->l_state.vertical_msize++;
		break;
	case TWM_ARG_ID_MASTERADD:
		ws->l_state.vertical_mwin++;
		break;
	case TWM_ARG_ID_MASTERDEL:
		if (ws->l_state.vertical_mwin > 0)
			ws->l_state.vertical_mwin--;
		break;
	case TWM_ARG_ID_STACKINC:
		ws->l_state.vertical_stacks++;
		break;
	case TWM_ARG_ID_STACKDEC:
		if (ws->l_state.vertical_stacks > 1)
			ws->l_state.vertical_stacks--;
		break;
	default:
		return;
	}
}

void
vertical_stack(struct workspace *ws, struct twm_geometry *g)
{
	DNPRINTF(TWM_D_STACK, "vertical_stack: workspace: %d\n", ws->idx);

	stack_master(ws, g, 0, 0);
}

void
horizontal_config(struct workspace *ws, int id)
{
	DNPRINTF(TWM_D_STACK, "horizontal_config: workspace: %d\n", ws->idx);

	switch (id) {
	case TWM_ARG_ID_STACKRESET:
	case TWM_ARG_ID_STACKINIT:
		ws->l_state.horizontal_mwin = 1;
		ws->l_state.horizontal_msize = TWM_H_SLICE / 2;
		ws->l_state.horizontal_stacks = 1;
		break;
	case TWM_ARG_ID_MASTERSHRINK:
		if (ws->l_state.horizontal_msize > 1)
			ws->l_state.horizontal_msize--;
		break;
	case TWM_ARG_ID_MASTERGROW:
		if (ws->l_state.horizontal_msize < TWM_H_SLICE - 1)
			ws->l_state.horizontal_msize++;
		break;
	case TWM_ARG_ID_MASTERADD:
		ws->l_state.horizontal_mwin++;
		break;
	case TWM_ARG_ID_MASTERDEL:
		if (ws->l_state.horizontal_mwin > 0)
			ws->l_state.horizontal_mwin--;
		break;
	case TWM_ARG_ID_STACKINC:
		ws->l_state.horizontal_stacks++;
		break;
	case TWM_ARG_ID_STACKDEC:
		if (ws->l_state.horizontal_stacks > 1)
			ws->l_state.horizontal_stacks--;
		break;
	default:
		return;
	}
}

void
horizontal_stack(struct workspace *ws, struct twm_geometry *g)
{
	DNPRINTF(TWM_D_STACK, "horizontal_stack: workspace: %d\n", ws->idx);

	stack_master(ws, g, 1, 0);
}

/* fullscreen view */
void
max_stack(struct workspace *ws, struct twm_geometry *g)
{
	XWindowChanges		wc;
	struct twm_geometry	gg = *g;
	struct ws_win		*win, *wintrans = NULL, *parent = NULL;
	unsigned int		mask;
	int			winno;

	DNPRINTF(TWM_D_STACK, "max_stack: workspace: %d\n", ws->idx);

	if (ws == NULL)
		return;

	winno = count_win(ws, 0);
	if (winno == 0 && count_win(ws, 1) == 0)
		return;

	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient) {
			wintrans = win;
			parent = find_window(win->transient);
			continue;
		}

		if (win->floating && win->floatmaxed == 0 ) {
			/*
			 * retain geometry for retrieval on exit from
			 * max_stack mode
			 */
			store_float_geom(win, ws->r);
			win->floatmaxed = 1;
		}

		/* only reconfigure if necessary */
		if (win->g.x != gg.x || win->g.y != gg.y || win->g.w != gg.w ||
		    win->g.h != gg.h) {
			bzero(&wc, sizeof wc);
			win->g.x = wc.x = gg.x;
			win->g.y = wc.y = gg.y;
			if (bar_enabled){
				wc.border_width = 1;
				win->g.w = wc.width = gg.w;
				win->g.h = wc.height = gg.h;
			} else {
				wc.border_width = 0;
				win->g.w = wc.width = gg.w + 2;
				win->g.h = wc.height = gg.h + 2;
			}
			mask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
			XConfigureWindow(display, win->id, mask, &wc);
			configreq_win(win);
		}
		/* unmap only if we don't have multi screen */
		if (win != ws->focus)
			if (!(ScreenCount(display) > 1 || outputs > 1))
				unmap_window(win);
	}

	/* put the last transient on top */
	if (wintrans) {
		if (parent)
			XMapRaised(display, parent->id);
		stack_floater(wintrans, ws->r);
		focus_magic(wintrans, TWM_F_TRANSIENT);
	}
}

static inline char *char_p_of_unsigned_char_p(unsigned char *p)
{
	return (char *) p;
}

void
send_to_ws(struct twm_region *r, union arg *args)
{
	int			wsid = args->id;
	struct ws_win		*win = win;
	struct workspace	*ws, *nws;
	Atom			ws_idx_atom = 0;
	unsigned char		ws_idx_str[TWM_PROPLEN];
	union arg		a;

	if (r && r->ws)
		win = r->ws->focus;
	else
		return;
	if (win == NULL)
		return;
	if (win->ws->idx == wsid)
		return;

	DNPRINTF(TWM_D_MOVE, "send_to_ws: win: %lu\n", win->id);

	ws = win->ws;
	nws = &win->s->ws[wsid];

	a.id = TWM_ARG_ID_FOCUSPREV;
	focus(r, &a);
	unmap_window(win);
	TAILQ_REMOVE(&ws->winlist, win, entry);
	TAILQ_INSERT_TAIL(&nws->winlist, win, entry);
	win->ws = nws;

	/* Try to update the window's workspace property */
	ws_idx_atom = XInternAtom(display, "_TWM_WS", False);
	if (ws_idx_atom &&
	    snprintf(char_p_of_unsigned_char_p(ws_idx_str),
		    TWM_PROPLEN, "%d", nws->idx) < TWM_PROPLEN) {
		DNPRINTF(TWM_D_PROP, "setting property _TWM_WS to %s\n",
		    ws_idx_str);
		XChangeProperty(display, win->id, ws_idx_atom, XA_STRING, 8,
		    PropModeReplace, ws_idx_str, TWM_PROPLEN);
	}

	stack();
}

void
wkill(struct twm_region *r, union arg *args)
{
	DNPRINTF(TWM_D_MISC, "wkill %d\n", args->id);

	if (r->ws->focus == NULL)
		return;

	if (args->id == TWM_ARG_ID_KILLWINDOW)
		XKillClient(display, r->ws->focus->id);
	else
		if (r->ws->focus->can_delete)
			client_msg(r->ws->focus, adelete);
}


int
floating_toggle_win(struct ws_win *win)
{
	struct twm_region	*r;

	if (win == NULL)
		return 0;

	if (!win->ws->r)
		return 0;

	r = win->ws->r;

	/* reject floating toggles in max stack mode */
	if (win->ws->cur_layout == &layouts[TWM_MAX_STACK])
		return 0;

	if (win->floating) {
		if (!win->floatmaxed) {
			/* retain position for refloat */
			store_float_geom(win, r);
		}
		win->floating = 0;
	} else {
		if (win->g_floatvalid) {
			/* refloat at last floating relative position */
			win->g.x = win->g_float.x - win->rg_float.x + r->g.x;
			win->g.y = win->g_float.y - win->rg_float.y + r->g.y;
			win->g.w = win->g_float.w;
			win->g.h = win->g_float.h;
		}
		win->floating = 1;
	}

	ewmh_update_actions(win);

	return 1;
}

void
floating_toggle(struct twm_region *r, union arg *args)
{
	struct ws_win		*win = r->ws->focus;
	union arg		a;

	if (win == NULL)
		return;

	ewmh_update_win_state(win, ewmh[_NET_WM_STATE_ABOVE].atom,
	    _NET_WM_STATE_TOGGLE);

	stack();

	if (win == win->ws->focus) {
		a.id = TWM_ARG_ID_FOCUSCUR;
		focus(win->ws->r, &a);
	}
}

void
resize_window(struct ws_win *win, int center)
{
	unsigned int		mask;
	XWindowChanges		wc;
	struct twm_region	*r;

	r = root_to_region(win->wa.root);
	bzero(&wc, sizeof wc);
	mask = CWBorderWidth | CWWidth | CWHeight;
	wc.border_width = border_width;
	wc.width = win->g.w;
	wc.height = win->g.h;
	if (center == TWM_ARG_ID_CENTER) {
		wc.x = (WIDTH(r) - win->g.w) / 2;
		wc.y = (HEIGHT(r) - win->g.h) / 2;
		mask |= CWX | CWY;
	}

	DNPRINTF(TWM_D_STACK, "resize_window: win %lu x %d y %d w %d h %d\n",
	    win->id, wc.x, wc.y, wc.width, wc.height);

	XConfigureWindow(display, win->id, mask, &wc);
	configreq_win(win);
}

void
resize(struct ws_win *win, union arg *args)
{
	XEvent			ev;
	Time			time = 0;
	struct twm_region	*r = win->ws->r;
	int			relx, rely;
	union arg		a;


	DNPRINTF(TWM_D_MOUSE, "resize: win %lu floating %d trans %lu\n",
	    win->id, win->floating, win->transient);

	if (!(win->transient != 0 || win->floating != 0))
		return;

	/* reject resizes in max mode for floaters (transient ok) */
	if (win->floatmaxed)
		return;

	win->manual = 1;
	ewmh_update_win_state(win, ewmh[_TWM_WM_STATE_MANUAL].atom,
	    _NET_WM_STATE_ADD);
	/* raise the window = move to last in window list */
	a.id = TWM_ARG_ID_MOVELAST;
	swapwin(r, &a);
	stack();

	if (XGrabPointer(display, win->id, False, MOUSEMASK, GrabModeAsync,
	    GrabModeAsync, None, None /* cursor */, CurrentTime) != GrabSuccess)
		return;

	/* place pointer at bottom left corner or nearest point inside r */
	if ( win->g.x + win->g.w < r->g.x + r->g.w - 1)
		relx = win->g.w;
	else
		relx = r->g.x + r->g.w - win->g.x - 2;

	if ( win->g.y + win->g.h < r->g.y + r->g.h - 1)
		rely = win->g.h;
	else
		rely = r->g.y + r->g.h - win->g.y - 2;

	XWarpPointer(display, None, win->id, 0, 0, 0, 0, relx, rely);
	do {
		XMaskEvent(display, MOUSEMASK | ExposureMask |
		    SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			/* do not allow resize outside of region */
			if (	ev.xmotion.x_root < r->g.x ||
				ev.xmotion.x_root > r->g.x + r->g.w - 1 ||
				ev.xmotion.y_root < r->g.y ||
				ev.xmotion.y_root > r->g.y + r->g.h - 1)
				continue;

			if (ev.xmotion.x <= 1)
				ev.xmotion.x = 1;
			if (ev.xmotion.y <= 1)
				ev.xmotion.y = 1;
			win->g.w = ev.xmotion.x;
			win->g.h = ev.xmotion.y;

			/* not free, don't sync more than 60 times / second */
			if ((ev.xmotion.time - time) > (1000 / 60) ) {
				time = ev.xmotion.time;
				XSync(display, False);
				resize_window(win, args->id);
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	if (time) {
		XSync(display, False);
		resize_window(win, args->id);
	}
	store_float_geom(win,r);

	XWarpPointer(display, None, win->id, 0, 0, 0, 0, win->g.w - 1,
	    win->g.h - 1);
	XUngrabPointer(display, CurrentTime);

	/* drain events */
	while (XCheckMaskEvent(display, EnterWindowMask, &ev));
}

void
move_window(struct ws_win *win)
{
	unsigned int		mask;
	XWindowChanges		wc;
	struct twm_region	*r;

	r = root_to_region(win->wa.root);
	bzero(&wc, sizeof wc);
	mask = CWX | CWY;
	wc.x = win->g.x;
	wc.y = win->g.y;

	DNPRINTF(TWM_D_STACK, "move_window: win %lu x %d y %d w %d h %d\n",
	    win->id, wc.x, wc.y, wc.width, wc.height);

	XConfigureWindow(display, win->id, mask, &wc);
	configreq_win(win);
}

void
move(struct ws_win *win, union arg *args)
{
	XEvent			ev;
	Time			time = 0;
	struct twm_region	*r = win->ws->r;
	union arg		a;

	DNPRINTF(TWM_D_MOUSE, "move: win %lu floating %d trans %lu\n",
	    win->id, win->floating, win->transient);

	/* in max_stack mode should only move transients */
	if (win->ws->cur_layout == &layouts[TWM_MAX_STACK] && !win->transient)
		return;

	win->manual = 1;
	if (win->floating == 0 && !win->transient) {
		win->floating = 1;
		ewmh_update_win_state(win, ewmh[_NET_WM_STATE_ABOVE].atom,
		    _NET_WM_STATE_ADD);
	}
	ewmh_update_win_state(win, ewmh[_TWM_WM_STATE_MANUAL].atom,
	    _NET_WM_STATE_ADD);

	/* raise the window = move to last in window list */
	a.id = TWM_ARG_ID_MOVELAST;
	swapwin(r, &a);
	stack();

	if (XGrabPointer(display, win->id, False, MOUSEMASK, GrabModeAsync,
	    GrabModeAsync, None, None /* cursor */, CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(display, None, win->id, 0, 0, 0, 0, -1, -1);
	do {
		XMaskEvent(display, MOUSEMASK | ExposureMask |
		    SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			/* don't allow to move window origin out of region */
			if (	ev.xmotion.x_root < r->g.x ||
				ev.xmotion.x_root > r->g.x + r->g.w - 1 ||
				ev.xmotion.y_root < r->g.y ||
				ev.xmotion.y_root > r->g.y + r->g.h - 1)
				continue;

			win->g.x = ev.xmotion.x_root;
			win->g.y = ev.xmotion.y_root;

			/* not free, don't sync more than 60 times / second */
			if ((ev.xmotion.time - time) > (1000 / 60) ) {
				time = ev.xmotion.time;
				XSync(display, False);
				move_window(win);
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	if (time) {
		XSync(display, False);
		move_window(win);
	}
	store_float_geom(win,r);
	XWarpPointer(display, None, win->id, 0, 0, 0, 0, 0, 0);
	XUngrabPointer(display, CurrentTime);

	/* drain events */
	while (XCheckMaskEvent(display, EnterWindowMask, &ev));
}

/* user/key callable function IDs */
enum keyfuncid {
	kf_cycle_layout,
	kf_stack_reset,
	kf_master_shrink,
	kf_master_grow,
	kf_master_add,
	kf_master_del,
	kf_stack_inc,
	kf_stack_dec,
	kf_swap_main,
	kf_focus_next,
	kf_focus_prev,
	kf_swap_next,
	kf_swap_prev,
	kf_spawn_term,
	kf_spawn_menu,
	kf_quit,
	kf_restart,
	kf_focus_main,
	kf_ws_1,
	kf_ws_2,
	kf_ws_3,
	kf_ws_4,
	kf_ws_5,
	kf_ws_6,
	kf_ws_7,
	kf_ws_8,
	kf_ws_9,
	kf_ws_10,
	kf_ws_next,
	kf_ws_prev,
	kf_ws_prior,
	kf_screen_next,
	kf_screen_prev,
	kf_mvws_1,
	kf_mvws_2,
	kf_mvws_3,
	kf_mvws_4,
	kf_mvws_5,
	kf_mvws_6,
	kf_mvws_7,
	kf_mvws_8,
	kf_mvws_9,
	kf_mvws_10,
	kf_bar_toggle,
	kf_wind_kill,
	kf_wind_del,
	kf_screenshot_all,
	kf_screenshot_wind,
	kf_float_toggle,
	kf_version,
	kf_spawn_lock,
	kf_spawn_initscr,
	kf_spawn_custom,
	kf_dumpwins,
	kf_invalid
};

/* key definitions */
void
dummykeyfunc(struct twm_region *r, union arg *args)
{
};

void
legacyfunc(struct twm_region *r, union arg *args)
{
};

struct keyfunc {
	char			name[TWM_FUNCNAME_LEN];
	void			(*func)(struct twm_region *r, union arg *);
	union arg		args;
} keyfuncs[kf_invalid + 1] = {
	/* name			function	argument */
	{ "cycle_layout",	cycle_layout,	{0} },
	{ "stack_reset",	stack_config,	{.id = TWM_ARG_ID_STACKRESET} },
	{ "master_shrink",	stack_config,	{.id = TWM_ARG_ID_MASTERSHRINK} },
	{ "master_grow",	stack_config,	{.id = TWM_ARG_ID_MASTERGROW} },
	{ "master_add",		stack_config,	{.id = TWM_ARG_ID_MASTERADD} },
	{ "master_del",		stack_config,	{.id = TWM_ARG_ID_MASTERDEL} },
	{ "stack_inc",		stack_config,	{.id = TWM_ARG_ID_STACKINC} },
	{ "stack_dec",		stack_config,	{.id = TWM_ARG_ID_STACKDEC} },
	{ "swap_main",		swapwin,	{.id = TWM_ARG_ID_SWAPMAIN} },
	{ "focus_next",		focus,		{.id = TWM_ARG_ID_FOCUSNEXT} },
	{ "focus_prev",		focus,		{.id = TWM_ARG_ID_FOCUSPREV} },
	{ "swap_next",		swapwin,	{.id = TWM_ARG_ID_SWAPNEXT} },
	{ "swap_prev",		swapwin,	{.id = TWM_ARG_ID_SWAPPREV} },
	{ "spawn_term",		spawnterm,	{.argv = spawn_term} },
	{ "spawn_menu",		legacyfunc,	{0} },
	{ "quit",		quit,		{0} },
	{ "restart",		restart,	{0} },
	{ "focus_main",		focus,		{.id = TWM_ARG_ID_FOCUSMAIN} },
	{ "ws_1",		switchws,	{.id = 0} },
	{ "ws_2",		switchws,	{.id = 1} },
	{ "ws_3",		switchws,	{.id = 2} },
	{ "ws_4",		switchws,	{.id = 3} },
	{ "ws_5",		switchws,	{.id = 4} },
	{ "ws_6",		switchws,	{.id = 5} },
	{ "ws_7",		switchws,	{.id = 6} },
	{ "ws_8",		switchws,	{.id = 7} },
	{ "ws_9",		switchws,	{.id = 8} },
	{ "ws_10",		switchws,	{.id = 9} },
	{ "ws_next",		cyclews,	{.id = TWM_ARG_ID_CYCLEWS_UP} },
	{ "ws_prev",		cyclews,	{.id = TWM_ARG_ID_CYCLEWS_DOWN} },
	{ "ws_prior",		priorws,	{0} },
	{ "screen_next",	cyclescr,	{.id = TWM_ARG_ID_CYCLESC_UP} },
	{ "screen_prev",	cyclescr,	{.id = TWM_ARG_ID_CYCLESC_DOWN} },
	{ "mvws_1",		send_to_ws,	{.id = 0} },
	{ "mvws_2",		send_to_ws,	{.id = 1} },
	{ "mvws_3",		send_to_ws,	{.id = 2} },
	{ "mvws_4",		send_to_ws,	{.id = 3} },
	{ "mvws_5",		send_to_ws,	{.id = 4} },
	{ "mvws_6",		send_to_ws,	{.id = 5} },
	{ "mvws_7",		send_to_ws,	{.id = 6} },
	{ "mvws_8",		send_to_ws,	{.id = 7} },
	{ "mvws_9",		send_to_ws,	{.id = 8} },
	{ "mvws_10",		send_to_ws,	{.id = 9} },
	{ "bar_toggle",		bar_toggle,	{0} },
	{ "wind_kill",		wkill,		{.id = TWM_ARG_ID_KILLWINDOW} },
	{ "wind_del",		wkill,		{.id = TWM_ARG_ID_DELETEWINDOW} },
	{ "screenshot_all",	legacyfunc,	{0} },
	{ "screenshot_wind",	legacyfunc,	{0} },
	{ "float_toggle",	floating_toggle,{0} },
	{ "version",		version,	{0} },
	{ "spawn_lock",		legacyfunc,	{0} },
	{ "spawn_initscr",	legacyfunc,	{0} },
	{ "spawn_custom",	dummykeyfunc,	{0} },
	{ "dumpwins",		dumpwins,	{0} },
	{ "invalid key func",	NULL,		{0} },
};
struct key {
	unsigned int		mod;
	KeySym			keysym;
	enum keyfuncid		funcid;
	char			*spawn_name;
};
int				keys_size = 0, keys_length = 0;
struct key			*keys = NULL;

/* mouse */
enum { client_click, root_click };
struct button {
	unsigned int		action;
	unsigned int		mask;
	unsigned int		button;
	void			(*func)(struct ws_win *, union arg *);
	union arg		args;
} buttons[] = {
	  /* action	key		mouse button	func	args */
	{ client_click,	MODKEY,		Button3,	resize,	{.id = TWM_ARG_ID_DONTCENTER} },
	{ client_click,	MODKEY | ShiftMask, Button3,	resize,	{.id = TWM_ARG_ID_CENTER} },
	{ client_click,	MODKEY,		Button1,	move,	{0} },
};

void
update_modkey(unsigned int mod)
{
	int			i;

	mod_key = mod;
	for (i = 0; i < keys_length; i++)
		if (keys[i].mod & ShiftMask)
			keys[i].mod = mod | ShiftMask;
		else
			keys[i].mod = mod;

	for (i = 0; i < LENGTH(buttons); i++)
		if (buttons[i].mask & ShiftMask)
			buttons[i].mask = mod | ShiftMask;
		else
			buttons[i].mask = mod;
}

/* spawn */
struct spawn_prog {
	char			*name;
	int			argc;
	char			**argv;
};

int				spawns_size = 0, spawns_length = 0;
struct spawn_prog		*spawns = NULL;

void
spawn_custom(struct twm_region *r, union arg *args, char *spawn_name)
{
	union arg		a;
	struct spawn_prog	*prog = NULL;
	int			i;
	char			*ap, **real_args;

	DNPRINTF(TWM_D_SPAWN, "spawn_custom %s\n", spawn_name);

	/* find program */
	for (i = 0; i < spawns_length; i++) {
		if (!strcasecmp(spawn_name, spawns[i].name))
			prog = &spawns[i];
	}
	if (prog == NULL) {
		fprintf(stderr, "spawn_custom: program %s not found\n",
		    spawn_name);
		return;
	}

	/* make room for expanded args */
	if ((real_args = calloc(prog->argc + 1, sizeof(char *))) == NULL)
		err(1, "spawn_custom: calloc real_args");

	/* expand spawn_args into real_args */
	for (i = 0; i < prog->argc; i++) {
		ap = prog->argv[i];
		DNPRINTF(TWM_D_SPAWN, "spawn_custom: raw arg = %s\n", ap);
		if (!strcasecmp(ap, "$bar_border")) {
			if ((real_args[i] =
			    strdup(r->s->c[TWM_S_COLOR_BAR_BORDER].name))
			    == NULL)
				err(1,  "spawn_custom border color");
		} else if (!strcasecmp(ap, "$bar_color")) {
			if ((real_args[i] =
			    strdup(r->s->c[TWM_S_COLOR_BAR].name))
			    == NULL)
				err(1, "spawn_custom bar color");
		} else if (!strcasecmp(ap, "$bar_font")) {
			if ((real_args[i] = strdup(bar_fonts[bar_fidx]))
			    == NULL)
				err(1, "spawn_custom bar fonts");
		} else if (!strcasecmp(ap, "$bar_font_color")) {
			if ((real_args[i] =
			    strdup(r->s->c[TWM_S_COLOR_BAR_FONT].name))
			    == NULL)
				err(1, "spawn_custom color font");
		} else if (!strcasecmp(ap, "$color_focus")) {
			if ((real_args[i] =
			    strdup(r->s->c[TWM_S_COLOR_FOCUS].name))
			    == NULL)
				err(1, "spawn_custom color focus");
		} else if (!strcasecmp(ap, "$color_unfocus")) {
			if ((real_args[i] =
			    strdup(r->s->c[TWM_S_COLOR_UNFOCUS].name))
			    == NULL)
				err(1, "spawn_custom color unfocus");
		} else {
			/* no match --> copy as is */
			if ((real_args[i] = strdup(ap)) == NULL)
				err(1, "spawn_custom strdup(ap)");
		}
		DNPRINTF(TWM_D_SPAWN, "spawn_custom: cooked arg = %s\n",
		    real_args[i]);
	}

#ifdef TWM_DEBUG
	if ((twm_debug & TWM_D_SPAWN) != 0) {
		fprintf(stderr, "spawn_custom: result = ");
		for (i = 0; i < prog->argc; i++)
			fprintf(stderr, "\"%s\" ", real_args[i]);
		fprintf(stderr, "\n");
	}
#endif

	a.argv = real_args;
	spawn(r, &a);
	for (i = 0; i < prog->argc; i++)
		free(real_args[i]);
	free(real_args);
}

void
setspawn(struct spawn_prog *prog)
{
	int			i, j;

	if (prog == NULL || prog->name == NULL)
		return;

	/* find existing */
	for (i = 0; i < spawns_length; i++) {
		if (!strcmp(spawns[i].name, prog->name)) {
			/* found */
			if (prog->argv == NULL) {
				/* delete */
				DNPRINTF(TWM_D_SPAWN,
				    "setspawn: delete #%d %s\n",
				    i, spawns[i].name);
				free(spawns[i].name);
				for (j = 0; j < spawns[i].argc; j++)
					free(spawns[i].argv[j]);
				free(spawns[i].argv);
				j = spawns_length - 1;
				if (i < j)
					spawns[i] = spawns[j];
				spawns_length--;
				free(prog->name);
			} else {
				/* replace */
				DNPRINTF(TWM_D_SPAWN,
				    "setspawn: replace #%d %s\n",
				    i, spawns[i].name);
				free(spawns[i].name);
				for (j = 0; j < spawns[i].argc; j++)
					free(spawns[i].argv[j]);
				free(spawns[i].argv);
				spawns[i] = *prog;
			}
			/* found case handled */
			free(prog);
			return;
		}
	}

	if (prog->argv == NULL) {
		fprintf(stderr,
		    "error: setspawn: cannot find program %s", prog->name);
		free(prog);
		return;
	}

	/* not found: add */
	if (spawns_size == 0 || spawns == NULL) {
		spawns_size = 4;
		DNPRINTF(TWM_D_SPAWN, "setspawn: init list %d\n", spawns_size);
		spawns = malloc((size_t)spawns_size *
		    sizeof(struct spawn_prog));
		if (spawns == NULL) {
			fprintf(stderr, "setspawn: malloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	} else if (spawns_length == spawns_size) {
		spawns_size *= 2;
		DNPRINTF(TWM_D_SPAWN, "setspawn: grow list %d\n", spawns_size);
		spawns = realloc(spawns, (size_t)spawns_size *
		    sizeof(struct spawn_prog));
		if (spawns == NULL) {
			fprintf(stderr, "setspawn: realloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	}

	if (spawns_length < spawns_size) {
		DNPRINTF(TWM_D_SPAWN, "setspawn: add #%d %s\n",
		    spawns_length, prog->name);
		i = spawns_length++;
		spawns[i] = *prog;
	} else {
		fprintf(stderr, "spawns array problem?\n");
		if (spawns == NULL) {
			fprintf(stderr, "spawns array is NULL!\n");
			quit(NULL, NULL);
		}
	}
	free(prog);
}

int
setconfspawn(char *selector, char *value, int flags)
{
	struct spawn_prog	*prog;
	char			*vp, *cp, *word;

	DNPRINTF(TWM_D_SPAWN, "setconfspawn: [%s] [%s]\n", selector, value);
	if ((prog = calloc(1, sizeof *prog)) == NULL)
		err(1, "setconfspawn: calloc prog");
	prog->name = strdup(selector);
	if (prog->name == NULL)
		err(1, "setconfspawn prog->name");
	if ((cp = vp = strdup(value)) == NULL)
		err(1, "setconfspawn: strdup(value) ");
	while ((word = strsep(&cp, " \t")) != NULL) {
		DNPRINTF(TWM_D_SPAWN, "setconfspawn: arg [%s]\n", word);
		if (cp)
			cp += (long)strspn(cp, " \t");
		if (strlen(word) > 0) {
			prog->argc++;
			prog->argv = realloc(prog->argv,
			    prog->argc * sizeof(char *));
			if ((prog->argv[prog->argc - 1] = strdup(word)) == NULL)
				err(1, "setconfspawn: strdup");
		}
	}
	free(vp);

	setspawn(prog);

	DNPRINTF(TWM_D_SPAWN, "setconfspawn: done\n");
	return (0);
}

void
setup_spawn(void)
{
	setconfspawn("term",		"xterm",		0);
	setconfspawn("screenshot_all",	"screenshot.sh full",	0);
	setconfspawn("screenshot_wind",	"screenshot.sh window",	0);
	setconfspawn("lock",		"xlock",		0);
	setconfspawn("initscr",		"initscreen.sh",	0);
	setconfspawn("menu",		"dmenu_run"
					" -fn $bar_font"
					" -nb $bar_color"
					" -nf $bar_font_color"
					" -sb $bar_border"
					" -sf $bar_color",	0);
}

/* key bindings */
#define TWM_MODNAME_SIZE	32
#define	TWM_KEY_WS		"\n+ \t"
int
parsekeys(char *keystr, unsigned int currmod, unsigned int *mod, KeySym *ks)
{
	char			*cp, *name;
	KeySym			uks;
	DNPRINTF(TWM_D_KEY, "parsekeys: enter [%s]\n", keystr);
	if (mod == NULL || ks == NULL) {
		DNPRINTF(TWM_D_KEY, "parsekeys: no mod or key vars\n");
		return (1);
	}
	if (keystr == NULL || strlen(keystr) == 0) {
		DNPRINTF(TWM_D_KEY, "parsekeys: no keystr\n");
		return (1);
	}
	cp = keystr;
	*ks = NoSymbol;
	*mod = 0;
	while ((name = strsep(&cp, TWM_KEY_WS)) != NULL) {
		DNPRINTF(TWM_D_KEY, "parsekeys: key [%s]\n", name);
		if (cp)
			cp += (long)strspn(cp, TWM_KEY_WS);
		if (strncasecmp(name, "MOD", TWM_MODNAME_SIZE) == 0)
			*mod |= currmod;
		else if (!strncasecmp(name, "Mod1", TWM_MODNAME_SIZE))
			*mod |= Mod1Mask;
		else if (!strncasecmp(name, "Mod2", TWM_MODNAME_SIZE))
			*mod += Mod2Mask;
		else if (!strncmp(name, "Mod3", TWM_MODNAME_SIZE))
			*mod |= Mod3Mask;
		else if (!strncmp(name, "Mod4", TWM_MODNAME_SIZE))
			*mod |= Mod4Mask;
		else if (strncasecmp(name, "SHIFT", TWM_MODNAME_SIZE) == 0)
			*mod |= ShiftMask;
		else if (strncasecmp(name, "CONTROL", TWM_MODNAME_SIZE) == 0)
			*mod |= ControlMask;
		else {
			*ks = XStringToKeysym(name);
			XConvertCase(*ks, ks, &uks);
			if (ks == NoSymbol) {
				DNPRINTF(TWM_D_KEY,
				    "parsekeys: invalid key %s\n",
				    name);
				return (1);
			}
		}
	}
	DNPRINTF(TWM_D_KEY, "parsekeys: leave ok\n");
	return (0);
}

char *
strdupsafe(char *str)
{
	if (str == NULL)
		return (NULL);
	else
		return (strdup(str));
}

void
setkeybinding(unsigned int mod, KeySym ks, enum keyfuncid kfid, char *spawn_name)
{
	int			i, j;
	DNPRINTF(TWM_D_KEY, "setkeybinding: enter %s [%s]\n",
	    keyfuncs[kfid].name, spawn_name);
	/* find existing */
	for (i = 0; i < keys_length; i++) {
		if (keys[i].mod == mod && keys[i].keysym == ks) {
			if (kfid == kf_invalid) {
				/* found: delete */
				DNPRINTF(TWM_D_KEY,
				    "setkeybinding: delete #%d %s\n",
				    i, keyfuncs[keys[i].funcid].name);
				free(keys[i].spawn_name);
				j = keys_length - 1;
				if (i < j)
					keys[i] = keys[j];
				keys_length--;
				DNPRINTF(TWM_D_KEY, "setkeybinding: leave\n");
				return;
			} else {
				/* found: replace */
				DNPRINTF(TWM_D_KEY,
				    "setkeybinding: replace #%d %s %s\n",
				    i, keyfuncs[keys[i].funcid].name,
				    spawn_name);
				free(keys[i].spawn_name);
				keys[i].mod = mod;
				keys[i].keysym = ks;
				keys[i].funcid = kfid;
				keys[i].spawn_name = strdupsafe(spawn_name);
				DNPRINTF(TWM_D_KEY, "setkeybinding: leave\n");
				return;
			}
		}
	}
	if (kfid == kf_invalid) {
		fprintf(stderr,
		    "error: setkeybinding: cannot find mod/key combination");
		DNPRINTF(TWM_D_KEY, "setkeybinding: leave\n");
		return;
	}
	/* not found: add */
	if (keys_size == 0 || keys == NULL) {
		keys_size = 4;
		DNPRINTF(TWM_D_KEY, "setkeybinding: init list %d\n", keys_size);
		keys = malloc((size_t)keys_size * sizeof(struct key));
		if (!keys) {
			fprintf(stderr, "malloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	} else if (keys_length == keys_size) {
		keys_size *= 2;
		DNPRINTF(TWM_D_KEY, "setkeybinding: grow list %d\n", keys_size);
		keys = realloc(keys, (size_t)keys_size * sizeof(struct key));
		if (!keys) {
			fprintf(stderr, "realloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	}
	if (keys_length < keys_size) {
		j = keys_length++;
		DNPRINTF(TWM_D_KEY, "setkeybinding: add #%d %s %s\n",
		    j, keyfuncs[kfid].name, spawn_name);
		keys[j].mod = mod;
		keys[j].keysym = ks;
		keys[j].funcid = kfid;
		keys[j].spawn_name = strdupsafe(spawn_name);
	} else {
		fprintf(stderr, "keys array problem?\n");
		if (!keys) {
			fprintf(stderr, "keys array problem\n");
			quit(NULL, NULL);
		}
	}
	DNPRINTF(TWM_D_KEY, "setkeybinding: leave\n");
}

int
setconfbinding(char *selector, char *value, int flags)
{
	enum keyfuncid		kfid;
	unsigned int		mod;
	KeySym			ks;
	int			i;
	DNPRINTF(TWM_D_KEY, "setconfbinding: enter\n");
	if (selector == NULL) {
		DNPRINTF(TWM_D_KEY, "setconfbinding: unbind %s\n", value);
		if (parsekeys(value, mod_key, &mod, &ks) == 0) {
			kfid = kf_invalid;
			setkeybinding(mod, ks, kfid, NULL);
			return (0);
		} else
			return (1);
	}
	/* search by key function name */
	for (kfid = 0; kfid < kf_invalid; (kfid)++) {
		if (strncasecmp(selector, keyfuncs[kfid].name,
		    TWM_FUNCNAME_LEN) == 0) {
			DNPRINTF(TWM_D_KEY, "setconfbinding: %s: match\n",
			    selector);
			if (parsekeys(value, mod_key, &mod, &ks) == 0) {
				setkeybinding(mod, ks, kfid, NULL);
				return (0);
			} else
				return (1);
		}
	}
	/* search by custom spawn name */
	for (i = 0; i < spawns_length; i++) {
		if (strcasecmp(selector, spawns[i].name) == 0) {
			DNPRINTF(TWM_D_KEY, "setconfbinding: %s: match\n",
			    selector);
			if (parsekeys(value, mod_key, &mod, &ks) == 0) {
				setkeybinding(mod, ks, kf_spawn_custom,
				    spawns[i].name);
				return (0);
			} else
				return (1);
		}
	}
	DNPRINTF(TWM_D_KEY, "setconfbinding: no match\n");
	return (1);
}

void
setup_keys(void)
{
	setkeybinding(MODKEY,		XK_space,	kf_cycle_layout,NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_space,	kf_stack_reset,	NULL);
	setkeybinding(MODKEY,		XK_h,		kf_master_shrink,NULL);
	setkeybinding(MODKEY,		XK_l,		kf_master_grow,	NULL);
	setkeybinding(MODKEY,		XK_comma,	kf_master_add,	NULL);
	setkeybinding(MODKEY,		XK_period,	kf_master_del,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_comma,	kf_stack_inc,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_period,	kf_stack_dec,	NULL);
	setkeybinding(MODKEY,		XK_Return,	kf_swap_main,	NULL);
	setkeybinding(MODKEY,		XK_j,		kf_focus_next,	NULL);
	setkeybinding(MODKEY,		XK_k,		kf_focus_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_j,		kf_swap_next,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_k,		kf_swap_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Return,	kf_spawn_term,	NULL);
	setkeybinding(MODKEY,		XK_p,		kf_spawn_custom,	"menu");
	setkeybinding(MODKEY|ShiftMask,	XK_q,		kf_quit,	NULL);
	setkeybinding(MODKEY,		XK_q,		kf_restart,	NULL);
	setkeybinding(MODKEY,		XK_m,		kf_focus_main,	NULL);
	setkeybinding(MODKEY,		XK_1,		kf_ws_1,	NULL);
	setkeybinding(MODKEY,		XK_2,		kf_ws_2,	NULL);
	setkeybinding(MODKEY,		XK_3,		kf_ws_3,	NULL);
	setkeybinding(MODKEY,		XK_4,		kf_ws_4,	NULL);
	setkeybinding(MODKEY,		XK_5,		kf_ws_5,	NULL);
	setkeybinding(MODKEY,		XK_6,		kf_ws_6,	NULL);
	setkeybinding(MODKEY,		XK_7,		kf_ws_7,	NULL);
	setkeybinding(MODKEY,		XK_8,		kf_ws_8,	NULL);
	setkeybinding(MODKEY,		XK_9,		kf_ws_9,	NULL);
	setkeybinding(MODKEY,		XK_0,		kf_ws_10,	NULL);
	setkeybinding(MODKEY,		XK_Right,	kf_ws_next,	NULL);
	setkeybinding(MODKEY,		XK_Left,	kf_ws_prev,	NULL);
	setkeybinding(MODKEY,		XK_a,		kf_ws_prior,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Right,	kf_screen_next,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Left,	kf_screen_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_1,		kf_mvws_1,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_2,		kf_mvws_2,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_3,		kf_mvws_3,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_4,		kf_mvws_4,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_5,		kf_mvws_5,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_6,		kf_mvws_6,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_7,		kf_mvws_7,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_8,		kf_mvws_8,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_9,		kf_mvws_9,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_0,		kf_mvws_10,	NULL);
	setkeybinding(MODKEY,		XK_b,		kf_bar_toggle,	NULL);
	setkeybinding(MODKEY,		XK_Tab,		kf_focus_next,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Tab,		kf_focus_prev,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_x,		kf_wind_kill,	NULL);
	setkeybinding(MODKEY,		XK_x,		kf_wind_del,	NULL);
	setkeybinding(MODKEY,		XK_s,		kf_spawn_custom,	"screenshot_all");
	setkeybinding(MODKEY|ShiftMask,	XK_s,		kf_spawn_custom,	"screenshot_wind");
	setkeybinding(MODKEY,		XK_t,		kf_float_toggle,NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_v,		kf_version,	NULL);
	setkeybinding(MODKEY|ShiftMask,	XK_Delete,	kf_spawn_custom,	"lock");
	setkeybinding(MODKEY|ShiftMask,	XK_i,		kf_spawn_custom,	"initscr");
#ifdef TWM_DEBUG
	setkeybinding(MODKEY|ShiftMask,	XK_d,		kf_dumpwins,	NULL);
#endif
}

void
updatenumlockmask(void)
{
	unsigned int		i, j;
	XModifierKeymap		*modmap;

	DNPRINTF(TWM_D_MISC, "updatenumlockmask\n");
	numlockmask = 0;
	modmap = XGetModifierMapping(display);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
			  == XKeysymToKeycode(display, XK_Num_Lock))
				numlockmask = (1 << i);

	XFreeModifiermap(modmap);
}

void
grabkeys(void)
{
	unsigned int		i, j, k;
	KeyCode			code;
	unsigned int		modifiers[] =
	    { 0, LockMask, numlockmask, numlockmask | LockMask };

	DNPRINTF(TWM_D_MISC, "grabkeys\n");
	updatenumlockmask();

	for (k = 0; k < ScreenCount(display); k++) {
		if (TAILQ_EMPTY(&screens[k].rl))
			continue;
		XUngrabKey(display, AnyKey, AnyModifier, screens[k].root);
		for (i = 0; i < keys_length; i++) {
			if ((code = XKeysymToKeycode(display, keys[i].keysym)))
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(display, code,
					    keys[i].mod | modifiers[j],
					    screens[k].root, True,
					    GrabModeAsync, GrabModeAsync);
		}
	}
}

void
grabbuttons(struct ws_win *win, int focused)
{
	unsigned int		i, j;
	unsigned int		modifiers[] =
	    { 0, LockMask, numlockmask, numlockmask|LockMask };

	updatenumlockmask();
	XUngrabButton(display, AnyButton, AnyModifier, win->id);
	if (focused) {
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].action == client_click)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(display, buttons[i].button,
					    buttons[i].mask | modifiers[j],
					    win->id, False, BUTTONMASK,
					    GrabModeAsync, GrabModeSync, None,
					    None);
	} else
		XGrabButton(display, AnyButton, AnyModifier, win->id, False,
		    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
}

const char *quirkname[] = {
	"NONE",		/* config string for "no value" */
	"FLOAT",
	"TRANSSZ",
	"ANYWHERE",
	"XTERM_FONTADJ",
	"FULLSCREEN",
};

/* TWM_Q_WS: retain '|' for back compat for now (2009-08-11) */
#define	TWM_Q_WS		"\n|+ \t"
int
parsequirks(char *qstr, unsigned long *quirk)
{
	char			*cp, *name;
	int			i;

	if (quirk == NULL)
		return (1);

	cp = qstr;
	*quirk = 0;
	while ((name = strsep(&cp, TWM_Q_WS)) != NULL) {
		if (cp)
			cp += (long)strspn(cp, TWM_Q_WS);
		for (i = 0; i < LENGTH(quirkname); i++) {
			if (!strncasecmp(name, quirkname[i], TWM_QUIRK_LEN)) {
				DNPRINTF(TWM_D_QUIRK, "parsequirks: %s\n", name);
				if (i == 0) {
					*quirk = 0;
					return (0);
				}
				*quirk |= 1 << (i-1);
				break;
			}
		}
		if (i >= LENGTH(quirkname)) {
			DNPRINTF(TWM_D_QUIRK,
			    "parsequirks: invalid quirk [%s]\n", name);
			return (1);
		}
	}
	return (0);
}

void
setquirk(const char *class, const char *name, const int quirk)
{
	int			i, j;

	/* find existing */
	for (i = 0; i < quirks_length; i++) {
		if (!strcmp(quirks[i].class, class) &&
		    !strcmp(quirks[i].name, name)) {
			if (!quirk) {
				/* found: delete */
				DNPRINTF(TWM_D_QUIRK,
				    "setquirk: delete #%d %s:%s\n",
				    i, quirks[i].class, quirks[i].name);
				free(quirks[i].class);
				free(quirks[i].name);
				j = quirks_length - 1;
				if (i < j)
					quirks[i] = quirks[j];
				quirks_length--;
				return;
			} else {
				/* found: replace */
				DNPRINTF(TWM_D_QUIRK,
				    "setquirk: replace #%d %s:%s\n",
				    i, quirks[i].class, quirks[i].name);
				free(quirks[i].class);
				free(quirks[i].name);
				quirks[i].class = strdup(class);
				quirks[i].name = strdup(name);
				quirks[i].quirk = quirk;
				return;
			}
		}
	}
	if (!quirk) {
		fprintf(stderr,
		    "error: setquirk: cannot find class/name combination");
		return;
	}
	/* not found: add */
	if (quirks_size == 0 || quirks == NULL) {
		quirks_size = 4;
		DNPRINTF(TWM_D_QUIRK, "setquirk: init list %d\n", quirks_size);
		quirks = malloc((size_t)quirks_size * sizeof(struct quirk));
		if (!quirks) {
			fprintf(stderr, "setquirk: malloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	} else if (quirks_length == quirks_size) {
		quirks_size *= 2;
		DNPRINTF(TWM_D_QUIRK, "setquirk: grow list %d\n", quirks_size);
		quirks = realloc(quirks, (size_t)quirks_size * sizeof(struct quirk));
		if (!quirks) {
			fprintf(stderr, "setquirk: realloc failed\n");
			perror(" failed");
			quit(NULL, NULL);
		}
	}
	if (quirks_length < quirks_size) {
		DNPRINTF(TWM_D_QUIRK, "setquirk: add %d\n", quirks_length);
		j = quirks_length++;
		quirks[j].class = strdup(class);
		quirks[j].name = strdup(name);
		quirks[j].quirk = quirk;
	} else {
		fprintf(stderr, "quirks array problem?\n");
		if (!quirks) {
			fprintf(stderr, "quirks array problem!\n");
			quit(NULL, NULL);
		}
	}
}

int
setconfquirk(char *selector, char *value, int flags)
{
	char			*cp, *class, *name;
	int			retval;
	unsigned long		quirks;
	if (selector == NULL)
		return (0);
	if ((cp = strchr(selector, ':')) == NULL)
		return (0);
	*cp = '\0';
	class = selector;
	name = cp + 1;
	if ((retval = parsequirks(value, &quirks)) == 0)
		setquirk(class, name, quirks);
	return (retval);
}

void
setup_quirks(void)
{
	setquirk("MPlayer",		"xv",		TWM_Q_FLOAT | TWM_Q_FULLSCREEN);
	setquirk("OpenOffice.org 3.2",	"VCLSalFrame",	TWM_Q_FLOAT);
	setquirk("Firefox-bin",		"firefox-bin",	TWM_Q_TRANSSZ);
	setquirk("Firefox",		"Dialog",	TWM_Q_FLOAT);
	setquirk("Gimp",		"gimp",		TWM_Q_FLOAT | TWM_Q_ANYWHERE);
	setquirk("XTerm",		"xterm",	TWM_Q_XTERM_FONTADJ);
	setquirk("xine",		"Xine Window",	TWM_Q_FLOAT | TWM_Q_ANYWHERE);
	setquirk("Xitk",		"Xitk Combo",	TWM_Q_FLOAT | TWM_Q_ANYWHERE);
	setquirk("xine",		"xine Panel",	TWM_Q_FLOAT | TWM_Q_ANYWHERE);
	setquirk("Xitk",		"Xine Window",	TWM_Q_FLOAT | TWM_Q_ANYWHERE);
	setquirk("xine",		"xine Video Fullscreen Window",	TWM_Q_FULLSCREEN | TWM_Q_FLOAT);
	setquirk("pcb",			"pcb",		TWM_Q_FLOAT);
}

/* conf file stuff */
#define TWM_CONF_FILE	"torba.conf"

enum	{ TWM_S_BAR_DELAY, TWM_S_BAR_ENABLED, TWM_S_STACK_ENABLED,
	  TWM_S_CLOCK_ENABLED, TWM_S_CLOCK_FORMAT, TWM_S_CYCLE_EMPTY,
	  TWM_S_CYCLE_VISIBLE, TWM_S_SS_ENABLED, TWM_S_TERM_WIDTH,
	  TWM_S_TITLE_CLASS_ENABLED, TWM_S_TITLE_NAME_ENABLED, TWM_S_WINDOW_NAME_ENABLED,
	  TWM_S_FOCUS_MODE, TWM_S_DISABLE_BORDER, TWM_S_BORDER_WIDTH, TWM_S_BAR_FONT,
	  TWM_S_BAR_ACTION, TWM_S_SPAWN_TERM, TWM_S_SS_APP, TWM_S_DIALOG_RATIO,
	  TWM_S_BAR_AT_BOTTOM
	};

int
setconfvalue(char *selector, char *value, int flags)
{
	switch (flags) {
	case TWM_S_BAR_DELAY:
		bar_delay = atoi(value);
		break;
	case TWM_S_BAR_ENABLED:
		bar_enabled = atoi(value);
		break;
	case TWM_S_BAR_AT_BOTTOM:
		bar_at_bottom = atoi(value);
		break;
	case TWM_S_STACK_ENABLED:
		stack_enabled = atoi(value);
		break;
	case TWM_S_CLOCK_ENABLED:
		clock_enabled = atoi(value);
		break;
	case TWM_S_CLOCK_FORMAT:
#ifndef TWM_DENY_CLOCK_FORMAT
		free(clock_format);
		if ((clock_format = strdup(value)) == NULL)
			err(1, "setconfvalue: clock_format");
#endif
		break;
	case TWM_S_CYCLE_EMPTY:
		cycle_empty = atoi(value);
		break;
	case TWM_S_CYCLE_VISIBLE:
		cycle_visible = atoi(value);
		break;
	case TWM_S_SS_ENABLED:
		ss_enabled = atoi(value);
		break;
	case TWM_S_TERM_WIDTH:
		term_width = atoi(value);
		break;
	case TWM_S_TITLE_CLASS_ENABLED:
		title_class_enabled = atoi(value);
		break;
	case TWM_S_WINDOW_NAME_ENABLED:
		window_name_enabled = atoi(value);
		break;
	case TWM_S_TITLE_NAME_ENABLED:
		title_name_enabled = atoi(value);
		break;
	case TWM_S_FOCUS_MODE:
		if (!strcmp(value, "default"))
			focus_mode = TWM_FOCUS_DEFAULT;
		else if (!strcmp(value, "follow_cursor"))
			focus_mode = TWM_FOCUS_FOLLOW;
		else if (!strcmp(value, "synergy"))
			focus_mode = TWM_FOCUS_SYNERGY;
		else
			err(1, "focus_mode");
		break;
	case TWM_S_DISABLE_BORDER:
		disable_border = atoi(value);
		break;
	case TWM_S_BORDER_WIDTH:
		border_width = atoi(value);
		break;
	case TWM_S_BAR_FONT:
		free(bar_fonts[0]);
		if ((bar_fonts[0] = strdup(value)) == NULL)
			err(1, "setconfvalue: bar_font");
		break;
	case TWM_S_BAR_ACTION:
		free(bar_argv[0]);
		if ((bar_argv[0] = strdup(value)) == NULL)
			err(1, "setconfvalue: bar_action");
		break;
	case TWM_S_SPAWN_TERM:
		free(spawn_term[0]);
		if ((spawn_term[0] = strdup(value)) == NULL)
			err(1, "setconfvalue: spawn_term");
		break;
	case TWM_S_SS_APP:
		break;
	case TWM_S_DIALOG_RATIO:
		dialog_ratio = atof(value);
		if (dialog_ratio > 1.0 || dialog_ratio <= .3)
			dialog_ratio = .6;
		break;
	default:
		return (1);
	}
	return (0);
}

int
setconfmodkey(char *selector, char *value, int flags)
{
	if (!strncasecmp(value, "Mod1", strlen("Mod1")))
		update_modkey(Mod1Mask);
	else if (!strncasecmp(value, "Mod2", strlen("Mod2")))
		update_modkey(Mod2Mask);
	else if (!strncasecmp(value, "Mod3", strlen("Mod3")))
		update_modkey(Mod3Mask);
	else if (!strncasecmp(value, "Mod4", strlen("Mod4")))
		update_modkey(Mod4Mask);
	else
		return (1);
	return (0);
}

int
setconfcolor(char *selector, char *value, int flags)
{
	setscreencolor(value, ((selector == NULL)?-1:atoi(selector)), flags);
	return (0);
}

int
setconfregion(char *selector, char *value, int flags)
{
	custom_region(value);
	return (0);
}

/* config options */
struct config_option {
	char			*optname;
	int (*func)(char*, char*, int);
	int funcflags;
};
struct config_option configopt[] = {
	{ "bar_enabled",		setconfvalue,	TWM_S_BAR_ENABLED },
	{ "bar_at_bottom",		setconfvalue,	TWM_S_BAR_AT_BOTTOM },
	{ "bar_border",			setconfcolor,	TWM_S_COLOR_BAR_BORDER },
	{ "bar_color",			setconfcolor,	TWM_S_COLOR_BAR },
	{ "bar_font_color",		setconfcolor,	TWM_S_COLOR_BAR_FONT },
	{ "bar_font",			setconfvalue,	TWM_S_BAR_FONT },
	{ "bar_action",			setconfvalue,	TWM_S_BAR_ACTION },
	{ "bar_delay",			setconfvalue,	TWM_S_BAR_DELAY },
	{ "bind",			setconfbinding,	0 },
	{ "stack_enabled",		setconfvalue,	TWM_S_STACK_ENABLED },
	{ "clock_enabled",		setconfvalue,	TWM_S_CLOCK_ENABLED },
	{ "clock_format",		setconfvalue,	TWM_S_CLOCK_FORMAT },
	{ "color_focus",		setconfcolor,	TWM_S_COLOR_FOCUS },
	{ "color_unfocus",		setconfcolor,	TWM_S_COLOR_UNFOCUS },
	{ "cycle_empty",		setconfvalue,	TWM_S_CYCLE_EMPTY },
	{ "cycle_visible",		setconfvalue,	TWM_S_CYCLE_VISIBLE },
	{ "dialog_ratio",		setconfvalue,	TWM_S_DIALOG_RATIO },
	{ "modkey",			setconfmodkey,	0 },
	{ "program",			setconfspawn,	0 },
	{ "quirk",			setconfquirk,	0 },
	{ "region",			setconfregion,	0 },
	{ "spawn_term",			setconfvalue,	TWM_S_SPAWN_TERM },
	{ "screenshot_enabled",		setconfvalue,	TWM_S_SS_ENABLED },
	{ "screenshot_app",		setconfvalue,	TWM_S_SS_APP },
	{ "window_name_enabled",	setconfvalue,	TWM_S_WINDOW_NAME_ENABLED },
	{ "term_width",			setconfvalue,	TWM_S_TERM_WIDTH },
	{ "title_class_enabled",	setconfvalue,	TWM_S_TITLE_CLASS_ENABLED },
	{ "title_name_enabled",		setconfvalue,	TWM_S_TITLE_NAME_ENABLED },
	{ "focus_mode",			setconfvalue,   TWM_S_FOCUS_MODE },
	{ "disable_border",		setconfvalue,   TWM_S_DISABLE_BORDER },
	{ "border_width",		setconfvalue,	TWM_S_BORDER_WIDTH },
};


int
conf_load(char *filename)
{
	FILE			*config;
	char			*line, *cp, *optsub, *optval;
	size_t			linelen, lineno = 0;
	int			wordlen, i, optind;
	struct config_option	*opt;

	DNPRINTF(TWM_D_CONF, "conf_load begin\n");

	if (filename == NULL) {
		fprintf(stderr, "conf_load: no filename\n");
		return (1);
	}
	if ((config = fopen(filename, "r")) == NULL) {
		warn("conf_load: fopen");
		return (1);
	}

	while (!feof(config)) {
		if ((line = fparseln(config, &linelen, &lineno, NULL, 0))
		    == NULL) {
			if (ferror(config))
				err(1, "%s", filename);
			else
				continue;
		}
		cp = line;
		cp += strspn(cp, " \t\n"); /* eat whitespace */
		if (cp[0] == '\0') {
			/* empty line */
			free(line);
			continue;
		}
		/* get config option */
		wordlen = strcspn(cp, "=[ \t\n");
		if (wordlen == 0) {
			warnx("%s: line %zd: no option found",
			    filename, lineno);
			return (1);
		}
		optind = -1;
		for (i = 0; i < LENGTH(configopt); i++) {
			opt = &configopt[i];
			if (!strncasecmp(cp, opt->optname, wordlen) &&
			    strlen(opt->optname) == wordlen) {
				optind = i;
				break;
			}
		}
		if (optind == -1) {
			warnx("%s: line %zd: unknown option %.*s",
			    filename, lineno, wordlen, cp);
			return (1);
		}
		cp += wordlen;
		cp += strspn(cp, " \t\n"); /* eat whitespace */
		/* get [selector] if any */
		optsub = NULL;
		if (*cp == '[') {
			cp++;
			wordlen = strcspn(cp, "]");
			if (*cp != ']') {
				if (wordlen == 0) {
					warnx("%s: line %zd: syntax error",
					    filename, lineno);
					return (1);
				}
				asprintf(&optsub, "%.*s", wordlen, cp);
			}
			cp += wordlen;
			cp += strspn(cp, "] \t\n"); /* eat trailing */
		}
		cp += strspn(cp, "= \t\n"); /* eat trailing */
		/* get RHS value */
		optval = strdup(cp);
		/* call function to deal with it all */
		if (configopt[optind].func(optsub, optval,
		    configopt[optind].funcflags) != 0) {
			fprintf(stderr, "%s line %zd: %s\n",
			    filename, lineno, line);
			errx(1, "%s: line %zd: invalid data for %s",
			    filename, lineno, configopt[optind].optname);
		}
		free(optval);
		free(optsub);
		free(line);
	}

	fclose(config);
	DNPRINTF(TWM_D_CONF, "conf_load end\n");

	return (0);
}

void
set_child_transient(struct ws_win *win)
{
	struct ws_win		*parent;

	parent = find_window(win->transient);
	if (parent)
		parent->child_trans = win;
}

struct ws_win *
manage_window(Window id)
{
	Window			trans = 0;
	struct workspace	*ws;
	struct ws_win		*win, *ww;
	int			format, i, ws_idx, n, border_me = 0;
	unsigned long		nitems, bytes;
	Atom			ws_idx_atom = 0, type;
	Atom			*prot = NULL, *pp;
	unsigned char		ws_idx_str[TWM_PROPLEN], *prop = NULL;
	struct twm_region	*r;
	long			mask;
	const char		*errstr;
	XWindowChanges		wc;

	if ((win = find_window(id)) != NULL)
		return (win);	/* already being managed */

	/* see if we are on the unmanaged list */
	if ((win = find_unmanaged_window(id)) != NULL) {
		DNPRINTF(TWM_D_MISC, "manage previously unmanaged window "
		    "%lu\n", win->id);
		TAILQ_REMOVE(&win->ws->unmanagedlist, win, entry);
		TAILQ_INSERT_TAIL(&win->ws->winlist, win, entry);
		if (win->transient)
			set_child_transient(win);
		ewmh_update_actions(win);
		return (win);
	}

	if ((win = calloc(1, sizeof(struct ws_win))) == NULL)
		errx(1, "calloc: failed to allocate memory for new window");

	/* Get all the window data in one shot */
	ws_idx_atom = XInternAtom(display, "_TWM_WS", False);
	if (ws_idx_atom)
		XGetWindowProperty(display, id, ws_idx_atom, 0, TWM_PROPLEN,
		    False, XA_STRING, &type, &format, &nitems, &bytes, &prop);
	XGetWindowAttributes(display, id, &win->wa);
	XGetWMNormalHints(display, id, &win->sh, &mask);
	XGetTransientForHint(display, id, &trans);
	if (trans) {
		win->transient = trans;
		set_child_transient(win);
		DNPRINTF(TWM_D_MISC, "manage_window: win %lu transient %lu\n",
		    win->id, win->transient);
	}

	/* get supported protocols */
	if (XGetWMProtocols(display, id, &prot, &n)) {
		for (i = 0, pp = prot; i < n; i++, pp++) {
			if (*pp == takefocus)
				win->take_focus = 1;
			if (*pp == adelete)
				win->can_delete = 1;
		}
		if (prot)
			XFree(prot);
	}

	/*
	 * Figure out where to put the window. If it was previously assigned to
	 * a workspace (either by spawn() or manually moving), and isn't
	 * transient, * put it in the same workspace
	 */
	r = root_to_region(win->wa.root);
	if (prop && win->transient == 0) {
		DNPRINTF(TWM_D_PROP, "got property _TWM_WS=%s\n", prop);
		ws_idx = strtonum(char_p_of_unsigned_char_p(prop), 0, 9, &errstr);
		if (errstr) {
			DNPRINTF(TWM_D_EVENT, "window idx is %s: %s",
			    errstr, prop);
		}
		ws = &r->s->ws[ws_idx];
	} else {
		ws = r->ws;
		/* this should launch transients in the same ws as parent */
		if (id && trans)
			if ((ww = find_window(trans)) != NULL)
				if (ws->r) {
					ws = ww->ws;
					if (ww->ws->r)
						r = ww->ws->r;
					else
						fprintf(stderr,
						    "fix this bug mcbride\n");
					border_me = 1;
				}
	}

	/* set up the window layout */
	win->id = id;
	win->ws = ws;
	win->s = r->s;	/* this never changes */
	TAILQ_INSERT_TAIL(&ws->winlist, win, entry);

	win->g.w = win->wa.width;
	win->g.h = win->wa.height;
	win->g.x = win->wa.x;
	win->g.y = win->wa.y;
	win->g_floatvalid = 0;
	win->floatmaxed = 0;
	win->ewmh_flags = 0;

	/* Set window properties so we can remember this after reincarnation */
	if (ws_idx_atom && prop == NULL &&
	    snprintf(char_p_of_unsigned_char_p(ws_idx_str),
		    TWM_PROPLEN, "%d", ws->idx) < TWM_PROPLEN) {
		DNPRINTF(TWM_D_PROP, "setting property _TWM_WS to %s\n",
		    ws_idx_str);
		XChangeProperty(display, win->id, ws_idx_atom, XA_STRING, 8,
		    PropModeReplace, ws_idx_str, TWM_PROPLEN);
	}
	if (prop)
		XFree(prop);

	ewmh_autoquirk(win);

	if (XGetClassHint(display, win->id, &win->ch)) {
		DNPRINTF(TWM_D_CLASS, "class: %s name: %s\n",
		    win->ch.res_class, win->ch.res_name);

		/* java is retarded so treat it special */
		if (strstr(win->ch.res_name, "sun-awt")) {
			win->java = 1;
			border_me = 1;
		}

		for (i = 0; i < quirks_length; i++){
			if (!strcmp(win->ch.res_class, quirks[i].class) &&
			    !strcmp(win->ch.res_name, quirks[i].name)) {
				DNPRINTF(TWM_D_CLASS, "found: %s name: %s\n",
				    win->ch.res_class, win->ch.res_name);
				if (quirks[i].quirk & TWM_Q_FLOAT) {
					win->floating = 1;
					border_me = 1;
				}
				win->quirks = quirks[i].quirk;
			}
		}
	}

	/* alter window position if quirky */
	if (win->quirks & TWM_Q_ANYWHERE) {
		win->manual = 1; /* don't center the quirky windows */
		bzero(&wc, sizeof wc);
		mask = 0;
		if (bar_enabled && win->g.y < bar_height) {
			win->g.y = wc.y = bar_height;
			mask |= CWY;
		}
		if (win->g.w + win->g.x > WIDTH(r)) {
			win->g.x = wc.x = WIDTH(r) - win->g.w - 2;
			mask |= CWX;
		}
		border_me = 1;
	}

	/* Reset font sizes (the bruteforce way; no default keybinding). */
	if (win->quirks & TWM_Q_XTERM_FONTADJ) {
		for (i = 0; i < TWM_MAX_FONT_STEPS; i++)
			fake_keypress(win, XK_KP_Subtract, ShiftMask);
		for (i = 0; i < TWM_MAX_FONT_STEPS; i++)
			fake_keypress(win, XK_KP_Add, ShiftMask);
	}

	ewmh_get_win_state(win);
	ewmh_update_actions(win);
	ewmh_update_win_state(win, None, _NET_WM_STATE_REMOVE);

	/* border me */
	if (border_me) {
		bzero(&wc, sizeof wc);
		wc.border_width = border_width;
		mask = CWBorderWidth;
		XConfigureWindow(display, win->id, mask, &wc);
		configreq_win(win);
	}

	XSelectInput(display, id, EnterWindowMask | FocusChangeMask |
	    PropertyChangeMask | StructureNotifyMask);

	set_win_state(win, NormalState);

	/* floaters need to be mapped if they are in the current workspace */
	if ((win->floating || win->transient) && (ws->idx == r->ws->idx))
		XMapRaised(display, win->id);

	return (win);
}

void
free_window(struct ws_win *win)
{
	DNPRINTF(TWM_D_MISC, "free_window:  %lu\n", win->id);

	if (win == NULL)
		return;

	/* needed for restart wm */
	set_win_state(win, WithdrawnState);

	TAILQ_REMOVE(&win->ws->unmanagedlist, win, entry);

	if (win->ch.res_class)
		XFree(win->ch.res_class);
	if (win->ch.res_name)
		XFree(win->ch.res_name);

	kill_refs(win);

	/* paint memory */
	memset(win, 0xff, sizeof *win);	/* XXX kill later */

	free(win);
}

void
unmanage_window(struct ws_win *win)
{
	struct ws_win		*parent;

	if (win == NULL)
		return;

	DNPRINTF(TWM_D_MISC, "unmanage_window:  %lu\n", win->id);

	if (win->transient) {
		parent = find_window(win->transient);
		if (parent)
			parent->child_trans = NULL;
	}

	/* focus on root just in case */
	XSetInputFocus(display, PointerRoot, PointerRoot, CurrentTime);

	if (!win->floating)
		focus_prev(win);

	TAILQ_REMOVE(&win->ws->winlist, win, entry);
	TAILQ_INSERT_TAIL(&win->ws->unmanagedlist, win, entry);

	kill_refs(win);
}

void
focus_magic(struct ws_win *win, int do_trans)
{
	DNPRINTF(TWM_D_FOCUS, "focus_magic: %lu %d\n", WINID(win), do_trans);

	if (win == NULL)
		return;

	if (do_trans == TWM_F_TRANSIENT && win->child_trans) {
		/* win = parent & has a transient so focus on that */
		if (win->java) {
			focus_win(win->child_trans);
			if (win->child_trans->take_focus)
				client_msg(win, takefocus);
		} else {
			/* make sure transient hasn't dissapeared */
			if (validate_win(win->child_trans) == 0) {
				focus_win(win->child_trans);
				if (win->child_trans->take_focus)
					client_msg(win->child_trans, takefocus);
			} else {
				win->child_trans = NULL;
				focus_win(win);
				if (win->take_focus)
					client_msg(win, takefocus);
			}
		}
	} else {
		/* regular focus */
		focus_win(win);
		if (win->take_focus)
			client_msg(win, takefocus);
	}
}

void
expose(XEvent *e)
{
	DNPRINTF(TWM_D_EVENT, "expose: window: %lu\n", e->xexpose.window);
}

void
keypress(XEvent *e)
{
	unsigned int		i;
	KeySym			keysym;
	XKeyEvent		*ev = &e->xkey;

	DNPRINTF(TWM_D_EVENT, "keypress: window: %lu\n", ev->window);

	keysym = XKeycodeToKeysym(display, (KeyCode)ev->keycode, 0);
	for (i = 0; i < keys_length; i++)
		if (keysym == keys[i].keysym
		   && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		   && keyfuncs[keys[i].funcid].func) {
			if (keys[i].funcid == kf_spawn_custom)
				spawn_custom(
				    root_to_region(ev->root),
				    &(keyfuncs[keys[i].funcid].args),
				    keys[i].spawn_name
				    );
			else
				keyfuncs[keys[i].funcid].func(
				    root_to_region(ev->root),
				    &(keyfuncs[keys[i].funcid].args)
				    );
		}
}

void
buttonpress(XEvent *e)
{
	struct ws_win		*win;
	int			i, action;
	XButtonPressedEvent	*ev = &e->xbutton;

	DNPRINTF(TWM_D_EVENT, "buttonpress: window: %lu\n", ev->window);

	action = root_click;
	if ((win = find_window(ev->window)) == NULL)
		return;

	focus_magic(win, TWM_F_TRANSIENT);
	action = client_click;

	for (i = 0; i < LENGTH(buttons); i++)
		if (action == buttons[i].action && buttons[i].func &&
		    buttons[i].button == ev->button &&
		    CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(win, &buttons[i].args);
}

void
configurerequest(XEvent *e)
{
	XConfigureRequestEvent	*ev = &e->xconfigurerequest;
	struct ws_win		*win;
	int			new = 0;
	XWindowChanges		wc;

	if ((win = find_window(ev->window)) == NULL)
		if ((win = find_unmanaged_window(ev->window)) == NULL)
			new = 1;

	if (new) {
		DNPRINTF(TWM_D_EVENT, "configurerequest: new window: %lu\n",
		    ev->window);
		bzero(&wc, sizeof wc);
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(display, ev->window, ev->value_mask, &wc);
	} else {
		DNPRINTF(TWM_D_EVENT, "configurerequest: change window: %lu\n",
		    ev->window);
		if (win->floating) {
			if (ev->value_mask & CWX)
				win->g.x = ev->x;
			if (ev->value_mask & CWY)
				win->g.y = ev->y;
			if (ev->value_mask & CWWidth)
				win->g.w = ev->width;
			if (ev->value_mask & CWHeight)
				win->g.h = ev->height;
		}
		config_win(win);
	}
}

void
configurenotify(XEvent *e)
{
	struct ws_win		*win;
	long			mask;

	DNPRINTF(TWM_D_EVENT, "configurenotify: window: %lu\n",
	    e->xconfigure.window);

	win = find_window(e->xconfigure.window);
	if (win) {
		XGetWMNormalHints(display, win->id, &win->sh, &mask);
		adjust_font(win);
		if (font_adjusted)
			stack();
	}
}

void
destroynotify(XEvent *e)
{
	struct ws_win		*win;
	XDestroyWindowEvent	*ev = &e->xdestroywindow;

	DNPRINTF(TWM_D_EVENT, "destroynotify: window %lu\n", ev->window);

	if ((win = find_window(ev->window)) == NULL) {
		if ((win = find_unmanaged_window(ev->window)) == NULL)
			return;
		free_window(win);
		return;
	}

	/* make sure we focus on something */
	win->floating = 0;

	unmanage_window(win);
	stack();
	free_window(win);
}

void
enternotify(XEvent *e)
{
	XCrossingEvent		*ev = &e->xcrossing;
	XEvent			cne;
	struct ws_win		*win;
#if 0
	struct ws_win		*w;
	Window			focus_return;
	int			revert_to_return;
#endif
	DNPRINTF(TWM_D_FOCUS, "enternotify: window: %lu mode %d detail %d root "
	    "%lu subwindow %lu same_screen %d focus %d state %d\n",
	    ev->window, ev->mode, ev->detail, ev->root, ev->subwindow,
	    ev->same_screen, ev->focus, ev->state);

	switch (focus_mode) {
	case TWM_FOCUS_DEFAULT:
		if (QLength(display)) {
			DNPRINTF(TWM_D_EVENT, "ignore enternotify %d\n",
			    QLength(display));
			return;
		}
		break;
	case TWM_FOCUS_FOLLOW:
		break;
	case TWM_FOCUS_SYNERGY:
#if 0
	/*
	 * all these checks need to be in this order because the
	 * XCheckTypedWindowEvent relies on weeding out the previous events
	 *
	 * making this code an option would enable a follow mouse for focus
	 * feature
	 */

	/*
	 * state is set when we are switching workspaces and focus is set when
	 * the window or a subwindow already has focus (occurs during restart).
	 *
	 * Only honor the focus flag if last_focus_event is not FocusOut,
	 * this allows torba to continue to control focus when another
	 * program is also playing with it.
	 */
	if (ev->state || (ev->focus && last_focus_event != FocusOut)) {
		DNPRINTF(TWM_D_EVENT, "ignoring enternotify: focus\n");
		return;
	}

	/*
	 * happens when a window is created or destroyed and the border
	 * crosses the mouse pointer and when switching ws
	 *
	 * we need the subwindow test to see if we came from root in order
	 * to give focus to floaters
	 */
	if (ev->mode == NotifyNormal && ev->detail == NotifyVirtual &&
	    ev->subwindow == 0) {
		DNPRINTF(TWM_D_EVENT, "ignoring enternotify: NotifyVirtual\n");
		return;
	}

	/* this window already has focus */
	if (ev->mode == NotifyNormal && ev->detail == NotifyInferior) {
		DNPRINTF(TWM_D_EVENT, "ignoring enternotify: win has focus\n");
		return;
	}

	/* this window is being deleted or moved to another ws */
	if (XCheckTypedWindowEvent(display, ev->window, ConfigureNotify,
	    &cne) == True) {
		DNPRINTF(TWM_D_EVENT, "ignoring enternotify: configurenotify\n");
		XPutBackEvent(display, &cne);
		return;
	}

	if ((win = find_window(ev->window)) == NULL) {
		DNPRINTF(TWM_D_EVENT, "ignoring enternotify: win == NULL\n");
		return;
	}

	/*
	 * In fullstack kill all enters unless they come from a different ws
	 * (i.e. another region) or focus has been grabbed externally.
	 */
	if (win->ws->cur_layout->flags & TWM_L_FOCUSPREV &&
	    last_focus_event != FocusOut) {
		XGetInputFocus(display, &focus_return, &revert_to_return);
		if ((w = find_window(focus_return)) == NULL ||
		    w->ws == win->ws) {
			DNPRINTF(TWM_D_EVENT, "ignoring event: fullstack\n");
			return;
		}
	}
#endif
		break;
	}

	if ((win = find_window(ev->window)) == NULL) {
		DNPRINTF(TWM_D_EVENT, "ignoring enternotify: win == NULL\n");
		return;
	}

	/*
	 * if we have more enternotifies let them handle it in due time
	 */
	if (XCheckTypedEvent(display, EnterNotify, &cne) == True) {
		DNPRINTF(TWM_D_EVENT,
		    "ignoring enternotify: got more enternotify\n");
		XPutBackEvent(display, &cne);
		return;
	}

	focus_magic(win, TWM_F_TRANSIENT);
}

/* lets us use one switch statement for arbitrary mode/detail combinations */
#define MERGE_MEMBERS(a,b)	(((a & 0xffff) << 16) | (b & 0xffff))

void
focusevent(XEvent *e)
{
#if 0
	struct ws_win		*win;
	u_int32_t		mode_detail;
	XFocusChangeEvent	*ev = &e->xfocus;

	DNPRINTF(TWM_D_EVENT, "focusevent: %s window: %lu mode %d detail %d\n",
	    ev->type == FocusIn ? "entering" : "leaving",
	    ev->window, ev->mode, ev->detail);

	if (last_focus_event == ev->type) {
		DNPRINTF(TWM_D_FOCUS, "ignoring focusevent: bad ordering\n");
		return;
	}

	last_focus_event = ev->type;
	mode_detail = MERGE_MEMBERS(ev->mode, ev->detail);

	switch (mode_detail) {
	/* synergy client focus operations */
	case MERGE_MEMBERS(NotifyNormal, NotifyNonlinear):
	case MERGE_MEMBERS(NotifyNormal, NotifyNonlinearVirtual):

	/* synergy server focus operations */
	case MERGE_MEMBERS(NotifyWhileGrabbed, NotifyNonlinear):

	/* Entering applications like rdesktop that mangle the pointer */
	case MERGE_MEMBERS(NotifyNormal, NotifyPointer):

		if ((win = find_window(e->xfocus.window)) != NULL && win->ws->r)
			XSetWindowBorder(display, win->id,
			    win->ws->r->s->c[ev->type == FocusIn ?
			    TWM_S_COLOR_FOCUS : TWM_S_COLOR_UNFOCUS].color);
		break;
	default:
		fprintf(stderr, "ignoring focusevent\n");
		DNPRINTF(TWM_D_FOCUS, "ignoring focusevent\n");
		break;
	}
#endif
}

void
mapnotify(XEvent *e)
{
	struct ws_win		*win;
	XMapEvent		*ev = &e->xmap;

	DNPRINTF(TWM_D_EVENT, "mapnotify: window: %lu\n", ev->window);

	win = manage_window(ev->window);
	if (win)
		set_win_state(win, NormalState);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent		*ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	struct ws_win		*win;
	struct twm_region	*r;
	XWindowAttributes	wa;
	XMapRequestEvent	*ev = &e->xmaprequest;

	DNPRINTF(TWM_D_EVENT, "maprequest: window: %lu\n",
	    e->xmaprequest.window);

	if (!XGetWindowAttributes(display, ev->window, &wa))
		return;
	if (wa.override_redirect)
		return;

	win = manage_window(e->xmaprequest.window);
	if (win == NULL)
		return; /* can't happen */

	stack();

	/* make new win focused */
	r = root_to_region(win->wa.root);
	if (win->ws == r->ws)
		focus_magic(win, TWM_F_GENERIC);
}

void
propertynotify(XEvent *e)
{
	struct ws_win		*win;
	XPropertyEvent		*ev = &e->xproperty;

	DNPRINTF(TWM_D_EVENT, "propertynotify: window: %lu\n",
	    ev->window);

	if (ev->state == PropertyDelete)
		return; /* ignore */
	win = find_window(ev->window);
	if (win == NULL)
		return;

	switch (ev->atom) {
	case XA_WM_NORMAL_HINTS:
#if 0
		long		mask;
		XGetWMNormalHints(display, win->id, &win->sh, &mask);
		fprintf(stderr, "normal hints: flag 0x%x\n", win->sh.flags);
		if (win->sh.flags & PMinSize) {
			win->g.w = win->sh.min_width;
			win->g.h = win->sh.min_height;
			fprintf(stderr, "min %d %d\n", win->g.w, win->g.h);
		}
		XMoveResizeWindow(display, win->id,
		    win->g.x, win->g.y, win->g.w, win->g.h);
#endif
		if (window_name_enabled)
			bar_update();
		break;
	default:
		break;
	}
}

void
unmapnotify(XEvent *e)
{
	struct ws_win		*win;

	DNPRINTF(TWM_D_EVENT, "unmapnotify: window: %lu\n", e->xunmap.window);

	/* determine if we need to help unmanage this window */
	win = find_window(e->xunmap.window);
	if (win == NULL)
		return;

	if (getstate(e->xunmap.window) == NormalState) {
		unmanage_window(win);
		stack();
	}
}

void
visibilitynotify(XEvent *e)
{
	int			i;
	struct twm_region	*r;

	DNPRINTF(TWM_D_EVENT, "visibilitynotify: window: %lu\n",
	    e->xvisibility.window);
	if (e->xvisibility.state == VisibilityUnobscured)
		for (i = 0; i < ScreenCount(display); i++)
			TAILQ_FOREACH(r, &screens[i].rl, entry)
				if (e->xvisibility.window == r->bar_window)
					bar_update();
}

void
clientmessage(XEvent *e)
{
	XClientMessageEvent *ev;
	struct ws_win *win;

	ev = &e->xclient;

	win = find_window(ev->window);
	if (win == NULL)
		return;

	DNPRINTF(TWM_D_EVENT, "clientmessage: window: 0x%lx type: %ld \n",
	    ev->window, ev->message_type);

	if (ev->message_type == ewmh[_NET_ACTIVE_WINDOW].atom) {
		DNPRINTF(TWM_D_EVENT, "clientmessage: _NET_ACTIVE_WINDOW \n");
		focus_win(win);
	}
	if (ev->message_type == ewmh[_NET_CLOSE_WINDOW].atom) {
		DNPRINTF(TWM_D_EVENT, "clientmessage: _NET_CLOSE_WINDOW \n");
		if (win->can_delete)
			client_msg(win, adelete);
		else
			XKillClient(display, win->id);
	}
	if (ev->message_type == ewmh[_NET_MOVERESIZE_WINDOW].atom) {
		DNPRINTF(TWM_D_EVENT, "clientmessage: _NET_MOVERESIZE_WINDOW \n");
		if (win->floating) {
			if (ev->data.l[0] & (1<<8)) /* x */
				win->g.x = ev->data.l[1];
			if (ev->data.l[0] & (1<<9)) /* y */
				win->g.y = ev->data.l[2];
			if (ev->data.l[0] & (1<<10)) /* width */
				win->g.w = ev->data.l[3];
			if (ev->data.l[0] & (1<<11)) /* height */
				win->g.h = ev->data.l[4];
		}
		else {
			/* TODO: Change stack sizes */
		}
		config_win(win);
	}
	if (ev->message_type == ewmh[_NET_WM_STATE].atom) {
		DNPRINTF(TWM_D_EVENT, "clientmessage: _NET_WM_STATE \n");
		ewmh_update_win_state(win, ev->data.l[1], ev->data.l[0]);
		if (ev->data.l[2])
			ewmh_update_win_state(win, ev->data.l[2], ev->data.l[0]);

		stack();
	}
}

int
xerror_start(Display *d, XErrorEvent *ee)
{
	other_wm = 1;
	return (-1);
}

int
xerror(Display *d, XErrorEvent *ee)
{
	/* fprintf(stderr, "error: %p %p\n", display, ee); */
	return (-1);
}

int
active_wm(void)
{
	other_wm = 0;
	xerrorxlib = XSetErrorHandler(xerror_start);

	/* this causes an error if some other window manager is running */
	XSelectInput(display, DefaultRootWindow(display),
	    SubstructureRedirectMask);
	XSync(display, False);
	if (other_wm)
		return (1);

	XSetErrorHandler(xerror);
	XSync(display, False);
	return (0);
}

void
new_region(struct twm_screen *s, int x, int y, int w, int h)
{
	struct twm_region	*r, *n;
	struct workspace	*ws = NULL;
	int			i;

	DNPRINTF(TWM_D_MISC, "new region: screen[%d]:%dx%d+%d+%d\n",
	     s->idx, w, h, x, y);

	/* remove any conflicting regions */
	n = TAILQ_FIRST(&s->rl);
	while (n) {
		r = n;
		n = TAILQ_NEXT(r, entry);
		if (X(r) < (x + w) &&
		    (X(r) + WIDTH(r)) > x &&
		    Y(r) < (y + h) &&
		    (Y(r) + HEIGHT(r)) > y) {
		    	if (r->ws->r != NULL)
				r->ws->old_r = r->ws->r;
			r->ws->r = NULL;
			XDestroyWindow(display, r->bar_window);
			TAILQ_REMOVE(&s->rl, r, entry);
			TAILQ_INSERT_TAIL(&s->orl, r, entry);
		}
	}

	/* search old regions for one to reuse */

	/* size + location match */
	TAILQ_FOREACH(r, &s->orl, entry)
		if (X(r) == x && Y(r) == y &&
		    HEIGHT(r) == h && WIDTH(r) == w)
			break;

	/* size match */
	TAILQ_FOREACH(r, &s->orl, entry)
		if (HEIGHT(r) == h && WIDTH(r) == w)
			break;

	if (r != NULL) {
		TAILQ_REMOVE(&s->orl, r, entry);
		/* try to use old region's workspace */
		if (r->ws->r == NULL)
			ws = r->ws;
	} else
		if ((r = calloc(1, sizeof(struct twm_region))) == NULL)
			errx(1, "calloc: failed to allocate memory for screen");

	/* if we don't have a workspace already, find one */
	if (ws == NULL) {
		for (i = 0; i < TWM_WS_MAX; i++)
			if (s->ws[i].r == NULL) {
				ws = &s->ws[i];
				break;
			}
	}

	if (ws == NULL)
		errx(1, "no free workspaces\n");

	X(r) = x;
	Y(r) = y;
	WIDTH(r) = w;
	HEIGHT(r) = h;
	r->s = s;
	r->ws = ws;
	r->ws_prior = NULL;
	ws->r = r;
	outputs++;
	TAILQ_INSERT_TAIL(&s->rl, r, entry);
}

void
scan_xrandr(int i)
{
#ifdef TWM_XRR_HAS_CRTC
	XRRCrtcInfo		*ci;
	XRRScreenResources	*sr;
	int			c;
	int			ncrtc = 0;
#endif /* TWM_XRR_HAS_CRTC */
	struct twm_region	*r;


	if (i >= ScreenCount(display))
		errx(1, "invalid screen");

	/* remove any old regions */
	while ((r = TAILQ_FIRST(&screens[i].rl)) != NULL) {
		r->ws->old_r = r->ws->r = NULL;
		XDestroyWindow(display, r->bar_window);
		TAILQ_REMOVE(&screens[i].rl, r, entry);
		TAILQ_INSERT_TAIL(&screens[i].orl, r, entry);
	}
	outputs = 0;

	/* map virtual screens onto physical screens */
#ifdef TWM_XRR_HAS_CRTC
	if (xrandr_support) {
		sr = XRRGetScreenResources(display, screens[i].root);
		if (sr == NULL)
			new_region(&screens[i], 0, 0,
			    DisplayWidth(display, i),
			    DisplayHeight(display, i));
		else
			ncrtc = sr->ncrtc;

		for (c = 0, ci = NULL; c < ncrtc; c++) {
			ci = XRRGetCrtcInfo(display, sr, sr->crtcs[c]);
			if (ci->noutput == 0)
				continue;

			if (ci != NULL && ci->mode == None)
				new_region(&screens[i], 0, 0,
				    DisplayWidth(display, i),
				    DisplayHeight(display, i));
			else
				new_region(&screens[i],
				    ci->x, ci->y, ci->width, ci->height);
		}
		if (ci)
			XRRFreeCrtcInfo(ci);
		XRRFreeScreenResources(sr);
	} else
#endif /* TWM_XRR_HAS_CRTC */
	{
		new_region(&screens[i], 0, 0, DisplayWidth(display, i),
		    DisplayHeight(display, i));
	}
}

void
screenchange(XEvent *e) {
	XRRScreenChangeNotifyEvent	*xe = (XRRScreenChangeNotifyEvent *)e;
	struct twm_region		*r;
	int				i;

	DNPRINTF(TWM_D_EVENT, "screenchange: %lu\n", xe->root);

	if (!XRRUpdateConfiguration(e))
		return;

	/* silly event doesn't include the screen index */
	for (i = 0; i < ScreenCount(display); i++)
		if (screens[i].root == xe->root)
			break;
	if (i >= ScreenCount(display))
		errx(1, "screenchange: screen not found\n");

	/* brute force for now, just re-enumerate the regions */
	scan_xrandr(i);

	/* add bars to all regions */
	for (i = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			bar_setup(r);
	stack();
}

void
grab_windows(void)
{
	Window			d1, d2, *wins = NULL;
	XWindowAttributes	wa;
	unsigned int		no;
	int			i, j;
	long			state, manage;

	for (i = 0; i < ScreenCount(display); i++) {
		if (!XQueryTree(display, screens[i].root, &d1, &d2, &wins, &no))
			continue;

		/* attach windows to a region */
		/* normal windows */
		for (j = 0; j < no; j++) {
			if (!XGetWindowAttributes(display, wins[j], &wa) ||
			    wa.override_redirect ||
			    XGetTransientForHint(display, wins[j], &d1))
				continue;

			state = getstate(wins[j]);
			manage = state == IconicState;
			if (wa.map_state == IsViewable || manage)
				manage_window(wins[j]);
		}
		/* transient windows */
		for (j = 0; j < no; j++) {
			if (!XGetWindowAttributes(display, wins[j], &wa) ||
			    wa.override_redirect)
				continue;

			state = getstate(wins[j]);
			manage = state == IconicState;
			if (XGetTransientForHint(display, wins[j], &d1) &&
			    manage)
				manage_window(wins[j]);
		}
		if (wins) {
			XFree(wins);
			wins = NULL;
		}
	}
}

void
setup_screens(void)
{
	int			i, j, k;
	int			errorbase, major, minor;
	struct workspace	*ws;
	int			ws_idx_atom;

	if ((screens = calloc(ScreenCount(display),
	     sizeof(struct twm_screen))) == NULL)
		errx(1, "calloc: screens");

	ws_idx_atom = XInternAtom(display, "_TWM_WS", False);

	/* initial Xrandr setup */
	xrandr_support = XRRQueryExtension(display,
	    &xrandr_eventbase, &errorbase);
	if (xrandr_support)
		if (XRRQueryVersion(display, &major, &minor) && major < 1)
			xrandr_support = 0;

	/* map physical screens */
	for (i = 0; i < ScreenCount(display); i++) {
		DNPRINTF(TWM_D_WS, "setup_screens: init screen %d\n", i);
		screens[i].idx = i;
		TAILQ_INIT(&screens[i].rl);
		TAILQ_INIT(&screens[i].orl);
		screens[i].root = RootWindow(display, i);

		/* set default colors */
		setscreencolor("red", i + 1, TWM_S_COLOR_FOCUS);
		setscreencolor("rgb:88/88/88", i + 1, TWM_S_COLOR_UNFOCUS);
		setscreencolor("rgb:00/80/80", i + 1, TWM_S_COLOR_BAR_BORDER);
		setscreencolor("black", i + 1, TWM_S_COLOR_BAR);
		setscreencolor("rgb:a0/a0/a0", i + 1, TWM_S_COLOR_BAR_FONT);

		/* set default cursor */
		XDefineCursor(display, screens[i].root,
		    XCreateFontCursor(display, XC_left_ptr));

		/* init all workspaces */
		/* XXX these should be dynamically allocated too */
		for (j = 0; j < TWM_WS_MAX; j++) {
			ws = &screens[i].ws[j];
			ws->idx = j;
			ws->focus = NULL;
			ws->r = NULL;
			ws->old_r = NULL;
			TAILQ_INIT(&ws->winlist);
			TAILQ_INIT(&ws->unmanagedlist);

			for (k = 0; layouts[k].l_stack != NULL; k++)
				if (layouts[k].l_config != NULL)
					layouts[k].l_config(ws,
					    TWM_ARG_ID_STACKINIT);
			ws->cur_layout = &layouts[0];
		}

		scan_xrandr(i);

		if (xrandr_support)
			XRRSelectInput(display, screens[i].root,
			    RRScreenChangeNotifyMask);
	}
}

void
setup_globals(void)
{
	if ((bar_fonts[0] = strdup("-*-terminus-medium-*-*-*-*-*-*-*-*-*-*-*"))
	    == NULL)
		err(1, "setup_globals: strdup");
	if ((bar_fonts[1] = strdup("-*-times-medium-r-*-*-*-*-*-*-*-*-*-*"))
	    == NULL)
		err(1, "setup_globals: strdup");
	if ((bar_fonts[2] = strdup("-misc-fixed-medium-r-*-*-*-*-*-*-*-*-*-*"))
	    == NULL)
		err(1, "setup_globals: strdup");
	if ((spawn_term[0] = strdup("xterm")) == NULL)
		err(1, "setup_globals: strdup");
	if ((clock_format = strdup("%a %b %d %R %Z %Y")) == NULL)
		errx(1, "strdup");
}

void
workaround(void)
{
	int			i;
	Atom			netwmcheck, netwmname, utf8_string;
	Window			root, win;

	/* work around sun jdk bugs, code from wmname */
	netwmcheck = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
	netwmname = XInternAtom(display, "_NET_WM_NAME", False);
	utf8_string = XInternAtom(display, "UTF8_STRING", False);
	for (i = 0; i < ScreenCount(display); i++) {
		root = screens[i].root;
		win = XCreateSimpleWindow(display,root, 0, 0, 1, 1, 0,
		    screens[i].c[TWM_S_COLOR_UNFOCUS].color,
		    screens[i].c[TWM_S_COLOR_UNFOCUS].color);

		XChangeProperty(display, root, netwmcheck, XA_WINDOW, 32,
		    PropModeReplace, (unsigned char *)&win,1);
		XChangeProperty(display, win, netwmcheck, XA_WINDOW, 32,
		    PropModeReplace, (unsigned char *)&win,1);
		XChangeProperty(display, win, netwmname, utf8_string, 8,
		    PropModeReplace, (unsigned char*)"LG3D", strlen("LG3D"));
	}
}

int
main(int argc, char *argv[])
{
	struct passwd		*pwd;
	struct twm_region	*r, *rr;
	struct ws_win		*winfocus = NULL;
	struct timeval		tv;
	union arg		a;
	char			conf[PATH_MAX], *cfile = NULL;
	struct stat		sb;
	XEvent			e;
	int			xfd, i;
	fd_set			rd;
	struct sigaction	sact;

	start_argv = argv;
	fprintf(stderr, "Welcome to torba V%s cvs tag: %s\n",
	    TWM_VERSION, cvstag);
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		warnx("no locale support");

	if (!(display = XOpenDisplay(0)))
		errx(1, "can not open display");

	if (active_wm())
		errx(1, "other wm running");

	/* handle some signals */
	bzero(&sact, sizeof(sact));
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	sact.sa_handler = sighdlr;
	sigaction(SIGINT, &sact, NULL);
	sigaction(SIGQUIT, &sact, NULL);
	sigaction(SIGTERM, &sact, NULL);
	sigaction(SIGHUP, &sact, NULL);

	sact.sa_handler = sighdlr;
	sact.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sact, NULL);

	astate = XInternAtom(display, "WM_STATE", False);
	aprot = XInternAtom(display, "WM_PROTOCOLS", False);
	adelete = XInternAtom(display, "WM_DELETE_WINDOW", False);
	takefocus = XInternAtom(display, "WM_TAKE_FOCUS", False);

	/* look for local and global conf file */
	pwd = getpwuid(getuid());
	if (pwd == NULL)
		errx(1, "invalid user %d", getuid());

	setup_screens();
	setup_globals();
	setup_keys();
	setup_quirks();
	setup_spawn();

	/* load config */
	snprintf(conf, sizeof conf, "%s/.%s", pwd->pw_dir, TWM_CONF_FILE);
	if (stat(conf, &sb) != -1) {
		if (S_ISREG(sb.st_mode))
			cfile = conf;
	} else {
		/* try global conf file */
		snprintf(conf, sizeof conf, "/etc/%s", TWM_CONF_FILE);
		if (!stat(conf, &sb))
			if (S_ISREG(sb.st_mode))
				cfile = conf;
	}
	if (cfile)
		conf_load(cfile);

	setup_ewmh();
	/* set some values to work around bad programs */
	workaround();

	/* grab existing windows (before we build the bars) */
	grab_windows();

	/* setup all bars */
	for (i = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			if (winfocus == NULL)
				winfocus = TAILQ_FIRST(&r->ws->winlist);
			bar_setup(r);
		}

	unfocus_all();

	grabkeys();
	stack();

	xfd = ConnectionNumber(display);
	while (running) {
		while (XPending(display)) {
			XNextEvent(display, &e);
			if (running == 0)
				goto done;
			if (e.type < LASTEvent) {
				dumpevent(&e);
				if (handler[e.type])
					handler[e.type](&e);
				else
					DNPRINTF(TWM_D_EVENT,
					    "win: %lu unknown event: %d\n",
					    e.xany.window, e.type);
			} else {
				switch (e.type - xrandr_eventbase) {
				case RRScreenChangeNotify:
					screenchange(&e);
					break;
				default:
					DNPRINTF(TWM_D_EVENT,
					    "win: %lu unknown xrandr event: "
					    "%d\n", e.xany.window, e.type);
					break;
				}
			}
		}

		/* if we are being restarted go focus on first window */
		if (winfocus) {
			rr = winfocus->ws->r;
			if (rr == NULL) {
				/* not a visible window */
				winfocus = NULL;
				continue;
			}
			/* move pointer to first screen if multi screen */
			if (ScreenCount(display) > 1 || outputs > 1)
				XWarpPointer(display, None, rr->s[0].root,
				    0, 0, 0, 0, rr->g.x,
				    rr->g.y + (bar_enabled ? bar_height : 0));

			a.id = TWM_ARG_ID_FOCUSCUR;
			focus(rr, &a);
			winfocus = NULL;
			continue;
		}

		FD_ZERO(&rd);
		FD_SET(xfd, &rd);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		if (select(xfd + 1, &rd, NULL, NULL, &tv) == -1)
			if (errno != EINTR)
				DNPRINTF(TWM_D_MISC, "select failed");
		if (restart_wm == 1)
			restart(NULL, NULL);
		if (running == 0)
			goto done;
		if (bar_alarm) {
			bar_alarm = 0;
			bar_update();
		}
	}
done:
	teardown_ewmh();
	bar_extra_stop();
	XFreeGC(display, bar_gc);
	XCloseDisplay(display);

	return (0);
}

/* vim:set tw=8 sw=8 noexpandtab tw=80: */
