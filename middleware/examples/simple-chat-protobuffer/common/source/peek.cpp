#include <peek.h>

#include <iostream>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#include <app_log.h>

int casual::peek(unsigned int msecs)
{
   // http://stackoverflow.com/questions/9053175/is-it-possible-to-set-timeout-for-stdcin
   // http://stackoverflow.com/questions/6848128/peek-stdin-using-pthreads
   struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };

   int ret = 0;
   ret = poll(&pfd, 1, msecs);  // timeout in millisecods 
   return ret;
}

