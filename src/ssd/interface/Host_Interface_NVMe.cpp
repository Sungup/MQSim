#include <stdexcept>
#include "../../sim/Engine.h"
#include "Host_Interface_NVMe.h"
#include "../NvmTransactionFlashRD.h"
#include "../NvmTransactionFlashWR.h"
#include "../../host/Host_Defs.h"


namespace SSD_Components
{
  Input_Stream_Manager_NVMe::Input_Stream_Manager_NVMe(Host_Interface_Base* host_interface, uint16_t queue_fetch_szie) :
    Input_Stream_Manager_Base(host_interface), Queue_fetch_size(queue_fetch_szie)
  {}

  stream_id_type Input_Stream_Manager_NVMe::Create_new_stream(IO_Flow_Priority_Class priority_class, LHA_type start_logical_sector_address, LHA_type end_logical_sector_address,
    uint64_t submission_queue_base_address, uint16_t submission_queue_size,  uint64_t completion_queue_base_address, uint16_t completion_queue_size)
  {
    if(end_logical_sector_address < start_logical_sector_address)
      PRINT_ERROR("Error in allocating address range to a stream in host interface: the start address should be smaller than the end address.")
    Input_Stream_NVMe* input_stream = new Input_Stream_NVMe(priority_class, start_logical_sector_address, end_logical_sector_address, 
      submission_queue_base_address, submission_queue_size, completion_queue_base_address, completion_queue_size);
    this->input_streams.push_back(input_stream);
    return (stream_id_type)(this->input_streams.size() - 1);
  }

  inline void Input_Stream_Manager_NVMe::Submission_queue_tail_pointer_update(stream_id_type stream_id, uint16_t tail_pointer_value)
  {
    ((Input_Stream_NVMe*)input_streams[stream_id])->Submission_tail = tail_pointer_value;

    if (((Input_Stream_NVMe*)input_streams[stream_id])->On_the_fly_requests < Queue_fetch_size)
    {
      ((Host_Interface_NVMe*)host_interface)->request_fetch_unit->Fetch_next_request(stream_id);
      ((Input_Stream_NVMe*)input_streams[stream_id])->On_the_fly_requests++;
      ((Input_Stream_NVMe*)input_streams[stream_id])->Submission_head++;//Update submission queue head after starting fetch request
      if (((Input_Stream_NVMe*)input_streams[stream_id])->Submission_head == ((Input_Stream_NVMe*)input_streams[stream_id])->Submission_queue_size)//Circular queue implementation
        ((Input_Stream_NVMe*)input_streams[stream_id])->Submission_head = 0;
    }
  }

  inline void Input_Stream_Manager_NVMe::Completion_queue_head_pointer_update(stream_id_type stream_id, uint16_t head_pointer_value)
  {
    ((Input_Stream_NVMe*)input_streams[stream_id])->Completion_head = head_pointer_value;

    if (((Input_Stream_NVMe*)input_streams[stream_id])->Completed_user_requests.size() > 0)//If this check is true, then the host interface couldn't send the completion queue entry, since the completion queue was full
    {
      UserRequest* request = ((Input_Stream_NVMe*)input_streams[stream_id])->Completed_user_requests.front();
      ((Input_Stream_NVMe*)input_streams[stream_id])->Completed_user_requests.pop_front();
      inform_host_request_completed(stream_id, request);
    }
  }

  inline void Input_Stream_Manager_NVMe::Handle_new_arrived_request(UserRequest* request)
  {
    auto* stream = (Input_Stream_NVMe*)input_streams[request->Stream_id];

    stream->Submission_head_informed_to_host++;

    if (stream->Submission_head_informed_to_host == stream->Submission_queue_size)//Circular queue implementation
      stream->Submission_head_informed_to_host = 0;

    if (request->Type == UserRequestType::READ)
    {
      stream->Waiting_user_requests.push_back(request);
      stream->STAT_rd_requests++;
      segment_user_request(request);

      ((Host_Interface_NVMe*)host_interface)->broadcast_user_request_arrival_signal(request);
    }
    else//This is a write request
    {
      stream->Waiting_user_requests.push_back(request);
      stream->STAT_wr_requests++;
      ((Host_Interface_NVMe*)host_interface)->request_fetch_unit->Fetch_write_data(request);
    }
  }

