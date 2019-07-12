#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include "exec/params/ExecParameterSet.h"
#include "exec/HostSystem.h"
#include "exec/SsdDevice.h"
#include "utils/rapidxml/rapidxml.hpp"
#include "utils/Logical_Address_Partitioning_Unit.h"

using namespace std;

#define FILE_PATH_OPT     "-i"
#define WORKLOAD_PATH_OPT "-w"

force_inline void
command_line_args(char* argv[],
                  string& input_file_path,
                  string& workload_file_path)
{
  for (int arg_cntr = 1; arg_cntr < 5; arg_cntr++) {
    string arg = argv[arg_cntr];

    if (arg.compare(0, strlen(FILE_PATH_OPT), FILE_PATH_OPT) == 0) {
      input_file_path.assign(argv[++arg_cntr]);
      continue;
    }

    if (arg.compare(0, strlen(WORKLOAD_PATH_OPT), WORKLOAD_PATH_OPT) == 0) {
      workload_file_path.assign(argv[++arg_cntr]);
      continue;
    }
  }
}

force_inline void
__dump_workload_definitions(const std::string& file_path,
                            IOFlowScenariosList& io_scenarios)
{

  cout << "Using MQSim's default workload definitions." << endl
       << "Writing the default workload definitions to the expected "
       << "workload definition file." << endl;

  IOFlowScenario scenario;
  scenario.push_back(std::make_shared<SyntheticFlowParamSet>(12344));
  scenario.push_back(std::make_shared<SyntheticFlowParamSet>(6533));

  io_scenarios.push_back(scenario);

  cout << "Writing default workload parameters to the expected input file."
       << endl;

  Utils::XmlWriter xmlwriter;
  string tmp;
  xmlwriter.Open(file_path);
  tmp = "MQSim_IO_Scenarios";
  xmlwriter.Write_open_tag(tmp);
  tmp = "IO_Scenario";
  xmlwriter.Write_open_tag(tmp);

  for (auto& flow : scenario)
    flow->XML_serialize(xmlwriter);

  xmlwriter.Write_close_tag();
  xmlwriter.Write_close_tag();
  xmlwriter.Close();

  std::cout << "[====================] Done!\n" << std::endl;
}

force_inline IOFlowScenariosList
__read_workload_definitions(const string& workload_defs_file_path)
{
  IOFlowScenariosList io_scenarios;

  ifstream workload_defs_file;
  workload_defs_file.open(workload_defs_file_path.c_str());

  if (!workload_defs_file) {
    std::cerr << "The specified workload definition file does not exist!"
              << std::endl;

    __dump_workload_definitions(workload_defs_file_path, io_scenarios);
    return io_scenarios;
  }

  /// 1. Read input workload parameters
  string line((std::istreambuf_iterator<char>(workload_defs_file)),
              std::istreambuf_iterator<char>());

  workload_defs_file.close();

  /// 2. Check default flag option in file
  if (line == "USE_INTERNAL_PARAMS") {
    __dump_workload_definitions(workload_defs_file_path, io_scenarios);
    return io_scenarios;
  }

  /// TODO Remove RapidXml because of the old c++ standard (<C++11).
  ///
  /// Alloc temp string space for XML parsing. RapidXml support only
  /// the non-const data type. Because of this reason, we should alloc char
  /// array dynamically using new/delete.
  std::shared_ptr<char> temp(new char[line.length() + 1],
                             std::default_delete<char[]>());
  strcpy(temp.get(), line.c_str());

  rapidxml::xml_document<> doc;
  doc.parse<0>(temp.get());
  auto* scenarios = doc.first_node("MQSim_IO_Scenarios");

  if (scenarios) {
    for (auto node = scenarios->first_node("IO_Scenario");
         node ;
         node = node->next_sibling("IO_Scenario")) {
      IOFlowScenario scene;

      for (auto flow = node->first_node(); flow; flow = flow->next_sibling()) {
        if (!strcmp(flow->name(), "SyntheticFlowParamSet"))
          scene.emplace_back(std::make_shared<SyntheticFlowParamSet>(flow));

        else if (!strcmp(flow->name(), "TraceFlowParameterSet"))
          scene.emplace_back(std::make_shared<TraceFlowParameterSet>(flow));
      }

      if (!scene.empty())
        io_scenarios.push_back(scene);
    }
  } else {
    std::cerr << "Error in the workload definition file!" << endl;

    __dump_workload_definitions(workload_defs_file_path, io_scenarios);
    return io_scenarios;
  }

  // Last of all, if scenario is empty, insert default io scenarios.
  if (io_scenarios.empty())
    __dump_workload_definitions(workload_defs_file_path, io_scenarios);

  return io_scenarios;
}

