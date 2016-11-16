/* Copyright holders: John Elliott
   see COPYING for more details
*/
/*Hercules InColor emulation*/

#include <stdlib.h>
#include "ibm.h"
#include "device.h"
#include "mem.h"
#include "timer.h"
#include "video.h"
#include "vid_herculesplus.h"


/* extended CRTC registers */

#define HERCULESPLUS_CRTC_XMODE   20 /* xMode register */
#define HERCULESPLUS_CRTC_UNDER   21	/* Underline */
#define HERCULESPLUS_CRTC_OVER    22 /* Overstrike */

/* character width */
#define HERCULESPLUS_CW    ((herculesplus->crtc[HERCULESPLUS_CRTC_XMODE] & HERCULESPLUS_XMODE_90COL) ? 8 : 9)

/* mode control register */
#define HERCULESPLUS_CTRL_GRAPH   0x02
#define HERCULESPLUS_CTRL_ENABLE  0x08
#define HERCULESPLUS_CTRL_BLINK   0x20
#define HERCULESPLUS_CTRL_PAGE1   0x80

/* CRTC status register */
#define HERCULESPLUS_STATUS_HSYNC 0x01		/* horizontal sync */
#define HERCULESPLUS_STATUS_LIGHT 0x02
#define HERCULESPLUS_STATUS_VIDEO 0x08
#define HERCULESPLUS_STATUS_ID    0x50		/* Card identification */
#define HERCULESPLUS_STATUS_VSYNC 0x80		/* -vertical sync */

/* configuration switch register */
#define HERCULESPLUS_CTRL2_GRAPH 0x01
#define HERCULESPLUS_CTRL2_PAGE1 0x02

/* extended mode register */
#define HERCULESPLUS_XMODE_RAMFONT 0x01
#define HERCULESPLUS_XMODE_90COL   0x02


/* Read/write control */
#define HERCULESPLUS_RWCTRL_WRMODE   0x30
#define HERCULESPLUS_RWCTRL_POLARITY 0x40

/* exception register */
#define HERCULESPLUS_EXCEPT_CURSOR  0x0F		/* Cursor colour */
#define HERCULESPLUS_EXCEPT_PALETTE 0x10		/* Enable palette register */
#define HERCULESPLUS_EXCEPT_ALTATTR 0x20		/* Use alternate attributes */



/* Default palette */
static unsigned char defpal[16] = 
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};

static uint32_t herculesplus_rgb[64];

