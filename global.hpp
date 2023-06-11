#pragma once
#include <string>
#include <vector>
#include <exception>
using namespace std;


struct parse_error : runtime_error { using runtime_error::runtime_error; };


struct Node {
	string type, val;
	vector<Node> list;

	Node& push(const Node& n) {
		return list.push_back(n), list.back();
	}

	void pop() {
		if (list.size()) list.pop_back();
	}

	Node popout() {
		if (list.size()) {
			auto n = list.back();
			list.pop_back();
			return n;
		}
		return {};
	}

	void show(int ind = 0) const {
		printf("%s%s%s%s\n", string(ind*2, ' ').c_str(), type.c_str(), (val.length() ? " :: " : ""), val.c_str());
		for (auto& n : list)
			n.show(ind+1);
	}
};