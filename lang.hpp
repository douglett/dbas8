#pragma once
#include <iostream>
#include <algorithm>
#include <vector>
#include "global.hpp"
#include "tokenizer.hpp"
using namespace std;


// - PROG
// - DIM ::= dim IDENT (, IDENT, ...)
// - FUNCTION ::= function IDENT '(' ')' $eol BLOCK $eol END FUNCTION
// BLOCK ::= STMT*
// STMT ::= EMPTYLN | LET | IF
// EMPTYLN ::= ?comment $eol
// LET ::= IDENT = EXPR
// IF ::= if EXPR $eol BLOCK $eol (?ELSEIF) (?ELSE) end if
// ELSEIF ::= else if EXPR $eol BLOCK
// ELSE ::= else $eol BLOCK
// - WHILE
// - PRINT

// EXPR ::= OR
// OR ::= AND || OR
// AND ::= CMP && AND
// CMP ::= ADD ( == != >= <= > < ) ADD
// ADD ::= MUL (+|-) ADD
// MUL ::= VALUE (*|/) MUL
// VALUE ::= NUMBER | IDENT | BRACKETS
// BRACKETS ::= '(' EXPR ')'


struct Lang {
	Tokenizer tok;
	Node prog;

	// --- general parsing ---

	int parse() {
		prog = { "prog" };
		return block(prog);
	}

	int parsec() {
		try 
			{ return parse(); }
		catch ( parse_error& p ) 
			{ return printf( "%s\n", p.what() ), 0; }
	}

	int expect(const string& type, const string& val) {
		Node n;
		return expect(n, type, val);
	}
	int expect(Node& parent, const string& type, const string& val) {
		return tok.peek() == val
			? parent.push({ type, tok.get() }), 1
			: 0;
	}

	int error() {
		throw parse_error("::parse_fail::");
	}

	// --- basic types ---

	int ident(Node& parent) {
		return tok.isident(tok.peek()) && !iskeyword(tok.peek())
			? parent.push({ "identifier", tok.get() }), 1
			: 0;
	}

	int iskeyword(const string& str) {
		static const vector<string> vec = { "if", "else", "for", "while", "function", "end" };
		return find(vec.begin(), vec.end(), str) != vec.end();
	}

	int number(Node& parent) {
		return tok.isnumber(tok.peek())
			? parent.push({ "number", tok.get() }), 1
			: 0;
	}

	int pendl(Node& parent) {
		return tok.peek() == tok.TOK_EOL || tok.peek() == tok.TOK_EOF
			// ? parent.push({ "endl", tok.get() }), 1
			? ( tok.get(), 1 )
			: 0;
	}

	// --- statements ---

	// EMPTYLN ::= ?comment $eol
	int emptyln(Node& parent) {
		if ( tok.iscomment(tok.peek(), tok.COMM) && tok.peek(1) == tok.TOK_EOL ) return tok.get(), tok.get(), 1;
		if ( tok.peek() == tok.TOK_EOL ) return tok.get(), 1;
		return 0;
	}

	// LET ::= IDENT = EXPR
	int let(Node& parent) {
		Node& n = parent.push({ "let" });
		if ( ident(n) && expect("operator", "=") )
			return 
				expr(n) && pendl(n)
				? 1 : error();
		return parent.pop(), 0;
	}

	// IF ::= if EXPR $eol BLOCK $eol (?ELSEIF) (?ELSE) end if
	int pif(Node& parent) {
		Node& n = parent.push({ "if" });
		if ( expect("keyword", "if") ) {
			( expr(n) && pendl(n) && block(n) ) || error();
			while ( pelseif(n) ) ;  // else-if (optional, many)
			pelse(n);               // else (optional)
			( expect("keyword", "end") && expect("keyword", "if") && pendl(n) ) || error();
			return 1;
		}
		return parent.pop(), 0;
	}