/* Mapping of inks to RGB */
static unsigned char init_rgb[64][3] =
{
				// rgbRGB
        { 0x00, 0x00, 0x00 },	// 000000
        { 0x00, 0x00, 0xaa },	// 000001
        { 0x00, 0xaa, 0x00 },	// 000010
        { 0x00, 0xaa, 0xaa },	// 000011
        { 0xaa, 0x00, 0x00 },	// 000100
        { 0xaa, 0x00, 0xaa },	// 000101
        { 0xaa, 0xaa, 0x00 },	// 000110
        { 0xaa, 0xaa, 0xaa },	// 000111
        { 0x00, 0x00, 0x55 },	// 001000
        { 0x00, 0x00, 0xff },	// 001001
        { 0x00, 0xaa, 0x55 },	// 001010
        { 0x00, 0xaa, 0xff },	// 001011
        { 0xaa, 0x00, 0x55 },	// 001100
        { 0xaa, 0x00, 0xff },	// 001101
        { 0xaa, 0xaa, 0x55 },	// 001110
        { 0xaa, 0xaa, 0xff },	// 001111
        { 0x00, 0x55, 0x00 },	// 010000
        { 0x00, 0x55, 0xaa },	// 010001
        { 0x00, 0xff, 0x00 },	// 010010
        { 0x00, 0xff, 0xaa },	// 010011
        { 0xaa, 0x55, 0x00 },	// 010100
        { 0xaa, 0x55, 0xaa },	// 010101
        { 0xaa, 0xff, 0x00 },	// 010110
        { 0xaa, 0xff, 0xaa },	// 010111
        { 0x00, 0x55, 0x55 },	// 011000
        { 0x00, 0x55, 0xff },	// 011001
        { 0x00, 0xff, 0x55 },	// 011010
        { 0x00, 0xff, 0xff },	// 011011
        { 0xaa, 0x55, 0x55 },	// 011100
        { 0xaa, 0x55, 0xff },	// 011101
        { 0xaa, 0xff, 0x55 },	// 011110
        { 0xaa, 0xff, 0xff },	// 011111
        { 0x55, 0x00, 0x00 },	// 100000
        { 0x55, 0x00, 0xaa },	// 100001
        { 0x55, 0xaa, 0x00 },	// 100010
        { 0x55, 0xaa, 0xaa },	// 100011
        { 0xff, 0x00, 0x00 },	// 100100
        { 0xff, 0x00, 0xaa },	// 100101
        { 0xff, 0xaa, 0x00 },	// 100110
        { 0xff, 0xaa, 0xaa },	// 100111
        { 0x55, 0x00, 0x55 },	// 101000
        { 0x55, 0x00, 0xff },	// 101001
        { 0x55, 0xaa, 0x55 },	// 101010
        { 0x55, 0xaa, 0xff },	// 101011
        { 0xff, 0x00, 0x55 },	// 101100
        { 0xff, 0x00, 0xff },	// 101101
        { 0xff, 0xaa, 0x55 },	// 101110
        { 0xff, 0xaa, 0xff },	// 101111
        { 0x55, 0x55, 0x00 },	// 110000
        { 0x55, 0x55, 0xaa },	// 110001
        { 0x55, 0xff, 0x00 },	// 110010
        { 0x55, 0xff, 0xaa },	// 110011
        { 0xff, 0x55, 0x00 },	// 110100
        { 0xff, 0x55, 0xaa },	// 110101
        { 0xff, 0xff, 0x00 },	// 110110
        { 0xff, 0xff, 0xaa },	// 110111
        { 0x55, 0x55, 0x55 },	// 111000
        { 0x55, 0x55, 0xff },	// 111001
        { 0x55, 0xff, 0x55 },	// 111010
        { 0x55, 0xff, 0xff },	// 111011
        { 0xff, 0x55, 0x55 },	// 111100
        { 0xff, 0x55, 0xff },	// 111101
        { 0xff, 0xff, 0x55 },	// 111110
        { 0xff, 0xff, 0xff },	// 111111
};



typedef struct herculesplus_t
{
        mem_mapping_t mapping;
        
        uint8_t crtc[32];
        int crtcreg;

        uint8_t ctrl, ctrl2, stat;

        int dispontime, dispofftime;
        int vidtime;
        
        int firstline, lastline;

        int linepos, displine;
        int vc, sc;
        uint16_t ma, maback;
        int con, coff, cursoron;
        int dispon, blink;
        int vsynctime, vadj;

        uint8_t *vram;
} herculesplus_t;

void herculesplus_recalctimings(herculesplus_t *herculesplus);
void herculesplus_write(uint32_t addr, uint8_t val, void *p);
uint8_t herculesplus_read(uint32_t addr, void *p);


void herculesplus_out(uint16_t addr, uint8_t val, void *p)
{
        herculesplus_t *herculesplus = (herculesplus_t *)p;
/*        pclog("InColor out %04X %02X\n",addr,val); */
        switch (addr)
        {
                case 0x3b0: case 0x3b2: case 0x3b4: case 0x3b6:
                herculesplus->crtcreg = val & 31;
                return;
                case 0x3b1: case 0x3b3: case 0x3b5: case 0x3b7:
		if (herculesplus->crtcreg > 22) return;
                herculesplus->crtc[herculesplus->crtcreg] = val;
                if (herculesplus->crtc[10] == 6 && herculesplus->crtc[11] == 7) /*Fix for Generic Turbo XT BIOS, which sets up cursor registers wrong*/
                {
                        herculesplus->crtc[10] = 0xb;
                        herculesplus->crtc[11] = 0xc;
                }
                herculesplus_recalctimings(herculesplus);
                return;
                case 0x3b8:
                herculesplus->ctrl = val;
                return;
                case 0x3bf:
                herculesplus->ctrl2 = val;
                if (val & 2)
                        mem_mapping_set_addr(&herculesplus->mapping, 0xb0000, 0x10000);
                else
                        mem_mapping_set_addr(&herculesplus->mapping, 0xb0000, 0x08000);
                return;
        }
}