  inline void Input_Stream_Manager_NVMe::Handle_arrived_write_data(UserRequest* request)
  {
    segment_user_request(request);
    ((Host_Interface_NVMe*)host_interface)->broadcast_user_request_arrival_signal(request);
  }

  inline void Input_Stream_Manager_NVMe::Handle_serviced_request(UserRequest* request)
  {
    stream_id_type stream_id = request->Stream_id;
    auto* stream = (Input_Stream_NVMe*)input_streams[stream_id];

    stream->Waiting_user_requests.remove(request);
    stream->On_the_fly_requests--;

    PRINT_DEBUG("** Host Interface: Request #" << request->ID << " from stream #" << stream_id << " is finished")

    if (request->Type == UserRequestType::READ)//If this is a read request, then the read data should be written to host memory
      ((Host_Interface_NVMe*)host_interface)->request_fetch_unit->Send_read_data(request);

    if (stream->Submission_head != stream->Submission_tail)//there are waiting requests in the submission queue but have not been fetched, due to Queue_fetch_size limit
    {
      ((Host_Interface_NVMe*)host_interface)->request_fetch_unit->Fetch_next_request(stream_id);
      stream->On_the_fly_requests++;
      stream->Submission_head++;//Update submission queue head after starting fetch request
      if (stream->Submission_head == stream->Submission_queue_size)//Circular queue implementation
        stream->Submission_head = 0;
    }

    if (stream->Completion_head > stream->Completion_tail)//Check if completion queue is full
    {
      if (stream->Completion_tail + 1 == stream->Completion_head)//completion queue is full
      {
        stream->Completed_user_requests.push_back(request);//Wait while the completion queue is full
        return;
      }
    }
    else if(stream->Completion_tail - stream->Completion_head == stream->Completion_queue_size -1)
    {
      stream->Completed_user_requests.push_back(request);//Wait while the completion queue is full
      return;
    }

    inform_host_request_completed(stream_id, request);//Completion queue is not full, so the device can DMA the completion queue entry to the host
    delete_request_nvme(request);
  }
  
  uint16_t Input_Stream_Manager_NVMe::Get_submission_queue_depth(stream_id_type stream_id)
  {
    return ((Input_Stream_NVMe*)this->input_streams[stream_id])->Submission_queue_size;
  }
  
  uint16_t Input_Stream_Manager_NVMe::Get_completion_queue_depth(stream_id_type stream_id)
  {
    return ((Input_Stream_NVMe*)this->input_streams[stream_id])->Completion_queue_size;
  }

  IO_Flow_Priority_Class Input_Stream_Manager_NVMe::Get_priority_class(stream_id_type stream_id)
  {
    return ((Input_Stream_NVMe*)this->input_streams[stream_id])->Priority_class;
  }

  inline void Input_Stream_Manager_NVMe::inform_host_request_completed(stream_id_type stream_id, UserRequest* request)
  {
    auto* stream = (Input_Stream_NVMe*)input_streams[stream_id];

    ((Request_Fetch_Unit_NVMe*)((Host_Interface_NVMe*)host_interface)->request_fetch_unit)->Send_completion_queue_element(request, stream->Submission_head_informed_to_host);
    stream->Completion_tail++;//Next free slot in the completion queue
    if (stream->Completion_tail == stream->Completion_queue_size)//Circular queue implementation
      stream->Completion_tail = 0;
  }
  
