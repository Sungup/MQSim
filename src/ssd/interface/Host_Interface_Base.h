#ifndef HOST_INTERFACE_BASE_H
#define HOST_INTERFACE_BASE_H

#include <vector>
#include "../request/UserRequest.h"
#include "../../sim/Sim_Object.h"
#include "../../sim/Sim_Reporter.h"
#include "../../host/PCIe_Switch.h"
#include "../../host/PCIe_Message.h"
#include "../dcm/Data_Cache_Manager_Base.h"
#include <stdint.h>
#include <cstring>

#include "HostInterfaceHandler.h"
#include "../phy/PhyHandler.h"

namespace Host_Components
{
  class PCIe_Switch;
}

namespace SSD_Components
{
  class Data_Cache_Manager_Base;
  class Host_Interface_Base;

  // ============================
  // Declare of Input_Stream_Base
  // ============================
  #define DEFINE_ISB_AVG_IO_OPERATION_TIME(IO, OP)         \
  sim_time_type STAT_avg_##IO##_##OP##_time() const

  #define IMPL_ISB_AVG_IO_OPERATION_TIME(IO, OP, VALUE)    \
  force_inline sim_time_type                               \
  Input_Stream_Base::STAT_avg_##IO##_##OP##_time() const { \
    return (STAT_##IO##_transactions == 0)                 \
             ? 0                                           \
             : (VALUE                                      \
                  / STAT_##IO##_transactions               \
                  / SIM_TIME_TO_MICROSECONDS_COEFF);       \
  }