uint8_t herculesplus_in(uint16_t addr, void *p)
{
        herculesplus_t *herculesplus = (herculesplus_t *)p;
/*        pclog("InColor in %04X %02X %04X:%04X %04X\n",addr,(herculesplus->stat & 0xF) | ((herculesplus->stat & 8) << 4),CS,pc,CX); */
        switch (addr)
        {
                case 0x3b0: case 0x3b2: case 0x3b4: case 0x3b6:
                return herculesplus->crtcreg;
                case 0x3b1: case 0x3b3: case 0x3b5: case 0x3b7:
		if (herculesplus->crtcreg > 22) return 0xff;
                return herculesplus->crtc[herculesplus->crtcreg];
                case 0x3ba:
		/* 0x50: InColor card identity */
                return (herculesplus->stat & 0xf) | ((herculesplus->stat & 8) << 4) | 0x10;
        }
        return 0xff;
}

void herculesplus_write(uint32_t addr, uint8_t val, void *p)
{
        herculesplus_t *herculesplus = (herculesplus_t *)p;

	unsigned char wmask = herculesplus->crtc[HERCULESPLUS_CRTC_MASK];
	unsigned char wmode = herculesplus->crtc[HERCULESPLUS_CRTC_RWCTRL] & HERCULESPLUS_RWCTRL_WRMODE;
	unsigned char fg    = herculesplus->crtc[HERCULESPLUS_CRTC_RWCOL] & 0x0F;
	unsigned char bg    = (herculesplus->crtc[HERCULESPLUS_CRTC_RWCOL] >> 4)&0x0F;
	unsigned char w;
	unsigned char vmask;	/* Mask of bit within byte */
	unsigned char pmask;	/* Mask of plane within colour value */
	unsigned char latch;

        egawrites++;

	/* Horrible hack, I know, but it's the only way to fix the 440FX BIOS filling the VRAM with garbage until Tom fixes the memory emulation. */
	if ((cs == 0xE0000) && (cpu_state.pc == 0xBF2F) && (romset == ROM_440FX))  return;
	if ((cs == 0xE0000) && (cpu_state.pc == 0xBF77) && (romset == ROM_440FX))  return;

	addr &= 0xFFFF;

	herculesplus->vram[addr] = val;
}

uint8_t herculesplus_read(uint32_t addr, void *p)
{
        herculesplus_t *herculesplus = (herculesplus_t *)p;
	unsigned plane;
	unsigned char lp    = herculesplus->crtc[HERCULESPLUS_CRTC_PROTECT];
	unsigned char value = 0;
	unsigned char dc;	/* "don't care" register */
	unsigned char bg;	/* background colour */
	unsigned char fg;
	unsigned char mask, pmask;

        egareads++;

	addr &= 0xFFFF;
    return herculesplus->vram[addr];
}



void herculesplus_recalctimings(herculesplus_t *herculesplus)
{
        double disptime;
	double _dispontime, _dispofftime;
        disptime = herculesplus->crtc[0] + 1;
        _dispontime  = herculesplus->crtc[1];
        _dispofftime = disptime - _dispontime;
        _dispontime  *= MDACONST;
        _dispofftime *= MDACONST;
	herculesplus->dispontime  = (int)(_dispontime  * (1 << TIMER_SHIFT));
	herculesplus->dispofftime = (int)(_dispofftime * (1 << TIMER_SHIFT));
}


