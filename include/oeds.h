/*
 * oeds.h -- Orcs & Elves DS (USA, YOEE): the types and functions restored from
 * the arm9 binary (0x02000000). Offsets are the DS build's; where a structure
 * matches the BREW game's (oe-re), the BREW name is kept.
 *
 * The original was C++ (Applet.cpp, Game.cpp, Render.cpp, ScriptThread.cpp,
 * ...); here every method is Class_method(Class *self, ...), as in oe-re.
 * Every field's offset is checked (FIELD_AT) against the one the code uses.
 *
 * Functions not restored yet keep Ghidra's name with their address where
 * their use does not tell what they do (Canvas_FUN_0201a718).
 */
#ifndef OEDS_H
#define OEDS_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>		/* abs */
#include <string.h>		/* memset */

typedef int8_t		s8;
typedef uint8_t		u8;
typedef int16_t		s16;
typedef uint16_t	u16;
typedef int32_t		s32;
typedef uint32_t	u32;
typedef int64_t		s64;
typedef uint64_t	u64;

#define FIELD_AT( T, f, off )	_Static_assert( offsetof( T, f ) == ( off ), #T "." #f " at " #off )
#define SIZE_IS( T, n )			_Static_assert( sizeof( T ) == ( n ), "sizeof " #T " is " #n )

typedef struct App			App;
typedef struct Game			Game;
typedef struct Render		Render;
typedef struct Canvas		Canvas;
typedef struct Player		Player;
typedef struct ScriptThread	ScriptThread;
typedef struct Entity		Entity;
typedef struct Combat		Combat;
typedef struct Hud			Hud;
typedef struct Text			Text;
typedef struct MenuSystem	MenuSystem;
typedef struct Sound		Sound;
typedef struct InputStream	InputStream;
typedef struct OutputStream	OutputStream;
typedef struct Particles	Particles;
typedef struct EntityDefs	EntityDefs;

/* ---------------------------------------------------------------- Nitro SDK / C library (sdk/) */

u64		OS_GetTick( void );								/* 0x02005864 */
void	OS_DebugBreak( void );							/* 0x02005f3c (nothing in the retail build?) */
s64		_ll_sdiv( s64 a, s64 b );						/* 0x02010c68 */
u64		_ll_udiv( u64 a, u64 b );						/* 0x02010e18 */
int		_u32_div_rem( u32 a, u32 b, u32 *rem );			/* 0x02010e60 (a % b comes back in r1) */
int		vsnprintf( char *buf, int size, const char *fmt, void *ap );	/* 0x0200366c */
int		FixedSqrt( u64 x );								/* 0x02012d00 (an int square root) */
s32		OS_GetLockID( void );							/* 0x020034e0 */
void	OS_ReleaseLockID( u16 id );						/* 0x02003538 */
void	*OS_AllocFromHeap( int arena, int heap, u32 size );	/* 0x02005094 */
void	OS_FreeToHeap( int arena, int heap, void *p );	/* 0x0200519c */
int		OS_GetTotalAllocSize( int arena, int heap );	/* 0x02005468 */
int		OS_GetTotalFreeSize( int arena, int heap );		/* 0x020054cc */
void	DC_FlushRange( const void *p, u32 n );			/* 0x02004ba8 */
void	MI_CpuCopy8( const void *src, void *dst, u32 n );	/* 0x020063bc */
void	MI_CpuFill32( u32 v, void *dst, u32 n );		/* 0x02006288 */

typedef struct { u8 data[16]; } MIUncompContextLZ;
void	MI_InitUncompContextLZ( MIUncompContextLZ *ctx, void *dst, const u32 *header );	/* 0x02006568 */
int		MI_ReadUncompLZ8( MIUncompContextLZ *ctx, const void *src, u32 n );	/* 0x02006598 bytes still to come */

/* FSFile (0x48): only what the game reads of it */
typedef struct {
	u8		pad0[0x24];
	u32		top;			/* +0x24 the file's first byte in the ROM */
	u32		bottom;			/* +0x28 */
	u32		pos;			/* +0x2c */
	u8		pad30[0x48 - 0x30];
} FSFile;
#define FS_SEEK_SET		0
#define FS_SEEK_CUR		1
void	FS_InitFile( FSFile *f );						/* 0x0200874c */
int		FS_OpenFile( FSFile *f, const char *path );		/* 0x020089e4 */
int		FS_CloseFile( FSFile *f );						/* 0x02008a2c */
int		FS_ReadFile( FSFile *f, void *dst, int n );		/* 0x02008b28 -1 on failure */
int		FS_SeekFile( FSFile *f, int offset, int whence );	/* 0x02008b38 */

#define CARD_REQ_READ_BACKUP			6
#define CARD_REQ_WRITE_BACKUP			7
#define CARD_REQUEST_MODE_RECEIVE		0
#define CARD_REQUEST_MODE_SEND_VERIFY	1
void	CARD_LockBackup( u16 lockId );					/* 0x0200ad5c */
void	CARD_UnlockBackup( u16 lockId );				/* 0x0200ad6c */
int		CARD_GetResultCode( void );						/* 0x0200ad10 */
int		CARDi_RequestStreamCommand( u32 src, u32 dst, u32 len, void *callback, void *arg, int async,
			int req, int retries, int mode );		/* 0x0200b1dc */

/* ms since the start: OS_GetTick() counts 33.514 MHz / 64 */
#define OS_TICKS_TO_MS( t )	( (int)_ll_udiv( (u64)( t ) << 6, 0x82ea ) )
#define NOW_MS()			OS_TICKS_TO_MS( OS_GetTick() )

/* ---------------------------------------------------------------- String (a 16 byte text object) */

typedef struct String {
	u8		data[0x10];
} String;

void	String_clear( String *self, int keep );			/* 0x02054aec */
int		String_length( const String *self );			/* 0x020545d0 */
int		String_charAt( const String *self, int i );		/* 0x020545d8 */
void	String_setCharAt( String *self, int i, int c );	/* 0x02054718 */

/* ---------------------------------------------------------------- App (App.cpp, Applet.cpp) */

extern App *app;									/* 0x02074f8c */

struct App {
	int			time;								/* +0x00 ms of this frame */
	int			gameTime;							/* +0x04 */
	u8			pad8[8];
	u64			randState;							/* +0x10 a 64-bit LCG: state = state * randMul + randAdd */
	u64			randMul;							/* +0x18 */
	u64			randAdd;							/* +0x20 */
	Canvas		*canvas;							/* +0x28 */
	Render		*render;							/* +0x2c */
	u8			pad30[4];
	Player		*player;							/* +0x34 */
	Game		*game;								/* +0x38 */
	EntityDefs	*entityDefs;						/* +0x3c */
	Combat		*combat;							/* +0x40 */
	Hud			*hud;								/* +0x44 */
	Text		*text;								/* +0x48 */
	MenuSystem	*menuSystem;						/* +0x4c */
	void		*weaponView;						/* +0x50 */
	Particles	*particles;							/* +0x54 */
	Sound		*sound;								/* +0x58 */
	u8			pad5c[0x2d1984 - 0x5c];
	int			errorCode;							/* +0x2d1984 */
	char		errorStr[0x100];					/* +0x2d1988 */
	int			hasBackup;							/* +0x2d1a88 the cartridge has backup memory (saves) */
	int			backupOk;							/* +0x2d1a8c 0: the backup could not be read */
	u8			pad2d1a90[0x2d1aac - 0x2d1a90];
	int			statGoldA;							/* +0x2d1aac (both lose what is stolen) */
	int			statGoldB;							/* +0x2d1ab0 */
};
FIELD_AT( App, randState, 0x10 );	FIELD_AT( App, canvas, 0x28 );	FIELD_AT( App, player, 0x34 );
FIELD_AT( App, sound, 0x58 );		FIELD_AT( App, errorCode, 0x2d1984 );	FIELD_AT( App, statGoldB, 0x2d1ab0 );

/* the next number of the applet's generator (inlined where it is used) */
static inline u64 App_random64( App *self ) {
	self->randState = self->randState * self->randMul + self->randAdd;
	return self->randState;
}
/* a random number in [0, n): the high word's low byte, modulo n (as the scripts use it) */
static inline int App_randomBelow( App *self, int n ) {
	u32 rem;
	_u32_div_rem( (u32)( App_random64( self ) >> 32 ) & 0xff, (u32)n, &rem );
	return (int)rem;
}

