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

#ifndef _PIPE_LINE_RUN_TIME
#define _PIPE_LINE_RUN_TIME

#include <pthread.h>

namespace TEMPLET {

class PLine{
	friend void *tFuncPL(void *p);
public:
	PLine(int np);
	virtual ~PLine();
	//int seg(){return n_seg;}
	void map(int seg,int proc){map_t[seg]=proc;};
//++
	void segmet_range(int from,int to){seg_from=from;seg_to=to;}
	void initial_iter(int num){ini_iter=num;}
	
	virtual bool next(int it)=0;
	virtual void prcseg(int it,int seg)=0;
public:	
	void send(void*,size_t){};
	void recv(void*,size_t){};
	
	virtual void send_seg(int seg)=0;
	virtual void recv_seg(int seg)=0;
	
	void run();
private:
	int n_proc;
	int* map_t;
	pthread_t* thread;
	pthread_attr_t attr;
	pthread_cond_t* event;
	pthread_mutex_t* event_mut;
	pthread_mutex_t cs;
	bool* left;
	bool* right;
	volatile bool done;
	volatile int cur_proc;
	volatile int last_it;
private:
	int ini_iter;
	int seg_from;
	int seg_to;
};

}
#endif
