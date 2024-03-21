/*!
 * \file       cs_frame_mux_data.h
 * \brief      OpenCSD : Creates CSID muxed data frames for test data.
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

#ifndef ARM_CS_FRAME_MUX_DATA_H_INCLUDED
#define ARM_CS_FRAME_MUX_DATA_H_INCLUDED


#include <cstdio>
#include <inttypes.h>
#include <vector>

class CSFrameMuxData
{
public:
	CSFrameMuxData();
	~CSFrameMuxData();


	void initMux(const size_t frame_capacity = 16);	// initialise the muxer state with initial capacity for frames

	/** creates CS data frames from input data associated with the ID.
	 *
	 * @in_data : current block of input data.
	 * @in_size : size of input data.
	 * @in_CSID : CS ID associated with this data.
	 * @pad_end : pad last frame at end of data (otherwise wait for more input)
	 *
	 * calling with in_size == 0, and padEnd == true will cause any incomplete frame to be padded.
	 *
	 * return : bytes used
	 */
	int muxInData(const uint8_t* in_data, const uint32_t in_size, const uint8_t in_CSID, const bool padEnd);

	/* extract complete frames into output buffer. Removes these frames from the current frame vector
	 *  return bytes written to input buffer.
	 */
	int extractFrames(uint8_t* out_frame_buffer, const uint32_t out_size);

	/* true if an incomplete frame exists */
	const bool hasIncompleteFrame() const { return (bool)(curr_frame_idx != 0); };

	/* number of complete frames in the buffer */
	const int numFrames() const { return (cs_frames.size() / frame_size_bytes); };

	/* direct access to frame buffer and size */
	const uint8_t* getFrameBuffer() { return cs_frames.data(); };
	size_t getFrameBufferSize() { return cs_frames.size(); };
	void clearFrames(int nFrames); // remove copied frames


private:
	void copyCurrFrameToStack(); // copy the completed current frame to the frame stack
	void setCSIDByte(const int frame_idx, const uint8_t newCSID, const bool nextDataNewID = true);
	void padCurrFrame();

	static const int frame_size_bytes = 16;

	std::vector<uint8_t> cs_frames;	// completed frame data
	int frames_since_id;	// coutn of frames since last ID
	uint8_t curr_frame[frame_size_bytes];	// the current in progress frame.
	int curr_frame_idx;		// index into current frame for the next data/id value
	uint8_t curr_CSID;		// current CSID for the muxer
};

#endif // ARM_CS_FRAME_MUX_DATA_H_INCLUDED
