/*
Copyright (C) 2001-2022 The Exult Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "CombatStats_gump.h"

#include "Gump_manager.h"
#include "Paperdoll_gump.h"
#include "actors.h"
#include "game.h"
#include "gamewin.h"
#include "items.h"
#include "misc_buttons.h"

#include <algorithm>

/*
 *  Statics:
 */

static const int colx    = 110;
static const int coldx   = 29;
static const int rowy[7] = {15, 29, 42, 73, 87, 93, 106};
// ponytail: gumps/cstats/N art only has frames for party_size 1-9 (Avatar +
// up to 8). Sizes above that reuse the widest frame and squeeze columns to
// fit the same span instead of overflowing the gump. Real fix is new art
// frames sized for party_size up to EXULT_PARTY_MAX + 1.
static const int cstats_art_sizes = 9;

/*
 *  Create stats display.
 */
CombatStats_gump::CombatStats_gump(int initx, int inity) : Gump(nullptr, initx, inity, game->get_shape("gumps/cstats/1")) {
	set_object_area(TileRect(0, 0, 0, 0), 23, 83);

	party_size = gwin->get_party(party, 1);

	const int shnum = game->get_shape("gumps/cstats/1") + std::min(party_size, cstats_art_sizes) - 1;
	ShapeID::set_shape(shnum, 0);

	col_step = party_size > cstats_art_sizes
					   ? (coldx * (cstats_art_sizes - 1)) / (party_size - 1)
					   : coldx;

	int i;    // Blame MSVC
	for (i = 0; i < party_size; i++) {
		add_elem(new Halo_button(this, colx + i * col_step, rowy[4], party[i]));
		add_elem(new Combat_mode_button(this, colx + i * col_step + 1, rowy[3], party[i]));
		add_elem(new Face_button(this, colx + i * col_step - 13, rowy[0], party[i]));
	}
}

/*
 *  Paint on screen.
 */

void CombatStats_gump::paint() {
	Gump_manager* gman = gumpman;

	Gump::paint();

	if (gwin->failed_copy_protection()) {
		const int   oinkx = 91;
		const char* oink  = get_text_msg(0x6F1 - msg_file_start);    // "Oink!"
		for (int i = 0; i < party_size; i++) {
			sman->paint_text(2, oink, x + oinkx + i * col_step, y + rowy[1]);
			sman->paint_text(2, oink, x + oinkx + i * col_step, y + rowy[2]);
		}
		sman->paint_text(2, oink, x + oinkx, y + rowy[5]);
		sman->paint_text(2, oink, x + oinkx, y + rowy[6]);
	} else {
		// stats for all party members
		for (int i = 0; i < party_size; i++) {
			gman->paint_num(party[i]->get_effective_prop(Actor::combat), x + colx + i * col_step, y + rowy[1]);
			gman->paint_num(party[i]->get_property(Actor::health), x + colx + i * col_step, y + rowy[2]);
		}
		// magic stats only for Avatar
		gman->paint_num(party[0]->get_property(Actor::magic), x + colx, y + rowy[5]);
		gman->paint_num(party[0]->get_property(Actor::mana), x + colx, y + rowy[6]);
	}
}
