/*
 * \file       ss_to_dcdtree.cpp
 * \brief      OpenCSD : 
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

#include "ss_to_dcdtree.h"
#include "ss_key_value_names.h"


CreateDcdTreeFromSnapShot::CreateDcdTreeFromSnapShot() :
    m_bInit(false),
    m_pDecodeTree(0),
    m_pReader(0),
    m_pErrLogInterface(0),    
    m_bPacketProcOnly(false),
    m_BufferFileName("")
{
    m_errlog_handle = 0;
    m_add_create_flags = 0;
}

CreateDcdTreeFromSnapShot::~CreateDcdTreeFromSnapShot()
{
    destroyDecodeTree();
}
    
void CreateDcdTreeFromSnapShot::initialise(SnapShotReader *pReader, ITraceErrorLog *pErrLogInterface)
{
    if((pErrLogInterface != 0) && (pReader != 0))
    {
        m_pReader = pReader;
        m_pErrLogInterface = pErrLogInterface;
        m_errlog_handle = m_pErrLogInterface->RegisterErrorSource("ss2_dcdtree");
        m_bInit = true;
    }
}

std::string CreateDcdTreeFromSnapShot::getBufferFileNameFromBuffName(const std::string& buff_name)
{
    Parser::TraceBufferSourceTree tree;
    std::string buffFileName = "";

    if (m_pReader->getTraceBufferSourceTree(buff_name, tree))
    {
        buffFileName = m_pReader->getSnapShotDir() + tree.buffer_info.dataFileName;
    }
    return buffFileName;
}


bool CreateDcdTreeFromSnapShot::createDecodeTree(const std::string &SourceName, bool bPacketProcOnly, uint32_t add_create_flags)
{   
    ocsd_err_t err = OCSD_OK;

    m_add_create_flags = add_create_flags;
    if(m_bInit)
    {
        if(!m_pReader->snapshotReadOK())
        {
            LogError("Supplied snapshot reader has not correctly read the snapshot.\n");
            return false;
        }

        m_bPacketProcOnly = bPacketProcOnly;
        Parser::TraceBufferSourceTree tree;

        if(m_pReader->getTraceBufferSourceTree(SourceName, tree))
        {
            int numDecodersCreated = 0; // count how many we create - if none then give up.
            uint32_t formatter_flags = OCSD_DFRMTR_FRAME_MEM_ALIGN;

            /* make a note of the trace binary file name + path to ss directory */            
            m_BufferFileName = m_pReader->getSnapShotDir() + tree.buffer_info.dataFileName;

            ocsd_dcd_tree_src_t src_format = tree.buffer_info.dataFormat == "source_data" ? OCSD_TRC_SRC_SINGLE : OCSD_TRC_SRC_FRAME_FORMATTED;

            if (tree.buffer_info.dataFormat == "dstream_coresight")
                formatter_flags = OCSD_DFRMTR_HAS_FSYNCS;

            /* create the initial device tree */
            // TBD:     handle syncs / hsyncs data from TPIU
            m_pDecodeTree = DecodeTree::CreateDecodeTree(src_format, formatter_flags);
            if(m_pDecodeTree == 0)
            {
                LogError("Failed to create decode tree object\n");
                return false;
            }

            // use our error logger - don't use the tree default.
            m_pDecodeTree->setAlternateErrorLogger(m_pErrLogInterface);

            if(!bPacketProcOnly)
            {
                m_pDecodeTree->createMemAccMapper();
            }

            /* run through each protocol source to this buffer... */
            std::map<std::string, std::string>::iterator it = tree.source_core_assoc.begin();

            while(it != tree.source_core_assoc.end())
            {
                Parser::Parsed *etm_dev, *core_dev;
                if(m_pReader->getDeviceData(it->first,&etm_dev))
                {
                    // found the device data for this device.

                    // see if we have a core name (STM / ITM not associated with a core);
                    std::string coreDevName = it->second;
                    if(coreDevName.size() > 0)
                    {
                        if(m_pReader->getDeviceData(coreDevName,&core_dev))
                        {
                            if(createPEDecoder(core_dev->deviceTypeName,etm_dev))
                            {
                                numDecodersCreated++;
                                if(!bPacketProcOnly &&(core_dev->dumpDefs.size() > 0))
                                {
                                    err = processDumpfiles(core_dev->dumpDefs);
                                }
                            }
                            else
                            {
                                std::ostringstream oss;
                                oss << "Failed to create decoder for source " << it->first << ".\n";
                                LogError(oss.str());
                            }
                        }
                        else
                        {
                            // Could not find the device data for the core.
                            // unexpected - since we created the associations.
                            std::ostringstream oss;
                            oss << "Failed to get device data for source " << it->first << ".\n";
                            LogError(oss.str());
                        }
                    }
                    else
                    {
                        // none-core source 
                        if(createSTDecoder(etm_dev))
                        {
                             numDecodersCreated++;
                        }
                        else
                        {
                            std::ostringstream oss;
                            oss << "Failed to create decoder for none core source " << it->first << ".\n";
                            LogError(oss.str());
                        }
                    }
                }
                else
                {
                    // TBD: could not find the device data for the source.
                    // again unexpected - suggests ss format error.
                    std::ostringstream oss;
                    oss << "Failed to find device data for source " << it->first << ".\n";
                    LogError(oss.str());
                }
                if(src_format == OCSD_TRC_SRC_SINGLE)
                    it = tree.source_core_assoc.end();
                else
                    it++;
            }

            if((numDecodersCreated == 0) || (err != OCSD_OK))
            {
                // nothing useful found 
                destroyDecodeTree();
            }
        }
        else
        {
            std::ostringstream oss;
            oss << "Failed to get parsed source tree for buffer " << SourceName << ".\n";
            LogError(oss.str());
        }
    }
    return (bool)(m_pDecodeTree != 0);
}

