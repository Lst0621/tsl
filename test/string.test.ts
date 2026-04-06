import { get_alphabet_from_strings, get_concat_and_suffix_func } from "../lang/string";
import {
    generate_semigroup,
    get_all_idempotent_elements,
    get_definite_k,
    get_highest_idempotent_power,
    is_abelian,
    is_group,
    is_monoid,
} from "../math/semigroup";
import { equals } from "../func";

describe("definite k-suffix semigroup", () => {
    const alphabet = get_alphabet_from_strings(["abcd"]);
    const k = 3;
    const concatSuffix = get_concat_and_suffix_func(k);
    const strs = generate_semigroup(alphabet, concatSuffix, equals);

    test("highest idempotent power equals k", () => {
        expect(get_highest_idempotent_power(strs, concatSuffix, equals)).toBe(k);
    });

    test("number of idempotent elements is |alphabet|^k", () => {
        const idempotents = get_all_idempotent_elements(strs, concatSuffix, equals);
        expect(idempotents.length).toBe(Math.pow(alphabet.length, k));
    });

    test("definite k equals k", () => {
        expect(get_definite_k(strs, concatSuffix, equals)).toBe(k);
    });

    test("not abelian", () => {
        expect(is_abelian(strs, concatSuffix, equals)).toBe(false);
    });

    test("no identity element (not a monoid)", () => {
        const identity = is_monoid(strs, concatSuffix, equals)[1];
        expect(identity).toBeNull();
    });

    test("not a group", () => {
        expect(is_group(strs, concatSuffix, equals)[0]).toBe(false);
    });
});
