/**
 ** Information about a shapes file.
 **
 ** Written: 1/23/02 - JSF
 **/

/*
Copyright (C) 2002-2013 The Exult Team

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
#  include <config.h>
#endif

#include "shapefile.h"
#include "u7drag.h"
#include "shapegroup.h"
#include "shapevga.h"
#include "shapelst.h"
#include "chunklst.h"
#include "npclst.h"
#include "paledit.h"
#include "utils.h"
#include "Flex.h"
#include "exceptions.h"
#include "combo.h"
#include "studio.h"

using std::vector;
using std::string;
using std::cerr;
using std::endl;
using std::ofstream;

/*
 *  Delete file and groups.
 */

Shape_file_info::~Shape_file_info(
) {
	delete groups;
	delete browser;
}

/*
 *  Get the main browser for this file, or create it.
 *
 *  Output: ->browser.
 */

Object_browser *Shape_file_info::get_browser(
    Shape_file_info *vgafile,
    unsigned char *palbuf
) {
	if (browser)
		return browser;     // Okay.
	browser = create_browser(vgafile, palbuf, 0);
	// Add a reference (us).
	gtk_widget_ref(browser->get_widget());
	return browser;
}

/*
 *  Cleanup.
 */

Image_file_info::~Image_file_info(
) {
	delete ifile;
}

/*
 *  Create a browser for our data.
 */

Object_browser *Image_file_info::create_browser(
    Shape_file_info *vgafile,   // THE 'shapes.vga' file.
    unsigned char *palbuf,      // Palette for displaying.
    Shape_group *g          // Group, or 0.
) {
	Shape_chooser *chooser = new Shape_chooser(ifile, palbuf, 400, 64,
	        g, this);
	// Fonts?  Show 'A' as the default.
	if (strcasecmp(basename.c_str(), "fonts.vga") == 0)
		chooser->set_framenum0('A');
	if (this == vgafile) {      // Main 'shapes.vga' file?
		chooser->set_shapes_file(
		    static_cast<Shapes_vga_file *>(vgafile->get_ifile()));
	}
	return chooser;
}

/*
 *  Write out if modified.  May throw exception.
 */

void Image_file_info::flush(
) {
	if (!modified)
		return;
	modified = false;
	int nshapes = ifile->get_num_shapes();
	int shnum;          // First read all entries.
	Shape **shapes = new Shape *[nshapes];
	for (shnum = 0; shnum < nshapes; shnum++)
		shapes[shnum] = ifile->extract_shape(shnum);
	string filestr("<PATCH>/"); // Always write to 'patch'.
	filestr += basename;
	// !flex means single-shape.
	write_file(filestr.c_str(), shapes, nshapes, !ifile->is_flex());
	delete [] shapes;
	// Tell Exult to reload this file.
	unsigned char buf[Exult_server::maxlength];
	unsigned char *ptr = &buf[0];
	Write2(ptr, ifile->get_u7drag_type());
	ExultStudio *studio = ExultStudio::get_instance();
	studio->send_to_server(Exult_server::reload_shapes, buf, ptr - buf);
}

/*
 *  Revert to what's on disk.
 *
 *  Output: True (meaning we support 'revert').
 */

bool Image_file_info::revert(
) {
	if (modified) {
		ifile->load(pathname.c_str());
		modified = false;
	}
	return true;
}

/*
 *  Write a shape file.  (Note:  static method.)
 *  May print an error.
 */

void Image_file_info::write_file(
    const char *pathname,       // Full path.
    Shape **shapes,         // List of shapes to write.
    int nshapes,            // # shapes.
    bool single         // Don't write a FLEX file.
) {
	ofstream out;
	U7open(out, pathname);      // May throw exception.
	if (single) {
		if (nshapes)
			shapes[0]->write(out);
		out.flush();
		if (!out.good())
			throw file_write_exception(pathname);
		out.close();
		return;
	}
	Flex_writer writer(out, "Written by ExultStudio", nshapes);
	// Write all out.
	for (int shnum = 0; shnum < nshapes; shnum++) {
		if (shapes[shnum]->get_modified() || shapes[shnum]->get_from_patch())
			shapes[shnum]->write(out);
		writer.mark_section_done();
	}
	if (!writer.close())
		throw file_write_exception(pathname);
}

