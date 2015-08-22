/*--------------------------------------------------------------------------*/
/*  Copyright 2010-2014 Sergey Vostokin                                     */
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

#include "pipe.h"

#include <assert.h>

namespace TEMPLET {

PLine::PLine(int np)
{
	n_proc=np;
	map_t=new int[n_proc];for(int i=0;i<n_proc;i++)map_t[i]=-1;
	thread=new HANDLE[n_proc];
	event=new HANDLE[n_proc];
	left=new bool[n_proc];
	right=new bool[n_proc];
	ini_iter=seg_from=seg_to=0;
}

PLine::~PLine()
{
	delete map_t;
	delete thread;
	delete event;
	delete left;
	delete right;
}

void PLine::run()
{
	DWORD id;
	DWORD_PTR mask;
	DWORD res;

	InitializeCriticalSection(&cs);
	
	for(int i=0;i<n_proc;i++){
		event[i]=CreateEvent(NULL,FALSE,FALSE,NULL);//auto,non-signaled
		assert(event[i]);
		left[i]=false;right[i]=true;
	}

	done=false;cur_proc=0;
	
	for(int i=0;i<n_proc;i++){
		thread[i]=CreateThread(NULL,NULL,tFuncPL,this,0,&id);
		assert(thread[i]);
		
		if(map_t[i]>=0){
			mask=((DWORD_PTR)1<<map_t[i]);
			res=SetThreadAffinityMask(thread[i],mask);
			assert(res);
		}
	}
	left[0]=true;SetEvent(event[0]);
	//run
	WaitForMultipleObjects(n_proc,thread,TRUE,INFINITE);
	for(int i=0;i<n_proc;i++){
		CloseHandle(thread[i]);
		CloseHandle(event[i]);
	}
	DeleteCriticalSection(&cs);
}

DWORD WINAPI tFuncPL(LPVOID p)
{
	PLine* obj=(PLine*)p;
	int proc;
	int n_proc=obj->n_proc;
	int it=obj->ini_iter;

	EnterCriticalSection(&obj->cs);
	proc=obj->cur_proc++;
	LeaveCriticalSection(&obj->cs);

	int size=(obj->seg_to-obj->seg_from+1)/n_proc;
	int extr=(obj->seg_to-obj->seg_from+1)%n_proc;

	int from=obj->seg_from + size*proc + (proc >= extr ? extr : proc);
	int to=from + (proc+1 <= extr ? size+1 : size) - 1;

	for(;;){
		if(proc==0 && !obj->next(it)){
			EnterCriticalSection(&obj->cs);
			obj->done=true;
			obj->last_it=it-1;
			if(n_proc>1)SetEvent(obj->event[1]);
			LeaveCriticalSection(&obj->cs);
			return 0;
		}
		while(!obj->left[proc] || !obj->right[proc])
			WaitForSingleObject(obj->event[proc],INFINITE);
		
		if(proc>0)obj->left[proc]=false; if(proc<n_proc-1)obj->right[proc]=false;
		
		for(int i=from;i<=to;i++)obj->prcseg(it,i);

		if(obj->done && obj->last_it==it){
			if(proc<n_proc-1){
				obj->left[proc+1]=true;
				SetEvent(obj->event[proc+1]);
			}
			return 0;
		}

		it++;

		if(proc>0){
			obj->right[proc-1]=true;
			SetEvent(obj->event[proc-1]);
		}
		if(proc<n_proc-1){
			obj->left[proc+1]=true;
			SetEvent(obj->event[proc+1]);
		}
	}
	return 0;
}

}