/*
 * kgarten.c
 *
 * A kindergarten simulator.
 * Bad things happen if teachers and children
 * are not synchronized properly.
 *
 * 
 * Author: 
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 *
 * Additional Authors:
 * Stefanos Gerangelos <sgerag@cslab.ece.ntua.gr>
 * Anastassios Nanos <ananos@cslab.ece.ntua.gr>
 * Operating Systems course, ECE, NTUA
 *
 */

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>


/* START OF MODIFICATIONS */

#define THRESHOLD 10  //meta apo 10 monades xronou pairnei proteraiotita o daskalos pou perimenei 

/* END OF MODIFICATIONS */

/* 
 * POSIX thread functions do not return error numbers in errno,
 * but in the actual return value of the function call instead.
 * This macro helps with error reporting in this case.
 */
#define perror_pthread(ret, msg) \
	do { errno = ret; perror(msg); } while (0)

/* START OF MODIFICATIONS */

//tha ftiaksoume dyo oures (FIFO) se pinaka (me kykliko tropo), gia tis opoies kseroume 
//to maximum arithmo stoixeiwn pou borei na ginei enqueue (paidia/daskaloi)
//i mia oura afora daskalous pou perimenoun na vgoun
//kai i alli paidia pou perimenoun na boun
//wste na eksasfalizetai peperasmeni anamoni

struct queue {
	int* queue;    	 //i oura
	int isempty;  	 //an einai empty tote einai 1 allios 0
	int front;     	 //index prwtou
	int back;	   	 //index teleutaiou
	int currently_in;  //posoi einai mesa
	int size;          //megethos ouras
};

/* END OF MODIFICATIONS */

/* A virtual kindergarten */
struct kgarten_struct {

	/* START OF MODIFICATIONS */

	int child_threads;        //mas xreiazetai se kapoia simeia gia na vroume indexes
	pthread_cond_t cond;
	int time;       		  //theoroume oti mia monada xronou pernaei se kathe auksisi/meiwsi paidiou/daskalou
	struct queue* tq;         //oura daskalwn pou theloun na vgoun
	struct queue* cq;	        //oura paidiwn pou theloun na boun
	int* teachers_wait_start; //xronos pou arixei i anamoni twn daskalwn gia eksodo
					  //kathe index antistoixei ston (index + child_threads) daskalo
					  //otan enas daskalos den perimenei na vgei i timi einai -10					

	/* END OF MODIFICATIONS */

	/* ... */

	/*
	 * You may NOT modify anything in the structure below this
	 * point. 
	 */
	int vt;
	int vc;
	int ratio;

	pthread_mutex_t mutex;
};

/*
 * A (distinct) instance of this structure
 * is passed to each thread
 */
struct thread_info_struct {
	pthread_t tid; /* POSIX thread id, as returned by the library */

	struct kgarten_struct *kg;
	int is_child;  /* Nonzero if this thread simulates children, zero otherwise */

	int thrid;     /* Application-defined thread id */
	int thrcnt;
	unsigned int rseed;
};

/* START OF MODIFICATIONS */

//synartiseis enqueue, dequeue kai seefront gia tis sygekrimenes oures poy theloume se autin tin askisi
//ypenthimizetai oti xrisimopoiountai pinakes me kykliko tropo

void enqueue (struct queue *q, int data)
{
	if (q->currently_in == q->size) 
	{
		perror("enqueue in full queue");
		exit(1);
	}
	if (q->isempty == 1) 
	{
		q->front = 0;
		q->back = 0;
		(q->queue)[0] = data;
		q->isempty = 0;
	}
	else 
	{
		q->back = ((q->back) + 1) % (q->size);
		(q->queue)[q->back] = data;
	}
	(q->currently_in)++;
}

void dequeue (struct queue *q)
{
	if (q->isempty) 
	{
		perror("dequeue in empty queue");
		exit(1);
	}
	q->front = ((q->front) + 1) % (q->size);
	(q->currently_in)--;
	if ((q->currently_in) == 0) q->isempty = 1;
}

int seefront (struct queue *q)
{
	if (q->isempty) return -1;
	return (q->queue)[q->front];
}

/* END OF MODIFICATIONS */

int safe_atoi(char *s, int *val)
{
	long l;
	char *endp;

	l = strtol(s, &endp, 10);
	if (s != endp && *endp == '\0') {
		*val = l;
		return 0;
	} else
		return -1;
}

