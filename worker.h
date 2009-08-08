
#ifndef _WORKER_H_
#define _WORKER_H_

#include "tqueue.h"

struct WorkRequest {
	virtual int64_t run() { return 0; }
};

class Worker {
	tqueue<WorkRequest> request;
	tqueue<int64_t> response;
	int num_threads;
	pthread_t thread[MAX_THREADS];

	int req_count;
	int res_count;

	bool running;

public:
	Worker(int num){
		num_threads = num;
		running = true;

		req_count = 0;
		res_count = 0;

		if(num_threads > 1)
			for(int i = 0; i < num_threads; i++)
				pthread_create(&(thread[i]), NULL, (void* (*)(void*)) threadRunner, this);
	}

	~Worker(){
		running = false;
		request.nonblock();

		if(num_threads > 1)
			for(int i = 0; i < num_threads; i++)
				pthread_join(thread[i], NULL);
	}

	static void * threadRunner(void * blah){
		Worker * w = (Worker *) blah;
		WorkRequest * req;
		int64_t * ret;
		
		while(w->running && (req = w->request.pop())){
			ret = new int64_t;

			*ret = req->run();

			w->response.push(ret);

			delete req;
		}
		return NULL;
	}

	void add(WorkRequest * req){
		req_count++;
		request.push(req);
	}

	int64_t wait(){
		int64_t ret = 0;

		if(num_threads > 1){
			while(res_count < req_count){
				int64_t * v = response.pop();
				ret += *v;
				delete v;
				res_count++;
			}
		}else{
			WorkRequest * req;
			while((req = request.pop(0))){
				ret += req->run();
				delete req;
				res_count++;
			}
		}

		return ret;
	}
};

#endif

