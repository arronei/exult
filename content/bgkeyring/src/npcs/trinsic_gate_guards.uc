/*
 *
 *  Copyright (C) 2026  The Exult Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*	NPC 257 and 258 are Trinsic's two generic gate guards. Their map data
 *	routes straight to the shared shape-806 guard conversation (function
 *	0x326) -- not through the 0x400+npcnum per-NPC slot -- which is why
 *	every generic guard in the game says "My name is not important."
 *	Overriding the shared shape function itself, with an itemref check for
 *	just these two, gives them a name without touching any other guard in
 *	the game. Everything else replicates the original conversation
 *	byte-for-byte.
 */

enum trinsic_gate_guards {
	ALFRED = -257,
	CONNER = -258,
	GUARD_FACE = -259
};

extern void letPassOnTrinsicPassword 0x834();

void TrinsicGateGuard object#(0x326) () {
	if (event == DOUBLECLICK) {
		GUARD_FACE->say("You see a tough-looking guard who takes his job -very- seriously.");
		say("@Halt! State your business.@");
		add(["name", "job", "bye"]);
		if (gflags[GOT_TRINSIC_PASSWORD]) {
			add("password");
		}
		if (!gflags[REFUSED_MURDER_INVESTIGATION]) {
			add("investigating a murder");
		}

		converse(0) {
			case "name" (remove):
				if (get_npc_number() == ALFRED) {
					say("@Alfred@");
					set_npc_name("Alfred");
					set_item_flag(MET);
				} else if (get_npc_number() == CONNER) {
					say("@Conner@");
					set_npc_name("Conner");
					set_item_flag(MET);
				} else {
					say("@My name is not important.@");
				}

			case "job":
				say("@I keep villains and knaves out of Trinsic and keep a record of all who leave. Thou must have a password to leave.@");
				add("password");

			case "investigating a murder" (remove):
				say("@I did get a report of a commotion at the east gate.@");
				add("commotion");
				add("east gate");

			case "commotion" (remove):
				say("@The report didn't give details.@");

			case "east gate" (remove):
				say("@The east gate is on the other side of town. The daily report said Johnson is on duty there.@");
				add("Johnson");

			case "Johnson" (remove):
				say("@He's the guard assigned to the east gate.@");

			case "password":
				say("@What is the password?@");
				var options = ["Please", "Long live the king", "Uhh, I don't know"];
				if (gflags[GOT_TRINSIC_PASSWORD]) {
					options = [options, "Blackbird"];
				}
				if (askForResponse(options) == "Blackbird") {
					letPassOnTrinsicPassword();
					say("@Very well, thou mayest pass.@*");
					abort;
				} else {
					say("@Thou dost not know the password. Sorry. The Mayor can give thee the proper password.@");
					gflags[NEEDS_TRINSIC_PASSWORD] = true;
				}

			case "bye":
				say("@Goodbye.@*");
				break;
		}
	}
}
