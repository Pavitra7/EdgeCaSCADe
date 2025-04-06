#include <string>
#include <vector>

#include <CLI11.hpp>
#include <basic_simulator.h>

int main(int argc, char **argv) {
  std::string binaryFile; // assign to default
  std::string inputFile;
  std::string outputFile;
  std::string traceFile;
  std::string signatureFile;
  std::vector<std::string> benchArgs, pargs;
  bool benchmark = false;
  std::string breakpoint = "-1";
  std::string timeout = "-1";
  std::string debugCycle = "-1";
  std::string dtb = "";

  CLI::App app{"Comet RISC-V Simulator"};
  app.add_option("-f,--file", binaryFile,
                 "Specifies the RISC-V program binary file (elf)")
      ->required();
  app.add_option("-i,--input", inputFile,
                 "Specifies the input file (used as standard input of the "
                 "running program)");
  app.add_option(
      "-o,--output", outputFile,
      "Specifies the output file (standard output of the running program)");
  app.add_option("-t,--trace-file", traceFile,
                 "Specifies trace file for simulator output");
  app.add_option("-a,--program-args", pargs,
                 "Specifies command line arguments for the binary program");
  app.add_option("-s,--signature-output", signatureFile,
                 "Specifies signature file for testing purposes");
  app.add_option("-p", benchmark,
                 "Tell if the simulation needs to output benchmark "
                 "performances (0-No/1-Yes)",
                 true);
  app.add_option("-b,--break", breakpoint,
                 "Provide a breakpoint at the cycle given (along with gdb : "
                 "break basic_simulator.cpp:129)");
  app.add_option("-e,--end", timeout,
                 "Add a timeout option to the execution (the simulator stops "
                 "if this number of cycle is reached)");
  app.add_option("-d,--start-debug", debugCycle,
                 "Specifies from wich cycle to start the debug process."
                 "(Cycles will only be printed when this number of cycle is reached)");
#if LINUXOS
  app.add_option("--dtb", dtb,
                 "Specifies the DTB of Comet."
                 "(Used for Linux)");
#endif

  CLI11_PARSE(app, argc, argv);

  // add the binary file name at the start of argv[]
  benchArgs.push_back(binaryFile);
  for (auto a : pargs)
    benchArgs.push_back(a);
  BasicSimulator sim(binaryFile, benchArgs, inputFile, outputFile, traceFile,
                     signatureFile, benchmark, dtb);

  sim.breakpoint = std::stoi(breakpoint, NULL);
  sim.timeout = std::stoi(timeout, NULL);
  sim.debugCycle = std::stoi(debugCycle, NULL);

  sim.run();
  printf("Begining time is %ld",clock());

  return 0;
}
