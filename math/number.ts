import {gcd} from "./math.js";
import {matrix_add_number, matrix_inverse_number, matrix_multiply_number, transpose} from "./matrix.js";

export function multiply_mod_n(a: number, b: number, n: number): number {
    return a * b % n;
}

export function get_multiply_mod_n_function(n: number) {
    return (a: number, b: number) => multiply_mod_n(a, b, n)
}

export function add_mod_n(a: number, b: number, n: number): number {
    return (a + b) % n;
}

export function get_add_mod_n_function(n: number) {
    return (a: number, b: number) => add_mod_n(a, b, n)
}

export function add_inverse_mod_n(a: number, n: number) {
    return (n - (a % n)) % n;
}

export function get_add_inverse_mod_n_function(n: number) {
    return (a: number) => add_inverse_mod_n(a, n)
}

export function mul_inverse_mod_n(a: number, n: number) {
    // ax=1 (mod n)
    if (gcd(a, n) != 1) {
        throw Error("No multiplication inverse exists for " + a + " mod " + n);
    }
    for (let i = 1; i < n; i++) {
        if (multiply_mod_n(i, a, n) == 1) {
            return i;
        }
    }
    throw Error("No multiplication inverse exists for " + a + " mod " + n);
}

export function get_mul_inverse_mod_n_function(n: number) {
    return (a: number) => mul_inverse_mod_n(a, n)
}

export function are_co_prime(a: number, b: number): boolean {
    return gcd(a, b) == 1
}

export function is_prime(a: number) {
    if (a <= 1) {
        return false
    }
    if (a == 2 || a == 3 || a == 5) {
        return true
    }
    for (let i = 2; i * i <= a; i++) {
        if (a % i == 0) {
            return false
        }
    }
    return true
}

export function* gen_prime(): Generator<number> {
    let n: number = 2
    while (true) {
        if (is_prime(n)) {
            yield n
        }
        n++
    }
}

export function get_first_n_primes(n: number) {
    let primes: number[] = []
    for (let prime of gen_prime()) {
        primes.push(prime)
        if (primes.length == n) {
            break
        }
    }
    return primes
}

export function totient(n: number): number {
    let ans: number = 0
    for (let i = 1; i <= n; i++) {
        if (are_co_prime(i, n)) {
            ans += 1
        }
    }
    return ans
}

function complex_to_matrix(complex: number[] | number) {
    if (typeof complex == "number") {
        return complex_to_matrix([complex, 0])
    }
    // TODO size check
    let real = complex[0]
    let imaginary = complex[1]
    return [[real, -imaginary], [imaginary, real]]
}

function matrix_to_complex(matrix: number[][]) {
    // TODO size check
    let real = matrix[0][0]
    let imaginary = matrix[1][0]
    return [real, imaginary]
}

export function complex_add(a: number[] | number, b: number[] | number) {
    let mat_a = complex_to_matrix(a)
    let mat_b = complex_to_matrix(b)
    // console.log(mat_a, mat_b)
    return matrix_to_complex(matrix_add_number(mat_a, mat_b))
}

export function complex_multiply(a: number[] | number, b: number[] | number) {
    let mat_a = complex_to_matrix(a)
    let mat_b = complex_to_matrix(b)
    // console.log(mat_a, mat_b)
    return matrix_to_complex(matrix_multiply_number(mat_a, mat_b))
}

export function get_conjugate(a: number[] | number) {
    let mat_a = complex_to_matrix(a)
    return matrix_to_complex(transpose(mat_a))
}

export function complex_inverse(a: number[] | number) {
    let mat_a = complex_to_matrix(a)
    return matrix_to_complex(matrix_inverse_number(mat_a))
}

export function complex_divide(a: number[] | number, b: number[] | number) {
    let b_inv = complex_inverse(b)
    return complex_multiply(a, b_inv)
}