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
                           std::string logging_file_path)
  : MQSimEngine::Sim_Object(name),
    _host_io_req_pool(),
    flow_id(flow_id),
    start_lsa_on_device(start_lsa_on_device),
    end_lsa_on_device(end_lsa_on_device),
    io_queue_id(io_queue_id),
    priority_class(priority_class),
    stop_time(stop_time),
    initial_occupancy_ratio(initial_occupancy_ratio),
    total_requests_to_be_generated(total_requets_to_be_generated),
    SSD_device_type(SSD_device_type),
    pcie_root_complex(pcie_root_complex),
    sata_hba(sata_hba),
    STAT_generated_request_count(0),
    STAT_generated_read_request_count(0),
    STAT_generated_write_request_count(0),
    STAT_ignored_request_count(0),
    STAT_serviced_request_count(0),
    STAT_serviced_read_request_count(0),
    STAT_serviced_write_request_count(0),
    _stat_dev_rd_response_time(MAXIMUM_TIME, 0),
    _stat_dev_wr_response_time(MAXIMUM_TIME, 0),
    _stat_rd_req_delay(MAXIMUM_TIME, 0),
    _stat_wr_req_delay(MAXIMUM_TIME, 0),
    _stat_short_term_dev_resp(),
    _stat_short_term_req_delay(),
    STAT_transferred_bytes_total(0),
    STAT_transferred_bytes_read(0),
    STAT_transferred_bytes_write(0),
    progress(0),
    next_progress_step(0),
    enabled_logging(enabled_logging),
    logging_period(logging_period),
    logging_file_path(logging_file_path)
{
  HostIORequest* t= nullptr;

  switch (SSD_device_type)
  {
  case HostInterface_Types::NVME:
    for (uint16_t cmdid = 0; cmdid < (uint16_t)(0xffffffff); cmdid++)
      available_command_ids.insert(cmdid);

    for (uint16_t cmdid = 0; cmdid < nvme_submission_queue_size; cmdid++)
      request_queue_in_memory.push_back(t);

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
    break;
  default:
    break;
  }
}

