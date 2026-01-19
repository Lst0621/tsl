import {wasmMinus} from "../wasm_api.js";

console.log(wasmMinus(10, 3)); // 7


export function gcd(a_in: number, b_in: number): number {
    let a: number = Math.abs(a_in);
    let b: number = Math.abs(b_in);
    if (a == 0 || b == 0) {
        return a + b
    }

    if (a == b) {
        return a
    }

    if (a > b) {
        let tmp: number = a
        a = b
        b = tmp
    }

    // a < b
    while (true) {
        let res = b % a
        if (res == 0) {
            return a
        }
        b = a
        a = res
    }
}

export function array_eq<T>(a: T[], b: T[]): boolean {
    let len_a: number = a.length;
    let len_b: number = b.length;
    if (len_a != len_b) {
        return false;
    }
    for (let i = 0; i < len_a; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

export function array_eq_2d<T>(a: T[][], b: T[][]): boolean {
    let len_a: number = a.length;
    let len_b: number = b.length;
    if (len_a != len_b) {
        return false;
    }
    for (let i = 0; i < len_a; i++) {
        if (!array_eq(a[i], b[i])) {
            return false;
        }
    }
    return true;
}
