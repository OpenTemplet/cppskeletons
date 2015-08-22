/*--------------------------------------------------------------------------*/
/*  Copyright 2011-2013 Sergey Vostokin, Yuriy Nazarov,                     */
/*                      Ilya Chernomyrdin                                   */
/*  Licensed under the Apache License, Version 2.0 (the "License");         */
/*  you may not use this file except in compliance with the License.        */
/*  You may obtain a copy of the License at                                 */
/*                                                                          */
/*  http://www.apache.org/licenses/LICENSE-2.0                              */
/*                                                                          */
/*  Unless required by applicable law or agreed to in writing, software     */
/*  distributed under the License is distributed on an "AS IS" BASIS,       */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*/
/*  See the License for the specific language governing permissions and     */
/*  limitations under the License.                                          */
/*--------------------------------------------------------------------------*/

#ifndef _TASK_BAG_RUN_TIME
#define _TASK_BAG_RUN_TIME

#include <mpi.h>
#include <cstddef>

namespace TEMPLET {

#define My_MPI_TAG_Task 0
#define My_MPI_TAG_Result 1

enum TBagCommand{
	ControlWord_InputTask,
	ControlWord_Result,
	ControlWord_End
};

#define ALLOC_SIZE 1024

struct mpi_msg{
	TBagCommand	cmd;
	size_t		size;	//size of next message
};

class TBag{
public:
	class Task{
	public:
		Task();

		virtual~Task();
		void send(void*,size_t);
		void recv(void*,size_t);

		virtual void send_task()=0;
		virtual void recv_task()=0;
		virtual void send_result()=0;
		virtual void recv_result()=0;

		/**/
		void* buf;
		size_t allocated;
		size_t used;
	};
public:
	TBag(int num_prc, int argc, char **argv);
	virtual ~TBag();
	virtual Task* createTask()=0;
	
	void run();
	virtual bool if_job()=0;
	virtual void put(Task*)=0;
	virtual void get(Task*)=0;
	virtual void proc(Task*)=0;
	
	double speedup(){return total_workers_count;};
	double duration(){return _duration;};
private:
	Task* task;
private:
	volatile int c_active;
	int worker_number;
	int total_workers_count;
	double _duration;
};
}
#endif