/*
 *  Cleanup.
 */

Chunks_file_info::~Chunks_file_info(
) {
	delete file;
}

/*
 *  Create a browser for our data.
 */

Object_browser *Chunks_file_info::create_browser(
    Shape_file_info *vgafile,   // THE 'shapes.vga' file.
    unsigned char *palbuf,      // Palette for displaying.
    Shape_group *g          // Group, or 0.
) {
	// Must be 'u7chunks' (for now).
	return new Chunk_chooser(vgafile->get_ifile(), *file, palbuf,
	                         400, 64, g);
}

/*
 *  Write out if modified.  May throw exception.
 */

void Chunks_file_info::flush(
) {
	if (!modified)
		return;
	modified = false;
	cerr << "Chunks should be stored by Exult" << endl;
}

/*
 *  Create a browser for our data.
 */

Object_browser *Npcs_file_info::create_browser(
    Shape_file_info *vgafile,   // THE 'shapes.vga' file.
    unsigned char *palbuf,      // Palette for displaying.
    Shape_group *g          // Group, or 0.
) {
	return new Npc_chooser(vgafile->get_ifile(), palbuf,
	                       400, 64, g, this);
}

/*
 *  Read in an NPC from Exult.
 */
bool Npcs_file_info::read_npc(unsigned num) {
	if (num > npcs.size())
		npcs.resize(num + 1);
	ExultStudio *studio = ExultStudio::get_instance();
	int server_socket = studio->get_server_socket();
	unsigned char buf[Exult_server::maxlength];
	Exult_server::Msg_type id;
	int datalen;
	unsigned char *ptr, *newptr;
	ptr = newptr = &buf[0];
	Write2(ptr, num);
	if (!studio->send_to_server(Exult_server::npc_info, buf, ptr - buf) ||
	        !Exult_server::wait_for_response(server_socket, 100) ||
	        (datalen = Exult_server::Receive_data(server_socket,
	                   id, buf, sizeof(buf))) == -1 ||
	        id != Exult_server::npc_info ||
	        Read2(newptr) != num)
		return false;

	npcs[num].shapenum = Read2(newptr); // -1 if unused.
	if (npcs[num].shapenum >= 0) {
		npcs[num].unused = (*newptr++ != 0);
		utf8Str utf8name(reinterpret_cast<char *>(newptr));
		npcs[num].name = utf8name;
	} else {
		npcs[num].unused = true;
		npcs[num].name = "";
	}
	return true;
}

/*
 *  Get Exult's list of NPC's.
 */

void Npcs_file_info::setup(
) {
	modified = false;
	npcs.resize(0);
	ExultStudio *studio = ExultStudio::get_instance();
	int server_socket = studio->get_server_socket();
	// Should get immediate answer.
	unsigned char buf[Exult_server::maxlength];
	Exult_server::Msg_type id;
	int num_npcs, datalen;
	if (Send_data(server_socket, Exult_server::npc_unused) == -1 ||
	        !Exult_server::wait_for_response(server_socket, 100) ||
	        (datalen = Exult_server::Receive_data(server_socket,
	                   id, buf, sizeof(buf))) == -1 ||
	        id != Exult_server::npc_unused) {
		cerr << "Error sending data to server." << endl;
		return;
	}
	unsigned char *ptr = &buf[0], *newptr;
	num_npcs = Read2(ptr);
	npcs.resize(num_npcs);
	for (int i = 0; i < num_npcs; ++i) {
		ptr = newptr = &buf[0];
		Write2(ptr, i);
		if (!studio->send_to_server(Exult_server::npc_info,
		                            buf, ptr - buf) ||
		        !Exult_server::wait_for_response(server_socket, 100) ||
		        (datalen = Exult_server::Receive_data(server_socket,
		                   id, buf, sizeof(buf))) == -1 ||
		        id != Exult_server::npc_info ||
		        Read2(newptr) != i) {
			npcs.resize(0);
			cerr << "Error getting info for NPC #" << i << endl;
			return;
		}
		npcs[i].shapenum = Read2(newptr);   // -1 if unused.
		if (npcs[i].shapenum >= 0) {
			npcs[i].unused = (*newptr++ != 0);
			utf8Str utf8name(reinterpret_cast<char *>(newptr));
			npcs[i].name = utf8name;
		} else {
			npcs[i].unused = true;
			npcs[i].name = "";
		}
	}
	return;
}

