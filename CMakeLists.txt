cmake_minimum_required(VERSION 3.16)
project(controller)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.2.sdk)
set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl@3/")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib")

find_package(OpenSSL REQUIRED)
find_package(cpprestsdk REQUIRED)

#find_package(libiperf REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread log program_options chrono)
find_library(IPERF_LIBRARY iperf)

# Open SSL Package
set(OpenSSL_LIBRARIES
        "${OPENSSL_ROOT_DIR}lib/libcrypto.dylib"
        "${OPENSSL_ROOT_DIR}lib/libssl.dylib")
include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(
        controller
        main.cpp
        controller/MasterController.cpp
        controller/MasterController.h
        utils/LockList/LockList.cpp
        utils/LockList/LockList.h
        Services/DeviceRegister/RegisterDevices.cpp
        Services/DeviceRegister/RegisterDevices.h
        Constants/RequestTypes.h model/enums/RequestTypeEnum.h
        Services/WorkQueueManager/WorkQueueManager.cpp
        Services/WorkQueueManager/WorkQueueManager.h
        model/data_models/WorkItems/BaseWorkItem/WorkItem.cpp
        model/data_models/WorkItems/BaseWorkItem/WorkItem.h
        model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.cpp
        model/data_models/WorkItems/ProcessingItem/HighProcessingItem/HighProcessingItem.h
        Constants/LayerType.h model/enums/LayerTypeEnum.h Constants/DNN_Type.h
        model/enums/DNN_Type_Enum.h Constants/ControllerRequestPaths.h
        model/data_models/ComputationDevice/ComputationDevice.cpp
        model/data_models/ComputationDevice/ComputationDevice.h
        model/data_models/LinkAct/LinkAct.cpp
        model/data_models/LinkAct/LinkAct.h
        model/data_models/Network/Network.cpp
        model/data_models/Network/Network.h
        Services/LowCompServices/LowCompServices.cpp
        Services/LowCompServices/LowCompServices.h
        Constants/DNNPadding.h
        model/data_models/CompResult/HighCompResult/HighCompResult.cpp
        model/data_models/CompResult/HighCompResult/HighCompResult.h
        model/data_models/TileRegion/TileRegion.cpp
        model/data_models/TileRegion/TileRegion.h
        Services/AllocationAlgorithmServices/AllocationAlgorithmServices.cpp
        Services/AllocationAlgorithmServices/AllocationAlgorithmServices.h
        Constants/CompNetCon.h
        model/data_models/WorkItems/StateUpdate/StateUpdate.cpp
        model/data_models/WorkItems/StateUpdate/StateUpdate.h
        Services/NetworkServices/NetworkServices.cpp
        Services/NetworkServices/NetworkServices.h
        model/data_models/CommunicationModels/BaseCommunicationModel/BaseCommModel.cpp
        model/data_models/CommunicationModels/BaseCommunicationModel/BaseCommModel.h
        Constants/CommTypes.h model/enums/CommsEnums.h
        Services/NetworkQueueManager/NetworkQueueManager.cpp
        Services/NetworkQueueManager/NetworkQueueManager.h
        model/data_models/NetworkCommsModels/BaseNetworkCommsModel/BaseNetworkCommsModel.cpp
        model/data_models/NetworkCommsModels/BaseNetworkCommsModel/BaseNetworkCommsModel.h
        model/enums/NetworkCommsTypes.h Constants/CLIENT_DETAILS.h
        utils/UtilFunctions/UtilFunctions.cpp utils/UtilFunctions/UtilFunctions.h
        utils/IPerfTest/IPerfTest.cpp utils/IPerfTest/IPerfTest.h
        Services/HighCompServices/HighCompServices.cpp
        Services/HighCompServices/HighCompServices.h
        Constants/FTP_CONFIG.h
        model/data_models/PartitionConfig/PartitionConfig.h
        model/data_models/PartitionConfig/PartitionConfig.cpp
        Constants/AllocationMacros.h
        model/data_models/NetworkCommsModels/HighComplexityAllocation/HighComplexityAllocationComms.cpp
        model/data_models/NetworkCommsModels/HighComplexityAllocation/HighComplexityAllocationComms.h
        model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.cpp
        model/data_models/WorkItems/ProcessingItem/LowProcessingItem/LowProcessingItem.h
        model/data_models/CompResult/LowCompResult/LowCompResult.cpp
        model/data_models/CompResult/LowCompResult/LowCompResult.h
        model/data_models/CompResult/BaseCompResult/BaseCompResult.cpp
        model/data_models/CompResult/BaseCompResult/BaseCompResult.h
        model/data_models/NetworkCommsModels/LowComplexityAllocation/LowComplexityAllocationComms.cpp
        model/data_models/NetworkCommsModels/LowComplexityAllocation/LowComplexityAllocationComms.h
        model/data_models/WorkItems/PruneItem/PruneItem.cpp model/data_models/WorkItems/PruneItem/PruneItem.h
        model/data_models/NetworkCommsModels/OutboundUpdate/OutboundUpdate.cpp
        model/data_models/NetworkCommsModels/OutboundUpdate/OutboundUpdate.h
        model/data_models/NetworkCommsModels/HaltNetworkCommsModel/HaltNetworkCommsModel.cpp
        model/data_models/NetworkCommsModels/HaltNetworkCommsModel/HaltNetworkCommsModel.h
        model/enums/LogTypes.h
        Services/LOG_MANAGER/LogManager.cpp
        Services/LOG_MANAGER/LogManager.h
        Constants/LOG_CONSTANT.h
        model/data_models/NetworkCommsModels/OutboundPrune/OutboundPrune.cpp
        model/data_models/NetworkCommsModels/OutboundPrune/OutboundPrune.h
        model/data_models/WorkItems/HaltWorkItem/HaltWorkItem.cpp
        model/data_models/WorkItems/HaltWorkItem/HaltWorkItem.h
        Constants/ModeMacros.h
        Constants/RequestSizes.h
        model/data_models/Bucket/Bucket.h
        model/data_models/Bucket/Bucket.cpp
        model/data_models/TimeWindow/TimeWindow.h
        model/data_models/TimeWindow/TimeWindow.cpp
        model/data_models/ResourceWindow/ResourceWindow.h
        model/data_models/ResourceWindow/ResourceWindow.h
        model/data_models/ResourceWindow/ResourceWindow.cpp
        model/data_models/ResourceTransformation/ResourceTransformation.h
        model/data_models/ResourceTransformation/ResourceTransformation.cpp
        Services/ProcessingDataStructureConversion/ProcessingDataStructureConversion.h
        Services/ProcessingDataStructureConversion/ProcessingDataStructureConversion.cpp
        model/data_models/ResourceAvailabilityList/ResourceAvailabilityList.cpp
        model/data_models/ResourceAvailabilityList/ResourceAvailabilityList.h
        Services/NetworkLinkDiscFunctions/NetworkDiscFunctions.cpp
        Services/NetworkLinkDiscFunctions/NetworkDiscFunctions.h
        model/data_models/WorkItems/BandwidthUpdate/BandwidthUpdateItem.cpp
        model/data_models/WorkItems/BandwidthUpdate/BandwidthUpdateItem.h
        Constants/EMA.h
        Constants/NETWORK_DISC_PARAMS.h
        model/data_models/NetworkCommsModels/BandwidthTestCommsModel/BandwidthTestCommsModel.cpp
        model/data_models/NetworkCommsModels/BandwidthTestCommsModel/BandwidthTestCommsModel.h
)

target_link_libraries(controller PRIVATE cpprestsdk::cpprest ${OpenSSL_LIBRARIES} ${IPERF_LIBRARY})