/*
 * \file       itm_decode_test.cpp
 * \brief      OpenCSD : Tests for ITM decoder
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

/* test program - runs on internal sets of test data. */

#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>

#include "opencsd.h"              // the library
#include "itm_test.h"			  // header to access test data
#include "common/cs_frame_mux_data.h"

static ocsdMsgLogger logger;
static int logOpts = ocsdMsgLogger::OUT_STDOUT | ocsdMsgLogger::OUT_FILE;
static std::string logfileName = "itm_decode_test.ppl";
static ITMConfig config;
static ocsdDefaultErrorLogger errLogger;

static IDecoderMngr* pDecoderMngr = 0;              // decoder manager - handle creation and destruction of ITM decoders
static ITrcDataIn* pDecoderIn = 0;                  // data in interface
static TraceComponent* pItmTraceComp = 0;           // itm decoder
static std::vector<ItemPrinter*> printer_list;      // list of object printers - for printer factory to descroy later
static std::string testName = "all";

static std::string dumpFileName = "itm_testdata";
static std::string dumpFileExt = ".bin";
static std::string inFileName = "";
static bool bProcessInFile = false;
static uint8_t muxCSID = 0;  // CSID to read / dump with coresight format
static bool bDoFileDump = false;
static bool muteIDPrint = false;  // test option : remove ID / Idx from print to allow compare of idfferent formats

static TraceFormatterFrameDecoder cs_frame_demux;

bool process_cmd_line_logger_opts(int argc, char* argv[])
{
    bool badLoggerOpts = false;
    bool bChangingOptFlags = false;
    int newlogOpts = ocsdMsgLogger::OUT_NONE;
    std::string opt;
    if (argc > 1)
    {
        int options_to_process = argc - 1;
        int optIdx = 1;
        while (options_to_process > 0)
        {
            opt = argv[optIdx];
            if (opt == "-logstdout")
            {
                newlogOpts |= ocsdMsgLogger::OUT_STDOUT;
                bChangingOptFlags = true;
            }
            else if (opt == "-logstderr")
            {
                newlogOpts |= ocsdMsgLogger::OUT_STDERR;
                bChangingOptFlags = true;
            }
            else if (opt == "-logfile")
            {
                newlogOpts |= ocsdMsgLogger::OUT_FILE;
                bChangingOptFlags = true;
            }
            else if (opt == "-logfilename")
            {
                options_to_process--;
                optIdx++;
                if (options_to_process)
                {
                    logfileName = argv[optIdx];
                    newlogOpts |= ocsdMsgLogger::OUT_FILE;
                    bChangingOptFlags = true;
                }
                else
                {
                    // no file name following -infile
                    std::ostringstream errstr;
                    errstr << "Missing <filename> following -logfilename option\n";
                    logger.LogMsg(errstr.str());
                    badLoggerOpts = true;
                }
            }
            options_to_process--;
            optIdx++;
        }
    }
    if (bChangingOptFlags)
        logOpts = newlogOpts;
    return badLoggerOpts;
}

void print_help()
{
    std::ostringstream oss;
    oss << "ITM decode test - commands\n\n";
    oss << "\nRunning built in tests:\n\n";
    oss << "-test <name>        Run test named \"name\", run all tests otherwise\n";
    oss << "-list               List available tests then exit\n";
    oss << "\nUsing an input file for decode\n";
    oss << "-infile <filename>  use <filename> as binary input of raw ITM data, instead of built in tests\n";
    oss << "-csid <id>          set CSID to use for input file. Implies input file is CoreSight frame formatted\n";
    oss << "\nOutput:\n";
    oss << "   Setting any of these options cancels the default output to file & stdout,\n   using _only_ the options supplied.\n\n";
    oss << "-logstdout          Output to stdout -> console.\n";
    oss << "-logstderr          Output to stderr.\n";
    oss << "-logfile            Output to default file - " << logfileName << "\n";
    oss << "-logfilename <name> Output to file <name> \n";

    // undocumented dev & test only options
    // -fdump - dump all tests as a single buffer in a binary file. use -csid to CS frame format 
    // -muteid - prevent the printing of Idx:<N> ID:<M> in the files to better compare outputs between raw and CS files.


    logger.LogMsg(oss.str());
}