static void herculesplus_draw_char_rom(herculesplus_t *herculesplus, int x, uint8_t chr, uint8_t attr)
{
	unsigned            i;
	int                 elg, blk;
	unsigned            ull;
	unsigned            val;
	unsigned	    ifg, ibg;
	const unsigned char *fnt;
	uint32_t	    fg, bg;
	int		    cw = HERCULESPLUS_CW;

	blk = 0;
	if (herculesplus->ctrl & HERCULESPLUS_CTRL_BLINK) 
	{
		if (attr & 0x80) 
		{
			blk = (herculesplus->blink & 16);
		}
		attr &= 0x7f;
	}

	/* MDA-compatible attributes */
	ibg = 0;
	ifg = 7;
	if ((attr & 0x77) == 0x70)	 /* Invert */
	{
		ifg = 0;
		ibg = 7;
	}	
	if (attr & 8) 
	{
		ifg |= 8;	/* High intensity FG */
	}
	if (attr & 0x80) 
	{
		ibg |= 8;	/* High intensity BG */
	}
	if ((attr & 0x77) == 0)	/* Blank */
	{
		ifg = ibg;
	}
	ull = ((attr & 0x07) == 1) ? 13 : 0xffff;

	fg = herculesplus_rgb[defpal[ifg]];
	bg = herculesplus_rgb[defpal[ibg]];

	if (herculesplus->crtc[HERCULESPLUS_CRTC_XMODE] & HERCULESPLUS_XMODE_90COL) 
	{
		elg = 0;
	} 
	else 
	{
		elg = ((chr >= 0xc0) && (chr <= 0xdf));
	}

	fnt = &(fontdatm[chr][herculesplus->sc]);

	if (blk)
	{
		val = 0x000;	/* Blinking, draw all background */
	}
	else if (herculesplus->sc == ull) 
	{
		val = 0x1ff;	/* Underscore, draw all foreground */
	}
	else 
	{
		val = fnt[0] << 1;
	
		if (elg) 
		{
			val |= (val >> 1) & 1;
		}
	}
	for (i = 0; i < cw; i++) 
	{
		((uint32_t *)buffer32->line[herculesplus->displine])[x * cw + i] = (val & 0x100) ? fg : bg;
		val = val << 1;
	}
}


static void herculesplus_draw_char_ram4(herculesplus_t *herculesplus, int x, uint8_t chr, uint8_t attr)
{
	unsigned            i;
	int                 elg, blk;
	unsigned            ull;
	unsigned            val;
	unsigned	    ifg, ibg, cfg, pmask, plane;
	const unsigned char *fnt;
	uint32_t	    fg;
	int		    cw = HERCULESPLUS_CW;
	int                 blink   = herculesplus->ctrl & HERCULESPLUS_CTRL_BLINK;

	blk = 0;
	if (blink)
	{
		if (attr & 0x80) 
		{
			blk = (herculesplus->blink & 16);
		}
		attr &= 0x7f;
	}

	/* MDA-compatible attributes */
	ibg = 0;
	ifg = 7;
	if ((attr & 0x77) == 0x70)	 /* Invert */
	{
		ifg = 0;
		ibg = 7;
	}	
	if (attr & 8) 
	{
		ifg |= 8;	/* High intensity FG */
	}
	if (attr & 0x80) 
	{
		ibg |= 8;	/* High intensity BG */
	}
	if ((attr & 0x77) == 0)	/* Blank */
	{
		ifg = ibg;
	}
	ull = ((attr & 0x07) == 1) ? 13 : 0xffff;
	if (herculesplus->crtc[HERCULESPLUS_CRTC_XMODE] & HERCULESPLUS_XMODE_90COL) 
	{
		elg = 0;
	} 
	else 
	{
		elg = ((chr >= 0xc0) && (chr <= 0xdf));
	}
	fnt = herculesplus->vram + 0x4000 + 16 * chr + herculesplus->sc;

	if (blk)
	{
		/* Blinking, draw all background */
		val = 0x000;	
	}
	else if (herculesplus->sc == ull) 
	{
		/* Underscore, draw all foreground */
		val = 0x1ff;
	}
	else 
	{
		val = fnt[0x00000] << 1;
	
		if (elg) 
		{
			val |= (val[0] >> 1) & 1;
		}
	}
	for (i = 0; i < cw; i++) 
	{
		/* Generate pixel colour */
		cfg = 0;
		pmask = 1;
		/* cfg = colour of foreground pixels */
		if (altattr && (attr & 0x77) == 0) cfg = ibg; /* 'blank' attribute */
		if (palette)
		{
			fg = herculesplus_rgb[herculesplus->palette[cfg]];
		} 
		else 
		{
			fg = herculesplus_rgb[defpal[cfg]];
		}
		
		((uint32_t *)buffer32->line[herculesplus->displine])[x * cw + i] = fg;
		val = val << 1;
	}
}


