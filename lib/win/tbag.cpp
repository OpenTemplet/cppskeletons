/*--------------------------------------------------------------------------*/
/*  Copyright 2010-2013 Sergey Vostokin                                     */
/*                                                                          */
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
#include <assert.h>

namespace TEMPLET {

TBag::TBag(int num_prc,int argc, char* argv[])
{
	nproc=num_prc;

	task=new Task*[nproc];
	thread=new HANDLE[nproc];

	InitializeCriticalSection(&cs);
	await=CreateEvent(NULL,FALSE,FALSE,NULL);//auto,non-signaled
	assert(await);

	_duration=0.0;
}

TBag::~TBag()
{
	delete task;
	delete thread;
	DeleteCriticalSection(&cs);
	CloseHandle(await);
}

void TBag::run()
{
	DWORD id;
	
	cur_task=0;
	c_active=0;

	for(int i=0;i<nproc;i++){
		task[i]=createTask();
		thread[i]=CreateThread(NULL,0,tFunc,this,0,&id);
		assert(thread[i]&&task[i]);
	}
	
	//run
	LARGE_INTEGER frequency;
	LARGE_INTEGER t1,t2;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&t1);

	WaitForMultipleObjects(nproc,thread,TRUE,INFINITE);

	QueryPerformanceCounter(&t2);
	_duration=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;

	for(int i=0;i<nproc;i++){
		CloseHandle(thread[i]);
		delete task[i];
	}
}

DWORD WINAPI tFunc(LPVOID p)
{
	TBag* b=(TBag*)p;
	TBag::Task* task;

	EnterCriticalSection(&b->cs);
	task=b->task[b->cur_task++];
	for(;;){
		while(!b->if_job()){
			if(!b->c_active){
				SetEvent(b->await);
				LeaveCriticalSection(&b->cs);
				return 0;
			}
			LeaveCriticalSection(&b->cs);
			WaitForSingleObject(b->await,INFINITE);
			EnterCriticalSection(&b->cs);
		}
		b->get(task);
		b->c_active++;
		LeaveCriticalSection(&b->cs);

		b->proc(task);
		
		EnterCriticalSection(&b->cs);
		b->c_active--;
		b->put(task);
		SetEvent(b->await);
	}
	return 0;
}
}