void CreateDcdTreeFromSnapShot::destroyDecodeTree()
{
    if(m_pDecodeTree)
        DecodeTree::DestroyDecodeTree(m_pDecodeTree);
    m_pDecodeTree = 0;
    m_pReader = 0;
    m_pErrLogInterface = 0;
    m_errlog_handle = 0;
    m_BufferFileName = "";
}

void  CreateDcdTreeFromSnapShot::LogError(const std::string &msg)
{
    ocsdError err(OCSD_ERR_SEV_ERROR,OCSD_ERR_TEST_SS_TO_DECODER,msg);
    m_pErrLogInterface->LogError(m_errlog_handle,&err);
}

void CreateDcdTreeFromSnapShot::LogError(const ocsdError &err)
{
    m_pErrLogInterface->LogError(m_errlog_handle,&err);
}

bool CreateDcdTreeFromSnapShot::createPEDecoder(const std::string &coreName, Parser::Parsed *devSrc)
{
    bool bCreatedDecoder = false;
    std::string devTypeName = devSrc->deviceTypeName;

    // split off .x from type name.
    std::string::size_type pos = devTypeName.find_first_of('.');
    if(pos != std::string::npos)
        devTypeName = devTypeName.substr(0,pos);

    // split according to protocol 
    if(devTypeName == ETMv4Protocol)
    {
        bCreatedDecoder = createETMv4Decoder(coreName,devSrc);
    }
    else if(devTypeName == ETMv3Protocol)
    {
        bCreatedDecoder = createETMv3Decoder(coreName,devSrc);
    }
    else if(devTypeName == PTMProtocol || devTypeName == PFTProtocol)
    {
        bCreatedDecoder = createPTMDecoder(coreName,devSrc);
    }
    else if (devTypeName == ETEProtocol)
    {
        bCreatedDecoder = createETEDecoder(coreName, devSrc);
    }

    return bCreatedDecoder;
}

