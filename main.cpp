#include "lang.hpp"
#include "runtime.hpp"
#include <iostream>
using namespace std;


int main() {
	printf("hello world\n");

	Lang l;
	l.tok.loadf("testscripts/test2.bas");
	int ok = l.parsec();
	// printf("completed: %d\n", ok);
	l.prog.show();
	if (!ok) return 1;

	Runtime r;
	r.prog = l.prog;
	r.runc();
	r.showmem();
}