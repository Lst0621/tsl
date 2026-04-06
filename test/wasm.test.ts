import {
    wasm_number_of_sequences,
    wasm_number_of_sequences_all,
    wasm_get_gl_n_zm_size,
    wasm_matrix_det,
} from "../wasm_api";

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
