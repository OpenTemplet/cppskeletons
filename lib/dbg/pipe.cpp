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

#include <malloc.h>
#include <string.h>

namespace TEMPLET {

PLine::PLine(int np):n_proc(np)
{
	buf=malloc(ALLOC_SIZE);
	size=ALLOC_SIZE;
	cur=0;
	ini_iter=seg_from=seg_to=0;
}

PLine::~PLine()
{
	free(buf);
}

void PLine::send(void*p,size_t s)
{
	while(cur+s>size){buf=realloc(buf,size+ALLOC_SIZE);size+=ALLOC_SIZE;}
	memmove((char*)buf+cur,p,s);cur+=s;
}

void PLine::recv(void*p,size_t s)
{
	memmove(p,(char*)buf+cur,s);cur+=s;
}

void PLine::run()
{
	int it=ini_iter;
	while(next(it)){
		for(int i=seg_from;i<=seg_to;i++){
			prcseg(it,i);
			cur=0;send_seg(i);
			cur=0;recv_seg(i);
		}
		it++;
	}
}

}