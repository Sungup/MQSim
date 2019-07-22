#include "IO_Flow_Base.h"

#include "../ssd/interface/Host_Interface_Defs.h"
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
                           sim_time_type stop_time,
                           double initial_occupancy_ratio,
                           uint32_t total_requets_to_be_generated,
                           HostInterface_Types SSD_device_type,
                           PCIe_Root_Complex* pcie_root_complex,
                           SATA_HBA* sata_hba,
                           bool enabled_logging,
                           sim_time_type logging_period,
                           const std::string& logging_file_path)
  : MQSimEngine::Sim_Object(name),
    __flow_id(flow_id),
    __priority_class(priority_class),
    __start_lsa_on_dev(start_lsa_on_device),
    __end_lsa_on_dev(end_lsa_on_device),
    __pcie_root_complex(pcie_root_complex),
    __sata_hba(sata_hba),
    __host_io_req_pool(),
    io_queue_id(io_queue_id),
    stop_time(stop_time),
    _initial_occupancy_ratio(initial_occupancy_ratio),
    total_requests_to_be_generated(total_requets_to_be_generated),
    SSD_device_type(SSD_device_type),
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
    _stat_short_term_dev_resp(),
    _stat_short_term_req_delay(),
    _stat_transferred_reads(),
    _stat_transferred_writes(),
    progress(0),
    next_progress_step(0),
    enabled_logging(enabled_logging),
    logging_period(logging_period),
    logging_file_path(logging_file_path)
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
  next_logging_milestone = logging_period;
  if (enabled_logging)
    log_file.open(logging_file_path, std::ofstream::out);
  log_file << "SimulationTime(us)\t" << "ReponseTime(us)\t" << "EndToEndDelay(us)"<< std::endl;

  _stat_short_term_dev_resp.reset();
  _stat_short_term_req_delay.reset();
}