void *safe_malloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL) {
		fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
			size);
		exit(1);
	}

	return p;
}

void usage(char *argv0)
{
	fprintf(stderr, "Usage: %s thread_count child_threads c_t_ratio\n\n"
		"Exactly two argument required:\n"
		"    thread_count: Total number of threads to create.\n"
		"    child_threads: The number of threads simulating children.\n"
		"    c_t_ratio: The allowed ratio of children to teachers.\n\n",
		argv0);
	exit(1);
}

void bad_thing(int thrid, int children, int teachers)
{
	int thing, sex;
	int namecnt, nameidx;
	char *name, *p;
	char buf[1024];

	char *things[] = {
		"Little %s put %s finger in the wall outlet and got electrocuted!",
		"Little %s fell off the slide and broke %s head!",
		"Little %s was playing with matches and lit %s hair on fire!",
		"Little %s drank a bottle of acid with %s lunch!",
		"Little %s caught %s hand in the paper shredder!",
		"Little %s wrestled with a stray dog and it bit %s finger off!"
	};
	char *boys[] = {
		"George", "John", "Nick", "Jim", "Constantine",
		"Chris", "Peter", "Paul", "Steve", "Billy", "Mike",
		"Vangelis", "Antony"
	};
	char *girls[] = {
		"Maria", "Irene", "Christina", "Helena", "Georgia", "Olga",
		"Sophie", "Joanna", "Zoe", "Catherine", "Marina", "Stella",
		"Vicky", "Jenny"
	};

	thing = rand() % 4;
	sex = rand() % 2;

	namecnt = sex ? sizeof(boys)/sizeof(boys[0]) : sizeof(girls)/sizeof(girls[0]);
	nameidx = rand() % namecnt;
	name = sex ? boys[nameidx] : girls[nameidx];

	p = buf;
	p += sprintf(p, "*** Thread %d: Oh no! ", thrid);
	p += sprintf(p, things[thing], name, sex ? "his" : "her");
	p += sprintf(p, "\n*** Why were there only %d teachers for %d children?!\n",
		teachers, children);

	/* Output everything in a single atomic call */
	printf("%s", buf);
}

void child_enter(struct thread_info_struct *thr)
{
	if (!thr->is_child) {
		fprintf(stderr, "Internal error: %s called for a Teacher thread.\n",
			__func__);
		exit(1);
	}

	fprintf(stderr, "THREAD %d: CHILD ENTER\n", thr->thrid); 

	pthread_mutex_lock(&thr->kg->mutex);
	//fprintf(stderr, "THREAD %d: CHILD ENTER\n", thr->thrid); used for debugging

	/* START OF MODIFICATIONS */

	int first_teacher_to_go_wait_start = -10; //arxikopoieitai etsi wste an to afairesoume apo to time na exoume 
								//apotelesma megalytero apo time kai na to aporipsoume ws invalid

	int flag;   //otan einai 1 yparxoun daskaloi pou exoun perimenei perissotero apo threshold
	int t = thr->kg->vt;       //teachers
	int c = thr->kg->vc;       //children
	int ratio = thr->kg->ratio; //ratio
	enqueue(thr->kg->cq, thr->thrid); //vazoume to paidi stin oura twn paidiwn pou theloun na boun
	int kid_turn = seefront(thr->kg->cq); //vlepoume pianou paidiou seira einai

	/* apo edw kai mexri tin while vriskoume ton daskalo pou perimenei tin perissoteri wra (diladi ton FIRST IN stin oura)
	kai meta vriskoume posi wra perimenei wste na rythmisoume to flag*/
	int first_teacher_to_go = seefront(thr->kg->tq); 
	if (first_teacher_to_go == -1) flag = 0;   //an einai -1 simainei oti den perimenei kanenas daskalos
	else
	{
		first_teacher_to_go_wait_start = (thr->kg->teachers_wait_start)[first_teacher_to_go - thr->kg->child_threads];
		if ((thr->kg->time - first_teacher_to_go_wait_start > THRESHOLD) && (thr->kg->time - first_teacher_to_go_wait_start <= thr->kg->time)) flag = 1;
		else flag = 0;
	}
	//the following command was used for debugging 
	//printf("		fttgo = %d  wait_start = %d  flag = %d\n", first_teacher_to_go, first_teacher_to_go_wait_start, flag);

	//to paidi perimenei otan:
	//eite den eparkoun oi daskaloi
	//eite perimenoun daskaloi panw apo threshold
	//eite den einai i seira tou na bei
	while ((t < ((c + 1.0) / ratio)) || flag == 1 || kid_turn != thr->thrid)
	{
		pthread_cond_wait(&thr->kg->cond, &thr->kg->mutex);

		/* ksanakanoume oti kaname prin tin while gia epikairopoisi*/

		t = thr->kg->vt;
		c = thr->kg->vc;
		kid_turn = seefront(thr->kg->cq);
		first_teacher_to_go = seefront(thr->kg->tq);
		if (first_teacher_to_go == -1) flag = 0;
		else
		{
			first_teacher_to_go_wait_start = (thr->kg->teachers_wait_start)[first_teacher_to_go - thr->kg->child_threads];
			if (thr->kg->time - first_teacher_to_go_wait_start >= THRESHOLD && thr->kg->time - first_teacher_to_go_wait_start <= thr->kg->time) flag = 1;
			else flag = 0;
		}
	} 
	++(thr->kg->vc);     //auksisi paidiwn
	++(thr->kg->time);   //auksisi xronou
	
	dequeue(thr->kg->cq);			     //vgazoume to paidi apo tin oura
	pthread_cond_broadcast(&thr->kg->cond); //gia na dei to epomeno paidi pou exei seira mipws borei na bei

	/* END OF MODIFICATIONS */

	//the 2 following commands were used for debugging
	//fprintf(stderr, "THREAD %d: CHILD ENTERED\n", thr->thrid); 
	//printf("				time is %d\n", thr->kg->time);
	pthread_mutex_unlock(&thr->kg->mutex);
}