void dump_test_data()
{
    itm_test_data_t* tests = get_item_tests();
    itm_test_data_t* curr_test = 0;
    std::ostringstream oss;
    int test_idx = 0;
    int bytes = 0;
    std::ofstream outfile;
    CSFrameMuxData frameMux;

    curr_test = &tests[test_idx];
    oss << "Dumping test data to binary file";
    if (muxCSID)
    {
        oss << " using 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint16_t)muxCSID << " as trace ID in CoreSight formatted frames\n\n";
    }
    else
    {
        oss << " as raw ITM data with no CoreSight frame formatting\n\n";
    }
    logger.LogMsg(oss.str());

    // create the dump file name - add CS ID value if used.
    oss.str("");
    oss << dumpFileName;
    if (muxCSID)
        oss << "_ID_0x" << std::hex << std::setw(2) << std::setfill('0') << (uint16_t)muxCSID;
    oss << dumpFileExt;
    dumpFileName = oss.str();

    outfile.open(dumpFileName, std::ofstream::out | std::ofstream::binary);
    if (!outfile.is_open()) {
        oss.str("");
        oss << "Error: failed to open file " << dumpFileName << "\n";
        logger.LogMsg(oss.str());
    }

    while (curr_test->test_name != 0)
    {
        oss.str("Writing test ");
        oss << curr_test->test_name << ", " << std::dec << curr_test->test_data_size << " bytes\n";
        logger.LogMsg(oss.str());

        if (muxCSID)
            frameMux.muxInData(curr_test->test_packets, curr_test->test_data_size, muxCSID, false);
        else
            outfile.write((const char *)curr_test->test_packets, curr_test->test_data_size);

        bytes += curr_test->test_data_size;

        curr_test = &tests[++test_idx];
    }

    if (muxCSID)
    {
        // write framed data to the file - once padded.
        if (frameMux.hasIncompleteFrame())
            frameMux.muxInData(0, 0, 0, true);

        // write frame buffer direct to file.
        outfile.write((const char *)frameMux.getFrameBuffer(), frameMux.getFrameBufferSize());

        // clear the frames we copied directly
        frameMux.clearFrames(frameMux.getFrameBufferSize() / 16);
    }

    outfile.close();

    oss.str("");
    oss << "File dump complete - ";
    oss << bytes << " total protocol bytes\n";
    logger.LogMsg(oss.str());
}

void list_tests()
{
    itm_test_data_t* tests = get_item_tests();
    itm_test_data_t* curr_test = 0;
    std::ostringstream oss;
    int test_idx = 0;

    curr_test = &tests[test_idx];
    oss << "List of built in tests to run:-\n\n";

    while (curr_test->test_name != 0)
    {
        oss << "Name: " << curr_test->test_name << "\nDescription: " << curr_test->test_description << "\n\n";
        curr_test = &tests[++test_idx];
    }

    logger.LogMsg(oss.str());
}

bool process_cmd_line_opts(int argc, char* argv[], bool &optExit)
{
    bool bOptsOK = true;
    std::string opt;
    std::ostringstream errstr;

    optExit = false;

    if (argc > 1)
    {

        int options_to_process = argc - 1;
        int optIdx = 1;
        while ((options_to_process > 0) && bOptsOK)
        {
            opt = argv[optIdx];
            if ((opt == "-logstdout") || (opt == "-logstderr") ||
                (opt == "-logfile") || (opt == "-logfilename"))
            {
                // skip all these as processed earlier

                // also additionally skip any filename parameter
                if (opt == "-logfilename")
                {
                    options_to_process--;
                    optIdx++;
                }
            }
            else if (opt == "-test")
            {
                options_to_process--;
                optIdx++;
                if (options_to_process)
                {
                    testName = argv[optIdx];
                }
                else
                {
                    // no test name following test
                    bOptsOK = false;
                }
            }
            else if ((opt == "-help") || (opt == "--help") || (opt == "-h"))
            {
                print_help();
                optExit = true;
                bOptsOK = false;
            }
            else if (opt == "-list")
            {
                list_tests();
                optExit = true;
                bOptsOK = false;
            }
            else if (opt == "-fdump")
            {
                bDoFileDump = true;
            }
            else if (opt == "-csid")
            {
                options_to_process--;
                optIdx++;
                if (options_to_process)
                {
                    muxCSID = ((uint8_t)strtoul(argv[optIdx], 0, 0)) & 0x7f;
                }
                else
                {
                    bOptsOK = false;
                    errstr << "Missing CSID following -csid option\n";
                    logger.LogMsg(errstr.str());
                }
            }
            else if (opt == "-muteid")
            {
                muteIDPrint = true;
            }
            else if (opt == "-infile")
            {
                options_to_process--;
                optIdx++;
                if (options_to_process)
                {
                    inFileName = argv[optIdx];
                    bProcessInFile = true;
                }
                else
                {
                    // no file name following -infile
                    bOptsOK = false;
                    errstr << "Missing <filename> following -infile option\n";
                    logger.LogMsg(errstr.str());
                }
            }
            else
            {
                errstr << "ITM Decode test : Warning: Ignored unknown option " << argv[optIdx] << "." << std::endl;
                logger.LogMsg(errstr.str());
            }
            options_to_process--;
            optIdx++;
        }
    }

    if (bDoFileDump && bOptsOK)
    {
        dump_test_data();
        optExit = true;
        bOptsOK = false;
    }
    return bOptsOK;
}

