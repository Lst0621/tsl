import {
    inner_product,
    matrix_add_number,
    matrix_inverse_number,
    matrix_multiply_number,
} from "../math/matrix";

describe("matrix_multiply_number", () => {
    test("2x2 multiplication", () => {
        const a = [[1, 2], [3, 4]];
        const b = [[5, 6], [7, 8]];
        expect(matrix_multiply_number(a, b)).toEqual([[19, 22], [43, 50]]);
    });
});

describe("matrix_add_number", () => {
    test("2x2 addition", () => {
        const a = [[1, 2], [3, 4]];
        const b = [[5, 6], [7, 8]];
        expect(matrix_add_number(a, b)).toEqual([[6, 8], [10, 12]]);
    });
});

describe("matrix_inverse_number", () => {
    test("diagonal matrix with -1 is its own inverse", () => {
        const a = [[1, 0], [0, -1]];
        expect(matrix_inverse_number(a)).toEqual([[1, 0], [0, -1]]);
    });
});

describe("inner_product", () => {
    test("[1,2] dot [3,4] = 11", () => {
        expect(inner_product([1, 2], [3, 4])).toBe(11);
    });
});