void child_exit(struct thread_info_struct *thr)
{

	if (!thr->is_child) {
		fprintf(stderr, "Internal error: %s called for a Teacher thread.\n",
			__func__);
		exit(1);
	}

	fprintf(stderr, "THREAD %d: CHILD EXIT\n", thr->thrid); 
	
	pthread_mutex_lock(&thr->kg->mutex);             
	--(thr->kg->vc);   
	//fprintf(stderr, "THREAD %d: CHILD EXIT\n", thr->thrid); used for debugging

	/* START OF MODIFICATIONS */

	++(thr->kg->time);  //auksisi xronou
	
	pthread_cond_broadcast(&thr->kg->cond);  //ksipnima olwn gia na ksanaelegksoun tis sinthikes tous

	/* END OF MODIFICATIONS */

	//the 2 following commands were used for debugging
	//fprintf(stderr, "THREAD %d: CHILD EXITED\n", thr->thrid); 
	//printf("				time is %d\n", thr->kg->time);
	pthread_mutex_unlock(&thr->kg->mutex);
}

void teacher_enter(struct thread_info_struct *thr)
{
	if (thr->is_child) {
		fprintf(stderr, "Internal error: %s called for a Child thread.\n",
			__func__);
		exit(1);
	}

	fprintf(stderr, "THREAD %d: TEACHER ENTER\n", thr->thrid); 

	pthread_mutex_lock(&thr->kg->mutex);
	++(thr->kg->vt);

	/* START OF MODIFICATIONS */
	
	//fprintf(stderr, "THREAD %d: TEACHER ENTER\n", thr->thrid); used for debugging 

	++(thr->kg->time);        //auksisi xronou
	
	pthread_cond_broadcast(&thr->kg->cond);  //ksipnima olwn gia na ksanaelegksoun tis sinthikes tous

	/* END OF MODIFICATIONS */

	//the 2 following commands were used for debugging
	//fprintf(stderr, "THREAD %d: TEACHER ENTERED\n", thr->thrid); 
	//printf("				time is %d\n", thr->kg->time);
	pthread_mutex_unlock(&thr->kg->mutex);
}