void setup_config()
{
    ocsd_itm_cfg cfg;

    // zero out control register
    cfg.reg_tcr = 0;       

    // add in any CS ID value.
    cfg.reg_tcr |= ((uint32_t)(muxCSID & 0x7f)) << 16;
    
    config = &cfg;
}

void destroy_itm_objects()
{
    if (pDecoderMngr && pItmTraceComp)
    {
        pDecoderMngr->destroyDecoder(pItmTraceComp);
        pItmTraceComp = 0;
    }

    PktPrinterFact::destroyAllPrinters(printer_list);
    pDecoderMngr = 0;
}

/* not using the normal decode tree - create and connect all elements for ITM decode 
We get the decoder manager for ITM protocol  from the list of registered decoders in the 
library and use this to create the decode path.

Data flow:

  [ binary data ]  /  [input file]
        |
        V
  {{ CS frame deformatter }} := optional iff file input and csid set
        |
        V
  { ITrcDataIn }
        |
        V
  { TrcPktProcItm } -> { IPktRawDataMon } -> [ Itm packet printer using ItmTrcPacket ]
        |
        V
  { TrcPktDecodeItm }
        |
        V
  [ TrcGenericElementPrinter for OcsdTraceElement :  Packet printer for decoder generic elements ]

*/
ocsd_err_t setup_decoder()
{
    int crtFlags = OCSD_CREATE_FLG_FULL_DECODER;
    ocsd_err_t err = OCSD_OK;
    uint8_t CSID;
    ItemPrinter* pItemPrinter = 0;

    // grab the list of decoders in the library 
    OcsdLibDcdRegister* lib_reg = OcsdLibDcdRegister::getDecoderRegister();
    if (lib_reg == 0)
        return OCSD_ERR_NOT_INIT;

    // find the manager for the named ITM decoder
    if ((err = lib_reg->getDecoderMngrByName(OCSD_BUILTIN_DCD_ITM, &pDecoderMngr)) != OCSD_OK)
        return err;

    // create the decoder - creates both TrcPktProcItm and TrcPktDecodeItm and attaches them together
    setup_config();
    CSID = config.getTraceID();
    if ((err = pDecoderMngr->createDecoder(crtFlags, (int)CSID, &config, &pItmTraceComp)) != OCSD_OK)
        return err;

    // add an error logger
    if (err == OCSD_OK)
        err = pDecoderMngr->attachErrorLogger(pItmTraceComp, &errLogger);
    
    // get a pointer to the input interface of the decoder
    if (err == OCSD_OK)
        err = pDecoderMngr->getDataInputI(pItmTraceComp, &pDecoderIn);

    // if we are inputting a file and it has muxCSID - create the links to the demux here
    if (bProcessInFile && muxCSID && (err == OCSD_OK))
    {
        // we assume no syncs for these tests
        err = cs_frame_demux.Init();
        if (err == OCSD_OK)
            err = cs_frame_demux.Configure(OCSD_DFRMTR_FRAME_MEM_ALIGN);
        if (err == OCSD_OK)
            err = cs_frame_demux.getErrLogAttachPt()->attach(&errLogger);

        // attach decoder to CSID input of demux
        if (err == OCSD_OK)
            err = cs_frame_demux.getIDStreamAttachPt(CSID)->attach(pDecoderIn);

        // set data in to the demux input
        if (err == OCSD_OK)
            pDecoderIn = dynamic_cast<ITrcDataIn*>(&cs_frame_demux);
    }

    // create a packet printer typed for ITM printing
    pItemPrinter = PktPrinterFact::createProtocolPrinter(printer_list, OCSD_PROTOCOL_ITM, CSID, &logger);
    if (!pItemPrinter)
        err = OCSD_ERR_MEM;

    // attach the packet printer to the monitor output of the ITM packet processor
    if (err == OCSD_OK)
    {

        // cast the base printer class to its protocol specific class.
        PacketPrinter<ItmTrcPacket>* pTPrinter = dynamic_cast<PacketPrinter<ItmTrcPacket> *>(pItemPrinter);
        err = pDecoderMngr->attachPktMonitor(pItmTraceComp, (IPktRawDataMon<ItmTrcPacket> *) pTPrinter);
        pTPrinter->muteIDPrint(muteIDPrint);
    }

    // use printer factory to create a generic element output printer and attach message logger.
    if (err == OCSD_OK) {
        TrcGenericElementPrinter* pOutPrinter = PktPrinterFact::createGenElemPrinter(printer_list, &logger);
        if (pOutPrinter)
        {
            // output for the decoded trace
            err = pDecoderMngr->attachOutputSink(pItmTraceComp, pOutPrinter);
            pOutPrinter->muteIDPrint(muteIDPrint);
            
        }
        else
            err = OCSD_ERR_MEM;
    }

    // if any error after we created the decoder - destroy it now.
    if (err != OCSD_OK)
        destroy_itm_objects();

    return err;
}

