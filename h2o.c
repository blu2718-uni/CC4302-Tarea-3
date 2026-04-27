#include <unistd.h>
#include <pthread.h>

#include "pss.h"
#include "h2o.h"

pthread_mutex_t m;
Queue *oq, *hq;

typedef struct { 
  int ready;
  H2O *molecule;
  Oxygen *atom;
  pthread_cond_t c;
} oxyRequest;

typedef struct {
  int ready;
  H2O *molecule;
  Hydrogen *atom;
  pthread_cond_t c;
} hdgRequest;

void initH2O(void) {
  pthread_mutex_init(&m, NULL);
  oq = makeQueue();
  hq = makeQueue();
}

void endH2O(void) {
  pthread_mutex_destroy(&m);
  destroyQueue(oq);
  destroyQueue(hq);
}

H2O *combineOxy(Oxygen *o) {
  H2O *result;
  pthread_mutex_lock(&m);
  if (queueLength(hq) > 1) {
    hdgRequest *h1 = get(hq);
    hdgRequest *h2 = get(hq);

    h1->ready = 1; h2->ready = 1;
    
    result = makeH2O(h1->atom, h2->atom, o);
    
    h1->molecule = result;
    h2->molecule = result;
    
    pthread_cond_signal(&h1->c);
    pthread_cond_signal(&h2->c);
  } else {
    oxyRequest req = {0, NULL, o, PTHREAD_COND_INITIALIZER};

    put(oq, &req);
    
    while (!req.ready) {
      pthread_cond_wait(&req.c, &m);
    }
    
    result = req.molecule;
  }
  pthread_mutex_unlock(&m);
  return result;
}

H2O *combineHydro(Hydrogen *h) {
  H2O *result;
  pthread_mutex_lock(&m);
  if (queueLength(oq) > 0 && queueLength(hq) > 0) {
    hdgRequest *h1 = get(hq);
    oxyRequest *o1 = get(oq);

    h1->ready = 1; o1->ready = 1;

    result = makeH2O(h1->atom, h, o1->atom);

    h1->molecule = result;
    o1->molecule = result;

    pthread_cond_signal(&h1->c);
    pthread_cond_signal(&o1->c);
  } else {
    hdgRequest req = {0, NULL, h, PTHREAD_COND_INITIALIZER};
    
    put(hq, &req);

    while (!req.ready) {
      pthread_cond_wait(&req.c, &m);
    }

    result = req.molecule;
  }
  pthread_mutex_unlock(&m);
  return result;
}
