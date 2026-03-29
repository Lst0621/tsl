#include <gtest/gtest.h>

#include "linear_recurrence.h"

TEST(LinearRecurrenceTest, FibonacciRecursiveAndMatrixMatch) {
    // Fibonacci: F(n) = F(n-1) + F(n-2), F(0)=0, F(1)=1
    LinearRecurrence fib({1, 1});
    std::vector<LinearRecurrence::Coeff> init = {0, 1};

    for (size_t n = 0; n <= 30; ++n) {
        EXPECT_EQ(fib.evaluate_recursive(init, n), fib.evaluate_matrix(init, n))
            << "Mismatch at n=" << n;
    }
}

TEST(LinearRecurrenceTest, EvaluateAtN0) {
    LinearRecurrence fib({1, 1});
    std::vector<LinearRecurrence::Coeff> init = {0, 1};
    EXPECT_EQ(fib.evaluate(init, 0), 0);
    EXPECT_EQ(fib.evaluate_matrix(init, 0), 0);
    EXPECT_EQ(fib.evaluate_recursive(init, 0), 0);
}

TEST(LinearRecurrenceTest, FibonacciLargeTerms) {
    LinearRecurrence fib({1, 1});
    std::vector<LinearRecurrence::Coeff> init = {0, 1};

    EXPECT_EQ(fib.evaluate(init, 50), 12586269025LL);
    EXPECT_EQ(fib.evaluate_matrix(init, 70), 190392490709135LL);
}

TEST(LinearRecurrenceTest, AutoThresholdSwitchStillCorrect) {
    LinearRecurrence fib({1, 1}, 20);
    std::vector<LinearRecurrence::Coeff> init = {0, 1};

    EXPECT_EQ(fib.evaluate(init, 19), fib.evaluate_recursive(init, 19));
    EXPECT_EQ(fib.evaluate(init, 20), fib.evaluate_matrix(init, 20));
}

TEST(LinearRecurrenceTest, FibonacciCharacteristicPolynomialAnnilhilatesMatrix) {
    LinearRecurrence fib({1, 1});
    Polynomial<LinearRecurrence::Coeff> p = fib.characteristic_polynomial();
    Matrix<LinearRecurrence::Coeff> m = fib.transition_matrix();

    Matrix<LinearRecurrence::Coeff> p_of_m = p.apply(m);
    Matrix<LinearRecurrence::Coeff> zero(p_of_m.get_rows(), p_of_m.get_cols());

    EXPECT_TRUE(p_of_m == zero) << "p(M) should be the zero matrix";
}
