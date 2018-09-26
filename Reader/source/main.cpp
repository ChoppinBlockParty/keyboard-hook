#include "hook.hpp"

#include <boost/program_options.hpp>

#include <iostream>

int main(int argc, char* argv[]) {
  namespace po = boost::program_options;
  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Displays help")("print,p", "print input devices")(
    "input,i", po::value<int>(), "specify input device")(
    "fnwin,f", po::value<int>(), "use fn as window key");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  int device = -1;
  bool print_events_option = false;
  bool use_fn_as_super_key = false;

  if (vm.count("help")) {
    std::cout << "If no arguments are provided will show available devices to replace"
              << std::endl;
    std::cout << "Most probably root privilegies are required" << std::endl;
    std::cout << desc << std::endl;
    return 0;
  }

  if (vm.count("print") <= 0) {
    if (vm.count("input")) {
      device = vm["input"].as<int>();
    }
  } else {
    if (vm.count("input")) {
      device = vm["input"].as<int>();
      print_events_option = true;
    }

    if (vm.count("fnwin")) {
      use_fn_as_super_key = true;
    }
  }

  setupHook(device, print_events_option, use_fn_as_super_key);

  return 0;
}
