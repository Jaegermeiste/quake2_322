/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef __QMENU_H__
#define __QMENU_H__

#define MAXMENUITEMS	64

#define MTYPE_SLIDER		0
#define MTYPE_LIST			1
#define MTYPE_ACTION		2
#define MTYPE_SPINCONTROL	3
#define MTYPE_SEPARATOR  	4
#define MTYPE_FIELD			5

#define	K_TAB			9
#define	K_ENTER			13
#define	K_ESCAPE		27
#define	K_SPACE			32

// normal keys should be passed as lowercased ascii

#define	K_BACKSPACE		127
#define	K_UPARROW		128
#define	K_DOWNARROW		129
#define	K_LEFTARROW		130
#define	K_RIGHTARROW	131

#define QMF_LEFT_JUSTIFY	0x00000001
#define QMF_GRAYED			0x00000002
#define QMF_NUMBERSONLY		0x00000004

typedef struct menuframework_s
{
	int x, y;
	int	cursor;

	int	nitems;
	int nslots;
	void *items[64];

	const char *statusbar;

	void (*cursordraw)( struct menuframework_s *m );
	
} menuframework_t;

typedef struct menucommon_s
{
	int type;
	const char *name;
	int x, y;
	menuframework_t *parent;
	int cursor_offset;
	int	localdata[4];
	unsigned flags;

	const char *statusbar;

	void (*callback)( void *self );
	void (*statusbarfunc)( void *self );
	void (*ownerdraw)( void *self );
	void (*cursordraw)( void *self );
} menucommon_t;

typedef struct menufield_s
{
	menucommon_t generic;

	char		buffer[80];
	int			cursor;
	int			length;
	int			visible_length;
	int			visible_offset;
} menufield_t;

typedef struct menuslider_s
{
	menucommon_t generic;

	float minvalue;
	float maxvalue;
	float curvalue;

	float range;
} menuslider_t;

typedef struct menulist_s
{
	menucommon_t generic;

	int curvalue;

	const char **itemnames;
} menulist_t;

typedef struct menuaction_s
{
	menucommon_t generic;
} menuaction_t;

typedef struct menuseparator_s
{
	menucommon_t generic;
} menuseparator_t;

int Field_Key( menufield_t *field, int key );

void	Menu_AddItem( menuframework_t *menu, void *item );
void	Menu_AdjustCursor( menuframework_t *menu, int dir );
void	Menu_Center( menuframework_t *menu );
void	Menu_Draw( menuframework_t *menu );
void	*Menu_ItemAtCursor( menuframework_t *m );
int Menu_SelectItem( menuframework_t *s );
void	Menu_SetStatusBar( menuframework_t *s, const char *string );
void	Menu_SlideItem( menuframework_t *s, int dir );
int		Menu_TallySlots( menuframework_t *menu );

void	 Menu_DrawString( int, int, const char * );
void	 Menu_DrawStringDark( int, int, const char * );
void	 Menu_DrawStringR2L( int, int, const char * );
void	 Menu_DrawStringR2LDark( int, int, const char * );

#endif
