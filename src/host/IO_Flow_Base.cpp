#include "IO_Flow_Base.h"

#include "../sim/Engine.h"

#include "PCIe_Root_Complex.h"
#include "SATA_HBA.h"

// Children for IO_Flow_Base
#include "IO_Flow_Synthetic.h"
#include "IO_Flow_Trace_Based.h"

using namespace Host_Components;

IO_Flow_Base::IO_Flow_Base(const sim_object_id_type& name,
                           uint16_t flow_id,
                           LHA_type start_lsa_on_device,
                           LHA_type end_lsa_on_device,
                           uint16_t io_queue_id,
                           uint16_t nvme_submission_queue_size,
                           uint16_t nvme_completion_queue_size,
                           IO_Flow_Priority_Class priority_class,
                           sim_time_type /* stop_time */,
                           double initial_occupancy_ratio,
                           uint32_t total_requets_to_be_generated,
                           HostInterface_Types SSD_device_type,
                           PCIe_Root_Complex* pcie_root_complex,
                           SATA_HBA* sata_hba,
                           bool enabled_logging,
                           sim_time_type logging_period,
                           std::string logging_file_path)
  : MQSimEngine::Sim_Object(name),
    __flow_id(flow_id),
    __priority_class(priority_class),
    __start_lsa_on_dev(start_lsa_on_device),
    __end_lsa_on_dev(end_lsa_on_device),
    __pcie_root_complex(pcie_root_complex),
    __sata_hba(sata_hba),
    __host_io_req_pool(),
    __sq_entry_pool(),
    __req_submitter(this,
                    SSD_device_type == HostInterface_Types::NVME
                      ? &IO_Flow_Base::__submit_nvme_request
                      : &IO_Flow_Base::__submit_sata_request),
    _initial_occupancy_ratio(initial_occupancy_ratio),
    total_requests_to_be_generated(total_requets_to_be_generated),
    SSD_device_type(SSD_device_type),
    nvme_queue_pair(),
    io_queue_id(io_queue_id),
    _generated_req(0),
    _serviced_req(0),
    _stat_generated_reads(),
    _stat_generated_writes(),
    _stat_serviced_reads(),
    _stat_serviced_writes(),
    _stat_dev_rd_response_time(MAXIMUM_TIME, 0),
    _stat_dev_wr_response_time(MAXIMUM_TIME, 0),
    _stat_rd_req_delay(MAXIMUM_TIME, 0),
    _stat_wr_req_delay(MAXIMUM_TIME, 0),
    _stat_transferred_reads(),
    _stat_transferred_writes(),
    __announcing_at(0),
    __logging_on(enabled_logging),
    __logging_period(__logging_on ? logging_period : MAXIMUM_TIME),
    __logging_path(std::move(logging_file_path)),
    __logging_dev_resp(),
    __logging_req_delay(),
    __logging_at(0)
{
  if (SSD_device_type == HostInterface_Types::NVME) {
    for (uint16_t cmdid = 0; cmdid < (uint16_t)(0xffffffff); cmdid++)
      available_command_ids.insert(cmdid);

    for (uint16_t cmdid = 0; cmdid < nvme_submission_queue_size; cmdid++)
      request_queue_in_memory.push_back(nullptr);

    nvme_queue_pair.Submission_queue_size = nvme_submission_queue_size;
    nvme_queue_pair.Submission_queue_head = 0;
    nvme_queue_pair.Submission_queue_tail = 0;
    nvme_queue_pair.Completion_queue_size = nvme_completion_queue_size;
    nvme_queue_pair.Completion_queue_head = 0;
    nvme_queue_pair.Completion_queue_tail = 0;
    switch (io_queue_id)//id = 0: admin queues, id = 1 to 8, normal I/O queues
    {
    case 0:
      throw std::logic_error("I/O queue id 0 is reserved for NVMe admin queues and should not be used for I/O flows");
    case 1:
      nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_1;
      nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_1;
      nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_1;
      nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_1;
      break;
    case 2:
      nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_2;
      nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_2;
      nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_2;
      nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_2;
      break;
    case 3:
      nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_3;
      nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_3;
      nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_3;
      nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_3;
      break;
    case 4:
      nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_4;
      nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_4;
      nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_4;
      nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_4;
      break;
    case 5:
      nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_5;
      nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_5;
      nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_5;
      nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_5;
      break;
    case 6:
      nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_6;
      nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_6;
      nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_6;
      nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_6;
      break;
    case 7:
      nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_7;
      nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_7;
      nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_7;
      nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_7;
      break;
    case 8:
      nvme_queue_pair.Submission_queue_memory_base_address = SUBMISSION_QUEUE_MEMORY_8;
      nvme_queue_pair.Submission_tail_register_address_on_device = SUBMISSION_QUEUE_REGISTER_8;
      nvme_queue_pair.Completion_queue_memory_base_address = COMPLETION_QUEUE_MEMORY_8;
      nvme_queue_pair.Completion_head_register_address_on_device = COMPLETION_QUEUE_REGISTER_8;
      break;
    default:
      break;
    }
  }
}

