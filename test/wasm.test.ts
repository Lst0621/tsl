import {
    wasm_number_of_sequences,
    wasm_number_of_sequences_all,
} from "../wasm/ts/wasm_api_number_of_sequences";
import {
    wasm_get_gl_n_zm_size,
    wasm_get_gl_n_zm,
    wasm_is_matrix_group,
    wasm_matrix_det,
    wasm_matrix_inverse_mod,
} from "../wasm/ts/wasm_api_gl_matrix";
import { gen_general_linear_n_zm } from "../math/group";
import {
    matrix_inverse,
    matrix_multiply_zn,
} from "../math/matrix";
import {
    get_add_mod_n_function,
    get_multiply_mod_n_function,
    get_add_inverse_mod_n_function,
    get_mul_inverse_mod_n_function,
} from "../math/number";

describe("wasm_number_of_sequences", () => {
    test("known result for [2,3,6,7,8] with [7,10]", () => {
        expect(wasm_number_of_sequences([2, 3, 6, 7, 8], [7, 10])).toBe(1158);
    });

    test("empty arr returns 0", () => {
        expect(wasm_number_of_sequences([], [7, 10])).toBe(0);
    });

    test("empty seq returns 0", () => {
        expect(wasm_number_of_sequences([2, 3], [])).toBe(0);
    });
});

describe("wasm_number_of_sequences_all", () => {
    test("returns correct dimensions (8 x 11) for seq=[7,10]", () => {
        const result = wasm_number_of_sequences_all([2, 3, 6, 7, 8], [7, 10]);
        expect(Array.isArray(result)).toBe(true);
        expect(result.length).toBe(8);
        expect(result[0].length).toBe(11);
    });

    test("all entries match individual wasm_number_of_sequences calls", () => {
        const arr = [2, 3, 6, 7, 8];
        const result = wasm_number_of_sequences_all(arr, [7, 10]);
        for (let i = 0; i <= 7; i++) {
            for (let j = 0; j <= 10; j++) {
                expect(result[i][j]).toBe(wasm_number_of_sequences(arr, [i, j]));
            }
        }
    });
});

describe("wasm_get_gl_n_zm_size", () => {
    const cases = [
        { n: 2, m: 2, expected: 6 },
        { n: 3, m: 2, expected: 168 },
        { n: 2, m: 3, expected: 48 },
        { n: 3, m: 3, expected: 11232 },
    ];

    test.each(cases)("|GL($n, Z_$m)| = $expected", ({ n, m, expected }) => {
        expect(wasm_get_gl_n_zm_size(n, m)).toBe(expected);
    });
});

describe("wasm_is_matrix_group (zero-copy)", () => {
    test("GL(2, Z_2) = 6 forms a group", () => {
        const gl = wasm_get_gl_n_zm(2, 2);
        expect(gl.count).toBe(6);
        expect(wasm_is_matrix_group(gl)).toBe(true);
        gl.free();
    });

    test("GL(3, Z_2) = 168 forms a group", () => {
        const gl = wasm_get_gl_n_zm(3, 2);
        expect(gl.count).toBe(168);
        expect(wasm_is_matrix_group(gl)).toBe(true);
        gl.free();
    });

    test("GL(2, Z_3) = 48 forms a group", () => {
        const gl = wasm_get_gl_n_zm(2, 3);
        expect(gl.count).toBe(48);
        expect(wasm_is_matrix_group(gl)).toBe(true);
        gl.free();
    });

    test("removing one element breaks the group (JS array path)", () => {
        const gl = gen_general_linear_n_zm(2, 2);
        const subset = gl.slice(1);
        const flat = subset.flatMap(m => flattenMatrix(m));
        expect(wasm_is_matrix_group(flat, subset.length, 2, 2)).toBe(false);
    });
});

describe("wasm_get_gl_n_zm", () => {
    test("toFlatArray matches JS gen_general_linear_n_zm for GL(2, Z_3)", () => {
        const wasmGl = wasm_get_gl_n_zm(2, 3);
        const jsGl = gen_general_linear_n_zm(2, 3);
        expect(wasmGl.count).toBe(jsGl.length);

        const wasmFlat = wasmGl.toFlatArray();
        const wasmMatrices: string[] = [];
        for (let k = 0; k < wasmGl.count; k++) {
            wasmMatrices.push(wasmFlat.slice(k * 4, (k + 1) * 4).join(","));
        }
        const jsMatrices: string[] = [];
        for (const mat of jsGl) {
            jsMatrices.push(flattenMatrix(mat).join(","));
        }
        expect(wasmMatrices.sort()).toEqual(jsMatrices.sort());
        wasmGl.free();
    });

    test("toFlatArray matches JS gen_general_linear_n_zm for GL(2, Z_2)", () => {
        const wasmGl = wasm_get_gl_n_zm(2, 2);
        const jsGl = gen_general_linear_n_zm(2, 2);
        expect(wasmGl.count).toBe(jsGl.length);

        const wasmFlat = wasmGl.toFlatArray();
        const wasmMatrices: string[] = [];
        for (let k = 0; k < wasmGl.count; k++) {
            wasmMatrices.push(wasmFlat.slice(k * 4, (k + 1) * 4).join(","));
        }
        const jsMatrices: string[] = [];
        for (const mat of jsGl) {
            jsMatrices.push(flattenMatrix(mat).join(","));
        }
        expect(wasmMatrices.sort()).toEqual(jsMatrices.sort());
        wasmGl.free();
    });
});

