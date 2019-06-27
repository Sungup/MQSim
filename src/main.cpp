#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <cstring>
#include <numeric>
#include "ssd/SSD_Defs.h"
#include "exec/Execution_Parameter_Set.h"
#include "exec/SSD_Device.h"
#include "exec/Host_System.h"
#include "utils/rapidxml/rapidxml.hpp"
#include "utils/DistributionTypes.h"

using namespace std;

void command_line_args(char* argv[], string& input_file_path, string& workload_file_path)
{

  for (int arg_cntr = 1; arg_cntr < 5; arg_cntr++)
  {
    string arg = argv[arg_cntr];

    char file_path_switch[] = "-i";
    if (arg.compare(0, strlen(file_path_switch), file_path_switch) == 0) {
      input_file_path.assign(argv[++arg_cntr]);
      //cout << input_file_path << endl;
      continue;
    }

    char workload_path_switch[] = "-w";
    if (arg.compare(0, strlen(workload_path_switch), workload_path_switch) == 0) {
      workload_file_path.assign(argv[++arg_cntr]);
      //cout << workload_file_path << endl;
      continue;
    }
  }
}

force_inline void
dump_workload_definitions(const std::string& file_path,
                          IOFlowScenariosList& io_scenarios)
{

  std::cout << "Using MQSim's default workload definitions." << std::endl
            << "Writing the default workload definitions to the expected "
            << "workload definition file." << std::endl;

  IOFlowScenario scenario;
  scenario.push_back(std::make_shared<IOFlowParameterSetSynthetic>(12344));
  scenario.push_back(std::make_shared<IOFlowParameterSetSynthetic>(6533));

  io_scenarios.push_back(scenario);

  PRINT_MESSAGE("Writing default workload parameters to the expected input file.")

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

force_inline void
read_workload_definitions(const string& workload_defs_file_path,
                          IOFlowScenariosList& io_scenarios)
{
  ifstream workload_defs_file;
  workload_defs_file.open(workload_defs_file_path.c_str());

  if (!workload_defs_file) {
    std::cerr << "The specified workload definition file does not exist!"
              << std::endl;

    dump_workload_definitions(workload_defs_file_path, io_scenarios);
    return;
  }

  /// 1. Read input workload parameters
  string line((std::istreambuf_iterator<char>(workload_defs_file)),
              std::istreambuf_iterator<char>());

  workload_defs_file.close();

  /// 2. Check default flag option in file
  if (line == "USE_INTERNAL_PARAMS") {
    dump_workload_definitions(workload_defs_file_path, io_scenarios);
    return;
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
    for (auto node = scenarios->first_node("IO_Scenario"); node ; node = node->next_sibling("IO_Scenario")) {

      IOFlowScenario scenario;

      for (auto flow_def = node->first_node(); flow_def; flow_def = flow_def->next_sibling()) {
        if (!strcmp(flow_def->name(), "IOFlowParameterSetSynthetic"))
          scenario.emplace_back(std::make_shared<IOFlowParameterSetSynthetic>(flow_def));

        else if (!strcmp(flow_def->name(), "IOFlowParameterSetTraceBased"))
          scenario.emplace_back(std::make_shared<IOFlowParameterSetTraceBased>(flow_def));
      }

      if (!scenario.empty())
        io_scenarios.push_back(scenario);
    }
  }
  else
  {
    std::cerr << "Error in the workload definition file!" << endl;

    dump_workload_definitions(workload_defs_file_path, io_scenarios);
    return;
  }

  // Last of all, if scenario is empty, fill the default scenario in io_scenarios.
  if (io_scenarios.empty())
    dump_workload_definitions(workload_defs_file_path, io_scenarios);

}

void collect_results(SSD_Device& ssd, Host_System& host, const char* output_file_path)
{
  Utils::XmlWriter xmlwriter;
  xmlwriter.Open(output_file_path);

  std::string tmp("MQSim_Results");
  xmlwriter.Write_open_tag(tmp);
  
  host.Report_results_in_XML("", xmlwriter);
  ssd.Report_results_in_XML("", xmlwriter);

  xmlwriter.Write_close_tag();

  auto IO_flows = host.Get_io_flows();
  for (auto& flow : host.Get_io_flows()) {
    cout << "Flow " << flow->ID() << " - total requests generated: " << flow->Get_generated_request_count()
         << " total requests serviced:" << flow->Get_serviced_request_count() << endl;
    cout << "                   - device response time: " << flow->Get_device_response_time() << " (us)"
         << " end-to-end request delay:" << flow->Get_end_to_end_request_delay() << " (us)" << endl;
  }
  //cin.get();
}

void print_help()
{
  cout << "MQSim - A simulator for modern NVMe and SATA SSDs developed at SAFARI group in ETH Zurich" << endl <<
    "Standalone Usage:" << endl <<
    "./MQSim [-i path/to/config/file] [-w path/to/workload/file]" << endl;
}

int main(int argc, char* argv[])
{
  string ssd_config_file_path, workload_defs_file_path;
  if (argc != 5)
  {
    // MQSim expects 2 arguments: 1) the path to the SSD configuration definition file, and 2) the path to the workload definition file
    print_help();
    return 1;
  }

  command_line_args(argv, ssd_config_file_path, workload_defs_file_path);

  Execution_Parameter_Set exec_params(ssd_config_file_path);
  IOFlowScenariosList io_scenarios;

  read_workload_definitions(workload_defs_file_path, io_scenarios);

  int s_no = 0;
  for (auto& io_scen : io_scenarios) {
    ++s_no;

    time_t start_time = time(nullptr);
    char* dt = ctime(&start_time);
    PRINT_MESSAGE("MQSim started at " << dt)
    PRINT_MESSAGE("******************************")
    PRINT_MESSAGE("Executing scenario " << s_no << " out of " << io_scenarios.size() << " .......")

    //The simulator should always be reset, before starting the actual simulation
    Simulator->Reset();

    exec_params.Host_Configuration.IO_Flow_Definitions.assign(io_scen.begin(), io_scen.end());

    // Create SSD_Device based on the specified parameters
    SSD_Device ssd(exec_params.SSD_Device_Configuration,
                   exec_params.Host_Configuration.IO_Flow_Definitions);

    //Create Host_System based on the specified parameters
    exec_params.Host_Configuration.Input_file_path = workload_defs_file_path.substr(0, workload_defs_file_path.find_last_of("."));

    Host_System host(&exec_params.Host_Configuration,
                     exec_params.SSD_Device_Configuration.Enabled_Preconditioning,
                     ssd.Host_interface);

    host.Attach_ssd_device(&ssd);

    Simulator->Start_simulation();

    time_t end_time = time(nullptr);
    dt = ctime(&end_time);
    PRINT_MESSAGE("MQSim finished at " << dt)
    auto duration = (uint64_t)difftime(end_time, start_time);
    PRINT_MESSAGE("Total simulation time: " << duration / 3600 << ":" << (duration % 3600) / 60 << ":" << ((duration % 3600) % 60))
    PRINT_MESSAGE("");

    PRINT_MESSAGE("Writing results to output file .......");
    collect_results(ssd, host, (workload_defs_file_path.substr(0, workload_defs_file_path.find_last_of('.')) + "_scenario_" + std::to_string(s_no) + ".xml").c_str());
  }
  cout << "Simulation complete" << endl;

  return 0;
}
