/*
 * \file       ss_to_dcdtree.cpp
 * \brief      Reference CoreSight Trace Decoder : 
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

bool CreateDcdTreeFromSnapShot::createDecodeTree(const std::string &SourceName, bool bPacketProcOnly)
{    
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

            /* make a note of the trace binary file name + path to ss directory */            
            m_BufferFileName = m_pReader->getSnapShotDir() + tree.buffer_info.dataFileName;

            /* create the initial device tree */
            // TBD: handle raw input with no formatted frame data.
            //      handle syncs / hsyncs data from TPIU
            m_pDecodeTree = DecodeTree::CreateDecodeTree(RCTDL_TRC_SRC_FRAME_FORMATTED,RCTDL_DFRMTR_FRAME_MEM_ALIGN); 
            
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
                                it++;
                                numDecodersCreated++;
                            }
                            else
                            {
                                // TBD: failed to create decoder for the device
                                // may be unsupported so just create ones we know about.
                            }
                        }
                        else
                        {
                            // TBD: could not find the device data for the core.
                            // unexpected - since we created the associations.
                        }
                    }
                    else
                    {
                        // TBD: none-core source 
                        // not supported in the library at present.
                    }
                }
                else
                {
                    // TBD: could not find the device data for the source.
                    // again unexpected - suggests ss format error.
                }
            }

            if(numDecodersCreated == 0)
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
    rctdlError err(RCTDL_ERR_SEV_ERROR,RCTDL_ERR_TEST_SS_TO_DECODER,msg);
    m_pErrLogInterface->LogError(m_errlog_handle,&err);
}

bool CreateDcdTreeFromSnapShot::createPEDecoder(const std::string &coreName, Parser::Parsed *devSrc)
{
    bool bCreatedDecoder = false;

    // split according to protocol 
    if(devSrc->deviceTypeName == ETMv4Protocol)
    {
        bCreatedDecoder = createETMv4Decoder(coreName,devSrc);
    }

    return bCreatedDecoder;
}

bool CreateDcdTreeFromSnapShot::createETMv4Decoder(const std::string &coreName, Parser::Parsed *devSrc, const bool bDataChannel /* = false*/)
{
    bool createdDecoder = false;
    bool configError = false;
    
    // generate the config data from the device data.
    EtmV4Config config;
    std::string reg_value;

    struct _regs_to_access {
        const char *pszName;
        bool failIfMissing;
        uint32_t *value;
        uint32_t val_default;
    } regs_to_access[] = {
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

    for(int rv = 0; rv < sizeof(regs_to_access)/sizeof(struct _regs_to_access); rv++)
    {
        if(!getRegByPrefix( devSrc->regDefs,
                            regs_to_access[rv].pszName,
                            regs_to_access[rv].value,
                            regs_to_access[rv].failIfMissing,
                            regs_to_access[rv].val_default))
        {
            configError = true;
            break;
        }
    }

    // so far, so good
    if(!configError)
    {
        rctdl_arch_profile_t ap = m_arch_profiles.getArchProfile(coreName);
        if(ap.arch != ARCH_UNKNOWN)
        {
            config.arch_ver = ap.arch;
            config.core_prof = ap.profile;
        }
        else
        {
            std::ostringstream oss;
            oss << "Unrecognized Core name " << coreName << ". Cannot evaluate profile or architecture.";
            LogError(oss.str());
            configError = true;
        }
    }
    
    // good config - generate the decoder on the tree.
    if(!configError)
    {
        if(m_bPacketProcOnly)
            m_pDecodeTree->createETMv4PktProcessor(&config,bDataChannel);
        else
            m_pDecodeTree->createETMv4Decoder(&config,bDataChannel);
        createdDecoder = true;
    }

    return createdDecoder;
}

// strip out any parts with brackets
bool CreateDcdTreeFromSnapShot::getRegByPrefix(std::map<std::string, std::string, Util::CaseInsensitiveLess> &regDefs, 
    const std::string &prefix,              
    uint32_t *value,
    bool failIfMissing,
    uint32_t val_default)
{
    std::ostringstream oss;
    bool bFound = false;
    std::map<std::string, std::string, Util::CaseInsensitiveLess>::iterator it;
    std::string prefix_cmp;
    std::string::size_type pos;
    std::string strval;
    *value = 0;

    it = regDefs.begin();
    while((it != regDefs.end()) && !bFound)
    {
        prefix_cmp = it->first;
        pos = prefix_cmp.find_first_of('(');
        if(pos != std::string::npos)
        {
            prefix_cmp = prefix_cmp.substr(0, pos);
        }
        if(prefix_cmp == prefix)
        {
            strval = it->second;
            bFound = true;
        }
        it++;
    }

    if(bFound)
        *value = strtoul(strval.c_str(),0,0);
    else
    {
        rctdl_err_severity_t sev = RCTDL_ERR_SEV_ERROR;
        if(failIfMissing)
        {
            bFound = false;
            oss << "Error:";
        }
        else
        {
            oss << "Warning:";
            sev = RCTDL_ERR_SEV_WARN;

        }
        oss << "Missing " << prefix << "\n";
        m_pErrLogInterface->LogMessage(m_errlog_handle, sev, oss.str());
    }
    return bFound;
}


/* End of File ss_to_dcdtree.cpp */
