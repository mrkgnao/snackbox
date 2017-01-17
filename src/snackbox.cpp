#include "snackbox/snackbox.hpp"

#include "snackbox/snackbar.hpp"
#include "snackbox/logger.hpp"

#include <thread>

bool killed = false;

void exit_caught(int sig) {
  snackbox::logger::warn("Caught keyboard interrupt, exiting!");
  exit(EXIT_SUCCESS);
}

void createBar() {
  snackbox::Snackbar batt("battery", 5, 5, 300, 22);
  batt.mainloop();
}

void createAnotherBar() {
  snackbox::Snackbar dateBar("date", 310, 5, 300, 22);
  dateBar.mainloop();
}

int main(void) {
  signal(SIGINT, exit_caught);
  std::thread t1(createBar);
  // std::thread t2(createAnotherBar);

  t1.join();
  // t2.join();
  return EXIT_SUCCESS;
}
