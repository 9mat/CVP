#include "function.h"

QuarticFunction::QuarticFunction(const Network &n) : 
  net(n), to(n.getNVertex()) {
  FOR(i,n.arcs.size()) to[net.arcs[i].head].push_back(i);
}

Real QuarticFunction::f(const Vector &v) const{
  Real sum = 0.0, x, c;
  int arc;
  
  assert(v.size()==net.arcs.size());

  FOR(i, v.size()){
    FOR(j, to[net.arcs[i].tail].size()){
      arc = to[net.arcs[i].tail][j];
      x = (v[arc]/net.arcs[arc].cap);
      sum += 20*x*x*x*x;
    }
    x = (v[i]/net.arcs[i].cap);
    sum += 100*x*x;
    
    c = Real(net.arcs[i].head+1.0)/Real(net.arcs[i].tail+1.0);
    sum += c*v[i];
  }
  return sum;
}

Vector QuarticFunction::g(const Vector &v) const{
  Vector d(v.size(),0.0);
  Real x, c, sum;
  int a;

  FOR(i, v.size()) {
    sum = 0;
    FOR(j, to[net.arcs[i].tail].size()){
      a = to[net.arcs[i].tail][j];
      x = v[a]; c = net.arcs[a].cap;
      d[a] += 80*x*x*x/(c*c*c*c);
    }

    x = v[i]; c = net.arcs[i].cap;
    d[i] += 200*x/(c*c);

    c = Real(net.arcs[i].head+1.0)/Real(net.arcs[i].tail+1.0);
    d[i] += c;
  }
  return d;
}

BPRFunction::BPRFunction(const MultiCommoNetwork &n, Real a, Real b): 
  net(n), alpha(a), beta(b) {}

Real BPRFunction::f(const Vector &x) const {
  int K = net.commoflows.size(), A = net.arcs.size();
  assert(K*A == x.size()); // debug
  Real sum = 0.0, ya, ca, ta;
  FOR(a, A){
    ya = 0.0;
    FOR(i, K) ya += x[a*K + i];
    ca = net.arcs[a].cap;
    ta = net.arcs[a].cost;
    sum += ta*ya*(1 + alpha/(beta+1)*pow(ya/ca,beta));
  }
  return sum;
}

Vector BPRFunction::g(const Vector &x) const {
  int K = net.commoflows.size(), A = net.arcs.size();
  assert(K*A == x.size()); // debug
  Vector d(K*A);
  Real ya, ca, dd, ta;
  FOR(a, A){
    ya = 0.0; ca = net.arcs[a].cap; ta = net.arcs[a].cost;
    FOR(i, K) ya += x[a*K + i];
    dd = ta + ta*alpha*pow(ya/ca,beta);
    FOR(i,K) d[a*K + i] = dd;
  }
  return d;
}

#define PHI 0.6180339887498948482045868343656

//
// recuresively do golden search
// x1 and x2 are the two middle points in the search segment AB
// e.g    A----x1----x2----B
// f1, f2, b1, b2 are the values of the function and the betas
// correspoding to x1 and x2
// fm and bm are to save the minimum value and beta while searching
//
void golden_search_recursive (Vector &x1, Vector &x2, 
			      Real f1, Real f2, Real &fm,
			      Real b1, Real b2, Real &bm,
			      Function *obj,
			      int count) {
  if (f1 > f2){
    x1 -= x2; x1 *= (-PHI); x1 += x2;
    f1 = obj->f(x1);
    b1 = b2 + PHI*(b2-b1);
    if(f1<fm) fm=f1, bm=b1;
  } 
  else {
    x2 -= x1; x2 *= (-PHI); x2 += x1;
    f2 = obj->f(x2);
    b2 = b1 + PHI*(b1-b2);
    if(f2<fm) fm=f2, bm=b2;
  }
  
  if(count == 0) return;
  golden_search_recursive (x2,x1,f2,f1,fm,b2,b1,bm,obj,count-1);
}

// golden search between A and B
double golden_search (const Vector &A, const Vector &B, Function *obj, int niteration){
  Vector x1(A), x2(A);

  x1 -= B; x1*=PHI; x1+=B; // x1 = PHI*A + (1-PHI)*B = PHI*(A-B) - B;
  x2 += B; x2 -= x1; // x2 = (1-PHI)*A + PHI*B = A + B - x1

  Real f1 = obj->f(x1), f2 = obj->f(x2), b1 = 1-PHI, b2 = PHI;
  Real fm = f1, bm = b1;

  golden_search_recursive (x1,x2,f1,f2,fm,b1,b2,bm,obj,niteration);
  return bm;
}

// Naive line search between A and B
double line_search (const Vector &A, const Vector &B, Function *obj, int niteration){
  Vector x(A), dx(B);
  Real fmin = obj->f(x), imin = 0.0, f;
  dx -= A; dx *= (1.0/niteration);
  FOR(i, niteration){
    x += dx;
    f = obj->f(x);
    if(f < fmin) fmin = f, imin = i+1;
  }
  return imin / niteration;
}