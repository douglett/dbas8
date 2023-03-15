#pragma once
#include <map>
#include "global.hpp"
using namespace std;


struct Runtime {
	Node prog;
	map<string, int> mem;

	int run() {
		// for (const auto& n : prog.list)
		// 	stmt(n);
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
			printf("%-10s :: %d\n", p.first.c_str(), p.second);
	}

	void error(const string& msg = "") {
		throw parse_error("::runtime_fail:: " + msg);
	}

	void block(const Node& n) {
		for (auto& st : n.list)
			stmt(st);
	}

	int stmt(const Node& n) {
		// printf("%s\n", n.type.c_str());
		if      (n.type == "let") mem[ n.list.at(0).val ] = expr( n.list.at(1) );
		else if (n.type == "if") {
			if ( expr(n.list.at(0)) ) return block( n.list.at(1) ), 1;  // first condition
			for (int i = 2; i < n.list.size(); i++)
				if ( stmt(n.list.at(i)) ) return 1;  // sub-conditions
		}
		else if (n.type == "elseif") return expr(n.list.at(0)) ? (block( n.list.at(1) ), 1) : 0;
		else if (n.type == "else")   return block( n.list.at(0) ), 1;
		else    error("unknown statement: " + n.type);
		return 0;
	}

	int expr(const Node& n) {
		if      (n.type == "expr") return expr( n.list.at(0) );
		else if (n.type == "number") return stoi( n.val );
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
		return error("unknown expr: " + n.type + " [" + n.val + "]"), 0;
	}
};