static void herculesplus_draw_char_ram48(herculesplus_t *herculesplus, int x, uint8_t chr, uint8_t attr)
{
	unsigned            i;
	int                 elg, blk, ul, ol, bld;
	unsigned            ull, oll, ulc, olc;
	unsigned            val;
	unsigned	    ifg, ibg, cfg, pmask, plane;
	const unsigned char *fnt;
	uint32_t	    fg;
	int		    cw = HERCULESPLUS_CW;
	int                 blink   = herculesplus->ctrl & HERCULESPLUS_CTRL_BLINK;
	int		    font = (attr & 0x0F);

	if (font >= 12) font &= 7;

	blk = 0;
	if (blink)
	{
		if (attr & 0x40) 
		{
			blk = (herculesplus->blink & 16);
		}
		attr &= 0x7f;
	}
	/* MDA-compatible attributes */
	if (blink) 
	{
		ibg = (attr & 0x80) ? 8 : 0;
		bld = 0;
		ol  = (attr & 0x20) ? 1 : 0;
		ul  = (attr & 0x10) ? 1 : 0;
	} 
	else 
	{
		bld = (attr & 0x80) ? 1 : 0;
		ibg = (attr & 0x40) ? 0x0F : 0;
		ol  = (attr & 0x20) ? 1 : 0;
		ul  = (attr & 0x10) ? 1 : 0;
	}
	if (ul) 
	{ 
		ull = herculesplus->crtc[HERCULESPLUS_CRTC_UNDER] & 0x0F;
		ulc = (herculesplus->crtc[HERCULESPLUS_CRTC_UNDER] >> 4) & 0x0F;
		if (ulc == 0) ulc = 7;
	} 
	else 
	{
		ull = 0xFFFF;
	}
	if (ol) 
	{ 
		oll = herculesplus->crtc[HERCULESPLUS_CRTC_OVER] & 0x0F;
		olc = (herculesplus->crtc[HERCULESPLUS_CRTC_OVER] >> 4) & 0x0F;
		if (olc == 0) olc = 7;
	} 
	else 
	{
		oll = 0xFFFF;
	}

	if (herculesplus->crtc[HERCULESPLUS_CRTC_XMODE] & HERCULESPLUS_XMODE_90COL) 
	{
		elg = 0;
	} 
	else 
	{
		elg = ((chr >= 0xc0) && (chr <= 0xdf));
	}
	fnt = herculesplus->vram + 0x4000 + 16 * chr + 4096 * font + herculesplus->sc;

	if (blk)
	{
		/* Blinking, draw all background */
		val = 0x000;	
	}
	else if (herculesplus->sc == ull) 
	{
		/* Underscore, draw all foreground */
		val = 0x1ff;
	}
	else 
	{
		val = fnt[0x00000] << 1;
	
		if (elg) 
		{
			val |= (val >> 1) & 1;
		}
		if (bld) 
		{
			val |= (val >> 1);
		}
	}
	for (i = 0; i < cw; i++) 
	{
		/* Generate pixel colour */
		cfg = 0;
		pmask = 1;
		if (herculesplus->sc == oll)
		{
			cfg = olc ^ ibg;	/* Strikethrough */
		}
		else if (herculesplus->sc == ull)
		{
			cfg = ulc ^ ibg;	/* Underline */
		}
		else
		{
            cfg |= (ibg & pmask);
		}
		
		((uint32_t *)buffer32->line[herculesplus->displine])[x * cw + i] = fg;
		val = val << 1;
	}
}