void	App_setErrorCode( App *self, int code );	/* 0x02013760 */
void	App_setErrorCodeRaw( App *self, int code );	/* 0x02013780 */
void	App_setErrorStr( App *self, const char *fmt, ... );	/* 0x02013790 */
int		App_getErrorCode( App *self );				/* 0x020137d8 */
void	App_fatal( App *self, const char *fmt, ... );	/* 0x0201362c "MAX_SCRIPT_STACK", "invalid line %d", ... */
void	App_printf( App *self, const char *fmt, ... );	/* 0x020136fc */
void	App_showSaveError( App *self, int string );	/* 0x0201349c a full screen of text string (group 0): 0xdc read, 0xdd write */
int		App_updateMemUsed( App *self );				/* 0x02013b80 */
extern int	appHeap;								/* 0x02074f84 the main heap */
extern int	appMemUsed;								/* 0x02074f88 */
void	*App_malloc( u32 size );					/* 0x02013bcc (operator new, new[]: 0x02013bfc, 0x02013c08) */
void	App_free( void *p );						/* 0x02013be4 */
void	App_delete( void *p );						/* 0x02013c14 */
void	App_deleteArray( void *p );					/* 0x02013c28 */

/* The DS's error macro: the code, and in a debug build its name, file, line and function (the
 * retail binary still passes them: "EFAILED", "Util.cpp", 0x4c, "DirToAngle") */
#define EFAILED		0
#define EDEBUG		(-666)
#define APP_ERROR( code, name, file, line, func )	App_setErrorCode( app, ( code ) )

/* A debug printf (a 512 byte buffer, written nowhere in the retail build); the binary keeps the
 * calls and their texts ("script#%d.%d paused", "%d Camera::Update done %d", ...) */
void	Dbg_printf( const char *fmt, ... );			/* 0x020139d0 (Ghidra: "Game_linkEntity") */

/* ---------------------------------------------------------------- Util (Util.cpp) */

/* the 8 map directions of the .bsp and the scripts. Angles: 1024 a turn, 0 east, 0x100 north
 * (y grows to the south) */
enum {
	DIR_WEST, DIR_EAST, DIR_NORTH, DIR_SOUTH,
	DIR_NORTHEAST, DIR_NORTHWEST, DIR_SOUTHEAST, DIR_SOUTHWEST
};
int		Util_DirToAngle( int dir );					/* 0x02012e18 */
int		Util_AngleToDir( int angle );				/* 0x02012f0c */
int		Util_RelativeQuadrant( int unused, int ax, int ay, int angle, int bx, int by );	/* 0x0203cf88 */

#define TILE_CENTER( t )	( ( t ) * 64 + 32 )

/* ---------------------------------------------------------------- Render (Render.cpp) */

#define MAP_TILES		32
#define MAX_LINES		1024
#define MAX_SPRITES		600
#define TILE_HAS_EVENT	0x20						/* Render.mapFlags (DS; BREW 0x08) */

typedef struct {
	s16		x1, y1, x2, y2;		/* +0x00 */
	s16		texture;			/* +0x08 */
	s16		pad;
	u32		flags;				/* +0x0c 0x40 open, 0x400 locked, 0x80000 moving, ... */
} Line;
SIZE_IS( Line, 0x10 );

/* Sprite.info */
#define SPRITE_FRAME		0xf00
#define SPRITE_HIDDEN		0x1000
#define SPRITE_WALLTEX		0x4000
#define SPRITE_ONWALL		0x78000		/* facing one of the 4 walls */
#define SPRITE_SEEN			0x8000000

typedef struct Sprite {
	s16		x, y, z;			/* +0x00 world units (z 32 on the floor) */
	s16		pad6;
	u32		info;				/* +0x08 tile | frame << 8 | SPRITE_* */
	Entity	*ent;				/* +0x0c */
	u32		param;				/* +0x10 */
	u8		renderMode;			/* +0x14 */
	u8		scale;				/* +0x15 64 = 1.0 */
	u8		pad16[0x20 - 0x16];
	struct RenderNode *node;	/* +0x20 the BSP leaf whose list it is in */
	struct Sprite *nextInNode;	/* +0x24 */
} Sprite;
SIZE_IS( Sprite, 0x28 );

typedef struct {
	u32		op;
	u32		arg;
	u32		flags;		/* bits 16..24 the event states it runs in, 0x100 run, 0x200 once, 0x2000000 ... */
} Command;

/* a tile event: bits 0..8 tile, 9..19 first command, 20..26 commands, 27..30 state, 31 ... */
#define EVENT_FIRST( e )	( ( (e) & 0xffe00 ) >> 9 )
#define EVENT_COUNT( e )	( ( (e) & 0x7f00000 ) >> 20 )
#define EVENT_STATE( e )	( ( (e) & 0x78000000 ) >> 27 )

typedef struct {
	int		duration;			/* +0x00 ms, 0 idle */
	int		startTime;			/* +0x04 */
	int		to;					/* +0x08 the flat it goes to */
	int		from;				/* +0x0c */
} FlatLerp;

/* a BSP node: split on x or y at the low 16 bits of split; children: hi16 below, lo16 above */
#define NODE_SPLIT_X	0x10000
#define NODE_SPLIT_Y	0x20000
typedef struct RenderNode {
	u8		pad0[8];
	u32		split;				/* +0x08 */
	u32		children;			/* +0x0c */
	struct Sprite *sprites;		/* +0x10 the sprites in the leaf */
	int		drawnFrame;			/* +0x14 */
} RenderNode;
SIZE_IS( RenderNode, 0x18 );
#define MAX_NODES	512

struct Render {
	u8			pad0[0x2c];
	RenderNode	nodes[MAX_NODES];					/* +0x2c */
	u8			pad302c[0x3030 - 0x302c];
	Line		lines[MAX_LINES];					/* +0x3030 */
	int			numLines;							/* +0x7030 */
	Sprite		sprites[MAX_SPRITES];				/* +0x7034 */
	u8			padcdf4[0xcdf8 - 0xcdf4];
	int			numSprites;							/* +0xcdf8 */
	s16			tileEventTiles[(0xcff0 - 0xcdfc) / 2];	/* +0xcdfc the tiles of the events, ascending */
	u32			tileEvents[(0xd3d8 - 0xcff0) / 4];	/* +0xcff0 */
	int			numTileEvents;						/* +0xd3d8 */
	Command		commands[(0x12840 - 0xd3dc) / 12];	/* +0xd3dc */
	u8			pad1283c[0x12840 - 0x1283c];
	s16			mapFlags[MAP_TILES * MAP_TILES];	/* +0x12840 */
	String		mapName;							/* +0x13040 */
	u8			pad13050[0x1306c - 0x13050];
	u8			floorFlat;							/* +0x1306c */
	u8			ceilFlat;							/* +0x1306d */
	u8			pad1306e[2];
	FlatLerp	flatLerps[2];						/* +0x13070 floor, ceiling */
	int			oscillate;							/* +0x13090 the water bob of entities */
	u8			pad13094[0x131f0 - 0x13094];
	u16			ceilColor;							/* +0x131f0 RGB555 */
	u16			floorColor;							/* +0x131f2 */
	int			globalFloor;						/* +0x131f4 */
	int			globalCeil;							/* +0x131f8 */
	u8			pad131fc[0x1360c - 0x131fc];
	int			curFloorFlat;						/* +0x1360c */
	int			curCeilFlat;						/* +0x13610 */
	u8			pad13614[0x13650 - 0x13614];
	s32			sinTable[1024];						/* +0x13650 16.16, 1024 a turn */
	u8			pad14650[0x146a0 - 0x14650];
	int			viewX, viewY, viewZ, viewAngle;		/* +0x146a0 the view of the last frame */
	u8			pad146b0[0x14f10 - 0x146b0];
	int			frame;								/* +0x14f10 frames drawn */
	u8			pad14f14[0x22ab8 - 0x14f14];
	int			fade;								/* +0x22ab8 32: none */
	u8			pad22abc[0x22bc8 - 0x22abc];
	int			polyId;								/* +0x22bc8 the next polygon ID (1..62) */
	u8			pad22bcc[0x237488 - 0x22bcc];
	u32			progAlpha;							/* +0x237488 */
	u8			pad23748c[4];
	u8			diffuse[3], ambient[3], specular[3], emission[3];	/* +0x237490 the material colours */
};
#define REG_POLYGON_ATTR	( *(volatile u32 *)0x040004a4 )
int		Render_progExpr( Render *self, const u8 **pc, int bias, int time );	/* 0x0204be28 a program's value, << 12 */
FIELD_AT( Render, lines, 0x3030 );		FIELD_AT( Render, numLines, 0x7030 );	FIELD_AT( Render, sprites, 0x7034 );
FIELD_AT( Render, numSprites, 0xcdf8 );	FIELD_AT( Render, tileEventTiles, 0xcdfc );	FIELD_AT( Render, tileEvents, 0xcff0 );
FIELD_AT( Render, numTileEvents, 0xd3d8 );	FIELD_AT( Render, commands, 0xd3dc );	FIELD_AT( Render, mapFlags, 0x12840 );
FIELD_AT( Render, floorFlat, 0x1306c );	FIELD_AT( Render, flatLerps, 0x13070 );	FIELD_AT( Render, oscillate, 0x13090 );
FIELD_AT( Render, ceilColor, 0x131f0 );	FIELD_AT( Render, globalCeil, 0x131f8 );	FIELD_AT( Render, curFloorFlat, 0x1360c );
FIELD_AT( Render, sinTable, 0x13650 );	FIELD_AT( Render, viewX, 0x146a0 );		FIELD_AT( Render, viewAngle, 0x146ac );