// create an ETMv4 decoder based on the deviceN.ini file.
bool CreateDcdTreeFromSnapShot::createETMv4Decoder(const std::string &coreName, Parser::Parsed *devSrc, const bool bDataChannel /* = false*/)
{
    bool createdDecoder = false;
    bool configOK = true;
    
    // generate the config data from the device data.
    ocsd_etmv4_cfg config;

    regs_to_access_t regs_to_access[] = {
        { ETMv4RegCfg, true, &config.reg_configr, 0 },
        { ETMv4RegIDR, true, &config.reg_traceidr, 0 },
        { ETMv4RegIDR0, true, &config.reg_idr0, 0 },
        { ETMv4RegIDR1, false, &config.reg_idr1, 0x4100F403 },
        { ETMv4RegIDR2, true, &config.reg_idr2, 0 },
        { ETMv4RegIDR8, false, &config.reg_idr8, 0 },
        { ETMv4RegIDR9, false, &config.reg_idr9, 0 },
        { ETMv4RegIDR10, false, &config.reg_idr10, 0 },
        { ETMv4RegIDR11, false, &config.reg_idr11, 0 },
        { ETMv4RegIDR12, false, &config.reg_idr12, 0 },
        { ETMv4RegIDR13,false, &config.reg_idr13, 0 },
    };

    // extract registers
    configOK = getRegisters(devSrc->regDefs,sizeof(regs_to_access)/sizeof(regs_to_access_t), regs_to_access);

    // extract core profile
    if(configOK)
        configOK = getCoreProfile(coreName,config.arch_ver,config.core_prof);

    // good config - generate the decoder on the tree.
    if(configOK)
    {
        ocsd_err_t err = OCSD_OK;
        EtmV4Config configObj(&config);
        const char *decoderName = bDataChannel ? OCSD_BUILTIN_DCD_ETMV4D : OCSD_BUILTIN_DCD_ETMV4I;

        err = m_pDecodeTree->createDecoder(decoderName, m_add_create_flags | (m_bPacketProcOnly ? OCSD_CREATE_FLG_PACKET_PROC : OCSD_CREATE_FLG_FULL_DECODER),&configObj);
        
        if(err ==  OCSD_OK)
            createdDecoder = true;
        else
        {
            std::string msg = "Snapshot processor : failed to create " +  (std::string)decoderName + " decoder on decode tree.";
            LogError(ocsdError(OCSD_ERR_SEV_ERROR,err,msg));
        }
    }

    return createdDecoder;
}

bool CreateDcdTreeFromSnapShot::createETEDecoder(const std::string &coreName, Parser::Parsed *devSrc)
{
    bool createdDecoder = false;
    bool configOK = true;

    // generate the config data from the device data.
    ocsd_ete_cfg config;   

    // ete regs are same names Etmv4 in places...
    regs_to_access_t regs_to_access[] = {
        { ETMv4RegCfg, true, &config.reg_configr, 0 },
        { ETMv4RegIDR, true, &config.reg_traceidr, 0 },
        { ETMv4RegIDR0, true, &config.reg_idr0, 0 },
        { ETMv4RegIDR1, false, &config.reg_idr1, 0x4100F403 },
        { ETMv4RegIDR2, true, &config.reg_idr2, 0 },
        { ETMv4RegIDR8, false, &config.reg_idr8, 0 },
        { ETERegDevArch, false, &config.reg_devarch, 0x47705A13 },
    };

    // extract registers
    configOK = getRegisters(devSrc->regDefs, sizeof(regs_to_access) / sizeof(regs_to_access_t), regs_to_access);

    // extract core profile
    if (configOK)
        configOK = getCoreProfile(coreName, config.arch_ver, config.core_prof);

    // good config - generate the decoder on the tree.
    if (configOK)
    {
        ocsd_err_t err = OCSD_OK;
        ETEConfig configObj(&config);
        const char *decoderName = OCSD_BUILTIN_DCD_ETE;

        err = m_pDecodeTree->createDecoder(decoderName, m_add_create_flags | (m_bPacketProcOnly ? OCSD_CREATE_FLG_PACKET_PROC : OCSD_CREATE_FLG_FULL_DECODER), &configObj);

        if (err == OCSD_OK)
            createdDecoder = true;
        else
        {
            std::string msg = "Snapshot processor : failed to create " + (std::string)decoderName + " decoder on decode tree.";
            LogError(ocsdError(OCSD_ERR_SEV_ERROR, err, msg));
        }
    }

    return createdDecoder;
}

