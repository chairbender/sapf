#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "VM.hpp"
#include "MathOps.hpp" // TODO: needed?
#include "doctest.h"

#define CHECK_ARR(expected, actual, n) \
	do { \
		LOOP(i,n) { CHECK(out[i] == doctest::Approx(expected[i]).epsilon(1e-9)); } \
	} while (0)


// so we can access ops directly
extern BinaryOp* gBinaryOpPtr_plus;
extern BinaryOp* gBinaryOpPtr_minus;
extern BinaryOp* gBinaryOpPtr_mul;
extern BinaryOp* gBinaryOpPtr_div;

//TODO: Actually should be Z
// TODO: Use the CAPTURE to avoid dupe
TEST_CASE("BinaryOp_plus loopz") {
	double aa[] = {1, 2, 3};
	double bb[] = {4, 5, 6};
	double zero[] = {0};
	double out[3];

	SUBCASE("stride 1") {
		double expected[] = {5, 7, 9};
		gBinaryOpPtr_plus->loopz(3, aa, 1, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("stride 0") {
		double expected[] = {5, 5, 5};
		gBinaryOpPtr_plus->loopz(3, aa, 0, bb, 0, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 1 bstride 0") {
		double expected[] = {5, 6, 7};
		gBinaryOpPtr_plus->loopz(3, aa, 1, bb, 0, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 0 bstride 1") {
		double expected[] = {5, 6, 7};
		gBinaryOpPtr_plus->loopz(3, aa, 0, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 0 a = 0") {
		double expected[] = {4, 5, 6};
		gBinaryOpPtr_plus->loopz(3, zero, 0, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("bstride 0 b = 0") {
		double expected[] = {1, 2, 3};
		gBinaryOpPtr_plus->loopz(3, aa, 1, zero, 0, out);

		CHECK_ARR(expected, out, 3);
	}
}

TEST_CASE("BinaryOp_minus loopz") {
	double aa[] = {1, 2, 3};
	double bb[] = {4, 5, 6};
	double zero[] = {0};
	double out[3];

	SUBCASE("stride 1") {
		double expected[] = {-3, -3, -3};
		gBinaryOpPtr_minus->loopz(3, aa, 1, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("stride 0") {
		double expected[] = {-3, -3, -3};
		gBinaryOpPtr_minus->loopz(3, aa, 0, bb, 0, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 1 bstride 0") {
		double expected[] = {-3, -2, -1};
		gBinaryOpPtr_minus->loopz(3, aa, 1, bb, 0, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 0 bstride 1") {
		double expected[] = {-3, -4, -5};
		gBinaryOpPtr_minus->loopz(3, aa, 0, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 0 a = 0") {
		double expected[] = {-4, -5, -6};
		gBinaryOpPtr_minus->loopz(3, zero, 0, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("bstride 0 b = 0") {
		double expected[] = {1, 2, 3};
		gBinaryOpPtr_minus->loopz(3, aa, 1, zero, 0, out);

		CHECK_ARR(expected, out, 3);
	}
}

TEST_CASE("BinaryOp_mul loopz") {
	double aa[] = {1, 2, 3};
	double bb[] = {4, 5, 6};
	double one[] = {1};
	double out[3];

	SUBCASE("stride 1") {
		double expected[] = {4, 10, 18};
		gBinaryOpPtr_mul->loopz(3, aa, 1, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("stride 0") {
		double expected[] = {4, 4, 4};
		gBinaryOpPtr_mul->loopz(3, aa, 0, bb, 0, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 1 bstride 0") {
		double expected[] = {4, 8, 12};
		gBinaryOpPtr_mul->loopz(3, aa, 1, bb, 0, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 0 bstride 1") {
		double expected[] = {4, 5, 6};
		gBinaryOpPtr_mul->loopz(3, aa, 0, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 0 a = 1") {
		double expected[] = {4, 5, 6};
		gBinaryOpPtr_mul->loopz(3, one, 0, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("bstride 0 b = 1") {
		double expected[] = {1, 2, 3};
		gBinaryOpPtr_mul->loopz(3, aa, 1, one, 0, out);

		CHECK_ARR(expected, out, 3);
	}
}

TEST_CASE("BinaryOp_div loopz") {
	double aa[] = {1, 2, 3};
	double bb[] = {4, 5, 6};
	double one[] = {1};
	double out[3];

	SUBCASE("stride 1") {
		double expected[] = {.25, .4, .5};
		gBinaryOpPtr_div->loopz(3, aa, 1, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("stride 0") {
		double expected[] = {.25, .25, .25};
		gBinaryOpPtr_div->loopz(3, aa, 0, bb, 0, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 1 bstride 0") {
		double expected[] = {.25, .5, .75};
		gBinaryOpPtr_div->loopz(3, aa, 1, bb, 0, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 0 bstride 1") {
		double expected[] = {.25, .2, 1./6};
		gBinaryOpPtr_div->loopz(3, aa, 0, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("astride 0 a = 1") {
		double expected[] = {.25, .2, 1./6};
		gBinaryOpPtr_div->loopz(3, one, 0, bb, 1, out);

		CHECK_ARR(expected, out, 3);
	}

	SUBCASE("bstride 0 b = 1") {
		double expected[] = {1, 1, 1};
		gBinaryOpPtr_div->loopz(3, aa, 1, one, 0, out);

		CHECK_ARR(expected, out, 3);
	}
}