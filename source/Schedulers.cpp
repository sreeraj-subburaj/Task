#include "../headers/Schedulers.h"
#include <climits>
#include <cmath>
#include <iostream>
Scheduler::Scheduler() {
    next_pcb_index = -1;
    ready_queue = NULL;
    blocked_queue = new DList<PCB>();
}

//constructor for non-RR algs
Scheduler::Scheduler(DList<PCB> *rq, CPU *cp, int alg){
    ready_queue = rq;
    cpu = cp;
    dispatcher = NULL;
    next_pcb_index = -1;
    algorithm = alg;
    blocked_queue = new DList<PCB>();
}

//constructor for RR alg
Scheduler::Scheduler(DList<PCB> *rq, CPU *cp, int alg, int tq){
    ready_queue = rq;
    cpu = cp;
    dispatcher = NULL;
    next_pcb_index = -1;
    algorithm = alg;
    timeq = timer = tq;
    blocked_queue = new DList<PCB>();;
}

//dispatcher needed to be set after construction since they mutually include each other
//can only be set once
void Scheduler::setdispatcher(Dispatcher *disp) {
    if(dispatcher == NULL) dispatcher = disp;
}

//dispatcher uses this to determine which process in the queue to grab
int Scheduler::getnext() {
    return next_pcb_index;
}

//switch for the different algorithms
void Scheduler::execute() {
    if (timer > 0) timer -= 0.5;
    if (ready_queue->size()) {
       
                // Continue scheduling the process
                switch (algorithm) {
                    case 0:
                        fcfs();
                        break;
                    case 1:
                        srtf();
                        break;
                    case 2:
                        rr();
                        break;
                    case 3:
                        pp();
                        break;
                    case 4:
                        pr();
                        break;
                    default:
                        break;
                }
            }
 }


//simply waits for cpu to go idle and then tells dispatcher to load next in queue
void Scheduler::fcfs() {
    next_pcb_index = 0;
    if(cpu->isidle())
    dispatcher->interrupt();
}

//shortest remaining time first
void Scheduler::srtf() {
    float short_time;
    int short_index = -1;

    //if cpu is idle, initialize shortest time to head of queue
    if(!cpu->isidle()) short_time = cpu->getpcb()->time_left;
    else {
        short_time = ready_queue->gethead()->time_left;
        short_index = 0;
    }

    //now search through queue for actual shortest time
    for(int index = 0; index < ready_queue->size(); ++index){
        if(ready_queue->getindex(index)->time_left < short_time){ //less than ensures FCFS is used for tie
            short_index = index;
            short_time = ready_queue->getindex(index)->time_left;
        }
    }

    //-1 means nothing to schedule, only happens if cpu is already working on shortest
    if(short_index >= 0) {
        next_pcb_index = short_index;
        dispatcher->interrupt();
    }
}

//round robin, simply uses timer and interrupts dispatcher when timer is up, schedules next in queue
void Scheduler::rr() {
    if(cpu->isidle() || timer <= 0){
        timer = timeq;
        next_pcb_index = 0;
        dispatcher->interrupt();
    }
}



void Scheduler::pp() {
    PCB* current_pcb = cpu->getpcb();
    int highest_priority = INT_MAX;
    int highest_priority_index = -1;
    bool preempt = false;
    for (int i = 0; i < ready_queue->size(); ++i) {
        PCB* pcb = ready_queue->getindex(i);
        if (pcb != nullptr && pcb->priority < highest_priority) {
            highest_priority = pcb->priority;
            highest_priority_index = i;
        }
    }
    if (cpu->isidle()) {
        preempt = true;
    } else if (timer <= 0 && highest_priority_index != -1) {
        if (highest_priority < current_pcb->priority) {
            preempt = true;
        }
    } else if (!cpu->isidle() && highest_priority < current_pcb->priority) {
        preempt = true;
    }

    if (preempt) {

        next_pcb_index = highest_priority_index;
        dispatcher->interrupt(); 
        timer = timeq; 
    } else {
        timer -= 0.5;
    }
}


void Scheduler::pr() {
        if (cpu->isidle() || timer <= 0) {
            timer = timeq;
            if (ready_queue->size() > 0) {
                // Select a random process from the ready queue
                int random_index = rand() % ready_queue->size();
                next_pcb_index = random_index;
                dispatcher->interrupt();
            }
        }
    }

/*
 *
 * Dispatcher Implementation
 *
 */
Dispatcher::Dispatcher(){
    cpu = NULL;
    scheduler = NULL;
    ready_queue = NULL;
    clock = NULL;
    _interrupt = false;

}

Dispatcher::Dispatcher(CPU *cp, Scheduler *sch, DList<PCB> *rq, Clock *cl) {
    cpu = cp;
    scheduler = sch;
    ready_queue = rq;
    clock = cl;
    _interrupt = false;

};

//function to handle switching out pcbs and storing back into ready queue
PCB* Dispatcher::switchcontext(int index) {
    PCB* old_pcb = cpu->pcb;
    PCB* new_pcb = new PCB(ready_queue->removeindex(scheduler->getnext()));
    cpu->pcb = new_pcb;
    return old_pcb;
}

//executed every clock cycle, only if scheduler interrupts it
void Dispatcher::execute() {
    if(_interrupt) {
        PCB* old_pcb = switchcontext(scheduler->getnext());
        if(old_pcb != NULL){ //only consider it a switch if cpu was still working on process
            old_pcb->num_context++;
            cpu->getpcb()->wait_time += .5;
            clock->step();
            ready_queue->add_end(*old_pcb);
            delete old_pcb;
        }
        _interrupt = false;
    }
}

//routine for scheudler to interrupt it
void Dispatcher::interrupt() {
    _interrupt = true;
}
