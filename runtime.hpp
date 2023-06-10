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
	// static Var null() { return { VT_NULL }; }
};


struct Runtime {
	Node prog;
	map<string, Var> mem;

	int run() {
		block( prog.list.at(0) );
		return 0;
	}

	int runc() {
		try 
			{ return run(); }
		catch ( parse_error& p ) 
			{ return printf( "%s\n", p.what() ), 0; }
	}

	void showmem() {
		for (auto p : mem)
			printf("%-10s :: %d\n", p.first.c_str(), p.second.i);
	}

	int error(const string& msg = "") {
		throw parse_error("::runtime_fail:: " + msg);
	}

	void block(const Node& n) {
		for (auto& st : n.list)
			stmt(st);
	}

	int stmt(const Node& n) {
		// printf("%s\n", n.type.c_str());
		if      (n.type == "let") memseti( n.list.at(0).val, expr( n.list.at(1) ) );
		else if (n.type == "if") {
			if ( expr(n.list.at(0)) ) return block( n.list.at(1) ), 1;  // first condition
			for (int i = 2; i < n.list.size(); i++)
				if ( stmt(n.list.at(i)) ) return 1;  // sub-conditions
		}
		else if (n.type == "elseif") return expr(n.list.at(0)) ? (block( n.list.at(1) ), 1) : 0;
		else if (n.type == "else")   return block( n.list.at(0) ), 1;
		else if (n.type == "while")  while ( expr(n.list.at(0)) )  block( n.list.at(1) );
		else    error("unknown statement: " + n.type);
		return 0;
	}

	int expr(const Node& n) {
		if      (n.type == "expr") return expr( n.list.at(0) );
		else if (n.type == "number") return stoi( n.val );
		else if (n.type == "identifier") return memgeti(n.val);
		else if (n.val  == "||") return expr( n.list.at(0) ) || expr( n.list.at(1) );
		else if (n.val  == "&&") return expr( n.list.at(0) ) && expr( n.list.at(1) );
		else if (n.val  == "==") return expr( n.list.at(0) ) == expr( n.list.at(1) );
		else if (n.val  == "!=") return expr( n.list.at(0) ) != expr( n.list.at(1) );
		else if (n.val  == ">=") return expr( n.list.at(0) ) >= expr( n.list.at(1) );
		else if (n.val  == "<=") return expr( n.list.at(0) ) <= expr( n.list.at(1) );
		else if (n.val  == ">" ) return expr( n.list.at(0) ) >  expr( n.list.at(1) );
		else if (n.val  == "<" ) return expr( n.list.at(0) ) <  expr( n.list.at(1) );
		else if (n.val  == "+" ) return expr( n.list.at(0) ) +  expr( n.list.at(1) );
		else if (n.val  == "-" ) return expr( n.list.at(0) ) -  expr( n.list.at(1) );
		else if (n.val  == "*" ) return expr( n.list.at(0) ) *  expr( n.list.at(1) );
		else if (n.val  == "/" ) return expr( n.list.at(0) ) /  expr( n.list.at(1) );
		return error("unknown expr: " + n.type + " [" + n.val + "]");
	}

	// memory
	Var& memget(const string& id) {
		if (!mem.count(id)) error("missing identifier: [" + id + "]");
		return mem[id];
	}
	int& memgeti(const string& id) {
		if (memget(id).type != VT_INT) error("expected int, identifier: [" + id + "]");
		return memget(id).i;
	}
	void memseti(const string& id, int i) {
		mem[ id ] = { VT_INT, i };
	}
};