static void herculesplus_text_line(herculesplus_t *herculesplus, uint16_t ca)
{
        int drawcursor;
	int x, c;
        uint8_t chr, attr;
	uint32_t col;

	for (x = 0; x < herculesplus->crtc[1]; x++)
	{
		chr  = herculesplus->vram[(herculesplus->ma << 1) & 0x3fff];
                attr = herculesplus->vram[((herculesplus->ma << 1) + 1) & 0x3fff];

                drawcursor = ((herculesplus->ma == ca) && herculesplus->con && herculesplus->cursoron);

		switch (herculesplus->crtc[HERCULESPLUS_CRTC_XMODE] & 5)
		{
			case 0:
			case 4:	/* ROM font */
				herculesplus_draw_char_rom(herculesplus, x, chr, attr);
				break;
			case 1: /* 4k RAMfont */
				herculesplus_draw_char_ram4(herculesplus, x, chr, attr);
				break;
			case 5: /* 48k RAMfont */
				herculesplus_draw_char_ram48(herculesplus, x, chr, attr);
				break;

		}
		++herculesplus->ma;
                if (drawcursor)
                {
			int cw = HERCULESPLUS_CW;
			uint8_t ink = (attr & 0x08) | 7;

			col = defpal[ink];
			for (c = 0; c < cw; c++)
			{
				((uint32_t *)buffer32->line[herculesplus->displine])[x * cw + c] = col;
			}
		}
	}
}


static void herculesplus_graphics_line(herculesplus_t *herculesplus)
{
	uint8_t mask;
	uint16_t ca;
	int x, c, plane, col;
	uint8_t ink;
	uint8_t val;

	/* Graphics mode. */
        ca = (herculesplus->sc & 3) * 0x2000;
        if ((herculesplus->ctrl & HERCULESPLUS_CTRL_PAGE1) && (herculesplus->ctrl2 & HERCULESPLUS_CTRL2_PAGE1))
		ca += 0x8000;

	for (x = 0; x < herculesplus->crtc[1]; x++)
	{
		val = herculesplus->vram[((herculesplus->ma << 1) & 0x1fff) + ca + 0x10000 * plane]

		herculesplus->ma++;
		for (c = 0; c < 8; c++)
		{
			val >>= 1;
			col = defpal[val & 1];

			((uint32_t *)buffer32->line[herculesplus->displine])[(x << 4) + c] = herculesplus_rgb[col];
		}
	}
}

