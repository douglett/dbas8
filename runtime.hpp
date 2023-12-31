#pragma once
#include <map>
#include "global.hpp"
using namespace std;


enum VARTYPE {
	VT_NULL = 0,
	VT_INT,
	VT_STRING,
	VT_OBJECT
};
struct Var {
	VARTYPE type;
	int i;
	string s;

	static Var null() { return { VT_NULL }; }
	static Var num(int i) { return { VT_INT, i }; }
	static Var str(const string& s) { return { VT_STRING, 0, s }; }
	static Var obj(int addr) { return { VT_OBJECT, addr }; }
	
	string tostring() const {
		switch (type) {
		case VT_INT:     return to_string(i);
		case VT_STRING:  return s;
		case VT_OBJECT:  return "@" + to_string(i);
		case VT_NULL:    return "$NULL";
		}
		throw runtime_error("type error");
	}
	string typestring() const {
		switch (type) {
		case VT_INT:     return "number";
		case VT_STRING:  return "string";
		case VT_OBJECT:  return "object";
		case VT_NULL:    return "null";
		}
		throw runtime_error("type error");
	}
};
struct Obj {
	int addr;
	map<string, Var> obj;
	vector<Var> arr;
};


struct Runtime {
	Node prog;
	map<string, Var> mem;
	map<int, Obj> heap;
	int heapaddr = 0;

	int run() {
		block( prog.list.at(0) );
		return 0;
	}

	int runc() {
		printf(":running:\n");
		try 
			{ return run(); }
		catch ( runtime_error& p ) 
			{ return printf( "%s\n", p.what() ), 0; }
	}

	void showmem() {
		for (const auto& p : mem)
			printf("%-10s :: %s\n", p.first.c_str(), p.second.tostring().c_str());
		for (const auto& p : heap)
			printf("@%08d  :: %s\n", p.first, "{}");
	}

	int error(const string& msg = "") {
		throw runtime_error("::runtime_fail:: " + msg);
	}

	void block(const Node& n) {
		for (auto& st : n.list)
			stmt(st);
	}

	int stmt(const Node& n) {
		// printf("%s\n", n.type.c_str());
		if      (n.type == "let") varpath_set( n.list.at(0), expr2(n.list.at(1)) );
		else if (n.type == "if") {
			if ( expr2i(n.list.at(0)) )
				return block( n.list.at(1) ), 1;  // first condition
			for (int i = 2; i < n.list.size(); i++)
				if ( stmt(n.list.at(i)) ) return 1;  // sub-conditions
		}
		else if (n.type == "elseif")  return expr2i(n.list.at(0)) ? (block( n.list.at(1) ), 1) : 0;
		else if (n.type == "else")    return block( n.list.at(0) ), 1;
		else if (n.type == "while")   while ( expr2i(n.list.at(0)) )  block( n.list.at(1) );
		else if (n.type == "delete")  memfree( expr2(n.list.at(0)) );
		else    error("unknown statement: " + n.type);
		return 0;
	}

