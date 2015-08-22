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

#include <cstddef>

namespace TEMPLET {

#define ALLOC_SIZE 1024

class PLine{
public:
	PLine(int np);//--����� ���������(�������� ���������)
					//++����� ���������
	virtual ~PLine();
	//int seg(){return n_proc;}
	void map(int process,int processor){};//--���������� �������� �� ���������
								//++���������� �������� �� ���������
	//++
	void segmet_range(int from,int to){seg_from=from;seg_to=to;}
	void initial_iter(int num){ini_iter=num;}

	virtual bool next(int it)=0;//������� true ���� ��������� ��������� ��� ���� ��������
	virtual void prcseg(int it,int seg)=0;//��������� ������� seg �� �������� it
public:	
	void send(void*,size_t);//��������� ����������/�������������� ���������
	void recv(void*,size_t);
	
	virtual void send_seg(int seg)=0;//�������� ������ ��� ����������/�������������� ���������
	virtual void recv_seg(int seg)=0;

	void run();
private:
	int n_proc;
private:
	int ini_iter;
	int seg_from;
	int seg_to;
private:
	void*  buf;
	size_t size;
	size_t cur;
};

}
#endif
