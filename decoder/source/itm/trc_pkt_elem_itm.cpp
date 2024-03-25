/*
 * \file       trc_pkt_elem_itm.cpp
 * \brief      OpenCSD : STM decode - packet class
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

#include <sstream>
#include <iomanip>
#include "opencsd/itm/trc_pkt_elem_itm.h"

ItmTrcPacket::ItmTrcPacket()
{
    initPacket();
}

ItmTrcPacket& ItmTrcPacket::operator =(const ocsd_itm_pkt* p_pkt)
{
    *dynamic_cast<ocsd_itm_pkt*>(this) = *p_pkt;
    return *this;
}

void ItmTrcPacket::initPacket()
{
	type = ITM_PKT_RESERVED;
	src_id = 0;
	value = 0;
	val_sz = 0;
	val_ext = 0;
	err_type = ITM_PKT_NO_ERR_TYPE;
}

void ItmTrcPacket::setValue(const uint32_t pkt_val, const uint8_t val_size_bytes)
{
	static uint32_t masks[] = {
		0xFF,
		0xFFFF,
		0xFFFFFF,
		0xFFFFFFFF,
	};

	val_sz = val_size_bytes;
	if (val_sz < 1 || val_sz > 4)
		val_sz = 4;
	value = pkt_val & masks[val_sz - 1];

}

const uint64_t ItmTrcPacket::getExtValue() const
{
	uint64_t extval = 0;

	if (val_sz < 5)
		return (uint64_t)getValue();
	extval = (uint64_t)value | (((uint64_t)val_ext) << 32);
	return extval;
}

void ItmTrcPacket::setExtValue(const uint64_t pkt_ext_val)
{
	value = (uint32_t)(pkt_ext_val & 0xFFFFFFFF);
	val_ext = (uint8_t)((pkt_ext_val >> 32) & 0xFF);
	val_sz = 5;
}

void ItmTrcPacket::pktTypeName(const ocsd_itm_pkt_type pkt_type, std::string& name, std::string& desc) const
{
	std::ostringstream oss_name;
	std::ostringstream oss_desc;
	
	switch (pkt_type)
	{
	/* markers for unknown packets / state */
	case ITM_PKT_NOTSYNC:
		oss_name << "ITM_NOTSYNC";
		oss_desc << "ITM data stream not synchronised";
		break;

	case ITM_PKT_INCOMPLETE_EOT:     /**< Incomplete packet flushed at end of trace. */
		oss_name << "ITM_INCOMPLETE_EOT";
		oss_desc << "Incomplete packet flushed at end of trace";
		break;

/* valid packet types */
	case ITM_PKT_ASYNC:				/**< sync packet */
		oss_name << "ITM_ASYNC";
		oss_desc << "Alignment synchronisation packet";
		break;

	case ITM_PKT_OVERFLOW:			/**< overflow packet */
		oss_name << "ITM_OVERFLOW";
		oss_desc << "ITM overflow packet";
		break;

	case ITM_PKT_SWIT:				/**< Software stimulus packet */
		oss_name << "ITM_SWIT";
		oss_desc << "Software Stimulus write packet";
		break;

	case ITM_PKT_DWT:				/**< DWT hardware stimulus packet */
		oss_name << "ITM_DWT";
		oss_desc << "DWT hardware stimulus write";
		break;

	case ITM_PKT_TS_LOCAL:			/**< Timestamp packet using local timestamp source */
		oss_name << "ITM_TS_LOCAL";
		oss_desc << "Local Timestamp";
		break;

	case ITM_PKT_TS_GLOBAL_1:		/**< Timestamp packet bits [25:0] from the global timestamp source */
		oss_name << "ITM_GTS_1";
		oss_desc << "Global Timestamp [25:0]";
		break;

	case ITM_PKT_TS_GLOBAL_2:		/**< Timestamp packet bits [63:26] or [42:26] from the global timestamp source */
		oss_name << "ITM_GTS_2";
		oss_desc << "Global Timestamp [{63|42}:26]";
		break;

	case ITM_PKT_EXTENSION:			/**< Extension packet */
		oss_name << "ITM_EXTENSION";
		oss_desc << "Extension packet";
		break;

/* packet errors */
	case ITM_PKT_BAD_SEQUENCE:
		oss_name << "ITM_BAD_SEQUENCE";
		oss_desc << "Invalid sequence in packet";
		break;

	case ITM_PKT_RESERVED:
		oss_name << "ITM_RESERVED";
		oss_desc << "Reserved Packet Header";
		break;

	default:
		oss_name << "ITM_UNKNOWN";
		oss_desc << "ERROR: unknown packet type";
		break;
	}

	desc = oss_desc.str();
	name = oss_name.str();
}

void ItmTrcPacket::printValSize(std::string& valStr) const
{
	std::ostringstream oss;
	if (val_sz <= 4)
		oss << "0x" << std::hex << std::setw(val_sz * 2) << std::setfill('0') << value;
	else
	{
		oss << "0x" << std::hex << std::setw(2) << std::setfill('0') << (uint16_t)val_ext;
		oss << std::hex << std::setw(8) << std::setfill('0') << value;
	}
	valStr = oss.str();
}