force_inline bool
IO_Flow_Base::__sq_is_full(const NVMe_Queue_Pair& Q) const
{
  return Q.Submission_queue_tail < Q.Submission_queue_size - 1
           ? Q.Submission_queue_tail + 1 == Q.Submission_queue_head
           : Q.Submission_queue_head == 0;
}

force_inline void
IO_Flow_Base::__update_sq_tail(NVMe_Queue_Pair& Q)
{
  ++Q.Submission_queue_tail;

  if (Q.Submission_queue_tail == Q.Submission_queue_size)
    nvme_queue_pair.Submission_queue_tail = 0;
}

void IO_Flow_Base::Start_simulation()
{
  __logging_at = __logging_period;
  if (__logging_on)
    __lfstream.open(__logging_path, std::ofstream::out);
  __lfstream << "SimulationTime(us)\t" << "ReponseTime(us)\t" << "EndToEndDelay(us)"<< std::endl;

  __logging_dev_resp.reset();
  __logging_req_delay.reset();
}


force_inline void
IO_Flow_Base::__update_stats_by_request(sim_time_type now,
                                        const HostIORequest* request)
{
  sim_time_type device_response_time = now - request->Enqueue_time;
  sim_time_type request_delay = now - request->Arrival_time;
  ++_serviced_req;

  __logging_dev_resp += device_response_time;
  __logging_req_delay += request_delay;

  if (request->Type == HostIOReqType::READ)
  {
    ++_stat_serviced_reads;
    _stat_dev_rd_response_time += device_response_time;
    _stat_rd_req_delay += request_delay;
    _stat_transferred_reads += request->requested_size();
  }
  else
  {
    ++_stat_serviced_writes;
    _stat_dev_wr_response_time += device_response_time;
    _stat_wr_req_delay += request_delay;
    _stat_transferred_writes += request->requested_size();
  }
}

force_inline void
IO_Flow_Base::__announce_progress()
{
  // Announce simulation progress
  int progress = this->_get_progress();

  // TODO dedicate this block from this announce function
  if (progress >= __announcing_at)
  {
    char bar[PROG_BAR_SIZE];
    char* pos = bar;

    *pos = '[';

    for (int i = 0; i < PROG_BAR_WIDTH; i += PROGRESSING_STEP) {
      if (i < progress) *(++pos) = '=';
      else if (i == progress) *(++pos) = '>';
      else *(++pos) = ' ';
    }

    *(++pos) = ']';
    *(++pos) = '\0';

    std::cout << bar << " " << progress << "% progress in " << ID() << std::endl;
    __announcing_at += PROGRESSING_STEP;
  }

  // TODO dedicate this block from this class
  auto sim = Simulator;

  if (__logging_at < sim->Time())
  {
    __lfstream << sim->Time() / SIM_TIME_TO_MICROSECONDS_COEFF << "\t"
             << __logging_dev_resp.avg(SIM_TIME_TO_MICROSECONDS_COEFF) << "\t"
             << __logging_req_delay.avg(SIM_TIME_TO_MICROSECONDS_COEFF) << std::endl;

    __logging_dev_resp.reset();
    __logging_req_delay.reset();

    __logging_at = sim->Time() + __logging_period;
  }
}

force_inline void
IO_Flow_Base::__enqueue_to_sq(HostIORequest* request)
{
  auto cmd_iter = available_command_ids.begin();
  auto cmd_id = *cmd_iter;

  if (nvme_software_request_queue[cmd_id] != nullptr)
    throw mqsim_error("Unexpected situation in IO_Flow_Base! "
                      "Overwriting an unhandled I/O request in the queue!");

  request->IO_queue_info = cmd_id;
  nvme_software_request_queue[cmd_id] = request;
  available_command_ids.erase(cmd_iter);

  request_queue_in_memory[nvme_queue_pair.Submission_queue_tail] = request;

  __update_sq_tail(nvme_queue_pair);

  request->Enqueue_time = Simulator->Time();

  // Based on NVMe protocol definition, the updated tail pointer should be
  // informed to the device
  __pcie_root_complex->Write_to_device(nvme_queue_pair.Submission_tail_register_address_on_device,
                                       nvme_queue_pair.Submission_queue_tail);
}