int		Render_findTileEvent( Render *self, int tile );		/* 0x02042080 */
RenderNode *Render_findLeaf( Render *self, int x, int y );	/* 0x020457fc */
RenderNode *Render_findNode( Render *self, int x, int y );	/* 0x020450fc (a copy of findLeaf) */
int		Render_inSameLeaf( Render *self, int x1, int y1, int x2, int y2 );	/* 0x020457c8 */
void	Render_relinkSpriteAt( Render *self, struct Sprite *s, int x, int y );	/* 0x020451a8 */
int		Render_boxVisible( Render *self, int x, int y, int z );	/* 0x02044680 the 3D hardware's box test, 20.12 */
int		Render_tileTouchesWall( Render *self, int tx, int ty, int unused );	/* EV_LERPBLOCK */

/* ---------------------------------------------------------------- Entity (Entity.cpp, EntityDefManager.cpp) */

enum {
	ETYPE_PLAYER = 1, ETYPE_MONSTER = 2, ETYPE_ITEM = 6, ETYPE_DECOR = 7, ETYPE_CORPSE = 9, ETYPE_SWITCH = 14
};

typedef struct {
	s16		tile;			/* +0x00 */
	u8		pad2[8 - 2];
	s16		name;			/* +0x08 its name in the entity strings */
	s16		nameString;		/* +0x0a the string shown (Entity.name = it | 0x10000) */
	u8		padc[0xe - 0xc];
	u8		eType;			/* +0x0e */
	u8		eSubType;		/* +0x0f */
	s16		parm;			/* +0x10 */
} EntityDef;

typedef struct {
	u8		pad0[0xa];
	u8		weapon;			/* +0x0a */
	u8		padb;
	int		x, y;			/* +0x0c a monster's: where its sprite is going */
	u8		pad14[4];
} CombatEntity;
SIZE_IS( CombatEntity, 0x18 );

typedef struct {
	u8		type;			/* +0x00 the monster's goal */
	u8		flags;			/* +0x01 */
	u8		pad2;
	u8		x, y;			/* +0x03 the goal's tile */
	u8		pad5;
	s16		turns;			/* +0x06 */
} MonsterGoal;

typedef struct {
	CombatEntity	ce;			/* +0x00 */
	Entity			*nextOnList;	/* +0x18 the active or inactive ring (Game) */
	Entity			*prevOnList;	/* +0x1c */
	Entity			*nextAttacker;	/* +0x20 Game.combatMonsters */
	Entity			*target;	/* +0x24 */
	MonsterGoal		goal;		/* +0x28 */
	int				destX, destY;	/* +0x30 */
	u8				pad38[0x40 - 0x38];
	u16				flags;		/* +0x40 0x80 corpse shown, 0x200 ..., 0x400 scaled, 0x2000 ... */
	u8				frozen;		/* +0x42 */
	u8				pad43;
	u8				burning;	/* +0x44 */
} Monster;
FIELD_AT( Monster, goal, 0x28 );	FIELD_AT( Monster, destX, 0x30 );	FIELD_AT( Monster, flags, 0x40 );

/* Entity.info */
#define ENT_SPRITE_MASK		0xffff		/* sprite + 1 */
#define ENT_REMOVED			0x20000
#define ENT_ALIVE			0x40000
#define ENT_IN_RING			0x80000
#define ENT_LINE			0x200000
#define ENT_LERPING			0x400000
#define ENT_LINKED			0x4000000
#define ENT_MOVED			0x8000000
#define ENT_AGGRO			0x20000000
#define ENT_CORPSE			0x40000000
#define ENT_DIRTY			0x80000000u

struct Entity {
	EntityDef	*def;		/* +0x00 */
	Monster		*monster;	/* +0x04 */
	Entity		*next;		/* +0x08 on its tile */
	Entity		*prev;		/* +0x0c */
	s16			tile;		/* +0x10 */
	u8			emitter;	/* +0x12 its ambient sound (Sound emitters) */
	u8			pad13;
	u32			name;		/* +0x14 string group << 16 | string */
	u32			info;		/* +0x18 */
	int			param;		/* +0x1c */
};
SIZE_IS( Entity, 0x20 );

void	*EntityDefs_find( EntityDefs *self, int eType, int subType, int parm );
void	Entity_resurrect( Entity *self, int x, int y, int z );
void	Entity_kill( Entity *self, int byScript );				/* 0x02025b0c (Ghidra Entity_FUN_02025b0c) */
void	Entity_takeDamage( Entity *self, int damage );
void	Entity_knockback( Entity *player, int x, int y, int distance );
void	Entity_openReliquary( Entity *self, int tx, int ty, int what, String *msg );	/* 0x0202715c */
void	Entity_aiThink( Entity *self );
int		Entity_findOpenAdjacentTile( int x, int y, int eType, int *ox, int *oy, Entity *ignore );	/* 0x020296c4 */
int		Entity_isNPC( Entity *self );							/* 0x0202d6a8 (Ghidra FUN_0202d6a8) */
void	MonsterGoal_reset( MonsterGoal *self );

void	CombatEntity_setMaxHealth( CombatEntity *self, int v );
void	CombatEntity_setHealth( CombatEntity *self, int v );
int		CombatEntity_getHealth( CombatEntity *self );
int		CombatEntity_getMaxHealth( CombatEntity *self );
void	CombatEntity_setStrength( CombatEntity *self, int v );
void	CombatEntity_setDefense( CombatEntity *self, int v );	/* 0x02023330 */
void	CombatEntity_setAccuracy( CombatEntity *self, int v );	/* 0x02023368 */
void	CombatEntity_addArmor( CombatEntity *self, int v );

/* ---------------------------------------------------------------- Combat (Combat.cpp) */

struct Combat {
	u8			pad0[0xf14];
	Render		*render;							/* +0xf14 */
};
FIELD_AT( Combat, render, 0xf14 );

int		Combat_angle8( int dx, int dy );						/* 0x02022234 */
void	Combat_launchFlames( Combat *self, int x1, int y1, int x2, int y2 );	/* 0x020222a0 */
void	*Combat_launchSpellSprite( Combat *self, int *from, int *to );
void	Combat_start( Combat *self, Entity *attacker, Entity *target, int unused );
void	Combat_checkSurrounded( Combat *self );

/* ---------------------------------------------------------------- Player (Player.cpp) */

struct Player {
	u8			pad0[0x3e];
	u8			crystalCharges;						/* +0x3e (EV_CRYSTALHEAL: a give of 3/1 that does not add one heals) */
	u8			pad3f[0x78 - 0x3f];
	u32			weapons;							/* +0x78 */
	u8			pad7c[0x84 - 0x7c];
	u16			spells;								/* +0x84 the wand spells learned (Player_give type 11): 0 force, 1 fire, 2 lightning, 3 stun */
	u8			pad86[0x88 - 0x86];
	int			level;								/* +0x88 */
	u8			pad8c[0x90 - 0x8c];
	int			nextLevelXP;						/* +0x90 */
	int			gold;								/* +0x94 */
	u8			pad98[0xb0 - 0x98];
	CombatEntity	ce;								/* +0xb0 (ce.weapon is the weapon in hand) */
	u8			padc8[4];
	int			noTarget;							/* +0xcc monsters do not wake (Game_activateEx) */
	u8			padd0[0xf0 - 0xd0];
	u32			secretsMaps;						/* +0xf0 the maps all of whose secrets were found */
	u8			padf4[0x100 - 0xf4];
	s16			listIds[8];							/* +0x100 a list the scripts mark (EV 0x73) */
	int			listCount;							/* +0x110 */
	u8			listBitsB;							/* +0x114 state 2 */
	u8			listBitsA;							/* +0x115 state 1 */
	u8			pad116[0x134 - 0x116];
	int			wandChatStr;						/* +0x134 */
	u8			pad138[0x14c - 0x138];
	u32			collectibles;						/* +0x14c 15 bits (EV 0x75) */
	u8			numCollectibles;					/* +0x150 */
	u8			pad151[0x15c - 0x151];
	int			attacked;							/* +0x15c */
	u8			pad160[0x170 - 0x160];
	u8			prevWeapon;							/* +0x170 */
	u8			pad171[0x178 - 0x171];
	u32			firePits;							/* +0x178 */
	u8			pad17c[0x1a7 - 0x17c];
	u8			unk1a7;								/* +0x1a7 */
	u8			pad1a8[0x1cc - 0x1a8];
	int			hammerCuesTime;						/* +0x1cc */
	u8			dragonMood;							/* +0x1d0 */
	u8			pad1d1[3];
	int			gameEnded;							/* +0x1d4 */
	s8			dragonState;						/* +0x1d8 1: its timer starts with the next turn (2) */
	u8			pad1d9[3];
	int			dragonStateTime;					/* +0x1dc */
	u8			pad1e0[4];
	u32			flags;								/* +0x1e4 EV_SET / EV_TRIGGERIF */
	u8			pad1e8[4];
	int			crystalSound;						/* +0x1ec */
};
FIELD_AT( Player, weapons, 0x78 );	FIELD_AT( Player, ce, 0xb0 );	FIELD_AT( Player, secretsMaps, 0xf0 );
FIELD_AT( Player, listIds, 0x100 );	FIELD_AT( Player, wandChatStr, 0x134 );	FIELD_AT( Player, collectibles, 0x14c );
FIELD_AT( Player, attacked, 0x15c );	FIELD_AT( Player, prevWeapon, 0x170 );	FIELD_AT( Player, firePits, 0x178 );
FIELD_AT( Player, hammerCuesTime, 0x1cc );	FIELD_AT( Player, gameEnded, 0x1d4 );	FIELD_AT( Player, flags, 0x1e4 );
FIELD_AT( Player, crystalSound, 0x1ec );

