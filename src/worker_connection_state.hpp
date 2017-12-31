// Copyright 2017 Hunter L. Allen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __WORKER_CONNECTION_STATE_HPP__
#define __WORKER_CONNECTION_STATE_HPP__
enum connection_state
{
  CONNECT_TO_MASTER,         /* connect to master */
  WAIT_FOR_JOB,              /* wait for a job */
  WAIT_FOR_CLIENT_CONNECT,   /* wait for a client to connect */
  PROCESS_JOB,               /* process a job */
  DISCONNECT_CLIENT          /* disconnect the client */
};
#endif