void teacher_exit(struct thread_info_struct *thr)
{
	if (thr->is_child) {
		fprintf(stderr, "Internal error: %s called for a Child thread.\n",
			__func__);
		exit(1);
	}

	fprintf(stderr, "THREAD %d: TEACHER EXIT\n", thr->thrid); 

	pthread_mutex_lock(&thr->kg->mutex);
	//fprintf(stderr, "THREAD %d: TEACHER EXIT\n", thr->thrid);  used for debugging

	/* START OF MODIFICATIONS */

	int t_index = thr->thrid - thr->kg->child_threads;  //index gia na apothikeusoume xrono enarksis anamonis
	(thr->kg->teachers_wait_start)[t_index] = thr->kg->time;

	
	int t = thr->kg->vt;  //teachers
	int c = thr->kg->vc;  //children
	int ratio = thr->kg->ratio;
	enqueue(thr->kg->tq, thr->thrid);  //bainei o daskalos stin oura autwn pou theloun an vgoun
	int teacher_turn = seefront(thr->kg->tq); //vlepoume pianou seira einai

	//o daskalos den vgainei an:
	//paidia > (daskaloi - 1) * ratio
	//den einai i seira tou

	//the following command was used for debugging
	//printf("		teacher turn is %d thrid is %d\n", teacher_turn, thr->thrid);
	while (c > ((t - 1) * ratio) || teacher_turn != thr->thrid) 
	{
		pthread_cond_wait(&thr->kg->cond, &thr->kg->mutex);

		/* ksanakanoume tis anatheseis mipws exei allaksei kati sto endiameso */
		
		t = thr->kg->vt;
		c = thr->kg->vc;
		teacher_turn = seefront(thr->kg->tq); 
	}
	--(thr->kg->vt);
	++(thr->kg->time);
	
	dequeue(thr->kg->tq);
	(thr->kg->teachers_wait_start)[t_index] = -10; //gia programmatistikous logous wste ston elegxo na exoume invalid 
								     //apotelesma kai na katalavoume oti den perimenei na fygei

	pthread_cond_broadcast(&thr->kg->cond); //se periptwsi pou perimeni allos daskalos pou borei na vgei
	
	/* END OF MODIFICATIONS */

	//the 2 following commands were used for debugging
	//fprintf(stderr, "THREAD %d: TEACHER EXITED\n", thr->thrid); 
	//printf("				time is %d\n", thr->kg->time);
	pthread_mutex_unlock(&thr->kg->mutex);
}

/*
 * Verify the state of the kindergarten.
 */
void verify(struct thread_info_struct *thr)
{
        struct kgarten_struct *kg = thr->kg;
        int t, c, r;

        c = kg->vc;
        t = kg->vt;
        r = kg->ratio;

        fprintf(stderr, "            Thread %d: Teachers: %d, Children: %d\n",
                thr->thrid, t, c);

        if (c > t * r) {
                bad_thing(thr->thrid, c, t);
                exit(1);
        }
}


/* 
 * A single thread.
 * It simulates either a teacher, or a child.
 */
void *thread_start_fn(void *arg)
{
	/* We know arg points to an instance of thread_info_struct */
	struct thread_info_struct *thr = arg;
	char *nstr;									

	fprintf(stderr, "Thread %d of %d. START.\n", thr->thrid, thr->thrcnt); 
	
	nstr = thr->is_child ? "Child" : "Teacher";                            
	for (;;) {
		fprintf(stderr, "Thread %d [%s]: Entering.\n", thr->thrid, nstr); 
		if (thr->is_child)
			child_enter(thr);
		else
			teacher_enter(thr);
	
		fprintf(stderr, "Thread %d [%s]: Entered.\n", thr->thrid, nstr);   

		/*
		 * We're inside the critical section,
		 * just sleep for a while.
		 */
		/* usleep(rand_r(&thr->rseed) % 1000000 / (thr->is_child ? 10000 : 1)); */
        	pthread_mutex_lock(&thr->kg->mutex);
		verify(thr);
        	pthread_mutex_unlock(&thr->kg->mutex);

		usleep(rand_r(&thr->rseed) % 1000000);

		fprintf(stderr, "Thread %d [%s]: Exiting.\n", thr->thrid, nstr);       
		/* CRITICAL SECTION END */

		if (thr->is_child)
			child_exit(thr);
		else
			teacher_exit(thr);

		fprintf(stderr, "Thread %d [%s]: Exited.\n", thr->thrid, nstr);    

		/* Sleep for a while before re-entering */
		/* usleep(rand_r(&thr->rseed) % 100000 * (thr->is_child ? 100 : 1)); */
		usleep(rand_r(&thr->rseed) % 100000);

        	pthread_mutex_lock(&thr->kg->mutex);
		verify(thr);
        	pthread_mutex_unlock(&thr->kg->mutex);
	}

	fprintf(stderr, "Thread %d of %d. END.\n", thr->thrid, thr->thrcnt);
	
	return NULL;
}