	Var expr2(const Node& n) {
		if      (n.type == "expr")   return expr2(n.list.at(0));
		else if (n.type == "number") return { VT_INT, stoi(n.val) };
		else if (n.type == "strlit") return { VT_STRING, 0, n.val };
		else if (n.type == "objlit") return memalloc();
		//else if (n.type == "identifier") return stackget(n.val);
		else if (n.type == "varpath") return varpath_get(n);
		// logical comparisons
		else if (n.val  == "||") return Var::num( expr2i(n.list.at(0)) || expr2i(n.list.at(1)) );
		else if (n.val  == "&&") return Var::num( expr2i(n.list.at(0)) && expr2i(n.list.at(1)) );
		// typed equality
		else if (n.val  == "==") return Var::num(  expr2cmp( expr2(n.list.at(0)), expr2(n.list.at(1)) ) );
		else if (n.val  == "!=") return Var::num( !expr2cmp( expr2(n.list.at(0)), expr2(n.list.at(1)) ) );
		// maths
		else if (n.val  == ">=") return Var::num( expr2i(n.list.at(0)) >= expr2i(n.list.at(1)) );
		else if (n.val  == "<=") return Var::num( expr2i(n.list.at(0)) <= expr2i(n.list.at(1)) );
		else if (n.val  == ">" ) return Var::num( expr2i(n.list.at(0)) >  expr2i(n.list.at(1)) );
		else if (n.val  == "<" ) return Var::num( expr2i(n.list.at(0)) <  expr2i(n.list.at(1)) );
		else if (n.val  == "+" ) return Var::num( expr2i(n.list.at(0)) +  expr2i(n.list.at(1)) );
		else if (n.val  == "-" ) return Var::num( expr2i(n.list.at(0)) -  expr2i(n.list.at(1)) );
		else if (n.val  == "*" ) return Var::num( expr2i(n.list.at(0)) *  expr2i(n.list.at(1)) );
		else if (n.val  == "/" ) return Var::num( expr2i(n.list.at(0)) /  expr2i(n.list.at(1)) );
		// special
		// else if (n.val  == "..") return Var::str( expr2(n.list.at(0)).tostring() + expr2(n.list.at(1)).tostring() );
		// error
		return error("unknown expr: " + n.type + " [" + n.val + "]"), Var::null();
	}

	int expr2i(const Node& n) {
		auto v = expr2(n);
		if (v.type != VT_INT) error("expected int in expression");
		return v.i;
	}

	int expr2cmp(const Var& l, const Var& r) {
		if      (l.type != r.type) return error("invalid comparison: " + l.typestring() + ", " + r.typestring());
		switch (l.type) {
		case VT_INT:     return l.i == r.i;
		case VT_STRING:  return l.s == r.s;
		case VT_OBJECT:  return l.s == r.s;
		case VT_NULL:    return true;
		}
		return error("type error");
	}

	// memory
	Var& stackget(const string& id) {
		if (!mem.count(id)) error("missing identifier: [" + id + "]");
		return mem[id];
	}
	void stackset(const string& id, const Var& v) {
		mem[ id ] = v;
	}
	Var memalloc() {
		heapaddr++;
		heap[heapaddr] = { heapaddr };
		return Var::obj(heapaddr);
	}
	void memfree(const Var& addr) {
		if (addr.type != VT_OBJECT || !heap.count(addr.i)) error("bad memory access");
		heap.erase(addr.i);
	}
	Obj& memget(int addr) {
		if (!heap.count(addr)) error("missing object at [" + to_string(addr) + "]");
		return heap[addr];
	}
	Var& memgetprop(int addr, const string& id) {
		Obj& obj = memget(addr);
		if (!obj.obj.count(id)) error("missing object property [" + id + "]");
		return obj.obj[id];
	}

	// following paths to memory
	Var& varpath_get(const Node& p, int offset = 0) {
		// stack access
		Var& top = stackget( p.list.at(0).val );
		Var* ptr = &top;
		// heap access
		for (int i = 1; i < p.list.size() + offset; i++) {
			if      (ptr->type != VT_OBJECT) error("expected object in path");
			else if (p.list.at(i).type == "identifier") ptr = &memgetprop(ptr->i, p.list.at(i).val);
			else    error("varpath_get error");
		}
		return *ptr;
	}
	
	void varpath_set(const Node& p, const Var& v) {
		// stack access
		if (p.list.size() == 1) { stackset( p.list.at(0).val, v ); return; }
		// penultimate item in heap
		Var& ptr = varpath_get(p, -1);
		// set value
		if      (ptr.type != VT_OBJECT) error("expected object in path");
		else if (p.list.back().type == "identifier") memget(ptr.i).obj[ p.list.back().val ] = v;
		else    error("varpath_set error");
	}
};