void
IO_Flow_Base::__submit_nvme_request(HostIORequest *request)
{
  // If either of software or hardware queue is full
  if (__sq_is_full(nvme_queue_pair) || available_command_ids.empty()) {
    waiting_requests.push_back(request);
    return;
  }

  __enqueue_to_sq(request);
}

void
IO_Flow_Base::__submit_sata_request(HostIORequest *request)
{
  request->Source_flow_id = __flow_id;
  __sata_hba->Submit_io_request(request);
}

void
IO_Flow_Base::SATA_consume_io_request(HostIORequest* request)
{
  __update_stats_by_request(Simulator->Time(), request);

  request->release();

  __announce_progress();
}

void
IO_Flow_Base::NVMe_consume_io_request(CQEntry* cqe)
{
  //Find the request and update statistics
  HostIORequest* request = nvme_software_request_queue[cqe->Command_Identifier];
  nvme_software_request_queue.erase(cqe->Command_Identifier);
  available_command_ids.insert(cqe->Command_Identifier);

  __update_stats_by_request(Simulator->Time(), request);

  request->release();

  nvme_queue_pair.Submission_queue_head = cqe->SQ_Head;

  /// MQSim always assumes that the request is processed correctly,
  /// so no need to check cqe->SF_P

  // If the submission queue is not full anymore, then enqueue waiting requests
  while(!waiting_requests.empty()) {
    if (__sq_is_full(nvme_queue_pair) || available_command_ids.empty())
      break;

    HostIORequest* new_req = waiting_requests.front();
    waiting_requests.pop_front();

    __enqueue_to_sq(new_req);
  }

  cqe->release();

  __announce_progress();
}

SQEntry* IO_Flow_Base::NVMe_read_sqe(uint64_t address)
{
  HostIORequest* request = request_queue_in_memory[(uint16_t)((address - nvme_queue_pair.Submission_queue_memory_base_address) / SQEntry::size())];

  if (request == nullptr)
    throw std::invalid_argument(this->ID() + ": Request to access a submission queue entry that does not exist.");

  /// Currently generated sq entries information are same between READ and WRITE
  uint8_t op_code = (request->Type == HostIOReqType::READ)
                      ? NVME_READ_OPCODE : NVME_WRITE_OPCODE;

  return __sq_entry_pool.construct(op_code,
                                   request->IO_queue_info,
                                   (DATA_MEMORY_REGION),
                                   (DATA_MEMORY_REGION + 0x1000),
                                   lsb_of_lba(request->Start_LBA),
                                   msb_of_lba(request->Start_LBA),
                                   lsb_of_lba_count(request->LBA_count));
}

void IO_Flow_Base::NVMe_update_and_submit_completion_queue_tail()
{
  nvme_queue_pair.Completion_queue_head++;
  if (nvme_queue_pair.Completion_queue_head == nvme_queue_pair.Completion_queue_size)
    nvme_queue_pair.Completion_queue_head = 0;
  __pcie_root_complex->Write_to_device(nvme_queue_pair.Completion_head_register_address_on_device, nvme_queue_pair.Completion_queue_head);//Based on NVMe protocol definition, the updated head pointer should be informed to the device
}