int main(int argc, char *argv[])
{
	int i, ret, thrcnt, chldcnt, ratio;
	struct thread_info_struct *thr;
	struct kgarten_struct *kg;
	struct queue *tq;
	struct queue *cq;

	/*
	 * Parse the command line
	 */
	if (argc != 4)
		usage(argv[0]);
	if (safe_atoi(argv[1], &thrcnt) < 0 || thrcnt <= 0) {
		fprintf(stderr, "`%s' is not valid for `thread_count'\n", argv[1]);
		exit(1);
	}
	if (safe_atoi(argv[2], &chldcnt) < 0 || chldcnt < 0 || chldcnt > thrcnt) {
		fprintf(stderr, "`%s' is not valid for `child_threads'\n", argv[2]);
		exit(1);
	}
	if (safe_atoi(argv[3], &ratio) < 0 || ratio < 1) {
		fprintf(stderr, "`%s' is not valid for `c_t_ratio'\n", argv[3]);
		exit(1);
	}


	/*
	 * Initialize kindergarten and random number generator 
	 */
	srand(time(NULL));

	kg = safe_malloc(sizeof(*kg));
	kg->vt = kg->vc = 0;
	kg->ratio = ratio;

	ret = pthread_mutex_init(&kg->mutex, NULL);
	if (ret) {
		perror_pthread(ret, "pthread_mutex_init");
		exit(1);
	}

	/* START OF MODIFICATIONS */

	//arxikopoiiseis twn padwn

	//condition variable:
	ret = pthread_cond_init(&kg->cond, NULL);
	if (ret) {
		perror_pthread(ret, "pthread_cond_init");
		exit(1);
	}

	kg->time = 0; //arxikopoiisi xronou
	
	//arxikopoiisi struct ouras paidiwn kai daskalwn 
	//arxikopoisi pinaka arxwn anamonis twn daskalwn
	//kai error handling
	kg->teachers_wait_start = (int*)malloc((thrcnt - chldcnt) * sizeof(int));
	tq = (struct queue*)malloc(sizeof(struct queue));
	cq = (struct queue*)malloc(sizeof(struct queue));
	if (tq == NULL || cq == NULL || kg->teachers_wait_start == NULL)
	{
		perror("malloc failed");
		exit(1);
	}

	//auti i arxikopoisi ginetai gia programmatistikous logous
	for (i=0; i<(thrcnt - chldcnt); i++) (kg->teachers_wait_start)[i] = -10;

	//oi dyo pinakes stous opoios ylopoieitai i oura
	int teacher_queue[thrcnt - chldcnt];
	int children_queue[chldcnt];

	kg->child_threads = chldcnt; //to xreiazomaste gia euresi indexes pinakwn

	//apo edw mexri to telos twn modifications ginontai
	//oi anatheseis sta struct twn ourwn
	kg->tq = tq;
	kg->cq = cq;

	tq->queue = teacher_queue;
	tq->isempty = 1;
	tq->currently_in = 0;
	tq->size = thrcnt - chldcnt;

	cq->queue = children_queue;
	cq->isempty = 1;
	cq->currently_in = 0;
	cq->size = chldcnt;

	/* END OF MODIFICATIONS */

	/* ... */

	/*
	 * Create threads
	 */

	thr = safe_malloc(thrcnt * sizeof(*thr));
	
	for (i = 0; i < thrcnt; i++) {   
		/* Initialize per-thread structure */
		
		thr[i].kg = kg;

		thr[i].thrid = i;

		thr[i].thrcnt = thrcnt;

		thr[i].is_child = (i < chldcnt);

		thr[i].rseed = rand();

		/* Spawn new thread */
		ret = pthread_create(&thr[i].tid, NULL, thread_start_fn, &thr[i]);
		if (ret) {
			perror_pthread(ret, "pthread_create");
			exit(1);
		}
	}

	/*
	 * Wait for all threads to terminate
	 */
	for (i = 0; i < thrcnt; i++) {
		ret = pthread_join(thr[i].tid, NULL);
		if (ret) {
			perror_pthread(ret, "pthread_join");
			exit(1);
		}
	}

	printf("OK.\n");

	return 0;
}
