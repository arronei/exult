/*
Copyright (C) 2000  Ryan Nunn

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

// Tab Size = 4

#ifndef __XMIDI_h_
#define __XMIDI_h_

#if !AUTOCONFIGURED
#include "../autoconfig.h"
#endif

#include <string>
#include "common_types.h"
#include "databuf.h"

// Conversion types for Midi files
#define XMIDI_CONVERT_NOCOVERSION	0
#define XMIDI_CONVERT_MT32_TO_GM	1
#define XMIDI_CONVERT_MT32_TO_GS	2
#define XMIDI_CONVERT_GSMT_TO_GS	0

struct midi_event
{
	int		time;
	unsigned char	status;

	unsigned char	data[2];

	uint32	len;
	unsigned char	*buffer;

	midi_event	*next;
};

class   XMIDI
{
public:
	struct  midi_descriptor
	{
		uint16	type;
		uint16	tracks;
	};

protected:
	midi_descriptor	info;

private:
	midi_event		**events;
	signed short	*timing;

	midi_event		*list;
	midi_event		*current;
	
	const static char	mt32asgm[128];
	const static char	mt32asgs[256];
	int				convert_from_mt32;
	bool			*fixed;

public:
	XMIDI(DataSource *source, int pconvert);
	~XMIDI();

	int number_of_tracks()
	{
		if (info.type != 1)
			return info.tracks;
		else
			return 1;
	};

	// Retrieve it to a data source
	int retrieve (uint32 track, DataSource *dest);
	
	// External Event list functions
	int retrieve (uint32 track, midi_event **dest, int &ppqn);
	static void DeleteEventList (midi_event *mlist);

	// Not yet implimented
	// int apply_patch (int track, DataSource *source);

private:
	XMIDI(); // No default constructor
        
	// List manipulation
	void CreateNewEvent (int time);

	// Variable length quantity
	int GetVLQ (DataSource *source, uint32 &quant);
	int GetVLQ2 (DataSource *source, uint32 &quant);
	int PutVLQ(DataSource *dest, uint32 value);

	void MovePatchVolAndPan (int channel = -1);
	void DuplicateAndMerge (int num = 0);

	int ConvertEvent (const int time, const unsigned char status, DataSource *source, const int size);
	int ConvertSystemMessage (const int time, const unsigned char status, DataSource *source);

	int ConvertFiletoList (DataSource *source, bool is_xmi);
	uint32 ConvertListToMTrk (DataSource *dest, midi_event *mlist);

	int ExtractTracksFromXmi (DataSource *source);
	int ExtractTracksFromMid (DataSource *source);
	
	int ExtractTracks (DataSource *source);
};

#endif