  void Input_Stream_Manager_NVMe::segment_user_request(UserRequest* user_request)
  {
    LHA_type lsa = user_request->Start_LBA;
    LHA_type lsa2 = user_request->Start_LBA;
    uint32_t req_size = user_request->SizeInSectors;

    page_status_type access_status_bitmap = 0;
    uint32_t hanled_sectors_count = 0;
    uint32_t transaction_size = 0;

    auto* stream = (Input_Stream_NVMe*)input_streams[user_request->Stream_id];
    while (hanled_sectors_count < req_size)
    {      
      //Check if LSA is in the correct range allocted to the stream
      if (lsa < stream->Start_logical_sector_address || lsa >stream->End_logical_sector_address)
        lsa = stream->Start_logical_sector_address + (lsa % (stream->End_logical_sector_address - (stream->Start_logical_sector_address)));
      LHA_type internal_lsa = lsa - stream->Start_logical_sector_address;//For each flow, all lsa's should be translated into a range starting from zero

      transaction_size = host_interface->sectors_per_page - (uint32_t)(lsa % host_interface->sectors_per_page);
      if (hanled_sectors_count + transaction_size >= req_size)
      {
        transaction_size = req_size - hanled_sectors_count;
      }
      LPA_type lpa = internal_lsa / host_interface->sectors_per_page;

      page_status_type temp = ~(0xffffffffffffffff << (uint32_t)transaction_size);
      access_status_bitmap = temp << (uint32_t)(internal_lsa % host_interface->sectors_per_page);

      if (user_request->Type == UserRequestType::READ)
        _make_read_tr(user_request, user_request->Stream_id, transaction_size,
                      access_status_bitmap, lpa);
      else
        _make_write_tr(user_request, user_request->Stream_id, transaction_size,
                       access_status_bitmap, lpa);

      lsa = lsa + transaction_size;
      hanled_sectors_count += transaction_size;      
    }
  }

  Request_Fetch_Unit_NVMe::Request_Fetch_Unit_NVMe(Host_Interface_Base* host_interface) :
    Request_Fetch_Unit_Base(host_interface), current_phase(0xffff), number_of_sent_cqe(0) {}
  
  void Request_Fetch_Unit_NVMe::Process_pcie_write_message(uint64_t address, void * payload, uint32_t /* payload_size */)
  {
    auto* hi = (Host_Interface_NVMe*)host_interface;
    auto  val = (uint64_t)payload;

    auto stream_id = register_addr_to_stream_id(address);

    if (!is_valid_nvme_stream_id(stream_id))
      throw std::invalid_argument("Unknown register is written!");

    if (is_cq_address(address))
      ((Input_Stream_Manager_NVMe*)(hi->input_stream_manager))->Completion_queue_head_pointer_update(stream_id, (uint16_t)val);
    else
      ((Input_Stream_Manager_NVMe*)(hi->input_stream_manager))->Submission_queue_tail_pointer_update(stream_id, (uint16_t)val);
  }
  
  void Request_Fetch_Unit_NVMe::Process_pcie_read_message(uint64_t /* address */, void * payload, uint32_t payload_size)
  {
    auto* hi = (Host_Interface_NVMe*)host_interface;
    DMA_Req_Item* dma_req_item = dma_list.front();
    dma_list.pop_front();

    switch (dma_req_item->Type)
    {
    case DMA_Req_Type::REQUEST_INFO:
    {
      auto* new_reqeust = _user_req_pool.construct();
      new_reqeust->IO_command_info = payload;
      new_reqeust->Stream_id = (stream_id_type)((uint64_t)(dma_req_item->object));
      new_reqeust->Priority_class = ((Input_Stream_Manager_NVMe*)host_interface->input_stream_manager)->Get_priority_class(new_reqeust->Stream_id);
      new_reqeust->STAT_InitiationTime = Simulator->Time();
      auto* sqe = (SQEntry*)payload;
      switch (sqe->Opcode)
      {
      case NVME_READ_OPCODE:
        new_reqeust->Type = UserRequestType::READ;
        new_reqeust->Start_LBA = ((LHA_type)sqe->Command_specific[1]) << 31U | (LHA_type)sqe->Command_specific[0];//Command Dword 10 and Command Dword 11
        new_reqeust->SizeInSectors = sqe->Command_specific[2] & (LHA_type)(0x0000ffff);
        new_reqeust->Size_in_byte = new_reqeust->SizeInSectors * SECTOR_SIZE_IN_BYTE;
        break;
      case NVME_WRITE_OPCODE:
        new_reqeust->Type = UserRequestType::WRITE;
        new_reqeust->Start_LBA = ((LHA_type)sqe->Command_specific[1]) << 31U | (LHA_type)sqe->Command_specific[0];//Command Dword 10 and Command Dword 11
        new_reqeust->SizeInSectors = sqe->Command_specific[2] & (LHA_type)(0x0000ffff);
        new_reqeust->Size_in_byte = new_reqeust->SizeInSectors * SECTOR_SIZE_IN_BYTE;
        break;
      default:
        throw std::invalid_argument("NVMe command is not supported!");
      }
      ((Input_Stream_Manager_NVMe*)(hi->input_stream_manager))->Handle_new_arrived_request(new_reqeust);
      break;
    }
    case DMA_Req_Type::WRITE_DATA:
      ((UserRequest*)dma_req_item->object)->assign_data(payload, payload_size);
      ((Input_Stream_Manager_NVMe*)(hi->input_stream_manager))->Handle_arrived_write_data((UserRequest*)dma_req_item->object);
      break;
    default:
      break;
    }
    delete dma_req_item;
  }

