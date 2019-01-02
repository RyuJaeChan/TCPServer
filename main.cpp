#define DEV 20181231
#ifdef DEV

#include "GameManager.h"
int main() {
	GameManager gameManager;
	return 0;
}


#else

#include <iostream>
#include <vector>

using namespace std;


class T {
public:
	int t;
	T() : t(11) {
		cout << "init T" << endl;
	}
};

int main() {
	T();

	vector<T> v;
	v.push_back(T());

	cout << v.front().t << endl;

	int i;
	cin >> i;
	return 0;
}

#endif