describe("wasm_matrix_det", () => {
    test("2x2 determinant", () => {
        expect(wasm_matrix_det([3, 8, 4, 6], 2)).toBe(-14);
    });

    test("3x3 determinant", () => {
        expect(wasm_matrix_det([2, 5, 1, 3, 1, 4, 1, 2, 3], 3)).toBe(-30);
    });

    test("5x5 circulant matrix has non-zero determinant", () => {
        const mat = [
            1, 2, 3, 4, 5,
            2, 3, 4, 5, 1,
            3, 4, 5, 1, 2,
            4, 5, 1, 2, 3,
            5, 1, 2, 3, 4,
        ];
        expect(wasm_matrix_det(mat, 5)).not.toBe(0);
    });

    test("throws on dimension mismatch", () => {
        expect(() => wasm_matrix_det([1, 2, 3], 2)).toThrow();
    });
});

function flattenMatrix(mat: number[][]): number[] {
    const result: number[] = [];
    for (const row of mat) {
        for (const val of row) {
            result.push(val);
        }
    }
    return result;
}

function identityFlat(n: number): number[] {
    const I = new Array(n * n).fill(0);
    for (let i = 0; i < n; i++) {
        I[i * n + i] = 1;
    }
    return I;
}

function matMulModFlat(a: number[], b: number[], n: number, m: number): number[] {
    const out = new Array(n * n).fill(0);
    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            let sum = 0;
            for (let k = 0; k < n; k++) {
                sum += a[i * n + k] * b[k * n + j];
            }
            out[i * n + j] = ((sum % m) + m) % m;
        }
    }
    return out;
}

describe("wasm_matrix_inverse_mod", () => {
    test("2x2 inverse over Z_5: A * A^{-1} = I", () => {
        const A = [3, 1, 4, 2];
        const inv = wasm_matrix_inverse_mod(A, 2, 5);
        const product = matMulModFlat(A, inv, 2, 5);
        expect(product).toEqual(identityFlat(2));
    });

    test("3x3 inverse over Z_7: A * A^{-1} = I", () => {
        const A = [1, 2, 3, 0, 1, 4, 5, 6, 0];
        const inv = wasm_matrix_inverse_mod(A, 3, 7);
        const product = matMulModFlat(A, inv, 3, 7);
        expect(product).toEqual(identityFlat(3));
    });

    test("all GL(2, Z_3) elements: WASM inverse matches JS inverse", () => {
        const p = 3;
        const n = 2;
        const gl = gen_general_linear_n_zm(n, p);
        for (const mat2d of gl) {
            const flat = flattenMatrix(mat2d);
            const wasmInv = wasm_matrix_inverse_mod(flat, n, p);
            const jsInv = matrix_inverse(
                mat2d,
                get_add_mod_n_function(p),
                get_multiply_mod_n_function(p),
                get_add_inverse_mod_n_function(p),
                get_mul_inverse_mod_n_function(p),
            );
            expect(wasmInv).toEqual(flattenMatrix(jsInv));
        }
    });

    test("GL(2, Z_5): WASM and JS agree on all 480 inverses", () => {
        const p = 5;
        const n = 2;
        const gl = gen_general_linear_n_zm(n, p);
        expect(gl.length).toBe(480);
        for (const mat2d of gl) {
            const flat = flattenMatrix(mat2d);
            const wasmInv = wasm_matrix_inverse_mod(flat, n, p);
            const product = matMulModFlat(flat, wasmInv, n, p);
            expect(product).toEqual(identityFlat(n));
        }
    });

    test("GL(3, Z_2): WASM inverse for all 168 matrices", () => {
        const p = 2;
        const n = 3;
        const gl = gen_general_linear_n_zm(n, p);
        expect(gl.length).toBe(168);
        for (const mat2d of gl) {
            const flat = flattenMatrix(mat2d);
            const wasmInv = wasm_matrix_inverse_mod(flat, n, p);
            const product = matMulModFlat(flat, wasmInv, n, p);
            expect(product).toEqual(identityFlat(n));
        }
    });

    test("GL(3, Z_3): WASM sampled inverses (step=40), same as JS test", () => {
        const p = 3;
        const n = 3;
        const gl = gen_general_linear_n_zm(n, p);
        expect(gl.length).toBe(11232);
        const step = 40;
        for (let i = 0; step * i < gl.length; i++) {
            const mat2d = gl[step * i];
            const flat = flattenMatrix(mat2d);
            const wasmInv = wasm_matrix_inverse_mod(flat, n, p);
            const product = matMulModFlat(flat, wasmInv, n, p);
            expect(product).toEqual(identityFlat(n));
        }
    });
});
