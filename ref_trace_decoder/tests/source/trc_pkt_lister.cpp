/*
 * \file       trc_pkt_lister.cpp
 * \brief      Reference CoreSight Trace Decoder : Trace Packet Lister Test program
 * 
 * \copyright  Copyright (c) 2015, ARM Limited. All Rights Reserved.
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

/* Test program / utility - list trace packets in supplied snapshot. */

#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>

#include "rctdl.h"
#include "trace_snapshots.h"
#include "pkt_printer_t.h"

static bool process_cmd_line_opts(rctdlMsgLogger &logger, int argc, char* argv[]);
static void ListTracePackets(rctdlMsgLogger &logger, rctdlDefaultErrorLogger &err_logger, SnapShotReader &reader, const std::string &trace_buffer_name);

    // default path
#ifdef WIN32
static std::string ss_path = ".\\";
#else
static std::string ss_path = "./";
#endif

static std::string source_buffer_name = "";    // source name - used if more than one source
static bool all_source_ids = true;      // output all IDs in source.
static std::vector<uint8_t> id_list;    // output specific IDs in source

static int logOpts = rctdlMsgLogger::OUT_STDOUT | rctdlMsgLogger::OUT_FILE;
static std::string logfileName = "trc_pkt_lister.log";

int main(int argc, char* argv[])
{
    // TBD: get the logger cmd line options here.

    rctdlMsgLogger logger;
    logger.setLogOpts(logOpts);
    logger.setLogFileName(logfileName.c_str());

    rctdlDefaultErrorLogger err_log;
    err_log.initErrorLogger(RCTDL_ERR_SEV_INFO,&logger);

    if(!process_cmd_line_opts(logger, argc, argv))
        return -1;

    SnapShotReader ss_reader;
    ss_reader.setSnapshotDir(ss_path);
    ss_reader.setErrorLogger(&err_log);


    if(ss_reader.snapshotFound())
    {
        if(ss_reader.readSnapShot())
        {
            std::vector<std::string> sourceBuffList;
            if(ss_reader.getSourceBufferNameList(sourceBuffList))
            {
                if(source_buffer_name.size() == 0)
                {
                    if(sourceBuffList.size() > 1)
                    {
                        // TBD : choose buffer - for now use first found
                        source_buffer_name = sourceBuffList[0];

                    }
                    else 
                        source_buffer_name = sourceBuffList[0];
                }

                std::ostringstream oss;
                oss << "Using " << source_buffer_name << " as trace source\n";
                logger.LogMsg(oss.str());
                ListTracePackets(logger, err_log,ss_reader,source_buffer_name);

            }
            else
                logger.LogMsg("Trace Packet Lister : No trace source buffer names found\n");
        }
        else
            logger.LogMsg("Trace Packet Lister : Failed to read snapshot\n");
    }
    else
    {
        std::ostringstream oss;
        oss << "Trace Packet Lister : Snapshot path" << ss_path << "not found\n";
        logger.LogMsg(oss.str());
    }

    
    return 0;
}


bool process_cmd_line_opts(rctdlMsgLogger &logger, int argc, char* argv[])
{
    bool bOptsOK = true;
    if(argc > 1)
    {
        int options_to_process = argc - 1;
        int optIdx = 1;
        while((options_to_process > 0) && bOptsOK)
        {
            if(strcmp(argv[optIdx], "-ss_dir") == 0)
            {
                options_to_process--;
                optIdx++;
                if(options_to_process)
                    ss_path = argv[optIdx];
                else
                {
                    logger.LogMsg("Error: Missing directory string on -ss_dir option\n");
                    bOptsOK = false;
                }
            }
            else if(strcmp(argv[optIdx], "-id") == 0)
            {
                options_to_process--;
                optIdx++;
                if(options_to_process)
                {
                    uint8_t Id = (uint8_t)strtoul(argv[optIdx],0,0);
                    if((Id == 0) || (Id >=70)) 
                    {                        
                        std::ostringstream iderrstr;
                        iderrstr << "Error: invalid ID number " << Id << " on -id option" << std::endl;
                        logger.LogMsg(iderrstr.str());
                        bOptsOK = false;
                    }
                    else
                    {
                        all_source_ids = false;
                        id_list.push_back(Id);
                    }                   
                }
                else
                {
                    logger.LogMsg("Error: No ID number on -id option\n");
                    bOptsOK = false;
                }
            }
            else if(strcmp(argv[optIdx], "-src_name") == 0)
            {
                options_to_process--;
                optIdx++;
                if(options_to_process)
                    source_buffer_name = argv[optIdx];
                else
                {
                    logger.LogMsg("Error: Missing source name string on -src_name option\n");
                    bOptsOK = false;
                }
            }
            else
            {
                std::ostringstream errstr;
                errstr << "Warning: Ignored unknown option " << argv[optIdx] << "." << std::endl;
                logger.LogMsg(errstr.str());
            }
            options_to_process--;
            optIdx++;
        }
        
    }
    return bOptsOK;
}

