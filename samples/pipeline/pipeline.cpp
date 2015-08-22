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

#include "pipe.h"
#include <stdio.h>

#include <iostream>
using namespace std;

// параллельноый алгоритм метода Зейделя для задачи теплопроводности с г/у первого рода

const int T=50;
const int N=6;
double a[N][N];

class Pipe: public TEMPLET::PLine{
public:
	Pipe(int ns):PLine(ns){
		segmet_range(1,N-2);
		initial_iter(0);
	}
	~Pipe(){}
	
	bool next(int it){return it<T;}
	
	void prcseg(int it,int seg){
		for(int i=1;i<N-1;i++)	a[i][seg]=0.25*(a[i-1][seg]+a[i+1][seg]+a[i][seg-1]+a[i][seg+1]);
	}
	void send_seg(int seg){}
	void recv_seg(int seg){}
};

int main(int argc, char* argv[])
{
	// инициализация матрицы
	for(int i=0;i<N;i++){
		a[i][0]=0.0;
		a[i][N-1]=100.0;
	}

	for(int j=1;j<N-1;j++)
		a[0][j]=a[N-1][j]=100.0/(N-1)*j;
	
	for(int i=1;i<N-1;i++)for(int j=1;j<N-1;j++)a[i][j]=0.0;

	cout<<"A=\n";
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			cout.width(10);cout.precision(5);
			cout<<a[i][j]<<" ";
		}
		cout<<"\n";
	}

	cout<<"\n";

	// последовательный алгоритм
	for(int t=0;t<T;t++)
	for(int j=1;j<N-1;j++)
	for(int i=1;i<N-1;i++)
		a[i][j]=0.25*(a[i-1][j]+a[i+1][j]+a[i][j-1]+a[i][j+1]);

	cout<<"A'=\n";
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			cout.width(10);cout.precision(5);
			cout<<a[i][j]<<" ";
		}
		cout<<"\n";
	}

	cout<<"------------------------------------\n";

	// повторная инициализация матрицы
	for(int i=0;i<N;i++){
		a[i][0]=0.0;
		a[i][N-1]=100.0;
	}

	for(int j=1;j<N-1;j++)
		a[0][j]=a[N-1][j]=100.0/(N-1)*j;
	
	for(int i=1;i<N-1;i++)for(int j=1;j<N-1;j++)a[i][j]=0.0;

	//параллельный алгоритм
	Pipe pipe(N-2);
	pipe.run();

	cout<<"A'=\n";
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			cout.width(10);cout.precision(5);
			cout<<a[i][j]<<" ";
		}
		cout<<"\n";
	}

	return 0;
}