  void Request_Fetch_Unit_NVMe::Fetch_next_request(stream_id_type stream_id)
  {
    auto* dma_req_item = new DMA_Req_Item;
    dma_req_item->Type = DMA_Req_Type::REQUEST_INFO;
    dma_req_item->object = (void *)(intptr_t)stream_id;
    dma_list.push_back(dma_req_item);

    auto* hi = (Host_Interface_NVMe*)host_interface;
    Input_Stream_NVMe* im = ((Input_Stream_NVMe*)hi->input_stream_manager->input_streams[stream_id]);
    host_interface->Send_read_message_to_host(im->Submission_queue_base_address + im->Submission_head *
                                                                                    SQEntry::size(),
                                              SQEntry::size());
  }

  void Request_Fetch_Unit_NVMe::Fetch_write_data(UserRequest* request)
  {
    auto* dma_req_item = new DMA_Req_Item;
    dma_req_item->Type = DMA_Req_Type::WRITE_DATA;
    dma_req_item->object = (void *)request;
    dma_list.push_back(dma_req_item);

    auto* sqe = (SQEntry*) request->IO_command_info;
    host_interface->Send_read_message_to_host((sqe->PRP_entry_2<<31U) | sqe->PRP_entry_1, request->Size_in_byte);
  }

  void Request_Fetch_Unit_NVMe::Send_completion_queue_element(UserRequest* request, uint16_t sq_head_value)
  {
    auto* hi = (Host_Interface_NVMe*)host_interface;
    auto* cqe = _cq_entry_pool.construct(sq_head_value,
                                         FLOW_ID_TO_Q_ID(request->Stream_id),
                                         0x0001U & current_phase,
                                         ((SQEntry*)request->IO_command_info)->Command_Identifier);

    Input_Stream_NVMe* im = ((Input_Stream_NVMe*)hi->input_stream_manager->input_streams[request->Stream_id]);
    host_interface->Send_write_message_to_host(im->Completion_queue_base_address + im->Completion_tail *
                                                                                     CQEntry::size(), cqe,
                                               CQEntry::size());
    number_of_sent_cqe++;
    if (number_of_sent_cqe % im->Completion_queue_size == 0)
    {
      if (current_phase == 0xffff)//According to protocol specification, the value of the Phase Tag is inverted each pass through the Completion Queue
        current_phase = 0xfffe;
      else
        current_phase = 0xffff;
    }
  }

  void Request_Fetch_Unit_NVMe::Send_read_data(UserRequest* request)
  {
    auto* sqe = (SQEntry*)request->IO_command_info;
    host_interface->Send_write_message_to_host(sqe->PRP_entry_1, request->Data, request->Size_in_byte);
  }

  Host_Interface_NVMe::Host_Interface_NVMe(const sim_object_id_type& id,
                                           LHA_type max_logical_sector_address,
                                           uint16_t submission_queue_depth,
                                           uint16_t completion_queue_depth,
                                           uint32_t no_of_input_streams,
                                           uint16_t queue_fetch_size,
                                           uint32_t sectors_per_page,
                                           Data_Cache_Manager_Base* cache)
    : Host_Interface_Base(id,
                          HostInterface_Types::NVME,
                          max_logical_sector_address,
                          sectors_per_page,
                          cache),
      submission_queue_depth(submission_queue_depth),
      completion_queue_depth(completion_queue_depth),
      no_of_input_streams(no_of_input_streams)
  {
    this->input_stream_manager = new Input_Stream_Manager_NVMe(this, queue_fetch_size);
    this->request_fetch_unit = new Request_Fetch_Unit_NVMe(this);
  }