/*
 *  Init.
 */

Flex_file_info::Flex_file_info(
    const char *bnm,        // Basename,
    const char *pnm,        // Full pathname,
    Flex *fl,           // Flex file (we'll own it).
    Shape_group_file *g     // Group file (or 0).
) : Shape_file_info(bnm, pnm, g), flex(fl), write_flat(false) {
	entries.resize(flex->number_of_objects());
	lengths.resize(entries.size());
}

/*
 *  Init. for single-palette.
 */

Flex_file_info::Flex_file_info(
    const char *bnm,        // Basename,
    const char *pnm,        // Full pathname,
    unsigned size           // File size.
) : Shape_file_info(bnm, pnm, 0), flex(0), write_flat(true) {
	entries.resize(size > 0);
	lengths.resize(entries.size());
	if (size > 0) {         // Read in whole thing.
		std::ifstream in;
		U7open(in, pnm);    // Shouldn't fail.
		entries[0] = new char[size];
		in.read(entries[0], size);
		lengths[0] = size;
	}
}

/*
 *  Cleanup.
 */

Flex_file_info::~Flex_file_info(
) {
	delete flex;
	int cnt = entries.size();
	for (int i = 0; i < cnt; i++)
		delete entries[i];
}

/*
 *  Get i'th entry.
 */

char *Flex_file_info::get(
    unsigned i,
    size_t &len
) {
	if (i < entries.size()) {
		if (!entries[i]) {  // Read it if necessary.
			entries[i] = flex->retrieve(i, len);
			lengths[i] = len;
		}
		len = lengths[i];
		return entries[i];
	} else
		return 0;
}

/*
 *  Set i'th entry.
 */

void Flex_file_info::set(
    unsigned i,
    char *newentry,         // Allocated data that we'll own.
    int entlen          // Length.
) {
	if (i > entries.size())
		return;
	if (i == entries.size()) {  // Appending?
		entries.push_back(newentry);
		lengths.push_back(entlen);
	} else {
		delete entries[i];
		entries[i] = newentry;
		lengths[i] = entlen;
	}
}

/*
 *  Swap the i'th and i+1'th entries.
 */

void Flex_file_info::swap(
    unsigned i
) {
	assert(i < entries.size() - 1);
	char *tmpent = entries[i];
	int tmplen = lengths[i];
	entries[i] = entries[i + 1];
	lengths[i] = lengths[i + 1];
	entries[i + 1] = tmpent;
	lengths[i + 1] = tmplen;
}

/*
 *  Remove i'th entry.
 */

void Flex_file_info::remove(
    unsigned i
) {
	assert(i < entries.size());
	delete entries[i];      // Free memory.
	entries.erase(entries.begin() + i);
	lengths.erase(lengths.begin() + i);
}

/*
 *  Create a browser for our data.
 */

Object_browser *Flex_file_info::create_browser(
    Shape_file_info *vgafile,   // THE 'shapes.vga' file.
    unsigned char *palbuf,      // Palette for displaying.
    Shape_group *g          // Group, or 0.
) {
	const char *bname = basename.c_str();
	if (strcasecmp(bname, "palettes.flx") == 0 ||
	        strcasecmp(".pal", bname + strlen(bname) - 4) == 0)
		return new Palette_edit(this);
	return new Combo_chooser(vgafile->get_ifile(), this, palbuf,
	                         400, 64, g);
}