// create an ETMv3 decoder based on the register values in the deviceN.ini file.
bool CreateDcdTreeFromSnapShot::createETMv3Decoder(const std::string &coreName, Parser::Parsed *devSrc)
{
    bool createdDecoder = false;
    bool configOK = true;
    
    // generate the config data from the device data.
    ocsd_etmv3_cfg cfg_regs;

    regs_to_access_t regs_to_access[] = {
        { ETMv3PTMRegIDR, true, &cfg_regs.reg_idr, 0 },
        { ETMv3PTMRegCR, true, &cfg_regs.reg_ctrl, 0 },
        { ETMv3PTMRegCCER, true, &cfg_regs.reg_ccer, 0 },
        { ETMv3PTMRegTraceIDR, true, &cfg_regs.reg_trc_id, 0}
    };

    // extract registers
    configOK = getRegisters(devSrc->regDefs,sizeof(regs_to_access)/sizeof(regs_to_access_t), regs_to_access);

    // extract core profile
    if(configOK)
        configOK = getCoreProfile(coreName,cfg_regs.arch_ver,cfg_regs.core_prof);

    // good config - generate the decoder on the tree.
    if(configOK)
    {
        EtmV3Config config(&cfg_regs);
        ocsd_err_t err = OCSD_OK;
        err = m_pDecodeTree->createDecoder(OCSD_BUILTIN_DCD_ETMV3, m_bPacketProcOnly ? OCSD_CREATE_FLG_PACKET_PROC : OCSD_CREATE_FLG_FULL_DECODER,&config);

        if(err ==  OCSD_OK)
            createdDecoder = true;
        else
            LogError(ocsdError(OCSD_ERR_SEV_ERROR,err,"Snapshot processor : failed to create ETMV3 decoder on decode tree."));
    }
    return createdDecoder;
}

bool CreateDcdTreeFromSnapShot::createPTMDecoder(const std::string &coreName, Parser::Parsed *devSrc)
{
    bool createdDecoder = false;
    bool configOK = true;
    
    // generate the config data from the device data.
    
    ocsd_ptm_cfg config;

    regs_to_access_t regs_to_access[] = {
        { ETMv3PTMRegIDR, true, &config.reg_idr, 0 },
        { ETMv3PTMRegCR, true, &config.reg_ctrl, 0 },
        { ETMv3PTMRegCCER, true, &config.reg_ccer, 0 },
        { ETMv3PTMRegTraceIDR, true, &config.reg_trc_id, 0}
    };

    // extract registers
    configOK = getRegisters(devSrc->regDefs,sizeof(regs_to_access)/sizeof(regs_to_access_t), regs_to_access);

    // extract core profile
    if(configOK)
        configOK = getCoreProfile(coreName,config.arch_ver,config.core_prof);

    // good config - generate the decoder on the tree.
    if(configOK)
    {
        PtmConfig configObj(&config);
        ocsd_err_t err = OCSD_OK;
        err = m_pDecodeTree->createDecoder(OCSD_BUILTIN_DCD_PTM, m_bPacketProcOnly ? OCSD_CREATE_FLG_PACKET_PROC : OCSD_CREATE_FLG_FULL_DECODER,&configObj);

        if(err ==  OCSD_OK)
            createdDecoder = true;
        else
            LogError(ocsdError(OCSD_ERR_SEV_ERROR,err,"Snapshot processor : failed to create PTM decoder on decode tree."));
    }
    return createdDecoder;
}

bool CreateDcdTreeFromSnapShot::createSTDecoder(Parser::Parsed *devSrc)
{
    bool bCreatedDecoder = false;
    std::string devTypeName = devSrc->deviceTypeName;

    // split off .x from type name.
    std::string::size_type pos = devTypeName.find_first_of('.');
    if(pos != std::string::npos)
        devTypeName = devTypeName.substr(0,pos);

    if(devTypeName == STMProtocol)
    {
        bCreatedDecoder = createSTMDecoder(devSrc);
    }
    else if (devTypeName == ITMProtocol)
    {
        bCreatedDecoder = createITMDecoder(devSrc);
    }

    return bCreatedDecoder;
}