ocsd_err_t process_block(ocsd_trc_index_t &trace_index, const uint8_t* data, const uint32_t block_size,
                                   uint32_t& num_processed, ocsd_datapath_resp_t &dataPathResp)
{
    ocsd_err_t err = OCSD_OK;
    uint32_t processedThisPass;

    dataPathResp = OCSD_RESP_CONT;
    while ((num_processed < block_size) && !OCSD_DATA_RESP_IS_FATAL(dataPathResp))
    {
        dataPathResp = pDecoderIn->TraceDataIn(OCSD_OP_DATA,
            trace_index,
            block_size - num_processed,
            data + num_processed,
            &processedThisPass);
        num_processed += processedThisPass;
        trace_index += processedThisPass;
    }

    // fatal error - no futher processing
    if (OCSD_DATA_RESP_IS_FATAL(dataPathResp))
    {
        std::ostringstream oss;
        oss << "Itm Test : Data Path fatal error\n";
        logger.LogMsg(oss.str());
        ocsdError* perr = errLogger.GetLastError();
        if (perr != 0)
        {
            logger.LogMsg(ocsdError::getErrorString(perr));
            err = perr->getErrorCode();
        }
        else
            err = OCSD_ERR_FAIL;
    }

    return err;
}

ocsd_err_t run_test(itm_test_data_t* curr_test, ocsd_trc_index_t &trace_index)
{
    ocsd_datapath_resp_t dataPathResp = OCSD_RESP_CONT;
    ocsd_err_t err = OCSD_OK;
    //ocsd_trc_index_t trace_index = 0;
    uint32_t numBytesProcessed = 0;

    err = process_block(trace_index, curr_test->test_packets, curr_test->test_data_size, numBytesProcessed, dataPathResp);

    if (!OCSD_DATA_RESP_IS_FATAL(dataPathResp))
        pDecoderIn->TraceDataIn(OCSD_OP_EOT, 0, 0, 0, 0);

    // reset the decoder
    pDecoderIn->TraceDataIn(OCSD_OP_RESET, 0, 0, 0, 0);

    return err;
}

