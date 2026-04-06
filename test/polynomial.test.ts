import { poly_eval, poly_to_latex_low_to_high, poly_to_latex_high_to_low } from "../math/polynomial";

describe("poly_eval", () => {
    test("constant polynomial", () => {
        expect(poly_eval([5], 100)).toBe(5);
    });

    test("linear polynomial", () => {
        expect(poly_eval([3, 2], 4)).toBe(11);
    });

    test("quadratic polynomial", () => {
        expect(poly_eval([1, 0, 1], 3)).toBe(10);
    });

    test("cubic polynomial at x=2", () => {
        expect(poly_eval([2, 3, 0, 1], 2)).toBe(16);
    });

    test("evaluating at x=0 returns constant term", () => {
        expect(poly_eval([7, 99, 42], 0)).toBe(7);
    });

    test("evaluating at x=1 sums all coefficients", () => {
        expect(poly_eval([1, 2, 3, 4], 1)).toBe(10);
    });

    test("negative coefficients", () => {
        expect(poly_eval([-1, 2], 3)).toBe(5);
    });

    test("empty polynomial returns 0", () => {
        expect(poly_eval([], 5)).toBe(0);
    });
});

describe("poly_to_latex_low_to_high", () => {
    test("zero polynomial", () => {
        expect(poly_to_latex_low_to_high([0])).toBe("");
    });

    test("constant", () => {
        expect(poly_to_latex_low_to_high([5])).toBe("5");
    });

    test("linear", () => {
        expect(poly_to_latex_low_to_high([3, 2])).toBe("3+2x");
    });

    test("negative leading coefficient", () => {
        expect(poly_to_latex_low_to_high([1, 0, -1])).toContain("-");
    });

    test("coefficient of 1 is implicit", () => {
        expect(poly_to_latex_low_to_high([0, 1])).toBe("+x");
    });

    test("coefficient of -1 shows minus sign only", () => {
        expect(poly_to_latex_low_to_high([0, -1])).toBe("-x");
    });
});

describe("poly_to_latex_high_to_low", () => {
    test("quadratic in descending order", () => {
        const result = poly_to_latex_high_to_low([1, 2, 3]);
        expect(result).toMatch(/^3/);
        expect(result).toContain("+1");
    });
});
