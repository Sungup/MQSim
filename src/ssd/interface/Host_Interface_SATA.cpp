#include "Host_Interface_SATA.h"

namespace SSD_Components
{
  Input_Stream_Manager_SATA::Input_Stream_Manager_SATA(Host_Interface_Base* host_interface, uint16_t ncq_depth,
    LHA_type start_logical_sector_address, LHA_type end_logical_sector_address) :
    ncq_depth(ncq_depth), Input_Stream_Manager_Base(host_interface)
  {
    if (end_logical_sector_address < start_logical_sector_address)
      PRINT_ERROR("Error in allocating address range to a stream in host interface: the start address should be smaller than the end address.")

    auto* input_stream = new Input_Stream_SATA(start_logical_sector_address, end_logical_sector_address, 0, 0);
    this->input_streams.push_back(input_stream);
  }

  void Input_Stream_Manager_SATA::Set_ncq_address(uint64_t submission_queue_base_address, uint64_t completion_queue_base_address)
  {
    ((Input_Stream_SATA*)this->input_streams[SATA_STREAM_ID])->Submission_queue_base_address = submission_queue_base_address;
    ((Input_Stream_SATA*)this->input_streams[SATA_STREAM_ID])->Completion_queue_base_address = completion_queue_base_address;
  }

  inline void Input_Stream_Manager_SATA::Submission_queue_tail_pointer_update(uint16_t tail_pointer_value)
  {
    ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_tail = tail_pointer_value;
    ((Host_Interface_SATA*)host_interface)->request_fetch_unit->Fetch_next_request(SATA_STREAM_ID);
    ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->On_the_fly_requests++;
    ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head++;
    if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head == ncq_depth)
      ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head = 0;
  }

  inline void Input_Stream_Manager_SATA::Completion_queue_head_pointer_update(uint16_t head_pointer_value)
  {
    ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_head = head_pointer_value;

    if (!((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completed_user_requests.empty())
    {
      UserRequest* request = ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completed_user_requests.front();
      ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completed_user_requests.pop_front();
      inform_host_request_completed(request);
    }
  }

  inline void Input_Stream_Manager_SATA::Handle_new_arrived_request(UserRequest* request)
  {
    ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head_informed_to_host++;
    if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head_informed_to_host == ncq_depth)
      ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head_informed_to_host = 0;
    if (request->Type == UserRequestType::READ)
    {
      ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Waiting_user_requests.push_back(request);
      ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->STAT_rd_requests++;
      segment_user_request(request);

      ((Host_Interface_SATA*)host_interface)->broadcast_user_request_arrival_signal(request);
    }
    else//This is a write request
    {
      ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Waiting_user_requests.push_back(request);
      ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->STAT_wr_requests++;
      ((Host_Interface_SATA*)host_interface)->request_fetch_unit->Fetch_write_data(request);
    }
  }

  inline void Input_Stream_Manager_SATA::Handle_arrived_write_data(UserRequest* request)
  {
    segment_user_request(request);
    ((Host_Interface_SATA*)host_interface)->broadcast_user_request_arrival_signal(request);
  }

  inline void Input_Stream_Manager_SATA::Handle_serviced_request(UserRequest* request)
  {
    ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Waiting_user_requests.remove(request);
    ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->On_the_fly_requests--;

    PRINT_DEBUG("** Host Interface: Request #" << request->ID << " is finished")

      if (request->Type == UserRequestType::READ)//If this is a read request, then the read data should be written to host memory
        ((Host_Interface_SATA*)host_interface)->request_fetch_unit->Send_read_data(request);

    if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head != ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_tail)
    {
      ((Host_Interface_SATA*)host_interface)->request_fetch_unit->Fetch_next_request(SATA_STREAM_ID);
      ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->On_the_fly_requests++;
      ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head++;//Update submission queue head after starting fetch request
      if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head == ncq_depth)//Circular queue implementation
        ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head = 0;
    }

    if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_head > ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail)//Check if completion queue is full
    {
      if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail + 1 == ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_head)//completion queue is full
      {
        ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completed_user_requests.push_back(request);//Wait while the completion queue is full
        return;
      }
    }
    else if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail - ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_head
      == ncq_depth - 1)
    {
      ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completed_user_requests.push_back(request);//Wait while the completion queue is full
      return;
    }

    inform_host_request_completed(request);//Completion queue is not full, so the device can DMA the completion queue entry to the host
    delete_request_nvme(request);
  }

  inline void Input_Stream_Manager_SATA::inform_host_request_completed(UserRequest* request)
  {
    ((Request_Fetch_Unit_SATA*)((Host_Interface_SATA*)host_interface)->request_fetch_unit)->Send_completion_queue_element(request, ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Submission_head_informed_to_host);
    ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail++;//Next free slot in the completion queue
    if (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail == ncq_depth)//Circular queue implementation
      ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Completion_tail = 0;
  }

  void Input_Stream_Manager_SATA::segment_user_request(UserRequest* user_request)
  {
    LHA_type lsa = user_request->Start_LBA;
    LHA_type lsa2 = user_request->Start_LBA;
    uint32_t req_size = user_request->SizeInSectors;

    page_status_type access_status_bitmap = 0;
    uint32_t hanled_sectors_count = 0;
    uint32_t transaction_size = 0;
    while (hanled_sectors_count < req_size)
    {
      //Check if LSA is in the correct range allocted to the stream
      if (lsa < ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Start_logical_sector_address || lsa >((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->End_logical_sector_address)
        lsa = ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Start_logical_sector_address
                + (lsa % (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->End_logical_sector_address - (((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Start_logical_sector_address)));
      LHA_type internal_lsa = lsa - ((Input_Stream_SATA*)input_streams[SATA_STREAM_ID])->Start_logical_sector_address;//For each flow, all lsa's should be translated into a range starting from zero


      transaction_size = host_interface->sectors_per_page - (uint32_t)(lsa % host_interface->sectors_per_page);
      if (hanled_sectors_count + transaction_size >= req_size)
      {
        transaction_size = req_size - hanled_sectors_count;
      }
      LPA_type lpa = internal_lsa / host_interface->sectors_per_page;

      page_status_type temp = ~(0xffffffffffffffff << (uint32_t)transaction_size);
      access_status_bitmap = temp << (uint32_t)(internal_lsa % host_interface->sectors_per_page);

      if (user_request->Type == UserRequestType::READ)
        _make_read_tr(user_request, SATA_STREAM_ID, transaction_size,
                      access_status_bitmap, lpa);
      else
        _make_write_tr(user_request, SATA_STREAM_ID, transaction_size,
                       access_status_bitmap, lpa);

      lsa = lsa + transaction_size;
      hanled_sectors_count += transaction_size;
    }
  }

  Request_Fetch_Unit_SATA::Request_Fetch_Unit_SATA(Host_Interface_Base* host_interface, uint16_t ncq_depth) :
    Request_Fetch_Unit_Base(host_interface), current_phase(0xffff), number_of_sent_cqe(0), ncq_depth(ncq_depth) {}

  void Request_Fetch_Unit_SATA::Process_pcie_write_message(uint64_t address, void * payload, uint32_t /* payload_size */)
  {
    auto* hi = (Host_Interface_SATA*)host_interface;
    auto val = (uint64_t)payload;
    switch (address)
    {
    case NCQ_SUBMISSION_REGISTER:
      ((Input_Stream_Manager_SATA*)(hi->input_stream_manager))->Submission_queue_tail_pointer_update((uint16_t)val);
      break;
    case NCQ_COMPLETION_REGISTER:
      ((Input_Stream_Manager_SATA*)(hi->input_stream_manager))->Completion_queue_head_pointer_update((uint16_t)val);
      break;
    default:
      throw std::invalid_argument("Unknown register is written in Request_Fetch_Unit_SATA!");
    }
  }

  void Request_Fetch_Unit_SATA::Process_pcie_read_message(uint64_t /* address */, void * payload, uint32_t payload_size)
  {
    auto* hi = (Host_Interface_SATA*)host_interface;
    DMA_Req_Item* dma_req_item = dma_list.front();
    dma_list.pop_front();

    switch (dma_req_item->Type)
    {
    case DMA_Req_Type::REQUEST_INFO:
    {
      auto* new_reqeust = _user_req_pool.construct();
      new_reqeust->IO_command_info = payload;
      new_reqeust->Stream_id = (stream_id_type)((uint64_t)(dma_req_item->object));
      new_reqeust->STAT_InitiationTime = Simulator->Time();
      auto* sqe = (SubmissionQueueEntry*)payload;
      switch (sqe->Opcode)
      {
      case SATA_READ_OPCODE:
        new_reqeust->Type = UserRequestType::READ;
        new_reqeust->Start_LBA = ((LHA_type)sqe->Command_specific[1]) << 31U | (LHA_type)sqe->Command_specific[0];//Command Dword 10 and Command Dword 11
        new_reqeust->SizeInSectors = sqe->Command_specific[2] & (LHA_type)(0x0000ffff);
        new_reqeust->Size_in_byte = new_reqeust->SizeInSectors * SECTOR_SIZE_IN_BYTE;
        break;
      case SATA_WRITE_OPCODE:
        new_reqeust->Type = UserRequestType::WRITE;
        new_reqeust->Start_LBA = ((LHA_type)sqe->Command_specific[1]) << 31U | (LHA_type)sqe->Command_specific[0];//Command Dword 10 and Command Dword 11
        new_reqeust->SizeInSectors = sqe->Command_specific[2] & (LHA_type)(0x0000ffff);
        new_reqeust->Size_in_byte = new_reqeust->SizeInSectors * SECTOR_SIZE_IN_BYTE;
        break;
      default:
        throw std::invalid_argument("SATA command is not supported!");
      }
      ((Input_Stream_Manager_SATA*)(hi->input_stream_manager))->Handle_new_arrived_request(new_reqeust);
      break;
    }
    case DMA_Req_Type::WRITE_DATA:
      ((UserRequest*)dma_req_item->object)->assign_data(payload, payload_size);
      ((Input_Stream_Manager_SATA*)(hi->input_stream_manager))->Handle_arrived_write_data((UserRequest*)dma_req_item->object);
      break;
    default:
      break;
    }
    delete dma_req_item;
  }

  void Request_Fetch_Unit_SATA::Fetch_next_request(stream_id_type /* stream_id */)
  {
    auto* dma_req_item = new DMA_Req_Item;
    dma_req_item->Type = DMA_Req_Type::REQUEST_INFO;
    dma_req_item->object = (void *)SATA_STREAM_ID;
    dma_list.push_back(dma_req_item);

    auto* hi = (Host_Interface_SATA*)host_interface;
    Input_Stream_SATA* im = ((Input_Stream_SATA*)hi->input_stream_manager->input_streams[SATA_STREAM_ID]);
    host_interface->Send_read_message_to_host(im->Submission_queue_base_address + im->Submission_head * sizeof(SubmissionQueueEntry), sizeof(SubmissionQueueEntry));
  }

  void Request_Fetch_Unit_SATA::Fetch_write_data(UserRequest* request)
  {
    auto* dma_req_item = new DMA_Req_Item;
    dma_req_item->Type = DMA_Req_Type::WRITE_DATA;
    dma_req_item->object = (void *)request;
    dma_list.push_back(dma_req_item);

    auto* sqe = (SubmissionQueueEntry*)request->IO_command_info;
    host_interface->Send_read_message_to_host((sqe->PRP_entry_2 << 31U) | sqe->PRP_entry_1, request->Size_in_byte);
  }

  void Request_Fetch_Unit_SATA::Send_completion_queue_element(UserRequest* request, uint16_t sq_head_value)
  {
    auto* hi = (Host_Interface_SATA*)host_interface;
    auto* cqe = new CompletionQueueEntry;
    cqe->SQ_Head = sq_head_value;
    cqe->SQ_ID = FLOW_ID_TO_Q_ID(SATA_STREAM_ID);
    cqe->SF_P = 0x0001U & current_phase;
    cqe->Command_Identifier = ((SubmissionQueueEntry*)request->IO_command_info)->Command_Identifier;
    Input_Stream_SATA* im = ((Input_Stream_SATA*)hi->input_stream_manager->input_streams[SATA_STREAM_ID]);
    host_interface->Send_write_message_to_host(im->Completion_queue_base_address + im->Completion_tail * sizeof(CompletionQueueEntry), cqe, sizeof(CompletionQueueEntry));
    number_of_sent_cqe++;
    if (number_of_sent_cqe % ncq_depth == 0)
    {
      if (current_phase == 0xffff)//According to protocol specification, the value of the Phase Tag is inverted each pass through the Completion Queue
        current_phase = 0xfffe;
      else
        current_phase = 0xffff;
    }
  }

  void Request_Fetch_Unit_SATA::Send_read_data(UserRequest* request)
  {
    auto* sqe = (SubmissionQueueEntry*)request->IO_command_info;
    host_interface->Send_write_message_to_host(sqe->PRP_entry_1, request->Data, request->Size_in_byte);
  }

  Host_Interface_SATA::Host_Interface_SATA(const sim_object_id_type& id,
    const uint16_t ncq_depth, const LHA_type max_logical_sector_address, const uint32_t sectors_per_page, Data_Cache_Manager_Base* cache) :
    Host_Interface_Base(id, HostInterface_Types::SATA, max_logical_sector_address, sectors_per_page, cache), ncq_depth(ncq_depth)
  {
    this->input_stream_manager = new Input_Stream_Manager_SATA(this, ncq_depth, 0, max_logical_sector_address);
    this->request_fetch_unit = new Request_Fetch_Unit_SATA(this, ncq_depth);
  }

  void Host_Interface_SATA::Set_ncq_address(const uint64_t submission_queue_base_address, const uint64_t completion_queue_base_address)
  {
    ((Input_Stream_Manager_SATA*)this->input_stream_manager)->Set_ncq_address(submission_queue_base_address, completion_queue_base_address);
  }

  void Host_Interface_SATA::Validate_simulation_config()
  {
    Host_Interface_Base::Validate_simulation_config();
    if (this->input_stream_manager == nullptr)
      throw std::logic_error("Input stream manager is not set for Host Interface");
    if (this->request_fetch_unit == nullptr)
      throw std::logic_error("Request fetch unit is not set for Host Interface");
  }

  void Host_Interface_SATA::Start_simulation() {}

  void Host_Interface_SATA::Execute_simulator_event(MQSimEngine::SimEvent* /* event */) {}

  void Host_Interface_SATA::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
  {
    std::string tmp = name_prefix + ".HostInterface";
    xmlwriter.Write_open_tag(tmp);

    std::string attr = "Name";
    std::string val = ID();
    xmlwriter.Write_attribute_string(attr, val);

    attr = "Average_Read_Transaction_Turnaround_Time";
    val = std::to_string(
      input_stream_manager->avg_rd_turnaround_time(SATA_STREAM_ID));
    xmlwriter.Write_attribute_string(attr, val);

    attr = "Average_Read_Transaction_Execution_Time";
    val = std::to_string(
      input_stream_manager->avg_rd_execution_time(SATA_STREAM_ID));
    xmlwriter.Write_attribute_string(attr, val);

    attr = "Average_Read_Transaction_Transfer_Time";
    val = std::to_string(
      input_stream_manager->avg_rd_transfer_time(SATA_STREAM_ID));
    xmlwriter.Write_attribute_string(attr, val);

    attr = "Average_Read_Transaction_Waiting_Time";
    val = std::to_string(
      input_stream_manager->avg_rd_waiting_time(SATA_STREAM_ID));
    xmlwriter.Write_attribute_string(attr, val);

    attr = "Average_Write_Transaction_Turnaround_Time";
    val = std::to_string(
      input_stream_manager->avg_wr_turnaround_time(SATA_STREAM_ID));
    xmlwriter.Write_attribute_string(attr, val);

    attr = "Average_Write_Transaction_Execution_Time";
    val = std::to_string(
      input_stream_manager->avg_wr_execution_time(SATA_STREAM_ID));
    xmlwriter.Write_attribute_string(attr, val);

    attr = "Average_Write_Transaction_Transfer_Time";
    val = std::to_string(
      input_stream_manager->avg_wr_transfer_time(SATA_STREAM_ID));
    xmlwriter.Write_attribute_string(attr, val);

    attr = "Average_Write_Transaction_Waiting_Time";
    val = std::to_string(
      input_stream_manager->avg_wr_waiting_time(SATA_STREAM_ID));
    xmlwriter.Write_attribute_string(attr, val);

    xmlwriter.Write_close_tag();
  }

  uint16_t Host_Interface_SATA::Get_ncq_depth()
  {
    return ncq_depth;
  }
}