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
		// if      (n.type == "let") memset( n.list.at(0).val, expr2( n.list.at(1) ) );
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

	// int expr(const Node& n) {
	// 	if      (n.type == "expr") return expr( n.list.at(0) );
	// 	else if (n.type == "number") return stoi( n.val );
	// 	else if (n.type == "identifier") return memgeti(n.val);
	// 	else if (n.val  == "||") return expr( n.list.at(0) ) || expr( n.list.at(1) );
	// 	else if (n.val  == "&&") return expr( n.list.at(0) ) && expr( n.list.at(1) );
	// 	else if (n.val  == "==") return expr( n.list.at(0) ) == expr( n.list.at(1) );
	// 	else if (n.val  == "!=") return expr( n.list.at(0) ) != expr( n.list.at(1) );
	// 	else if (n.val  == ">=") return expr( n.list.at(0) ) >= expr( n.list.at(1) );
	// 	else if (n.val  == "<=") return expr( n.list.at(0) ) <= expr( n.list.at(1) );
	// 	else if (n.val  == ">" ) return expr( n.list.at(0) ) >  expr( n.list.at(1) );
	// 	else if (n.val  == "<" ) return expr( n.list.at(0) ) <  expr( n.list.at(1) );
	// 	else if (n.val  == "+" ) return expr( n.list.at(0) ) +  expr( n.list.at(1) );
	// 	else if (n.val  == "-" ) return expr( n.list.at(0) ) -  expr( n.list.at(1) );
	// 	else if (n.val  == "*" ) return expr( n.list.at(0) ) *  expr( n.list.at(1) );
	// 	else if (n.val  == "/" ) return expr( n.list.at(0) ) /  expr( n.list.at(1) );
	// 	return error("unknown expr: " + n.type + " [" + n.val + "]");
	// }

	Var expr2(const Node& n) {
		if      (n.type == "expr")   return expr2(n.list.at(0));
		else if (n.type == "number") return { VT_INT, stoi(n.val) };
		else if (n.type == "strlit") return { VT_STRING, 0, n.val };
		else if (n.type == "objlit") return memalloc();
		else if (n.type == "identifier") return memget(n.val);
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
	Var& memget(const string& id) {
		if (!mem.count(id)) error("missing identifier: [" + id + "]");
		return mem[id];
	}
	// int& memgeti(const string& id) {
	// 	if (memget(id).type != VT_INT) error("expected int, identifier: [" + id + "]");
	// 	return memget(id).i;
	// }
	// string& memgets(const string& id) {
	// 	if (memget(id).type != VT_STRING) error("expected string, identifier: [" + id + "]");
	// 	return memget(id).s;
	// }
	void memset(const string& id, const Var& v) {
		mem[ id ] = v;
	}
	// void memseti(const string& id, int i) {
	// 	mem[ id ] = { VT_INT, i };
	// }
	Var memalloc() {
		heapaddr++;
		heap[heapaddr] = { heapaddr };
		return Var::obj(heapaddr);
	}
	void memfree(const Var& addr) {
		if (addr.type != VT_OBJECT || !heap.count(addr.i)) error("bad memory access");
		heap.erase(addr.i);
	}

	// following paths to memory
	void varpath_set(const Node& p, const Var& v) {
		memset( p.list.at(0).val, v );
	}
};