bool CreateDcdTreeFromSnapShot::createSTMDecoder(Parser::Parsed *devSrc)
{
    bool createdDecoder = false;
    bool configOK = true;

    // generate the config data from the device data.
    
    ocsd_stm_cfg config;

    regs_to_access_t regs_to_access[] = {
        { STMRegTCSR, true, &config.reg_tcsr, 0 }
    };

    configOK = getRegisters(devSrc->regDefs,sizeof(regs_to_access)/sizeof(regs_to_access_t), regs_to_access);
    if(configOK)
    {
        ocsd_err_t err = OCSD_OK;
        STMConfig configObj(&config);

        err = m_pDecodeTree->createDecoder(OCSD_BUILTIN_DCD_STM, m_bPacketProcOnly ? OCSD_CREATE_FLG_PACKET_PROC : OCSD_CREATE_FLG_FULL_DECODER,&configObj);

        if(err ==  OCSD_OK)
            createdDecoder = true;
        else
            LogError(ocsdError(OCSD_ERR_SEV_ERROR,err,"Snapshot processor : failed to create STM decoder on decode tree."));
    }

    return createdDecoder;
}

bool CreateDcdTreeFromSnapShot::createITMDecoder(Parser::Parsed* devSrc)
{
    bool createdDecoder = false;
    bool configOK = true;

    // generate the config data from the device data.

    ocsd_itm_cfg config;

    regs_to_access_t regs_to_access[] = {
        { ITMRegTCR, true, &config.reg_tcr, 0 }
    };

    configOK = getRegisters(devSrc->regDefs, sizeof(regs_to_access) / sizeof(regs_to_access_t), regs_to_access);
    if (configOK)
    {
        ocsd_err_t err = OCSD_OK;
        ITMConfig configObj(&config);

        err = m_pDecodeTree->createDecoder(OCSD_BUILTIN_DCD_ITM, m_bPacketProcOnly ? OCSD_CREATE_FLG_PACKET_PROC : OCSD_CREATE_FLG_FULL_DECODER, &configObj);

        if (err == OCSD_OK)
            createdDecoder = true;
        else
            LogError(ocsdError(OCSD_ERR_SEV_ERROR, err, "Snapshot processor : failed to create ITM decoder on decode tree."));
    }

    return createdDecoder;
}



// get a set of register values.
bool CreateDcdTreeFromSnapShot::getRegisters(std::map<std::string, std::string, Util::CaseInsensitiveLess> &regDefs, int numRegs, regs_to_access_t *reg_access_array)
{
    bool regsOK = true;

    for(int rv = 0; rv < numRegs; rv++)
    {
        if(!getRegByPrefix( regDefs,reg_access_array[rv]))
            regsOK = false;
    }
    return regsOK;
}

// strip out any parts with brackets
bool CreateDcdTreeFromSnapShot::getRegByPrefix(std::map<std::string, std::string, Util::CaseInsensitiveLess> &regDefs, 
    regs_to_access_t &reg_accessor)
{
    std::ostringstream oss;
    bool bFound = false;
    std::map<std::string, std::string, Util::CaseInsensitiveLess>::iterator it;
    std::string prefix_cmp;
    std::string::size_type pos;
    std::string strval;
    
    *reg_accessor.value = 0;

    it = regDefs.begin();
    while((it != regDefs.end()) && !bFound)
    {
        prefix_cmp = it->first;
        pos = prefix_cmp.find_first_of('(');
        if(pos != std::string::npos)
        {
            prefix_cmp = prefix_cmp.substr(0, pos);
        }
        if(prefix_cmp == reg_accessor.pszName)
        {
            strval = it->second;
            bFound = true;
        }
        it++;
    }

    if(bFound)
        *reg_accessor.value = strtoul(strval.c_str(),0,0);
    else
    {
        ocsd_err_severity_t sev = OCSD_ERR_SEV_ERROR;
        if(reg_accessor.failIfMissing)
        {
            oss << "Error:";
        }
        else
        {
            // no fail if missing - set any default and just warn.
            bFound = true;
            oss << "Warning: Default set for register. ";
            sev = OCSD_ERR_SEV_WARN;
            *reg_accessor.value = reg_accessor.val_default;
        }
        oss << "Missing " << reg_accessor.pszName << "\n";
        m_pErrLogInterface->LogMessage(m_errlog_handle, sev, oss.str());
    }
    return bFound;
}