IO_Flow_Base::~IO_Flow_Base()
{
  log_file.close();
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

void IO_Flow_Base::SATA_consume_io_request(HostIORequest* request)
{
  auto sim = Simulator;

  sim_time_type device_response_time = sim->Time() - request->Enqueue_time;
  sim_time_type request_delay = sim->Time() - request->Arrival_time;
  STAT_serviced_request_count++;

  _stat_short_term_dev_resp += device_response_time;
  _stat_short_term_req_delay += request_delay;

  STAT_transferred_bytes_total += request->LBA_count * SECTOR_SIZE_IN_BYTE;

  if (request->Type == HostIOReqType::READ)
  {
    _stat_dev_rd_response_time += device_response_time;
    _stat_rd_req_delay += request_delay;
    STAT_serviced_read_request_count++;
    STAT_transferred_bytes_read += request->LBA_count * SECTOR_SIZE_IN_BYTE;
  }
  else
  {
    _stat_dev_wr_response_time += device_response_time;
    _stat_wr_req_delay += request_delay;
    STAT_serviced_write_request_count++;
    STAT_transferred_bytes_write += request->LBA_count * SECTOR_SIZE_IN_BYTE;
  }

  request->release();

  //Announce simulation progress
  if (stop_time > 0)
  {
    progress = int(sim->Time() / (double)stop_time * 100);
  }
  else
  {
    progress = int(STAT_serviced_request_count / (double)total_requests_to_be_generated * 100);
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
    PRINT_MESSAGE(progress_bar << " " << progress << "% progress in " << ID() << std::endl)
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
void IO_Flow_Base::NVMe_consume_io_request(CompletionQueueEntry* cqe)
{
  auto sim = Simulator;
  //Find the request and update statistics
  HostIORequest* request = nvme_software_request_queue[cqe->Command_Identifier];
  nvme_software_request_queue.erase(cqe->Command_Identifier);
  available_command_ids.insert(cqe->Command_Identifier);
  sim_time_type device_response_time = sim->Time() - request->Enqueue_time;
  sim_time_type request_delay = sim->Time() - request->Arrival_time;
  STAT_serviced_request_count++;

  _stat_short_term_dev_resp += device_response_time;
  _stat_short_term_req_delay += request_delay;

  STAT_transferred_bytes_total += request->LBA_count * SECTOR_SIZE_IN_BYTE;

  if (request->Type == HostIOReqType::READ)
  {
    STAT_serviced_read_request_count++;
    _stat_dev_rd_response_time += device_response_time;
    _stat_rd_req_delay += request_delay;
    STAT_transferred_bytes_read += request->LBA_count * SECTOR_SIZE_IN_BYTE;
  }
  else
  {
    STAT_serviced_write_request_count++;
    _stat_dev_wr_response_time += device_response_time;
    _stat_wr_req_delay += request_delay;
    STAT_transferred_bytes_write += request->LBA_count * SECTOR_SIZE_IN_BYTE;
  }

  request->release();

  nvme_queue_pair.Submission_queue_head = cqe->SQ_Head;

  //MQSim always assumes that the request is processed correctly, so no need to check cqe->SF_P

  //If the submission queue is not full anymore, then enqueue waiting requests
  while(waiting_requests.size() > 0)
    if (!NVME_SQ_FULL(nvme_queue_pair) && available_command_ids.size() > 0)
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
        NVME_UPDATE_SQ_TAIL(nvme_queue_pair);
      }
      new_req->Enqueue_time = sim->Time();
      pcie_root_complex->Write_to_device(nvme_queue_pair.Submission_tail_register_address_on_device, nvme_queue_pair.Submission_queue_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
    }
    else break;

  delete cqe;

  //Announce simulation progress
  if (stop_time > 0)
  {
    progress = int(sim->Time() / (double)stop_time * 100);
  }
  else
  {
    progress = int(STAT_serviced_request_count / (double)total_requests_to_be_generated * 100);
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
    PRINT_MESSAGE(progress_bar << " " << progress << "% progress in " << ID() << std::endl)
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
SubmissionQueueEntry* IO_Flow_Base::NVMe_read_sqe(uint64_t address)
{
  SubmissionQueueEntry* sqe = new SubmissionQueueEntry;
  HostIORequest* request = request_queue_in_memory[(uint16_t)((address - nvme_queue_pair.Submission_queue_memory_base_address) / sizeof(SubmissionQueueEntry))];
  if (request == nullptr)
    throw std::invalid_argument(this->ID() + ": Request to access a submission queue entry that does not exist.");
  sqe->Command_Identifier = request->IO_queue_info;
  if (request->Type == HostIOReqType::READ)
  {
    sqe->Opcode = NVME_READ_OPCODE;
    sqe->Command_specific[0] = (uint32_t) request->Start_LBA;
    sqe->Command_specific[1] = (uint32_t)(request->Start_LBA >> 32);
    sqe->Command_specific[2] = ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff);
    sqe->PRP_entry_1 = (DATA_MEMORY_REGION);//Dummy addresses, just to emulate data read/write access
    sqe->PRP_entry_2 = (DATA_MEMORY_REGION + 0x1000);//Dummy addresses
  }
  else
  {
    sqe->Opcode = NVME_WRITE_OPCODE;
    sqe->Command_specific[0] = (uint32_t)request->Start_LBA;
    sqe->Command_specific[1] = (uint32_t)(request->Start_LBA >> 32);
    sqe->Command_specific[2] = ((uint32_t)((uint16_t)request->LBA_count)) & (uint32_t)(0x0000ffff);
    sqe->PRP_entry_1 = (DATA_MEMORY_REGION);//Dummy addresses, just to emulate data read/write access
    sqe->PRP_entry_2 = (DATA_MEMORY_REGION + 0x1000);//Dummy addresses
  }
  return sqe;
}
void IO_Flow_Base::Submit_io_request(HostIORequest* request)
{
  switch (SSD_device_type)
  {
  case HostInterface_Types::NVME:
    if (NVME_SQ_FULL(nvme_queue_pair) || available_command_ids.size() == 0)//If either of software or hardware queue is full
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
        NVME_UPDATE_SQ_TAIL(nvme_queue_pair);
      }
      request->Enqueue_time = Simulator->Time();
      pcie_root_complex->Write_to_device(nvme_queue_pair.Submission_tail_register_address_on_device, nvme_queue_pair.Submission_queue_tail);//Based on NVMe protocol definition, the updated tail pointer should be informed to the device
    }
    break;
  case HostInterface_Types::SATA:
    request->Source_flow_id = flow_id;
    sata_hba->Submit_io_request(request);
    break;
  }
}
void IO_Flow_Base::NVMe_update_and_submit_completion_queue_tail()
{
  nvme_queue_pair.Completion_queue_head++;
  if (nvme_queue_pair.Completion_queue_head == nvme_queue_pair.Completion_queue_size)
    nvme_queue_pair.Completion_queue_head = 0;
  pcie_root_complex->Write_to_device(nvme_queue_pair.Completion_head_register_address_on_device, nvme_queue_pair.Completion_queue_head);//Based on NVMe protocol definition, the updated head pointer should be informed to the device
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

  std::string attr;
  std::string val;

  attr = "Request_Count";
  val = std::to_string(STAT_generated_request_count);
  xmlwriter.Write_attribute_string(attr, val);

  attr = "Read_Request_Count";
  val = std::to_string(STAT_generated_read_request_count);
  xmlwriter.Write_attribute_string(attr, val);

  attr = "Write_Request_Count";
  val = std::to_string(STAT_generated_write_request_count);
  xmlwriter.Write_attribute_string(attr, val);

  attr = "IOPS";
  val = std::to_string((double)STAT_generated_request_count / (sim->Time() / SIM_TIME_TO_SECONDS_COEFF));
  xmlwriter.Write_attribute_string(attr, val);

  attr = "IOPS_Read";
  val = std::to_string((double)STAT_generated_read_request_count / (sim->Time() / SIM_TIME_TO_SECONDS_COEFF));
  xmlwriter.Write_attribute_string(attr, val);

  attr = "IOPS_Write";
  val = std::to_string((double)STAT_generated_write_request_count / (sim->Time() / SIM_TIME_TO_SECONDS_COEFF));
  xmlwriter.Write_attribute_string(attr, val);

  attr = "Bytes_Transferred";
  val = std::to_string((double)STAT_transferred_bytes_total);
  xmlwriter.Write_attribute_string(attr, val);

  attr = "Bytes_Transferred_Read";
  val = std::to_string((double)STAT_transferred_bytes_read);
  xmlwriter.Write_attribute_string(attr, val);

  attr = "Bytes_Transferred_Write";
  val = std::to_string((double)STAT_transferred_bytes_write);
  xmlwriter.Write_attribute_string(attr, val);

  attr = "Bandwidth";
  val = std::to_string((double)STAT_transferred_bytes_total / (sim->Time() / SIM_TIME_TO_SECONDS_COEFF));
  xmlwriter.Write_attribute_string(attr, val);

  attr = "Bandwidth_Read";
  val = std::to_string((double)STAT_transferred_bytes_read / (sim->Time() / SIM_TIME_TO_SECONDS_COEFF));
  xmlwriter.Write_attribute_string(attr, val);

  attr = "Bandwidth_Write";
  val = std::to_string((double)STAT_transferred_bytes_write / (sim->Time() / SIM_TIME_TO_SECONDS_COEFF));
  xmlwriter.Write_attribute_string(attr, val);

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