void ItmTrcPacket::printDWTPacket(std::string& valStr) const
{
	std::ostringstream oss;
	std::string val;

	const char* excepFn[] = {
		"reserved",
		"entered",
		"exited",
		"returned to",
	};

	if (src_id == 0) {
		oss << "[Event Counter: 0x" << std::hex << std::setw(2) << std::setfill('0') << value << "; Flags: ";
		if (value & ITM_DWT_ECNTR_CPI)
			oss << " CPI ";
		else
			oss << " --- ";
		if (value & ITM_DWT_ECNTR_EXC)
			oss << " EXC ";
		else
			oss << " --- ";
		if (value & ITM_DWT_ECNTR_SLP)
			oss << " Sleep ";
		else
			oss << " --- ";
		if (value & ITM_DWT_ECNTR_LSU)
			oss << " LSU ";
		else
			oss << " --- ";
		if (value & ITM_DWT_ECNTR_FLD)
			oss << " Fold ";
		else
			oss << " --- ";
		if (value & ITM_DWT_ECNTR_CYC)
			oss << " CYC ";
		else
			oss << " --- ";
		oss << "] ";
	}
	else if (src_id == 1) 
	{
		oss << "[Exception Num:  0x" << std::hex << std::setw(4) << std::setfill('0') << (uint16_t)(value & 0x1FF);
		oss << "(" << excepFn[((value >> 12) & 0x3)] << ") ]";
	}
	else if (src_id == 2) 
	{
		printValSize(val);
		oss << "[PC Sample: " << val << "] ";
	}
	else if (src_id >= 8 && src_id <= 23)
	{
		uint8_t dt_type = (src_id >> 3) & 0x3;
		uint8_t dt_rw = src_id & 0x01;
		uint16_t dt_comp = (uint16_t)(src_id >> 1 & 0x3);

		printValSize(val);

		/* PC value packet */
		if ((dt_type == 0x1) && !dt_rw)
			oss << "[Data Trc: comp=" << dt_comp << "; PC Value=" << val << " ] ";
		else if ((dt_type == 0x1) && dt_rw)
			oss << "[Data Trc: comp=" << dt_comp << "; Address=" << val << " ] ";
		else if (dt_type == 0x2)
			oss << "[Data Trc: comp=" << dt_comp << "; Val " << (dt_rw ? "write: " : "read: ") << val << "] ";
	}
	else
		oss << "[Reserved discriminator value] ";

	valStr = oss.str();
}

void ItmTrcPacket::printTSLocalPacket(std::string& valStr) const
{
	std::ostringstream oss;
	std::string val;

	const char* ts_tc_type[] = {
		"TS Sync",
		"TS Delayed",
		"TS Sync, Packet Delayed",
		"TS Delayed, Packet Delayed",
	};

	printValSize(val);
	oss << val << " { " << ts_tc_type[(src_id & 3)] << " }";
	valStr = oss.str();
}

void ItmTrcPacket::toString(std::string& str) const
{
	std::string name, desc, desc_err, val;
	std::ostringstream oss;
	int bitsize = 0;
	int gts1_bitSize[] = { 6, 13, 20, 25 };

	pktTypeName(type, name, desc);
	str = name + ": ";

	switch (type)
	{
		/* print addtional packet data */
	case ITM_PKT_SWIT:
		printValSize(val);
		oss << "{src id: 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint16_t)src_id << "} ";
		str += oss.str() + " " + val;
		break;
		
	case ITM_PKT_DWT:
		oss << "{desc: 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint16_t)src_id << "} ";
		printDWTPacket(val);
		str += oss.str() + val;
		break; 

	case ITM_PKT_TS_LOCAL:
		printTSLocalPacket(val);
		str += val;
		break;

	case ITM_PKT_TS_GLOBAL_1:
		oss << "{ TS bits [" << gts1_bitSize[val_sz-1] << ":0]";
		if (src_id & 0x1)
			oss << ", Clk change";
		if (src_id & 0x2)
			oss << ", Clk wrap";
		oss << "} ";
		printValSize(val);
		str += oss.str() + val;
		break;

	case ITM_PKT_TS_GLOBAL_2:
		if (val_sz == 5)
			oss << "{ TS bits [63:26]} ";
		else
			oss << "{ TS bits [47:26]} ";
		printValSize(val);
		str += oss.str() + val;
		break;

	case ITM_PKT_EXTENSION:
		bitsize = (int)(src_id & 0x1F) + 1;
		/* 
		 * srd_id contains source bit + N bitsize of value N:0 (N=2,9,16,23,31)
		 */
		if ((bitsize == 3) && ((src_id & 0x80) == 0))
		{
			/* stimulus port page number */
			oss << "{stim port page} 0x" << std::hex << std::setw(2) << std::setfill('0') << value;
		}
		else
		{
			oss << "{unknown extension type, " << bitsize << " bits } 0x" << std::hex << std::setw(((bitsize / 4) + 1)) << std::setfill('0') << value;
		}
		str += oss.str();
		break;


	case ITM_PKT_INCOMPLETE_EOT:
	case ITM_PKT_BAD_SEQUENCE:
		/* print the original type of these packets */
		pktTypeName(err_type, name, desc_err);
		oss << "[Init type: " << name << "] ";
		str += oss.str();
		break;

		/* no additional info */
	case ITM_PKT_ASYNC:
	case ITM_PKT_OVERFLOW:	
	case ITM_PKT_RESERVED:
	case ITM_PKT_NOTSYNC:
	default:
		break;
	}

	str += "; '" + desc + "'";
}

void ItmTrcPacket::toStringFmt(const uint32_t fmtFlags, std::string& str) const
{
	toString(str);
}