void ListTracePackets(rctdlMsgLogger &logger, rctdlDefaultErrorLogger &err_logger, SnapShotReader &reader, const std::string &trace_buffer_name)
{
    CreateDcdTreeFromSnapShot tree_creator;
    tree_creator.initialise(&reader,&err_logger);
    if(tree_creator.createDecodeTree(trace_buffer_name, true))
    {
        std::vector<ItemPrinter *> printers;
        DecodeTreeElement *pElement;
        DecodeTree *dcd_tree = tree_creator.getDecodeTree();
        uint8_t elemID;
        
        // attach packet printers to each trace source in the tree
        pElement = dcd_tree->getFirstElement(elemID);
        while(pElement)
        {
            switch(pElement->getProtocol())
            {
            case RCTDL_PROTOCOL_ETMV4I:
                PacketPrinter<EtmV4ITrcPacket> *pPrinter = new (std::nothrow) PacketPrinter<EtmV4ITrcPacket>(elemID,&logger);
                if(pPrinter)
                {
                    pElement->getEtmV4IPktProc()->getPacketOutAttachPt()->attach(pPrinter);
                    printers.push_back(pPrinter); // save printer to destroy it later
                }
                break;

                // TBD : handle other protocol types.
            }
            pElement = dcd_tree->getNextElement(elemID);
        }

        // check if we have attached at least one printer
        if(printers.size() > 0)
        {
            // need to push the data through the decode tree.
            std::ifstream in;
            in.open(tree_creator.getBufferFileName(),std::ifstream::in | std::ifstream::binary);
            if(in.is_open())
            {
                rctdl_datapath_resp_t dataPathResp = RCTDL_RESP_CONT;
                static const int bufferSize = 1024;
                uint8_t trace_buffer[bufferSize];   // temporary buffer to load blocks of data from the file
                uint32_t trace_index = 0;           // index into the overall trace buffer (file).

                // process the file, a buffer load at a time
                while(!in.eof() && !RCTDL_DATA_RESP_IS_FATAL(dataPathResp))
                {
                    in.read((char *)&trace_buffer[0],bufferSize);   // load a block of data into the buffer

                    int nBuffRead = in.gcount();    // get count of data loaded.
                    int nBuffProcessed = 0;         // amount processed in this buffer.
                    uint32_t nUsedThisTime = 0;

                    // process the current buffer load until buffer done, or fatal error occurs
                    while((nBuffProcessed < nBuffRead) && !RCTDL_DATA_RESP_IS_FATAL(dataPathResp))
                    {
                        if(RCTDL_DATA_RESP_IS_CONT(dataPathResp))
                        {
                            dataPathResp = dcd_tree->TraceDataIn(
                                RCTDL_OP_DATA,
                                trace_index,
                                nBuffRead - nBuffProcessed,
                                &(trace_buffer[0])+nBuffProcessed,
                                &nUsedThisTime);

                            nBuffProcessed += nUsedThisTime;
                            trace_index += nUsedThisTime;
                        }
                        else
                        {
                            dataPathResp = dcd_tree->TraceDataIn(RCTDL_OP_FLUSH,0,0,0,0);
                        }
                    }
                }
                // mark end of trace into the data path
                dcd_tree->TraceDataIn(RCTDL_OP_EOT,0,0,0,0);
                // close the input file.
                in.close();
            }
            else
            {
            }

        }

        // clean up

        // get rid of the decode tree.
        tree_creator.destroyDecodeTree();

        // get rid of all the printers.
        std::vector<ItemPrinter *>::iterator it;
        it = printers.begin();
        while(it != printers.end())
        {
            delete *it;
            it++;
        }
    }
}

/* End of File trc_pkt_lister.cpp */
