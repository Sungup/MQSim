
set(Boost_USE_STATIC_LIBS ${STATIC_BOOST})
set(Boost_USE_STATIC_RUNTIME ${STATIC_BOOST})
find_package(Boost COMPONENTS system filesystem)

find_package(Boost COMPONENTS program_options REQUIRED)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
else (Boost_FOUND)
    message(FATAL_ERROR "Boost library doesn't exist in this system!!")
endif (Boost_FOUND)

# RapidJSON corrently not in use until
#
# find_package(RapidJSON)
# if (RapidJSON_FOUND)
#     include_directories(${RAPIDJSON_INCLUDE_DIRS})
# else (RapidJSON_FOUND)
#     message(FATAL_ERROR "RapidJSON doesn't exist in this system!!")
# endif (RapidJSON_FOUND)

set(MQSIM_EXEC_SOURCES
    exec/params/DeviceParameterSet.cpp
    exec/params/DeviceParameterSet.h
    exec/params/ExecParameterSet.cpp
    exec/params/ExecParameterSet.h
    exec/params/FlashParameterSet.cpp
    exec/params/FlashParameterSet.h
    exec/params/HostParameterSet.cpp
    exec/params/HostParameterSet.h
    exec/params/IOFlowParamSet.cpp
    exec/params/IOFlowParamSet.h
    exec/params/ParameterSetBase.h
    exec/HostSystem.cpp
    exec/HostSystem.h
    exec/SsdDevice.cpp
    exec/SsdDevice.h)

set(MQSIM_HOST_SOURCES
    host/ioflow/IO_Flow_Base.cpp
    host/ioflow/IO_Flow_Base.h
    host/ioflow/IO_Flow_Synthetic.cpp
    host/ioflow/IO_Flow_Synthetic.h
    host/ioflow/IO_Flow_Trace_Based.cpp
    host/ioflow/IO_Flow_Trace_Based.h
    host/ioflow/IoFlowLog.cpp
    host/ioflow/IoFlowLog.h
    host/ioflow/IoFlowStats.cpp
    host/ioflow/IoFlowStats.h
    host/ioflow/IoQueueInfo.h
    host/ioflow/SubmissionQueue.h
    host/ioflow/SynFlowRandGen.cpp
    host/ioflow/SynFlowRandGen.h
    host/Host_Defs.h
    host/HostIORequest.h
    host/PCIe_Link.cpp
    host/PCIe_Link.h
    host/PCIeMessage.h
    host/PCIe_Root_Complex.cpp
    host/PCIe_Root_Complex.h
    host/PCIeSwitch.cpp
    host/PCIeSwitch.h
    host/SATA_HBA.cpp
    host/SATA_HBA.h)

set(MQSIM_NVM_CHIP_SOURCES
    nvm_chip/NVM_Chip.h
    nvm_chip/NVM_Memory_Address.h
    nvm_chip/NVM_Types.h
    nvm_chip/flash_memory/Block.h
    nvm_chip/flash_memory/Die.h
    nvm_chip/flash_memory/FlashTypes.h
    nvm_chip/flash_memory/Flash_Chip.cpp
    nvm_chip/flash_memory/Flash_Chip.h
    nvm_chip/flash_memory/FlashCommand.h
    nvm_chip/flash_memory/Page.h
    nvm_chip/flash_memory/Physical_Page_Address.cpp
    nvm_chip/flash_memory/Physical_Page_Address.h
    nvm_chip/flash_memory/Plane.h)

set(MQSIM_SIM_SOURCES
    sim/Engine.cpp
    sim/Engine.h
    sim/EventTree.cpp
    sim/EventTree.h
    sim/Sim_Defs.h
    sim/SimEvent.h
    sim/Sim_Object.h
    sim/Sim_Object.cpp
    sim/Sim_Reporter.h
    sim/Sim_Reporter.cpp)