force_inline void
IO_Flow_Base::__update_stats_by_request(sim_time_type now,
                                        const HostIORequest* request)
{
  sim_time_type device_response_time = now - request->Enqueue_time;
  sim_time_type request_delay = now - request->Arrival_time;
  ++_serviced_req;

  _stat_short_term_dev_resp += device_response_time;
  _stat_short_term_req_delay += request_delay;

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
  auto sim = Simulator;

  //Announce simulation progress
  if (stop_time > 0)
  {
    progress = int(sim->Time() / (double)stop_time * 100);
  }
  else
  {
    progress = int(_serviced_req / (double)total_requests_to_be_generated * 100);
  }

  if (progress >= next_progress_step)
  {
    std::string progress_bar;
    int barWidth = 100;
    progress_bar += "[";
    int pos = progress;
    for (int i = 0; i < barWidth; i += 5) {
      if (i < pos) progress_bar += "=";
      else if (i == pos) progress_bar += ">";
      else progress_bar += " ";
    }
    progress_bar += "] ";
    std::cout << progress_bar << " " << progress << "% progress in " << ID() << std::endl;
    next_progress_step += 5;
  }

  if (sim->Time() > next_logging_milestone)
  {
    log_file << sim->Time() / SIM_TIME_TO_MICROSECONDS_COEFF << "\t"
             << _stat_short_term_dev_resp.avg(SIM_TIME_TO_MICROSECONDS_COEFF) << "\t"
             << _stat_short_term_req_delay.avg(SIM_TIME_TO_MICROSECONDS_COEFF) << std::endl;

    _stat_short_term_dev_resp.reset();
    _stat_short_term_req_delay.reset();

    next_logging_milestone = sim->Time() + logging_period;
  }
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
  auto sim = Simulator;

  //Find the request and update statistics
  HostIORequest* request = nvme_software_request_queue[cqe->Command_Identifier];
  nvme_software_request_queue.erase(cqe->Command_Identifier);
  available_command_ids.insert(cqe->Command_Identifier);

  __update_stats_by_request(sim->Time(), request);

  request->release();

  nvme_queue_pair.Submission_queue_head = cqe->SQ_Head;

  //MQSim always assumes that the request is processed correctly, so no need to check cqe->SF_P

  //If the submission queue is not full anymore, then enqueue waiting requests
  while(!waiting_requests.empty())
    if (!__sq_is_full(nvme_queue_pair) && !available_command_ids.empty())
    {
      HostIORequest* new_req = waiting_requests.front();
      waiting_requests.pop_front();
      if (nvme_software_request_queue[*available_command_ids.begin()] != nullptr)
        PRINT_ERROR("Unexpteced situation in IO_Flow_Base! Overwriting a waiting I/O request in the queue!")
      else
      {
        new_req->IO_queue_info = *available_command_ids.begin();
        nvme_software_request_queue[*available_command_ids.begin()] = new_req;
        available_command_ids.erase(available_command_ids.begin());
        request_queue_in_memory[nvme_queue_pair.Submission_queue_tail] = new_req;
        __update_sq_tail(nvme_queue_pair);
      }
      new_req->Enqueue_time = sim->Time();
      __pcie_root_complex->Write_to_device(nvme_queue_pair.Submission_tail_register_address_on_device, nvme_queue_pair.Submission_queue_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
    }
    else break;

  cqe->release();

  __announce_progress();
}

SQEntry* IO_Flow_Base::NVMe_read_sqe(uint64_t address)
{
  HostIORequest* request = request_queue_in_memory[(uint16_t)((address - nvme_queue_pair.Submission_queue_memory_base_address) / SQEntry::size())];

  if (request == nullptr)
    throw std::invalid_argument(this->ID() + ": Request to access a submission queue entry that does not exist.");

  if (request->Type == HostIOReqType::READ)
  {
    return __sq_entry_pool.construct(NVME_READ_OPCODE,
                                     request->IO_queue_info,
                                     (DATA_MEMORY_REGION),
                                     (DATA_MEMORY_REGION + 0x1000),
                                     (uint32_t)request->Start_LBA,
                                     (uint32_t)(request->Start_LBA >> 32U),
                                     ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff));
  }
  else
  {
    return __sq_entry_pool.construct(NVME_WRITE_OPCODE,
                                     request->IO_queue_info,
                                     (DATA_MEMORY_REGION),
                                     (DATA_MEMORY_REGION + 0x1000),
                                     (uint32_t)request->Start_LBA,
                                     (uint32_t)(request->Start_LBA >> 32U),
                                     ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff));
  }
}
void IO_Flow_Base::Submit_io_request(HostIORequest* request)
{
  switch (SSD_device_type)
  {
  case HostInterface_Types::NVME:
    if (__sq_is_full(nvme_queue_pair) || available_command_ids.empty())//If either of software or hardware queue is full
      waiting_requests.push_back(request);
    else
    {
      if (nvme_software_request_queue[*available_command_ids.begin()] != nullptr)
        PRINT_ERROR("Unexpteced situation in IO_Flow_Base! Overwriting an unhandled I/O request in the queue!")
      else
      {
        request->IO_queue_info = *available_command_ids.begin();
        nvme_software_request_queue[*available_command_ids.begin()] = request;
        available_command_ids.erase(available_command_ids.begin());
        request_queue_in_memory[nvme_queue_pair.Submission_queue_tail] = request;
        __update_sq_tail(nvme_queue_pair);
      }
      request->Enqueue_time = Simulator->Time();
      __pcie_root_complex->Write_to_device(nvme_queue_pair.Submission_tail_register_address_on_device, nvme_queue_pair.Submission_queue_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
    }
    break;
  case HostInterface_Types::SATA:
    request->Source_flow_id = __flow_id;
    __sata_hba->Submit_io_request(request);
    break;
  }
}
void IO_Flow_Base::NVMe_update_and_submit_completion_queue_tail()
{
  nvme_queue_pair.Completion_queue_head++;
  if (nvme_queue_pair.Completion_queue_head == nvme_queue_pair.Completion_queue_size)
    nvme_queue_pair.Completion_queue_head = 0;
  __pcie_root_complex->Write_to_device(nvme_queue_pair.Completion_head_register_address_on_device, nvme_queue_pair.Completion_queue_head);//Based on NVMe protocol definition, the updated head pointer should be informed to the device
}

const IoQueueInfo&
IO_Flow_Base::queue_info()
{
  return nvme_queue_pair;
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
