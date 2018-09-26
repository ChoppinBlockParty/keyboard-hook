#include "hook.hpp"

#include <boost/program_options.hpp>

int main(int argc, char* argv[]) {
  namespace po = boost::program_options;
  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()("print,p", "print input devices")(
    "input,i", po::value<int>(), "specify input device")(
    "fnwin,f", po::value<int>(), "use fn as window key");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  int device = -1;
  bool doShowEvent = false;
  bool useFnAsWindowKey = false;

  if (vm.count("print") <= 0) {
    if (vm.count("input")) {
      device = vm["input"].as<int>();
    }
  } else {
    if (vm.count("input")) {
      device = vm["input"].as<int>();
      doShowEvent = true;
    }

    if (vm.count("input")) {
      useFnAsWindowKey = true;
    }
  }

  setupHook(device, doShowEvent, useFnAsWindowKey);

  return 0;
}
