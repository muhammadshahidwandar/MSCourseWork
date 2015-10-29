///////////////////////Muhammad Shahid CE131001/////////////////////
/////////////////////FIR Filter Implementation//////////////////////
#include<stdio.h>
#include<conio.h>
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */
#define N 10
#define M 10000
//#define CLOCKS_PER_SEC;
void main()
{ 
	int x[M];
	float y[M];
    float h[N];
	int cbuf[N];
	int i,j;
	int x_index,newst;

	clock_t start, end;
	double cpu_time_used;
	//double seconds;
	////////initialization
	 x_index = 0;
	 newst   = 0;
	 //time(&start);
	 start = clock();
	 ////type casting to remove truncation warning
	h[0] =(float)0.0201;
	h[1] =(float)0.0201;
	h[2] =(float)0.2309 ;
	h[3] =(float)0.2309;
	h[4] =(float)0.4981;
	h[5] =(float)0.4981;
	h[6] =(float)0.2309;
	h[7] =(float)0.2309;
	h[8] =(float)0.0201;
	h[9] =(float)0.0201;
	
	for(i=0;i<N;i++)
         cbuf[i]=0;
	for(i=0;i<M;i=i+20)
	{
		for(j=0;j<N;j++)
		{
			x[i+j]=1;
			x[i+j+10]=0;
		}
	}
//////////////initialization completed/////////////////

////////////////////////////
	for(i=0;i<M;i++)
	{   
		cbuf[newst]=x[i];
		y[i]=0;
		x_index = newst;
	 for(j=0;j<N;j++)
	 {
		 y[i]+=h[j]*cbuf[x_index];
		 x_index-=1;
		 if(x_index==-1) 
			 x_index = N-1;
		// printf(" %f\n", y[i]);
	 }
		// printf(" %f\n",y[i]);
	 newst+=1;
if(newst == N)
newst = 0;

	}
//time(&now);
	 end = clock();

cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
//seconds = difftime(now,start);

 printf ("%f seconds program execution time\n", cpu_time_used);
	
for(i=0;i<20;i++)	
printf(" %f\n", y[i]);

_getch();
}