import {
    complex_add,
    complex_inverse,
    complex_multiply,
    get_add_inverse_mod_n_function,
    get_conjugate,
    get_first_n_primes,
} from "../math/number";

describe("modular arithmetic", () => {
    test("additive inverse mod 7: inv(0) = 0", () => {
        const addInverse = get_add_inverse_mod_n_function(7);
        expect(addInverse(0)).toBe(0);
    });

    test("additive inverse mod 7: inv(i) = 7 - i for i in 1..6", () => {
        const addInverse = get_add_inverse_mod_n_function(7);
        for (let i = 1; i < 7; i++) {
            expect(addInverse(i)).toBe(7 - i);
        }
    });
});

describe("complex numbers", () => {
    test("(1+2i)(3+4i) = -5+10i", () => {
        expect(complex_multiply([1, 2], [3, 4])).toEqual([-5, 10]);
    });

    test("(1+2i)*4 = 4+8i", () => {
        expect(complex_multiply([1, 2], 4)).toEqual([4, 8]);
    });

    test("(1+2i)+(3+4i) = 4+6i", () => {
        expect(complex_add([1, 2], [3, 4])).toEqual([4, 6]);
    });

    test("conjugate of 1+2i is 1-2i", () => {
        expect(get_conjugate([1, 2])).toEqual([1, -2]);
    });

    test("inverse of i is -i", () => {
        expect(complex_inverse([0, 1])).toEqual([0, -1]);
    });

    test("inverse of 1 is 1+0i", () => {
        const result = complex_inverse(1);
        expect(result[0]).toBe(1);
        expect(result[1] === 0).toBe(true);
    });
});

describe("prime generation", () => {
    test("first 10 primes", () => {
        expect(get_first_n_primes(10)).toEqual([2, 3, 5, 7, 11, 13, 17, 19, 23, 29]);
    });
});