int		Player_countCollectibles( Player *self );					/* 0x0203cbac */
void	Player_setListState( Player *self, int id, int state );		/* 0x0203f698 */
void	Player_selectWeapon( Player *self, int weapon );
void	Player_selectNextWeapon( Player *self );
void	Player_setWandMood( Player *self, int mood );
void	Player_hurtFeedback( Player *self, int unused );
void	Player_takeDamage( Player *self, int damage, int unused );
void	Player_modifyStat( Player *self, int stat, int delta );
int		Player_hasStat( Player *self, int stat );				/* 0x0203c46c (EV_REQSTAT) */
void	Player_give( Player *self, int type, int what, int count, int unused );
void	Player_setListState( Player *self, int id, int state );
int		Player_hasItemCount( Player *self, int type, int what, int min, int max );	/* 0x0203def8 (EV_REQITEM) */
void	Player_addHealth( Player *self, int v );
void	Player_addXP( Player *self, int xp );
int		Player_xpForLevel( Player *self, int level );
void	Player_showHelp( Player *self, int what, int force );
void	Player_addNote( Player *self, int id, int unused );		/* 0x0203f5f0 (EV_NOTE) */
void	Player_disableWeapons( Player *self );					/* 0x0203c230 */
void	Player_enableWeapons( Player *self );					/* 0x0203c248 */
void	Player_countSecrets( Player *self, int *found, int *total );	/* 0x0203c124 */
void	Player_adjustDragonMood( Player *self, int delta );		/* 0x02041544 */
void	Player_FUN_0203f77c( Player *self );						/* after EV_GOTO */

/* ---------------------------------------------------------------- Hud, Text, MenuSystem */

struct Text {
	u8			pad0[0x7c];
	int			numArgs;							/* +0x7c the %s / %d of the next string */
};
FIELD_AT( Text, numArgs, 0x7c );

struct Hud {
	u8			pad0[0xa4];
	int			numSubtitles;						/* +0xa4 */
	u8			pada8[0xf4 - 0xa8];
	int			wandChat;							/* +0xf4 */
	u8			padf8[0x3e0 - 0xf8];
	String		subtitle2;							/* +0x3e0 a cinematic's caption, 2 lines */
	String		subtitle1;							/* +0x3f0 */
	String		message;							/* +0x400 */
	u8			pad410[0x430 - 0x410];
	String		forced;								/* +0x430 EV_FORCEMESSAGE */
};
FIELD_AT( Hud, wandChat, 0xf4 );	FIELD_AT( Hud, subtitle2, 0x3e0 );	FIELD_AT( Hud, forced, 0x430 );

struct MenuSystem {
	u8			pad0[0x180e4];
	s16			menuId;								/* +0x180e4 */
	u8			pad180e6[0x1812c - 0x180e6];
	s8			saveSlot;							/* +0x1812c the slot (0..2) the game saves to / loads from, -1 none */
	s8			pendingSlot;						/* +0x1812d */
	u8			pad1812e[0x184d4 - 0x1812e];
	u16			unk184d4;							/* +0x184d4 (EV_ENDGAME: 0xffff) */
	u8			pad184d6[0x18580 - 0x184d6];
	u16			lastCollectible;					/* +0x18580 (EV 0x75) */
	u8			pad18582[0x18598 - 0x18582];
	int			saveCorrupted;						/* +0x18598 a message for the player */
};
FIELD_AT( MenuSystem, menuId, 0x180e4 );	FIELD_AT( MenuSystem, lastCollectible, 0x18580 );

void	Text_getString( Text *self, int group, int id, String *out );		/* Ghidra "Hud_addMessage" */
void	Text_wrap( Text *self, String *in, int width, String *out );		/* 0x020655d4 */
void	Text_wrapLines( Text *self, String *in, int width, String *out, int a, int b, int c );	/* 0x02065634 */
void	Text_setArgString( Text *self, String *s );			/* 0x02065148 */
void	Text_setArgInt( Text *self, int v );				/* 0x020650fc */
void	Hud_addMessage( Hud *self, String *s, int kind );	/* 0x020391f0 */
void	Hud_showString( Hud *self, int group, int id, int kind );	/* 0x02039190 */
void	Hud_wandChat( Hud *self, int group, int id );		/* 0x020393cc */
String	*Hud_messageBuffer( Hud *self, int kind );			/* 0x02039578 */
void	Hud_finishMessage( Hud *self );						/* 0x020395d4 */
void	Hud_randomBark( Hud *self, int kind, int unused );
void	Hud_gameMessage( Hud *self, int stringId, int unused );	/* Ghidra "App_snprintf": a string of the game as a message */
void	MenuSystem_setMenu( MenuSystem *self, int kind, int menu );	/* 0x020593f4 */
void	Automap_visit( MenuSystem *self, int tx, int ty );	/* 0x0205b480 (the map is a menu) */
void	WeaponView_reset( void *self );						/* 0x02069e98 */

/* ---------------------------------------------------------------- Sound (the ambient emitters too) */

#define MAX_SOUND_EMITTERS	0x60
#define MAX_SOUND_HEADERS	0x200

typedef struct {
	u8		enabled;		/* +0x00 (Sound+0x3244 + n * 0x1c; EV 0x74) */
	u8		pad1;
	s16		x, y;			/* +0x02 0, 0: everywhere */
	u8		volume;			/* +0x06 */
	u8		pan;			/* +0x07 */
	s16		id;				/* +0x08 the sound */
	u8		padA[2];
	s32		channel;		/* +0x0c the sound playing for it (channel | id << 8), -1 none */
	int		unk10;			/* +0x10 */
	s16		unk14;			/* +0x14 */
	u8		pad16[2];
	int		mode;			/* +0x18 0..2 (placed or not, the header's bits 10..) */
} SoundEmitter;
SIZE_IS( SoundEmitter, 0x1c );

typedef struct {
	u32		flags;			/* +0x00 0..4 volume, 5..9 pan, 10.. (a loop?) */
	u8		pad4[0xc];
	int		rate;			/* +0x10 */
	int		end;			/* +0x14 its end in data.bin (the one before it: its start) */
} SoundHeader;
SIZE_IS( SoundHeader, 0x18 );

struct Sound {
	int				firstMapEmitter;				/* +0x00 the map's first emitter (EV 0x74 adds its number) */
	int				started;						/* +0x04 */
	App				*app;							/* +0x08 (its random numbers pick a sound's variant) */
	s16				listenerX, listenerY, listenerAngle;	/* +0x0c */
	u8				pad12[2];
	FSFile			data;							/* +0x14 rom:/sounds/data.bin */
	u8				channels[8][0x3c];				/* +0x5c */
	int				numHeaders;						/* +0x23c */
	SoundHeader		headers[MAX_SOUND_HEADERS];		/* +0x240 rom:/sounds/headers.bin */
	int				numEmitters;					/* +0x3240 */
	SoundEmitter	emitters[MAX_SOUND_EMITTERS];	/* +0x3244 the .bsp's "emitters" */
	u8				buffers[24][0x4000];			/* +0x3cc4 the sounds' data, loaded 16 KB a piece */
	struct { u8 *data; u16 id; u8 users; u8 pad; } slots[24];	/* +0x63cc4 */
};
FIELD_AT( Sound, data, 0x14 );	FIELD_AT( Sound, headers, 0x240 );	FIELD_AT( Sound, emitters, 0x3244 );
FIELD_AT( Sound, slots, 0x63cc4 );

