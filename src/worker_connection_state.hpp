#ifndef __WORKER_CONNECTION_STATE_HPP__
#define __WORKER_CONNECTION_STATE_HPP__
enum connection_state {
   CONNECT_TO_MASTER,        /* connect to master */
   WAIT_FOR_JOB,             /* wait for a job */
   WAIT_FOR_CLIENT_CONNECT,  /* wait for a client to connect */
   PROCESS_JOB,              /* process a job */
   DISCONNECT_CLIENT         /* disconnect the client */
};
#endif