void herculesplus_poll(void *p)
{
        herculesplus_t *herculesplus = (herculesplus_t *)p;
        uint16_t ca = (herculesplus->crtc[15] | (herculesplus->crtc[14] << 8)) & 0x3fff;
        int x;
        int oldvc;
        int oldsc;

        if (!herculesplus->linepos)
        {
//                pclog("InColor poll %i %i\n", herculesplus->vc, herculesplus->sc);
                herculesplus->vidtime += herculesplus->dispofftime;
                herculesplus->stat |= 1;
                herculesplus->linepos = 1;
                oldsc = herculesplus->sc;
                if ((herculesplus->crtc[8] & 3) == 3) 
                        herculesplus->sc = (herculesplus->sc << 1) & 7;
                if (herculesplus->dispon)
                {
                        if (herculesplus->displine < herculesplus->firstline)
                        {
                                herculesplus->firstline = herculesplus->displine;
                                video_wait_for_buffer();
                        }
                        herculesplus->lastline = herculesplus->displine;
                        if ((herculesplus->ctrl & HERCULESPLUS_CTRL_GRAPH) && (herculesplus->ctrl2 & HERCULESPLUS_CTRL2_GRAPH))
                        {
				herculesplus_graphics_line(herculesplus);
                        }
                        else
                        {
				herculesplus_text_line(herculesplus, ca);
                        }
                }
                herculesplus->sc = oldsc;
                if (herculesplus->vc == herculesplus->crtc[7] && !herculesplus->sc)
                {
                        herculesplus->stat |= 8;
//                        printf("VSYNC on %i %i\n",vc,sc);
                }
                herculesplus->displine++;
                if (herculesplus->displine >= 500) 
                        herculesplus->displine = 0;
        }
        else
        {
                herculesplus->vidtime += herculesplus->dispontime;
                if (herculesplus->dispon) 
                        herculesplus->stat &= ~1;
                herculesplus->linepos = 0;
                if (herculesplus->vsynctime)
                {
                        herculesplus->vsynctime--;
                        if (!herculesplus->vsynctime)
                        {
                                herculesplus->stat &= ~8;
//                                printf("VSYNC off %i %i\n",vc,sc);
                        }
                }
                if (herculesplus->sc == (herculesplus->crtc[11] & 31) || ((herculesplus->crtc[8] & 3) == 3 && herculesplus->sc == ((herculesplus->crtc[11] & 31) >> 1))) 
                { 
                        herculesplus->con = 0; 
                        herculesplus->coff = 1; 
                }
                if (herculesplus->vadj)
                {
                        herculesplus->sc++;
                        herculesplus->sc &= 31;
                        herculesplus->ma = herculesplus->maback;
                        herculesplus->vadj--;
                        if (!herculesplus->vadj)
                        {
                                herculesplus->dispon = 1;
                                herculesplus->ma = herculesplus->maback = (herculesplus->crtc[13] | (herculesplus->crtc[12] << 8)) & 0x3fff;
                                herculesplus->sc = 0;
                        }
                }
                else if (herculesplus->sc == herculesplus->crtc[9] || ((herculesplus->crtc[8] & 3) == 3 && herculesplus->sc == (herculesplus->crtc[9] >> 1)))
                {
                        herculesplus->maback = herculesplus->ma;
                        herculesplus->sc = 0;
                        oldvc = herculesplus->vc;
                        herculesplus->vc++;
                        herculesplus->vc &= 127;
                        if (herculesplus->vc == herculesplus->crtc[6]) 
                                herculesplus->dispon = 0;
                        if (oldvc == herculesplus->crtc[4])
                        {
//                                printf("Display over at %i\n",displine);
                                herculesplus->vc = 0;
                                herculesplus->vadj = herculesplus->crtc[5];
                                if (!herculesplus->vadj) herculesplus->dispon=1;
                                if (!herculesplus->vadj) herculesplus->ma = herculesplus->maback = (herculesplus->crtc[13] | (herculesplus->crtc[12] << 8)) & 0x3fff;
                                if ((herculesplus->crtc[10] & 0x60) == 0x20) herculesplus->cursoron = 0;
                                else                                     herculesplus->cursoron = herculesplus->blink & 16;
                        }
                        if (herculesplus->vc == herculesplus->crtc[7])
                        {
                                herculesplus->dispon = 0;
                                herculesplus->displine = 0;
                                herculesplus->vsynctime = 16;//(crtcm[3]>>4)+1;
                                if (herculesplus->crtc[7])
                                {
//                                        printf("Lastline %i Firstline %i  %i\n",lastline,firstline,lastline-firstline);
                                        if ((herculesplus->ctrl & HERCULESPLUS_CTRL_GRAPH) && (herculesplus->ctrl2 & HERCULESPLUS_CTRL2_GRAPH)) 
					{
						x = herculesplus->crtc[1] << 4;
					}
                                        else
					{
                                               x = herculesplus->crtc[1] * 9;
					}
                                        herculesplus->lastline++;
                                        if (x != xsize || (herculesplus->lastline - herculesplus->firstline) != ysize)
                                        {
                                                xsize = x;
                                                ysize = herculesplus->lastline - herculesplus->firstline;
//                                                printf("Resize to %i,%i - R1 %i\n",xsize,ysize,crtcm[1]);
                                                if (xsize < 64) xsize = 656;
                                                if (ysize < 32) ysize = 200;
                                                updatewindowsize(xsize, ysize);
                                        }
					video_blit_memtoscreen(0, herculesplus->firstline, 0, herculesplus->lastline - herculesplus->firstline, xsize, herculesplus->lastline - herculesplus->firstline);
                                        frames++;
                                        if ((herculesplus->ctrl & HERCULESPLUS_CTRL_GRAPH) && (herculesplus->ctrl2 & HERCULESPLUS_CTRL2_GRAPH))
                                        {
                                                video_res_x = herculesplus->crtc[1] * 16;
                                                video_res_y = herculesplus->crtc[6] * 4;
                                                video_bpp = 1;
                                        }
                                        else
                                        {
                                                video_res_x = herculesplus->crtc[1];
                                                video_res_y = herculesplus->crtc[6];
                                                video_bpp = 0;
                                        }
                                }
                                herculesplus->firstline = 1000;
                                herculesplus->lastline = 0;
                                herculesplus->blink++;
                        }
                }
                else
                {
                        herculesplus->sc++;
                        herculesplus->sc &= 31;
                        herculesplus->ma = herculesplus->maback;
                }
                if ((herculesplus->sc == (herculesplus->crtc[10] & 31) || ((herculesplus->crtc[8] & 3) == 3 && herculesplus->sc == ((herculesplus->crtc[10] & 31) >> 1))))
                {
                        herculesplus->con = 1;
//                        printf("Cursor on - %02X %02X %02X\n",crtcm[8],crtcm[10],crtcm[11]);
                }
        }
}