  #define IMPL_ISB_AVG_IO_OPERATION_TIME_BY_VAL(IO, TYPE) \
  IMPL_ISB_AVG_IO_OPERATION_TIME(IO, TYPE, STAT_total_##IO##_##TYPE##_time)

  #define IMPL_ISB_AVG_IO_OPERATION_TIME_BY_FUNC(IO, TYPE) \
  IMPL_ISB_AVG_IO_OPERATION_TIME(IO, TYPE, STAT_total_##IO##_##TYPE##_time())

  class Input_Stream_Base
  {
  public:
    uint32_t STAT_rd_requests;
    uint32_t STAT_wr_requests;
    uint32_t STAT_rd_transactions;
    uint32_t STAT_wr_transactions;

    sim_time_type STAT_total_rd_execution_time;
    sim_time_type STAT_total_rd_transfer_time;
    sim_time_type STAT_total_rd_waiting_time;
    sim_time_type STAT_total_wr_execution_time;
    sim_time_type STAT_total_wr_transfer_time;
    sim_time_type STAT_total_wr_waiting_time;

    Input_Stream_Base();
    virtual ~Input_Stream_Base() = default;

    sim_time_type STAT_total_rd_turnaround_time() const;
    sim_time_type STAT_total_wr_turnaround_time() const;

    void accumulate_rd_transaction_time(NVM_Transaction* tr);
    void accumulate_wr_transaction_time(NVM_Transaction* tr);

    DEFINE_ISB_AVG_IO_OPERATION_TIME(rd, execution);
    DEFINE_ISB_AVG_IO_OPERATION_TIME(rd, transfer);
    DEFINE_ISB_AVG_IO_OPERATION_TIME(rd, waiting);
    DEFINE_ISB_AVG_IO_OPERATION_TIME(rd, turnaround);

    DEFINE_ISB_AVG_IO_OPERATION_TIME(wr, execution);
    DEFINE_ISB_AVG_IO_OPERATION_TIME(wr, transfer);
    DEFINE_ISB_AVG_IO_OPERATION_TIME(wr, waiting);
    DEFINE_ISB_AVG_IO_OPERATION_TIME(wr, turnaround);
  };

  force_inline
  Input_Stream_Base::Input_Stream_Base()
    : STAT_rd_requests(0),
      STAT_wr_requests(0),
      STAT_rd_transactions(0),
      STAT_wr_transactions(0),
      STAT_total_rd_execution_time(0),
      STAT_total_rd_transfer_time(0),
      STAT_total_rd_waiting_time(0),
      STAT_total_wr_execution_time(0),
      STAT_total_wr_transfer_time(0),
      STAT_total_wr_waiting_time(0)
  { }

  force_inline sim_time_type
  Input_Stream_Base::STAT_total_rd_turnaround_time() const
  {
    return STAT_total_rd_execution_time
             + STAT_total_rd_transfer_time
             + STAT_total_rd_waiting_time;
  }

  force_inline sim_time_type
  Input_Stream_Base::STAT_total_wr_turnaround_time() const
  {
    return STAT_total_wr_execution_time
             + STAT_total_wr_transfer_time
             + STAT_total_wr_waiting_time;
  }

  force_inline void
  Input_Stream_Base::accumulate_rd_transaction_time(NVM_Transaction* tr)
  {
    STAT_total_rd_execution_time += tr->STAT_execution_time;
    STAT_total_rd_transfer_time  += tr->STAT_transfer_time;
    STAT_total_rd_waiting_time   += tr->STAT_waiting_time();
  }

  force_inline void
  Input_Stream_Base::accumulate_wr_transaction_time(NVM_Transaction* tr)
  {
    STAT_total_wr_execution_time += tr->STAT_execution_time;
    STAT_total_wr_transfer_time  += tr->STAT_transfer_time;
    STAT_total_wr_waiting_time   += tr->STAT_waiting_time();
  }

  IMPL_ISB_AVG_IO_OPERATION_TIME_BY_VAL(rd, execution)
  IMPL_ISB_AVG_IO_OPERATION_TIME_BY_VAL(rd, transfer)
  IMPL_ISB_AVG_IO_OPERATION_TIME_BY_VAL(rd, waiting)
  IMPL_ISB_AVG_IO_OPERATION_TIME_BY_FUNC(rd, turnaround)

  IMPL_ISB_AVG_IO_OPERATION_TIME_BY_VAL(wr, execution)
  IMPL_ISB_AVG_IO_OPERATION_TIME_BY_VAL(wr, transfer)
  IMPL_ISB_AVG_IO_OPERATION_TIME_BY_VAL(wr, waiting)
  IMPL_ISB_AVG_IO_OPERATION_TIME_BY_FUNC(wr, turnaround)

  // ====================================
  // Declare of Input_Stream_Manager_Base
  // ====================================

  #define DEFINE_ISMB_AVG_IO_OPERATION_TIME(IO, OP) \
  sim_time_type avg_##IO##_##OP##_time(stream_id_type stream_id)

  #define IMPL_ISMB_AVG_IO_OPERATION_TIME(IO, OP)                             \
  force_inline sim_time_type                                                  \
  Input_Stream_Manager_Base::avg_##IO##_##OP##_time(stream_id_type stream_id) \
  { return input_streams[stream_id]->STAT_avg_##IO##_##OP##_time(); }

  class Input_Stream_Manager_Base
  {
    friend class Request_Fetch_Unit_Base;
    friend class Request_Fetch_Unit_NVMe;
    friend class Request_Fetch_Unit_SATA;

  protected:
    Host_Interface_Base* host_interface;
    virtual void segment_user_request(UserRequest* user_request) = 0;
    std::vector<Input_Stream_Base*> input_streams;

  public:
    Input_Stream_Manager_Base(Host_Interface_Base* host_interface);
    virtual ~Input_Stream_Manager_Base();
    virtual void Handle_new_arrived_request(UserRequest* request) = 0;
    virtual void Handle_arrived_write_data(UserRequest* request) = 0;
    virtual void Handle_serviced_request(UserRequest* request) = 0;

    void Update_transaction_statistics(NVM_Transaction* transaction);

    DEFINE_ISMB_AVG_IO_OPERATION_TIME(rd, turnaround);
    DEFINE_ISMB_AVG_IO_OPERATION_TIME(rd, execution);
    DEFINE_ISMB_AVG_IO_OPERATION_TIME(rd, transfer);
    DEFINE_ISMB_AVG_IO_OPERATION_TIME(rd, waiting);
    DEFINE_ISMB_AVG_IO_OPERATION_TIME(wr, turnaround);
    DEFINE_ISMB_AVG_IO_OPERATION_TIME(wr, execution);
    DEFINE_ISMB_AVG_IO_OPERATION_TIME(wr, transfer);
    DEFINE_ISMB_AVG_IO_OPERATION_TIME(wr, waiting);

  };

  force_inline
  Input_Stream_Manager_Base::Input_Stream_Manager_Base(Host_Interface_Base* host_interface)
    : host_interface(host_interface)
  { }

  force_inline
  Input_Stream_Manager_Base::~Input_Stream_Manager_Base()
  {
    for (auto &stream : input_streams)
      delete stream;
  }

  force_inline void
  Input_Stream_Manager_Base::Update_transaction_statistics(NVM_Transaction* transaction)
  {
    switch (transaction->Type)
    {
    case Transaction_Type::READ:
      this->input_streams[transaction->Stream_id]->accumulate_rd_transaction_time(transaction);
      break;

    case Transaction_Type::WRITE:
      this->input_streams[transaction->Stream_id]->accumulate_wr_transaction_time(transaction);
      break;

    default:
      break;
    }
  }

  IMPL_ISMB_AVG_IO_OPERATION_TIME(rd, turnaround)
  IMPL_ISMB_AVG_IO_OPERATION_TIME(rd, execution)
  IMPL_ISMB_AVG_IO_OPERATION_TIME(rd, transfer)
  IMPL_ISMB_AVG_IO_OPERATION_TIME(rd, waiting)

  IMPL_ISMB_AVG_IO_OPERATION_TIME(wr, turnaround)
  IMPL_ISMB_AVG_IO_OPERATION_TIME(wr, execution)
  IMPL_ISMB_AVG_IO_OPERATION_TIME(wr, transfer)
  IMPL_ISMB_AVG_IO_OPERATION_TIME(wr, waiting)

  // ==================================
  // Declare of Request_Fetch_Unit_Base
  // ==================================
  class Request_Fetch_Unit_Base
  {
  protected:
    enum class DMA_Req_Type {
      REQUEST_INFO,
      WRITE_DATA
    };

    struct DMA_Req_Item
    {
      DMA_Req_Type Type;
      void * object;
    };

    UserReqPool _user_req_pool;
    Host_Interface_Base* host_interface;
    std::list<DMA_Req_Item*> dma_list;

  public:
    Request_Fetch_Unit_Base(Host_Interface_Base* host_interface);
    virtual ~Request_Fetch_Unit_Base();
    virtual void Fetch_next_request(stream_id_type stream_id) = 0;
    virtual void Fetch_write_data(UserRequest* request) = 0;
    virtual void Send_read_data(UserRequest* request) = 0;
    virtual void Process_pcie_write_message(uint64_t, void *, uint32_t) = 0;
    virtual void Process_pcie_read_message(uint64_t, void *, uint32_t) = 0;
  };

  force_inline
  Request_Fetch_Unit_Base::Request_Fetch_Unit_Base(Host_Interface_Base* host_interface)
    : _user_req_pool(),
      host_interface(host_interface)
  { }

  force_inline
  Request_Fetch_Unit_Base::~Request_Fetch_Unit_Base()
  {
    for (auto &dma_info : dma_list)
      delete dma_info;
  }

  // ==============================
  // Declare of Host_Interface_Base
  // ==============================
  class Host_Interface_Base : public MQSimEngine::Sim_Object
  {
    friend class Input_Stream_Manager_Base;
    friend class Input_Stream_Manager_NVMe;
    friend class Input_Stream_Manager_SATA;
    friend class Request_Fetch_Unit_Base;
    friend class Request_Fetch_Unit_NVMe;
    friend class Request_Fetch_Unit_SATA;

  private:
    const HostInterface_Types type;
    const LHA_type max_logical_sector_address;
    const uint32_t sectors_per_page;

    UserRequestServiceHandler<Host_Interface_Base> __user_request_handler;
    NvmTransactionHandler<Host_Interface_Base> __user_transaction_handler;

    UserRequestServiceHandlerList __connected_user_req_signal_handlers;

    Data_Cache_Manager_Base* cache;

    Host_Components::PCIe_Switch* __pcie_switch;

  protected:
    // TODO Check how to change this members private
    Input_Stream_Manager_Base* input_stream_manager;
    Request_Fetch_Unit_Base* request_fetch_unit;

  protected:
    void broadcast_user_request_arrival_signal(UserRequest* user_request);

    void __handle_user_request_signal_from_cache(UserRequest* request);
    void __handle_user_transaction_signal_from_cache(NVM_Transaction* transaction);

  public:
    Host_Interface_Base(const sim_object_id_type& id,
                        HostInterface_Types type,
                        LHA_type max_logical_sector_address,
                        uint32_t sectors_per_page,
                        Data_Cache_Manager_Base* cache);

    virtual ~Host_Interface_Base();
    void Setup_triggers();

    void connect_to_user_request_signal(UserRequestServiceHandlerBase& handler);

    void Consume_pcie_message(Host_Components::PCIe_Message* message);
    void Send_read_message_to_host(uint64_t address, uint32_t request_read_data_size);
    void Send_write_message_to_host(uint64_t address, void* message, uint32_t message_size);

    HostInterface_Types GetType() const;
    LHA_type Get_max_logical_sector_address() const;
    uint32_t Get_no_of_LHAs_in_an_NVM_write_unit() const;

    void Attach_to_device(Host_Components::PCIe_Switch* pcie_switch);
  };

  force_inline void
  Host_Interface_Base::broadcast_user_request_arrival_signal(UserRequest* user_request)
  {
    for (auto& handler : __connected_user_req_signal_handlers)
      (*handler)(user_request);
  }

  force_inline void
  Host_Interface_Base::connect_to_user_request_signal(UserRequestServiceHandlerBase& handler)
  {
    __connected_user_req_signal_handlers.emplace_back(&handler);
  }

  force_inline void
  Host_Interface_Base::Consume_pcie_message(Host_Components::PCIe_Message* message)
  {
    if (message->Type == Host_Components::PCIe_Message_Type::READ_COMP)
      request_fetch_unit->Process_pcie_read_message(message->Address, message->Payload, message->Payload_size);
    else
      request_fetch_unit->Process_pcie_write_message(message->Address, message->Payload, message->Payload_size);

    delete message;
  }

  force_inline HostInterface_Types
  Host_Interface_Base::GetType() const
  {
    return type;
  }

  force_inline LHA_type
  Host_Interface_Base::Get_max_logical_sector_address() const
  {
    return max_logical_sector_address;
  }

  force_inline uint32_t
  Host_Interface_Base::Get_no_of_LHAs_in_an_NVM_write_unit() const
  {
    return sectors_per_page;
  }

  force_inline void
  Host_Interface_Base::Attach_to_device(Host_Components::PCIe_Switch* pcie_switch)
  {
    this->__pcie_switch = pcie_switch;
  }

}
#endif //HOST_INTERFACE_BASE_H