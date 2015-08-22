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

#include "tbag.h"
#include <queue>
#include <cstdlib>

#ifndef WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif

namespace TEMPLET {

#define MANAGEMENT_WORKER_NUMBER 0
#define FIRST_PROCESSING_WORKER_NUMBER (MANAGEMENT_WORKER_NUMBER+1)

TBag::TBag(int num_prc, int argc, char **argv) {
	MPI_Init(&argc, &argv);
	task = 0;
	worker_number = 0;
	total_workers_count = 0;
	_duration=0.0;
}

TBag::~TBag() {
	if(task)delete task;
}

TBag::Task::Task(){
	allocated=ALLOC_SIZE;
	used=0;
	buf = malloc(ALLOC_SIZE);
}

TBag::Task::~Task(){
	free(buf);
}

void TBag::Task::send(void*p, size_t size) {
	if(used+size > allocated) {
		buf = realloc(buf, used+size);
		allocated = used+size;
	}
	memmove((char*)buf+used, p, size);
	used += size;
}

void TBag::Task::recv(void*p, size_t size) {
	if(used+size > allocated) {
		printf("recv exceeded message buffer\n");
		MPI_Abort(MPI_COMM_WORLD,0);
	}
	memmove(p, (char*)buf+used, size);
	used += size;
}

void TBag::run() {

#ifndef WIN32
	struct timeval tim1,tim2;
#else
	LARGE_INTEGER frequency;
	LARGE_INTEGER t1,t2;
	QueryPerformanceFrequency(&frequency);
#endif

	MPI_Comm_rank(MPI_COMM_WORLD, &worker_number);
	MPI_Comm_size(MPI_COMM_WORLD, &total_workers_count);

	task = createTask();

	if (worker_number == MANAGEMENT_WORKER_NUMBER) {

#ifndef WIN32
	gettimeofday(&tim1, NULL);   
#else
	QueryPerformanceCounter(&t1);
#endif
		mpi_msg message_buff;
		c_active = 0;
		std::queue<int> numbers_of_idle_workers;
		for(int i = FIRST_PROCESSING_WORKER_NUMBER; i < total_workers_count; i++){
			numbers_of_idle_workers.push(i);
		}

		do{
			while (numbers_of_idle_workers.size() && if_job()) {
				int destination_worker_number = numbers_of_idle_workers.front();
				numbers_of_idle_workers.pop();

				get(task);

				task->used = 0;
				task->send_task();

				message_buff.cmd = ControlWord_InputTask;
				message_buff.size = task->used;
				MPI_Send(&message_buff, sizeof(message_buff), MPI_BYTE, destination_worker_number, My_MPI_TAG_Task, MPI_COMM_WORLD);
				MPI_Send(task->buf,     task->used,           MPI_BYTE, destination_worker_number, My_MPI_TAG_Task, MPI_COMM_WORLD);
				c_active++;
			}

			MPI_Status status;
			MPI_Recv(&message_buff, sizeof(message_buff), MPI_BYTE, MPI_ANY_SOURCE,    My_MPI_TAG_Result, MPI_COMM_WORLD, &status);
			if(message_buff.size > task->allocated){
				task->buf = realloc(task->buf, message_buff.size);
				task->allocated = message_buff.size;
			}
			MPI_Recv(task->buf,     message_buff.size,    MPI_BYTE, status.MPI_SOURCE, My_MPI_TAG_Result, MPI_COMM_WORLD, &status);
			
			task->used = 0;
			task->recv_result();

			put(task);
			c_active--;
			numbers_of_idle_workers.push(status.MPI_SOURCE);
		}while(c_active || if_job());

		for (int i = FIRST_PROCESSING_WORKER_NUMBER; i < total_workers_count; i++) {
			message_buff.cmd = ControlWord_End;
			message_buff.size = 0;
			MPI_Send(&message_buff, sizeof(message_buff), MPI_BYTE, i, My_MPI_TAG_Task, MPI_COMM_WORLD);
		}

		MPI_Finalize();

#ifndef WIN32	
		gettimeofday(&tim2, NULL); 
		_duration=tim2.tv_sec+(tim2.tv_usec/1000000.0)-tim1.tv_sec+(tim1.tv_usec/1000000.0); 
#else
		QueryPerformanceCounter(&t2);
		_duration=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;
#endif

	} else {
		MPI_Status status;
		mpi_msg message_buff;
		bool NoMoreTasks = false;

		while (!NoMoreTasks) {
			MPI_Recv(&message_buff, sizeof(message_buff), MPI_BYTE, MANAGEMENT_WORKER_NUMBER, My_MPI_TAG_Task, MPI_COMM_WORLD, &status);

			switch (message_buff.cmd) {
			case ControlWord_InputTask:
				if (message_buff.size > task->allocated) {
					task->buf = realloc(task->buf, message_buff.size);
					task->allocated = message_buff.size;
				}
				MPI_Recv(task->buf, message_buff.size, MPI_BYTE, MANAGEMENT_WORKER_NUMBER, My_MPI_TAG_Task, MPI_COMM_WORLD, &status);
				
				task->used = 0;
				task->recv_task();
				
				proc(task);
				
				task->used = 0;
				task->send_result();

				message_buff.cmd = ControlWord_Result;
				message_buff.size = task->used;

				MPI_Send(&message_buff, sizeof(message_buff), MPI_BYTE, MANAGEMENT_WORKER_NUMBER, My_MPI_TAG_Result, MPI_COMM_WORLD);
				MPI_Send(task->buf,     task->used,           MPI_BYTE, MANAGEMENT_WORKER_NUMBER, My_MPI_TAG_Result, MPI_COMM_WORLD);
				
				break;
			case ControlWord_End:
				NoMoreTasks = true;
				break;
			default:
				printf("Unexpected ControlWord\n");
				MPI_Abort(MPI_COMM_WORLD,0);
				break;
			}
		}
		MPI_Finalize();
		exit(0);
	}
}

}
