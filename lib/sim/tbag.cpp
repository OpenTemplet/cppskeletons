/*--------------------------------------------------------------------------*/
/*  Copyright 2014 Sergey Vostokin, Vladislav Martyniuk                     */
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

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <intrin.h>
#pragma intrinsic(__rdtsc)

#include <windows.h>
#include <iostream>

namespace TEMPLET {

TBag::TBag(int num_prc,int argc, char* argv[]) : _curr_time(0.0),_user_time(0.0),_duration(0.0),_num_proc(num_prc)
{
	_tasks = new Task*[num_prc];
	_events = new Evt[num_prc];
}

TBag::~TBag()
{
	delete[] _tasks;
	delete[] _events;
}

void TBag::run()
{
	for (int i = 0; i < _num_proc; ++i)
	{
		_events[i]._offset=i;

		_tasks[i] = createTask();
		_tasks[i]->offset=i;
		_bids.push(_tasks[i]);
	}

	Evt *e;
	Task *task;
	double duration;

	_busy=false;
	_curr_time = 0.0;
	_user_time=0.0;
	_event._type = MASTER;
	plan(&_event, 0.0);

	///////// установка привязки
	if(!SetThreadAffinityMask(GetCurrentThread(),0x1)){
		printf("\nSetThreadAffinityMask FAILED\n");return;
	}

	///////// установка приоритета
	if(!SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST)){
		printf("\nTHREAD_PRIORITY_ABOVE_NORMAL FAILED\n");return;
	}

	LARGE_INTEGER frequency;
	LARGE_INTEGER t1,t2;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&t1);

	while(!_calendar.empty())
	{
		e=_calendar.top();
		_calendar.pop();
		
		assert(_curr_time <= e->_plan_time);

		_curr_time=e->_plan_time;
		e->_planed=false;

		switch(e->_type)
		{
			case MASTER:
				if (!_results.empty())
				{
					_busy=true;

					task = _results.front();
					_results.pop();

					put(task,duration);
					_user_time+=duration;

					_events[task->offset]._type=PUT;
					plan(&_events[task->offset],duration);
				}
				else if(!_bids.empty())
				{
					_busy=true;

					task = _bids.front();
					_bids.pop();

					_events[task->offset]._if_job=if_job(duration);
					_user_time+=duration;

					_events[task->offset]._type=IF_JOB;
					plan(&_events[task->offset],duration);
				}
				
				e->_type=DEFAULT;
				break;
			
			case IF_JOB:
				if(e->_if_job){
					get(_tasks[e->_offset],duration);
					_user_time+=duration;

					e->_type=GET;
					plan(e,duration);
				}
				else{
					_bids.push(_tasks[e->_offset]);
					e->_type=DEFAULT;

					_busy=false;
				}
				break;

			case PUT:
				e->_if_job=if_job(duration);
				_user_time+=duration;

				e->_type=IF_JOB;
				plan(e,duration);
				break;

			case GET:
				proc(_tasks[e->_offset],duration);
				_user_time+=duration;
				
				e->_type=PROC;
				plan(e,duration);

				_event._type=MASTER;
				plan(&_event,0);

				_busy=false;
				break;
				
			case PROC:
				_results.push(_tasks[e->_offset]);
				e->_type=DEFAULT;
				
				if(!_busy && _event._type==DEFAULT){
					_event._type=MASTER;
					plan(&_event,0);
				}
				break;

			case DEFAULT: 
				assert(0);
				break;
		}
	}
	
	QueryPerformanceCounter(&t2);
	_duration=(double)(t2.QuadPart-t1.QuadPart)/frequency.QuadPart;

	for (int i = 0; i < _num_proc; ++i)	delete _tasks[i];
}

double TBag::speedup()
{
	return _user_time/_curr_time;
}

double TBag::duration()
{
	return _duration;
}

void TBag::plan(Evt* e, double t)
{
	assert(!e->_planed);
	e->_plan_time=_curr_time + t;
	e->_planed = true;
	_calendar.push(e);
}

bool TBag::if_job(double& t)
{
	unsigned long long ticks = __rdtsc();
	bool res = if_job();
	ticks = __rdtsc() - ticks;
	t = (double)ticks;
	return res;
}

void TBag::put(Task* task, double&  t)
{
	unsigned long long ticks = __rdtsc();
	put(task);
	ticks = __rdtsc() - ticks;
	t = (double)ticks;
}

void TBag::get(Task* task, double& t)
{
	unsigned long long ticks = __rdtsc();
	get(task);
	ticks = __rdtsc() - ticks;
	t = (double)ticks;
}

void TBag::proc(Task* task, double& t)
{
	unsigned long long ticks = __rdtsc();
	proc(task);
	ticks = __rdtsc() - ticks;
	t = (double)ticks;
}

}