/*
 *  Write out if modified.  May throw exception.
 */

void Flex_file_info::flush(
) {
	if (!modified)
		return;
	modified = false;
	int cnt = entries.size();
	size_t len;
	int i;
	for (i = 0; i < cnt; i++) { // Make sure all are read.
		if (!entries[i])
			get(i, len);
	}
	ofstream out;
	string filestr("<PATCH>/"); // Always write to 'patch'.
	filestr += basename;
	U7open(out, filestr.c_str());   // May throw exception.
	if (cnt <= 1 && write_flat) { // Write flat file.
		if (cnt)
			out.write(entries[0], lengths[0]);
		out.close();
		if (!out.good())
			throw file_write_exception(filestr.c_str());
		return;
	}
	Flex_writer writer(out, "Written by ExultStudio", cnt);
	// Write all out.
	for (int i = 0; i < cnt; i++) {
		out.write(entries[i], lengths[i]);
		writer.mark_section_done();
	}
	if (!writer.close())
		throw file_write_exception(filestr.c_str());
}

/*
 *  Revert to what's on disk.
 *
 *  Output: True (meaning we support 'revert').
 */

bool Flex_file_info::revert(
) {
	if (!modified)
		return true;
	modified = false;
	int cnt = entries.size();
	for (int i = 0; i < cnt; i++) {
		delete entries[i];
		entries[i] = 0;
		lengths[i] = 0;
	}
	if (flex) {
		cnt = flex->number_of_objects();
		entries.resize(cnt);
		lengths.resize(entries.size());
	} else {            // Single palette.
		std::ifstream in;
		U7open(in, pathname.c_str());   // Shouldn't fail.
		in.seekg(0, std::ios::end); // Figure size.
		int sz = in.tellg();
		cnt = sz > 0 ? 1 : 0;
		entries.resize(cnt);
		lengths.resize(entries.size());
		in.seekg(0);
		if (cnt) {
			entries[0] = new char[sz];
			in.read(entries[0], sz);
			lengths[0] = sz;
		}
	}
	return true;
}

/*
 *  Delete set's entries.
 */

Shape_file_set::~Shape_file_set(
) {
	for (vector<Shape_file_info *>::iterator it = files.begin();
	        it != files.end(); ++it)
		delete(*it);
}

/*
 *  This routines tries to create files that don't yet exist.
 *
 *  Output: true if successful or if we don't need to create it.
 */

static bool Create_file(
    const char *basename,       // Base file name.
    const string &pathname      // Full name.
) {
	int namelen = strlen(basename);
	if (strcasecmp(".flx", basename + namelen - 4) == 0) {
		// We can create an empty flx.
		ofstream out;
		U7open(out, pathname.c_str());  // May throw exception.
		Flex_writer writer(out, "Written by ExultStudio", 0);
		if (!writer.close())
			throw file_write_exception(pathname.c_str());
		return true;
	} else if (strcasecmp(".pal", basename + namelen - 4) == 0) {
		// Empty 1-palette file.
		ofstream out;
		U7open(out, pathname.c_str());  // May throw exception.
		out.close();        // Empty file.
		return true;
	} else if (strcasecmp("npcs", basename) == 0)
		return true;        // Don't need file.
	return false;           // Might add more later.
}

/*
 *  Create a new 'Shape_file_info', or return existing one.
 *
 *  Output: ->file info, or 0 if error.
 */

