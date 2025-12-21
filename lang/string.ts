import {cartesian_product} from "../math/set.js";
import {get_sup} from "../util.js";
import {generate_semigroup} from "../math/semigroup.js";
import {array_eq} from "../math/math.js";

export function get_all_prefixes(str: string): string[] {
    const prefixes: string[] = [];
    for (let i = 0; i <= str.length; i++) {
        prefixes.push(str.substring(0, i));
    }
    return prefixes;
}

export function get_all_chars(s: string): string[] {
    return Array.from(new Set(s.split(""))).sort();
}

export function get_alphabet_from_strings(strs: string[]) {
    return Array.from(new Set(strs.map(get_all_chars).flat()))
}

export function concat_string_lists(s1: string[], s2: string[]): string[] {
    return Array.from(new Set(cartesian_product([s1, s2]).map(
        (s) => s[0] + s[1]))).sort((a, b) => (a.length - b.length) || a.localeCompare(b));
}

// TODO play with power
export function concat_same_string_lists_n_times(s1: string[], n: number) {
    let result = Array.from(s1)
    for (let i = 1; i < n; i++) {
        result = concat_string_lists(result, s1)
    }
    return result
}

export function concat_same_string_lists_leq_n_times(s1: string[], n: number) {
    let s_with_empty = Array.from(s1).concat("")
    return concat_same_string_lists_n_times(s_with_empty, n)
}

//TODO empty handling
export function get_sub_seq_regex(s: string) {
    let sigma_star = "&Sigma;" + get_sup("*")
    return sigma_star + s.split("").join(sigma_star) + sigma_star
}

export function get_regex_for_disallowed_sub_seq(s: string[]) {
    return "(" + s.map(get_sub_seq_regex).join("&cup;") + ")" + get_sup("<mi>c</mi>")
}

export function sub_empty_with_ep(str: string) {
    if (str.length == 0) {
        return "ε"
    }
    return str;
}

let NO_LENGTH_LIMIT = -1

export function cat_subseq_leq_q(s1: string[], s2: string[], k: number = -1) {
    return concat_string_lists(s1, s2).filter(s => (k == NO_LENGTH_LIMIT || s.length <= k))
}

export function cat_subseq_of_blocklist(s1: string[], s2: string[], k: number, blocks: string[]) {
    return cat_subseq_leq_q(s1, s2, k).filter(s => blocks.some(block => is_sub_seq(block, s)))
}

export function get_all_subseq_leq_k(str: string, k: number = -1): string[] {
    if (str.length == 0) {
        return [""]
    }
    if (str.length == 1) {
        return (k == NO_LENGTH_LIMIT || k >= 1) ? [str, ""] : [""]
    }

    let half = Math.trunc(str.length / 2)
    let first_half = get_all_subseq_leq_k(str.substring(0, half), k)
    let second_half = get_all_subseq_leq_k(str.substring(half, str.length), k)
    return cat_subseq_leq_q(first_half, second_half, k)
}

export function get_subseq_of_blocklist(str: string, blocks: string[]): string[] {
    let k = Math.max(...(blocks.map(s => s.length)))
    if (str.length <= 1) {
        return get_all_subseq_leq_k(str).filter(s => blocks.some(block => is_sub_seq(block, s)))
    }

    let half = Math.trunc(str.length / 2)
    let first_half = get_subseq_of_blocklist(str.substring(0, half), blocks)
    let second_half = get_subseq_of_blocklist(str.substring(half, str.length), blocks)
    return cat_subseq_of_blocklist(first_half, second_half, k, blocks)
}

export function is_sub_seq(str: string, pattern: string) {
    if (str.length < pattern.length) {
        return false
    }
    if (pattern.length == 0) {
        return true;
    }

    // TODO for loop instead of recursive
    if (str.charAt(0) == pattern.charAt(0)) {
        return is_sub_seq(str.substring(1, str.length), pattern.substring(1, pattern.length));
    } else {
        return is_sub_seq(str.substring(1, str.length), pattern);
    }
}

export function get_all_subseq_for_blocks(blocks: string[]) {
    let alphabet: string[] = get_alphabet_from_strings(blocks)
    let k = Math.max(...(blocks.map(s => s.length)))
    let generators: string[][] = alphabet.map(x => [x, ""])
    generators.push([""])
    let concat = (s1: string[], s2: string[]) =>
        cat_subseq_of_blocklist(s1, s2, k, blocks)
    let all_subs = generate_semigroup(generators, concat, array_eq)
    return all_subs
}

export function subseq_remove_short(subs: string[]) {
    return subs.filter(x => !subs.some(y => (y.length > x.length && is_sub_seq(y, x))))
}