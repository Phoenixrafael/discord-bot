#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace BigNumber {
	struct bigNumber {
		std::string value;
		bigNumber operator + (bigNumber& b) {
			bigNumber& a;
			a.value = value;
		}
	};
}