	// ELSEIF ::= else if EXPR $eol BLOCK
	int pelseif(Node& parent) {
		Node& n = parent.push({ "elseif" });
		if ( tok.peek() == "else" && tok.peek(1) == "if" ) {
			tok.get(), tok.get();
			( expr(n) && pendl(n) && block(n) ) || error();
			return 1;
		}
		return parent.pop(), 0;
	}

	// ELSE ::= else $eol BLOCK
	int pelse(Node& parent) {
		Node& n = parent.push({ "else" });
		if ( expect("keyword", "else") )
			return ( pendl(n) && block(n) ) || error();
		return parent.pop(), 0;
	}

	// BLOCK ::= STMT*
	int block(Node& parent) {
		Node& n = parent.push({ "block" });
		while ( stmt(n) ) ;
		return 1;
	}

	// STMT ::= EMPTYLN | LET | IF
	int stmt(Node& parent) {
		return emptyln(parent) || let(parent) || pif(parent);
	}

	// --- expressions ---

	// EXPR ::= OR
	int expr(Node& parent) {
		Node& n = parent.push({ "expr" });
		return expror(n) || ( parent.pop(), 0 );
	}
	// (helper)
	Node& insertop(Node& expr, const string& op) {
		Node n = { "operator", op, { expr } };
		expr = n;
		return expr;
	}

	// OR ::= AND || OR
	int expror(Node& parent) {
		int ok = exprand(parent);
		if ( ok && tok.peek() == "|" && tok.peek(1) == "|" ) {
			auto& n = insertop( parent.list.back(), tok.get() + tok.get() );
			return expror(n) || error();
		}
		return ok;
	}

	// AND ::= CMP && AND
	int exprand(Node& parent) {
		int ok = exprcmp(parent);
		if ( ok && tok.peek() == "&" && tok.peek(1) == "&" ) {
			auto& n = insertop( parent.list.back(), tok.get() + tok.get() );
			return exprand(n) || error();
		}
		return ok;
	}

	// CMP ::= ADD ( == != >= <= > < ) ADD
	int exprcmp(Node& parent) {
		if ( !expradd(parent) ) return 0;
		string op;
		if      ( tok.peek() == "=" && tok.peek(1) == "=" ) op = tok.get() + tok.get();
		else if ( tok.peek() == "!" && tok.peek(1) == "=" ) op = tok.get() + tok.get();
		else if ( tok.peek() == ">" && tok.peek(1) == "=" ) op = tok.get() + tok.get();
		else if ( tok.peek() == "<" && tok.peek(1) == "=" ) op = tok.get() + tok.get();
		else if ( tok.peek() == "<" ) op = tok.get();
		else if ( tok.peek() == ">" ) op = tok.get();
		else    return 1;  // lhs found
		auto& n = insertop( parent.list.back(), op );
		return expradd(n) || error();  // rhs found
	}

	// ADD ::= MUL (+|-) ADD
	int expradd(Node& parent) {
		int ok = exprmul(parent);
		if ( ok && (tok.peek() == "+" || tok.peek() == "-") ) {
			auto& n = insertop( parent.list.back(), tok.get() );
			return expradd(n) || error();
		}
		return ok;
	}

	// MUL ::= VALUE (*|/) MUL
	int exprmul(Node& parent) {
		int ok = exprvalue(parent);
		if ( ok && (tok.peek() == "*" || tok.peek() == "/") ) {
			auto& n = insertop( parent.list.back(), tok.get() );
			return exprmul(n) || error();
		}
		return ok;
	}

	// VALUE ::= NUMBER | IDENT | BRACKETS
	int exprvalue(Node& parent) {
		return number(parent) || ident(parent) || exprbrackets(parent);
	}

	// BRACKETS ::= '(' EXPR ')'
	int exprbrackets(Node& parent) {
		if ( expect("operator", "(") )
			return ( expr(parent) && expect("operator", ")") ) || error();
		return 0;
	}
};