// DPI Interface
#include "svdpi.h"
#include "veriuser.h"

// String + constant + data type includes
#include <string.h>
#include <stdio.h> 
#include <errno.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/wait.h> 

// Can we catch SIGTERM, Ctrl+C in CVC?
#include  <signal.h>

// For timer
#include <time.h>
#include <math.h>

extern void svdpi_read(int, int);    // Imported from SystemVerilog
// extern void svdpi_write(void);    // Imported from SystemVerilog


void svdpi_setup (void);
void in_read (void);
void retrieve_inputs (void);
void get_input (void);
int input_timeout (int filedes, unsigned int seconds);
int return_input (const int a);
void out_write (const int, const int, const int, const int, const int, const int, 
                const int, const int, const int, const int, const int, const int, 
                const int);

static int init_pipes();

static int rpipe;
static int wpipe;

static int hz100 = -1;
static int pb = -1;
static char input [21] = "\0";

static int ss7;
static int ss6;
static int ss5;
static int ss4;
static int ss3;
static int ss2;
static int ss1;
static int ss0;
static int left;
static int right;
static int red;
static int green;
static int blue;

static time_t start_time;

static int init_pipes()
{
  char *w;
  char *r;

  static int init_pipes_flag = 0;

  if (init_pipes_flag) {
    return(0);
  }

  if ((w = getenv("SVDPI_TO_PIPE")) == NULL) {
    printf("ERROR: no write pipe to node.js\n");
    return(1);
  }
  if ((r = getenv("SVDPI_FROM_PIPE")) == NULL) {
    printf("ERROR: no read pipe from node.js\n");
    return(1);
  }
  wpipe = atoi(w);
  rpipe = atoi(r);
  init_pipes_flag = 1;
  return (0);
}

void caught_signal (int sig) {
  printf ("Signal caught: %d", sig);
}

void svdpi_setup (void)
{
    signal (SIGTERM, caught_signal);
    signal (SIGINT, caught_signal);
    if (init_pipes() == 1)
      exit (0);
    get_input();
    start_time = time (NULL);
}

/******************************************************************************/
// https://www.gnu.org/software/libc/manual/html_node/Waiting-for-I_002fO.html
/******************************************************************************/

int input_timeout (int filedes, unsigned int seconds)
{
  fd_set set;
  struct timeval timeout;

  /* Initialize the file descriptor set. */
  FD_ZERO (&set);
  FD_SET (filedes, &set);

  /* Initialize the timeout data structure. */
  timeout.tv_sec = seconds;
  timeout.tv_usec = 5000;

  /* select returns 0 if timeout, 1 if input available, -1 if error. */
  return TEMP_FAILURE_RETRY (select (FD_SETSIZE,
                                     &set, NULL, NULL,
                                     &timeout));
}
/******************************************************************************/

void get_input (void)
{
    char newinput [1024];
    //ffffffffffffffffffff
    while (strcmp (input, "\0") == 0 || input_timeout (rpipe, 0))
    {
        read (rpipe, newinput, 128);
        if (strcmp (newinput, "END SIMULATION") == 0)
        {
          exit(0);
        }
        for (int i = 19; i >= 0; i--)
            input [i] = newinput [19 - i];
    }
}

void timer_watch (void)
{
  time_t now;
        now = time (NULL);
        if (now - start_time > 300)
        {
          char buffer [100];
          snprintf (buffer, 100, "\nTime limit of 10 minutes exceeded! Stopping simulation.\n");
          write(wpipe, buffer, strlen(buffer) + 1);
          exit(1);
        }
}

void out_write (const int p_ss7,  const int p_ss6,   const int p_ss5, const int p_ss4,
                const int p_ss3,  const int p_ss2,   const int p_ss1, const int p_ss0,
                const int p_left, const int p_right, const int p_red, const int p_green, 
                const int p_blue)
{  
  if (ss7 != p_ss7 || ss6 != p_ss6 || ss5 != p_ss5 || ss4 != p_ss4 || ss3 != p_ss3 || 
        ss2 != p_ss2 || ss1 != p_ss1 || ss0 != p_ss0 || left != p_left || right != p_right || 
        red != p_red || green != p_green || blue != p_blue)
    {
        ss7 = p_ss7;
        ss6 = p_ss6;
        ss5 = p_ss5;
        ss4 = p_ss4;
        ss3 = p_ss3;
        ss2 = p_ss2;
        ss1 = p_ss1;
        ss0 = p_ss0;
        left = p_left;
        right = p_right;
        red = p_red;
        green = p_green;
        blue = p_blue;

        char buffer [500];
        snprintf (buffer, 200, "{\"LFTRED\":%d,\"RGTRED\":%d,\"REDLED\":%d,\"GRNLED\":%d,\"BLULED\":%d,\"SS7\":%d,\"SS6\":%d,\"SS5\":%d,\"SS4\":%d,\"SS3\":%d,\"SS2\":%d,\"SS1\":%d,\"SS0\":%d}\n",
                                left, right, red, green, blue, ss7, ss6, ss5, ss4, ss3, ss2, ss1, ss0);
        write(wpipe, buffer, strlen(buffer) + 1);
    } 
    return;
}

void in_read(void)
{  
    retrieve_inputs();
    svdpi_read (pb, hz100);
}

int return_input (const int a)
{
  return a ? hz100 : pb;
}

void retrieve_inputs()
{
    get_input();
    pb = 0;
    for (int i = 0; i < 20; i++)
    {
        pb = (pb << 1) | (input [i] == 'f' ? 0 : 1);
    }
    hz100 = hz100 == 0 ? 1 : 0;
}

//ffffffffffffffffffff
//ffffffffffffffffffft
//tttttttttttttttttttt
