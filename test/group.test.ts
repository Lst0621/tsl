import { gen_general_linear_n_zm, get_primitive_roots, get_u_n } from "../math/group";
import { generate_semigroup, is_group } from "../math/semigroup";
import { array_eq_2d } from "../math/math";
import {
    matrix_inverse,
    matrix_multiply_zn,
} from "../math/matrix";
import {
    get_add_inverse_mod_n_function,
    get_add_mod_n_function,
    get_mul_inverse_mod_n_function,
    get_multiply_mod_n_function,
} from "../math/number";

describe("GL(n, Z_m)", () => {
    test("|GL(2, Z_2)| = 6", () => {
        expect(gen_general_linear_n_zm(2, 2).length).toBe(6);
    });

    test("GL(2, Z_2) is closed under multiplication (forms a group)", () => {
        const gl = gen_general_linear_n_zm(2, 2);
        const mul = (a: number[][], b: number[][]) => matrix_multiply_zn(a, b, 2);
        const sg = generate_semigroup(gl, mul, array_eq_2d, 20);
        expect(sg.length).toBe(gl.length);
        expect(is_group(gl, mul, array_eq_2d)[0]).toBe(true);
    });

    test("|GL(3, Z_2)| = 168 and forms a group", () => {
        const gl = gen_general_linear_n_zm(3, 2);
        expect(gl.length).toBe(168);
        const mul = (a: number[][], b: number[][]) => matrix_multiply_zn(a, b, 2);
        expect(is_group(gl, mul, array_eq_2d)[0]).toBe(true);
    });

    test("|GL(2, Z_3)| = 48 and every element has an inverse", () => {
        const gl = gen_general_linear_n_zm(2, 3);
        expect(gl.length).toBe(48);

        const identity2 = [[1, 0], [0, 1]];
        for (let i = 0; i < gl.length; i++) {
            const mat = gl[i];
            const inv = matrix_inverse(
                mat,
                get_add_mod_n_function(3),
                get_multiply_mod_n_function(3),
                get_add_inverse_mod_n_function(3),
                get_mul_inverse_mod_n_function(3),
            );
            expect(matrix_multiply_zn(mat, inv, 3)).toEqual(identity2);
        }
    });

    test("|GL(3, Z_3)| = 11232 and sampled inverses are correct", () => {
        const gl = gen_general_linear_n_zm(3, 3);
        expect(gl.length).toBe(11232);

        const identity3 = [[1, 0, 0], [0, 1, 0], [0, 0, 1]];
        const step = 40;
        for (let i = 0; step * i < gl.length; i++) {
            const mat = gl[step * i];
            const inv = matrix_inverse(
                mat,
                get_add_mod_n_function(3),
                get_multiply_mod_n_function(3),
                get_add_inverse_mod_n_function(3),
                get_mul_inverse_mod_n_function(3),
            );
            expect(matrix_multiply_zn(mat, inv, 3)).toEqual(identity3);
        }
    });
});

describe("primitive roots", () => {
    test("for n=2..10, each primitive root generates U(n)", () => {
        for (let n = 2; n <= 10; n++) {
            const u = get_u_n(n);
            const roots = get_primitive_roots(n);
            for (const root of roots) {
                const g = generate_semigroup(
                    [root],
                    get_multiply_mod_n_function(n),
                    (a: number, b: number) => a === b,
                    n,
                );
                expect(g.length).toBe(u.length);
            }
        }
    });
});
