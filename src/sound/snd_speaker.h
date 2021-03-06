/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box distribution.
 *
 *		Header of the emulation of the PC speaker.
 *
 * Version:	@(#)snd_speaker.h	1.0.0	2019/11/15
 *
 * Authors:	Sarah Walker, <http://pcem-emulator.co.uk/>
 *		Miran Grca, <mgrca8@gmail.com>
 *
 *		Copyright 2008-2019 Sarah Walker.
 *		Copyright 2016-2019 Miran Grca.
 */
extern int	speaker_mute;

extern int	speaker_gated;
extern int	speaker_enable, was_speaker_enable;


extern void	speaker_init();

extern void	speaker_set_count(uint8_t new_m, int new_count);
extern void	speaker_update(void);