ocsd_err_t run_tests()
{
    ocsd_err_t err = OCSD_OK;
    itm_test_data_t* tests = get_item_tests();
    itm_test_data_t* curr_test = 0;
    int test_idx = 0;
    std::ostringstream oss;
    ocsd_trc_index_t trace_index = 0;

    curr_test = &tests[test_idx];

    while (curr_test->test_data_size && (err == OCSD_OK))
    {

        if ((testName == "all") || (testName == curr_test->test_name))
        {
            oss.str("");
            oss << "\nRunning test " << curr_test->test_name << " - " << curr_test->test_description << ";...\n";
            logger.LogMsg(oss.str());


            // run test
            err = run_test(curr_test, trace_index);

            // done
            oss.str("");
            oss << "\nTest " << curr_test->test_name << " complete.\n";
            logger.LogMsg(oss.str());
        }

        curr_test = &tests[++test_idx];
    }

    oss.str("");
    oss << "All tests complete\n";
    logger.LogMsg(oss.str());

    return err;
}

// program can process binary file containing ITM raw data.
ocsd_err_t process_binfile()
{
    ocsd_err_t err = OCSD_OK;
    std::ostringstream oss;
    std::ifstream in;

    oss << "\nDecoding ITM data from file: " << inFileName << ",\n";
    if (muxCSID) {
        oss << "as CoreSight frame formatted data with ITM stream in Trace ID 0x";
        oss << std::hex << std::setfill('0') << std::setw(2) << (uint16_t)muxCSID << ".\n";
    }
    else
        oss << "as raw ITM data, with no CoreSight frame formatting.\n";

    logger.LogMsg(oss.str());

    in.open(inFileName, std::ifstream::in | std::ifstream::binary);
    if (in.is_open())
    {
        ocsd_datapath_resp_t dataPathResp = OCSD_RESP_CONT;
        static const int bufferSize = 1024;
        uint8_t trace_buffer[bufferSize];   // temporary buffer to load blocks of data from the file
        uint32_t trace_index = 0;           // index into the overall trace buffer (file).

        // read files a block at a time
        while (!in.eof() && (err == OCSD_OK))
        {

            in.read((char*)&trace_buffer[0], bufferSize);

            std::streamsize nBuffRead = in.gcount();    // get count of data loaded.
            uint32_t nBuffProcessed = 0;         // amount processed in this buffer.

            err = process_block(trace_index, trace_buffer, nBuffRead, nBuffProcessed, dataPathResp);            
        }

        if (!OCSD_DATA_RESP_IS_FATAL(dataPathResp))
            pDecoderIn->TraceDataIn(OCSD_OP_EOT, 0, 0, 0, 0);

    }
    else
    {
        oss.str("");
        oss << "\nError: failed to open input file\n";
        logger.LogMsg(oss.str());
    }

    oss.str("");
    oss << "\nFile decode complete\n";
    logger.LogMsg(oss.str());
    return err;
}

int main(int argc, char* argv[])
{
    std::ostringstream moss;

    ocsd_err_t err = OCSD_OK;
    ocsd_hndl_err_log_t err_src_handle;
    bool OptExit = false;

    if (process_cmd_line_logger_opts(argc, argv))
    {
        printf("Bad logger command line options\nProgram Exiting\n");
        return -2;
    }

    logger.setLogOpts(logOpts);
    logger.setLogFileName(logfileName.c_str());

    // set the output logger in the error logger.
    errLogger.setOutputLogger(&logger);
    err_src_handle = errLogger.RegisterErrorSource("ITM_TEST");


    moss << "ITM decode test\n";
    moss << "-----------------------------------------------\n\n";
    moss << "** Library Version : " << ocsdVersion::vers_str() << "\n\n";
    logger.LogMsg(moss.str());

    if (!process_cmd_line_opts(argc, argv, OptExit))
    {
        // option just printed and wants to exit
        if (OptExit)
            return 0;

        moss.str("");
        moss << "\nBad command line options\nProgram exiting\n";
        logger.LogMsg(moss.str());
        print_help();
        return -1;
    }    

    // got through command line options - create the decoder and run tests.
    err = setup_decoder();
    if (err) 
    {
        const ocsdError &rErr = ocsdError(OCSD_ERR_SEV_ERROR, err, "Failed to set up ITM decoder\n");
        errLogger.LogError(err_src_handle, &rErr);
    }

    if (bProcessInFile)
        err = process_binfile();
    else
        err = run_tests();

    destroy_itm_objects();
    
    return (err == OCSD_OK) ? 0 : -3;
}