/* Ghidra calls Sound_play "Random_nextByte" */
int		Sound_play( Sound *self, Entity *source, int id, int flags, int priority );	/* 0x02052f88 */
int		Sound_isPlaying( Sound *self, int handle );			/* 0x020532ec */
void	Sound_setEmitter( Sound *self, int n, int on );		/* 0x020533c4 (EV 0x74) */
void	Sound_stopEmitter( Sound *self, SoundEmitter *e );	/* 0x0205333c */
int		Sound_playAt( Sound *self, int x, int y, int id, int flags, int priority, int a, int b, int c );	/* 0x02053010 */
void	Sound_setListener( Sound *self, int x, int y, int angle );	/* 0x02052bbc */
int		Sound_addEmitter( Sound *self, int id, int x, int y, int volume, int pan );	/* 0x02052e90 */
void	Entity_calcPosition( Entity *ent, int *x, int *y );	/* 0x02028168 */

/* ---------------------------------------------------------------- Particles */

typedef struct {
	u8		pad0[0x18];
	u32		color;			/* +0x18 */
	u8		pad1c;
	u8		flags;			/* +0x1d */
} ParticleSet;

extern const u32 particleColors[16];				/* 0x020718c4 */

ParticleSet	*Particles_spawn( Particles *self, int type, int x, int y, int z, int count );	/* 0x02069f5c */
void	Particles_start( Particles *self, ParticleSet *set, int time );		/* 0x02069fa8 */

/* ---------------------------------------------------------------- Canvas (Canvas.cpp) */

enum {
	CS_PLAYING = 3, CS_COMBAT = 4, CS_CAMERA = 0x14, CS_LOADING = 0x15
};

typedef struct {
	u8		type;
	u8		pad[3];
	u32		text;			/* group << 16 | string */
	ScriptThread *owner;	/* resumed when it is closed */
} HelpEntry;
SIZE_IS( HelpEntry, 0xc );

struct Canvas {
	int			scriptWaitUntil;					/* +0x00 */
	u8			pad4[0x18 - 4];
	int			viewX, viewY, viewZ, viewAngle;		/* +0x18 */
	int			destX, destY;						/* +0x28 */
	u8			pad30[0x3c - 0x30];
	int			destAngle;							/* +0x3c */
	u8			pad40[0x90 - 0x40];
	u8			turning;							/* +0x90 */
	u8			pad91[0xb4 - 0x91];
	int			changingMap;						/* +0xb4 */
	u8			padb8[0xcc - 0xb8];
	int			lineWidth;							/* +0xcc */
	int			state;								/* +0xd0 */
	u8			padd4[0xd8 - 0xd4];
	u16			mapId;								/* +0xd8 6000 + map */
	u8			padda[0xde - 0xda];
	s16			stringGroup;						/* +0xde the map's strings */
	u8			pade0[0xf4 - 0xe0];
	int			abortMove;							/* +0xf4 */
	u8			padf8[0x924 - 0xf8];
	int			storeOpen;							/* +0x924 */
	u8			pad928[0x9ac - 0x928];
	int			unk9ac;								/* +0x9ac (EV_ENDCINEMATIC clears it) */
	u8			pad9b0[0x9d4 - 0x9b0];
	u8			unk9d4;								/* +0x9d4 (EV_ENDCINEMATIC clears it) */
	u8			pad9d5[0xa10 - 0x9d5];
	String		text;								/* +0xa10 */
	u8			pada20[0xa3c - 0xa20];
	int			endGame;							/* +0xa3c */
	u8			pada40[0xa80 - 0xa40];
	HelpEntry	helpQueue[8];						/* +0xa80 */
	int			helpCount;							/* +0xae0 */
	String		messages[2];						/* +0xae4 */
	int			messageIndex;						/* +0xb04 */
	u8			padb08[0xb34 - 0xb08];
	MenuSystem	*menuSystem;						/* +0xb34 */
};
FIELD_AT( Canvas, viewX, 0x18 );	FIELD_AT( Canvas, destX, 0x28 );	FIELD_AT( Canvas, destAngle, 0x3c );
FIELD_AT( Canvas, turning, 0x90 );	FIELD_AT( Canvas, changingMap, 0xb4 );	FIELD_AT( Canvas, state, 0xd0 );
FIELD_AT( Canvas, mapId, 0xd8 );	FIELD_AT( Canvas, stringGroup, 0xde );	FIELD_AT( Canvas, abortMove, 0xf4 );
FIELD_AT( Canvas, storeOpen, 0x924 );	FIELD_AT( Canvas, text, 0xa10 );	FIELD_AT( Canvas, endGame, 0xa3c );
FIELD_AT( Canvas, helpQueue, 0xa80 );	FIELD_AT( Canvas, helpCount, 0xae0 );	FIELD_AT( Canvas, messages, 0xae4 );
FIELD_AT( Canvas, messageIndex, 0xb04 );	FIELD_AT( Canvas, menuSystem, 0xb34 );

void	Canvas_setState( Canvas *self, int state );						/* 0x02015028 */
void	Canvas_startFade( Canvas *self, int ms, int mode );				/* 0x0201b998 */
void	Canvas_startShake( Canvas *self, int ms, int intensity, int vibrate );
void	Canvas_finishRotation( Canvas *self );
void	Canvas_updateFacing( Canvas *self );
void	Canvas_FUN_0201a718( Canvas *self );								/* after EV_GOTO */
void	Canvas_showDialog( Canvas *self, ScriptThread *owner, int group, int id, int style, int flags, int extra );	/* 0x0201ac90 */
void	Canvas_addScriptHelp( Canvas *self, int group, int id, ScriptThread *owner, int type );	/* 0x0201b7e0 */
void	Canvas_addHelpString( Canvas *self, String *s, int type );		/* 0x0201b884 */
void	Canvas_dequeueHelp( Canvas *self, int force );					/* 0x0201b4c0 */
void	Canvas_showChatDialog( Canvas *self, ScriptThread *owner, String *text );

/* ---------------------------------------------------------------- Game (Game.cpp) */

#define MAX_CAMERAS		0x60
#define MAX_CAMERA_KEYS	0x180
#define MAX_LERP_SPRITES	16

typedef struct CameraKey {
	s16		duration;		/* +0x00 ms */
	s16		x, y, z;		/* +0x02 */
	s16		pitch, yaw, roll;	/* +0x08 */
	u8		pad0e[2];
	struct CameraKey *next;	/* +0x10 */
	struct CameraKey *prev;	/* +0x14 */
	u8		fromPlayer;		/* +0x18 1/2/4/8: x/y/z/angle from the player when started */
	u8		pad19[3];
} CameraKey;
SIZE_IS( CameraKey, 0x1c );

typedef struct {
	u8			pad0[4];
	CameraKey	*keys;		/* +0x04 */
	int			numKeys;	/* +0x08 */
	int			keyDone;	/* +0x0c */
	int			curKey;		/* +0x10 */
	ScriptThread *thread;	/* +0x14 resumed at the keys */
	int			x, y, z;	/* +0x18 << 12 */
	int			pitch, yaw, roll;	/* +0x24 */
} Camera;
SIZE_IS( Camera, 0x30 );

void	Camera_chainKeys( Camera *self, int loop );			/* 0x02013c3c */
void	Camera_snapToKey( Camera *self, int key );			/* 0x02013cf0 */
void	Camera_Update( Camera *self, int key, int time );		/* 0x02013d7c (Camera.cpp) */
int		Camera_spline( Camera *self, int t, int p0, int p1, int p2, int p3 );		/* 0x02013f74 20.12 */
int		Camera_splineAngle( Camera *self, int t, int a0, int a1, int a2, int a3 );	/* 0x02014180 */

/* LerpSprite.flags */
#define LERP_NO_WAIT	1
#define LERP_RELINKED	2		/* the entity is on its new tile already */
#define LERP_77			4		/* (DS op 0x77, the plain move) */

typedef struct {
	ScriptThread *thread;	/* +0x00 the one waiting on it */
	int		duration;		/* +0x04 */
	int		startTime;		/* +0x08 */
	s16		sprite;			/* +0x0c + 1, 0 free */
	s16		srcX, srcY, srcZ;	/* +0x0e */
	s16		dstX, dstY, dstZ;	/* +0x14 */
	s16		pad1a;
	u32		param;			/* +0x1c the sprite's */
	s16		dist;			/* +0x20 */
	u8		srcScale;		/* +0x22 */
	u8		dstScale;		/* +0x23 */
	u8		flags;			/* +0x24 */
	u8		pad25[3];
} LerpSprite;
SIZE_IS( LerpSprite, 0x28 );

/* ---------------------------------------------------------------- ScriptThread (ScriptThread.cpp) */

#define MAX_SCRIPT_STACK	8

/* ScriptThread_executeEvent results */
enum {
	SCRIPT_NOT_RUN,			/* 0: the command did not apply */
	SCRIPT_DONE,			/* 1 */
	SCRIPT_PAUSED,			/* 2: the thread waits (a dialog, a camera, a lerp, a timer) */
	SCRIPT_ABORT,			/* 3 */
	SCRIPT_RESTART			/* 4: from the event's first command again */
};

/* ScriptFrame.frameFlags */
#define FRAME_NESTED		0x01
#define FRAME_PAUSED		0x02	/* resumed when the frame above it finishes */
#define FRAME_NO_RESUME		0x04
#define FRAME_COMBAT		0x08	/* EV_AIGOAL started a fight */