void *herculesplus_init()
{
        int c;
        herculesplus_t *herculesplus = malloc(sizeof(herculesplus_t));
        memset(herculesplus, 0, sizeof(herculesplus_t));

        herculesplus->vram = malloc(0x10000);	/* 64k VRAM */

        timer_add(herculesplus_poll, &herculesplus->vidtime, TIMER_ALWAYS_ENABLED, herculesplus);
        mem_mapping_add(&herculesplus->mapping, 0xb0000, 0x10000, herculesplus_read, NULL, NULL, herculesplus_write, NULL, NULL,  NULL, 0, herculesplus);
        io_sethandler(0x03b0, 0x0010, herculesplus_in, NULL, NULL, herculesplus_out, NULL, NULL, herculesplus);

	for (c = 0; c < 64; c++)
	{
		herculesplus_rgb[c] = makecol32(init_rgb[c][0], init_rgb[c][1], init_rgb[c][2]);
	}

/* Initialise CRTC regs to safe values */
	herculesplus->crtc[HERCULESPLUS_CRTC_MASK  ] = 0x0F; /* All planes displayed */
	for (c = 0; c < 16; c++) 
	{
		herculesplus->palette[c] = defpal[c];
	}
	herculesplus->palette_idx = 0;

        return herculesplus;
}

void herculesplus_close(void *p)
{
        herculesplus_t *herculesplus = (herculesplus_t *)p;

        free(herculesplus->vram);
        free(herculesplus);
}

void herculesplus_speed_changed(void *p)
{
        herculesplus_t *herculesplus = (herculesplus_t *)p;
        
        herculesplus_recalctimings(herculesplus);
}

device_t herculesplus_device =
{
        "Hercules Plus",
        0,
        herculesplus_init,
        herculesplus_close,
        NULL,
        herculesplus_speed_changed,
        NULL,
        NULL
};