void IO_Flow_Base::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter)
{
  auto sim = Simulator;

  xmlwriter.Write_open_tag(name_prefix + ".IO_Flow");

  xmlwriter.Write_attribute_string("Name", ID());

  double seconds = sim->seconds();

  auto gen_reqs = _stat_generated_reads + _stat_generated_writes;

  xmlwriter.Write_attribute_string("Request_Count", gen_reqs.count());

  xmlwriter.Write_attribute_string("Read_Request_Count",
                                   _stat_generated_reads.count());

  xmlwriter.Write_attribute_string("Write_Request_Count",
                                   _stat_generated_writes.count());

  xmlwriter.Write_attribute_string("IOPS", gen_reqs.iops(seconds));

  xmlwriter.Write_attribute_string("IOPS_Read",
                                   _stat_generated_reads.iops(seconds));

  xmlwriter.Write_attribute_string("IOPS_Write",
                                   _stat_generated_writes.iops(seconds));

  auto transferred = _stat_transferred_reads + _stat_transferred_writes;

  xmlwriter.Write_attribute_string("Bytes_Transferred", transferred.sum());

  xmlwriter.Write_attribute_string("Bytes_Transferred_Read",
                                   _stat_transferred_reads.sum());

  xmlwriter.Write_attribute_string("Bytes_Transferred_Write",
                                   _stat_transferred_writes.sum());

  xmlwriter.Write_attribute_string("Bandwidth",
                                    transferred.bandwidth(seconds));

  xmlwriter.Write_attribute_string("Bandwidth_Read",
                                   _stat_transferred_reads.bandwidth(seconds));

  xmlwriter.Write_attribute_string("Bandwidth_Write",
                                   _stat_transferred_writes.bandwidth(seconds));

  // Device response time and end-to-end request delay
  auto dev_resp  = _stat_dev_rd_response_time + _stat_dev_wr_response_time;
  auto req_delay = _stat_rd_req_delay + _stat_wr_req_delay;

  xmlwriter.Write_attribute_string("Device_Response_Time",
                                   dev_resp.avg(SIM_TIME_TO_MICROSECONDS_COEFF));

  xmlwriter.Write_attribute_string("Min_Device_Response_Time",
                                   dev_resp.min(SIM_TIME_TO_MICROSECONDS_COEFF));

  xmlwriter.Write_attribute_string("Max_Device_Response_Time",
                                   dev_resp.max(SIM_TIME_TO_MICROSECONDS_COEFF));

  xmlwriter.Write_attribute_string("End_to_End_Request_Delay",
                                   req_delay.avg(SIM_TIME_TO_MICROSECONDS_COEFF));

  xmlwriter.Write_attribute_string("Min_End_to_End_Request_Delay",
                                   req_delay.min(SIM_TIME_TO_MICROSECONDS_COEFF));

  xmlwriter.Write_attribute_string("Max_End_to_End_Request_Delay",
                                   req_delay.max(SIM_TIME_TO_MICROSECONDS_COEFF));

  xmlwriter.Write_close_tag();
}

// ---------------
// IO Flow builder
// ---------------

IoFlowPtr
Host_Components::build_io_flow(const sim_object_id_type& host_id,
                               const HostParameterSet& host_params,
                               const IOFlowParamSet& flow_params,
                               const Utils::LogicalAddrPartition& lapu,
                               uint16_t flow_id,
                               Host_Components::PCIe_Root_Complex& root_complex,
                               Host_Components::SATA_HBA* sata_hba,
                               HostInterface_Types interface_type,
                               uint16_t sq_size,
                               uint16_t cq_size)
{
  auto& synthetic_params = (SyntheticFlowParamSet&) flow_params;
  auto& trace_params     = (TraceFlowParameterSet&) flow_params;

  switch (flow_params.Type) {
  case Flow_Type::SYNTHETIC:
    return std::make_shared<IO_Flow_Synthetic>(
      host_id + ".IO_Flow.Synth.No_" + std::to_string(flow_id),
      flow_id,
      lapu.available_start_lha(flow_id),
      lapu.available_end_lha(flow_id),
      synthetic_params.working_set_rate(),
      FLOW_ID_TO_Q_ID(flow_id),
      sq_size,
      cq_size,
      synthetic_params.Priority_Class,
      synthetic_params.read_rate(),
      synthetic_params.Address_Distribution,
      synthetic_params.hot_region_rate(),
      synthetic_params.Request_Size_Distribution,
      synthetic_params.Average_Request_Size,
      synthetic_params.Variance_Request_Size,
      synthetic_params.Synthetic_Generator_Type,
      synthetic_params.avg_arrival_time(),
      synthetic_params.Average_No_of_Reqs_in_Queue,
      synthetic_params.Generated_Aligned_Addresses,
      synthetic_params.Address_Alignment_Unit,
      synthetic_params.Seed,
      synthetic_params.Stop_Time,
      synthetic_params.init_occupancy_rate(),
      synthetic_params.Total_Requests_To_Generate,
      interface_type,
      &root_complex,
      sata_hba,
      host_params.Enable_ResponseTime_Logging,
      host_params.ResponseTime_Logging_Period_Length,
      host_params.stream_log_path(flow_id)
    );

  case Flow_Type::TRACE:
    return std::make_shared<IO_Flow_Trace_Based>(
      host_id + ".IO_Flow.Trace." + trace_params.File_Path,
      flow_id,
      lapu.available_start_lha(flow_id),
      lapu.available_end_lha(flow_id),
      FLOW_ID_TO_Q_ID(flow_id),
      sq_size,
      cq_size,
      trace_params.Priority_Class,
      trace_params.init_occupancy_rate(),
      trace_params.File_Path,
      trace_params.Time_Unit,
      trace_params.Relay_Count,
      trace_params.Percentage_To_Be_Executed,
      interface_type,
      &root_complex,
      sata_hba,
      host_params.Enable_ResponseTime_Logging,
      host_params.ResponseTime_Logging_Period_Length,
      host_params.stream_log_path(flow_id)
    );
  }
}