typedef struct {
	u32		event;			/* +0 the tile event (Render.tileEvents[]) */
	u32		flags;			/* +4 the ways the tile was entered (direction bits, 0x400 used, ...) */
	u8		cmdIndex;		/* +8 the command run last (0xff: none yet) */
	u8		frameFlags;		/* +9 */
	u8		pad[2];
} ScriptFrame;

struct ScriptThread {
	int			waitUntil;							/* +0x00 EV_WAIT */
	s8			state;								/* +0x04 SCRIPT_* of the last run */
	u8			pad5[3];
	int			inUse;								/* +0x08 */
	s8			numFrames;							/* +0x0c */
	u8			padd[3];
	ScriptFrame	frames[MAX_SCRIPT_STACK];			/* +0x10 */
	s8			id;									/* +0x70 (only in the debug texts) */
	u8			pad71[3];
	App			*app;								/* +0x74 */
};
FIELD_AT( ScriptThread, frames, 0x10 );	FIELD_AT( ScriptThread, id, 0x70 );	FIELD_AT( ScriptThread, app, 0x74 );

/* a game sprite (Game_gspriteAlloc): a door sliding, an animation (EV_ANIM), ... */
#define MAX_GAME_SPRITES	64
#define GS_INUSE			0x01
#define GS_MOVING			0x20
typedef struct {
	u8		state;			/* +0x00 doors: 1 opening, 2 closing */
	u8		pad1[3];
	int		startTime;		/* +0x04 */
	int		time8;			/* +0x08 */
	int		duration;		/* +0x0c */
	int		time10;			/* +0x10 */
	u8		flags;			/* +0x14 GS_* */
	u8		pad15[3];
	int		fromX, fromY, fromZ;	/* +0x18 */
	int		toX, toY, toZ;	/* +0x24 */
	int		vx, vy, vz;		/* +0x30 units a second */
	u8		pad3c[8];
	struct Sprite *sprite;	/* +0x44 */
	Line	*line;			/* +0x48 a door's */
} GameSprite;
SIZE_IS( GameSprite, 0x4c );

/* SaveData (Game+0xb290): the backup memory in reference-counted blocks. 0: magic, 4: the
 * checksum of the header (8: 64 block counts); 0x48 + slot * 0x80: the slots' tables (a checksum,
 * then 16 entries: the block offsets), slot 3 being the game being played (also kept in RAM).
 * Blocks: 4 config (0x40 bytes, from 0x248), 8 player (0x200, from 0xb48), 52 world (0x1000,
 * from 0xd348); each starts with the checksum of the rest. */
#define SAVE_MAGIC		0x081d1410u
#define SAVE_SLOTS		0x48
#define SAVE_TABLE		16
#define SAVE_BLOCKS		64
enum { SAVE_CONFIG, SAVE_PLAYER, SAVE_PLAYER2, SAVE_WORLD, SAVE_MAPS };	/* table entries (maps: 4 + map) */
typedef struct SaveData {
	App		*app;					/* +0x00 */
	u32		table[SAVE_TABLE];		/* +0x04 slot 3's */
	u8		used[SAVE_BLOCKS];		/* +0x44 the blocks' users */
} SaveData;
FIELD_AT( SaveData, used, 0x44 );

#define MAX_SCRIPT_THREADS	16
#define MAX_MOVING_BLOCKS	0x28

typedef struct {
	u8		pad0[0x64];
	int		dstX, dstY;		/* +0x64 */
	u8		pad6c[0x74 - 0x6c];
	int		active;			/* +0x74 */
	u8		pad78[4];
	int		blocking;		/* +0x7c EV 0x61 */
} MovingBlock;
SIZE_IS( MovingBlock, 0x80 );

struct Game {
	u8			pad0[0x10];
	int			wallX, wallY;						/* +0x10 where Game_trace met a wall */
	int			traceCount;							/* +0x18 */
	Entity		*hits[32];							/* +0x1c Game_trace's, nearest first on each tile */
	int			numTraceHits;						/* +0x9c Game_trace */
	Entity		*traceEntity;						/* +0xa0 the first hit */
	Camera		cameras[MAX_CAMERAS];				/* +0xa4 */
	int			numCameras;							/* +0x12a4 */
	CameraKey	cameraKeys[MAX_CAMERA_KEYS];		/* +0x12a8 */
	int			numCameraKeys;						/* +0x3ca8 */
	Entity		entities[400];						/* +0x3cac 0: the world, 1: the player, 2..17 monster clips */
	u8			pad6eac[4];
	Entity		*tileLists[MAP_TILES * MAP_TILES];	/* +0x6eb0 */
	u8			pad7eb0[0x9ad4 - 0x7eb0];
	u32			savedPosition;						/* +0x9ad4 tile x | y << 5 | direction << 10 (Game_loadState) */
	int			forceSnap;							/* +0x9ad8 */
	Entity		*inactiveMonsters;					/* +0x9adc a ring (Monster.nextOnList) */
	Entity		*activeMonsters;					/* +0x9ae0 */
	Entity		*combatMonsters;					/* +0x9ae4 */
	int			skipUntil;							/* +0x9ae8 */
	u8			pad9aec[0x9af4 - 0x9aec];
	int			firstLineEntity, lastLineEntity;	/* +0x9af4 the doors' entities */
	u8			pad9afc[0x9b04 - 0x9afc];
	int			scriptsDisabled;					/* +0x9b04 */
	int			skipping;							/* +0x9b08 a cinematic is being skipped */
	int			suppressDialog;						/* +0x9b0c */
	int			yieldFlag;							/* +0x9b10 */
	String		password;							/* +0x9b14 */
	int			monstersTurn;						/* +0x9b24 */
	u8			pad9b28[0x9b30 - 0x9b28];
	int			snapMonsters;						/* +0x9b30 */
	int			unk9b34, unk9b38;					/* +0x9b34 (Game_loadState: 0, 1) */
	u8			pad9b3c[0x9f58 - 0x9b3c];
	GameSprite	gsprites[MAX_GAME_SPRITES];			/* +0x9f58 doors sliding, animations */
	u8			padb258[0xb260 - 0xb258];
	u32			pendingMapChange;					/* +0xb260 */
	int			respawnX, respawnY, respawnDir;		/* +0xb264 */
	u16			respawnMap;							/* +0xb270 */
	u8			padb272[0xb274 - 0xb272];
	int			loadType;							/* +0xb274 Game_loadState's which */
	Entity		*alertMonster;						/* +0xb278 the one to make its alert sound */
	App			*app;								/* +0xb27c */
	Canvas		*canvas;							/* +0xb280 */
	Render		*render;							/* +0xb284 */
	Player		*player;							/* +0xb288 */
	u8			padb28c[0xb290 - 0xb28c];
	SaveData	*saveData;							/* +0xb290 */
	u8			padb294[0xb2d8 - 0xb294];
	Camera		*activeCamera;						/* +0xb2d8 */
	int			cameraStart;						/* +0xb2dc */
	s8			cameraKey;							/* +0xb2e0 */
	u8			padb2e1[3];
	int			skippable;							/* +0xb2e4 */
	MovingBlock	movingBlocks[MAX_MOVING_BLOCKS];	/* +0xb2e8 EV_LERPBLOCK */
	int			numMovingBlocks;					/* +0xc6e8 still moving and blocking */
	int			movingBlocksBusy;					/* +0xc6ec */
	LerpSprite	lerpSprites[MAX_LERP_SPRITES];		/* +0xc6f0 */
	s8			numLerpSprites;						/* +0xc970 */
	u8			padc971[3];
	ScriptThread threads[MAX_SCRIPT_THREADS];		/* +0xc974 */
	s8			numThreads;							/* +0xd0f4 */
	u8			padd0f5[0xd280 - 0xd0f5];
	s16			goldStolen;							/* +0xd280 */
	u8			padd282[0xd288 - 0xd282];
	u8			difficulty;							/* +0xd288 */
};
FIELD_AT( Game, cameras, 0xa4 );		FIELD_AT( Game, numCameras, 0x12a4 );	FIELD_AT( Game, cameraKeys, 0x12a8 );
FIELD_AT( Game, numCameraKeys, 0x3ca8 );	FIELD_AT( Game, entities, 0x3cac );	FIELD_AT( Game, tileLists, 0x6eb0 );
FIELD_AT( Game, combatMonsters, 0x9ae4 );	FIELD_AT( Game, skipUntil, 0x9ae8 );	FIELD_AT( Game, scriptsDisabled, 0x9b04 );
FIELD_AT( Game, yieldFlag, 0x9b10 );		FIELD_AT( Game, password, 0x9b14 );	FIELD_AT( Game, snapMonsters, 0x9b30 );
FIELD_AT( Game, pendingMapChange, 0xb260 );	FIELD_AT( Game, respawnMap, 0xb270 );	FIELD_AT( Game, activeCamera, 0xb2d8 );
FIELD_AT( Game, cameraKey, 0xb2e0 );		FIELD_AT( Game, skippable, 0xb2e4 );	FIELD_AT( Game, lerpSprites, 0xc6f0 );
FIELD_AT( Game, activeMonsters, 0x9ae0 );	FIELD_AT( Game, firstLineEntity, 0x9af4 );	FIELD_AT( Game, monstersTurn, 0x9b24 );
FIELD_AT( Game, app, 0xb27c );		FIELD_AT( Game, player, 0xb288 );	FIELD_AT( Game, movingBlocks, 0xb2e8 );
FIELD_AT( Game, numMovingBlocks, 0xc6e8 );	FIELD_AT( Game, numLerpSprites, 0xc970 );	FIELD_AT( Game, threads, 0xc974 );
FIELD_AT( Game, numThreads, 0xd0f4 );	FIELD_AT( Game, hits, 0x1c );	FIELD_AT( Game, numTraceHits, 0x9c );
FIELD_AT( Game, savedPosition, 0x9ad4 );	FIELD_AT( Game, gsprites, 0x9f58 );	FIELD_AT( Game, loadType, 0xb274 );
FIELD_AT( Game, saveData, 0xb290 );
FIELD_AT( Game, goldStolen, 0xd280 );		FIELD_AT( Game, difficulty, 0xd288 );