set(MQSIM_SSD_SOURCES
    ssd/mapping/AddressMappingUnitDefs.h
    ssd/mapping/Address_Mapping_Unit_Base.cpp
    ssd/mapping/Address_Mapping_Unit_Base.h
    ssd/mapping/Address_Mapping_Unit_Hybrid.cpp
    ssd/mapping/Address_Mapping_Unit_Hybrid.h
    ssd/mapping/Address_Mapping_Unit_Page_Level.cpp
    ssd/mapping/Address_Mapping_Unit_Page_Level.h
    ssd/mapping/AllocationScheme.cpp
    ssd/mapping/AllocationScheme.h
    ssd/phy/PhyHandler.h
    ssd/tsu/TSU_Defs.h
    ssd/dcm/Data_Cache_Flash.cpp
    ssd/dcm/Data_Cache_Flash.h
    ssd/dcm/Data_Cache_Manager_Base.cpp
    ssd/dcm/Data_Cache_Manager_Base.h
    ssd/dcm/Data_Cache_Manager_Flash_Advanced.cpp
    ssd/dcm/Data_Cache_Manager_Flash_Advanced.h
    ssd/dcm/Data_Cache_Manager_Flash_Simple.cpp
    ssd/dcm/Data_Cache_Manager_Flash_Simple.h
    ssd/dcm/DataCacheDefs.h
    ssd/dcm/DataCacheSlot.h
    ssd/dcm/MemoryTransferInfo.h
    ssd/fbm/BlockPoolSlotType.h
    ssd/fbm/Flash_Block_Manager.cpp
    ssd/fbm/Flash_Block_Manager.h
    ssd/fbm/Flash_Block_Manager_Base.cpp
    ssd/fbm/Flash_Block_Manager_Base.h
    ssd/fbm/PlaneBookKeepingType.h
    ssd/gc_and_wl/GC_and_WL_Unit_Base.cpp
    ssd/gc_and_wl/GC_and_WL_Unit_Base.h
    ssd/gc_and_wl/GC_and_WL_Unit_Page_Level.cpp
    ssd/gc_and_wl/GC_and_WL_Unit_Page_Level.h
    ssd/gc_and_wl/GCandWLUnitDefs.h
    ssd/interface/Host_Interface_Base.cpp
    ssd/interface/Host_Interface_Base.h
    ssd/interface/Host_Interface_Defs.h
    ssd/interface/Host_Interface_NVMe.cpp
    ssd/interface/Host_Interface_NVMe.h
    ssd/interface/Host_Interface_SATA.cpp
    ssd/interface/Host_Interface_SATA.h
    ssd/interface/HostInterfaceHandler.h
    ssd/phy/NVM_PHY_Base.cpp
    ssd/phy/NVM_PHY_Base.h
    ssd/phy/NVM_PHY_ONFI.cpp
    ssd/phy/NVM_PHY_ONFI.h
    ssd/phy/NVM_PHY_ONFI_NVDDR2.cpp
    ssd/phy/NVM_PHY_ONFI_NVDDR2.h
    ssd/request/UserRequest.h
    ssd/tsu/TSU_Base.cpp
    ssd/tsu/TSU_Base.h
    ssd/tsu/TSU_OutofOrder.cpp
    ssd/tsu/TSU_OutofOrder.h
    ssd/Flash_Transaction_Queue.h
    ssd/FTL.cpp
    ssd/FTL.h
    ssd/NVM_Firmware.cpp
    ssd/NVM_Firmware.h
    ssd/NvmTransaction.h
    ssd/NvmTransactionFlash.h
    ssd/NvmTransactionFlashER.h
    ssd/NvmTransactionFlashRD.h
    ssd/NvmTransactionFlashWR.h
    ssd/ONFI_Channel_Base.cpp
    ssd/ONFI_Channel_Base.h
    ssd/ONFI_Channel_NVDDR2.cpp
    ssd/ONFI_Channel_NVDDR2.h
    ssd/ONFIChannelDefs.h
    ssd/Queue_Probe.cpp
    ssd/Queue_Probe.h
    ssd/SSD_Defs.h
    ssd/Stats.cpp
    ssd/Stats.h)

set(MQSIM_UTILS_SOURCES
    utils/rapidxml/rapidxml.hpp
    utils/rapidxml/rapidxml_iterators.hpp
    utils/rapidxml/rapidxml_print.hpp
    utils/rapidxml/rapidxml_utils.hpp
    utils/CountingStats.h
    utils/CMRRandomGenerator.cpp
    utils/CMRRandomGenerator.h
    utils/DistributionTypes.h
    utils/EnumTools.h
    utils/Exception.h
    utils/Helper_Functions.cpp
    utils/Helper_Functions.h
    utils/InlineTools.h
    utils/Logical_Address_Partitioning_Unit.cpp
    utils/Logical_Address_Partitioning_Unit.h
    utils/ObjectPool.h
    utils/ProgressBar.h
    utils/RandomGenerator.cpp
    utils/RandomGenerator.h
    utils/ServiceHandler.h
    utils/StringTools.h
    utils/Workload_Statistics.h
    utils/XMLWriter.cpp
    utils/XMLWriter.h)

add_executable(MQSim
               main.cpp
               ${MQSIM_EXEC_SOURCES}
               ${MQSIM_HOST_SOURCES}
               ${MQSIM_NVM_CHIP_SOURCES}
               ${MQSIM_SIM_SOURCES}
               ${MQSIM_SSD_SOURCES}
               ${MQSIM_UTILS_SOURCES})

install(TARGETS MQSim RUNTIME DESTINATION bin)
