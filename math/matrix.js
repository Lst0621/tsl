import { get_add_mod_n_function, get_multiply_mod_n_function } from "./number.js";
export function transpose(a) {
    let m = a.length;
    let n = a[0].length;
    let ans = [];
    for (let i = 0; i < n; i++) {
        let array = [];
        for (let j = 0; j < m; j++) {
            array.push(a[j][i]);
        }
        ans.push(array);
    }
    return ans;
}
export function matrix_multiply_general(a, b, multiply, addition) {
    const rows_a = a.length;
    const cols_a = a[0].length;
    const rows_b = b.length;
    const cols_b = b[0].length;
    if (cols_a !== rows_b) {
        console.log(a, b);
        throw new Error("Matrix dimensions do not match for multiplication " + [rows_a, cols_a, rows_b, cols_a]);
    }
    const result = [];
    for (let i = 0; i < rows_a; i++) {
        const row = [];
        for (let j = 0; j < cols_b; j++) {
            let sum = multiply(a[i][0], b[0][j]);
            for (let k = 1; k < cols_a; k++) {
                const prod = multiply(a[i][k], b[k][j]);
                sum = addition(sum, prod);
            }
            row.push(sum);
        }
        result.push(row);
    }
    return result;
}
export function matrix_add_general(a, b, addition) {
    const rows_a = a.length;
    const cols_a = a[0].length;
    const rows_b = b.length;
    const cols_b = b[0].length;
    if (rows_a !== rows_b || rows_b !== cols_b) {
        console.log(a, b);
        throw new Error("Matrix dimensions do not match for addition " + [rows_a, cols_a, rows_b, cols_a]);
    }
    const result = [];
    for (let i = 0; i < rows_a; i++) {
        const row = [];
        for (let j = 0; j < cols_a; j++) {
            let sum = addition(a[i][j], b[i][j]);
            row.push(sum);
        }
        result.push(row);
    }
    return result;
}
export function matrix_multiply_number(a, b) {
    return matrix_multiply_general(a, b, (m, n) => m * n, (a, b) => a + b);
}
export function matrix_add_number(a, b) {
    return matrix_add_general(a, b, (a, b) => a + b);
}
export function matrix_multiply_zn(a, b, n) {
    return matrix_multiply_general(a, b, get_multiply_mod_n_function(n), get_add_mod_n_function(n));
}
export function inner_product(a, b) {
    let product = matrix_multiply_number([a], transpose([b]));
    return product[0][0];
}
export function get_det_func(get, n, multiply, addition, add_inverse) {
    if (n === 1)
        return get(0, 0);
    if (n === 2) {
        return addition(multiply(get(0, 0), get(1, 1)), add_inverse(multiply(get(0, 1), get(1, 0))));
    }
    let det = undefined;
    let first = true;
    for (let j = 0; j < n; j++) {
        const sub_get = (i, k) => get(i + 1, k < j ? k : k + 1);
        const sign = (j % 2 === 0) ? (x) => x : add_inverse;
        const cofactor = multiply(sign(get(0, j)), get_det_func(sub_get, n - 1, multiply, addition, add_inverse));
        if (first) {
            det = cofactor;
            first = false;
        }
        else {
            det = addition(det, cofactor);
        }
    }
    return det;
}
export function get_det(a, multiply, addition, add_inverse) {
    const n = a.length;
    if (n === 0 || a[0].length !== n) {
        throw new Error("Matrix must be square");
    }
    const get = (i, j) => a[i][j];
    return get_det_func(get, n, multiply, addition, add_inverse);
}
export function array_to_matrix(array, m, n) {
    if (array.length !== m * n) {
        throw new Error(`Array length ${array.length} does not match matrix dimensions ${m}×${n}`);
    }
    const matrix = [];
    for (let i = 0; i < m; i++) {
        const row = array.slice(i * n, (i + 1) * n);
        matrix.push(row);
    }
    return matrix;
}
export function get_inverse(a, addition, multiply, add_inverse, mul_inverse) {
    const n = a.length;
    if (n === 0 || a[0].length !== n) {
        throw new Error("Matrix must be square");
    }
    const get = (i, j) => a[i][j];
    const det = get_det_func(get, n, multiply, addition, add_inverse);
    const det_inv = mul_inverse(det);
    const adjugate = Array.from({ length: n }, () => Array(n));
    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            const sub_get = (r, c) => {
                const row = r < i ? r : r + 1;
                const col = c < j ? c : c + 1;
                return get(row, col);
            };
            const cofactor = get_det_func(sub_get, n - 1, multiply, addition, add_inverse);
            const sign = ((i + j) % 2 === 0) ? (x) => x : add_inverse;
            adjugate[j][i] = multiply(det_inv, sign(cofactor)); // Transposed
        }
    }
    return adjugate;
}
