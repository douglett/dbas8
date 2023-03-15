#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
using namespace std;


struct TokenDef {
	static int isident(const string& str) {
		if (str.length() == 0) return 0;
		if (!isalpha(str[0]) && str[0] != '_') return 0;
		for (int i = 1; i < str.length(); i++)
			if (!isalpha(str[i]) && !isdigit(str[i]) && str[i] != '_') return 0;
		return 1;
	}

	static int isnumber(const string& str) {
		if (str.length() == 0) return 0;
		for (int i = 0; i < str.length(); i++)
			if (!isdigit(str[i])) return 0;
		return 1;
	}

	static int iscomment(const string& str, const string& COMM) {
		// return str.length() >= 2 && str[0] == '/' && str[1] == '/';
		return str.substr(0, COMM.length()) == COMM;
	}

	static int isstrlit(const string& str) {
		return str.length() >= 2 && str.front() == '"' && str.back() == '"';
	}

	static string stripstrlit(const string& str) {
		return isstrlit(str) ? str.substr(1, str.length()-2) : str;
	}
};


struct Tokenizer : TokenDef {
	const string TOK_EOF = "$EOF";
	const string TOK_EOL = "$EOL";
	const string COMM    = "//";  // comment start
	vector<string> tokens;
	int pos = 0;
	int lno = 1;

	int loadf(const string& fname) {
		tokens = {}, pos = 0, lno = 1;
		fstream fs(fname, ios::in);
		string s;
		if (!fs.is_open())
			return fprintf(stderr, "error opening file: %s\n", fname.c_str()), 1;
		while (getline(fs, s))
			tokenizeline(s);
		return 0;
	}

	int loads(const string& prog) {
		tokens = {}, pos = 0, lno = 1;
		stringstream ss(prog);
		string s;
		while (getline(ss, s))
			tokenizeline(s);
		return 0;
	}
	
	string getlit(const string& line, int& i) {
		string s = "\"";
		for (++i ; i < line.length(); i++) {
			s += line[i];
			if (line[i] == '"') break;
		}
		return s;
	}

	void tokenizeline(const string& line) {
		string s;
		// helpers
		#define addtok()  (s.length() ? tokens.push_back(s), s = "" : s)
		#define addchr(c) (tokens.push_back(string(1, c)))
		// parse
		for (int i = 0; i < line.length(); i++)
			if      (isspace(line[i])) addtok();
			else if (isalnum(line[i])) s += line[i];
			else if (line.substr(i, COMM.length()) == COMM) addtok(), s = line.substr(i), i += s.length(), addtok();
			else if (line[i] == '"') addtok(), s = getlit(line, i), addtok();
			else    addtok(), addchr(line[i]);
		// finish up
		addtok();
		tokens.push_back(TOK_EOL);
	}

	const string& peek(int p=0) {
		return pos + p >= tokens.size() ? TOK_EOF : tokens[pos + p];
	}

	const string& get() {
		if (pos > tokens.size()) return TOK_EOF;
		if (tokens[pos] == TOK_EOL) lno++;
		return tokens[pos++];
	}
};