  stream_id_type Host_Interface_NVMe::Create_new_stream(IO_Flow_Priority_Class priority_class,
                                                        LHA_type start_logical_sector_address,
                                                        LHA_type end_logical_sector_address,
                                                        uint64_t submission_queue_base_address,
                                                        uint64_t completion_queue_base_address)
  {
    return ((Input_Stream_Manager_NVMe*)input_stream_manager)->Create_new_stream(priority_class, start_logical_sector_address, end_logical_sector_address,
      submission_queue_base_address, submission_queue_depth, completion_queue_base_address, completion_queue_depth);
  }

  void Host_Interface_NVMe::Validate_simulation_config()
  {
    Host_Interface_Base::Validate_simulation_config();
    if (this->input_stream_manager == nullptr)
      throw std::logic_error("Input stream manager is not set for Host Interface");
    if (this->request_fetch_unit == nullptr)
      throw std::logic_error("Request fetch unit is not set for Host Interface");
  }
  
  void Host_Interface_NVMe::Start_simulation() {}

  void Host_Interface_NVMe::Execute_simulator_event(MQSimEngine::SimEvent* /* event */) {}

  uint16_t Host_Interface_NVMe::Get_submission_queue_depth()
  {
    return submission_queue_depth;
  }

  uint16_t Host_Interface_NVMe::Get_completion_queue_depth()
  {
    return completion_queue_depth;
  }

  void Host_Interface_NVMe::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
  {
    std::string tmp = name_prefix + ".HostInterface";
    xmlwriter.Write_open_tag(tmp);

    std::string attr = "Name";
    std::string val = ID();
    xmlwriter.Write_attribute_string(attr, val);


    for (uint32_t stream_id = 0; stream_id < no_of_input_streams; stream_id++)
    {
      tmp = name_prefix + ".IO_Stream";
      xmlwriter.Write_open_tag(tmp);

      attr = "Stream_ID";
      val = std::to_string(stream_id);
      xmlwriter.Write_attribute_string(attr, val);

      attr = "Average_Read_Transaction_Turnaround_Time";
      val = std::to_string(
        input_stream_manager->avg_rd_turnaround_time(stream_id));
      xmlwriter.Write_attribute_string(attr, val);

      attr = "Average_Read_Transaction_Execution_Time";
      val = std::to_string(
        input_stream_manager->avg_rd_execution_time(stream_id));
      xmlwriter.Write_attribute_string(attr, val);

      attr = "Average_Read_Transaction_Transfer_Time";
      val = std::to_string(
        input_stream_manager->avg_rd_transfer_time(stream_id));
      xmlwriter.Write_attribute_string(attr, val);

      attr = "Average_Read_Transaction_Waiting_Time";
      val = std::to_string(input_stream_manager->avg_rd_waiting_time(stream_id));
      xmlwriter.Write_attribute_string(attr, val);

      attr = "Average_Write_Transaction_Turnaround_Time";
      val = std::to_string(
        input_stream_manager->avg_wr_turnaround_time(stream_id));
      xmlwriter.Write_attribute_string(attr, val);

      attr = "Average_Write_Transaction_Execution_Time";
      val = std::to_string(
        input_stream_manager->avg_wr_execution_time(stream_id));
      xmlwriter.Write_attribute_string(attr, val);

      attr = "Average_Write_Transaction_Transfer_Time";
      val = std::to_string(input_stream_manager->avg_wr_transfer_time(stream_id));
      xmlwriter.Write_attribute_string(attr, val);

      attr = "Average_Write_Transaction_Waiting_Time";
      val = std::to_string(input_stream_manager->avg_wr_waiting_time(stream_id));
      xmlwriter.Write_attribute_string(attr, val);

      xmlwriter.Write_close_tag();
    }

    xmlwriter.Write_close_tag();
  }
}