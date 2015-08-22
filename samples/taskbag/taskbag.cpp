/*--------------------------------------------------------------------------*/
/*  Copyright 2010-2015 Sergey Vostokin                                     */
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
#include <stdio.h>

#include <iostream>
using namespace std;

// параллельное умножение матриц
// с использованием портфеля задач

const int P=10;//число рабочих процессов (не используется в MPI)
const int N=10;
double a[N][N],b[N][N],c[N][N];

class TaskBag:public TEMPLET::TBag{
public:
	class TaskBagTask:public TBag::Task{
	public:
		TaskBagTask():TBag::Task(){}
		virtual~TaskBagTask(){}

		void send_task()  {send(&num,sizeof(num));}// запись задачи в поток  (только для распределенной реализации)
		void recv_task()  {recv(&num,sizeof(num));}// четние задачи из потока (только для распределенной реализации)
		void send_result(){send(&num,sizeof(num)); send(c[num],sizeof(double)*N);}// запись результата в поток (только для распределенной реализации)
		void recv_result(){recv(&num,sizeof(num)); recv(c[num],sizeof(double)*N);}// чтение результата из потока (только для распределенной реализации)

		int num;// номер вычисляемой строки
	};
public:
	TaskBag(int num_prc,int argc, char* argv[]):TBag(num_prc,argc,argv){
		cur=0;//номер текущей строки в матрице С
	}
	virtual ~TaskBag(){}
	TBag::Task* createTask(){return new TaskBagTask;}

	bool if_job(){return cur<N;}
	void put(Task*t){TaskBagTask* mt=(TaskBagTask*)t; }
	void get(Task*t){TaskBagTask* mt=(TaskBagTask*)t; mt->num=cur++;}
	void proc(Task*t){TaskBagTask* mt=(TaskBagTask*)t;
		int i=mt->num;
		for(int j=0;j<N;j++){// параллельное вычисление строки матрицы C
			c[i][j]=0.0;
			for(int k=0;k<N;k++)c[i][j]+=a[i][k]*b[k][j];
		}
	}

	int cur;//номер текущей строки в матрице С
};

int main(int argc, char* argv[])
{
	TaskBag bag(P,argc,argv);//используем P рабочих процессов(не используется в MPI)

	// инициализация
	for(int i=0;i<N;i++)
		for(int j=0;j<N;j++)a[i][j]=N*i+j;

	for(int i=0;i<N;i++)
		for(int j=0;j<N;j++)b[i][j]=N*i+j;

	// параллельное умножение матриц
	bag.run();
	cout<<"\nspeedup = "<<bag.speedup();//1-при логической отладке,
	//P-при параллельном выполении,
	//предсказываемое значение ускорения для данного P при дискретно-событийной эмуляции вычислений
	//внимание! режим эмуляции доступен только для ОС Windows 
	
	cout<<"\nduration = "<<bag.duration()<< " sec\n";

	// вывод результата параллельного умножения
	// отключить для больших N
	cout<<"\nC(parallel)=\n";
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			cout<<c[i][j]<<" ";
		}
		cout<<'\n';
	}

	// очистка матрицы С
	for(int i=0;i<N;i++)
	for(int j=0;j<N;j++) c[i][j]=0.0;
	
	// последовательное умножение матриц
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			c[i][j]=0.0;
			for(int k=0;k<N;k++)c[i][j]+=a[i][k]*b[k][j];
		}
	}

	// вывод результата последовательного умножения
	// отключить для больших N
	cout<<"\nC(serial)=\n";
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			cout<<c[i][j]<<" ";
		}
		cout<<'\n';
	}
	return 0;
}