Shape_file_info *Shape_file_set::create(
    const char *basename        // Like 'shapes.vga'.
) {
	// Already have it open?
	for (vector<Shape_file_info *>::iterator it = files.begin();
	        it != files.end(); ++it)
		if (strcasecmp((*it)->basename.c_str(), basename) == 0)
			return *it; // Found it.
	// Look in 'static', 'patch'.
	string sstr = string("<STATIC>/") + basename;
	string pstr = string("<PATCH>/") + basename;
	const char *spath = sstr.c_str(), *ppath = pstr.c_str();
	bool sexists = U7exists(spath);
	bool pexists = U7exists(ppath);
	if (!sexists && !pexists)   // Neither place.  Try to create.
		if (!(pexists = Create_file(basename, ppath)))
			return 0;
	// Use patch file if it exists.
	const char *fullname = pexists ? ppath : spath;
	string group_name(basename);    // Create groups file.
	group_name += ".grp";
	Shape_group_file *groups = new Shape_group_file(group_name.c_str());
	if (strcasecmp(basename, "shapes.vga") == 0)
		return append(new Image_file_info(basename, fullname,
		                                  new Shapes_vga_file(spath, U7_SHAPE_SHAPES, ppath),
		                                  groups));
	else if (strcasecmp(basename, "gumps.vga") == 0)
		return append(new Image_file_info(basename, fullname,
		                                  new Vga_file(spath, U7_SHAPE_GUMPS, ppath), groups));
	else if (strcasecmp(basename, "faces.vga") == 0)
		return append(new Image_file_info(basename, fullname,
		                                  new Vga_file(spath, U7_SHAPE_FACES, ppath), groups));
	else if (strcasecmp(basename, "sprites.vga") == 0)
		return append(new Image_file_info(basename, fullname,
		                                  new Vga_file(spath, U7_SHAPE_SPRITES, ppath), groups));
	else if (strcasecmp(basename, "paperdol.vga") == 0)
		return append(new Image_file_info(basename, fullname,
		                                  new Vga_file(spath, U7_SHAPE_PAPERDOL, ppath), groups));
	else if (strcasecmp(basename, "fonts.vga") == 0)
		return append(new Image_file_info(basename, fullname,
		                                  new Vga_file(spath, U7_SHAPE_FONTS, ppath), groups));
	else if (strcasecmp(basename, "u7chunks") == 0) {
		std::ifstream *file = new std::ifstream;
		U7open(*file, fullname);
		return append(new Chunks_file_info(basename, fullname,
		                                   file, groups));
	} else if (strcasecmp(basename, "npcs") == 0)
		return append(new Npcs_file_info(basename, fullname, groups));
	else if (strcasecmp(basename, "combos.flx") == 0 ||
	         strcasecmp(basename, "palettes.flx") == 0)
		return append(new Flex_file_info(basename, fullname,
		                                 new FlexFile(fullname), groups));
	else if (strcasecmp(".pal", basename + strlen(basename) - 4) == 0) {
		// Single palette?
		std::ifstream in;
		U7open(in, fullname);
		in.seekg(0, std::ios::end); // Figure size.
		int sz = in.tellg();
		return append(new Flex_file_info(basename, fullname, sz));
	} else {            // Not handled above?
		// Get image file for this path.
		Vga_file *ifile = new Vga_file(spath, U7_SHAPE_UNK, ppath);
		if (ifile->is_good())
			return append(new Image_file_info(basename, fullname,
			                                  ifile, groups));
		delete ifile;
	}
	cerr << "Error opening image file '" << basename << "'.\n";
	return 0;
}

/*
 *  Locates the NPC browser.
 *
 *  Output: ->file info, or 0 if not found.
 */

Shape_file_info *Shape_file_set::get_npc_browser(
) {
	for (vector<Shape_file_info *>::iterator it = files.begin();
	        it != files.end(); ++it)
		if (strcasecmp((*it)->basename.c_str(), "npcs") == 0)
			return *it; // Found it.
	return 0;   // Doesn't exist yet.
}

/*
 *  Write any modified image files.
 */

void Shape_file_set::flush(
) {
	for (vector<Shape_file_info *>::iterator it = files.begin();
	        it != files.end(); ++it)
		(*it)->flush();
}

/*
 *  Any files modified?
 */

bool Shape_file_set::is_modified(
) {
	for (vector<Shape_file_info *>::iterator it = files.begin();
	        it != files.end(); ++it)
		if ((*it)->modified)
			return true;
	return false;
}