void	Game_trace( Game *self, int x0, int y0, int x1, int y1, Entity *ignore, int mask );
Entity	*Game_findEntityAt( Game *self, int x, int y, int typeMask );
Entity	*Game_getTileEntities( Game *self, int x, int y );
void	Game_linkEntity( Game *self, Entity *ent, int tx, int ty );		/* 0x0202e368 */
void	Game_unlinkEntity( Game *self, Entity *ent );					/* Ghidra "Sound_play" (game, ent) */
void	Game_activateEx( Game *self, Entity *ent, int alert, int distCheck );
void	Game_deactivate( Game *self, Entity *ent );						/* 0x0202fb5c */
void	Game_removeFromCombat( Entity *ent );
int		Game_snapAllMonsterSprites( Game *self, int force );			/* 0x0203347c */
int		Game_canSnapMonsters( Game *self );								/* 0x020333f8 */
void	Game_snapMonsterSprite( Entity *ent );
void	Game_monsterAI( Game *self );									/* 0x0202fcbc */
void	Game_updateMonsterLerps( Game *self );							/* 0x0202ffbc */
void	Game_beginPlayerTurn( Game *self );								/* 0x020335b4 (also before EV_GOTO) */
void	Game_endPlayerTurn( Game *self );								/* 0x02033638 */
void	Game_checkInactive( Game *self, Entity *ent );					/* 0x02033bb0 */
void	Game_checkActive( Game *self, Entity *ent );					/* 0x02033c00 */
int		Game_finishMoves( Game *self, int force );						/* 0x02033444 */
int		Game_movingBlocksIdle( Game *self );							/* 0x02036c10 */
int		Game_finishMovingBlocks( Game *self, int force );				/* 0x02036c28 */
void	Game_placeMovingBlock( Game *self, MovingBlock *b, int x, int y );	/* 0x02036890 */
void	Game_touchEntity( Game *self, Entity *ent );
void	Game_freeLerpSprite( Game *self, LerpSprite *ls );				/* 0x02036d88 */
void	Game_freeScriptThread( Game *self, ScriptThread *t );			/* 0x02037070 */
void	Game_runScriptThreads( Game *self );							/* 0x02036f28 */
int		Entity_distSqFrom( Entity *ent, int x, int y );
int		Player_isVisible( Player *self );
int		EntityMonster_getSound( Monster *m, int which );
void	Sound_moveEmitter( Sound *self, int n, int x, int y );			/* 0x020533f8 */
void	Automap_tileChanged( MenuSystem *self, int tx, int ty );		/* 0x0205b480 */
int		Game_executeTile( Game *self, int tx, int ty, u32 flags, int nested );
void	Game_setLineLocked( Game *self, int line, int locked, int unused );
int		Game_useDoor( Game *self, int op, int line, int animate );
void	Game_setSwitch( Game *self, Entity *ent, int on );
void	Game_revealAutomapRect( Game *self, int x, int y, int w, int h );	/* 0x02030280 */
void	*Game_gspriteAllocAnim( Game *self, int tile, int x, int y, int z );
LerpSprite *Game_allocLerpSprite( Game *self, ScriptThread *thread, int sprite );
void	Game_setFirePit( Game *self, int pit, int on );					/* 0x02034b28 */
ScriptThread *Game_allocScriptThread( Game *self );						/* 0x02036fe4 */
void	Game_updateBossReinforcements( Game *self, int a, int b );
void	Game_addMovingBlock( Game *self, int x0, int y0, int x1, int y1, int ms, int blocking );
int		Game_touchTile( Game *self, int x, int y, int force );
GameSprite *Game_gspriteAlloc( Game *self, int type, int tile, int flags );	/* 0x02034bc4 */
void	Game_saveState( Game *self, int lastMap, int mapId, int x, int y, int angle, int x2, int y2, u32 what );	/* 0x02032824 */
int		Game_loadState( Game *self, int which );						/* 0x02032f68 */
void	Game_saveConfig( Game *self, int unused );						/* 0x02032274 */
void	Game_loadConfig( Game *self );									/* 0x02032594 */
int		Game_savePlayerState( Game *self, OutputStream *os, int mapId, int x, int y, int angle, int x2, int y2, u32 what );	/* 0x02032cf4 */
int		Game_loadPlayerState( Game *self, InputStream *is );			/* 0x02032e50 */
void	Game_saveWorld( Game *self, OutputStream *os, int full );		/* 0x02030f78 */
int		Dir_fromAngle( int angle );
void	Canvas_loadMap( Canvas *self, int mapId, int a );				/* 0x02015e7c */
void	App_nop( App *self );											/* 0x020139cc */

int		SaveData_init( SaveData *self );								/* 0x020668a8 */
u32		SaveData_getConfigOffset( SaveData *self, int slot );			/* 0x02066ab0 */
u32		SaveData_getPlayerOffset( SaveData *self, int slot, int which );	/* 0x02066b84 */
u32		SaveData_getWorldOffset( SaveData *self, int slot, int map );	/* 0x02066c6c */
int		SaveData_isValidSlot( SaveData *self, int slot );				/* 0x02066d5c */
void	SaveData_copySlot( SaveData *self, int dst, int src );			/* 0x02066d78 (Ghidra: "Game_saveWorldState") */
u32		SaveData_allocSmallBlock( SaveData *self, int take );			/* 0x02066f54 */
u32		SaveData_allocPlayerBlock( SaveData *self, int take );			/* 0x02066f9c */
u32		SaveData_allocWorldBlock( SaveData *self, int take );			/* 0x02066fe4 */
void	SaveData_writeDirectory( SaveData *self );						/* 0x02067030 */
void	SaveData_retainSlot( SaveData *self, int slot );				/* 0x02067104 */
void	SaveData_releaseSlot( SaveData *self, int slot );				/* 0x020672b0 */
void	SaveData_allocEntry( SaveData *self, int entry );				/* 0x020674f0 */
void	SaveData_rebuildUsed( SaveData *self );							/* 0x020675d0 */
void	SaveData_clearSlot( SaveData *self, int slot );					/* 0x02067670 */
void	SaveData_writeChecksum( SaveData *self, u32 offset, int len, u32 at );	/* 0x02067724 */
int		SaveData_verifyChecksum( SaveData *self, u32 offset, int len, u32 sum, int mode );	/* 0x0206787c */
int		SaveData_verifySlot( SaveData *self, int slot );				/* 0x020679a8 */
int		SaveData_verifySmallBlock( SaveData *self, u32 offset );		/* 0x02067c44 */
int		SaveData_verifyPlayerBlock( SaveData *self, u32 offset );		/* 0x02067d34 */
int		SaveData_verifyWorldBlock( SaveData *self, u32 offset );		/* 0x02067e24 */
void	SaveData_clearSlots( SaveData *self );							/* 0x02067f18 */
void	SaveData_format( SaveData *self );								/* 0x02067f50 */
void	SaveData_freeSmallBlocks( SaveData *self );						/* 0x02068024 */
void	SaveData_freePlayerBlocks( SaveData *self );					/* 0x0206803c */
void	SaveData_freeWorldBlocks( SaveData *self );						/* 0x02068054 */
void	SaveData_nop( SaveData *self );									/* 0x0206806c */

