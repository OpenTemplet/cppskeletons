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
	thread=new pthread_t[n_proc];
	event=new pthread_cond_t[n_proc];
	event_mut=new pthread_mutex_t[n_proc];
	left=new bool[n_proc];
	right=new bool[n_proc];
	ini_iter=seg_from=seg_to=0;
}

PLine::~PLine()
{
	delete map_t;
	delete thread;
	delete event;
	delete event_mut;
	delete left;
	delete right;
}

void *tFuncPL(void *p);

void PLine::run()
{
	int res;

	res=pthread_mutex_init(&cs,NULL);
	assert(res==0);
	
	for(int i=0;i<n_proc;i++){
		res=pthread_cond_init(&event[i],NULL);
		assert(res==0);
		res=pthread_mutex_init(&event_mut[i],NULL);
		assert(res==0);
		left[i]=false;right[i]=true;
	}

	done=false;cur_proc=0;
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for(int i=0;i<n_proc;i++){
		res=pthread_create(&thread[i],&attr,tFuncPL,this);
		assert(res==0);
		
		/*if(map_t[i]>=0){
			mask=((DWORD_PTR)1<<map_t[i]);
			res=SetThreadAffinityMask(thread[i],mask);
			assert(res);
		}*/
	}
	left[0]=true;pthread_mutex_lock(&event_mut[0]);pthread_cond_signal(&event[0]);pthread_mutex_unlock(&event_mut[0]);
	//run
	for(int i=0;i<n_proc;i++)pthread_join(thread[i], NULL);
	pthread_attr_destroy(&attr);
	for(int i=0;i<n_proc;i++){
		pthread_cond_destroy(&event[i]);
		pthread_mutex_destroy(&event_mut[i]);
	}
	pthread_mutex_destroy(&cs);
}

void *tFuncPL(void *p)
{
	PLine* obj=(PLine*)p;
	int proc;
	int n_proc=obj->n_proc;
	int it=obj->ini_iter;

	pthread_mutex_lock(&obj->cs);
	proc=obj->cur_proc++;
	pthread_mutex_unlock(&obj->cs);

	int size=(obj->seg_to-obj->seg_from+1)/n_proc;
	int extr=(obj->seg_to-obj->seg_from+1)%n_proc;

	int from=obj->seg_from + size*proc + (proc >= extr ? extr : proc);
	int to=from + (proc+1 <= extr ? size+1 : size) - 1;

	for(;;){
		if(proc==0 && !obj->next(it)){
			pthread_mutex_lock(&obj->cs);
			obj->done=true;
			obj->last_it=it-1;
			pthread_mutex_unlock(&obj->cs);
			
			if(n_proc>1){
				pthread_mutex_lock(&obj->event_mut[1]);
				pthread_cond_signal(&obj->event[1]);
				pthread_mutex_unlock(&obj->event_mut[1]);
			}
			return 0;
		}
		pthread_mutex_lock(&obj->event_mut[proc]);
		while(!obj->left[proc] || !obj->right[proc])
			pthread_cond_wait(&obj->event[proc],&obj->event_mut[proc]);
		pthread_mutex_unlock(&obj->event_mut[proc]);

		if(proc>0)obj->left[proc]=false; if(proc<n_proc-1)obj->right[proc]=false;
		
		for(int i=from;i<=to;i++)obj->prcseg(it,i);
		
		if(obj->done && obj->last_it==it){
			if(proc<n_proc-1){
				pthread_mutex_lock(&obj->event_mut[proc+1]);
				obj->left[proc+1]=true;
				pthread_cond_signal(&obj->event[proc+1]);
				pthread_mutex_unlock(&obj->event_mut[proc+1]);
			}
			return 0;
		}

		it++;

		if(proc>0){
			pthread_mutex_lock(&obj->event_mut[proc-1]);
			obj->right[proc-1]=true;
			pthread_cond_signal(&obj->event[proc-1]);
			pthread_mutex_unlock(&obj->event_mut[proc-1]);
		}
		if(proc<n_proc-1){
			pthread_mutex_lock(&obj->event_mut[proc+1]);
			obj->left[proc+1]=true;
			pthread_cond_signal(&obj->event[proc+1]);
			pthread_mutex_unlock(&obj->event_mut[proc+1]);
		}
	}
	return 0;
}

}