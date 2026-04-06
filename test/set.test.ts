import { cartesian_product, EndoFunction, gen_monoid_from_endofuncs, set_eq, union_sets } from "../math/set";
import {
    generate_semigroup,
    get_definite_k,
    get_highest_idempotent_power,
    is_abelian,
    is_aperiodic,
    is_associative,
    is_group,
    is_monoid,
} from "../math/semigroup";
import { get_alphabet_from_strings } from "../lang/string";
import { range } from "../util";

describe("cartesian_product", () => {
    test("{1,2} x {3,4}", () => {
        expect(cartesian_product([[1, 2], [3, 4]])).toEqual([
            [1, 3], [1, 4], [2, 3], [2, 4],
        ]);
    });
});

describe("set union semigroup", () => {
    const letters = "abcd";
    const alphabet = get_alphabet_from_strings([letters]);
    const singletons: Set<string>[] = alphabet.map((c: string) => new Set([c]));
    const generators: Set<string>[] = Array.from(singletons);
    generators.push(new Set());

    test("union is associative on generators", () => {
        expect(is_associative(generators, union_sets, set_eq)).toBe(true);
    });

    const allSets = generate_semigroup(generators, union_sets, set_eq, 5000);

    test("generates 2^4 = 16 subsets", () => {
        expect(allSets.length).toBe(Math.pow(2, alphabet.length));
    });

    test("union is abelian", () => {
        expect(is_abelian(allSets, union_sets, set_eq)).toBe(true);
    });

    test("not definite", () => {
        expect(get_definite_k(allSets, union_sets, set_eq)).toBe(-1);
    });

    test("highest idempotent power is 1", () => {
        expect(get_highest_idempotent_power(allSets, union_sets, set_eq)).toBe(1);
    });

    test("aperiodic with period 1", () => {
        expect(is_aperiodic(allSets, union_sets, set_eq)).toBe(1);
    });

    test("identity element is the empty set", () => {
        const identity = is_monoid(allSets, union_sets, set_eq)[1];
        expect(identity).not.toBeNull();
        expect(identity!.size).toBe(0);
    });
});

describe("endofunction", () => {
    const underlyingSet = range(1, 5);

    test("identity endofunction forms a trivial group", () => {
        const identity = new EndoFunction<number>(underlyingSet, underlyingSet, "");
        const result = is_group(
            [identity],
            (a, b) => a.multiply(b),
            (a, b) => a.eq(b),
        );
        expect(result[0]).toBe(true);
    });

    test("shift + swap generates S_4 (order 24)", () => {
        const shiftOne = new EndoFunction<number>(underlyingSet, [2, 3, 4, 1], "a");
        const swap12 = new EndoFunction<number>(underlyingSet, [2, 1, 3, 4], "b");
        const group = gen_monoid_from_endofuncs([shiftOne, swap12]);
        expect(group.length).toBe(24);

        const result = is_group(
            group,
            (a, b) => a.multiply(b),
            (a, b) => a.eq(b),
        );
        expect(result[0]).toBe(true);
    });
});