void
__collect_results(SsdDevice& ssd,
                  HostSystem& host,
                  const std::string& output_file_path)
{
  cout << "Writing results to output file ......." << endl;

  Utils::XmlWriter xmlwriter;
  xmlwriter.Open(output_file_path);

  xmlwriter.Write_open_tag("MQSim_Results");
  
  host.Report_results_in_XML("", xmlwriter);
  ssd.Report_results_in_XML("", xmlwriter);

  xmlwriter.Write_close_tag();

  cout << endl << "[Flow summary]" << endl;
  for (auto& flow : host.io_flows()) {
    cout << " - Flow ID: " << flow->ID() << endl
         << "   - total generated requests: " << flow->Get_generated_request_count() << endl
         << "   - total serviced requests:  " << flow->Get_serviced_request_count() << endl;
    cout << "   - device response time:     " << flow->Get_device_response_time() << " (us)" << endl
         << "   - end-to-end request delay: " << flow->Get_end_to_end_request_delay() << " (us)" << endl
         << endl;
  }
}

void print_help()
{
  cout << "MQSim "
          "- A simulator for modern NVMe and SATA SSDs developed at SAFARI "
          "group in ETH Zurich" << endl
       << endl
       << "Standalone Usage:" << endl
       << "./MQSim "
          "[-i path/to/config/file] "
          "[-w path/to/workload/file]" << endl;
}

void
__run(const ExecParameterSet& params,
      const IOFlowScenario& scenario,
      const std::string& result_file_path)
{
  /// ==========================================================================
  /// Simualtion Block
  time_t start_time = time(nullptr);
  char* dt = ctime(&start_time);

  cout << "MQSim started at " << dt;

  // The simulator should always be reset, before starting the actual simulation
  Simulator->Reset();

  // Create LogicalAddressPartitioningUnit
  StreamIdInfo stream_info(params.SSD_Device_Configuration,
                           scenario);

  Utils::LogicalAddrPartition addr_partitioner(params.SSD_Device_Configuration,
                                               stream_info,
                                               scenario.size());

  // Create SSD_Device based on the specified parameters
  SsdDevice ssd(params.SSD_Device_Configuration,
                 scenario,
                 addr_partitioner,
                 stream_info);

  // Create HostSystem based on the specified parameters
  HostSystem host(params.Host_Configuration,
                   scenario,
                   addr_partitioner,
                   ssd);

  Simulator->Start_simulation();

  time_t end_time = time(nullptr);
  dt = ctime(&end_time);

  cout << "MQSim finished at " << dt;
  /// ==========================================================================

  auto duration = uint64_t(difftime(end_time, start_time));
  cout << "Total simulation time: "
       << duration / 3600 << ":"
       << (duration % 3600) / 60
       << ":" << ((duration % 3600) % 60) << endl << endl;

  /// Report results.
  __collect_results(ssd, host, result_file_path);
}

int
main(int argc, char* argv[])
{
  string ssd_config_path, workload_defs_path;

  if (argc != 5) {
    // MQSim expects 2 arguments:
    //  1) the path to the SSD configuration definition file, and
    //  2) the path to the workload definition file
    print_help();
    return 1;
  }

  // 1. Argument parsing
  command_line_args(argv, ssd_config_path, workload_defs_path);

  // 2. Load workload definitions
  auto io_scenarios = __read_workload_definitions(workload_defs_path);

  // 3. Load ssd configurations
  ExecParameterSet exec_params(ssd_config_path, workload_defs_path);

  // 4. Run simulation
  int s_no = 0;
  for (auto& scenario : io_scenarios) {
    ++s_no;

    cout << "******************************" << endl
         << "Executing scenario " << s_no
         << " out of " << io_scenarios.size() << " ......." << endl;

    __run(exec_params,
          scenario,
          exec_params.result_file_path(s_no));
  }

  cout << "Simulation complete" << endl;

  return 0;
}
