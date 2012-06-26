#include <gigaspaces.h>
#include "HelloWorldMessage.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

int timeval_subtract (timeval* result, timespec* x, timespec* y);

timespec diff(timespec* start, timespec* end);
     
int main(int argc,char **argv)
{
	SpaceFinder finder;
//	std::string spaceUrl = "/./helloWorldEmbeddedSpace?groups=CPP"; 
	std::string spaceUrl = "jini://bubi3/*/mySpace"; 

	if(argc > 1)	spaceUrl = argv[1];
	std::cout << "---------- Hello World running................" << std::endl;

	try
	{
		///////////////////////////////////////////
		// retrieving a space proxy
		///////////////////////////////////////////

		std::cout << "Retrieving a space proxy to '" << spaceUrl << "'" << std::endl;
		SpaceProxyPtr space ( finder.find(spaceUrl) );
		if (space != NULL)
			std::cout << "Retrieved space proxy ok " << std::endl;
		else
		{
			std::cout << "ERROR: space proxy is NULL" << std::endl;
			std::cout << "Press Enter to exit..." << std::endl;
			std::cin.get();
			exit(EXIT_FAILURE);
		}

		space->clear(NULL);

		///////////////////////////////////////////
		// doing snapshot for Message
		///////////////////////////////////////////

		Message messageTemplate;
		space->snapshot(&messageTemplate);

		std::cout << "Did snapshot for Message class" << std::endl;

		///////////////////////////////////////////
		// writing a message to the space
		///////////////////////////////////////////

		time_t seconds_start = time (NULL);
		timespec tS, dS;

		MessagePtr msg( new Message() );

		timespec tsreq, tsrem;
		tsreq.tv_sec = 0;
		tsreq.tv_nsec = 10000000;		

		int batch_count = 100;
		int write_count = 0;
		MessagePtr m( new Message() );
		
		int test_duration_in_sec = 60;

                tS.tv_sec = 0;
                tS.tv_nsec = 0;
//                clock_settime(CLOCK_THREAD_CPUTIME_ID, &tS);
                clock_settime(CLOCK_MONOTONIC, &tS);
                clock_gettime(CLOCK_MONOTONIC, &dS);
		std::cout << "start time is: " << dS.tv_sec << ":" <<  dS.tv_nsec << std::endl;
	
		m->content = "0123456789012345678901234567890123456789012345678901234567890123";
		m->content += "0123456789012345678901234567890123456789012345678901234567890123";
		m->content += "0123456789012345678901234567890123456789012345678901234567890123";
		m->content += "0123456789012345678901234567890123456789012345678901234567890123";
		m->content += "0123456789012345678901234567890123456789012345678901234567890123";
		m->content += "0123456789012345678901234567890123456789012345678901234567890123";
		m->content += "0123456789012345678901234567890123456789012345678901234567890123";
		m->content += "0123456789012345678901234567890123456789012345678901234567890123";
		
		int max_batch = 4096;
		timespec *ptS = (timespec*) malloc(max_batch * sizeof(timespec));
		int tSc = 0;

		// run the test for 10 seconds
		do {

	                for (int i = 0; i < batch_count; i++) {
        	               m->id = i;
//                	        m->content = "Hello World";
                       	space->write(m.get(), NULL_TX, 5000000);
	                }
			write_count += batch_count;
			// sleep for 100 millisec - we will have 10 batched of 100 objects , effectivly close to 1000 write per sec TP
			// our measurment will include the sleep time. We will subtract it later
        	       nanosleep(&tsreq, &tsrem);

			clock_gettime(CLOCK_MONOTONIC, &tS);
//			std::cout << "batch time taken is: " << (tS.tv_sec - dS.tv_sec) << ":" <<  tS.tv_nsec << std::endl;
			// ignore the first 2 seconds results
			if (tS.tv_sec > 4) {

				(ptS + tSc)->tv_sec = tS.tv_sec;
				(ptS + tSc)->tv_nsec = tS.tv_nsec;

				tSc++;
			}	
		}
		while ((tS.tv_sec - dS.tv_sec) < test_duration_in_sec );


		std::cout << "Batch time taken is: " << tS.tv_sec << ":" <<  tS.tv_nsec << std::endl;
		std::cout << "Batch messages written: " << write_count << std::endl;
		std::cout << "Batch batches written: " << tSc << std::endl;

		double bps = ((double) tSc / (double) tS.tv_sec);
		std::cout << "Batch batches/sec: " << bps  << write_count << std::endl;

		double tsa[tSc];
		memset(tsa, 0, tSc * sizeof(double));		
		
		for (int t = 1; t < tSc; t++) {
			timespec dts = diff((ptS + t - 1), (ptS + t));
			//double nsec = (double)dts.tv_nsec / (double)1000000000;
			// here we subtract the sleep time from the latency measurment 
			double nsec = ((double)(dts.tv_nsec - tsreq.tv_nsec) / (double) 1000  / (double)batch_count);

			double td;
			for (int tt = 0; tt < tSc; tt++) {
				if (nsec < tsa[tt]) {
					td = tsa[tt];
					tsa[tt] = nsec;
					for (int ttt = tt + 1; ttt < tSc; ttt++) {
						nsec = tsa[ttt];							
						tsa[ttt] = td;
						td = nsec;
					}
					tt = tSc;
				}
				else {
					if (tsa[tt] ==0)
					{
						tsa[tt] = nsec;
						tt = tSc;
					}
				}
			}
		}

		int step = tSc / 100;
		std::cout << " we have " << tSc << " data points " << " we will sample every " << step << " data point " << std::endl;
		
		std::cout << " Percentile | Latency[microsec]" << std::endl;
		// we have tSc data points. generate histograme
		int percentile = 1;
		for (int dtsa = 0; dtsa < tSc; dtsa++) {
			if (dtsa > 0)  
			{
				if (dtsa % step ==0)
				{
					std::cout << percentile << " | " << tsa[dtsa]  << std::endl;
					percentile ++;
				}
			}
		}
		free(ptS);	
//		free(tsa);
	} 
	catch(XAPException &e){
		e.toLogger();
	}

}


int
timeval_subtract (timeval* result, timespec* x, timespec* y)
{
       /* Perform the carry for the later subtraction by updating y. */
       if (x->tv_nsec < y->tv_nsec) {
         int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
         y->tv_nsec -= 1000000000 * nsec;
         y->tv_sec += nsec;
       }
       if (x->tv_nsec - y->tv_nsec > 1000000000) {
         int nsec = (x->tv_nsec - y->tv_nsec) / 1000000000;
         y->tv_nsec += 1000000000 * nsec;
         y->tv_sec -= nsec;
       }
     
       /* Compute the time remaining to wait.
          tv_usec is certainly positive. */
       result->tv_sec = x->tv_sec - y->tv_sec;
       result->tv_usec = x->tv_nsec - y->tv_nsec;
     
       /* Return 1 if result is negative. */
       return x->tv_sec < y->tv_sec;
}     

timespec diff(timespec* start, timespec* end)
{
	timespec temp;
	if ((end->tv_nsec - start->tv_nsec) < 0) {
		temp.tv_sec = end->tv_sec - start->tv_sec - 1;
		temp.tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
	} else {
		temp.tv_sec = end->tv_sec - start->tv_sec;
		temp.tv_nsec = end->tv_nsec - start->tv_nsec;
	}
	return temp;
}