ScriptThread *ScriptThread_construct( ScriptThread *self );	/* 0x0204ce08 */
void	ScriptThread_reset( ScriptThread *self );			/* 0x0204ce1c */
int		ScriptThread_runEvent( ScriptThread *self, ScriptFrame *frame );	/* 0x0204ce30 */
int		ScriptThread_executeTile( ScriptThread *self, int tx, int ty, int flags, int nested );	/* 0x0204cfe4 */
int		ScriptThread_queueTile( ScriptThread *self, int tx, int ty, int flags, int nested );	/* 0x0204d0c4 */
void	ScriptThread_pushFrame( ScriptThread *self, u32 event, u32 flags, int nested );	/* 0x0204d1c0 */
int		ScriptThread_run( ScriptThread *self );				/* 0x0204d250 */
void	ScriptThread_popFrame( ScriptThread *self );		/* 0x0204d35c */
void	ScriptThread_init( ScriptThread *self );			/* 0x0204d370 */
int		ScriptThread_executeEvent( ScriptThread *self, ScriptFrame *frame, int op, u32 arg, u32 flags );	/* 0x0204d388 */

/* ---------------------------------------------------------------- Weapons (Combat.cpp) */

/* the DS's 0x24-byte weapon (Weapon_init 0x02024104; Combat+0xc8 + i * 0x24, the spells' in the spell circle) */
typedef struct {
	u8		pad0[4];
	int		dmgMin, dmgMax;			/* +0x04 */
	u8		rangeMin, rangeMax;		/* +0x0c tiles */
	u16		accuracy;				/* +0x0e */
	int		ammoType;				/* +0x10 Player+0x3d + it (1: the wand's energy) */
	int		cost;					/* +0x14 ammo a shot */
	int		damageScale;			/* +0x18 */
	u8		effect;					/* +0x1c projectile / effect (0x1c..0x1f: the spells) */
	u8		numShots;				/* +0x1d */
	u8		shotDelay;				/* +0x1e */
	u8		pad1f[5];
} Weapon;
SIZE_IS( Weapon, 0x24 );
void	Weapon_init( Weapon *w, int dmgMin, int dmgMax, int rangeMin, int rangeMax, int accuracy, int ammoType,
			int cost, int damageScale, int effect, int numShots, int shotDelay );	/* 0x02024104 */

/* ---------------------------------------------------------------- SpellCircle (the touch screen's, 0x02068070..) */

/* one global at 0x02093bfc; the spells' records (icon place, name, sequence) at 0x0206bd00 */
typedef struct {
	int			touched;				/* +0x00 */
	u8			pad4[0xc - 4];
	int			open;					/* +0x0c */
	void		*emitters;				/* +0x10 the stylus trail's sparkles (300) */
	u8			pad14[0x1c - 0x14];
	App			*app;					/* +0x1c */
	MenuSystem	*menu;					/* +0x20 */
	void		*ui;					/* +0x24 */
	u8			pad28[0x30 - 0x28];
	s16			s30, s32;				/* +0x30 */
	u8			b34, b35;				/* +0x34 */
	s16			s36;					/* +0x36 */
	u8			pad38[2];
	s16			s3a;					/* +0x3a */
	Weapon		spells[4];				/* +0x3c force, fire, lightning, stun */
	u8			padcc[0x1cc - 0xcc];
	s8			numTouches;				/* +0x1cc */
	u8			doneMask;				/* +0x1cd icons put in */
	u8			litMask;				/* +0x1ce */
	u8			castMask;				/* +0x1cf the spells affordable */
	u8			pad1d0[4];
	s8			step;					/* +0x1d4 icons of the sequence put in */
	s8			selected;				/* +0x1d5 the icon chosen with the pad */
	u8			pressedMask;			/* +0x1d6 */
	s8			spell;					/* +0x1d7 being cast */
	void		*casting;				/* +0x1d8 its record (0x0206bd00 + spell * 0x24), NULL: picking */
	int			castTime;				/* +0x1dc the sequence done: the name shows 1200 ms */
	int			blinkTime, blinkEnd;	/* +0x1e0 */
	u8			blinkMask;				/* +0x1e8 */
	u8			pad1e9[3];
	int			blinkOn;				/* +0x1ec */
} SpellCircle;
FIELD_AT( SpellCircle, spells, 0x3c );	FIELD_AT( SpellCircle, numTouches, 0x1cc );	FIELD_AT( SpellCircle, casting, 0x1d8 );
FIELD_AT( SpellCircle, blinkOn, 0x1ec );
extern SpellCircle spellCircle;			/* 0x02093bfc */
void	SpellCircle_reset( int redraw );						/* 0x02068b84 */
void	SpellCircle_iconPos( int icon, int *x, int *y );		/* 0x02068d18 */
int		SpellCircle_init( App **app );							/* 0x020696a4 */
int		SpellCircle_canCast( App **app, u32 mask );				/* 0x02069d14 */

/* ---------------------------------------------------------------- Stream (Stream.cpp: ROM files and saves) */

#define STREAM_BUFFER_SIZE	0x4000
#define SAVE_MARKER			0xcafebabeu		/* between the parts of a save */

extern u8	streamBuffer[STREAM_BUFFER_SIZE];		/* 0x02074fd4 the InputStreams' (one at a time) */
extern u8	*outputBuffer;							/* *0x0206f69c the OutputStream's */
extern u16	streamLockId;							/* 0x02074fd0 the OutputStream's card lock */

struct InputStream {
	int		size;			/* +0x00 the file's */
	App		*app;			/* +0x04 */
	int		isOpen;			/* +0x08 */
	int		pos;			/* +0x0c in buffer */
	FSFile	file;			/* +0x10 */
	u8		*buffer;		/* +0x58 streamBuffer */
	int		fromBackup;		/* +0x5c */
	int		mode;			/* +0x60 1: reading for a write (its error says so) */
	u32		backupStart;	/* +0x64 */
	u32		backupPos;		/* +0x68 */
	u16		lockId;			/* +0x6c */
	u8		pad6e[2];
	int		cardResult;		/* +0x70 */
};
FIELD_AT( InputStream, buffer, 0x58 );	FIELD_AT( InputStream, cardResult, 0x70 );

struct OutputStream {
	u32		start;			/* +0x00 in the backup */
	u32		pos;			/* +0x04 */
	u32		buffered;		/* +0x08 in outputBuffer */
	u32		size;			/* +0x0c */
	int		cardResult;		/* +0x10 */
};

InputStream *InputStream_construct( InputStream *self, int mode );	/* 0x02053bb4 */
int		InputStream_open( InputStream *self, const char *path );	/* 0x02053bfc */
int		InputStream_openBackup( InputStream *self, u32 offset, int size );	/* 0x02053c88 */
void	InputStream_close( InputStream *self );				/* 0x02053cd8 */
int		InputStream_fill( InputStream *self, int n );		/* 0x02053d08 */
int		InputStream_read( InputStream *self, void *dst, int n );	/* 0x02053d88 */
int		InputStream_readIntDirect( InputStream *self );		/* 0x02053eb4 */
int		InputStream_readCompressed( InputStream *self, const u32 *header, void *dst, u32 maxSize, u32 packedSize );	/* 0x02053ecc */
void	InputStream_seek( InputStream *self, int relative, u32 offset );	/* 0x02053fb0 */
int		InputStream_readInts( InputStream *self, int *dst, int count );	/* 0x02054050 */
int		InputStream_readShorts( InputStream *self, s16 *dst, int count );	/* 0x02054060 */
int		InputStream_readInt( InputStream *self );			/* 0x02054070 */
int		InputStream_readShort( InputStream *self );			/* 0x020540c4 */
int		InputStream_getPosition( InputStream *self );		/* 0x020540fc */
int		OutputStream_open( OutputStream *self, u32 offset, u32 size );	/* 0x0205412c */
void	OutputStream_flush( OutputStream *self );			/* 0x020541c0 */
int		OutputStream_getPosition( OutputStream *self );		/* 0x020542bc */
void	OutputStream_close( OutputStream *self );			/* 0x02054404 */
void	OutputStream_write( OutputStream *self, const void *data, int n, int flush );	/* 0x020542d4 */
void	OutStream_writeBool( OutputStream *self, int v );	/* 0x02054370 */
void	OutStream_writeByte( OutputStream *self, int v );	/* 0x02054398 */
void	OutStream_writeShort( OutputStream *self, int v );	/* 0x020543bc */
void	OutStream_writeInt( OutputStream *self, int v );	/* 0x020543e0 */

/* (ScriptThread.cpp's last functions, after executeEvent) */
int		ScriptThread_checkWait( ScriptThread *self, int now );	/* 0x02052710 */
void	ScriptThread_saveState( ScriptThread *self, OutputStream *os );	/* 0x02052744 */
void	ScriptThread_loadState( ScriptThread *self, InputStream *is );	/* 0x020527f4 */
void	ScriptThread_signal( ScriptThread *self, u32 frameFlags );	/* 0x020528f4 */

/* (Sound.cpp, between ScriptThread and Stream) */
void	Sound_saveEmitters( Sound *self, OutputStream *os );	/* 0x02053a74 */
void	Sound_loadEmitters( Sound *self, InputStream *is );		/* 0x02053af0 */

#endif