bool CreateDcdTreeFromSnapShot::getCoreProfile(const std::string &coreName, ocsd_arch_version_t &arch_ver, ocsd_core_profile_t &core_prof)
{
    bool profileOK = true;
    ocsd_arch_profile_t ap = m_arch_profiles.getArchProfile(coreName);
    if(ap.arch != ARCH_UNKNOWN)
    {
        arch_ver = ap.arch;
        core_prof = ap.profile;
    }
    else
    {
        std::ostringstream oss;
        oss << "Unrecognized Core name " << coreName << ". Cannot evaluate profile or architecture.";
        LogError(oss.str());
        profileOK = false;
    }
    return profileOK;
}

typedef struct mem_space_keys {
    const char* key_name;
    ocsd_mem_space_acc_t memspace;
} mem_space_keys_t;

static mem_space_keys_t space_map[] = {
    /* single spaces */
    { "EL1S", OCSD_MEM_SPACE_EL1S },
    { "EL1N", OCSD_MEM_SPACE_EL1N },
    { "EL1R", OCSD_MEM_SPACE_EL1R },
    { "EL2S", OCSD_MEM_SPACE_EL2S },
    { "EL2" , OCSD_MEM_SPACE_EL2 }, /* old EL2 NS name - prior to EL2S existing */
    { "EL2N", OCSD_MEM_SPACE_EL2 },
    { "EL2R", OCSD_MEM_SPACE_EL2R },
    { "EL3" , OCSD_MEM_SPACE_EL3  },
    { "ROOT", OCSD_MEM_SPACE_ROOT },
    /* multiple memory spaces */
    { "S"   , OCSD_MEM_SPACE_S},
    { "N"   , OCSD_MEM_SPACE_N},
    { "R"   , OCSD_MEM_SPACE_R},
    { "ANY" , OCSD_MEM_SPACE_ANY},
    /* older names - from spec but not expected to be used in future. */
    { "H"   , OCSD_MEM_SPACE_EL2 }, /* hypervisor - EL2 NS */
    { "P"   , OCSD_MEM_SPACE_EL1N }, /* privileged - EL1 NS */
    { "NP"   , OCSD_MEM_SPACE_EL1N }, /* non secure privileged - EL1 NS */
    { "SP"   , OCSD_MEM_SPACE_EL1S }, /* secure privileged - EL1 S */
    /* table terminator */
    { "", OCSD_MEM_SPACE_NONE},
};

/* get mem space from input string */
ocsd_mem_space_acc_t CreateDcdTreeFromSnapShot::getMemSpaceFromString(const std::string& memspace)
{
    
    
    ocsd_mem_space_acc_t mem_space = OCSD_MEM_SPACE_ANY;
    if (memspace.length() > 0) {
        int i = 0;

        while (space_map[i].memspace != OCSD_MEM_SPACE_NONE) {
            if (space_map[i].key_name == memspace) {
                mem_space = space_map[i].memspace;
                break;
            }
            i++;
        }
    }
    return mem_space;
}

ocsd_err_t CreateDcdTreeFromSnapShot::processDumpfiles(std::vector<Parser::DumpDef> &dumps)
{
    std::string dumpFilePathName;
    std::vector<Parser::DumpDef>::const_iterator it;
    ocsd_mem_space_acc_t mem_space;
    ocsd_err_t err = OCSD_OK;

    it = dumps.begin();
    while(it != dumps.end())
    {        
        dumpFilePathName = m_pReader->getSnapShotDir() + it->path;
        ocsd_file_mem_region_t region;
        ocsd_err_t err = OCSD_OK;

        region.start_address = it->address;
        region.file_offset = it->offset;
        region.region_size = it->length;
        mem_space = getMemSpaceFromString(it->space);

        // ensure we respect optional length and offset parameter and
        // allow multiple dump entries with same file name to define regions
        if (!TrcMemAccessorFile::isExistingFileAccessor(dumpFilePathName))
            err = m_pDecodeTree->addBinFileRegionMemAcc(&region, 1, mem_space, dumpFilePathName);
        else
            err = m_pDecodeTree->updateBinFileRegionMemAcc(&region, 1, mem_space, dumpFilePathName);
        if(err != OCSD_OK)
        {                            
            std::ostringstream oss;
            oss << "Failed to create memory accessor for file " << dumpFilePathName << ".";
            LogError(ocsdError(OCSD_ERR_SEV_ERROR,err,oss.str()));
        }
        it++;
    }
    return err;
}

/* End of File ss_to_dcdtree.cpp */
