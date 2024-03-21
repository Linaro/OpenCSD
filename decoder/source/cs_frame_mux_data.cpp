/*
 * \file       cs_frame_mux_data.cpp
 * \brief      OpenCSD : Class to take raw protocol data and mux into 16 byte coresight frames
 *
 * \copyright  Copyright (c) 2024, ARM Limited. All Rights Reserved.
 */

 /*
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *
  * 1. Redistributions of source code must retain the above copyright notice,
  * this list of conditions and the following disclaimer.
  *
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  * this list of conditions and the following disclaimer in the documentation
  * and/or other materials provided with the distribution.
  *
  * 3. Neither the name of the copyright holder nor the names of its contributors
  * may be used to endorse or promote products derived from this software without
  * specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

#include <cstring>

#include "common/cs_frame_mux_data.h"

CSFrameMuxData::CSFrameMuxData()
{
	initMux();
}

CSFrameMuxData::~CSFrameMuxData()
{

}

void CSFrameMuxData::initMux(const size_t frame_capacity /* = 16*/)
{	
	cs_frames.clear();
	for (int i = 0; i < frame_size_bytes; i++)
		curr_frame[i] = 0;
	curr_frame_idx = 0;
	curr_CSID = 0;
	frames_since_id = 0;
	// reserve 16 frames worth of data.
	cs_frames.reserve(frame_capacity * frame_size_bytes);
}

void CSFrameMuxData::copyCurrFrameToStack()
{
	// copy current frame and clear
	for (int i = 0; i < frame_size_bytes; i++)
	{	
		cs_frames.push_back(curr_frame[i]);
		curr_frame[i] = 0;
	}
	curr_frame_idx = 0;
	frames_since_id++;
}

void CSFrameMuxData::setCSIDByte(const int frame_idx, const uint8_t newCSID, const bool nextDataNewID /*= true*/)
{	
	curr_frame[frame_idx] = (newCSID << 1) | 0x1;
	curr_CSID = newCSID;
	frames_since_id = 0;

	if (!nextDataNewID)
	{
		curr_frame[15] |= (uint8_t)0x1 << (frame_idx / 2);
	}
}

// call if padding and incomplete frame. - curr_frame_idx != 0.
void CSFrameMuxData::padCurrFrame()
{
	uint8_t z_bytes[14] = { 0 };
	int bytesToPad;

	// pad scenarios - just ID0 in byte 14
	// 
	if (curr_frame_idx == 14)
	{
		setCSIDByte(curr_frame_idx, 0);
		copyCurrFrameToStack();
	}
	else 
	{ 
		// otherwise ID + n zero bytes.
		bytesToPad = 14 - curr_frame_idx;
		muxInData(z_bytes, bytesToPad, 0, false);
	}
}

int CSFrameMuxData::muxInData(const uint8_t* in_data, const uint32_t in_size, const uint8_t in_CSID, const bool padEnd)
{
	bool newCSID = (bool)(in_CSID != curr_CSID);
	uint32_t bytesProcessed = 0;
	
	while (bytesProcessed < in_size)
	{
		// odd index - Data byte - but handle ID change or flag byte
		if (curr_frame_idx % 2)
		{
			// reached the flag byte - save the current frame and reset the index.
			if (curr_frame_idx == 15)
				copyCurrFrameToStack();
			else
			{
				// data only - unless new ID
				if (newCSID)
				{
					// CSID changed but data in the previous ID byte
					// move to data to this byte and insert the ID in previous byte.
					// flag data as being for previous ID
					curr_frame[curr_frame_idx] = curr_frame[curr_frame_idx - 1];
					setCSIDByte(curr_frame_idx - 1, in_CSID, false);
				}
				else
				{
					// simply add the data byte
					curr_frame[curr_frame_idx] = in_data[bytesProcessed];
					bytesProcessed++;
				}
				curr_frame_idx++;
			}
		}
		// even index - ID / data byte
		else
		{
			// periodically insert a trace ID
			if ((curr_frame_idx == 0) && (frames_since_id >= 15))
				newCSID = true;

			// put ID in the ID/Data byte
			if (newCSID)
			{
				setCSIDByte(curr_frame_idx, in_CSID);
				newCSID = false;
			}
			else
			{
				// put data into the ID/Data byte. Save data bit 0 in flag byte
				curr_frame[curr_frame_idx] = in_data[bytesProcessed] & 0xFE;
				if (in_data[bytesProcessed] & 0x1)
					curr_frame[15] |= (uint8_t)0x1 << (curr_frame_idx / 2);
				bytesProcessed++;
			}
			curr_frame_idx++;
		}
	}

	// exiting with all input data used, but reached flag byte
	// so save the frame here
	if (curr_frame_idx == 15)
		copyCurrFrameToStack();

	// pad if requested and incomplete frame.
	if (padEnd && hasIncompleteFrame())
		padCurrFrame();

	return (int )bytesProcessed;
}


/* extract complete frames into output buffer. Removes these frames from the current frame vector
 *  return bytes written to input buffer.
 */
int CSFrameMuxData::extractFrames(uint8_t* out_frame_buffer, const uint32_t out_size)
{
	int copyframes = out_size / frame_size_bytes;
	int copybytes; 

	// calculate number of frames that may be copied
	if (copyframes > numFrames())
		copyframes = numFrames();

	copybytes = copyframes * frame_size_bytes;

	if (copybytes)
	{
		// copy out the frames from the first to buffer size / end of frames - whichever larger
		memcpy(out_frame_buffer, cs_frames.data(), copybytes);

		// remove the copied frames from the buffer.
		clearFrames(copyframes);
	}

	// return the number of bytes copied.
	return copybytes;
}

// remove first nFrames from the buffer after extraction
void CSFrameMuxData::clearFrames(int nFrames)
{
	if (nFrames > numFrames())
		nFrames = numFrames();
	cs_frames.erase(cs_frames.begin(), cs_frames.begin() + (nFrames * frame_size_bytes));
}
