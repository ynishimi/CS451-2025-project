#include <chrono>
#include <iostream>
#include <thread>
#include <array>

#include "parser.hpp"
#include "peer.hpp"

#include <signal.h>
#include "hello.h"

static void stop(int)
{
  // reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // immediately stop network packet processing
  std::cout << "Immediately stopping network packet processing.\n";

  // write/flush output file if necessary
  std::cout << "Writing output.\n";

  // exit directly from signal handler
  exit(0);
}

int main(int argc, char **argv)
{
  signal(SIGTERM, stop);
  signal(SIGINT, stop);

  // `true` means that a config file is required.
  // Call with `false` if no config file is necessary.
  bool requireConfig = true;

  Parser parser(argc, argv);
  parser.parse();

  hello();
  std::cout << std::endl;

  std::cout << "My PID: " << getpid() << "\n";
  std::cout << "From a new terminal type `kill -SIGINT " << getpid() << "` or `kill -SIGTERM "
            << getpid() << "` to stop processing packets\n\n";

  std::cout << "My ID: " << parser.srcId() << "\n\n";

  std::cout << "List of resolved hosts is:\n";
  std::cout << "==========================\n";
  auto hosts = parser.hosts();
  for (auto &host : hosts)
  {
    std::cout << host.srcId << "\n";
    std::cout << "Human-readable IP: " << host.ipReadable() << "\n";
    std::cout << "Machine-readable IP: " << host.ip << "\n";
    std::cout << "Human-readbale Port: " << host.portReadable() << "\n";
    std::cout << "Machine-readbale Port: " << host.port << "\n";
    std::cout << "\n";
  }
  std::cout << "\n";

  std::cout << "Path to output:\n";
  std::cout << "===============\n";
  std::cout << parser.outputPath() << "\n\n";

  std::cout << "Path to config:\n";
  std::cout << "===============\n";
  std::cout << parser.configPath() << "\n\n";

  std::cout << "Doing some initialization...\n\n";

  std::cout << "Broadcasting and delivering messages...\n\n";

  // read config file
  ifstream configFile(parser.configPath());
  string line;
  getline(configFile, line);
  stringstream ss(line);

  int p;  // num of proposal
  int vs; // max num of elements in a proposal
  int ds; // max num of distinct elements across all proposals

  ss >> p >> vs >> ds;

  vector<proposalSet> proposals(p);

  int prop_elem;
  for (int lattice_shot = 0; lattice_shot < p; lattice_shot++)
  {
    getline(configFile, line);
    stringstream ss(line);
    for (int j = 0; j < vs; j++)
    {
      if (ss >> prop_elem)
      {
        proposals[lattice_shot].insert(prop_elem);
      }
    }
  }

  configFile.close();

  Peer peer(parser, parser.outputPath(), p, vs, ds, proposals);
  peer.start();

  // After a process finishes broadcasting,
  // it waits forever for the delivery of messages.
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}
